#include "Body.h"

#include <SuperClass.h>

#include <Ext/House/Body.h>
#include <Utilities/Debug.h>
#include <algorithm> // lower_bound, rotate, sort

static inline void StartAutoFireRetryCooldown(SuperClass* pSuper)
{
	if (!pSuper) { return; } // guard for analyzer & safety

	// Use the SW's own recharge time; fall back to 60 ticks if missing/zero
	int rt = 0;
	if (pSuper->Type && pSuper->Type->RechargeTime > 0)
	{
		rt = pSuper->Type->RechargeTime;
	}
	else
	{
		rt = 60; // safe fallback
	}

	// Only (re)start if not already cooling down
	auto& timer = pSuper->RechargeTimer;   // safe after pSuper check
	if (timer.GetTimeLeft() <= 0)
	{
		timer.Start(rt);
	}
}

// SuperClass::Launch hook
DEFINE_HOOK(0x6CC390, SuperClass_Launch, 0x6)
{
	// Return codes for this site:
	//  - Return 0      => fall through to original game code
	//  - Return 0x6CDE40 => jump to the Ares/Phobos place/fire path (our usual “handled/skip” exit)
	enum { FallThrough = 0, JumpAresPlaceFire = 0x6CDE40 };

	GET(SuperClass* const, pSuper, ECX);
	GET_STACK(CellStruct const* const, pCell, 0x4);
	GET_STACK(bool const, isPlayer, 0x8);
	(void)isPlayer; // silence C4189 if unused here

	// Basic guards — if anything is off, bail to the common Ares path
	if (!pSuper || !pSuper->Type || !pSuper->Owner)
		return JumpAresPlaceFire;

	const auto* const pExt = SWTypeExt::ExtMap.Find(pSuper->Type);
	if (!pExt)
		return JumpAresPlaceFire;

	// Prevent every-frame spam when RechargeTimer is -1 on AutoFire
	if (pExt->SW_AutoFire && pSuper->RechargeTimer.GetTimeLeft() == -1)
	{
		pSuper->RechargeTimer.Start(0);
		return JumpAresPlaceFire; // skip launching this frame
	}

	// Predicate-only BP/CP gating for AutoFire (no mutations here)
	if (pExt->SW_AutoFire)
	{
		auto* const pOwnerExt = HouseExt::ExtMap.Find(pSuper->Owner);
		if (!pOwnerExt) return JumpAresPlaceFire;

		if (pExt->BattlePoints_Amount != 0 &&
			!pOwnerExt->CanTransactBattlePoints(pExt->BattlePoints_Amount))
		{
			StartAutoFireRetryCooldown(pSuper);
			return JumpAresPlaceFire; // skip
		}

		if (pExt->CommanderPoints_Amount != 0 &&
			!pOwnerExt->CanTransactCommanderPoints(pExt->CommanderPoints_Amount))
		{
			StartAutoFireRetryCooldown(pSuper);
			return JumpAresPlaceFire; // skip
		}
	}

	// If SWTypeExt handles the activation, continue via Ares path; otherwise fall through
	const bool handled = SWTypeExt::Activate(pSuper, *pCell, isPlayer);
	return handled ? JumpAresPlaceFire : FallThrough;
}

// Hook at 0x6CDE40 where Ares jumps to - this should catch Ares launches
DEFINE_HOOK(0x6CDE40, SuperClass_Place_FireExt, 0x5)
{
	GET(SuperClass* const, pSuper, ECX);
	GET_STACK(CellStruct const* const, pCell, 0x4);
	// GET_STACK(bool const, isPlayer, 0x8);

	// Check if the SuperClass pointer is valid and not corrupted.
	if (pSuper && VTable::Get(pSuper) == SuperClass::AbsVTable)
	{
		// Validate type/ext safely
		const auto pExt = SWTypeExt::ExtMap.Find(pSuper->Type);
		if (!pExt) { return 0; }

		// Validate AutoFire superweapons for ALL players (predicate only; no mutations)
		if (pExt->SW_AutoFire)
		{
			const auto pOwnerExt = HouseExt::ExtMap.Find(pSuper->Owner);
			if (!pOwnerExt) { return 0; }

			// BattlePoints gate
			if (pExt->BattlePoints_Amount != 0 &&
				!pOwnerExt->CanTransactBattlePoints(pExt->BattlePoints_Amount))
			{
				StartAutoFireRetryCooldown(pSuper); // prevent immediate retry
				return 0; // Skip the firing
			}

			// CommanderPoints gate
			if (pExt->CommanderPoints_Amount != 0 &&
				!pOwnerExt->CanTransactCommanderPoints(pExt->CommanderPoints_Amount))
			{
				StartAutoFireRetryCooldown(pSuper); // prevent immediate retry
				return 0; // Skip the firing
			}
			// ❗ No pre-deduction here — ApplyBattle/CommanderPoints on successful fire handles it.
		}

		SWTypeExt::FireSuperWeaponExt(pSuper, *pCell);
	}
	else
	{
		Debug::Log(__FUNCTION__": Hook entered with an invalid or corrupt SuperClass pointer.");
	}

	return 0;
}

DEFINE_HOOK(0x6CB5EB, SuperClass_Grant_ShowTimer, 0x5)
{
	GET(SuperClass*, pThis, ESI);

	if (SuperClass::ShowTimers.AddItem(pThis))
	{
		// Replace full re-sort with ordered insert (stable & faster when list grows)
		auto& v = SuperClass::ShowTimers;
		const int key = SWTypeExt::ExtMap.Find(pThis->Type)->ShowTimer_Priority.Get();

		// Descending order by priority (same as comparator in original sort)
		auto it = std::lower_bound(
			v.begin(), v.end(), key,
			[](SuperClass* a, int priorityKey)
			{
				const auto aExt = SWTypeExt::ExtMap.Find(a->Type);
				return aExt->ShowTimer_Priority.Get() > priorityKey;
			});

		// Move the just-added back element into position
		std::rotate(it, v.end() - 1, v.end());
	}

	return 0x6CB63E;
}

DEFINE_HOOK(0x6DBE74, Tactical_SuperLinesCircles_ShowDesignatorRange, 0x7)
{
	if (!Phobos::Config::ShowDesignatorRange
		|| !RulesExt::Global()->ShowDesignatorRange
		|| Unsorted::CurrentSWType == -1)
		return 0;

	const auto pSuperType = SuperWeaponTypeClass::Array.GetItem(Unsorted::CurrentSWType);
	if (!pSuperType) { return 0; }

	const auto pExt = SWTypeExt::ExtMap.Find(pSuperType);
	if (!pExt || !pExt->ShowDesignatorRange)
		return 0;

	for (const auto pCurrentTechno : TechnoClass::Array)
	{
		if (!pCurrentTechno) { continue; }

		const auto pCurrentTechnoType = pCurrentTechno->GetTechnoType();
		const auto pOwner = pCurrentTechno->Owner;
		if (!pCurrentTechnoType || !pOwner) { continue; }

		if (!pCurrentTechno->IsAlive
			|| pCurrentTechno->InLimbo
			|| (pOwner != HouseClass::CurrentPlayer && pOwner->IsAlliedWith(HouseClass::CurrentPlayer))                  // Ally objects are never designators or inhibitors
			|| (pOwner == HouseClass::CurrentPlayer && !pExt->SW_Designators.Contains(pCurrentTechnoType))               // Only owned objects can be designators
			|| (!pOwner->IsAlliedWith(HouseClass::CurrentPlayer) && !pExt->SW_Inhibitors.Contains(pCurrentTechnoType)))  // Only enemy objects can be inhibitors
		{
			continue;
		}

		const auto pTechnoTypeExt = TechnoTypeExt::ExtMap.Find(pCurrentTechnoType);
		if (!pTechnoTypeExt) { continue; }

		const float radius = (pOwner == HouseClass::CurrentPlayer)
			? static_cast<float>(pTechnoTypeExt->DesignatorRange.Get(pCurrentTechnoType->Sight))
			: static_cast<float>(pTechnoTypeExt->InhibitorRange.Get(pCurrentTechnoType->Sight));

		CoordStruct coords = pCurrentTechno->GetCenterCoords();
		coords.Z = MapClass::Instance.GetCellFloorHeight(coords);
		const auto color = pOwner->Color;
		Game::DrawRadialIndicator(false, true, coords, color, radius, false, true);
	}

	return 0;
}

DEFINE_HOOK(0x6CBEF4, SuperClass_AnimStage_UseWeeds, 0x6)
{
	enum
	{
		Ready = 0x6CBFEC,
		NotReady = 0x6CC064,
		ProgressInEax = 0x6CC066
	};

	constexpr int maxCounterFrames = 54;

	GET(SuperClass*, pSuper, ECX);
	GET(SuperWeaponTypeClass*, pSWType, EBX);

	if (!pSuper || !pSWType) { return 0; }

	const auto pExt = SWTypeExt::ExtMap.Find(pSWType);
	if (!pExt) { return 0; }

	if (pExt->UseWeeds)
	{
		if (pSuper->IsReady)
			return Ready;

		if (pExt->UseWeeds_StorageTimer)
		{
			int progress = static_cast<int>(pSuper->Owner->OwnedWeed.GetTotalAmount() * maxCounterFrames / pExt->UseWeeds_Amount);
			if (progress > maxCounterFrames)
				progress = maxCounterFrames;

			R->EAX(progress);
			return ProgressInEax;
		}
		else
		{
			return NotReady;
		}
	}

	return 0;
}

DEFINE_HOOK(0x6CBD2C, SuperClass_AI_UseWeeds, 0x6)
{
	enum
	{
		NothingChanged = 0x6CBE9D,
		SomethingChanged = 0x6CBD48,
		Charged = 0x6CBD73
	};

	enum
	{
		SWReadyTimer = 0,
		SWAlmostReadyTimer = 15,
		SWNotReadyTimer = 915
	};

	GET(SuperClass*, pSuper, ESI);
	if (!pSuper) { return 0; }

	const auto pExt = SWTypeExt::ExtMap.Find(pSuper->Type);
	if (!pExt) { return 0; }

	if (pExt->UseWeeds)
	{
		pSuper->Type->ShowTimer = false;
		const float totalAmount = pSuper->Owner->OwnedWeed.GetTotalAmount();
		const int weedAmount = pExt->UseWeeds_Amount;

		if (totalAmount >= weedAmount)
		{
			pSuper->Owner->OwnedWeed.RemoveAmount(static_cast<float>(weedAmount), 0);
			pSuper->RechargeTimer.Start(SWReadyTimer); // The Armageddon is here
			return Charged;
		}

		if (totalAmount >= pExt->UseWeeds_ReadinessAnimationPercentage * weedAmount)
		{
			pSuper->RechargeTimer.Start(SWAlmostReadyTimer); // The end is nigh!
		}
		else
		{
			pSuper->RechargeTimer.Start(SWNotReadyTimer); // 61 seconds > 60 seconds (animation activation threshold)
		}

		const int animStage = pSuper->AnimStage();
		if (pSuper->CameoChargeState != animStage)
		{
			pSuper->CameoChargeState = animStage;
			return SomethingChanged;
		}

		return NothingChanged;
	}

	return 0;
}

// This is pointless for SWs using weeds because their charge is tied to weed storage.
DEFINE_HOOK(0x6CC1E6, SuperClass_SetSWCharge_UseWeeds, 0x5)
{
	enum { Skip = 0x6CC251 };

	GET(SuperClass*, pSuper, EDI);
	if (!pSuper) { return 0; }

	const auto pExt = SWTypeExt::ExtMap.Find(pSuper->Type);
	if (!pExt) { return 0; }

	if (pExt->UseWeeds)
		return Skip;

	return 0;
}

#pragma region SW TabIndex
DEFINE_HOOK(0x6A5F6E, SidebarClass_6A5F20_TabIndex, 0x8)
{
	enum { ApplyTabIndex = 0x6A5FD3 };

	GET(AbstractType const, absType, ESI);
	GET(int const, typeIdx, EAX);

	R->EAX(SidebarClass::GetObjectTabIdx(absType, typeIdx, 0));
	return ApplyTabIndex;
}

DEFINE_HOOK(0x6A614D, SidebarClass_6A6140_TabIndex, 0x5)
{
	enum { ApplyTabIndex = 0x6A61B1 };

	GET(AbstractType const, absType, EDI);
	GET(int const, typeIdx, EBP);

	R->EAX(SidebarClass::GetObjectTabIdx(absType, typeIdx, 0));
	return ApplyTabIndex;
}

DEFINE_HOOK(0x6A633D, SidebarClass_AddCameo_TabIndex, 0x5)
{
	enum { ApplyTabIndex = 0x6A63B7 };

	GET(AbstractType const, absType, ESI);
	GET(int const, typeIdx, EBP);

	R->Stack(STACK_OFFSET(0x14, 0x4), SidebarClass::GetObjectTabIdx(absType, typeIdx, 0));
	return ApplyTabIndex;
}

DEFINE_HOOK(0x6ABC9D, SidebarClass_GetObjectTabIndex_Super, 0x5)
{
	enum { ApplyTabIndex = 0x6ABCA2 };

	GET(int const, typeIdx, EDX);

	if (typeIdx < 0 || typeIdx >= SuperWeaponTypeClass::Array.Count)
		return 0;

	const auto pSWType = SuperWeaponTypeClass::Array[typeIdx];
	const auto pSWTypExt = SWTypeExt::ExtMap.Find(pSWType);
	if (!pSWTypExt) { return 0; }

	R->EAX(pSWTypExt->TabIndex);
	return ApplyTabIndex;
}

DEFINE_HOOK(0x6AC67A, SidebarClass_6AC5F0_TabIndex, 0x5)
{
	enum { ApplyTabIndex = 0x6AC6D9 };

	GET(AbstractType const, absType, EAX);
	GET(int const, typeIdx, ESI);

	R->EAX(SidebarClass::GetObjectTabIdx(absType, typeIdx, 0));
	return ApplyTabIndex;
}

DEFINE_JUMP(LJMP, 0x6A8D07, 0x6A8D17) // Skip tabIndex check
#pragma endregion

// Full rewrite
DEFINE_HOOK(0x6CC367, SuperClass_IsReady_BattlePoints, 0xD)
{
	GET(SuperClass*, pSuper, ECX);
	enum { ReturnIsReady = 0x6CC37D, ReturnZero = 0x6CC381, SkipAll = 0x6CC383 };

	if (!pSuper || !pSuper->Type || !pSuper->Owner)
		return ReturnZero;
	if (pSuper->IsSuspended)
		return ReturnZero;

	if (pSuper->Type->UseChargeDrain)
	{
		R->AL(pSuper->ChargeDrainState != ChargeDrainState::Charging);
		return SkipAll;
	}

	const auto pExt = SWTypeExt::ExtMap.Find(pSuper->Type);
	if (!pExt || !pExt->IsAvailable(pSuper->Owner))
		return ReturnZero;

	// Cache once per call
	const auto pOwnerExt = HouseExt::ExtMap.Find(pSuper->Owner);

	// AutoFire: predicate-only BP/CP checks (no state changes)
	if (pExt->SW_AutoFire)
	{
		if (!pOwnerExt) return ReturnZero;

		if (pExt->BattlePoints_Amount != 0 &&
			!pOwnerExt->CanTransactBattlePoints(pExt->BattlePoints_Amount))
		{
			if (pSuper->RechargeTimer.GetTimeLeft() <= 0)
			{
				int rt = (pSuper->Type ? pSuper->Type->RechargeTime : 0);
				if (rt <= 0) rt = 60;
				pSuper->RechargeTimer.Start(rt);
			}
			return ReturnZero;
		}
		if (pExt->CommanderPoints_Amount != 0 &&
			!pOwnerExt->CanTransactCommanderPoints(pExt->CommanderPoints_Amount))
		{
				if (pSuper->RechargeTimer.GetTimeLeft() <= 0)
				{
					int rt = (pSuper->Type ? pSuper->Type->RechargeTime : 0);
					if (rt <= 0) rt = 60;
					pSuper->RechargeTimer.Start(rt);
				}
				return ReturnZero;
		}
	}

	// General readiness (non-AutoFire too)
	if (pExt->BattlePoints_Amount != 0)
	{
		if (!pOwnerExt || !pOwnerExt->CanTransactBattlePoints(pExt->BattlePoints_Amount))
			return ReturnZero;
	}
	if (pExt->CommanderPoints_Amount != 0)
	{
		if (!pOwnerExt || !pOwnerExt->CanTransactCommanderPoints(pExt->CommanderPoints_Amount))
			return ReturnZero;
	}

	return ReturnIsReady;
}
// Executed before the Ares hook for launching AI super weapons, SW->IsReady property won't be updated anymore
DEFINE_HOOK(0x4FD77C, ExpertAI_SuperWeaponAI_RecheckIsReady, 0x5)
{
	GET(HouseClass*, pHouse, EBX);
	if (!SessionClass::IsCampaign() || pHouse->IQLevel2 >= RulesClass::Instance->SuperWeapons)
	{
		for (auto const& pSuper : pHouse->Supers)
		{
			if (pSuper->IsReady)
				pSuper->IsReady = pSuper->CanFire();
		}
	}

	return 0;
}

// Hook removed - was causing crash by interfering with Ares patches

// Hook to intercept AutoFire superweapons and provide AI targeting for FindAuxTechno mode
DEFINE_HOOK(0x6CB920, SuperClass_ClickFire, 0x6)
{
	GET(SuperClass*, pSuper, ECX);
	GET_STACK(CellStruct*, pCell, 0x4);
	

	if (!pSuper || !pSuper->Type) { return 0; }

	const auto pExt = SWTypeExt::ExtMap.Find(pSuper->Type);
	if (!pExt) { return 0; }

	// Only handle AutoFire superweapons with FindAuxTechno targeting
	if (pExt->SW_AutoFire && pExt->ShouldUseAITargeting())
	{
		// Get target using FindAuxTechno AI targeting
		const CellStruct targetCell = pExt->GetAuxTechnoTarget(pSuper->Owner);

		if (targetCell != CellStruct::Empty)
		{
			*pCell = targetCell;
		}
		else
		{
			// No valid target found, don't fire
			return 0x6CB97E; // Skip firing
		}
	}

	return 0;
}

