#include "Body.h"

#include <BulletClass.h>
#include <UnitClass.h>
#include <SuperClass.h>
#include <GameOptionsClass.h>
#include <Ext/Anim/Body.h>
#include <Ext/House/Body.h>
#include <Ext/SWType/Body.h>
#include <Ext/WarheadType/Body.h>
#include <TacticalClass.h>
#include <PlanningTokenClass.h>
#include <algorithm> // swap-erase helper

// Small deterministic swap-erase. Order is not semantically used for RestrictedFactoryPlants.
template<typename TCont, typename TValue>
__forceinline void swap_erase_first(TCont& v, const TValue& value)
{
	for (size_t i = 0, n = v.size(); i < n; ++i)
	{
		if (v[i] == value)
		{
			if (i + 1 != n) { std::swap(v[i], v[n - 1]); }
			v.pop_back();
			return;
		}
	}
}

#pragma region Update

// After TechnoClass_AI
DEFINE_HOOK(0x43FE69, BuildingClass_AI, 0xA)
{
	GET(BuildingClass*, pThis, ESI);

	auto const pExt = BuildingExt::ExtMap.Find(pThis);
	pExt->DisplayIncomeString();
	pExt->ApplyPoweredKillSpawns();

	// Force airstrike targets to redraw every frame to account for tint intensity fluctuations.
	if (TechnoExt::ExtMap.Find(pThis)->AirstrikeTargetingMe)
		pThis->Mark(MarkType::Change);

	return 0;
}

DEFINE_HOOK(0x4403D4, BuildingClass_AI_ChronoSparkle, 0x6)
{
	enum { SkipGameCode = 0x44055D };

	GET(BuildingClass*, pThis, ESI);

	auto* const rules = RulesClass::Instance;
	if (!rules->ChronoSparkle1)
		return SkipGameCode;

	const auto displayPositions = RulesExt::Global()->ChronoSparkleBuildingDisplayPositions;
	auto* const pType = pThis->Type;
	const bool displayOnBuilding = (displayPositions & ChronoSparkleDisplayPosition::Building) != ChronoSparkleDisplayPosition::None;
	const bool displayOnSlots = (displayPositions & ChronoSparkleDisplayPosition::OccupantSlots) != ChronoSparkleDisplayPosition::None;
	const bool displayOnOccupants = (displayPositions & ChronoSparkleDisplayPosition::Occupants) != ChronoSparkleDisplayPosition::None;

	const int delay = RulesExt::Global()->ChronoSparkleDisplayDelay;
	const int occupantCount = displayOnSlots ? pType->MaxNumberOccupants : pThis->GetOccupantCount();
	const bool showOccupy = (occupantCount > 0) && (displayOnOccupants || displayOnSlots);

	if (showOccupy)
	{
		const CoordStruct renderCoords = pThis->GetRenderCoords();
		// vanilla used the ext vector only when MaxNumberOccupants > 10; preserve that
		auto const* const pTypeExt = (pType->MaxNumberOccupants <= 10) ? nullptr : BuildingTypeExt::ExtMap.Find(pType);
		const bool useTypeArray = (pTypeExt == nullptr);

		const Point2D* const flashesType = pType->MuzzleFlash; // fixed array in vanilla
		const int maxFlashes = useTypeArray
			? pType->MaxNumberOccupants
			: static_cast<int>(pTypeExt->OccupierMuzzleFlashes.size());

		// Clamp to available data to avoid any accidental OOB while keeping determinism
		const int count = (occupantCount < maxFlashes) ? occupantCount : maxFlashes;

		const int frame = Unsorted::CurrentFrame;
		auto* const tact = TacticalClass::Instance;

		for (int i = 0; i < count; ++i)
		{
			// identical cadence: spawn when (frame + i) % delay == 0
			if (((frame + i) % delay) == 0)
			{
				const Point2D muzzle = useTypeArray
					? flashesType[i]
					: pTypeExt->OccupierMuzzleFlashes[i]; // operator[] (no bounds checks in hot path)

				CoordStruct coords = renderCoords;
				const auto px = tact->ApplyMatrix_Pixel(muzzle);
				coords.X += px.X; coords.Y += px.Y;

				if (auto* const a = GameCreate<AnimClass>(rules->ChronoSparkle1, coords))
				{
					a->ZAdjust = -200;
				}
			}
		}
	}

	// building-center sparkle (unchanged semantics)
	if ((!showOccupy || displayOnBuilding) && ((Unsorted::CurrentFrame % delay) == 0))
	{
		GameCreate<AnimClass>(rules->ChronoSparkle1, pThis->GetCenterCoords());
	}

	return SkipGameCode;
}

#pragma endregion

DEFINE_HOOK(0x443C81, BuildingClass_ExitObject_InitialClonedHealth, 0x7)
{
	GET(BuildingClass*, pBuilding, ESI);

	if (auto const pInf = abstract_cast<InfantryClass*>(R->EDI<FootClass*>()))
	{
		if (pBuilding && pBuilding->Type->Cloning)
		{
			const double pct = GeneralUtils::GetRangedRandomOrSingleValue(
				BuildingTypeExt::ExtMap.Find(pBuilding->Type)->InitialStrength_Cloning);
			const int health = pInf->Type->Strength;
			const int strength = Math::clamp(static_cast<int>(health * pct), 1, health);
			pInf->Health = strength;
			pInf->EstimatedHealth = strength;
		}
	}

	return 0;
}

DEFINE_HOOK(0x449ADA, BuildingClass_MissionConstruction_DeployToFireFix, 0x0)
{
	GET(BuildingClass*, pThis, ESI);

	auto const pExt = BuildingExt::ExtMap.Find(pThis);

	if (pExt->DeployedTechno && pThis->LastTarget)
	{
		pThis->Target = pThis->LastTarget;
		pThis->QueueMission(Mission::Attack, false);
	}
	else
	{
		pThis->QueueMission(Mission::Guard, false);
	}

	return 0x449AE8;
}

#pragma region EMPulseCannon

namespace EMPulseCannonTemp
{
	int weaponIndex = 0;
}

DEFINE_HOOK(0x44CEEC, BuildingClass_Mission_Missile_EMPulseSelectWeapon, 0x6)
{
	enum { SkipGameCode = 0x44CEF8 };

	GET(BuildingClass*, pThis, ESI);

	int weaponIndexLocal = 0;
	auto const pExt = BuildingExt::ExtMap.Find(pThis);
	if (!pExt->EMPulseSW)
		return 0;

	auto const pSWExt = SWTypeExt::ExtMap.Find(pExt->EMPulseSW->Type);
	auto const pOwner = pThis->Owner;

	if (pSWExt->EMPulse_WeaponIndex >= 0)
	{
		weaponIndexLocal = pSWExt->EMPulse_WeaponIndex;
	}
	else
	{
		if (auto const pCell = MapClass::Instance.TryGetCellAt(pOwner->EMPTarget))
		{
			AbstractClass* pTarget = pCell;
			if (auto const pObject = pCell->GetContent())
				pTarget = pObject;

			weaponIndexLocal = pThis->SelectWeapon(pTarget);
		}
	}

	if (pSWExt->EMPulse_SuspendOthers)
	{
		auto const pHouseExt = HouseExt::ExtMap.Find(pOwner);
		const int index = pExt->EMPulseSW->Type->ArrayIndex;

		if (pHouseExt->SuspendedEMPulseSWs.count(index))
		{
			auto& super = pOwner->Supers;
			for (auto const& swidx : pHouseExt->SuspendedEMPulseSWs[index])
			{
				super[swidx]->IsSuspended = false;
			}
			pHouseExt->SuspendedEMPulseSWs[index].clear();
			pHouseExt->SuspendedEMPulseSWs.erase(index);
		}
	}

	pExt->EMPulseSW = nullptr;
	EMPulseCannonTemp::weaponIndex = weaponIndexLocal;
	R->EAX(pThis->GetWeapon(weaponIndexLocal));
	return SkipGameCode;
}

CoordStruct* __fastcall BuildingClass_GetFireCoords_Wrapper(BuildingClass* pThis, void* _, CoordStruct* pCrd, int /*weaponIndex*/)
{
	auto coords = MapClass::Instance.GetCellAt(pThis->Owner->EMPTarget)->GetCellCoords();
	pCrd = pThis->GetFLH(&coords, EMPulseCannonTemp::weaponIndex, *pCrd);
	return pCrd;
}

DEFINE_FUNCTION_JUMP(CALL6, 0x44D1F9, BuildingClass_GetFireCoords_Wrapper);

DEFINE_HOOK(0x44D455, BuildingClass_Mission_Missile_EMPulseBulletWeapon, 0x8)
{
	GET(WeaponTypeClass*, pWeapon, EBP);
	GET_STACK(BulletClass*, pBullet, STACK_OFFSET(0xF0, -0xA4));

	pBullet->SetWeaponType(pWeapon);
	return 0;
}

#pragma endregion

#pragma region KickOutStuckUnits

// Kick out stuck units when the factory building is not busy, only factory buildings can enter this hook
DEFINE_HOOK(0x450248, BuildingClass_UpdateFactory_KickOutStuckUnits, 0x6)
{
	GET(BuildingClass*, pThis, ESI);

	// Check every 15 frames for factories (original cadence preserved)
	if ((Unsorted::CurrentFrame % 15) == 0)
	{
		const auto* const pType = pThis->Type;
		if (pType->Factory != AbstractType::UnitType || !pType->WeaponsFactory || pType->Naval)
			return 0;
		if (pThis->QueuedMission == Mission::Unload)
			return 0;

		const auto mission = pThis->CurrentMission;
		const bool stuckWhileGuard = (mission == Mission::Guard) && !pThis->InLimbo;
		const bool stuckWhileUnload = (mission == Mission::Unload) && (pThis->MissionStatus == 1);
		if (stuckWhileGuard || stuckWhileUnload)
			BuildingExt::KickOutStuckUnits(pThis);
	}

	return 0;
}

// Should not kick out units if the factory building is in construction process
DEFINE_HOOK(0x4444A0, BuildingClass_KickOutUnit_NoKickOutInConstruction, 0xA)
{
	enum { ThisIsOK = 0x444565, ThisIsNotOK = 0x4444B3 };

	GET(BuildingClass* const, pThis, ESI);
	const auto mission = pThis->GetCurrentMission();
	return (mission == Mission::Unload || mission == Mission::Construction) ? ThisIsNotOK : ThisIsOK;
}

#pragma endregion

// Ares didn't have something like 0x7397E4 in its UnitDelivery code
DEFINE_HOOK(0x44FBBF, CreateBuildingFromINIFile_AfterCTOR_BeforeUnlimbo, 0x8)
{
	GET(BuildingClass* const, pBld, ESI);
	if (auto const pExt = BuildingExt::ExtMap.TryFind(pBld))
		pExt->IsCreatedFromMapFile = true;
	return 0;
}

DEFINE_HOOK(0x440B4F, BuildingClass_Unlimbo_SetShouldRebuild, 0x5)
{
	enum { ContinueCheck = 0x440B58, SkipSetShouldRebuild = 0x440B81 };

	if (SessionClass::IsCampaign())
	{
		GET(BuildingClass* const, pThis, ESI);

		// Preplaced structures are already managed before
		if (BuildingExt::ExtMap.Find(pThis)->IsCreatedFromMapFile)
			return SkipSetShouldRebuild;

		// Per-house dehardcoding: BaseNodes + SW-Delivery
		if (!HouseExt::ExtMap.Find(pThis->Owner)->RepairBaseNodes[GameOptionsClass::Instance.Difficulty].Get(RulesExt::Global()->RepairBaseNodes))
			return SkipSetShouldRebuild;
	}
	// Vanilla instruction: always repairable in other game modes
	return ContinueCheck;
}

DEFINE_HOOK(0x440EBB, BuildingClass_Unlimbo_NaturalParticleSystem_CampaignSkip, 0x5)
{
	enum { DoNotCreateParticle = 0x440F61 };
	GET(BuildingClass* const, pThis, ESI);
	return BuildingExt::ExtMap.Find(pThis)->IsCreatedFromMapFile ? DoNotCreateParticle : 0;
}

DEFINE_HOOK(0x4519A2, BuildingClass_UpdateAnim_SetParentBuilding, 0x6)
{
	GET(BuildingClass*, pThis, ESI);
	GET(AnimClass*, pAnim, EBP);

	AnimExt::ExtMap.Find(pAnim)->ParentBuilding = pThis;
	TechnoExt::ExtMap.Find(pThis)->AnimRefCount++;

	return 0;
}

DEFINE_HOOK(0x43D6E5, BuildingClass_Draw_ZShapePointMove, 0x5)
{
	enum { Apply = 0x43D6EF, Skip = 0x43D712 };

	GET(Mission, mission, EAX);

	if ((mission != Mission::Selling && mission != Mission::Construction))
		return Apply;

	GET(BuildingClass*, pThis, ESI);

	if (BuildingTypeExt::ExtMap.Find(pThis->Type)->ZShapePointMove_OnBuildup)
		return Apply;

	return Skip;
}

DEFINE_HOOK(0x4511D6, BuildingClass_AnimationAI_SellBuildup, 0x7)
{
	enum { Skip = 0x4511E6, Continue = 0x4511DF };

	GET(BuildingClass*, pThis, ESI);
	return BuildingTypeExt::ExtMap.Find(pThis->Type)->SellBuildupLength == pThis->Animation.Value ? Continue : Skip;
}

#pragma region FactoryPlant

DEFINE_HOOK(0x441501, BuildingClass_Unlimbo_FactoryPlant, 0x6)
{
	enum { Skip = 0x441553 };

	GET(BuildingClass*, pThis, ESI);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	if (!pTypeExt->FactoryPlant_AllowTypes.empty() || !pTypeExt->FactoryPlant_DisallowTypes.empty())
	{
		auto const pHouseExt = HouseExt::ExtMap.Find(pThis->Owner);
		pHouseExt->RestrictedFactoryPlants.push_back(pThis);
		return Skip;
	}

	return 0;
}

DEFINE_HOOK(0x448A31, BuildingClass_Captured_FactoryPlant1, 0x6)
{
	enum { Skip = 0x448A78 };

	GET(BuildingClass*, pThis, ESI);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	if (!pTypeExt->FactoryPlant_AllowTypes.empty() || !pTypeExt->FactoryPlant_DisallowTypes.empty())
	{
		auto const pHouseExt = HouseExt::ExtMap.Find(pThis->Owner);
		if (!pHouseExt->RestrictedFactoryPlants.empty())
		{
			// Faster deterministic removal
			swap_erase_first(pHouseExt->RestrictedFactoryPlants, pThis);
		}
		return Skip;
	}

	return 0;
}

DEFINE_HOOK(0x449149, BuildingClass_Captured_FactoryPlant2, 0x6)
{
	enum { Skip = 0x449197 };

	GET(BuildingClass*, pThis, ESI);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	if (!pTypeExt->FactoryPlant_AllowTypes.empty() || !pTypeExt->FactoryPlant_DisallowTypes.empty())
	{
		GET(HouseClass*, pNewOwner, EBP);
		HouseExt::ExtMap.Find(pNewOwner)->RestrictedFactoryPlants.push_back(pThis);
		return Skip;
	}

	return 0;
}

#pragma endregion

#pragma region DestroyableObstacle

template <bool remove = false>
static __forceinline void RecalculateCells(BuildingClass* pThis)
{
	auto const cells = BuildingExt::GetFoundationCells(pThis, pThis->GetMapCoords());
	auto& map = MapClass::Instance;

	for (auto const& cell : cells)
	{
		if (auto const pCell = map.TryGetCellAt(cell))
		{
			pCell->RecalcAttributes(DWORD(-1));

			if constexpr (remove)
			{
				map.ResetZones(cell);
			}
			else
			{
				map.RecalculateZones(cell);
			}

			map.RecalculateSubZones(cell);
		}
	}
}

DEFINE_HOOK(0x440D01, BuildingClass_Unlimbo_DestroyableObstacle, 0x6)
{
	GET(BuildingClass*, pThis, ESI);

	if (BuildingTypeExt::ExtMap.Find(pThis->Type)->IsDestroyableObstacle)
		RecalculateCells<false>(pThis);

	return 0;
}

DEFINE_HOOK(0x445D87, BuildingClass_Limbo_DestroyableObstacle, 0x6)
{
	GET(BuildingClass*, pThis, ESI);

	if (BuildingTypeExt::ExtMap.Find(pThis->Type)->IsDestroyableObstacle)
		RecalculateCells<true>(pThis);

	// only remove animation when the building is destroyed or sold
	if (pThis->Health > 0 && pThis->IsAlive && pThis->GetCurrentMission() != Mission::Selling)
		return 0;

	for (auto& bAnim : pThis->Anims)
	{
		if (bAnim && VTable::Get(bAnim) == 0x7E3354)
		{
			bAnim->UnInit();
			bAnim = nullptr;
		}
	}

	return 0;
}

DEFINE_HOOK(0x483D8E, CellClass_CheckPassability_DestroyableObstacle, 0x6)
{
	enum { IsBlockage = 0x483CD4 };

	GET(BuildingClass*, pBuilding, ESI);

	if (BuildingTypeExt::ExtMap.Find(pBuilding->Type)->IsDestroyableObstacle)
		return IsBlockage;

	return 0;
}

#pragma endregion

#pragma region UnitRepair

namespace UnitRepairTemp
{
	bool SeparateRepair = false;
}

DEFINE_HOOK(0x44C836, BuildingClass_Mission_Repair_UnitReload, 0x6)
{
	GET(BuildingClass*, pThis, EBP);

	auto const pType = pThis->Type;
	if (pType->UnitReload)
	{
		auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pType);

		if (pTypeExt->Units_RepairRate.isset())
		{
			const double repairRate = pTypeExt->Units_RepairRate.Get();
			if (repairRate < 0.0)
				return 0;

			const int rate = static_cast<int>(Math::max(repairRate * 900, 1));
			if (!(Unsorted::CurrentFrame % rate))
			{
				UnitRepairTemp::SeparateRepair = true;

				const int cap = pThis->RadioLinks.Capacity;
				for (auto i = 0; i < cap; ++i)
				{
					if (auto const pLink = pThis->GetNthLink(i))
					{
						if (!pLink->IsInAir()
							&& pLink->Health < pLink->GetTechnoType()->Strength
							&& pThis->SendCommand(RadioCommand::QueryMoving, pLink) == RadioCommand::AnswerPositive)
						{
							pThis->SendCommand(RadioCommand::RequestRepair, pLink);
						}
					}
				}

				UnitRepairTemp::SeparateRepair = false;
			}
		}
	}

	return 0;
}

DEFINE_HOOK(0x44B8F1, BuildingClass_Mission_Repair_Hospital, 0x6)
{
	enum { SkipGameCode = 0x44B8F7 };

	GET(BuildingClass*, pThis, EBP);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);
	const double repairRate = pTypeExt->Units_RepairRate.Get(RulesClass::Instance->IRepairRate);
	__asm { fld repairRate }

	return SkipGameCode;
}

DEFINE_HOOK(0x44BD38, BuildingClass_Mission_Repair_UnitRepair, 0x6)
{
	enum { SkipGameCode = 0x44BD3E };

	GET(BuildingClass*, pThis, EBP);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);
	const double repairRate = pTypeExt->Units_RepairRate.Get(RulesClass::Instance->URepairRate);
	__asm { fld repairRate }

	return SkipGameCode;
}

DEFINE_HOOK(0x6F4D1A, TechnoClass_ReceiveCommand_Repair, 0x5)
{
	enum { SkipEffects = 0x6F4DE5 };

	GET_STACK(TechnoClass*, pFrom, STACK_OFFSET(0x18, 0x4));
	GET(TechnoClass*, pThis, ESI);
	GET(int, repairStep, EAX);

	if (auto const pBuilding = abstract_cast<BuildingClass*>(pFrom))
	{
		auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pBuilding->Type);

		if (pBuilding->Type->UnitReload && pTypeExt->Units_RepairRate.isset() && !UnitRepairTemp::SeparateRepair)
			return SkipEffects;

		repairStep = pTypeExt->Units_RepairStep.Get(repairStep);
		const double repairPercent = pTypeExt->Units_RepairPercent.Get(RulesClass::Instance->RepairPercent);
		int repairCost = 0;

		if (pTypeExt->Units_UseRepairCost.Get(pThis->WhatAmI() != AbstractType::Infantry))
		{
			auto const pType = pThis->GetTechnoType();
			repairCost = static_cast<int>((pType->GetCost() / (pType->Strength / static_cast<double>(repairStep))) * repairPercent);
			if (repairCost < 1) repairCost = 1;
		}

		R->EAX(repairStep);
		R->EBX(repairCost);
	}

	return 0;
}

#pragma endregion

#pragma region EnableBuildingProductionQueue

DEFINE_HOOK(0x6AB689, SelectClass_Action_SkipBuildingProductionCheck, 0x5)
{
	enum { SkipGameCode = 0x6AB6CE };
	return RulesExt::Global()->BuildingProductionQueue ? SkipGameCode : 0;
}

DEFINE_HOOK(0x4FA520, HouseClass_BeginProduction_SkipBuilding, 0x5)
{
	enum { SkipGameCode = 0x4FA553 };
	return RulesExt::Global()->BuildingProductionQueue ? SkipGameCode : 0;
}

DEFINE_HOOK(0x4FA612, HouseClass_BeginProduction_ForceRedrawStrip, 0x5)
{
	SidebarClass::Instance.SidebarBackgroundNeedsRedraw = true;
	return 0;
}

DEFINE_HOOK(0x4C9C7B, FactoryClass_QueueProduction_ForceCheckBuilding, 0x7)
{
	enum { SkipGameCode = 0x4C9C9E };
	return RulesExt::Global()->BuildingProductionQueue ? SkipGameCode : 0;
}

DEFINE_HOOK(0x4FAAD8, HouseClass_AbandonProduction_RewriteForBuilding, 0x8)
{
	enum { CheckSame = 0x4FAB3D, SkipCheck = 0x4FAB64, Return = 0x4FAC9B };

	GET_STACK(const bool, all, STACK_OFFSET(0x18, 0x10));
	GET(const int, index, EBX);
	GET(const BuildCat, buildCat, ECX);
	GET(const AbstractType, absType, EBP);
	GET(FactoryClass* const, pFactory, ESI);

	if (buildCat == BuildCat::DontCare || all)
	{
		const auto pType = TechnoTypeClass::GetByTypeAndIndex(absType, index);
		const auto firstRemoved = pFactory->RemoveOneFromQueue(pType);

		if (firstRemoved)
		{
			SidebarClass::Instance.SidebarBackgroundNeedsRedraw = true; // Added, force redraw strip
			SidebarClass::Instance.RepaintSidebar(SidebarClass::GetObjectTabIdx(absType, index, 0));

			if (all)
				while (pFactory->RemoveOneFromQueue(pType));
			else
				return Return;
		}
		return CheckSame;
	}

	if (!pFactory->Object)
		return SkipCheck;

	if (!pFactory->RemoveOneFromQueue(TechnoTypeClass::GetByTypeAndIndex(absType, index)))
		return CheckSame;

	SidebarClass::Instance.SidebarBackgroundNeedsRedraw = true; // Added, force redraw strip
	SidebarClass::Instance.RepaintSidebar(SidebarClass::GetObjectTabIdx(absType, index, 0));

	return Return;
}

DEFINE_HOOK(0x6A9C54, StripClass_DrawStrip_FindFactoryDehardCode, 0x6)
{
	GET(TechnoTypeClass* const, pType, ECX);
	LEA_STACK(BuildCat*, pBuildCat, STACK_OFFSET(0x490, -0x490));

	if (const auto pBuildingType = abstract_cast<BuildingTypeClass*>(pType))
		*pBuildCat = pBuildingType->BuildCat;

	return 0;
}

DEFINE_HOOK(0x6A9789, StripClass_DrawStrip_NoGreyCameo, 0x6)
{
	enum { ContinueCheck = 0x6A9799, SkipGameCode = 0x6A97FB };

	GET(TechnoTypeClass* const, pType, EBX);
	GET_STACK(const bool, clicked, STACK_OFFSET(0x48C, -0x475));

	if (!RulesExt::Global()->BuildingProductionQueue)
	{
		if (pType->WhatAmI() == AbstractType::BuildingType && clicked)
			return SkipGameCode;
	}
	else if (const auto pBuildingType = abstract_cast<BuildingTypeClass*, true>(pType))
	{
		if (const auto pFactory = HouseClass::CurrentPlayer->GetPrimaryFactory(AbstractType::BuildingType, pType->Naval, pBuildingType->BuildCat))
		{
			if (const auto pProduct = abstract_cast<BuildingClass*>(pFactory->Object))
			{
				if (pFactory->IsDone() && pProduct->Type != pType
					&& ((pProduct->Type->BuildCat != BuildCat::Combat) ^ (pBuildingType->BuildCat == BuildCat::Combat)))
					return SkipGameCode;
			}
		}
	}

	return ContinueCheck;
}

DEFINE_HOOK(0x6AA88D, StripClass_RecheckCameo_FindFactoryDehardCode, 0x6)
{
	GET(TechnoTypeClass* const, pType, EBX);
	LEA_STACK(BuildCat*, pBuildCat, STACK_OFFSET(0x158, -0x158));

	if (const auto pBuildingType = abstract_cast<BuildingTypeClass*>(pType))
		*pBuildCat = pBuildingType->BuildCat;

	return 0;
}

#pragma endregion

#pragma region BarracksExitCell

DEFINE_HOOK(0x44EFD8, BuildingClass_FindExitCell_BarracksExitCell, 0x6)
{
	enum { SkipGameCode = 0x44F13B, ReturnFromFunction = 0x44F037 };

	GET(BuildingClass*, pThis, EBX);
	REF_STACK(CellStruct, resultCell, STACK_OFFSET(0x30, -0x20));

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	if (pTypeExt->BarracksExitCell.isset())
	{
		const Point2D offset = pTypeExt->BarracksExitCell.Get();
		auto exitCell = pThis->GetMapCoords();
		exitCell.X += (short)offset.X;
		exitCell.Y += (short)offset.Y;

		if (MapClass::Instance.CoordinatesLegal(exitCell))
		{
			GET(TechnoClass*, pTechno, ESI);
			auto const pCell = MapClass::Instance.GetCellAt(exitCell);

			if (pTechno->IsCellOccupied(pCell, FacingType::None, -1, nullptr, true) == Move::OK)
			{
				resultCell = exitCell;
				return ReturnFromFunction;
			}
		}

		return SkipGameCode;
	}

	return 0;
}

DEFINE_HOOK(0x444B83, BuildingClass_ExitObject_BarracksExitCell, 0x7)
{
	enum { SkipGameCode = 0x444C7C };

	GET(BuildingClass*, pThis, ESI);
	GET(const int, xCoord, EBP);
	GET(const int, yCoord, EDX);
	REF_STACK(CoordStruct, resultCoords, STACK_OFFSET(0x140, -0x108));

	auto const pType = pThis->Type;
	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pType);

	if (pTypeExt->BarracksExitCell.isset())
	{
		auto const exitCoords = pType->ExitCoord;
		resultCoords = CoordStruct { xCoord + exitCoords.X, yCoord + exitCoords.Y, exitCoords.Z };
		return SkipGameCode;
	}

	return 0;
}

DEFINE_HOOK(0x54BC99, JumpjetLocomotionClass_Ascending_BarracksExitCell, 0x6)
{
	enum { Continue = 0x54BCA3 };

	GET(BuildingTypeClass*, pType, EAX);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pType);

	if (pTypeExt->BarracksExitCell.isset())
		return Continue;

	return 0;
}

#pragma endregion

#pragma region BuildingFiring

DEFINE_HOOK(0x44B630, BuildingClass_MissionAttack_AnimDelayedFire, 0x6)
{
	enum { JustFire = 0x44B6C4, VanillaCheck = 0 };
	GET(BuildingClass* const, pThis, ESI);
	return (pThis->CurrentBurstIndex != 0 && !BuildingTypeExt::ExtMap.Find(pThis->Type)->IsAnimDelayedBurst) ? JustFire : VanillaCheck;
}

#pragma endregion

#pragma region BuildingWaypoints

bool __fastcall BuildingTypeClass_CanUseWaypoint(BuildingTypeClass* /*pThis*/)
{
	return RulesExt::Global()->BuildingWaypoints;
}
DEFINE_FUNCTION_JUMP(VTABLE, 0x7E4610, BuildingTypeClass_CanUseWaypoint)

DEFINE_HOOK(0x4AE95E, DisplayClass_sub_4AE750_DisallowBuildingNonAttackPlanning, 0x5)
{
	enum { SkipGameCode = 0x4AE982 };

	GET(ObjectClass* const, pObject, ECX);
	LEA_STACK(CellStruct*, pCell, STACK_OFFSET(0x20, 0x8));

	auto action = pObject->MouseOverCell(pCell);

	if (!PlanningNodeClass::PlanningModeActive || pObject->WhatAmI() != AbstractType::Building || action == Action::Attack)
		pObject->CellClickedAction(action, pCell, pCell, false);

	return SkipGameCode;
}

#pragma endregion

DEFINE_HOOK(0x4400F9, BuildingClass_AI_UpdateOverpower, 0x6)
{
	enum { SkipGameCode = 0x44019D };

	GET(BuildingClass*, pThis, ESI);

	if (!pThis->Type->Overpowerable)
		return SkipGameCode;

	int overPower = 0;

	for (int idx = pThis->Overpowerers.Count - 1; idx >= 0; --idx)
	{
		const auto pCharger = pThis->Overpowerers[idx];

		if (pCharger->Target != pThis)
		{
			pThis->Overpowerers.RemoveItem(idx);
			continue;
		}

		const auto pWeapon = pCharger->GetWeapon(1)->WeaponType;

		if (!pWeapon || !pWeapon->Warhead || !pWeapon->Warhead->ElectricAssault)
		{
			pThis->Overpowerers.RemoveItem(idx);
			continue;
		}

		const auto pWHExt = WarheadTypeExt::ExtMap.Find(pWeapon->Warhead);
		overPower += pWHExt->ElectricAssaultLevel;
	}

	const auto pBuildingTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);
	const int charge = pBuildingTypeExt->Overpower_ChargeWeapon;

	if (charge >= 0)
	{
		const int keepOnline = pBuildingTypeExt->Overpower_KeepOnline;
		pThis->IsOverpowered = overPower >= keepOnline + charge
			|| (pThis->Owner->GetPowerPercentage() == 1.0 && pThis->HasPower && overPower >= charge);
	}
	else
	{
		pThis->IsOverpowered = false;
	}

	return SkipGameCode;
}

DEFINE_HOOK_AGAIN(0x45563B, BuildingClass_IsPowerOnline_Overpower, 0x6)
DEFINE_HOOK(0x4555E4, BuildingClass_IsPowerOnline_Overpower, 0x6)
{
	enum { LowPower = 0x4556BE, Continue1 = 0x4555F0, Continue2 = 0x455643 };

	GET(BuildingClass*, pThis, ESI);
	const auto pBuildingTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);
	const int keepOnline = pBuildingTypeExt->Overpower_KeepOnline;

	if (keepOnline < 0)
		return LowPower;

	int overPower = 0;

	for (const auto pCharger : pThis->Overpowerers)
	{
		const auto pWeapon = pCharger->GetWeapon(1)->WeaponType;
		if (pWeapon && pWeapon->Warhead)
		{
			const auto pWHExt = WarheadTypeExt::ExtMap.Find(pWeapon->Warhead);
			overPower += pWHExt->ElectricAssaultLevel;
		}
	}

	return overPower < keepOnline ? LowPower : (R->Origin() == 0x4555E4 ? Continue1 : Continue2);
}
