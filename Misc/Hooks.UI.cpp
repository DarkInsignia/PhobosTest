#include <Phobos.h>

#include <Helpers/Macro.h>
#include <PreviewClass.h>
#include <Surface.h>
#include <ThemeClass.h>

#include <Ext/House/Body.h>
#include <Ext/Side/Body.h>
#include <Ext/Rules/Body.h>
#include <Ext/Scenario/Body.h>
#include <Ext/TechnoType/Body.h>
#include <Ext/SWType/Body.h>

#include <Misc/FlyingStrings.h>

#include <New/Entity/BannerClass.h>
#include <New/Type/BannerTypeClass.h>

#include <Utilities/Debug.h>
#include <Ext/Anim/Body.h>
#include <Unsorted.h>

namespace AnimParticlePrune
{
	static size_t   Cursor = 0;
	static unsigned Tick = 0;
	static unsigned BoostFrames = 0;
	static constexpr size_t BudgetNormal = 64;
	static constexpr size_t BudgetBoost = 512;
	static constexpr size_t SoftThresh = 800;   // tune
	static constexpr size_t HardThresh = 1600;  // tune

	static __forceinline void Service()
	{
		// normal run ~every 256 frames; boost runs every frame for a short period
		if (BoostFrames == 0 && ((++Tick & 0xFFu) != 0)) return;

		auto& v = AnimExt::AnimsWithAttachedParticles;
		const size_t n = v.size();

		// trigger/extend boost if we cross thresholds
		if (n > HardThresh)      BoostFrames = 180; // ~3s at 60fps
		else if (n > SoftThresh) BoostFrames = std::max<unsigned>(BoostFrames, 120);

		const size_t budget = (BoostFrames ? BudgetBoost : BudgetNormal);
		if (BoostFrames) { --BoostFrames; }

		size_t visited = 0;
		while (visited < budget && !v.empty())
		{
			if (Cursor >= v.size()) Cursor = 0;

			AnimClass* a = v[Cursor];
			AnimExt::ExtData* x = AnimExt::ExtMap.TryFind(a);
			const bool stale = (a == nullptr) || a->InLimbo || (x == nullptr) || (x->AttachedSystem == nullptr);

			if (stale)
			{
				AnimClass* moved = v.back();
				v[Cursor] = moved;
				v.pop_back(); // swap-erase stale
			}
			else
			{
				++Cursor;
				++visited;
			}
		}
	}
}

DEFINE_HOOK(0x777C41, UI_ApplyAppIcon, 0x9)
{
	if (Phobos::AppIconPath != nullptr && strlen(Phobos::AppIconPath))
	{
		Debug::Log("Applying AppIcon from \"%s\"\n", Phobos::AppIconPath);

		R->EAX(LoadImage(NULL, Phobos::AppIconPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE));
		return 0x777C4A;
	}

	return 0;
}

DEFINE_HOOK(0x640B8D, LoadingScreen_DisableEmptySpawnPositions, 0x6)
{
	GET(const bool, esi, ESI);
	if (Phobos::UI::DisableEmptySpawnPositions || !esi)
	{
		return 0x640CE2;
	}
	return 0x640B93;
}

//DEFINE_HOOK(0x640E78, LoadingScreen_DisableColorPoints, 0x6)
//{
//	return 0x641071;
//}

// Allow size = 0 for map previews
DEFINE_HOOK(0x641B41, LoadingScreen_SkipPreview, 0x8)
{
	GET(RectangleStruct*, pRect, EAX);
	if (pRect->Width > 0 && pRect->Height > 0)
	{
		return 0;
	}
	return 0x641D4E;
}

DEFINE_HOOK(0x641EE0, PreviewClass_ReadPreview, 0x6)
{
	GET(PreviewClass*, pThis, ECX);
	GET_STACK(const char*, lpMapFile, 0x4);

	CCFileClass file(lpMapFile);
	if (file.Exists() && file.Open(FileAccessMode::Read))
	{
		CCINIClass ini;
		ini.ReadCCFile(&file, true);
		ini.CurrentSection = nullptr;
		ini.CurrentSectionName = nullptr;

		ScenarioClass::Instance->ReadStartPoints(ini);

		R->EAX(pThis->ReadPreviewPack(ini));
	}
	else
		R->EAX(false);

	return 0x64203D;
}

DEFINE_HOOK(0x4A25E0, CreditsClass_GraphicLogic_HarvesterCounter, 0x7)
{
	auto const pPlayer = HouseClass::CurrentPlayer;
	if (pPlayer->Defeated)
		return 0;

	RectangleStruct vRect = DSurface::Sidebar->GetRect();
	auto* const pHouseExt = HouseExt::ExtMap.Find(pPlayer);

	// Fix 2: do this once and reuse everywhere below
	auto* const pSide = SideClass::Array.GetItem(pPlayer->SideIndex);
	auto* const pSideExt = SideExt::ExtMap.Find(pSide);

	int baseStartX = DSurface::Sidebar->GetWidth() / 2 - 70;
	int currentX = baseStartX;
	int yPosition = 2;
	const int spacing = 0;

	// CommanderPoints - always positioned first if enabled
	if (pHouseExt->AreCommanderPointsEnabled() && Phobos::UI::CommanderPointsSidebar_Show)
	{
		wchar_t counter[0x20];
		ColorStruct clrToolTip = pSideExt->Sidebar_CommanderPoints_Color.Get(Drawing::TooltipColor);
		int points = pHouseExt->CommanderPoints;

		// Check if label should be hidden - show only the number
		if (Phobos::UI::CommanderPointsSidebar_HideLabel)
		{
			swprintf_s(counter, L"%d", points);
		}
		else
		{
			if (Phobos::UI::CommanderPointsSidebar_Label_InvertPosition)
				swprintf_s(counter, L"%d %ls", points, Phobos::UI::CommanderPointsSidebar_Label);
			else
				swprintf_s(counter, L"%ls %d", Phobos::UI::CommanderPointsSidebar_Label, points);
		}

		Point2D vPos = {
			currentX + pSideExt->Sidebar_CommanderPoints_Offset.Get().X,
			yPosition + pSideExt->Sidebar_CommanderPoints_Offset.Get().Y
		};

		auto const TextFlags = static_cast<TextPrintType>(static_cast<int>(TextPrintType::UseGradPal | TextPrintType::Metal12)
				| static_cast<int>(pSideExt->Sidebar_CommanderPoints_Align.Get()));

		DSurface::Sidebar->DrawText(counter, &vRect, &vPos, Drawing::RGB_To_Int(clrToolTip), 0, TextFlags);

		// Estimate text width to advance position for BattlePoints
		// Use character count * average character width as approximation
		int textWidth = wcslen(counter) * 6; // Approximate 6 pixels per character for Metal12 font
		currentX += textWidth + spacing;
	}

	// BattlePoints - positioned after CommanderPoints or at base position if CP disabled
	if (pHouseExt->AreBattlePointsEnabled() && Phobos::UI::BattlePointsSidebar_Show)
	{
		wchar_t counter[0x20];
		ColorStruct clrToolTip = pSideExt->Sidebar_BattlePoints_Color.Get(Drawing::TooltipColor);
		int points = pHouseExt->BattlePoints;

		// Format with tighter spacing (no space between label and value) and optional percentage
		if (Phobos::UI::BattlePointsSidebar_Label_InvertPosition)
		{
			if (Phobos::UI::BattlePointsSidebar_DisplayAsPercentage)
				swprintf_s(counter, L"%d\u2607%ls", points, Phobos::UI::BattlePointsSidebar_Label);
			else
				swprintf_s(counter, L"%d%ls", points, Phobos::UI::BattlePointsSidebar_Label);
		}
		else
		{
			if (Phobos::UI::BattlePointsSidebar_DisplayAsPercentage)
				swprintf_s(counter, L"%ls%d\u2607", Phobos::UI::BattlePointsSidebar_Label, points);
			else
				swprintf_s(counter, L"%ls%d", Phobos::UI::BattlePointsSidebar_Label, points);
		}

		Point2D vPos = {
			currentX + pSideExt->Sidebar_BattlePoints_Offset.Get().X,
			yPosition + pSideExt->Sidebar_BattlePoints_Offset.Get().Y
		};

		auto const TextFlags = static_cast<TextPrintType>(static_cast<int>(TextPrintType::UseGradPal | TextPrintType::Metal12)
				| static_cast<int>(pSideExt->Sidebar_BattlePoints_Align.Get()));

		DSurface::Sidebar->DrawText(counter, &vRect, &vPos, Drawing::RGB_To_Int(clrToolTip), 0, TextFlags);
	}

	if (Phobos::UI::HarvesterCounter_Show && Phobos::Config::ShowHarvesterCounter)
	{
		//const auto pSideExt = SideExt::ExtMap.Find(SideClass::Array.GetItem(pPlayer->SideIndex));
		wchar_t counter[0x20];
		const auto nActive = HouseExt::ActiveHarvesterCount(pPlayer);
		const auto nTotal = HouseExt::TotalHarvesterCount(pPlayer);
		const auto nPercentage = nTotal == 0 ? 1.0 : (double)nActive / (double)nTotal;

		const ColorStruct clrToolTip = nPercentage > Phobos::UI::HarvesterCounter_ConditionYellow
			? Drawing::TooltipColor : nPercentage > Phobos::UI::HarvesterCounter_ConditionRed
			? pSideExt->Sidebar_HarvesterCounter_Yellow : pSideExt->Sidebar_HarvesterCounter_Red;

		swprintf_s(counter, L"%ls%d/%d", Phobos::UI::HarvesterLabel, nActive, nTotal);

		Point2D vPos = {
			DSurface::Sidebar->GetWidth() / 2 + 50 + pSideExt->Sidebar_HarvesterCounter_Offset.Get().X,
			2 + pSideExt->Sidebar_HarvesterCounter_Offset.Get().Y
		};

		DSurface::Sidebar->DrawText(counter, &vRect, &vPos, Drawing::RGB_To_Int(clrToolTip), 0,
			TextPrintType::UseGradPal | TextPrintType::Center | TextPrintType::Metal12);
	}

	if (Phobos::UI::PowerDelta_Show && Phobos::Config::ShowPowerDelta && pPlayer->Buildings.Count)
	{
		//const auto pSideExt = SideExt::ExtMap.Find(SideClass::Array.GetItem(pPlayer->SideIndex));
		wchar_t counter[0x20];

		ColorStruct clrToolTip;

		if (pPlayer->PowerBlackoutTimer.InProgress())
		{
			clrToolTip = pSideExt->Sidebar_PowerDelta_Grey;
			swprintf_s(counter, L"%ls", Phobos::UI::PowerBlackoutLabel);
		}
		else
		{
			const int delta = pPlayer->PowerOutput - pPlayer->PowerDrain;

			const double percent = pPlayer->PowerOutput != 0
				? (double)pPlayer->PowerDrain / (double)pPlayer->PowerOutput : pPlayer->PowerDrain != 0
				? Phobos::UI::PowerDelta_ConditionRed * 2.f : Phobos::UI::PowerDelta_ConditionYellow;

			clrToolTip = percent < Phobos::UI::PowerDelta_ConditionYellow
				? pSideExt->Sidebar_PowerDelta_Green : LESS_EQUAL(percent, Phobos::UI::PowerDelta_ConditionRed)
				? pSideExt->Sidebar_PowerDelta_Yellow : pSideExt->Sidebar_PowerDelta_Red;

			swprintf_s(counter, L"%ls%+d", Phobos::UI::PowerLabel, delta);
		}

		Point2D vPos = {
			DSurface::Sidebar->GetWidth() / 2 - 70 + pSideExt->Sidebar_PowerDelta_Offset.Get().X,
			2 + pSideExt->Sidebar_PowerDelta_Offset.Get().Y
		};

		auto const TextFlags = static_cast<TextPrintType>(static_cast<int>(TextPrintType::UseGradPal | TextPrintType::Metal12)
				| static_cast<int>(pSideExt->Sidebar_PowerDelta_Align.Get()));

		DSurface::Sidebar->DrawText(counter, &vRect, &vPos, Drawing::RGB_To_Int(clrToolTip), 0, TextFlags);
	}

	if (Phobos::UI::WeedsCounter_Show && Phobos::Config::ShowWeedsCounter)
	{
		//const auto pSideExt = SideExt::ExtMap.Find(SideClass::Array.GetItem(pPlayer->SideIndex));
		wchar_t counter[0x20];
		const ColorStruct clrToolTip = pSideExt->Sidebar_WeedsCounter_Color.Get(Drawing::TooltipColor);

		swprintf_s(counter, L"%d", static_cast<int>(pPlayer->OwnedWeed.GetTotalAmount()));

		Point2D vPos = {
			DSurface::Sidebar->GetWidth() / 2 + 50 + pSideExt->Sidebar_WeedsCounter_Offset.Get().X,
			2 + pSideExt->Sidebar_WeedsCounter_Offset.Get().Y
		};

		DSurface::Sidebar->DrawText(counter, &vRect, &vPos, Drawing::RGB_To_Int(clrToolTip), 0,
			TextPrintType::UseGradPal | TextPrintType::Center | TextPrintType::Metal12);
	}

	return 0;
}

DEFINE_HOOK_AGAIN(0x6CE8AA, Replace_XXICON_With_New, 0x7)   //SWTypeClass::Load
DEFINE_HOOK_AGAIN(0x6CEE31, Replace_XXICON_With_New, 0x7)   //SWTypeClass::ReadINI
DEFINE_HOOK_AGAIN(0x716D13, Replace_XXICON_With_New, 0x7)   //TechnoTypeClass::Load
DEFINE_HOOK(0x715A4D, Replace_XXICON_With_New, 0x7)         //TechnoTypeClass::ReadINI
{
	char pFilename[0x20];
	strcpy_s(pFilename, RulesExt::Global()->MissingCameo.data());
	_strlwr_s(pFilename);

	if (_stricmp(pFilename, GameStrings::XXICON_SHP)
		&& strstr(pFilename, ".shp"))
	{
		if (const auto pFile = FileSystem::LoadFile(RulesExt::Global()->MissingCameo, false))
		{
			R->EAX(pFile);
			return R->Origin() + 0xC;
		}
	}

	return 0;
}

DEFINE_HOOK(0x6A8463, StripClass_OperatorLessThan_CameoPriority, 0x5)
{
	GET_STACK(TechnoTypeClass*, pLeft, STACK_OFFSET(0x1C, -0x8));
	GET_STACK(TechnoTypeClass*, pRight, STACK_OFFSET(0x1C, -0x4));
	GET_STACK(const int, idxLeft, STACK_OFFSET(0x1C, 0x8));
	GET_STACK(const int, idxRight, STACK_OFFSET(0x1C, 0x10));
	GET_STACK(AbstractType, rttiLeft, STACK_OFFSET(0x1C, 0x4));
	GET_STACK(AbstractType, rttiRight, STACK_OFFSET(0x1C, 0xC));
	const auto pLeftTechnoExt = TechnoTypeExt::ExtMap.TryFind(pLeft);
	const auto pRightTechnoExt = TechnoTypeExt::ExtMap.TryFind(pRight);
	const auto pLeftSWExt = (rttiLeft == AbstractType::Special || rttiLeft == AbstractType::Super || rttiLeft == AbstractType::SuperWeaponType)
		? SWTypeExt::ExtMap.TryFind(SuperWeaponTypeClass::Array.GetItem(idxLeft)) : nullptr;
	const auto pRightSWExt = (rttiRight == AbstractType::Special || rttiRight == AbstractType::Super || rttiRight == AbstractType::SuperWeaponType)
		? SWTypeExt::ExtMap.TryFind(SuperWeaponTypeClass::Array.GetItem(idxRight)) : nullptr;

	if ((pLeftTechnoExt || pLeftSWExt) && (pRightTechnoExt || pRightSWExt))
	{
		const int leftPriority = pLeftTechnoExt ? pLeftTechnoExt->CameoPriority : pLeftSWExt->CameoPriority;
		const int rightPriority = pRightTechnoExt ? pRightTechnoExt->CameoPriority : pRightSWExt->CameoPriority;
		enum { rTrue = 0x6A8692, rFalse = 0x6A86A0 };

		if (leftPriority > rightPriority)
			return rTrue;
		else if (rightPriority > leftPriority)
			return rFalse;
	}

	// Restore overridden instructions
	GET(AbstractType, rtti1, ESI);
	return rtti1 == AbstractType::Special ? 0x6A8477 : 0x6A8468;
}

DEFINE_HOOK(0x6D4684, TacticalClass_Draw_FlyingStrings, 0x6)
{
	FlyingStrings::UpdateAll();
	AnimParticlePrune::Service();
	return 0;
}

DEFINE_HOOK(0x456776, BuildingClass_DrawRadialIndicator_Visibility, 0x6)
{
	enum { ContinueDraw = 0x456789, DoNotDraw = 0x456962 };
	GET(BuildingClass* const, pThis, ESI);

	if (HouseClass::IsCurrentPlayerObserver() || pThis->Owner->IsControlledByCurrentPlayer())
		return ContinueDraw;

	AffectedHouse const canSee = RulesExt::Global()->RadialIndicatorVisibility.Get();
	if (pThis->Owner->IsAlliedWith(HouseClass::CurrentPlayer) ? canSee & AffectedHouse::Allies : canSee & AffectedHouse::Enemies)
		return ContinueDraw;

	return DoNotDraw;
}

DEFINE_HOOK(0x6D4B25, TacticalClass_Render_Banner, 0x5)
{
	for (const auto& pBanner : BannerClass::Array)
		pBanner->Render();

	return 0;
}

#pragma region ShowBriefing

namespace BriefingTemp
{
	bool ShowBriefing = false;
}

__forceinline void ShowBriefing()
{
	if (BriefingTemp::ShowBriefing)
	{
		// Show briefing dialog.
		Game::SpecialDialog = 9;
		Game::ShowSpecialDialog();
		BriefingTemp::ShowBriefing = false;

		// Play scenario theme.
		const int theme = ScenarioClass::Instance->ThemeIndex;

		if (theme == -1)
			ThemeClass::Instance.Stop(true);
		else
			ThemeClass::Instance.Queue(theme);
	}
}

// Check if briefing dialog should be played before starting scenario.
DEFINE_HOOK(0x683E41, ScenarioClass_Start_ShowBriefing, 0x6)
{
	enum { SkipGameCode = 0x683E6B };

	GET_STACK(const bool, showBriefing, STACK_OFFSET(0xFC, -0xE9));

	// Don't show briefing dialog for non-campaign games etc.
	if (!Phobos::Config::ShowBriefing || !ScenarioExt::Global()->ShowBriefing || !showBriefing || !SessionClass::IsCampaign())
		return 0;

	BriefingTemp::ShowBriefing = true;

	int theme = ScenarioExt::Global()->BriefingTheme;

	if (theme == -1)
	{
		const SideClass* pSide = SideClass::Array.GetItemOrDefault(ScenarioClass::Instance->PlayerSideIndex);

		if (const auto pSideExt = SideExt::ExtMap.TryFind(pSide))
			theme = pSideExt->BriefingTheme;
	}

	if (theme != -1)
		ThemeClass::Instance.Queue(theme);

	// Skip over playing scenario theme.
	return SkipGameCode;
}

// Show the briefing dialog before entering game loop.
DEFINE_HOOK(0x48CE85, MainGame_ShowBriefing, 0x5)
{
	enum { SkipGameCode = 0x48CE8A };

	// Restore overridden instructions.
	SessionClass::Instance.Resume();

	ShowBriefing();

	return SkipGameCode;
}

// Show the briefing dialog on starting a new scenario after clearing another.
DEFINE_HOOK(0x55D14F, AuxLoop_ShowBriefing, 0x5)
{
	enum { SkipGameCode = 0x55D159 };

	// Restore overridden instructions.
	SessionClass::Instance.Resume();

	ShowBriefing();

	return SkipGameCode;
}

// Skip redrawing the screen if we're gonna show the briefing screen immediately after loading screen finishes on initially launched mission.
DEFINE_HOOK(0x683F66, PauseGame_ShowBriefing, 0x5)
{
	enum { SkipGameCode = 0x683FAA };

	if (BriefingTemp::ShowBriefing)
		return SkipGameCode;

	return 0;
}

// Skip redrawing the screen if we're gonna show the briefing screen immediately after loading screen finishes on succeeding missions.
DEFINE_HOOK(0x685D95, DoWin_ShowBriefing, 0x5)
{
	enum { SkipGameCode = 0x685D9F };

	if (BriefingTemp::ShowBriefing)
		return SkipGameCode;

	return 0;
}

// Set briefing dialog resume button text.
DEFINE_HOOK(0x65F764, BriefingDialog_ShowBriefing, 0x5)
{
	if (BriefingTemp::ShowBriefing)
	{
		GET(HWND, hDlg, ESI);

		auto const hResumeBtn = GetDlgItem(hDlg, 1059);
		SendMessageA(hResumeBtn, 1202, 0, reinterpret_cast<LPARAM>(Phobos::UI::ShowBriefingResumeButtonLabel));
	}

	return 0;
}

// Set briefing dialog resume button status bar label.
DEFINE_HOOK(0x604985, GetDialogUIStatusLabels_ShowBriefing, 0x5)
{
	if (BriefingTemp::ShowBriefing)
	{
		enum { SkipGameCode = 0x60498A };

		R->EAX(Phobos::UI::ShowBriefingResumeButtonStatusLabel);

		return SkipGameCode;
	}

	return 0;
}

#pragma endregion

bool __fastcall Fake_HouseIsAlliedWith(HouseClass* pThis, void*, HouseClass* CurrentPlayer)
{
	return (Phobos::Config::ShowPlanningPath && SessionClass::IsSingleplayer())
		|| pThis->IsControlledByCurrentPlayer()
		|| pThis->IsAlliedWith(CurrentPlayer);
}

DEFINE_FUNCTION_JUMP(CALL, 0x63B136, Fake_HouseIsAlliedWith);
DEFINE_FUNCTION_JUMP(CALL, 0x63B100, Fake_HouseIsAlliedWith);
DEFINE_FUNCTION_JUMP(CALL, 0x63B17F, Fake_HouseIsAlliedWith);
DEFINE_FUNCTION_JUMP(CALL, 0x63B1BA, Fake_HouseIsAlliedWith);
DEFINE_FUNCTION_JUMP(CALL, 0x63B2CE, Fake_HouseIsAlliedWith);

DEFINE_HOOK(0x69A317, SessionClass_PlayerColorIndexToColorSchemeIndex, 0x0)
{
	GET_STACK(int, index, 0x4);

	bool isRandom = index == PlayerColorSlot::Random;

	if (Phobos::Config::SkirmishUnlimitedColors)
	{
		// Allow player color indices to map directly to color scheme indices.
		if (isRandom)
			index = ColorScheme::FindIndex("LightGrey", 53);
		else
			index = index * 2 + 1;
	}
	else
	{
		// Vanilla behaviour.
		if (isRandom)
			index = ColorScheme::PlayerColorToColorSchemeLUT[PlayerColorSlot::White];
		else if (index < PlayerColorSlot::Count)
			index = ColorScheme::PlayerColorToColorSchemeLUT[index];
	}

	R->EAX(index);

	return 0x69A325;
}
