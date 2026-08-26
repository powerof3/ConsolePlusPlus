#pragma once

class Settings : public REX::TSingleton<Settings>
{
public:
	enum class PasteType : std::uint32_t
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

	[[nodiscard]] Key  GetPrimaryKey() const { return static_cast<Key>(primaryKey.GetValue()); }
	[[nodiscard]] Key  GetSecondaryKey() const { return static_cast<Key>(secondaryKey.GetValue()); }
	[[nodiscard]] bool PasteAtEnd() const { return pasteType.GetValue() == std::to_underlying(PasteType::kEndOfText); }

	// members
	static constexpr auto configPath = R"(Data\SKSE\Plugins\po3_ConsolePlusPlus.ini)";

	REX::TIniSetting<bool> enableCopyPaste{ "Settings", "bCopyPaste", true };
	REX::TIniSetting<bool> enableConsoleHistory{ "Settings", "bCacheConsoleHistory", true };

	REX::TIniSetting<std::uint32_t> consoleHistoryLimit{ "Settings", "iConsoleHistoryLimit", 50 };
	REX::TIniSetting<bool>          allowDuplicateHistory{ "Settings", "bAllowDuplicateConsoleHistory", true };

	REX::TIniSetting<std::uint32_t> primaryKey{ "CopyPaste", "iPrimaryKey", std::to_underlying(Key::kLeftControl) };
	REX::TIniSetting<std::uint32_t> secondaryKey{ "CopyPaste", "iSecondaryKey", std::to_underlying(Key::kV) };
	REX::TIniSetting<std::uint32_t> pasteType{ "CopyPaste", "iPasteType", std::to_underlying(PasteType::kCursor) };
	REX::TIniSetting<std::uint32_t> inputDelay{ "CopyPaste", "iInputDelay", 10 };

	std::optional<std::filesystem::path> consoleHistoryPath{};
	std::vector<std::string>             consoleHistoryEntries{};

private:
	void UpdateINISettings() const;
};
