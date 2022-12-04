#pragma once

namespace Console
{
	using EventResult = RE::BSEventNotifyControl;

	namespace util
	{
		std::string GetClipboardText();

		RE::GFxMovie* GetConsoleMovie();

		std::string GetVariableString(const RE::GFxMovie* a_movie, const char* a_path);
        std::size_t GetVariableInt(const RE::GFxMovie* a_movie, const char* a_path);
	}

	class Manager final :
		public RE::BSTEventSink<RE::MenuOpenCloseEvent>,
		public RE::BSTEventSink<RE::InputEvent*>
	{
	public:
		static Manager* GetSingleton()
		{
			static Manager singleton;
			return std::addressof(singleton);
		}

        static void Register()
		{
			logger::info("{:*^30}", "EVENTS");

			if (const auto UI = RE::UI::GetSingleton()) {
				UI->AddEventSink<RE::MenuOpenCloseEvent>(GetSingleton());
				logger::info("Registered menu open/close event");
			}
		}
	private:
        static void SaveCommands();
        static void LoadCommands();

        EventResult ProcessEvent(const RE::MenuOpenCloseEvent* a_evn, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;
	    EventResult ProcessEvent(RE::InputEvent* const* a_evn, RE::BSTEventSource<RE::InputEvent*>*) override;

		Manager() = default;
		Manager(const Manager&) = delete;
		Manager(Manager&&) = delete;

		~Manager() override = default;

		Manager& operator=(const Manager&) = delete;
		Manager& operator=(Manager&&) = delete;

		bool keyCombo1{ false };
		bool keyCombo2{ false };

		std::once_flag onGameStart;
	};
}
