#include "Body.h"

#include <BulletClass.h>
#include <HouseClass.h>
#include <InfantryClass.h>
#include <WarheadTypeClass.h>
#include <ScenarioClass.h>

#include <Ext/BuildingType/Body.h>
#include <Ext/Bullet/Body.h>
#include <Ext/Rules/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Ext/Cell/Body.h>

#include <Utilities/Macro.h>
/*
	Custom Radiations
	Worked out from old uncommented Ares RadSite Hook , adding some more hook
	and rewriting some in order to make this working perfecly
	Credit : Ares Team , for unused/uncommented source of Hook.RadSite
						,RulesData_LoadBeforeTypeData Hook
			Alex-B : GetRadSiteAt ,Helper that used at FootClass_AI & BuildingClass_AI
					Radiate , Uncommented
			me(Otamaa) adding some more stuffs and rewriting hook that cause crash

*/

DEFINE_HOOK(0x469150, BulletClass_Detonate_ApplyRadiation, 0x5)
{
	GET(BulletClass* const, pThis, ESI);
	GET_BASE(CoordStruct const* const, pCoords, 0x8);

	const auto pWeapon = pThis->GetWeaponType();

	if (pWeapon && pWeapon->RadLevel > 0 && MapClass::Instance.IsWithinUsableArea((*pCoords)))
	{
		const auto pExt = BulletExt::ExtMap.Find(pThis);
		const auto pWH = pThis->WH;
		const auto cell = CellClass::Coord2Cell(*pCoords);
		const auto spread = static_cast<int>(pWH->CellSpread);

		pExt->ApplyRadiationToCell(cell, spread, pWeapon->RadLevel);
	}

	return 0x46920B;
}
#ifndef __clang__
//unused function , safeguard
DEFINE_HOOK(0x46ADE0, BulletClass_ApplyRadiation_Unused, 0x5)
{
	Debug::Log(__FUNCTION__ " called ! , You are not supposed to be here!\n");
	return 0x46AE5E;
}
#endif
// Fix for desolator
DEFINE_HOOK(0x5213B4, InfantryClass_AIDeployment_CheckRad, 0x7)
{
	enum { FireCheck = 0x5213F4, SetMissionRate = 0x521484 };

	GET(InfantryClass*, pInfantry, ESI);
	GET(const int, weaponRadLevel, EBX);
	const auto pCell = pInfantry->GetCell();
	const auto pCellExt = CellExt::ExtMap.Find(pCell);
	int radLevel = 0;

	if (!pCellExt->RadSites.empty())
	{
		if (const auto pWeapon = pInfantry->GetDeployWeapon()->WeaponType)
		{
			const auto pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon);
			const auto pRadType = pWeaponExt->RadType;
			const float cellSpread = pWeapon->Warhead->CellSpread;

			for (const auto radSite : pCellExt->RadSites)
			{
				if (radSite->Spread == static_cast<int>(cellSpread) && RadSiteExt::ExtMap.Find(radSite)->Type == pRadType)
				{
					radLevel = radSite->GetRadLevel();
					break;
				}
			}
		}
	}

	return (!radLevel || (radLevel < weaponRadLevel / 3))
		? FireCheck : SetMissionRate;
}

// Fix for desolator unable to fire his deploy weapon when cloaked
DEFINE_HOOK(0x521478, InfantryClass_AIDeployment_FireNotOKCloakFix, 0x4)
{
	GET(InfantryClass* const, pThis, ESI);

	const auto pWeapon = pThis->GetDeployWeapon()->WeaponType;
	AbstractClass* pTarget = nullptr; //default WWP nullptr

	if (pWeapon
		&& pWeapon->DecloakToFire
		&& (pThis->CloakState == CloakState::Cloaked || pThis->CloakState == CloakState::Cloaking))
	{
		// FYI this are hack to immedietely stop the Cloaking
		// since this function is always failing to decloak and set target when cell is occupied
		// something is wrong somewhere  # Otamaa
		const int nDeployFrame = pThis->Type->Sequence->GetSequence(Sequence::DeployedFire).CountFrames;
		pThis->CloakDelayTimer.Start(nDeployFrame);

		pTarget = MapClass::Instance.TryGetCellAt(pThis->GetCoords());
	}

	pThis->SetTarget(pTarget); //Here we go

	return 0x521484;
}

// Too OP, be aware
DEFINE_HOOK(0x43FB23, BuildingClass_AI_Radiation, 0x5)
{
	GET(BuildingClass* const, pBuilding, ECX);

	if (pBuilding->Type->ImmuneToRadiation || pBuilding->InLimbo || pBuilding->BeingWarpedOut || pBuilding->TemporalTargetingMe)
		return 0;

	if (RulesExt::Global()->UseGlobalRadApplicationDelay)
	{
		const int delay = RulesExt::Global()->RadApplicationDelay_Building;

		if (delay == 0 || Unsorted::CurrentFrame % delay)
			return 0;
	}

	const auto buildingCoords = pBuilding->GetMapCoords();
	std::unordered_map<RadSiteClass*, int> damageCounts;

	for (auto pFoundation = pBuilding->GetFoundationData(false); *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
	{
		const auto nCurrentCoord = buildingCoords + *pFoundation;
		const auto pCell = MapClass::Instance.TryGetCellAt(nCurrentCoord);

		if (!pCell)
			continue;

		const auto pCellExt = CellExt::ExtMap.Find(pCell);
		std::vector<std::pair<RadTypeClass*, std::vector<std::pair<RadSiteClass*, int>>>> typeMap;
		typeMap.reserve(RadTypeClass::Array.size());

		for (const auto& [pRadSite, radLevel] : pCellExt->RadLevels)
		{
			if (radLevel <= 0)
				continue;

			const auto pRadExt = RadSiteExt::ExtMap.Find(pRadSite);
			const auto pRadType = pRadExt->Type;
			const int maxDamageCount = pRadType->GetBuildingDamageMaxCount();

			if (maxDamageCount > 0 && damageCounts[pRadSite] >= maxDamageCount)
				continue;

			if (!pRadType->GetWarhead())
				continue;

			if (!RulesExt::Global()->UseGlobalRadApplicationDelay)
			{
				const int delay = pRadType->GetBuildingApplicationDelay();

				if (delay == 0 || Unsorted::CurrentFrame % delay)
					continue;
			}

			const auto it = std::ranges::find_if(typeMap, [pRadType](std::pair<RadTypeClass*, std::vector<std::pair<RadSiteClass*, int>>> const& item) { return item.first == pRadType; });

			if (it != typeMap.cend())
			{
				it->second.emplace_back(pRadSite, radLevel);
			}
			else
			{
				std::vector<std::pair<RadSiteClass*, int>> sites;
				sites.reserve(pCellExt->RadLevels.size());
				sites.emplace_back(pRadSite, radLevel);
				typeMap.emplace_back(pRadType, std::move(sites));
			}
		}

		for (auto& [_, sites] : typeMap)
			std::ranges::stable_sort(sites, [](std::pair<RadSiteClass*, int> const& left, std::pair<RadSiteClass*, int> const& right) { return left.second > right.second; });

		for (const auto& [pRadType, sites] : typeMap)
		{
			const int radLevelMax = pRadType->GetLevelMax();
			int radLevelSum = 0;

			for (const auto& [pRadSite, radLevel] : sites)
			{
				const int remain = radLevelMax - radLevelSum;
				int damage = static_cast<int>(std::min(radLevel, remain) * pRadType->GetLevelFactor());

				if (pBuilding->IsAlive && !RadSiteExt::ExtMap.Find(pRadSite)->ApplyRadiationDamage(pBuilding, damage))
					return 0;

				if (radLevel >= remain)
					break;

				radLevelSum += radLevel;
			}
		}
	}

	return 0;
}

// skip Frame % RadApplicationDelay
DEFINE_JUMP(LJMP, 0x4DA554, 0x4DA56E);

// Hook Adjusted to support Ares RadImmune Ability check
DEFINE_HOOK(0x4DA59F, FootClass_AI_Radiation, 0x5)
{
	enum { Continue = 0x4DA63B, ReturnFromFunction = 0x4DAF00 };

	GET(FootClass* const, pFoot, ESI);

	if (pFoot->IsInPlayfield && !pFoot->TemporalTargetingMe
		&& (!RulesExt::Global()->UseGlobalRadApplicationDelay
			|| Unsorted::CurrentFrame % RulesClass::Instance->RadApplicationDelay == 0))
	{
		const auto pCell = pFoot->GetCell();
		const auto pCellExt = CellExt::ExtMap.Find(pCell);
		std::vector<std::pair<RadTypeClass*, std::vector<std::pair<RadSiteClass*, int>>>> typeMap;
		typeMap.reserve(RadTypeClass::Array.size());

		for (const auto& [pRadSite, radLevel] : pCellExt->RadLevels)
		{
			if (radLevel <= 0)
				continue;

			const auto pRadExt = RadSiteExt::ExtMap.Find(pRadSite);
			const auto pRadType = pRadExt->Type;

			if (!pRadType->GetWarhead())
				continue;

			if (!RulesExt::Global()->UseGlobalRadApplicationDelay)
			{
				const int delay = pRadType->GetApplicationDelay();

				if (delay == 0 || Unsorted::CurrentFrame % delay)
					continue;
			}

			const auto it = std::ranges::find_if(typeMap, [pRadType](std::pair<RadTypeClass*, std::vector<std::pair<RadSiteClass*, int>>> const& item) { return item.first == pRadType; });

			if (it != typeMap.cend())
			{
				it->second.emplace_back(pRadSite, radLevel);
			}
			else
			{
				std::vector<std::pair<RadSiteClass*, int>> sites;
				sites.reserve(pCellExt->RadLevels.size());
				sites.emplace_back(pRadSite, radLevel);
				typeMap.emplace_back(pRadType, std::move(sites));
			}
		}

		for (auto& [_, sites] : typeMap)
			std::ranges::stable_sort(sites, [](std::pair<RadSiteClass*, int> const& left, std::pair<RadSiteClass*, int> const& right) { return left.second > right.second; });

		for (const auto& [pRadType, sites] : typeMap)
		{
			const int radLevelMax = pRadType->GetLevelMax();
			int radLevelSum = 0;

			for (const auto& [pRadSite, radLevel] : sites)
			{
				const int remain = radLevelMax - radLevelSum;
				int damage = static_cast<int>(std::min(radLevel, remain) * pRadType->GetLevelFactor());

				if ((pFoot->IsAlive || !pFoot->IsSinking) && !RadSiteExt::ExtMap.Find(pRadSite)->ApplyRadiationDamage(pFoot, damage))
					return ReturnFromFunction;

				if (radLevel >= remain)
					break;

				radLevelSum += radLevel;
			}
		}
	}

	return pFoot->IsAlive ? Continue : ReturnFromFunction;
}

#define GET_RADSITE(reg, value)\
	GET(RadSiteClass* const, pThis, reg);\
	RadSiteExt::ExtData* pExt = RadSiteExt::ExtMap.Find(pThis);\
	auto output = pExt->Type-> value ;

/*
//All part of 0x65B580 Hooks is here
DEFINE_HOOK(65B593, RadSiteClass_Activate_Delay, 6)
{
	GET(RadSiteClass* const, pThis, ECX);
	const auto pExt = RadSiteExt::ExtMap.Find(pThis);

	const auto currentLevel = pThis->GetRadLevel();
	auto levelDelay = pExt->Type->GetLevelDelay();
	auto lightDelay = pExt->Type->GetLightDelay();

	if (currentLevel < levelDelay)
	{
		levelDelay = currentLevel;
		lightDelay = currentLevel;
	}

	R->ECX(levelDelay);
	R->EAX(lightDelay);

	return 0x65B59F;
}

DEFINE_HOOK(65B5CE, RadSiteClass_Activate_Color, 6)
{
	GET_RADSITE(ESI, GetColor());

	R->EAX(0);
	R->EDX(0);
	R->EBX(0);

	R->DL(output.G);
	R->EBP(R->EDX());

	R->BL(output.B);
	R->AL(output.R);

	// point out the missing register - Otamaa
	R->EDI(pThis);

	return 0x65B604;
}

DEFINE_HOOK(0x65B63E, RadSiteClass_Activate_LightFactor, 0x6)
{
	GET_RADSITE(ESI, GetLightFactor());

	__asm fmul output;

	return 0x65B644;
}

DEFINE_HOOK_AGAIN(0x65B6A0, RadSiteClass_Activate_TintFactor, 0x6)
DEFINE_HOOK_AGAIN(0x65B6CA, RadSiteClass_Activate_TintFactor, 0x6)
DEFINE_HOOK(0x65B6F2, RadSiteClass_Activate_TintFactor, 0x6)
{
	GET_RADSITE(ESI, GetTintFactor());

	__asm fmul output;

	return R->Origin() + 6;
}
*/

DEFINE_HOOK(0x65B843, RadSiteClass_AI_LevelDelay, 0x6)
{
	GET_RADSITE(ESI, GetLevelDelay());

	R->ECX(output);

	return 0x65B849;
}

DEFINE_HOOK(0x65B8B9, RadSiteClass_AI_LightDelay, 0x6)
{
	GET_RADSITE(ESI, GetLightDelay());

	R->ECX(output);

	return 0x65B8BF;
}

// Additional Hook below
DEFINE_HOOK(0x65BB67, RadSite_Deactivate, 0x6)
{
	GET_RADSITE(ECX, GetLevelDelay());
	GET(const int, val, EAX);

	R->EAX(val / output);
	R->EDX(val % output);

	return 0x65BB6D;
}

DEFINE_HOOK_AGAIN(0x65BE01, RadSiteClass_UpdateLevel, 0x6)// RadSiteClass_DecreaseRadiation_Decrease
DEFINE_HOOK_AGAIN(0x65BC6E, RadSiteClass_UpdateLevel, 0x6)// RadSiteClass_Deactivate_Decrease
DEFINE_HOOK(0x65BAC1, RadSiteClass_UpdateLevel, 0x8)// RadSiteClass_Radiate_Increase
{
	enum { SkipGameCode = 0x65BB11, SkipGameCode2 = 0x65BCBD, SkipGameCode3 = 0x65BE4C };

	GET(RadSiteClass*, pThis, EDX);
	GET(const int, distance, EAX);
	const int max = pThis->SpreadInLeptons;

	if (distance <= max)
	{
		CellStruct* cell = nullptr;

		if (R->Origin() == 0x65BAC1)
			cell = R->lea_Stack<CellStruct*>(STACK_OFFSET(0x60, -0x4C));
		else if (R->Origin() == 0x65BC6E)
			cell = R->lea_Stack<CellStruct*>(STACK_OFFSET(0x70, -0x5C));
		else
			cell = R->lea_Stack<CellStruct*>(STACK_OFFSET(0x60, -0x50));

		if (const auto pCellExt = CellExt::ExtMap.TryFind(MapClass::Instance.TryGetCellAt(*cell)))
		{
			auto& radLevels = pCellExt->RadLevels;

			const auto it = std::find_if(radLevels.begin(), radLevels.end(), [pThis](CellExt::RadLevel const& item) { return item.Rad == pThis; });

			if (R->Origin() == 0x65BAC1)
			{
				const int level = static_cast<int>(static_cast<double>(max - distance) / max * pThis->RadLevel);

				if (it != radLevels.end())
					it->Level = std::min(it->Level + level, RadSiteExt::ExtMap.Find(pThis)->Type->GetLevelMax());
				else
					radLevels.emplace_back(pThis, level);
			}
			else if (R->Origin() == 0x65BC6E)
			{
				if (it != radLevels.end())
				{
					GET_STACK(const int, stepCount, STACK_OFFSET(0x70, -0x30));
					const int level = static_cast<int>(static_cast<double>(max - distance) / max * pThis->RadLevel / pThis->LevelSteps * stepCount);
					it->Level = std::max(it->Level - std::max(level, 0), 0);
				}
			}
			else
			{
				if (it != radLevels.end())
				{
					const int stepCount = pThis->RadTimeLeft / RadSiteExt::ExtMap.Find(pThis)->Type->GetLevelDelay();
					const int level = static_cast<int>(static_cast<double>(max - distance) / max * pThis->RadLevel / pThis->LevelSteps * stepCount);
					it->Level = std::max(level, 0);
				}
			}
		}
	}

	if (R->Origin() == 0x65BAC1)
		return SkipGameCode;
	else if (R->Origin() == 0x65BC6E)
		return SkipGameCode2;
	else
		return SkipGameCode3;
}
