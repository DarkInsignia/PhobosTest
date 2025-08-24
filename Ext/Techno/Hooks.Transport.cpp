#include "Body.h"

#include <Ext/Scenario/Body.h>

DEFINE_HOOK_AGAIN(0x6FA33C, TechnoClass_ThreatEvals_OpenToppedOwner, 0x6) // TechnoClass::AI
DEFINE_HOOK_AGAIN(0x6F89F4, TechnoClass_ThreatEvals_OpenToppedOwner, 0x6) // TechnoClass::EvaluateCell
DEFINE_HOOK_AGAIN(0x6F7EC2, TechnoClass_ThreatEvals_OpenToppedOwner, 0x6) // TechnoClass::EvaluateObject
DEFINE_HOOK(0x6F8FD7, TechnoClass_ThreatEvals_OpenToppedOwner, 0x5)       // TechnoClass::Greatest_Threat
{
	enum { SkipCheckOne = 0x6F8FDC, SkipCheckTwo = 0x6F7EDA, SkipCheckThree = 0x6F8A0F, SkipCheckFour = 0x6FA37A };

	TechnoClass* pThis = nullptr;
	auto returnAddress = SkipCheckOne;

	switch (R->Origin())
	{
	case 0x6F8FD7:
		pThis = R->ESI<TechnoClass*>();
		break;
	case 0x6F7EC2:
		pThis = R->EDI<TechnoClass*>();
		returnAddress = SkipCheckTwo;
		break;
	case 0x6F89F4:
		pThis = R->ESI<TechnoClass*>();
		returnAddress = SkipCheckThree;
	case 0x6FA33C:
		pThis = R->ESI<TechnoClass*>();
		returnAddress = SkipCheckFour;
	default:
		return 0;
	}

	if (auto const pTransport = pThis->Transporter)
	{
		if (TechnoExt::ExtMap.Find(pTransport)->TypeExtData->Passengers_SyncOwner)
			return returnAddress;
	}

	return 0;
}

DEFINE_HOOK(0x701881, TechnoClass_ChangeHouse_Passenger_SyncOwner, 0x5)
{
	GET(TechnoClass*, pThis, ESI);

	if (auto pPassenger = pThis->Passengers.GetFirstPassenger())
	{
		if (TechnoExt::ExtMap.Find(pThis)->TypeExtData->Passengers_SyncOwner)
		{
			const auto pOwner = pThis->Owner;

			do
			{
				pPassenger->SetOwningHouse(pOwner, false);
				pPassenger = abstract_cast<FootClass*>(pPassenger->NextObject);
			}
			while (pPassenger);
		}
	}

	return 0;
}

DEFINE_HOOK(0x71067B, TechnoClass_EnterTransport, 0x7)
{
	GET(TechnoClass*, pThis, ESI);
	GET(FootClass*, pPassenger, EDI);

	if (pPassenger)
	{
		auto const pType = pPassenger->GetTechnoType();
		auto const pExt = TechnoExt::ExtMap.Find(pPassenger);
		auto const whatAmI = pPassenger->WhatAmI();
		auto const pTransTypeExt = TechnoExt::ExtMap.Find(pThis)->TypeExtData;

		if (pTransTypeExt->Passengers_SyncOwner && pTransTypeExt->Passengers_SyncOwner_RevertOnExit)
			pExt->OriginalPassengerOwner = pPassenger->Owner;

		if (whatAmI != AbstractType::Aircraft && whatAmI != AbstractType::Building
			&& pType->Ammo > 0 && pExt->TypeExtData->ReloadInTransport)
		{
			ScenarioExt::Global()->TransportReloaders.push_back(pExt);
		}
	}

	return 0;
}

DEFINE_HOOK(0x4DE722, FootClass_LeaveTransport, 0x6)
{
	GET(TechnoClass*, pThis, ESI);
	GET(FootClass*, pPassenger, EAX);

	if (pPassenger)
	{
		auto const pType = pPassenger->GetTechnoType();
		auto const pExt = TechnoExt::ExtMap.Find(pPassenger);
		auto const whatAmI = pPassenger->WhatAmI();
		auto const pTransTypeExt = TechnoExt::ExtMap.Find(pThis)->TypeExtData;

		// Remove from transport reloader list before switching house
		if (whatAmI != AbstractType::Aircraft && whatAmI != AbstractType::Building
			&& pType->Ammo > 0 && pExt->TypeExtData->ReloadInTransport)
		{
			auto& vec = ScenarioExt::Global()->TransportReloaders;
			vec.erase(std::remove(vec.begin(), vec.end(), pExt), vec.end());
		}

		if (pTransTypeExt->Passengers_SyncOwner
			&& pTransTypeExt->Passengers_SyncOwner_RevertOnExit
			&& pExt->OriginalPassengerOwner)
		{
			pPassenger->SetOwningHouse(pExt->OriginalPassengerOwner, false);
		}
	}

	return 0;
}

// Has to be done here, before Ares survivor hook to take effect.
DEFINE_HOOK(0x737F80, UnitClass_ReceiveDamage_Cargo_SyncOwner, 0x6)
{
	GET(UnitClass*, pThis, ESI);

	if (auto pPassenger = pThis->Passengers.GetFirstPassenger())
	{
		auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type);

		if (pTypeExt->Passengers_SyncOwner && pTypeExt->Passengers_SyncOwner_RevertOnExit)
		{
			do
			{
				auto const pExt = TechnoExt::ExtMap.Find(pPassenger);

				if (pExt->OriginalPassengerOwner)
					pPassenger->SetOwningHouse(pExt->OriginalPassengerOwner, false);

				pPassenger = abstract_cast<FootClass*>(pPassenger->NextObject);
			}
			while (pPassenger);
		}
	}

	return 0;
}

DEFINE_HOOK(0x51DF82, InfantryClass_FireAt_ReloadInTransport, 0x6)
{
	GET(InfantryClass* const, pThis, ESI);

	if (pThis->Transporter)
	{
		auto const pType = pThis->Type;
		auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

		if (pTypeExt->ReloadInTransport && pType->Ammo > 0 && pThis->Ammo < pType->Ammo)
			pThis->StartReloading();
	}

	return 0;
}

// Customizable OpenTopped Properties
// Author: Otamaa
DEFINE_HOOK(0x6F72D2, TechnoClass_IsCloseEnoughToTarget_OpenTopped_RangeBonus, 0xC)
{
	GET(TechnoClass* const, pThis, ESI);

	if (auto const pTransport = pThis->Transporter)
	{
		auto const pExt = TechnoExt::ExtMap.Find(pTransport)->TypeExtData;
		R->EAX(pExt->OpenTopped_RangeBonus.Get(RulesClass::Instance->OpenToppedRangeBonus));
		return 0x6F72DE;
	}

	return 0;
}

DEFINE_HOOK(0x71A82C, TemporalClass_AI_Opentopped_WarpDistance, 0xC)
{
	GET(TemporalClass* const, pThis, ESI);

	if (auto const pTransport = pThis->Owner->Transporter)
	{
		auto const pExt = TechnoExt::ExtMap.Find(pTransport)->TypeExtData;
		R->EDX(pExt->OpenTopped_WarpDistance.Get(RulesClass::Instance->OpenToppedWarpDistance));
		return 0x71A838;
	}

	return 0;
}

DEFINE_HOOK(0x710552, TechnoClass_SetOpenTransportCargoTarget_ShareTarget, 0x6)
{
	enum { ReturnFromFunction = 0x71057F };

	GET(TechnoClass* const, pThis, ECX);
	GET_STACK(AbstractClass* const, pTarget, STACK_OFFSET(0x8, 0x4));

	if (pTarget)
	{
		auto const pTypeExt = TechnoExt::ExtMap.Find(pThis)->TypeExtData;

		if (!pTypeExt->OpenTopped_ShareTransportTarget)
			return ReturnFromFunction;
	}

	return 0;
}

#pragma region NoQueueUpToEnterAndUnload

// Use a square range because it doesn't seem necessary to calculate the circular range
static inline bool IsCloseEnoughToEnter(UnitClass* pTransport, FootClass* pPassenger)
{
	return (std::abs(pPassenger->Location.X - pTransport->Location.X) < 384
		&& std::abs(pPassenger->Location.Y - pTransport->Location.Y) < 384
		&& std::abs(pPassenger->Location.Z - pTransport->Location.Z) < Unsorted::CellHeight);
}

// Rewrite from 0x73758A, replace send RadioCommand::QueryCanEnter
static inline bool CanEnterNow(UnitClass* pTransport, FootClass* pPassenger)
{
	if (!pTransport->Owner->IsAlliedWith(pPassenger) || pTransport->IsBeingWarpedOut())
		return false;

	// Added to prevent unexpected enter action
	if (pTransport->OnBridge || pPassenger->Deactivated || pPassenger->IsUnderEMP())
		return false;

	if (pPassenger->IsMindControlled() || pPassenger->ParasiteEatingMe)
		return false;

	const auto pManager = pPassenger->CaptureManager;

	if (pManager && pManager->IsControllingSomething())
		return false;

	const auto pTransportType = pTransport->Type;

	// Added to fit with AmphibiousEnter
	if (pTransport->GetCell()->LandType == LandType::Water && !TechnoTypeExt::ExtMap.Find(pTransportType)->AmphibiousEnter.Get(RulesExt::Global()->AmphibiousEnter))
		return false;

	const bool bySize = TechnoTypeExt::ExtMap.Find(pTransportType)->Passengers_BySize;
	const int passengerSize = static_cast<int>(pPassenger->GetTechnoType()->Size);

	if (passengerSize > static_cast<int>(pTransportType->SizeLimit))
		return false;

	const int maxSize = pTransportType->Passengers;
	const int predictSize = bySize ? (pTransport->Passengers.GetTotalSize() + passengerSize) : (pTransport->Passengers.NumPassengers + 1);
	const auto pLink = abstract_cast<FootClass*>(pTransport->GetNthLink());
	const bool needCalculate = pLink && pLink != pPassenger && pLink->Destination == pTransport;

	// When the most important passenger is close, need to prevent overlap
	if (needCalculate)
	{
		if (IsCloseEnoughToEnter(pTransport, pLink))
			return (predictSize <= (maxSize - (bySize ? static_cast<int>(pLink->GetTechnoType()->Size) : 1)));

		if (predictSize > (maxSize - (bySize ? static_cast<int>(pLink->GetTechnoType()->Size) : 1)))
		{
			pLink->QueueMission(Mission::None, false);
			pLink->SetDestination(nullptr, true);
			pLink->SendCommand(RadioCommand::NotifyUnlink, pTransport);
		}
	}

	return predictSize <= maxSize;
}

// Rewrite from 0x51A21B/0x73A6D1
static inline void DoEnterNow(UnitClass* pTransport, FootClass* pPassenger)
{
	// Vanilla only for infantry, but why
	if (const auto pTag = pTransport->AttachedTag)
		pTag->RaiseEvent(TriggerEvent::EnteredBy, pPassenger, CellStruct::Empty);

	// Vanilla did not handle SpawnManager and SlaveManager, so I don't care about these here either
	pPassenger->SetArchiveTarget(nullptr);
	pPassenger->MissionAccumulateTime = 0;
	pPassenger->GattlingValue = 0;
	pPassenger->CurrentGattlingStage = 0;

	pPassenger->Limbo(); // Don't swap order casually
	pPassenger->OnBridge = false; // Don't swap order casually, important
	pPassenger->NextObject = nullptr; // Don't swap order casually, very important

	pPassenger->SetDestination(nullptr, true); // Added, to prevent passengers from return to board position when survive
	pPassenger->QueueUpToEnter = nullptr; // Added, to prevent passengers from wanting to get on after getting off
	pPassenger->FrozenStill = true; // Added, to prevent the vehicles from stacking together when unloading
	pPassenger->SetSpeedPercentage(0.0); // Added, to stop the passengers and let OpenTopped work normally

	const auto pPassengerType = pPassenger->GetTechnoType();

	// Reinstalling Locomotor can avoid various issues such as teleportation, ignoring commands, and automatic return
	while (LocomotionClass::End_Piggyback(pPassenger->Locomotor));

	if (const auto pNewLoco = LocomotionClass::CreateInstance(pPassengerType->Locomotor))
	{
		pPassenger->Locomotor = std::move(pNewLoco);
		pPassenger->Locomotor->Link_To_Object(pPassenger);
	}

	pTransport->AddPassenger(pPassenger); // Don't swap order casually, very very important
	pPassenger->Transporter = pTransport;

	if (pTransport->Type->OpenTopped)
		pTransport->EnteredOpenTopped(pPassenger);

	if (pPassengerType->OpenTopped)
		pPassenger->SetTargetForPassengers(nullptr);

	pPassenger->Undiscover();
}

// The core part of the fast enter action
DEFINE_HOOK(0x4DA8A0, FootClass_Update_FastEnter, 0x6)
{
	GET(FootClass* const, pThis, ESI);

	if (const auto pDest = abstract_cast<UnitClass*>(pThis->CurrentMission == Mission::Enter ? pThis->GetNthLink() : pThis->QueueUpToEnter))
	{
		const auto pType = pDest->Type;

		if (pType->Passengers > 0 && TechnoTypeExt::ExtMap.Find(pType)->NoQueueUpToEnter.Get(RulesExt::Global()->NoQueueUpToEnter))
		{
			if (IsCloseEnoughToEnter(pDest, pThis))
			{
				const auto absType = pThis->WhatAmI();

				if ((absType == AbstractType::Infantry || absType == AbstractType::Unit) && CanEnterNow(pDest, pThis))
					DoEnterNow(pDest, pThis);
			}
			else if (!pThis->Destination // Move to enter position, prevent other passengers from waiting for call and not moving early
				&& !pDest->OnBridge && !pDest->Destination)
			{
				auto cell = CellStruct::Empty;
				reinterpret_cast<CellStruct*(__thiscall*)(FootClass*, CellStruct*, AbstractClass*)>(0x703590)(pThis, &cell, pDest);

				if (cell != CellStruct::Empty)
				{
					pThis->SetDestination(MapClass::Instance.GetCellAt(cell), true);
					pThis->QueueMission(Mission::Move, false);
					pThis->NextMission();
				}
			}
		}
	}

	return 0;
}

// Rewrite from 0x4835D5/0x74004B, replace check pThis->GetCell()->LandType != LandType::Water
static inline bool CanUnloadNow(UnitClass* pTransport, FootClass* pPassenger)
{
	if (TechnoTypeExt::ExtMap.Find(pTransport->Type)->AmphibiousUnload.Get(RulesExt::Global()->AmphibiousUnload))
		return GroundType::Array[static_cast<int>(pTransport->GetCell()->LandType)].Cost[static_cast<int>(pPassenger->GetTechnoType()->SpeedType)] != 0.0;

	return pTransport->GetCell()->LandType != LandType::Water;
}

namespace TransportUnloadTemp
{
	bool ShouldPlaySound = false;
}

// Interrupted due to insufficient location or other reasons
DEFINE_HOOK(0x73DC9C, UnitClass_Mission_Unload_NoQueueUpToUnloadBreak, 0xA)
{
	enum { SkipGameCode = 0x73E289 };

	GET(UnitClass* const, pThis, ESI);
	GET(FootClass* const, pPassenger, EDI);

	// Restore vanilla function
	pPassenger->Undiscover();

	// Clean up the unload space
	const bool alt = pThis->OnBridge;
	const auto pCell = pThis->GetCell();
	const auto coord = pCell->GetCoords();

	for (int i = 0; i < 8; ++i)
	{
		const auto pAdjCell = pCell->GetNeighbourCell(static_cast<FacingType>(i));
		const auto pTechno = pAdjCell->FindTechnoNearestTo(Point2D::Empty, alt, pThis);

		if (pTechno && pTechno->Owner->IsAlliedWith(pThis))
			pAdjCell->ScatterContent(coord, true, true, alt);
	}

	// Play the sound when interrupted
	if (TransportUnloadTemp::ShouldPlaySound) // Only when NoQueueUpToUnload enabled
	{
		TransportUnloadTemp::ShouldPlaySound = false;
		const auto pType = pThis->Type;

		if (TechnoTypeExt::ExtMap.Find(pType)->NoQueueUpToUnload.Get(RulesExt::Global()->NoQueueUpToUnload))
			VoxClass::PlayAtPos(pType->LeaveTransportSound, &pThis->Location);
	}

	return SkipGameCode;
}

// Within a single frame, cycle to get off the car
DEFINE_HOOK(0x73DC1E, UnitClass_Mission_Unload_NoQueueUpToUnloadLoop, 0xA)
{
	enum { UnloadLoop = 0x73D8CB, UnloadReturn = 0x73E289, NoUnloadReturn = 0x73D8AA };

	GET(UnitClass* const, pThis, ESI);

	const auto pType = pThis->Type;
	const auto pPassenger = pThis->Passengers.GetFirstPassenger();

	if (TechnoTypeExt::ExtMap.Find(pType)->NoQueueUpToUnload.Get(RulesExt::Global()->NoQueueUpToUnload))
	{
		if (!pPassenger || pThis->Passengers.NumPassengers <= pThis->NonPassengerCount)
		{
			// If unloading is required within one frame, the sound will only be played when the last passenger leaves
			VoxClass::PlayAtPos(pType->LeaveTransportSound, &pThis->Location);
			TransportUnloadTemp::ShouldPlaySound = false;
			return UnloadReturn;
		}
		else if (!CanUnloadNow(pThis, pPassenger))
		{
			VoxClass::PlayAtPos(pType->LeaveTransportSound, &pThis->Location);
			TransportUnloadTemp::ShouldPlaySound = false;
			pThis->MissionStatus = 0; // Retry
			return NoUnloadReturn;
		}

		TransportUnloadTemp::ShouldPlaySound = true;
		R->EBX(0); // Reset
		return UnloadLoop;
	}

	// PlayAtPos has already handled the situation where Sound is less than 0 internally, so unnecessary checks will be skipped
	VoxClass::PlayAtPos(pType->LeaveTransportSound, &pThis->Location);

	if (!pPassenger || CanUnloadNow(pThis, pPassenger))
		return UnloadReturn;

	pThis->MissionStatus = 0; // Retry
	return NoUnloadReturn;
}

#pragma endregion

#pragma region TransportFix

DEFINE_HOOK(0x51D45B, InfantryClass_Scatter_NoProcess, 0x6)
{
	enum { SkipGameCode = 0x51D47B };

	REF_STACK(const int, addr, STACK_OFFSET(0x50, 0));
	// Skip process in InfantryClass::UpdatePosition which can create invisible barrier
	return (addr == 0x51A4B5) ? SkipGameCode : 0;
}

DEFINE_HOOK(0x4D92BF, FootClass_Mission_Enter_CheckLink, 0x5)
{
	enum { NextAction = 0x4D92ED, NotifyUnlink = 0x4D92CE, DoNothing = 0x4D946C };

	GET(UnitClass* const, pThis, ESI);
	GET(const RadioCommand, answer, EAX);
	// Restore vanilla check
	if (pThis->IsTether || answer == RadioCommand::AnswerPositive)
		return NextAction;
	// The link should not be disconnected while the transporter is in motion (passengers waiting to enter),
	// as this will result in the first passenger not getting on board
	return answer == RadioCommand::RequestLoading ? DoNothing : NotifyUnlink;
}

DEFINE_HOOK(0x73769E, UnitClass_ReceiveCommand_NoEnterOnBridge, 0x6)
{
	enum { NoEnter = 0x73780F };

	GET(UnitClass* const, pThis, ESI);
	GET(TechnoClass* const, pCall, EDI);
	// If both the transport vehicle and passengers are on the bridge, they should not board
	return pThis->OnBridge && pCall->OnBridge ? NoEnter : 0;
}

DEFINE_HOOK(0x70D842, FootClass_UpdateEnter_NoMoveToBridge, 0x5)
{
	enum { NoMove = 0x70D84F };

	GET(TechnoClass* const, pEnter, EDI);
	// If the transport vehicle is on the bridge, passengers should wait in place for the transport vehicle to arrive
	return pEnter->OnBridge && (pEnter->WhatAmI() == AbstractType::Unit && static_cast<UnitClass*>(pEnter)->Type->Passengers > 0) ? NoMove : 0;
}

DEFINE_HOOK(0x70D910, FootClass_QueueEnter_NoMoveToBridge, 0x5)
{
	enum { NoMove = 0x70D977 };

	GET(TechnoClass* const, pEnter, EAX);
	// If the transport vehicle is on the bridge, passengers should wait in place for the transport vehicle to arrive
	return pEnter->OnBridge && (pEnter->WhatAmI() == AbstractType::Unit && static_cast<UnitClass*>(pEnter)->Type->Passengers > 0) ? NoMove : 0;
}

DEFINE_HOOK(0x7196BB, TeleportLocomotionClass_Process_MarkDown, 0xA)
{
	enum { SkipGameCode = 0x7196C5 };

	GET(FootClass*, pLinkedTo, ECX);
	// When Teleport units board transport vehicles on the bridge, the lack of this repair can lead to numerous problems
	// An impassable invisible barrier will be generated on the bridge (the object linked list of the cell will leave it)
	// And the transport vehicle will board on the vehicle itself (BFRT Passenger:..., BFRT)
	// If any infantry attempts to pass through this position on the bridge later, it will cause the game to freeze
	auto shouldMarkDown = [pLinkedTo]()
	{
		if (pLinkedTo->GetCurrentMission() != Mission::Enter)
			return true;

		const auto pEnter = pLinkedTo->GetNthLink();

		return (!pEnter || pEnter->GetTechnoType()->Passengers <= 0);
	};

	if (shouldMarkDown())
		pLinkedTo->Mark(MarkType::Down);

	return SkipGameCode;
}

#pragma endregion

#pragma region AmphibiousEnterAndUnload

// Related fix
DEFINE_HOOK(0x4B08EF, DriveLocomotionClass_Process_CheckUnload, 0x5)
{
	enum { SkipGameCode = 0x4B078C, ContinueProcess = 0x4B0903 };

	GET(ILocomotion* const, iloco, ESI);

	__assume(iloco != nullptr);

	const auto pFoot = static_cast<LocomotionClass*>(iloco)->LinkedTo;

	if (pFoot->GetCurrentMission() != Mission::Unload)
		return ContinueProcess;

	return (pFoot->GetTechnoType()->Passengers > 0 && pFoot->Passengers.GetFirstPassenger()) ? ContinueProcess : SkipGameCode;
}

DEFINE_HOOK(0x69FFB6, ShipLocomotionClass_Process_CheckUnload, 0x5)
{
	enum { SkipGameCode = 0x69FE39, ContinueProcess = 0x69FFCA };

	GET(ILocomotion* const, iloco, ESI);

	__assume(iloco != nullptr);

	const auto pFoot = static_cast<LocomotionClass*>(iloco)->LinkedTo;

	if (pFoot->GetCurrentMission() != Mission::Unload)
		return ContinueProcess;

	return (pFoot->GetTechnoType()->Passengers > 0 && pFoot->Passengers.GetFirstPassenger()) ? ContinueProcess : SkipGameCode;
}

// Rewrite from 0x718505
DEFINE_HOOK_AGAIN(0x7190B0, TeleportLocomotionClass_MovingTo_ReplaceMovementZone, 0x6)
DEFINE_HOOK(0x718F1E, TeleportLocomotionClass_MovingTo_ReplaceMovementZone, 0x6)
{
	GET(TechnoTypeClass* const, pType, EAX);

	auto movementZone = pType->MovementZone;

	if (movementZone == MovementZone::Fly || movementZone == MovementZone::Destroyer)
		movementZone = MovementZone::Normal;
	else if (movementZone == MovementZone::AmphibiousDestroyer)
		movementZone = MovementZone::Amphibious;

	R->EBP(movementZone);
	return R->Origin() + 0x6;
}

// Enter building
DEFINE_JUMP(LJMP, 0x43C38D, 0x43C3FF); // Skip amphibious and naval check if no Ares

// Enter unit
DEFINE_HOOK(0x73796B, UnitClass_ReceiveCommand_AmphibiousEnter, 0x7)
{
	enum { ContinueCheck = 0x737990, MoveToPassenger = 0x737974 };

	GET(UnitClass* const, pThis, ESI);

	if (pThis->OnBridge)
		return MoveToPassenger;

	if (TechnoTypeExt::ExtMap.Find(pThis->Type)->AmphibiousEnter.Get(RulesExt::Global()->AmphibiousEnter))
		return ContinueCheck;

	GET(CellClass* const, pCell, EBP);

	return (pCell->LandType != LandType::Water) ? ContinueCheck : MoveToPassenger;
}

// Unit unload

DEFINE_HOOK(0x7400B5, UnitClass_MouseOverObject_AmphibiousUnload, 0x7)
{
	enum { ContinueCheck = 0x7400C6, CannotUnload = 0x7400BE };

	GET(CellClass* const, pCell, EBX);

	if (pCell->LandType == LandType::Water) // I don't know why WW made a reverse judgment here? Because of the coast/beach?
		return ContinueCheck;

	GET(UnitClass* const, pThis, ESI);

	return TechnoTypeExt::ExtMap.Find(pThis->Type)->AmphibiousUnload.Get(RulesExt::Global()->AmphibiousUnload) ? ContinueCheck : CannotUnload;
}

DEFINE_HOOK(0x70107A, TechnoClass_CanDeploySlashUnload_AmphibiousUnload, 0x7)
{
	enum { ContinueCheck = 0x701087, CannotUnload = 0x700DCE };

	GET(CellClass* const, pCell, EBP);

	if (pCell->LandType != LandType::Water)
		return ContinueCheck;

	GET(UnitClass* const, pThis, ESI);

	return TechnoTypeExt::ExtMap.Find(pThis->Type)->AmphibiousUnload.Get(RulesExt::Global()->AmphibiousUnload) ? ContinueCheck : CannotUnload;
}

DEFINE_HOOK(0x73D769, UnitClass_Mission_Unload_AmphibiousUnload, 0x7)
{
	enum { MoveToLand = 0x73D772, UnloadCheck = 0x73D7E4 };

	GET(UnitClass* const, pThis, ESI);

	const auto pPassenger = pThis->Passengers.GetFirstPassenger();

	return (!pPassenger || CanUnloadNow(pThis, pPassenger)) ? UnloadCheck : MoveToLand;
}

DEFINE_HOOK(0x73D7AB, UnitClass_Mission_Unload_FindUnloadPosition, 0x5)
{
	GET(UnitClass* const, pThis, ESI);

	if (TechnoTypeExt::ExtMap.Find(pThis->Type)->AmphibiousUnload.Get(RulesExt::Global()->AmphibiousUnload))
	{
		if (const auto pPassenger = pThis->Passengers.GetFirstPassenger())
		{
			REF_STACK(SpeedType, speedType, STACK_OFFSET(0xBC, -0xB4));
			REF_STACK(MovementZone, movementZone, STACK_OFFSET(0xBC, -0xAC));

			const auto pType = pPassenger->GetTechnoType();
			speedType = pType->SpeedType; // Replace hard code SpeedType::Wheel
			movementZone = pType->MovementZone; // Replace hard code MovementZone::Normal
		}
	}

	return 0;
}

DEFINE_HOOK(0x73D7B7, UnitClass_Mission_Unload_CheckInvalidCell, 0x6)
{
	enum { CannotUnload = 0x73D87F };

	GET(const CellStruct, cell, EAX);

	return cell != CellStruct::Empty ? 0 : CannotUnload;
}

DEFINE_HOOK(0x740C9C, UnitClass_GetUnloadDirection_CheckUnloadPosition, 0x7)
{
	GET(UnitClass* const, pThis, EDI);

	if (TechnoTypeExt::ExtMap.Find(pThis->Type)->AmphibiousUnload.Get(RulesExt::Global()->AmphibiousUnload))
	{
		if (const auto pPassenger = pThis->Passengers.GetFirstPassenger())
		{
			GET(const int, speedType, EDX);
			R->EDX(speedType + static_cast<int>(pPassenger->GetTechnoType()->SpeedType)); // Replace hard code SpeedType::Foot
		}
	}

	return 0;
}

DEFINE_HOOK(0x73DAD8, UnitClass_Mission_Unload_PassengerLeavePosition, 0x5)
{
	GET(UnitClass* const, pThis, ESI);

	if (TechnoTypeExt::ExtMap.Find(pThis->Type)->AmphibiousUnload.Get(RulesExt::Global()->AmphibiousUnload))
	{
		GET(FootClass* const, pPassenger, EDI);
		REF_STACK(MovementZone, movementZone, STACK_OFFSET(0xBC, -0xAC));
		movementZone = pPassenger->GetTechnoType()->MovementZone; // Replace hard code MovementZone::Normal
	}

	return 0;
}

#pragma endregion
