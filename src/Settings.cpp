#include "Settings.h"

void Settings::LoadSettings()
{
	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(configPath);

	// 1.4.0 -  delete old settings
	if (ini.GetLongValue("Settings", "Command History Limit", -1) != -1) {
		ini.Delete("Settings", nullptr);
		ini.Delete("CopyPaste", nullptr);
	}

	ini::get_value(ini, enableCopyPaste, "Settings", "bCopyPaste", ";Copy text from clipboard and paste into console");
	ini::get_value(ini, enableConsoleHistory, "Settings", "bCacheConsoleHistory", ";Cache console entries between game instances");

	ini::get_value(ini, consoleHistoryLimit, "Settings", "iConsoleHistoryLimit", ";Number of console entries to save\n;Default: 50");
	ini::get_value(ini, allowDuplicateHistory, "Settings", "bAllowDuplicateConsoleHistory", ";Save duplicate console entries");

	ini::get_value(ini, primaryKey, "CopyPaste", "iPrimaryKey", ";Keyboard scan codes : https://wiki.nexusmods.com/index.php/DirectX_Scancodes_And_How_To_Use_Them\n;Default: Left Ctrl");
	ini::get_value(ini, secondaryKey, "CopyPaste", "iSecondaryKey", ";Default: V");
	ini::get_value(ini, pasteType, "CopyPaste", "iPasteType", ";0 - insert text at cursor position | 1 - append text");
	ini::get_value(ini, inputDelay, "CopyPaste", "iInputDelay", ";Delay between key press and text paste (in milliseconds)");

	(void)ini.SaveFile(configPath);

	if (consoleHistoryPath = logger::log_directory(); consoleHistoryPath) {
		consoleHistoryPath->remove_filename();  // remove "/SKSE"
		consoleHistoryPath->append("SkyrimConsoleHistory.txt");
		if (!std::filesystem::exists(*consoleHistoryPath)) {
			std::ofstream ofs{ *consoleHistoryPath };
		}
	}
}

const std::vector<std::string>& Settings::GetConsoleHistory()
{
	if (consoleHistoryEntries.empty()) {
		LoadConsoleHistoryFromFile();
	}

	return consoleHistoryEntries;
}

void Settings::SaveConsoleHistoryToFile(const RE::GFxValue& a_consoleHistoryVal)
{
	if (consoleHistoryPath) {
		consoleHistoryEntries.clear();

		a_consoleHistoryVal.VisitMembers([&]([[maybe_unused]] const char* a_name, const RE::GFxValue& a_val) {
			if (a_val.IsString()) {
				consoleHistoryEntries.emplace_back(a_val.GetString());
			}
		});

		// clear previous contents
		std::ofstream output_file(*consoleHistoryPath, std::ios_base::out);
		if (!consoleHistoryEntries.empty()) {
			if (consoleHistoryEntries.size() > consoleHistoryLimit) {
				std::ranges::reverse(consoleHistoryEntries);
				consoleHistoryEntries.resize(consoleHistoryLimit);
				std::ranges::reverse(consoleHistoryEntries);
			}
			if (!allowDuplicateHistory) {
				RemoveDuplicateHistory();
			}
			const std::ostream_iterator<std::string> output_iterator(output_file, "\n");
			std::ranges::copy(consoleHistoryEntries, output_iterator);
		}
		output_file.close();
	}
}

void Settings::LoadConsoleHistoryFromFile()
{
	consoleHistoryEntries.clear();

	LoadOldConsoleHistoryFromFile();

	if (consoleHistoryEntries.empty()) {
		if (consoleHistoryPath) {
			std::ifstream input_file(*consoleHistoryPath, std::ios_base::in);
			if (input_file.is_open()) {
				std::string str;
				while (std::getline(input_file, str)) {
					if (!str.empty()) {
						consoleHistoryEntries.push_back(str);
					}
				}
			}
			input_file.close();
		}
	}

	if (consoleHistoryEntries.size() > consoleHistoryLimit) {
		std::ranges::reverse(consoleHistoryEntries);
		consoleHistoryEntries.resize(consoleHistoryLimit);
		std::ranges::reverse(consoleHistoryEntries);
	}

	if (!allowDuplicateHistory) {
		RemoveDuplicateHistory();
	}
}

void Settings::LoadOldConsoleHistoryFromFile()
{
	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(configPath);

	if (const auto section = ini.GetSection("ConsoleCommands"); section && !section->empty()) {
		// GetSection sorts by string order, smh SimpleINI
		std::map<int, std::string> commandMap;
		for (const auto& [key, entry] : *section) {
			commandMap.emplace(key.nOrder, entry);
		}
		for (const auto& entry : commandMap | std::views::values) {
			consoleHistoryEntries.emplace_back(entry);
		}
		ini.Delete("ConsoleCommands", nullptr);
		(void)ini.SaveFile(configPath);
	}
}

void Settings::ClearConsoleHistoryFromFile() const
{
	if (consoleHistoryPath) {
		std::ofstream output_file(*consoleHistoryPath, std::ios_base::out);
		output_file.close();
	}
}

void Settings::RemoveDuplicateHistory()
{
	std::unordered_map<std::string, size_t> last_occurrence;
	for (auto it = consoleHistoryEntries.rbegin(); it != consoleHistoryEntries.rend();) {
		const auto& current = *it;
		if (auto [duplicate, inserted] = last_occurrence.emplace(current, std::distance(consoleHistoryEntries.rbegin(), it)); !inserted) {
			it = decltype(it)(consoleHistoryEntries.erase(std::next(it).base()));
		} else {
			++it;
		}
	}
}
