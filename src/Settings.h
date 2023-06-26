#pragma once

class Settings
{
public:
	enum class PasteType
	{
		kCursor,
		kEndOfText
	};

	[[nodiscard]] static Settings* GetSingleton();

	void                                   SaveCommands(std::vector<std::string>& a_commands) const;
	[[nodiscard]] std::vector<std::string> LoadCommands() const;
	void                                   ClearCommands() const;

	bool enableCopyPaste{ true };
	bool enableCommandCache{ true };

	Key           primaryKey{ Key::kLeftControl };
	Key           secondaryKey{ Key::kV };
	PasteType     pasteType{ PasteType::kCursor };
	std::uint32_t inputDelay{ 10 };

	std::uint32_t commandHistoryLimit{ 50 };

private:
	Settings();

	// members
	const wchar_t* path{ L"Data/SKSE/Plugins/po3_ConsolePlusPlus.ini" };
};
