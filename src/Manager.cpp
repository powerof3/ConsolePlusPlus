#include "Manager.h"
#include "Settings.h"

namespace Console
{
	namespace detail
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
			const auto UI = RE::UI::GetSingleton();
			const auto consoleMenu = UI ? UI->GetMenu<RE::Console>() : nullptr;

			return consoleMenu ? consoleMenu->uiMovie.get() : nullptr;
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

	void Manager::Register()
	{
		logger::info("{:*^30}", "HOOKS");

		Clear::Install();

		logger::info("{:*^30}", "EVENTS");

		if (const auto UI = RE::UI::GetSingleton()) {
			UI->AddEventSink<RE::MenuOpenCloseEvent>(GetSingleton());
			logger::info("Registered menu open/close event");
		}
	}

	void Manager::SaveConsoleHistory()
	{
		if (const auto consoleMovie = detail::GetConsoleMovie()) {
			RE::GFxValue commandsVal;
			consoleMovie->GetVariable(&commandsVal, "_global.Console.ConsoleInstance.Commands");

			if (commandsVal.IsArray()) {
				const auto size = commandsVal.GetArraySize();
				if (size == 0) {
					Settings::GetSingleton()->ClearConsoleHistoryFromFile();
					return;
				}

				logger::info("Saving console history to file ({} entries)", size);

				Settings::GetSingleton()->SaveConsoleHistoryToFile(commandsVal);
			}
		}
	}

	void Manager::LoadConsoleHistory()
	{
		if (!loadedConsoleHistory) {
			loadedConsoleHistory = true;

			SKSE::GetTaskInterface()->AddUITask([] {
				const std::vector<std::string> commands = Settings::GetSingleton()->GetConsoleHistory();

				if (commands.empty()) {
					logger::info("Console history not found...");
					return;
				}

				if (const auto consoleMovie = detail::GetConsoleMovie()) {
					logger::info("Loading console history from file...");

					RE::GFxValue commandsVal;
					consoleMovie->GetVariable(&commandsVal, "_global.Console.ConsoleInstance.Commands");

					if (commandsVal.IsArray()) {
						commandsVal.ClearElements();
						for (auto& command : commands) {
							RE::GFxValue element(command);
							commandsVal.PushBack(element);
						}

						consoleMovie->SetVariable("_global.Console.ConsoleInstance.Commands", commandsVal, RE::GFxMovie::SetVarType::kNormal);
					}
				}
			});
		}
	}

	void Manager::ClearConsoleHistory()
	{
		SKSE::GetTaskInterface()->AddUITask([] {
			if (const auto consoleMovie = detail::GetConsoleMovie()) {
				logger::info("Clearing console history...");

				RE::GFxValue commandsVal;
				consoleMovie->GetVariable(&commandsVal, "_global.Console.ConsoleInstance.Commands");

				if (commandsVal.IsArray()) {
					commandsVal.ClearElements();
					Settings::GetSingleton()->ClearConsoleHistoryFromFile();
				}
			}
		});
	}

	RE::BSEventNotifyControl Manager::ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (!a_evn || a_evn->menuName != RE::Console::MENU_NAME) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (const auto inputMgr = RE::BSInputDeviceManager::GetSingleton()) {
			keyCombo1 = false;
			keyCombo2 = false;

			const auto settings = Settings::GetSingleton();

			if (a_evn->opening) {
				if (settings->enableConsoleHistory) {
					LoadConsoleHistory();
				}
				if (settings->enableCopyPaste) {
					inputMgr->AddEventSink(GetSingleton());
				}
			} else {
				if (settings->enableConsoleHistory) {
					SKSE::GetTaskInterface()->AddUITask([] {
						SaveConsoleHistory();
					});
				}
				if (settings->enableCopyPaste) {
					inputMgr->RemoveEventSink(GetSingleton());
				}
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl Manager::ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*)
	{
		if (!a_evn || keyCombo1 && keyCombo2) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto settings = Settings::GetSingleton();
		bool       pasteAtEnd = settings->pasteType == Settings::PasteType::kEndOfText;

		for (auto event = *a_evn; event; event = event->next) {
			if (const auto button = event->AsButtonEvent()) {
				const auto key = static_cast<RE::BSKeyboardDevice::Keys::Key>(button->GetIDCode());
				if (key == settings->primaryKey) {  // hold left shift
					if (button->IsHeld()) {
						keyCombo1 = true;
					} else {
						keyCombo1 = false;
					}
				}
				if (keyCombo1 && key == settings->secondaryKey && button->IsDown()) {  // wait for V to be down, not pressed!
					keyCombo2 = true;
				}
			}
		}

		if (keyCombo1 && keyCombo2) {
			if (const auto clipboardText = detail::GetClipboardText(); !clipboardText.empty()) {
				std::jthread thread([this, settings, pasteAtEnd, clipboardText] {
					RE::BSInputDeviceManager::GetSingleton()->RemoveEventSink(GetSingleton());

					// delay until V has been inputted
					std::this_thread::sleep_for(std::chrono::milliseconds(settings->inputDelay));

					SKSE::GetTaskInterface()->AddUITask([this, pasteAtEnd, clipboardText] {
						if (const auto consoleMovie = detail::GetConsoleMovie()) {
							// get old text
							std::string oldText = detail::GetVariableString(consoleMovie, "_global.Console.ConsoleInstance.CommandEntry.text");
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
								bool        appended{ false };
								// get cursor position
								const auto cursorPos = detail::GetVariableInt(consoleMovie, "_global.Console.ConsoleInstance.CommandEntry.caretIndex");
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

		return RE::BSEventNotifyControl::kContinue;
	}

    namespace Clear
    {
		bool ClearHistory(const RE::SCRIPT_PARAMETER*, RE::SCRIPT_FUNCTION::ScriptData*, RE::TESObjectREFR*, RE::TESObjectREFR*, RE::Script*, RE::ScriptLocals*, double&, std::uint32_t&)
		{
			Manager::ClearConsoleHistory();
			return true;
		}

		void Install()
		{
			constexpr auto LONG_NAME = "ClearConsoleHistory"sv;
			constexpr auto SHORT_NAME = "ClearHistory"sv;

		    if (const auto function = RE::SCRIPT_FUNCTION::LocateConsoleCommand("ToggleContextOverlay"); function) {
				function->functionName = LONG_NAME.data();
				function->shortName = SHORT_NAME.data();
			    function->executeFunction = &ClearHistory;
				logger::debug("installed ClearConsole hook");
			}
		}
    }
}
