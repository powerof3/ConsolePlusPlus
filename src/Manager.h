#pragma once

namespace Console
{
	namespace detail
	{
		std::string GetClipboardText();

		RE::GFxMovie* GetConsoleMovie();
		bool GetConsoleCaretIndex(RE::GFxMovie* movie, double& caretIndexOut);
		bool CreateStringObject(RE::GFxMovie* movie, std::string_view text, RE::GFxValue& objectOut);
		bool GetStringObjectLength(const RE::GFxValue& object, double& lengthOut);
		bool GetStringObjectSubStr(RE::GFxValue& object, double start, double length, std::string& textOut);

		std::string GetVariableString(const RE::GFxMovie* a_movie, const char* a_path);
		std::size_t GetVariableInt(const RE::GFxMovie* a_movie, const char* a_path);
	}

	class Manager final :
		public REX::TSingleton<Manager>,
		public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
		public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		static void Register();
		static void ClearConsoleHistory();

	private:
		static void SaveConsoleHistory();
		void        LoadConsoleHistory();

		RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;
		RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*) override;

		bool PasteText(RE::GFxMovie* consoleMovie, const std::string& clipboardText, bool pasteAtEnd);

		// members
		bool keyCombo1{ false };
		bool keyCombo2{ false };

		bool loadedConsoleHistory{ false };
	};

	namespace Clear
	{
		void Install();
	}
}
