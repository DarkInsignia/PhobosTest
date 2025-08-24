#include "Phobos.h"

#include <CCINIClass.h>
#include <ScenarioClass.h>
#include <SessionClass.h>
#include <MessageListClass.h>
#include <HouseClass.h>
#include <GameOptionsClass.h>

#include <Utilities/Parser.h>
#include <Utilities/GeneralUtils.h>
#include <Utilities/Patch.h>
#include <Utilities/Macro.h>

#include "Misc/BlittersFix.h"

bool Phobos::UI::DisableEmptySpawnPositions = false;
bool Phobos::UI::ExtendedToolTips = false;
int Phobos::UI::MaxToolTipWidth = 0;
bool Phobos::UI::HarvesterCounter_Show = false;
double Phobos::UI::HarvesterCounter_ConditionYellow = 0.99;
double Phobos::UI::HarvesterCounter_ConditionRed = 0.5;
bool Phobos::UI::ProducingProgress_Show = false;
const wchar_t* Phobos::UI::CostLabel = L"";
const wchar_t* Phobos::UI::PowerLabel = L"";
const wchar_t* Phobos::UI::PowerBlackoutLabel = L"";
const wchar_t* Phobos::UI::TimeLabel = L"";
const wchar_t* Phobos::UI::HarvesterLabel = L"";
const wchar_t* Phobos::UI::ShowBriefingResumeButtonLabel = L"";
const wchar_t* Phobos::UI::SWShotsFormat = L"";
const wchar_t* Phobos::UI::BattlePoints_Label = L"";
const wchar_t* Phobos::UI::BattlePointsSidebar_Label = L"";
bool Phobos::UI::BattlePointsSidebar_Label_InvertPosition = false;
bool Phobos::UI::BattlePointsSidebar_DisplayAsPercentage = false;
const wchar_t* Phobos::UI::CommanderPoints_Label = L"";
const wchar_t* Phobos::UI::CommanderPointsSidebar_Label = L"";
bool Phobos::UI::CommanderPointsSidebar_Label_InvertPosition = false;
bool Phobos::UI::CommanderPointsSidebar_HideLabel = false;
bool Phobos::UI::BattlePointsSidebar_Show = false;
bool Phobos::UI::CommanderPointsSidebar_Show = false;
char Phobos::UI::ShowBriefingResumeButtonStatusLabel[32];
bool Phobos::UI::PowerDelta_Show = false;
double Phobos::UI::PowerDelta_ConditionYellow = 0.75;
double Phobos::UI::PowerDelta_ConditionRed = 1.0;
bool Phobos::UI::CenterPauseMenuBackground = false;
bool Phobos::UI::SuperWeaponSidebar = false;
bool Phobos::UI::SuperWeaponSidebar_Pyramid = true;
int Phobos::UI::SuperWeaponSidebar_Interval = 0;
int Phobos::UI::SuperWeaponSidebar_LeftOffset = 0;
int Phobos::UI::SuperWeaponSidebar_CameoHeight = 48;
int Phobos::UI::SuperWeaponSidebar_Max = 0;
int Phobos::UI::SuperWeaponSidebar_MaxColumns = INT32_MAX;
bool Phobos::UI::WeedsCounter_Show = false;
bool Phobos::UI::AnchoredToolTips = false;

bool Phobos::Config::ToolTipDescriptions = true;
bool Phobos::Config::ToolTipBlur = false;
bool Phobos::Config::PrioritySelectionFiltering = true;
bool Phobos::Config::DevelopmentCommands = true;
bool Phobos::Config::SuperWeaponSidebarCommands = false;
bool Phobos::Config::ShowPlanningPath = false;
bool Phobos::Config::ArtImageSwap = false;
bool Phobos::Config::ShowPlacementPreview = false;
bool Phobos::Config::EnableSelectBox = false;
bool Phobos::Config::DigitalDisplay_Enable = false;
bool Phobos::Config::MessageApplyHoverState = false;
bool Phobos::Config::MessageDisplayInCenter = false;
bool Phobos::Config::RealTimeTimers = false;
bool Phobos::Config::RealTimeTimers_Adaptive = false;
int Phobos::Config::CampaignDefaultGameSpeed = 2;
bool Phobos::Config::SkirmishUnlimitedColors = false;
bool Phobos::Config::ShowDesignatorRange = false;
bool Phobos::Config::SaveVariablesOnScenarioEnd = false;
bool Phobos::Config::SaveGameOnScenarioStart = true;
bool Phobos::Config::ShowBriefing = true;
bool Phobos::Config::ShowHarvesterCounter = false;
bool Phobos::Config::ShowPowerDelta = true;
bool Phobos::Config::ShowWeedsCounter = false;
bool Phobos::Config::HideLightFlashEffects = true;
bool Phobos::Config::ShowFlashOnSelecting = false;
bool Phobos::Config::UnitPowerDrain = false;
int Phobos::Config::SuperWeaponSidebar_RequiredSignificance = 0;

bool Phobos::Misc::CustomGS = false;
int Phobos::Misc::CustomGS_ChangeInterval[7] = { -1, -1, -1, -1, -1, -1, -1 };
int Phobos::Misc::CustomGS_ChangeDelay[7] = { 0, 1, 2, 3, 4, 5, 6 };
int Phobos::Misc::CustomGS_DefaultDelay[7] = { 0, 1, 2, 3, 4, 5, 6 };

DEFINE_HOOK(0x5FACDF, OptionsClass_LoadSettings_LoadPhobosSettings, 0x5)
{
	const auto phobosSection = "Phobos";

	Phobos::Config::ToolTipDescriptions = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "ToolTipDescriptions", true);
	Phobos::Config::ToolTipBlur = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "ToolTipBlur", false);
	Phobos::Config::PrioritySelectionFiltering = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "PrioritySelectionFiltering", true);
	Phobos::Config::ShowPlacementPreview = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "ShowPlacementPreview", true);
	Phobos::Config::MessageApplyHoverState = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "MessageApplyHoverState", false);
	Phobos::Config::MessageDisplayInCenter = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "MessageDisplayInCenter", false);
	Phobos::Config::RealTimeTimers = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "RealTimeTimers", false);
	Phobos::Config::RealTimeTimers_Adaptive = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "RealTimeTimers.Adaptive", false);
	Phobos::Config::EnableSelectBox = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "EnableSelectBox", false);
	Phobos::Config::DigitalDisplay_Enable = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "DigitalDisplay.Enable", false);
	Phobos::Config::SaveGameOnScenarioStart = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "SaveGameOnScenarioStart", true);
	Phobos::Config::ShowBriefing = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "ShowBriefing", true);
	Phobos::Config::ShowPowerDelta = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "ShowPowerDelta", true);
	Phobos::Config::ShowHarvesterCounter = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "ShowHarvesterCounter", true);
	Phobos::Config::ShowWeedsCounter = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "ShowWeedsCounter", true);
	Phobos::Config::HideLightFlashEffects = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "HideLightFlashEffects", false);
	Phobos::Config::ShowFlashOnSelecting = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "ShowFlashOnSelecting", false);
	Phobos::Config::SuperWeaponSidebar_RequiredSignificance = CCINIClass::INI_RA2MD.ReadInteger(phobosSection, "SuperWeaponSidebar.RequiredSignificance", 0);

	// Custom game speeds, 6 - i so that GS6 is index 0, just like in the engine
	Phobos::Config::CampaignDefaultGameSpeed = 6 - CCINIClass::INI_RA2MD.ReadInteger(phobosSection, "CampaignDefaultGameSpeed", 4);
	if (Phobos::Config::CampaignDefaultGameSpeed > 6 || Phobos::Config::CampaignDefaultGameSpeed < 0)
	{
		Phobos::Config::CampaignDefaultGameSpeed = 2;
	}

	{
		const byte temp = (byte)Phobos::Config::CampaignDefaultGameSpeed;

		Patch::Apply_RAW(0x55D77A, { temp }); // We overwrite the instructions that force GameSpeed to 2 (GS4)
		Patch::Apply_RAW(0x55D78D, { temp }); // when speed control is off. Doesn't need a hook.
	}

	Phobos::Config::ShowDesignatorRange = CCINIClass::INI_RA2MD.ReadBool(phobosSection, "ShowDesignatorRange", false);

	CCINIClass ini_uimd {};
	ini_uimd.LoadFromFile(GameStrings::UIMD_INI);

	// LoadingScreen
	{
		Phobos::UI::DisableEmptySpawnPositions =
			ini_uimd.ReadBool("LoadingScreen", "DisableEmptySpawnPositions", false);
	}

	// ToolTips
	{
		Phobos::UI::ExtendedToolTips =
			ini_uimd.ReadBool(GameStrings::ToolTips, "ExtendedToolTips", false);

		Phobos::UI::AnchoredToolTips =
			ini_uimd.ReadBool(GameStrings::ToolTips, "AnchoredToolTips", false);

		Phobos::UI::MaxToolTipWidth =
			ini_uimd.ReadInteger(GameStrings::ToolTips, "MaxWidth", 0);

		ini_uimd.ReadString(GameStrings::ToolTips, "CostLabel", NONE_STR, Phobos::readBuffer);
		Phobos::UI::CostLabel = GeneralUtils::LoadStringOrDefault(Phobos::readBuffer, L"$");

		ini_uimd.ReadString(GameStrings::ToolTips, "PowerLabel", NONE_STR, Phobos::readBuffer);
		Phobos::UI::PowerLabel = GeneralUtils::LoadStringOrDefault(Phobos::readBuffer, L"\u26a1"); // ⚡

		ini_uimd.ReadString(GameStrings::ToolTips, "PowerBlackoutLabel", NONE_STR, Phobos::readBuffer);
		Phobos::UI::PowerBlackoutLabel = GeneralUtils::LoadStringOrDefault(Phobos::readBuffer, L"\u26a1\u274c"); // ⚡❌

		ini_uimd.ReadString(GameStrings::ToolTips, "TimeLabel", NONE_STR, Phobos::readBuffer);
		Phobos::UI::TimeLabel = GeneralUtils::LoadStringOrDefault(Phobos::readBuffer, L"\u231a"); // ⌚

		ini_uimd.ReadString(GameStrings::ToolTips, "SWShotsFormat", NONE_STR, Phobos::readBuffer);
		Phobos::UI::SWShotsFormat = GeneralUtils::LoadStringOrDefault(Phobos::readBuffer, L"Shots: %d"); // ⌚

		ini_uimd.ReadString(GameStrings::ToolTips, "BattlePoints.Label", NONE_STR, Phobos::readBuffer);
		Phobos::UI::BattlePoints_Label = GeneralUtils::LoadStringOrDefault(Phobos::readBuffer, L"\u2605: "); // ★:

		ini_uimd.ReadString(GameStrings::ToolTips, "CommanderPoints.Label", NONE_STR, Phobos::readBuffer);
		Phobos::UI::CommanderPoints_Label = GeneralUtils::LoadStringOrDefault(Phobos::readBuffer, L"\u2605: "); // ★:
	}

	// Sidebar
	{
		Phobos::UI::HarvesterCounter_Show =
			ini_uimd.ReadBool(SIDEBAR_SECTION, "HarvesterCounter.Show", false);

		ini_uimd.ReadString(SIDEBAR_SECTION, "HarvesterCounter.Label", NONE_STR, Phobos::readBuffer);
		Phobos::UI::HarvesterLabel = GeneralUtils::LoadStringOrDefault(Phobos::readBuffer, L"\u26cf"); // ⛏

		Phobos::UI::HarvesterCounter_ConditionYellow =
			ini_uimd.ReadDouble(SIDEBAR_SECTION, "HarvesterCounter.ConditionYellow", Phobos::UI::HarvesterCounter_ConditionYellow);

		Phobos::UI::HarvesterCounter_ConditionRed =
			ini_uimd.ReadDouble(SIDEBAR_SECTION, "HarvesterCounter.ConditionRed", Phobos::UI::HarvesterCounter_ConditionRed);

		Phobos::UI::WeedsCounter_Show =
			ini_uimd.ReadBool(SIDEBAR_SECTION, "WeedsCounter.Show", false);

		Phobos::UI::ProducingProgress_Show =
			ini_uimd.ReadBool(SIDEBAR_SECTION, "ProducingProgress.Show", false);

		Phobos::UI::PowerDelta_Show =
			ini_uimd.ReadBool(SIDEBAR_SECTION, "PowerDelta.Show", false);

		Phobos::UI::PowerDelta_ConditionYellow =
			ini_uimd.ReadDouble(SIDEBAR_SECTION, "PowerDelta.ConditionYellow", Phobos::UI::PowerDelta_ConditionYellow);

		Phobos::UI::PowerDelta_ConditionRed =
			ini_uimd.ReadDouble(SIDEBAR_SECTION, "PowerDelta.ConditionRed", Phobos::UI::PowerDelta_ConditionRed);

		Phobos::UI::CenterPauseMenuBackground =
			ini_uimd.ReadBool(SIDEBAR_SECTION, "CenterPauseMenuBackground", Phobos::UI::CenterPauseMenuBackground);

		Phobos::UI::SuperWeaponSidebar =
			ini_uimd.ReadBool(SIDEBAR_SECTION, "SuperWeaponSidebar", Phobos::UI::SuperWeaponSidebar);

		Phobos::UI::SuperWeaponSidebar_Pyramid =
			ini_uimd.ReadBool(SIDEBAR_SECTION, "SuperWeaponSidebar.Pyramid", Phobos::UI::SuperWeaponSidebar_Pyramid);

		Phobos::UI::SuperWeaponSidebar_Interval =
			ini_uimd.ReadInteger(SIDEBAR_SECTION, "SuperWeaponSidebar.Interval", Phobos::UI::SuperWeaponSidebar_Interval);

		Phobos::UI::SuperWeaponSidebar_LeftOffset =
			ini_uimd.ReadInteger(SIDEBAR_SECTION, "SuperWeaponSidebar.LeftOffset", Phobos::UI::SuperWeaponSidebar_LeftOffset);

		Phobos::UI::SuperWeaponSidebar_LeftOffset = std::min(Phobos::UI::SuperWeaponSidebar_Interval, Phobos::UI::SuperWeaponSidebar_LeftOffset);

		Phobos::UI::SuperWeaponSidebar_CameoHeight =
			ini_uimd.ReadInteger(SIDEBAR_SECTION, "SuperWeaponSidebar.CameoHeight", Phobos::UI::SuperWeaponSidebar_CameoHeight);

		Phobos::UI::SuperWeaponSidebar_CameoHeight = std::max(48, Phobos::UI::SuperWeaponSidebar_CameoHeight);

		Phobos::UI::SuperWeaponSidebar_Max =
			ini_uimd.ReadInteger(SIDEBAR_SECTION, "SuperWeaponSidebar.Max", Phobos::UI::SuperWeaponSidebar_Max);

		const int reserveHeight = 96;
		const int screenHeight = GameOptionsClass::Instance.ScreenHeight - reserveHeight;

		if (Phobos::UI::SuperWeaponSidebar_Max > 0)
			Phobos::UI::SuperWeaponSidebar_Max = std::min(Phobos::UI::SuperWeaponSidebar_Max, screenHeight / Phobos::UI::SuperWeaponSidebar_CameoHeight);
		else
			Phobos::UI::SuperWeaponSidebar_Max = screenHeight / Phobos::UI::SuperWeaponSidebar_CameoHeight;

		Phobos::UI::SuperWeaponSidebar_MaxColumns =
			ini_uimd.ReadInteger(SIDEBAR_SECTION, "SuperWeaponSidebar.MaxColumns", Phobos::UI::SuperWeaponSidebar_MaxColumns);

		Phobos::UI::BattlePointsSidebar_Show = ini_uimd.ReadBool(SIDEBAR_SECTION, "BattlePointsSidebar.Show", false);
		Phobos::UI::CommanderPointsSidebar_Show = ini_uimd.ReadBool(SIDEBAR_SECTION, "CommanderPointsSidebar.Show", false);

		Phobos::UI::BattlePointsSidebar_Label_InvertPosition = ini_uimd.ReadBool(SIDEBAR_SECTION, "BattlePointsSidebar.Label.InvertPosition", false);
		Phobos::UI::BattlePointsSidebar_DisplayAsPercentage = ini_uimd.ReadBool(SIDEBAR_SECTION, "BattlePointsSidebar.DisplayAsPercentage", false);
		Phobos::UI::CommanderPointsSidebar_Label_InvertPosition = ini_uimd.ReadBool(SIDEBAR_SECTION, "CommanderPointsSidebar.Label.InvertPosition", false);
		Phobos::UI::CommanderPointsSidebar_HideLabel = ini_uimd.ReadBool(SIDEBAR_SECTION, "CommanderPointsSidebar.HideLabel", false);

		ini_uimd.ReadString(SIDEBAR_SECTION, "BattlePointsSidebar.Label", NONE_STR, Phobos::readBuffer);
		Phobos::UI::BattlePointsSidebar_Label = GeneralUtils::LoadStringOrDefault(Phobos::readBuffer, L"\u2605"); // %d ★

		ini_uimd.ReadString(SIDEBAR_SECTION, "CommanderPointsSidebar.Label", NONE_STR, Phobos::readBuffer);
		Phobos::UI::CommanderPointsSidebar_Label = GeneralUtils::LoadStringOrDefault(Phobos::readBuffer, L"\u2605"); // %d ★
	}

	// UISettings
	{
		ini_uimd.ReadString(UISETTINGS_SECTION, "ShowBriefingResumeButtonLabel", "GUI:Resume", Phobos::readBuffer);
		Phobos::UI::ShowBriefingResumeButtonLabel = GeneralUtils::LoadStringOrDefault(Phobos::readBuffer, L"");

		ini_uimd.ReadString(UISETTINGS_SECTION, "ShowBriefingResumeButtonStatusLabel", "STT:BriefingButtonReturn", Phobos::readBuffer);
		strcpy_s(Phobos::UI::ShowBriefingResumeButtonStatusLabel, Phobos::readBuffer);
	}

	return 0;
}

DEFINE_HOOK(0x52D21F, InitRules_ThingsThatShouldntBeSerailized, 0x6)
{
	CCINIClass* const pINI_RULESMD = CCINIClass::INI_Rules;

	RulesClass::Instance->Read_JumpjetControls(pINI_RULESMD);

	Phobos::Config::ArtImageSwap = pINI_RULESMD->ReadBool(GameStrings::General, "ArtImageSwap", false);
	Phobos::Config::UnitPowerDrain = pINI_RULESMD->ReadBool(GameStrings::General, "UnitPowerDrain", false);

	Phobos::Misc::CustomGS = pINI_RULESMD->ReadBool(GameStrings::General, "CustomGS", false);

	char tempBuffer[26];
	for (size_t i = 0; i <= 6; ++i)
	{
		int temp;
		_snprintf_s(tempBuffer, sizeof(tempBuffer), "CustomGS%d.ChangeDelay", 6 - i);
		temp = pINI_RULESMD->ReadInteger(GameStrings::General, tempBuffer, -1);
		if (temp >= 0 && temp <= 6)
			Phobos::Misc::CustomGS_ChangeDelay[i] = 6 - temp;

		_snprintf_s(tempBuffer, sizeof(tempBuffer), "CustomGS%d.DefaultDelay", 6 - i);
		temp = pINI_RULESMD->ReadInteger(GameStrings::General, tempBuffer, -1);
		if (temp >= 1)
			Phobos::Misc::CustomGS_DefaultDelay[i] = 6 - temp;

		_snprintf_s(tempBuffer, sizeof(tempBuffer), "CustomGS%d.ChangeInterval", 6 - i);
		temp = pINI_RULESMD->ReadInteger(GameStrings::General, tempBuffer, -1);
		if (temp >= 1)
			Phobos::Misc::CustomGS_ChangeInterval[i] = temp;
	}

	if (pINI_RULESMD->ReadBool(GameStrings::General, "FixTransparencyBlitters", true))
		BlittersFix::Apply();

	Phobos::Config::SkirmishUnlimitedColors = pINI_RULESMD->ReadBool(GameStrings::General, "SkirmishUnlimitedColors", false);
	// Disable Ares hook at this address so that our logic can run.
	if (Phobos::Config::SkirmishUnlimitedColors)
		Patch::Apply_RAW(0x69A310, { 0x8B, 0x44, 0x24, 0x04, 0xD1, 0xE0, 0x40 });

	Phobos::Config::SaveVariablesOnScenarioEnd = pINI_RULESMD->ReadBool(GameStrings::General, "SaveVariablesOnScenarioEnd", false);
#ifndef DEBUG
	Phobos::Config::DevelopmentCommands = pINI_RULESMD->ReadBool("GlobalControls", "DebugKeysEnabled", Phobos::Config::DevelopmentCommands);
#endif
	Phobos::Config::SuperWeaponSidebarCommands = pINI_RULESMD->ReadBool("GlobalControls", "SuperWeaponSidebarKeysEnabled", Phobos::Config::SuperWeaponSidebarCommands);
	Phobos::Config::ShowPlanningPath = pINI_RULESMD->ReadBool("GlobalControls", "DebugPlanningPaths", Phobos::Config::ShowPlanningPath);

	return 0;
}
