#pragma once

namespace Console
{
	namespace detail
	{
		std::string GetClipboardText();

		RE::GFxMovie* GetConsoleMovie();

		std::string GetVariableString(const RE::GFxMovie* a_movie, const char* a_path);
		std::size_t GetVariableInt(const RE::GFxMovie* a_movie, const char* a_path);
	}

	class Manager final :
		public REX::Singleton<Manager>,
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
