#pragma once

class Settings : public ISingleton<Settings>
{
public:
	enum class PasteType
	{
		kCursor,
		kEndOfText
	};

	void LoadSettings();

	[[nodiscard]] const std::vector<std::string>& GetConsoleHistory();

	void LoadConsoleHistoryFromFile();
	void LoadOldConsoleHistoryFromFile();
	void ClearConsoleHistoryFromFile() const;
	void RemoveDuplicateHistory();

	void SaveConsoleHistoryToFile(const RE::GFxValue& a_consoleHistoryVal);

	// members
	bool enableCopyPaste{ true };
	bool enableConsoleHistory{ true };

	Key           primaryKey{ Key::kLeftControl };
	Key           secondaryKey{ Key::kV };
	PasteType     pasteType{ PasteType::kCursor };
	std::uint32_t inputDelay{ 10 };

	std::uint32_t                        consoleHistoryLimit{ 50 };
	bool                                 allowDuplicateHistory{ true };
	std::optional<std::filesystem::path> consoleHistoryPath{};
	std::vector<std::string>             consoleHistoryEntries{};

	const wchar_t* configPath{ L"Data/SKSE/Plugins/po3_ConsolePlusPlus.ini" };
};
