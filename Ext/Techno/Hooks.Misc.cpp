#include "Body.h"

#include <SpawnManagerClass.h>
#include <TunnelLocomotionClass.h>

#include <Ext/Anim/Body.h>

#pragma region SlaveManagerClass

// Issue #601
// Author : TwinkleStar
DEFINE_HOOK(0x6B0C2C, SlaveManagerClass_FreeSlaves_SlavesFreeSound, 0x5)
{
	GET(TechnoClass*, pSlave, EDI);

	const auto pTypeExt = TechnoExt::ExtMap.Find(pSlave)->TypeExtData;
	const int sound = pTypeExt->SlavesFreeSound.Get(RulesClass::Instance->SlavesFreeSound);
	if (sound != -1)
		VocClass::PlayAt(sound, pSlave->Location);

	return 0x6B0C65;
}

DEFINE_HOOK(0x6B0B9C, SlaveManagerClass_Killed_DecideOwner, 0x6)
{
	enum { KillTheSlave = 0x6B0BDF, ChangeSlaveOwner = 0x6B0BB4 };

	GET(InfantryClass*, pSlave, ESI);

	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pSlave->Type);

	switch (pTypeExt->Slaved_OwnerWhenMasterKilled.Get())
	{
	case SlaveChangeOwnerType::Suicide:
		return KillTheSlave;

	case SlaveChangeOwnerType::Master:
		R->EAX(pSlave->Owner);
		return ChangeSlaveOwner;

	case SlaveChangeOwnerType::Neutral:
		if (const auto pNeutral = HouseClass::FindNeutral())
		{
			R->EAX(pNeutral);
			return ChangeSlaveOwner;
		}

	default: // SlaveChangeOwnerType::Killer
		break;
	}

	return 0x0;
}

// Fix slaves cannot always suicide due to armor multiplier or something
DEFINE_PATCH(0x6B0BF7,
	0x6A, 0x01  // push 1       // ignoreDefense=false->true
);

#pragma endregion

#pragma region SpawnManagerClass

DEFINE_HOOK(0x6B7265, SpawnManagerClass_AI_UpdateTimer, 0x6)
{
	GET(SpawnManagerClass* const, pThis, ESI);

	auto const pOwner = pThis->Owner;

	if (pOwner && pThis->Status == SpawnManagerStatus::Launching && pThis->CountDockedSpawns() != 0)
	{
		auto const pTypeExt = TechnoExt::ExtMap.Find(pOwner)->TypeExtData;

		if (pTypeExt->Spawner_DelayFrames.isset())
			R->EAX(std::min(pTypeExt->Spawner_DelayFrames.Get(), 10));

	}

	return 0;
}

DEFINE_HOOK_AGAIN(0x6B73BE, SpawnManagerClass_AI_SpawnTimer, 0x6)
DEFINE_HOOK(0x6B73AD, SpawnManagerClass_AI_SpawnTimer, 0x5)
{
	GET(SpawnManagerClass* const, pThis, ESI);

	if (auto const pOwner = pThis->Owner)
	{
		auto const pTypeExt = TechnoExt::ExtMap.Find(pOwner)->TypeExtData;

		if (pTypeExt->Spawner_DelayFrames.isset())
			R->ECX(pTypeExt->Spawner_DelayFrames.Get());
	}

	return 0;
}

DEFINE_HOOK_AGAIN(0x6B769F, SpawnManagerClass_AI_InitDestination, 0x7)
DEFINE_HOOK(0x6B7600, SpawnManagerClass_AI_InitDestination, 0x6)
{
	enum { SkipGameCode1 = 0x6B760E, SkipGameCode2 = 0x6B76DE };

	GET(SpawnManagerClass* const, pThis, ESI);
	GET(AircraftClass* const, pSpawnee, EDI);

	auto const pOwner = pThis->Owner;
	auto const pTypeExt = TechnoExt::ExtMap.Find(pOwner)->TypeExtData;

	if (pTypeExt->Spawner_AttackImmediately)
	{
		pSpawnee->SetTarget(pThis->Target);
		pSpawnee->QueueMission(Mission::Attack, true);
		pSpawnee->IsReturningFromAttackRun = false;
	}
	else
	{
		auto const mapCoords = pOwner->GetMapCoords();
		auto const pCell = MapClass::Instance.GetCellAt(mapCoords);
		pSpawnee->SetDestination(pCell->GetNeighbourCell(FacingType::North), true);
		pSpawnee->QueueMission(Mission::Move, false);
	}

	return R->Origin() == 0x6B7600 ? SkipGameCode1 : SkipGameCode2;
}

DEFINE_HOOK(0x6B6D44, SpawnManagerClass_Init_Spawns, 0x5)
{
	enum { Jump = 0x6B6DF0, Change = 0x6B6D53, Continue = 0 };
	GET(SpawnManagerClass*, pThis, ESI);
	GET_STACK(const size_t, i, STACK_OFFSET(0x1C, 0x4));

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Owner->GetTechnoType());

	if ((int) i >= pTypeExt->InitialSpawnsNumber.Get(pThis->SpawnCount))
	{
		GET(SpawnControl*, pControl, EBP);
		pControl->Unit = nullptr;
		pControl->SpawnTimer.Start(pThis->RegenRate);
		pControl->Status = SpawnNodeStatus::Dead;
		pThis->SpawnedNodes.AddItem(pControl);
		return Jump;
	}

	if (pTypeExt->Spawns_Queue.size() <= i || !pTypeExt->Spawns_Queue[i])
		return Continue;

	R->EAX(pTypeExt->Spawns_Queue[i]->CreateObject(pThis->Owner->Owner));
	return Change;
}

DEFINE_HOOK(0x6B78D3, SpawnManagerClass_Update_Spawns, 0x6)
{
	GET(SpawnManagerClass*, pThis, ESI);

	auto const pOwner = pThis->Owner;
	auto const pTypeExt = TechnoExt::ExtMap.Find(pOwner)->TypeExtData;

	if (pTypeExt->Spawns_Queue.empty())
		return 0;

	std::vector<AircraftTypeClass*> vec = pTypeExt->Spawns_Queue;

	for (auto const pNode : pThis->SpawnedNodes)
	{
		if (pNode->Unit)
		{
			auto it = std::find_if(vec.begin(), vec.end(), [=](auto pType) { return pType == pNode->Unit->GetTechnoType(); });
			if (it != vec.end())
				vec.erase(it);
		}
	}

	if (vec.empty() || !vec[0])
		return 0;

	R->EAX(vec[0]->CreateObject(pOwner->Owner));
	return 0x6B78EA;
}

DEFINE_HOOK(0x6B7282, SpawnManagerClass_AI_PromoteSpawns, 0x5)
{
	GET(SpawnManagerClass*, pThis, ESI);

	auto const pTypeExt = TechnoExt::ExtMap.Find(pThis->Owner)->TypeExtData;

	if (pTypeExt->Promote_IncludeSpawns)
	{
		for (auto const pNode : pThis->SpawnedNodes)
		{
			if (auto const pUnit = pNode->Unit)
			{
				const float unitVeterancy = pUnit->Veterancy.Veterancy;
				const float ownerVeterancy = pThis->Owner->Veterancy.Veterancy;

				if (unitVeterancy < ownerVeterancy)
					pUnit->Veterancy.Add(ownerVeterancy - unitVeterancy);
			}
		}
	}

	return 0;
}

DEFINE_HOOK(0x6B77B4, SpawnManagerClass_Update_RecycleSpawned, 0x7)
{
	enum { Recycle = 0x6B77FF, NoRecycle = 0x6B7838 };

	GET(SpawnManagerClass* const, pThis, ESI);
	GET(AircraftClass* const, pSpawner, EDI);
	GET(CellStruct* const, pCarrierMapCrd, EBP);

	auto const pCarrier = pThis->Owner;
	auto const pCarrierTypeExt = TechnoExt::ExtMap.Find(pCarrier)->TypeExtData;
	auto const spawnerCrd = pSpawner->GetCoords();

	auto shouldRecycleSpawned = [&]()
	{
		auto const& FLH = pCarrierTypeExt->Spawner_RecycleCoord;
		auto const recycleCrd = FLH != CoordStruct::Empty
			? TechnoExt::GetFLHAbsoluteCoords(pCarrier, FLH, pCarrierTypeExt->Spawner_RecycleOnTurret)
			: pCarrier->GetCoords();
		auto const deltaCrd = spawnerCrd - recycleCrd;
		const int recycleRange = pCarrierTypeExt->Spawner_RecycleRange.Get();

		if (recycleRange < 0)
		{
			// This is a fix to vanilla behavior. Buildings bigger than 1x1 will recycle the spawner correctly.
			// 182 is √2/2 * 256. 20 is same to vanilla behavior.
			return (pCarrier->WhatAmI() == AbstractType::Building)
				? (deltaCrd.X <= 182 && deltaCrd.Y <= 182 && deltaCrd.Z < 20)
				: (pSpawner->GetMapCoords() == *pCarrierMapCrd && deltaCrd.Z < 20);
		}

		return deltaCrd.Magnitude() <= recycleRange;
	};

	if (shouldRecycleSpawned())
	{
		if (pCarrierTypeExt->Spawner_RecycleAnim)
		{
			auto const pRecycleAnim = GameCreate<AnimClass>(pCarrierTypeExt->Spawner_RecycleAnim, spawnerCrd);
			auto const pAnimExt = AnimExt::ExtMap.Find(pRecycleAnim);
			auto const pSpawnOwner = pSpawner->Owner;
			pAnimExt->SetInvoker(pSpawner);
			AnimExt::SetAnimOwnerHouseKind(pRecycleAnim, pSpawnOwner, pSpawnOwner, false, true);
		}

		pSpawner->SetLocation(pCarrier->GetCoords());
		return Recycle;
	}

	return NoRecycle;
}

// Change destination to RecycleFLH.
DEFINE_HOOK(0x4D962B, FootClass_SetDestination_RecycleFLH, 0x5)
{
	GET(FootClass* const, pThis, EBP);

	auto const pCarrier = pThis->SpawnOwner;

	if (pCarrier && pCarrier == pThis->Destination) // This is a spawner returning to its carrier.
	{
		auto const pCarrierTypeExt = TechnoExt::ExtMap.Find(pCarrier)->TypeExtData;
		auto const& FLH = pCarrierTypeExt->Spawner_RecycleCoord;

		if (FLH != CoordStruct::Empty)
		{
			GET(CoordStruct*, pDestCrd, EAX);
			*pDestCrd += TechnoExt::GetFLHAbsoluteCoords(pCarrier, FLH, pCarrierTypeExt->Spawner_RecycleOnTurret) - pCarrier->GetCoords();
		}
	}

	return 0;
}

DEFINE_HOOK(0x6B74F0, SpawnManagerClass_AI_UseTurretFacing, 0x5)
{
	GET(SpawnManagerClass* const, pThis, ESI);

	auto const pTechno = pThis->Owner;

	if (pTechno->HasTurret() && TechnoExt::ExtMap.Find(pTechno)->TypeExtData->Spawner_UseTurretFacing)
		R->EAX(pTechno->SecondaryFacing.Current().Raw);

	return 0;
}

DEFINE_HOOK(0x418CF3, AircraftClass_Mission_Attack_ReturnToSpawnOwner, 0x5)
{
	enum { SkipGameCode = 0x418D00 };

	GET(AircraftClass* const, pThis, ESI);

	const auto pSpawnOwner = pThis->SpawnOwner;

	if (!pSpawnOwner)
		return 0;

	pThis->SetDestination(pSpawnOwner, true);
	pThis->QueueMission(Mission::Move, false);

	return SkipGameCode;
}

#pragma endregion

#pragma region WakeAnims

DEFINE_HOOK_AGAIN(0x69FEDC, Locomotion_Process_Wake, 0x6)  // Ship
DEFINE_HOOK_AGAIN(0x4B0814, Locomotion_Process_Wake, 0x6)  // Drive
DEFINE_HOOK(0x514AB4, Locomotion_Process_Wake, 0x6)  // Hover
{
	GET(ILocomotion* const, iloco, ESI);
	__assume(iloco != nullptr);
	const auto pTypeExt = TechnoExt::ExtMap.Find(static_cast<LocomotionClass*>(iloco)->LinkedTo)->TypeExtData;
	R->EDX(pTypeExt->Wake.Get(RulesClass::Instance->Wake));

	return R->Origin() + 0xC;
}

namespace GrappleUpdateTemp
{
	TechnoClass* pThis;
}

DEFINE_HOOK(0x629E9B, ParasiteClass_GrappleUpdate_MakeWake_SetContext, 0x5)
{
	GET(TechnoClass*, pThis, ECX);
	GrappleUpdateTemp::pThis = pThis;

	return 0;
}

DEFINE_HOOK(0x629FA3, ParasiteClass_GrappleUpdate_MakeWake, 0x6)
{
	const auto pTypeExt = TechnoExt::ExtMap.Find(GrappleUpdateTemp::pThis)->TypeExtData;
	R->EDX(pTypeExt->Wake_Grapple.Get(pTypeExt->Wake.Get(RulesClass::Instance->Wake)));

	return 0x629FA9;
}

DEFINE_HOOK(0x7365AD, UnitClass_Update_SinkingWake, 0x6)
{
	GET(UnitClass* const, pThis, ESI);

	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type);
	R->ECX(pTypeExt->Wake_Sinking.Get(pTypeExt->Wake.Get(RulesClass::Instance->Wake)));

	return 0x7365B3;
}

DEFINE_HOOK(0x737F05, UnitClass_ReceiveDamage_SinkingWake, 0x6)
{
	GET(UnitClass* const, pThis, ESI);

	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type);
	R->ECX(pTypeExt->Wake_Sinking.Get(pTypeExt->Wake.Get(RulesClass::Instance->Wake)));

	return 0x737F0B;
}

#pragma endregion

#pragma region TunnelLocomotionClass

DEFINE_HOOK(0x728F74, TunnelLocomotionClass_Process_KillAnims, 0x5)
{
	GET(ILocomotion*, pThis, ESI);

	const auto pLoco = static_cast<TunnelLocomotionClass*>(pThis);
	const auto pExt = TechnoExt::ExtMap.Find(pLoco->LinkedTo);
	pExt->IsBurrowed = true;

	if (const auto pShieldData = pExt->Shield.get())
		pShieldData->SetAnimationVisibility(false);

	for (auto const& attachEffect : pExt->AttachedEffects)
	{
		attachEffect->SetAnimationTunnelState(false);
	}

	return 0;
}

DEFINE_HOOK(0x728E5F, TunnelLocomotionClass_Process_RestoreAnims, 0x7)
{
	GET(ILocomotion*, pThis, ESI);

	const auto pLoco = static_cast<TunnelLocomotionClass*>(pThis);

	if (pLoco->State == TunnelLocomotionClass::State::PreDigOut)
	{
		const auto pExt = TechnoExt::ExtMap.Find(pLoco->LinkedTo);
		pExt->IsBurrowed = false;

		if (const auto pShieldData = pExt->Shield.get())
			pShieldData->SetAnimationVisibility(true);

		for (auto const& attachEffect : pExt->AttachedEffects)
		{
			attachEffect->SetAnimationTunnelState(true);
		}
	}

	return 0;
}

DEFINE_HOOK(0x728F89, TunnelLocomotionClass_Process_SubterraneanHeight1, 0x5)
{
	enum { Skip = 0x728FA6, Continue = 0x728F90 };

	GET(TechnoClass*, pLinkedTo, ECX);
	GET(const int, height, EAX);

	auto const pTypeExt = TechnoExt::ExtMap.Find(pLinkedTo)->TypeExtData;

	if (height == pTypeExt->SubterraneanHeight.Get(RulesExt::Global()->SubterraneanHeight))
		return Continue;

	return Skip;
}

DEFINE_HOOK(0x728FC6, TunnelLocomotionClass_Process_SubterraneanHeight2, 0x5)
{
	enum { Skip = 0x728FCD, Continue = 0x729021 };

	GET(TechnoClass*, pLinkedTo, ECX);
	GET(const int, height, EAX);

	auto const pTypeExt = TechnoExt::ExtMap.Find(pLinkedTo)->TypeExtData;

	if (height <= pTypeExt->SubterraneanHeight.Get(RulesExt::Global()->SubterraneanHeight))
		return Continue;

	return Skip;
}

DEFINE_HOOK(0x728FF2, TunnelLocomotionClass_Process_SubterraneanHeight3, 0x6)
{
	enum { SkipGameCode = 0x72900C };

	GET(TechnoClass*, pLinkedTo, ECX);
	GET(const int, heightOffset, EAX);
	REF_STACK(int, height, 0x14);

	auto const pTypeExt = TechnoExt::ExtMap.Find(pLinkedTo)->TypeExtData;
	const int subtHeight = pTypeExt->SubterraneanHeight.Get(RulesExt::Global()->SubterraneanHeight);
	height -= heightOffset;

	if (height < subtHeight)
		height = subtHeight;

	return SkipGameCode;
}

DEFINE_HOOK(0x7295E2, TunnelLocomotionClass_ProcessStateDigging_SubterraneanHeight, 0xC)
{
	enum { SkipGameCode = 0x7295EE };

	GET(TechnoClass*, pLinkedTo, EAX);
	REF_STACK(int, height, STACK_OFFSET(0x44, -0x8));

	auto const pTypeExt = TechnoExt::ExtMap.Find(pLinkedTo)->TypeExtData;
	height = pTypeExt->SubterraneanHeight.Get(RulesExt::Global()->SubterraneanHeight);

	return SkipGameCode;
}

#pragma endregion

#pragma region Disguise

DEFINE_HOOK(0x522790, InfantryClass_ClearDisguise_DefaultDisguise, 0x6)
{
	GET(InfantryClass*, pThis, ECX);
	auto const pExt = TechnoTypeExt::ExtMap.Find(pThis->Type);

	if (pExt->DefaultDisguise)
	{
		pThis->Disguise = pExt->DefaultDisguise;
		pThis->DisguisedAsHouse = pThis->Owner;
		pThis->Disguised = true;
		return 0x5227BF;
	}

	pThis->Disguised = false;

	return 0;
}

DEFINE_HOOK(0x74691D, UnitClass_UpdateDisguise_EMP, 0x6)
{
	GET(UnitClass*, pThis, ESI);
	// Remove mirage disguise if under emp or being flipped, approximately 15 deg
	// Deactivated mirage should still be able to keep disguise
	if (pThis->IsUnderEMP() || std::abs(pThis->AngleRotatedForwards) > 0.25 || std::abs(pThis->AngleRotatedSideways) > 0.25)
	{
		pThis->ClearDisguise();
		R->EAX(pThis->MindControlRingAnim);
		return 0x746AA5;
	}

	return 0x746931;
}

#pragma endregion

#pragma region AttackMindControlledDelay

bool __fastcall CanAttackMindControlled(TechnoClass* pControlled, TechnoClass* pRetaliator)
{
	const auto pMind = pControlled->MindControlledBy;

	if (!pMind || pRetaliator->Berzerk)
		return true;

	const auto pManager = pMind->CaptureManager;

	if (!pManager || !pRetaliator->Owner->IsAlliedWith(pManager->GetOriginalOwner(pControlled)))
		return true;

	return TechnoExt::ExtMap.Find(pControlled)->BeControlledThreatFrame <= Unsorted::CurrentFrame;
}

DEFINE_HOOK(0x7089E8, TechnoClass_AllowedToRetaliate_AttackMindControlledDelay, 0x6)
{
	enum { CannotRetaliate = 0x708B17 };

	GET(TechnoClass* const, pThis, ESI);
	GET(TechnoClass* const, pAttacker, EBP);

	return CanAttackMindControlled(pAttacker, pThis) ? 0 : CannotRetaliate;
}

DEFINE_HOOK(0x6F88BF, TechnoClass_CanAutoTargetObject_AttackMindControlledDelay, 0x6)
{
	enum { CannotSelect = 0x6F894F };

	GET(ObjectClass* const, pTarget, ESI);

	if (const auto pTechno = abstract_cast<TechnoClass*>(pTarget))
	{
		GET(TechnoClass* const, pThis, EDI);

		if (!CanAttackMindControlled(pTechno, pThis))
			return CannotSelect;
	}

	return 0;
}

#pragma endregion

#pragma region ExtendedGattlingRateDown

DEFINE_HOOK(0x70DE40, TechnoClass_GattlingValueRateDown_GattlingRateDownDelay, 0xA)
{
	enum { Return = 0x70DE62 };

	GET(BuildingClass* const, pThis, ECX);

	const auto pExt = TechnoExt::ExtMap.Find(pThis);
	const auto pTypeExt = pExt->TypeExtData;

	if (pTypeExt->RateDown_Delay < 0)
		return Return;

	++pExt->AccumulatedGattlingValue;
	int remain = pExt->AccumulatedGattlingValue;

	if (!pExt->ShouldUpdateGattlingValue)
		remain -= pTypeExt->RateDown_Delay;

	if (remain <= 0)
		return Return;

	// Time's up
	GET_STACK(int, rateDown, STACK_OFFSET(0x0, 0x4));
	pExt->AccumulatedGattlingValue = 0;
	pExt->ShouldUpdateGattlingValue = true;

	if (pThis->Ammo <= pTypeExt->RateDown_Cover_AmmoBelow)
		rateDown = pTypeExt->RateDown_Cover_Value;

	if (!rateDown)
	{
		pThis->GattlingValue = 0;
		return Return;
	}

	const int newValue = pThis->GattlingValue - (rateDown * remain);
	pThis->GattlingValue = (newValue <= 0) ? 0 : newValue;
	return Return;
}

DEFINE_HOOK(0x70DE70, TechnoClass_GattlingRateUp_GattlingRateDownReset, 0x5)
{
	GET(TechnoClass* const, pThis, ECX);

	const auto pExt = TechnoExt::ExtMap.Find(pThis);
	pExt->AccumulatedGattlingValue = 0;
	pExt->ShouldUpdateGattlingValue = false;

	return 0;
}

DEFINE_HOOK(0x70E01E, TechnoClass_GattlingRateDown_GattlingRateDownDelay, 0x6)
{
	enum { SkipGameCode = 0x70E04D };

	GET(TechnoClass* const, pThis, ESI);

	const auto pExt = TechnoExt::ExtMap.Find(pThis);
	const auto pTypeExt = pExt->TypeExtData;

	if (pTypeExt->RateDown_Delay < 0)
		return SkipGameCode;

	GET_STACK(const int, rateMult, STACK_OFFSET(0x10, 0x4));

	pExt->AccumulatedGattlingValue += rateMult;
	int remain = pExt->AccumulatedGattlingValue;

	if (!pExt->ShouldUpdateGattlingValue)
		remain -= pTypeExt->RateDown_Delay;

	if (remain <= 0 && rateMult)
		return SkipGameCode;

	// Time's up
	pExt->AccumulatedGattlingValue = 0;
	pExt->ShouldUpdateGattlingValue = true;

	if (!rateMult)
	{
		pThis->GattlingValue = 0;
		return SkipGameCode;
	}

	const auto rateDown = (pThis->Ammo <= pTypeExt->RateDown_Cover_AmmoBelow) ? pTypeExt->RateDown_Cover_Value.Get() : pTypeExt->OwnerObject()->RateDown;

	if (!rateDown)
	{
		pThis->GattlingValue = 0;
		return SkipGameCode;
	}

	const int newValue = pThis->GattlingValue - (rateDown * remain);
	pThis->GattlingValue = (newValue <= 0) ? 0 : newValue;
	return SkipGameCode;
}

#pragma endregion

#pragma region NoTurretUnitAlwaysTurnToTarget

DEFINE_HOOK(0x7410BB, UnitClass_GetFireError_CheckFacingError, 0x8)
{
	enum { NoNeedToCheck = 0x74132B, ContinueCheck = 0x7410C3 };

	GET(const FireError, fireError, EAX);

	if (fireError == FireError::OK)
		return ContinueCheck;

	GET(UnitClass* const, pThis, ESI);

	const auto pType = pThis->Type;

	if (!TechnoTypeExt::ExtMap.Find(pType)->NoTurret_TrackTarget.Get(RulesExt::Global()->NoTurret_TrackTarget))
		return NoNeedToCheck;

	return (fireError == FireError::REARM && !pType->Turret && !pThis->IsWarpingIn()) ? ContinueCheck : NoNeedToCheck;
}

#pragma endregion

#pragma region Misc

// I must not regroup my forces.
DEFINE_HOOK(0x739920, UnitClass_TryToDeploy_DisableRegroupAtNewConYard, 0x6)
{
	enum { SkipRegroup = 0x73992B, DoNotSkipRegroup = 0 };

	return RulesExt::Global()->GatherWhenMCVDeploy ? DoNotSkipRegroup : SkipRegroup;
}

DEFINE_HOOK(0x736234, UnitClass_ChronoSparkleDelay, 0x5)
{
	R->ECX(RulesExt::Global()->ChronoSparkleDisplayDelay);
	return 0x736239;
}

DEFINE_HOOK(0x51BAFB, InfantryClass_ChronoSparkleDelay, 0x5)
{
	R->ECX(RulesExt::Global()->ChronoSparkleDisplayDelay);
	return 0x51BB00;
}

DEFINE_HOOK_AGAIN(0x5F4718, ObjectClass_Select, 0x7)
DEFINE_HOOK(0x5F46AE, ObjectClass_Select, 0x7)
{
	GET(ObjectClass*, pThis, ESI);

	pThis->IsSelected = true;

	if (!Phobos::Config::ShowFlashOnSelecting)
		return 0;

	auto const duration = RulesExt::Global()->SelectionFlashDuration;

	if (duration > 0 && pThis->GetOwningHouse()->IsControlledByCurrentPlayer())
		pThis->Flash(duration);

	return 0;
}

DEFINE_HOOK(0x51B20E, InfantryClass_AssignTarget_FireOnce, 0x6)
{
	enum { SkipGameCode = 0x51B255 };

	GET(InfantryClass*, pThis, ESI);
	GET(AbstractClass*, pTarget, EBX);

	if (!pTarget)
	{
		auto const pExt = TechnoExt::ExtMap.Find(pThis);

		if (pExt->SkipTargetChangeResetSequence)
		{
			pThis->IsFiring = false;
			pExt->SkipTargetChangeResetSequence = false;
			return SkipGameCode;
		}
	}

	return 0;
}

// Update attached anim layers after parent unit changes layer.
void __fastcall DisplayClass_Submit_Wrapper(DisplayClass* pThis, void* _, ObjectClass* pObject)
{
	pThis->Submit(pObject);

	if (auto const pTechno = abstract_cast<TechnoClass*>(pObject))
		TechnoExt::UpdateAttachedAnimLayers(pTechno);
}

DEFINE_FUNCTION_JUMP(CALL, 0x54B18E, DisplayClass_Submit_Wrapper);  // JumpjetLocomotionClass_Process
DEFINE_FUNCTION_JUMP(CALL, 0x4CD4E7, DisplayClass_Submit_Wrapper);  // FlyLocomotionClass_Update

// Oct 26, 2024 - Starkku: Fixes SecondaryFire / SecondaryProne sequences not remapping to WetAttack in water.
// Ideally there would be WetAttackSecondary but adding new sequences would be a big undertaking.
// Also adds a toggle for not using water sequences at all
DEFINE_HOOK(0x51D7E0, InfantryClass_DoAction_Water, 0x5)
{
	enum { Continue= 0x51D7EC, SkipWaterSequences = 0x51D842, UseSwim = 0x51D83D, UseWetAttack = 0x51D82F };

	GET(InfantryClass*, pThis, ESI);
	GET(Sequence, sequence, EDI);

	R->EBP(0); // Restore overridden instructions.

	if (TechnoTypeExt::ExtMap.Find(pThis->Type)->OnlyUseLandSequences)
		return SkipWaterSequences;

	if (sequence == Sequence::Walk || sequence == Sequence::Crawl) // Restore overridden instructions.
		return UseSwim;
	else if (sequence == Sequence::SecondaryFire || sequence == Sequence::SecondaryProne)
		return UseWetAttack;

	return Continue;
}

bool __fastcall LocomotorCheckForBunkerable(TechnoTypeClass* pType)
{
	auto const loco = pType->Locomotor;

	// These locomotors either cause the game to crash or fail to enter the tank bunker properly.
	return loco != LocomotionClass::CLSIDs::Hover
		&& loco != LocomotionClass::CLSIDs::Mech
		&& loco != LocomotionClass::CLSIDs::Fly
		&& loco != LocomotionClass::CLSIDs::Droppod
		&& loco != LocomotionClass::CLSIDs::Rocket
		&& loco != LocomotionClass::CLSIDs::Ship;
}

DEFINE_HOOK(0x70FB73, FootClass_IsBunkerableNow_Dehardcode, 0x6)
{
	enum { CanEnter = 0x70FBAF, NoEnter = 0x70FB7D };

	GET(FootClass*, pThis, ESI);

	if (pThis->ParasiteEatingMe)
		return NoEnter;

	GET(TechnoTypeClass*, pType, EAX);

	if (!LocomotorCheckForBunkerable(pType))
		return NoEnter;

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);
	return pTypeExt->BunkerableAnyway ? CanEnter : 0;
}

DEFINE_HOOK(0x730D1F, DeployCommandClass_Execute_VoiceDeploy, 0x5)
{
	GET_STACK(const int, unitsToDeploy, STACK_OFFSET(0x18, -0x4));

	if (unitsToDeploy != 1)
		return 0;

	GET(TechnoClass* const, pThis, ESI);

	pThis->VoiceDeploy();

	return 0;
}

#pragma endregion
