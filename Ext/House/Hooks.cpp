#include "Body.h"

#include <Ext/Aircraft/Body.h>
#include <Ext/Scenario/Body.h>
#include "Ext/Techno/Body.h"
#include "Ext/Building/Body.h"
#include <Utilities/Debug.h>
#include <unordered_map>

DEFINE_HOOK(0x508C30, HouseClass_UpdatePower_UpdateCounter, 0x5)
{
	GET(HouseClass*, pThis, ECX);
	auto* const pHouseExt = HouseExt::ExtMap.Find(pThis);

	// Reset the cache
	pHouseExt->PowerPlantEnhancers.clear();

	// Count candidate enhancers per BuildingType by ArrayIndex
	const size_t typeCount = static_cast<size_t>(BuildingTypeClass::Array.Count);
	std::vector<uint16_t> counts(typeCount, 0); // compact and zero-initialized

	for (auto const pBld : pThis->Buildings)
	{
		if (TechnoExt::IsActive(pBld) && pBld->IsOnMap && pBld->HasPower)
		{
			const auto pType = pBld->Type;
			const auto pExt = BuildingTypeExt::ExtMap.Find(pType);

			if (!pExt->PowerPlantEnhancer_Buildings.empty()
				&& (pExt->PowerPlantEnhancer_Amount != 0
					|| pExt->PowerPlantEnhancer_Factor != 1.0f))
			{
				const int idx = pType->ArrayIndex;
				if (idx >= 0 && static_cast<size_t>(idx) < typeCount)
					++counts[static_cast<size_t>(idx)];
			}
		}
	}

	// Populate the associative container only for non-zero indices
	// (avoids creating lots of empty/default nodes)
	for (size_t i = 0; i < typeCount; ++i)
	{
		if (counts[i] != 0)
		{
			// works for std::map and std::unordered_map; avoids operator[] default insert
			pHouseExt->PowerPlantEnhancers.emplace(static_cast<int>(i), static_cast<int>(counts[i]));
		}
	}

	return 0;
}

// Power Plant Enhancer #131
DEFINE_HOOK(0x508CF2, HouseClass_UpdatePower_PowerOutput, 0x7)
{
	GET(HouseClass*, pThis, ESI);
	GET(BuildingClass*, pBld, EDI);

	pThis->PowerOutput += BuildingTypeExt::GetEnhancedPower(pBld, pThis);

	return 0x508D07;
}

// Trigger power recalculation on gain/loss of any techno, not just buildings.
DEFINE_HOOK_AGAIN(0x5025F0, HouseClass_RegisterGain, 0x5) // RegisterLoss
DEFINE_HOOK(0x502A80, HouseClass_RegisterGain, 0x8)
{
	if (!Phobos::Config::UnitPowerDrain)
		return 0;

	GET(HouseClass*, pThis, ECX);

	pThis->RecheckPower = true;

	return 0;
}

DEFINE_HOOK(0x508D8D, HouseClass_UpdatePower_Techno, 0x6)
{
	if (!Phobos::Config::UnitPowerDrain)
		return 0;

	GET(HouseClass*, pThis, ESI);

	// Static caches: only types with non-zero Power
	struct NonZeroLists
	{
		std::vector<const InfantryTypeClass*>  Inf;
		std::vector<const UnitTypeClass*>      Uni;
		std::vector<const AircraftTypeClass*>  Air;
		bool Built = false;
	};
	static NonZeroLists nz;

	if (!nz.Built)
	{
		// Infantry
		for (auto const pType : InfantryTypeClass::Array)
		{
			const auto* pExt = TechnoTypeExt::ExtMap.Find(pType);
			if (pExt->Power != 0) nz.Inf.push_back(pType);
		}
		// Units
		for (auto const pType : UnitTypeClass::Array)
		{
			const auto* pExt = TechnoTypeExt::ExtMap.Find(pType);
			if (pExt->Power != 0) nz.Uni.push_back(pType);
		}
		// Aircraft
		for (auto const pType : AircraftTypeClass::Array)
		{
			const auto* pExt = TechnoTypeExt::ExtMap.Find(pType);
			if (pExt->Power != 0) nz.Air.push_back(pType);
		}
		nz.Built = true;
	}

	auto accumulate = [pThis](const auto& vec)
		{
			for (const auto* pType : vec)
			{
				const int count = pThis->CountOwnedAndPresent(pType);
				if (!count) continue;

				const auto* pExt = TechnoTypeExt::ExtMap.Find(pType);
				// same math as before
				if (pExt->Power > 0)
					pThis->PowerOutput += pExt->Power * count;
				else
					pThis->PowerDrain -= pExt->Power * count;
			}
		};

	// Only iterate non-zero-power types (much fewer)
	accumulate(nz.Inf);
	accumulate(nz.Uni);
	accumulate(nz.Air);
	// Buildings are already accounted for elsewhere.

	return 0;
}

DEFINE_HOOK(0x73E474, UnitClass_Unload_Storage, 0x6)
{
	GET(BuildingClass* const, pBuilding, EDI);
	GET(int const, idxTiberium, EBP);
	REF_STACK(float, amount, 0x1C);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pBuilding->Type);

	auto const storageTiberiumIndex = RulesExt::Global()->Storage_TiberiumIndex;

	if (pTypeExt->Refinery_UseStorage && storageTiberiumIndex >= 0)
	{
		BuildingExt::StoreTiberium(pBuilding, amount, idxTiberium, storageTiberiumIndex);
		amount = 0.0f;
	}

	return 0;
}

namespace RecalcCenterTemp
{
	HouseExt::ExtData* pExtData;
}

DEFINE_HOOK(0x4FD166, HouseClass_RecalcCenter_SetContext, 0x5)
{
	GET(HouseClass* const, pThis, EDI);

	RecalcCenterTemp::pExtData = HouseExt::ExtMap.Find(pThis);

	return 0;
}

DEFINE_HOOK_AGAIN(0x4FD463, HouseClass_RecalcCenter_LimboDelivery, 0x6)
DEFINE_HOOK(0x4FD1CD, HouseClass_RecalcCenter_LimboDelivery, 0x6)
{
	enum { SkipBuilding1 = 0x4FD23B, SkipBuilding2 = 0x4FD4D5 };

	GET(BuildingClass* const, pBuilding, ESI);

	if (!MapClass::Instance.CoordinatesLegal(pBuilding->GetMapCoords()))
		return R->Origin() == 0x4FD1CD ? SkipBuilding1 : SkipBuilding2;

	auto const pExt = RecalcCenterTemp::pExtData;

	if (pExt && pExt->OwnsLimboDeliveredBuilding(pBuilding))
		return R->Origin() == 0x4FD1CD ? SkipBuilding1 : SkipBuilding2;

	return 0;
}

DEFINE_HOOK(0x4AC534, DisplayClass_ComputeStartPosition_IllegalCoords, 0x6)
{
	enum { SkipTechno = 0x4AC55B };

	GET(TechnoClass* const, pTechno, ECX);

	if (!MapClass::Instance.CoordinatesLegal(pTechno->GetMapCoords()))
		return SkipTechno;

	return 0;
}

#pragma region LimboTracking

// These hooks handle tracking objects that are limboed e.g not physically on the map or engaged in game logic updates.
// The objects are manually updated once after pre-placed objects have been parsed, buildings are ignored as the limboed pre-placed buildings
// are not relevant (walls that will be converted into overlays etc), after which automatic update on limbo/unlimbo and uninit is enabled.

namespace LimboTrackingTemp
{
	bool Enabled = false;
	bool IsBeingDeleted = false;
}

DEFINE_HOOK(0x687B18, ScenarioClass_ReadINI_StartTracking, 0x7)
{
	for (auto const pTechno : TechnoClass::Array)
	{
		auto const pType = pTechno->GetTechnoType();

		if (!pType->Insignificant && !pType->DontScore && pTechno->WhatAmI() != AbstractType::Building && pTechno->InLimbo)
		{
			auto const pOwnerExt = HouseExt::ExtMap.Find(pTechno->Owner);
			pOwnerExt->AddToLimboTracking(pType);
		}
	}

	LimboTrackingTemp::Enabled = true;

	return 0;
}

void __fastcall TechnoClass_UnInit_Wrapper(TechnoClass* pThis)
{

	if (LimboTrackingTemp::Enabled && pThis->InLimbo)
	{
		auto const pType = pThis->GetTechnoType();

		if (!pType->Insignificant && !pType->DontScore)
			HouseExt::ExtMap.Find(pThis->Owner)->RemoveFromLimboTracking(pType);
	}

	LimboTrackingTemp::IsBeingDeleted = true;
	pThis->ObjectClass::UnInit();
	LimboTrackingTemp::IsBeingDeleted = false;
}

DEFINE_FUNCTION_JUMP(CALL, 0x4DE60B, TechnoClass_UnInit_Wrapper);   // FootClass
DEFINE_FUNCTION_JUMP(VTABLE, 0x7E3FB4, TechnoClass_UnInit_Wrapper); // BuildingClass

DEFINE_HOOK(0x6F6BC9, TechnoClass_Limbo_AddTracking, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);

	auto const pType = pThis->GetTechnoType();

	if (LimboTrackingTemp::Enabled && !pType->Insignificant && !pType->DontScore && !LimboTrackingTemp::IsBeingDeleted)
	{
		auto const pOwnerExt = HouseExt::ExtMap.Find(pThis->Owner);
		pOwnerExt->AddToLimboTracking(pType);
	}

	return 0;
}

DEFINE_HOOK(0x6F6D85, TechnoClass_Unlimbo_RemoveTracking, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);

	auto const pType = pThis->GetTechnoType();
	auto const pExt = TechnoExt::ExtMap.Find(pThis);

	if (LimboTrackingTemp::Enabled && !pType->Insignificant && !pType->DontScore && pExt->HasBeenPlacedOnMap)
	{
		auto const pOwnerExt = HouseExt::ExtMap.Find(pThis->Owner);
		pOwnerExt->RemoveFromLimboTracking(pType);
	}
	else if (!pExt->HasBeenPlacedOnMap)
	{
		pExt->HasBeenPlacedOnMap = true;

		if (pExt->TypeExtData->AutoDeath_Behavior.isset())
			ScenarioExt::Global()->AutoDeathObjects.push_back(pExt);
	}

	return 0;
}

DEFINE_HOOK(0x7015C9, TechnoClass_Captured_UpdateTracking, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);
	GET(HouseClass* const, pNewOwner, EBP);

	auto const pType = pThis->GetTechnoType();
	auto const pExt = TechnoExt::ExtMap.Find(pThis);
	auto const pTypeExt = pExt->TypeExtData;
	auto const pOwnerExt = HouseExt::ExtMap.Find(pThis->Owner);
	auto const pNewOwnerExt = HouseExt::ExtMap.Find(pNewOwner);

	if (LimboTrackingTemp::Enabled && !pType->Insignificant && !pType->DontScore && pThis->InLimbo)
	{
		pOwnerExt->RemoveFromLimboTracking(pType);
		pNewOwnerExt->AddToLimboTracking(pType);
	}

	if (pTypeExt->Harvester_Counted)
	{
		auto& vec = pOwnerExt->OwnedCountedHarvesters;
		vec.erase(std::remove(vec.begin(), vec.end(), pThis), vec.end());

		pNewOwnerExt->OwnedCountedHarvesters.push_back(pThis);
	}

	if (const auto pMe = generic_cast<FootClass*, true>(pThis))
	{
		const bool I_am_human = pThis->Owner->IsControlledByHuman();

		if (I_am_human != pNewOwner->IsControlledByHuman())
		{
			if (const auto pConvertTo = I_am_human
				? pTypeExt->Convert_HumanToComputer.Get()
				: pTypeExt->Convert_ComputerToHuman.Get())
			{
				if (pConvertTo->WhatAmI() == pType->WhatAmI())
					TechnoExt::ConvertToType(pMe, pConvertTo);
			}

			if (!I_am_human)
				TechnoExt::ChangeOwnerMissionFix(pMe);
		}
	}

	for (const auto& pTrail : pExt->LaserTrails)
	{
		if (pTrail->Type->IsHouseColor)
			pTrail->CurrentColor = pNewOwner->LaserColor;
	}

	return 0;
}

#pragma endregion

DEFINE_HOOK(0x65EB8D, HouseClass_SendSpyPlanes_PlaceAircraft, 0x6)
{
	enum { SkipGameCode = 0x65EBE5, SkipGameCodeNoSuccess = 0x65EC12 };

	GET(AircraftClass* const, pAircraft, ESI);
	GET(CellStruct const, edgeCell, EDI);

	const bool result = AircraftExt::PlaceReinforcementAircraft(pAircraft, edgeCell);

	return result ? SkipGameCode : SkipGameCodeNoSuccess;
}

DEFINE_HOOK(0x65E997, HouseClass_SendAirstrike_PlaceAircraft, 0x6)
{
	enum { SkipGameCode = 0x65E9EE, SkipGameCodeNoSuccess = 0x65EA8B };

	GET(AircraftClass* const, pAircraft, ESI);
	GET(CellStruct const, edgeCell, EDI);

	const bool result = AircraftExt::PlaceReinforcementAircraft(pAircraft, edgeCell);

	return result ? SkipGameCode : SkipGameCodeNoSuccess;
}

// Vanilla and Ares all only hardcoded to find factory with BuildCat::DontCare...
static inline bool CheckShouldDisableDefensesCameo(HouseClass* pHouse, TechnoTypeClass* pType)
{
	if (const auto pBuildingType = abstract_cast<BuildingTypeClass*>(pType))
	{
		if (pBuildingType->BuildCat == BuildCat::Combat)
		{
			auto count = 0;

			if (const auto pFactory = pHouse->Primary_ForDefenses)
			{
				count = pFactory->CountTotal(pBuildingType);

				if (pFactory->Object && pFactory->Object->GetType() == pBuildingType && pBuildingType->BuildLimit > 0)
					--count;
			}

			auto buildLimitRemaining = [](HouseClass* pHouse, BuildingTypeClass* pBldType)
			{
				const auto BuildLimit = pBldType->BuildLimit;

				if (BuildLimit >= 0)
					return BuildLimit - BuildingTypeExt::CountOwnedNowWithDeployOrUpgrade(pBldType, pHouse);
				else
					return -BuildLimit - pHouse->CountOwnedEver(pBldType);
			};

			if (buildLimitRemaining(pHouse, pBuildingType) - count <= 0)
				return true;
		}
	}

	return false;
}

DEFINE_HOOK(0x50B669, HouseClass_ShouldDisableCameo_GreyCameo, 0x5)
{
	GET(HouseClass*, pThis, ECX);
	GET_STACK(TechnoTypeClass*, pType, 0x4);
	GET(const bool, aresDisable, EAX);

	if (aresDisable || !pType)
		return 0;

	// Check quick defense limit first; then the full group logic.
	if (CheckShouldDisableDefensesCameo(pThis, pType) || HouseExt::ReachedBuildLimit(pThis, pType, false))
		R->EAX(true);

	return 0;
}

DEFINE_HOOK(0x4FD77C, HouseClass_ExpertAI_Superweapons, 0x5)
{
	enum { SkipSWProcess = 0x4FD7A0 };

	if (RulesExt::Global()->AISuperWeaponDelay.isset())
		return SkipSWProcess;

	return 0;
}

DEFINE_HOOK(0x4F9038, HouseClass_AI_Superweapons, 0x5)
{
	GET(HouseClass*, pThis, ESI);

	if (!RulesExt::Global()->AISuperWeaponDelay.isset() || pThis->IsControlledByHuman() || pThis->Type->MultiplayPassive)
		return 0;

	const int delay = RulesExt::Global()->AISuperWeaponDelay.Get();

	if (delay > 0)
	{
		auto const pExt = HouseExt::ExtMap.Find(pThis);

		if (pExt->AISuperWeaponDelayTimer.HasTimeLeft())
			return 0;

		pExt->AISuperWeaponDelayTimer.Start(delay);
	}

	if (!SessionClass::IsCampaign() || pThis->IQLevel2 >= RulesClass::Instance->SuperWeapons)
		pThis->AI_TryFireSW();

	return 0;
}

DEFINE_HOOK_AGAIN(0x4FFA99, HouseClass_ExcludeFromMultipleFactoryBonus, 0x6)
DEFINE_HOOK(0x4FF9C9, HouseClass_ExcludeFromMultipleFactoryBonus, 0x6)
{
	GET(BuildingClass*, pBuilding, ESI);

	auto const pType = pBuilding->Type;

	if (BuildingTypeExt::ExtMap.Find(pType)->ExcludeFromMultipleFactoryBonus)
	{
		GET(HouseClass*, pThis, EDI);
		GET(const bool, isNaval, ECX);

		auto const pExt = HouseExt::ExtMap.Find(pThis);
		pExt->UpdateNonMFBFactoryCounts(pType->Factory, R->Origin() == 0x4FF9C9, isNaval);
	}

	return 0;
}

DEFINE_HOOK(0x500910, HouseClass_GetFactoryCount, 0x5)
{
	enum { SkipGameCode = 0x50095D };

	GET(HouseClass*, pThis, ECX);
	GET_STACK(AbstractType, rtti, 0x4);
	GET_STACK(const bool, isNaval, 0x8);

	auto const pExt = HouseExt::ExtMap.Find(pThis);
	R->EAX(pExt->GetFactoryCountWithoutNonMFB(rtti, isNaval));

	return SkipGameCode;
}

// Sell all and all in.
DEFINE_HOOK(0x4FD8F7, HouseClass_UpdateAI_OnLastLegs, 0x10)
{
	enum { ret = 0x4FD907 };

	GET(HouseClass*, pThis, EBX);

	if (RulesExt::Global()->AIFireSale)
	{
		auto const pExt = HouseExt::ExtMap.Find(pThis);

		if (RulesExt::Global()->AIFireSaleDelay <= 0 || pExt->AIFireSaleDelayTimer.Completed())
			pThis->Fire_Sale();
		else if (!pExt->AIFireSaleDelayTimer.HasStarted())
			pExt->AIFireSaleDelayTimer.Start(RulesExt::Global()->AIFireSaleDelay);
	}

	if (RulesExt::Global()->AIAllToHunt)
		pThis->All_To_Hunt();

	return ret;
}
