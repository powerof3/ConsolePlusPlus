#include "Settings.h"

Settings* Settings::GetSingleton()
{
	static Settings singleton;
	return std::addressof(singleton);
}

Settings::Settings()
{
	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	ini::get_value(ini, enableCopyPaste, "Settings", "Copy Paste", ";Copy text from clipboard and paste into console");
	ini::get_value(ini, enableCommandCache, "Settings", "Cache Commands", ";Cache commands between game instances");

	ini::get_value(ini, primaryKey, "CopyPaste", "Primary Key", ";Keyboard scan codes : https://wiki.nexusmods.com/index.php/DirectX_Scancodes_And_How_To_Use_Them\n;Default: Left Ctrl");
	ini::get_value(ini, secondaryKey, "CopyPaste", "Secondary Key", ";Default: V");
	ini::get_value(ini, pasteType, "CopyPaste", "Paste Type", ";0 - insert text at cursor position | 1 - append text");
	ini::get_value(ini, inputDelay, "CopyPaste", "Input delay", ";Delay between key press and text paste (in milliseconds)");

	(void)ini.SaveFile(path);
}

void Settings::SaveCommands(const std::vector<std::string>& a_commands) const
{
	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	ini.Delete("ConsoleCommands", nullptr);
	if (!a_commands.empty()) {
		for (std::uint32_t i = 0; i < a_commands.size(); i++) {
			ini.SetValue("ConsoleCommands", fmt::format("Command{}", i).c_str(), a_commands[i].c_str());
		}
	}

	(void)ini.SaveFile(path);
}

std::vector<std::string> Settings::LoadCommands() const
{
	std::vector<std::string> commands;

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	if (const auto section = ini.GetSection("ConsoleCommands"); section && !section->empty()) {
		for (const auto& entry : *section | std::views::values) {
			commands.emplace_back(entry);
		}
	}

	return commands;
}

void Settings::ClearCommands() const
{
	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path);

	ini.Delete("ConsoleCommands", nullptr);

	(void)ini.SaveFile(path);
}
