#include "Manager.h"
#include "Settings.h"

namespace Console
{
	namespace util
	{
		// https://stackoverflow.com/a/14763025
		std::string GetClipboardText()
		{
			std::string text;

			// Try opening the clipboard
			if (!IsClipboardFormatAvailable(CF_TEXT) || !OpenClipboard(nullptr)) {
				return text;
			}

			// Get handle of clipboard object for ANSI text
			const HANDLE hData = GetClipboardData(CF_TEXT);
			if (hData == nullptr) {
				return text;
			}

			// Lock the handle to get the actual text pointer
			if (const auto pszText = static_cast<const char*>(GlobalLock(hData)); pszText != nullptr) {
				text = pszText;
			}

			// Release the lock
			GlobalUnlock(hData);

			// Release the clipboard
			CloseClipboard();

			return text;
		}

		RE::GFxMovie* GetConsoleMovie()
		{
			if (const auto UI = RE::UI::GetSingleton()) {
				if (const auto consoleMenu = UI->GetMenu<RE::Console>()) {
					if (const auto& consoleMovie = consoleMenu->uiMovie) {
						return consoleMovie.get();
					}
				}
			}
			return nullptr;
		}

		std::string GetVariableString(const RE::GFxMovie* a_movie, const char* a_path)
		{
			RE::GFxValue textVar;
			a_movie->GetVariable(&textVar, a_path);
			return textVar.GetString();
		}

		std::size_t GetVariableInt(const RE::GFxMovie* a_movie, const char* a_path)
		{
			RE::GFxValue intVar;
			a_movie->GetVariable(&intVar, a_path);
			return intVar.GetUInt();
		}
	}

	void Manager::SaveCommands()
	{
		if (const auto consoleMovie = util::GetConsoleMovie()) {
			// MIC's Clear command does not clear command array

		    /*const auto commandHistory = util::GetVariableString(consoleMovie, "_global.Console.ConsoleInstance.CommandHistory.text");
			if (commandHistory.empty()) {
				Settings::GetSingleton()->ClearCommands();
				return;
			}*/ 

		    RE::GFxValue commandsVal;
			consoleMovie->GetVariable(&commandsVal, "_global.Console.ConsoleInstance.Commands");

			if (commandsVal.IsArray()) {
				const auto size = commandsVal.GetArraySize();
				if (size == 0) {
					Settings::GetSingleton()->ClearCommands();
					return;
				}
				std::vector<std::string> commands;
				commands.reserve(size);

				RE::GFxValue commandsElement;
				for (std::uint32_t i = 0; i < size; i++) {
					commandsVal.GetElement(i, &commandsElement);
					if (commandsElement.IsString()) {
						commands.emplace_back(commandsElement.GetString());
					}
				}

				Settings::GetSingleton()->SaveCommands(commands);
			}
		}
	}

	void Manager::LoadCommands()
	{
		if (const auto consoleMovie = util::GetConsoleMovie()) {
			const std::vector<std::string> commands = Settings::GetSingleton()->LoadCommands();

			if (commands.empty()) {
				return;
			}

			RE::GFxValue commandsVal;
			consoleMovie->GetVariable(&commandsVal, "_global.Console.ConsoleInstance.Commands");

			if (commandsVal.IsArray()) {
				const auto size = commands.size();
				commandsVal.SetArraySize(static_cast<std::uint32_t>(size));

				for (std::uint32_t i = 0; i < size; i++) {
					RE::GFxValue element(commands[i]);
					commandsVal.SetElement(i, element);
				}

				consoleMovie->SetVariable("_global.Console.ConsoleInstance.Commands", commandsVal, RE::GFxMovie::SetVarType::kNormal);
			}
		}
	}

	EventResult Manager::ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (!a_evn || a_evn->menuName != RE::Console::MENU_NAME) {
			return EventResult::kContinue;
		}

		if (const auto inputMgr = RE::BSInputDeviceManager::GetSingleton()) {
			keyCombo1 = false;
			keyCombo2 = false;

			const auto settings = Settings::GetSingleton();

			if (a_evn->opening) {
				std::call_once(onGameStart, [&] {
					if (settings->enableCommandCache) {
						logger::info("Loading cached commands from file...");
						SKSE::GetTaskInterface()->AddUITask([] {
							LoadCommands();
						});
					}
				});
				if (settings->enableCopyPaste) {
					inputMgr->AddEventSink(GetSingleton());
				}
			} else {
				if (settings->enableCommandCache) {
					logger::info("Saving commands to file...");
					SKSE::GetTaskInterface()->AddUITask([] {
						SaveCommands();
					});
				}
				if (settings->enableCopyPaste) {
					inputMgr->RemoveEventSink(GetSingleton());
				}
			}
		}

		return EventResult::kContinue;
	}

	EventResult Manager::ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*)
	{
		using InputType = RE::INPUT_EVENT_TYPE;

		if (!a_evn || keyCombo1 && keyCombo2) {
			return EventResult::kContinue;
		}

		const auto settings = Settings::GetSingleton();
		bool pasteAtEnd = settings->pasteType == Settings::PasteType::kEndOfText;

		for (auto event = *a_evn; event; event = event->next) {
			if (const auto button = event->AsButtonEvent(); button) {
				const auto key = static_cast<RE::BSKeyboardDevice::Keys::Key>(button->GetIDCode());
				if (key == settings->primaryKey && button->IsHeld()) {  // hold left shift
					keyCombo1 = true;
				}
				if (keyCombo1 && key == settings->secondaryKey && button->IsDown()) {  // wait for V to be down, not pressed!
					keyCombo2 = true;
				}
			}
		}

		if (keyCombo1 && keyCombo2) {
			if (const auto clipboardText = util::GetClipboardText(); !clipboardText.empty()) {
				std::jthread thread([this, settings, pasteAtEnd, clipboardText] {
					RE::BSInputDeviceManager::GetSingleton()->RemoveEventSink(GetSingleton());

					// delay until V has been inputted
					std::this_thread::sleep_for(std::chrono::milliseconds(settings->inputDelay));

					SKSE::GetTaskInterface()->AddUITask([this, pasteAtEnd, clipboardText] {
						if (const auto consoleMovie = util::GetConsoleMovie()) {
							// get old text
							std::string oldText = util::GetVariableString(consoleMovie, "_global.Console.ConsoleInstance.CommandEntry.text");
							// paste
							if (pasteAtEnd) {
								// append new text to old
								// erase key
								auto newText = oldText + clipboardText;
								if (!oldText.empty()) {
									newText.erase(oldText.length() - 1, 1);
								}
								// overwrite command text
								consoleMovie->SetVariable("_global.Console.ConsoleInstance.CommandEntry.text", newText.c_str());
								// move cursor to end of text
								const RE::GFxValue args[2]{ newText.length(), newText.length() };
								consoleMovie->Invoke("Selection.setSelection", nullptr, args, 2);
							} else {
								// erase text if only key pressed
								if (oldText.size() == 1) {
									oldText.erase(0, 1);
								}
								std::string newText = oldText;
								bool appended{ false };
								// get cursor position
								const auto cursorPos = util::GetVariableInt(consoleMovie, "_global.Console.ConsoleInstance.CommandEntry.caretIndex");
								// insert at cursor pos
								if (oldText.size() > cursorPos) {
									appended = false;
									newText.insert(cursorPos, clipboardText);
									newText.erase(cursorPos - 1, 1);
								} else {  // or append if cursor is at end
									appended = true;
									newText += clipboardText;
									if (!oldText.empty()) {
										newText.erase(oldText.length() - 1, 1);
									}
								}
								//	overwrite command text
								consoleMovie->SetVariable("_global.Console.ConsoleInstance.CommandEntry.text", newText.c_str());
								// move cursor
								const auto index =
									appended ?
										newText.length() :
										cursorPos + (clipboardText.length() - 1);
								const RE::GFxValue args[2]{ index, index };
								consoleMovie->Invoke("Selection.setSelection", nullptr, args, 2);
							}
						}

						RE::BSInputDeviceManager::GetSingleton()->AddEventSink(GetSingleton());
					});
				});
				thread.detach();
			}
			keyCombo1 = false;
			keyCombo2 = false;
		}

		return EventResult::kContinue;
	}
}
