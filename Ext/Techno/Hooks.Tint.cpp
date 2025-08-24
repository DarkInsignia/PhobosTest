#include <AircraftClass.h>
#include <InfantryClass.h>
#include <JumpjetLocomotionClass.h>
#include <SuperClass.h>

#include "Body.h"

#include <Ext/Anim/Body.h>

DEFINE_HOOK(0x43D386, BuildingClass_Draw_TintColor, 0x6)
{
	enum { SkipGameCode = 0x43D4EB };

	GET(BuildingClass*, pThis, ESI);

	int color = TechnoExt::GetTintColor(pThis, true, true, false);
	color |= TechnoExt::GetCustomTintColor(pThis);
	R->EDI(color);

	return SkipGameCode;
}

DEFINE_HOOK(0x43DC1C, BuildingClass_Draw2_TintColor, 0x6)
{
	enum { SkipGameCode = 0x43DD8E };

	GET(BuildingClass*, pThis, EBP);
	REF_STACK(int, color, STACK_OFFSET(0x12C, -0x110));

	color = TechnoExt::GetTintColor(pThis, true, true, false);
	color |= TechnoExt::GetCustomTintColor(pThis);

	return SkipGameCode;
}

DEFINE_HOOK(0x73BF95, UnitClass_DrawAsVoxel_Tint, 0x7)
{
	enum { SkipGameCode = 0x73C141 };

	GET(UnitClass*, pThis, EBP);
	GET(const int, flashIntensity, ESI);
	REF_STACK(int, intensity, STACK_OFFSET(0x1D0, 0x10));

	intensity = flashIntensity;

	if (pThis->IsIronCurtained())
		intensity = pThis->GetInvulnerabilityTintIntensity(intensity);

	if (TechnoExt::ExtMap.Find(pThis)->AirstrikeTargetingMe)
		intensity = pThis->GetAirstrikeTintIntensity(intensity);

	int color = TechnoExt::GetTintColor(pThis, true, true, true);
	TechnoExt::ApplyCustomTintValues(pThis, color, intensity);

	R->ESI(color);
	return SkipGameCode;
}

DEFINE_HOOK(0x518FC8, InfantryClass_Draw_TintColor, 0x6)
{
	enum { SkipGameCode = 0x519082 };

	GET(InfantryClass*, pThis, EBP);
	REF_STACK(int, color, STACK_OFFSET(0x54, -0x40));

	color = 0;
	color |= TechnoExt::GetTintColor(pThis, true, true, true);
	color |= TechnoExt::GetCustomTintColor(pThis);

	return SkipGameCode;
}

DEFINE_HOOK(0x51946D, InfantryClass_Draw_TintIntensity, 0x6)
{
	GET(InfantryClass*, pThis, EBP);
	GET(int, intensity, ESI);

	if (pThis->IsIronCurtained())
		intensity = pThis->GetInvulnerabilityTintIntensity(intensity);

	if (TechnoExt::ExtMap.Find(pThis)->AirstrikeTargetingMe)
		intensity = pThis->GetAirstrikeTintIntensity(intensity);

	intensity += TechnoExt::GetCustomTintIntensity(pThis);
	R->ESI(intensity);

	return 0;
}

DEFINE_HOOK(0x423420, AnimClass_Draw_TintColor, 0x6)
{
	enum { SkipGameCode = 0x4235D3 };

	GET(AnimClass*, pThis, ESI);
	GET(BuildingClass*, pBuilding, EAX);
	REF_STACK(int, color, STACK_OFFSET(0x110, -0xF4));
	REF_STACK(int, intensity, STACK_OFFSET(0x110, -0xD8));

	if (!pBuilding)
		pBuilding = AnimExt::ExtMap.Find(pThis)->ParentBuilding;

	if (pBuilding)
	{
		int discard = 0;
		color |= TechnoExt::GetTintColor(pBuilding, true, true, false);
		TechnoExt::ApplyCustomTintValues(pBuilding, color, pThis->Type->UseNormalLight ? discard : intensity);
	}

	R->EBP(color);
	return SkipGameCode;
}

DEFINE_HOOK(0x706389, TechnoClass_DrawObject_TintColor, 0x6)
{
	GET(TechnoClass*, pThis, ESI);
	GET(int, intensity, EBP);
	REF_STACK(int, color, STACK_OFFSET(0x54, 0x2C));

	auto const rtti = pThis->WhatAmI();
	const bool isAircraft = rtti == AbstractType::Aircraft;

	// SHP vehicles and aircraft
	if (rtti == AbstractType::Unit || isAircraft)
	{
		color |= TechnoExt::GetTintColor(pThis, true, true, !isAircraft);
		TechnoExt::ApplyCustomTintValues(pThis, color, intensity);
	}
	else if (rtti != AbstractType::Infantry)
	{
		intensity += TechnoExt::GetCustomTintIntensity(pThis);
	}

	R->EBP(intensity);

	return 0;
}

DEFINE_HOOK(0x70632E, TechnoClass_DrawShape_GetTintIntensity, 0x6)
{
	enum { SkipGameCode = 0x706389 };

	GET(TechnoClass*, pThis, ESI);
	GET(int, intensity, EAX);

	if (pThis->IsIronCurtained())
		intensity = pThis->GetInvulnerabilityTintIntensity(intensity);

	const auto pExt = TechnoExt::ExtMap.Find(pThis);

	if (pExt->AirstrikeTargetingMe)
		intensity = pThis->GetAirstrikeTintIntensity(intensity);

	R->EBP(intensity);
	return SkipGameCode;
}

DEFINE_HOOK(0x706786, TechnoClass_DrawVoxel_TintColor, 0x5)
{
	enum { SkipTint = 0x7067E4 };

	GET(TechnoClass*, pThis, EBP);

	auto const rtti = pThis->WhatAmI();

	// Vehicles already have had tint intensity as well as custom tints applied, no need to do it twice.
	if (rtti == AbstractType::Unit)
		return SkipTint;

	GET(int, intensity, EAX);
	REF_STACK(int, color, STACK_OFFSET(0x50, 0x24));

	if (rtti == AbstractType::Aircraft)
		color = TechnoExt::GetTintColor(pThis, true, true, false);

	// Non-aircraft voxels do not need custom tint color applied again, discard that component for them.
	int discardColor = 0;
	TechnoExt::ApplyCustomTintValues(pThis, rtti == AbstractType::Aircraft ? color : discardColor, intensity);

	if (pThis->IsIronCurtained())
		intensity = pThis->GetInvulnerabilityTintIntensity(intensity);

	const auto pExt = TechnoExt::ExtMap.Find(pThis);

	if (pExt->AirstrikeTargetingMe)
		intensity = pThis->GetAirstrikeTintIntensity(intensity);

	R->EDI(intensity);
	return SkipTint;
}

DEFINE_HOOK(0x43FA19, BuildingClass_Mark_TintIntensity, 0x7)
{
	GET(BuildingClass*, pThis, EDI);
	GET(int, intensity, ESI);

	intensity += TechnoExt::GetCustomTintIntensity(pThis);
	R->ESI(intensity);

	return 0;
}

namespace ICTintTemp
{
	bool IsForceShield = false;
}

DEFINE_HOOK(0x70E380, TechnoClass_InvulnerabilityIntensity_SetContext, 0x6)
{
	GET(TechnoClass*, pThis, ECX);

	ICTintTemp::IsForceShield = pThis->ForceShielded;

	return 0;
}

DEFINE_HOOK(0x70E475, TechnoClass_InvulnerabilityIntensity_Adjust, 0x5)
{
	enum { SkipGameCode = 0x70E488 };

	GET(int, intensity, EAX);

	if (intensity > 2000)
		intensity = 2000;

	const int max = static_cast<int>((ICTintTemp::IsForceShield
		? RulesExt::Global()->ForceShield_ExtraTintIntensity
		: RulesExt::Global()->IronCurtain_ExtraTintIntensity) * 1000);

	R->EAX(intensity + max);
	return SkipGameCode;
}

#pragma region LevelLighting

static bool __forceinline IsOnBridge(FootClass* pUnit)
{
	auto const pCell = MapClass::Instance.GetCellAt(pUnit->GetCoords());
	auto const pCellAdj = pCell->GetNeighbourCell(FacingType::North);
	const bool containsBridge = pCell->ContainsBridge();
	const bool containsBridgeDir = static_cast<bool>(pCell->Flags & CellFlags::BridgeDir);

	return (containsBridge || containsBridgeDir || pCellAdj->ContainsBridge())
		&& (!containsBridge || pCell->GetNeighbourCell(FacingType::West)->ContainsBridge());
}

static void __forceinline GetLevelIntensity(TechnoClass* pThis, int level, int& levelIntensity, int& cellIntensity, double levelMult, bool applyBridgeBonus)
{
	const int bridgeHeight = applyBridgeBonus ? 4 : 0;
	const int bridgeBonus = bridgeHeight * level;
	const double currentLevel = (pThis->GetHeight() / static_cast<double>(Unsorted::LevelHeight)) - bridgeHeight;
	levelIntensity = static_cast<int>(level * currentLevel * levelMult);
	cellIntensity = MapClass::Instance.GetCellAt(pThis->GetMapCoords())->Intensity_Normal + bridgeBonus;
}

DEFINE_HOOK(0x4148F4, AircraftClass_DrawIt_LevelIntensity, 0x5)
{
	enum { SkipGameCode = 0x414925 };

	GET(AircraftClass*, pThis, EBP);
	GET(int, level, EDI);

	if (PsyDom::Active())
		level = ScenarioClass::Instance->DominatorLighting.Level;
	else if (NukeFlash::IsFadingIn())
		level = ScenarioClass::Instance->NukeLighting.Level;

	int levelIntensity = 0;
	int cellIntensity = 1000;
	GetLevelIntensity(pThis, level, levelIntensity, cellIntensity, RulesExt::Global()->AircraftLevelLightMultiplier, false);

	R->ESI(levelIntensity);
	R->EBX(cellIntensity);

	return SkipGameCode;
}

DEFINE_HOOK(0x51933B, InfantryClass_DrawIt_LevelIntensity, 0x6)
{
	enum { SkipGameCode = 0x51944D };

	GET(InfantryClass*, pThis, EBP);
	GET(const int, level, EBX);

	if (locomotion_cast<JumpjetLocomotionClass*>(pThis->Locomotor))
	{
		int levelIntensity = 0;
		int cellIntensity = 1000;
		GetLevelIntensity(pThis, level, levelIntensity, cellIntensity, RulesExt::Global()->JumpjetLevelLightMultiplier, IsOnBridge(pThis));

		R->ESI(levelIntensity + cellIntensity);
		return SkipGameCode;
	}

	return 0;
}

DEFINE_HOOK(0x73CFA7, UnitClass_DrawIt_LevelIntensity, 0x6)
{
	enum { SkipGameCode = 0x73D0C3 };

	GET(UnitClass*, pThis, ESI);

	if (locomotion_cast<JumpjetLocomotionClass*>(pThis->Locomotor))
	{
		int level = ScenarioClass::Instance->NormalLighting.Level;

		if (LightningStorm::Active)
			level = ScenarioClass::Instance->IonLighting.Level;
		else if (PsyDom::Active())
			level = ScenarioClass::Instance->DominatorLighting.Level;
		else if (NukeFlash::IsFadingIn())
			level = ScenarioClass::Instance->NukeLighting.Level;

		int levelIntensity = 0;
		int cellIntensity = 1000;
		GetLevelIntensity(pThis, level, levelIntensity, cellIntensity, RulesExt::Global()->JumpjetLevelLightMultiplier, IsOnBridge(pThis));

		R->EBP(levelIntensity + cellIntensity);
		return SkipGameCode;
	}

	return 0;
}

#pragma endregion
