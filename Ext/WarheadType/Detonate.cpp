#include "Body.h"

#include <InfantryClass.h>
#include <BulletClass.h>
#include <HouseClass.h>
#include <ScenarioClass.h>
#include <Ext/House/Body.h>
#include <AnimTypeClass.h>
#include <AnimClass.h>
#include <BitFont.h>
#include <SuperClass.h>

#include <Ext/Anim/Body.h>
#include <Ext/Bullet/Body.h>
#include <Ext/BulletType/Body.h>
#include <Ext/SWType/Body.h>
#include <Misc/FlyingStrings.h>
#include <Utilities/Helpers.Alex.h>
#include <Utilities/EnumFunctions.h>

#pragma region CreateGap Calls

static void __stdcall Sub_4ADEE0(char a1, DWORD a2)
{
	JMP_STD(0x4ADEE0);
}

static void __stdcall Sub_4ADCD0(char a1, DWORD a2)
{
	JMP_STD(0x4ADCD0);
}

#pragma endregion

void WarheadTypeExt::ExtData::Detonate(TechnoClass* pOwner, HouseClass* pHouse, BulletExt::ExtData* pBulletExt, CoordStruct coords)
{

	auto const pBullet = pBulletExt ? pBulletExt->OwnerObject() : nullptr;

	if (pBulletExt && pBulletExt->InterceptorTechnoType)
		this->InterceptBullets(pOwner, pBullet, coords);

	if (pHouse)
	{
		const int createGap = this->CreateGap;

		if (createGap > 0)
		{
			const auto pCurrent = HouseClass::CurrentPlayer;

			if (!pHouse->IsControlledByCurrentPlayer() && !pHouse->IsAlliedWith(pCurrent) && !pCurrent->Defeated && !pCurrent->SpySatActive)
			{
				Sub_4ADEE0(0, 0);
				CellRangeIterator<CellClass>{}(CellClass::Coord2Cell(coords), this->CreateGap + 0.5, [](CellClass* pCell) {
						pCell->Flags &= ~CellFlags::Revealed;
						pCell->AltFlags &= ~AltCellFlags::Clear;
						pCell->ShroudCounter = 1;
						pCell->GapsCoveringThisCell = 0;
						return true;
				});
				Sub_4ADCD0(0, 0);
				pCurrent->Visionary = 0;
				MapClass::Instance.sub_657CE0();
				MapClass::Instance.MarkNeedsRedraw(2);
			}
		}
		else if (createGap < 0)
		{
			for (const auto pOtherHouse : HouseClass::Array)
			{
				if (!pHouse->IsAlliedWith(pOtherHouse) && !pOtherHouse->Defeated && !pOtherHouse->SpySatActive)
					MapClass::Instance.Reshroud(pOtherHouse);
			}
		}

		const int reveal = this->Reveal;

		if (reveal > 0)
		{
			const auto pCurrent = HouseClass::CurrentPlayer;

			if ((pHouse->IsControlledByCurrentPlayer() || pHouse->IsAlliedWith(pCurrent)) && !pCurrent->Defeated && !pCurrent->Visionary)
			{
				Sub_4ADEE0(0, 0);
				MapClass::Instance.RevealArea2(const_cast<CoordStruct*>(&coords), reveal, pHouse, 0, 0, 0, 0, 1);
				Sub_4ADCD0(0, 0);
				MapClass::Instance.sub_657CE0();
				MapClass::Instance.MarkNeedsRedraw(2);
			}
		}
		else if (reveal < 0)
		{
			MapClass::Instance.Reveal(pHouse);
		}

		if (this->TransactMoney)
		{
			pHouse->TransactMoney(this->TransactMoney);

			if (this->TransactMoney_Display)
			{
				auto displayCoords = this->TransactMoney_Display_AtFirer ? (pOwner ? pOwner->Location : coords) : coords;
				FlyingStrings::AddMoneyString(this->TransactMoney, pHouse, this->TransactMoney_Display_Houses, displayCoords, this->TransactMoney_Display_Offset);
			}
		}

		if (this->SpawnsCrate_Types.size() > 0)
		{
			const int index = GeneralUtils::ChooseOneWeighted(ScenarioClass::Instance->Random.RandomDouble(), &this->SpawnsCrate_Weights);

			if (index < static_cast<int>(this->SpawnsCrate_Types.size()))
				MapClass::Instance.PlacePowerupCrate(CellClass::Coord2Cell(coords), this->SpawnsCrate_Types.at(index));
		}

		for (const int swIdx : this->LaunchSW)
		{
			if (const auto pSuper = pHouse->Supers.GetItem(swIdx))
			{
				const auto pSWExt = SWTypeExt::ExtMap.Find(pSuper->Type);
				const auto cell = CellClass::Coord2Cell(coords);

				if (pHouse->CanTransactMoney(pSWExt->Money_Amount) &&
					(pSWExt->BattlePoints_Amount == 0 || HouseExt::ExtMap.Find(pHouse)->CanTransactBattlePoints(pSWExt->BattlePoints_Amount)) &&
					(pSWExt->CommanderPoints_Amount == 0 || HouseExt::ExtMap.Find(pHouse)->CanTransactCommanderPoints(pSWExt->CommanderPoints_Amount)) &&
					(!this->LaunchSW_RealLaunch || (pSuper->IsPresent && pSuper->IsReady && !pSuper->IsSuspended)))
				{
					if ((this->LaunchSW_IgnoreInhibitors || !pSWExt->HasInhibitor(pHouse, cell))
					&& (this->LaunchSW_IgnoreDesignators || pSWExt->HasDesignator(pHouse, cell)))
					{
						if (this->LaunchSW_DisplayMoney && pSWExt->Money_Amount != 0)
							FlyingStrings::AddMoneyString(pSWExt->Money_Amount, pHouse, this->LaunchSW_DisplayMoney_Houses, coords, this->LaunchSW_DisplayMoney_Offset);

						const int oldstart = pSuper->RechargeTimer.StartTime;
						const int oldleft = pSuper->RechargeTimer.TimeLeft;
						// If you don't set it ready, NewSWType::Active will give false in Ares if RealLaunch=false
						// and therefore it will reuse the vanilla routine, which will crash inside of it
						pSuper->SetReadiness(true);
						// TODO: Can we use ClickFire instead of Launch?
						pSuper->Launch(cell, pHouse->IsCurrentPlayer());
						pSuper->Reset();

						if (!this->LaunchSW_RealLaunch)
						{
							pSuper->RechargeTimer.StartTime = oldstart;
							pSuper->RechargeTimer.TimeLeft = oldleft;
						}
					}
				}
			}
		}
	}

	this->Crit_Active = false;
	this->Crit_CurrentChance = this->GetCritChance(pOwner);


	if (this->PossibleCellSpreadDetonate || this->Crit_CurrentChance > 0.0 || this->Transact)
	{
		if (!this->Crit_ApplyChancePerTarget)
			this->Crit_RandomBuffer = ScenarioClass::Instance->Random.RandomDouble();

		if (this->Crit_ActiveChanceAnims.size() > 0 && this->Crit_CurrentChance > 0.0)
		{
			const int idx = ScenarioClass::Instance->Random.RandomRanged(0, this->Crit_ActiveChanceAnims.size() - 1);
			auto const pAnim = GameCreate<AnimClass>(this->Crit_ActiveChanceAnims[idx], coords);
			AnimExt::SetAnimOwnerHouseKind(pAnim, pHouse, nullptr, false, true);
			AnimExt::ExtMap.Find(pAnim)->SetInvoker(pOwner, pHouse);
		}

		const bool bulletWasIntercepted = pBulletExt && (pBulletExt->InterceptedStatus & InterceptedStatus::Intercepted);
		const float cellSpread = this->OwnerObject()->CellSpread;

		if (std::abs(cellSpread) >= 0.1f)
		{
			auto targets = Helpers::Alex::getCellSpreadItems(coords, cellSpread, true);

			for (auto const pTarget : targets)
				this->DetonateOnOneUnit(pHouse, pTarget, pOwner, bulletWasIntercepted);

			if (this->Transact)
			{
				std::vector<TechnoClass*> transactTargets(targets.begin(), targets.end());
				this->TransactOnAllUnits(transactTargets, pHouse, pOwner);
			}
		}
		else if (pBullet && pBullet->Target)
		{
			if (pBullet->DistanceFrom(pBullet->Target) < Unsorted::LeptonsPerCell / 4) {
				switch (pBullet->Target->WhatAmI())
				{
				case BuildingClass::AbsID:
				case AircraftClass::AbsID:
				case UnitClass::AbsID:
				case InfantryClass::AbsID:
				{
					const auto Eligible = [&](TechnoClass* const pTech) -> TechnoClass*
						{
							if (CanDealDamage(pTech) &&
								CanTargetHouse(pHouse, pTech) &&
								pTech->GetTechnoType()->Trainable
								) return pTech;

							return nullptr;
						};

					this->DetonateOnOneUnit(pHouse, static_cast<TechnoClass*>(pBullet->Target), pOwner, bulletWasIntercepted);

					if (this->Transact)
					{
						this->TransactOnOneUnit(Eligible(static_cast<TechnoClass*>(pBullet->Target)), pOwner, 1);
					}

				}break;
				case CellClass::AbsID:
				{
					if (this->Transact)
						this->TransactOnOneUnit(nullptr, pOwner, 1);
				}break;
				default:
					break;
				}
			}
		}
		else if (auto pIntended = this->IntendedTarget)
		{
			if (coords.DistanceFrom(pIntended->GetCoords()) < double(Unsorted::LeptonsPerCell / 4)) {
				this->DetonateOnOneUnit(pHouse, pIntended, pOwner, bulletWasIntercepted);

				if (this->Transact) {
					const auto NotEligible = [this, pHouse, pOwner](TechnoClass* const pTech) {
						if (!CanDealDamage(pTech))
							return true;

						if (!pTech->GetTechnoType()->Trainable && this->Transact_Experience_IgnoreNotTrainable)
							return true;

						return !CanTargetHouse(pHouse, pTech);
					};

					std::vector<TechnoClass*> targets = { pIntended };
					this->TransactOnAllUnits(targets, pHouse, pOwner);
				}
			}
		}
	}
}

void WarheadTypeExt::ExtData::DetonateOnOneUnit(HouseClass* pHouse, TechnoClass* pTarget, TechnoClass* pOwner, bool bulletWasIntercepted)
{
	if (!pTarget || pTarget->InLimbo || !pTarget->IsAlive || !pTarget->Health || pTarget->IsSinking || pTarget->BeingWarpedOut)
		return;

	if (!this->CanTargetHouse(pHouse, pTarget) || !this->CanAffectTarget(pTarget))
		return;

	this->ApplyShieldModifiers(pTarget);

	if (this->RemoveDisguise)
		this->ApplyRemoveDisguise(pHouse, pTarget);

	if (this->RemoveMindControl)
		this->ApplyRemoveMindControl(pTarget);

	if (this->Crit_CurrentChance > 0.0 && (!this->Crit_SuppressWhenIntercepted || !bulletWasIntercepted))
		this->ApplyCrit(pHouse, pTarget, pOwner);

	if (this->Convert_Pairs.size() > 0)
		this->ApplyConvert(pHouse, pTarget);

	if (this->AttachEffects.AttachTypes.size() > 0 || this->AttachEffects.RemoveTypes.size() > 0 || this->AttachEffects.RemoveGroups.size() > 0)
		this->ApplyAttachEffects(pTarget, pHouse, pOwner);

	if (this->StealMoney_Amount != 0 && pOwner)
		this->ApplyStealMoney(pOwner, pTarget);

	if (this->BuildingSell || this->BuildingUndeploy)
		this->ApplyBuildingUndeploy(pTarget);

#ifdef LOCO_TEST_WARHEADS
	if (this->InflictLocomotor)
		this->ApplyLocomotorInfliction(pTarget);

	if (this->RemoveInflictedLocomotor)
		this->ApplyLocomotorInflictionReset(pTarget);
#endif

}

void WarheadTypeExt::ExtData::ApplyBuildingUndeploy(TechnoClass* pTarget)
{
	const auto pBuilding = abstract_cast<BuildingClass*, true>(pTarget);

	if (!pBuilding || !pBuilding->IsAlive || pBuilding->Health <= 0 || !pBuilding->IsOnMap || pBuilding->InLimbo)
		return;

	// Higher priority for selling
	if (this->BuildingSell)
	{
		if ((pBuilding->CanBeSold() && !pBuilding->IsStrange()) || this->BuildingSell_IgnoreUnsellable)
		{
			pBuilding->SetArchiveTarget(nullptr); // Reset to ensure it must to be sold
			pBuilding->Sell(1);
		}

		return;
	}

	// CanBeSold() will also check this
	const auto mission = pBuilding->CurrentMission;

	if (mission == Mission::Construction || mission == Mission::Selling)
		return;

	const auto pType = pBuilding->Type;

	if (!pType->UndeploysInto || (pType->ConstructionYard && !GameModeOptionsClass::Instance.MCVRedeploy))
		return;

	auto cell = pBuilding->GetMapCoords();
	const auto width = pType->GetFoundationWidth();
	const auto height = pType->GetFoundationHeight(false);

	// Offset of undeployment on large-scale buildings
	if (width > 2 || height > 2)
		cell += CellStruct { 1, 1 };

	if (this->BuildingUndeploy_Leave)
	{
		const auto pHouse = pBuilding->Owner;
		const auto pItems = Helpers::Alex::getCellSpreadItems(pBuilding->GetCoords(), 20);

		// Divide the surrounding units into 16 directions and record their costs
		int record[16] = {0};

		for (const auto& pItem : pItems)
		{
			// Only armed units that are not considered allies will be recorded
			if ((!pHouse || !pHouse->IsAlliedWith(pItem)) && pItem->IsArmed())
				record[pBuilding->GetTargetDirection(pItem).GetValue<4>()] += pItem->GetTechnoType()->Cost;
		}

		int costs = 0;
		int dir = 0;

		// Starting from 16, prevent negative numbers
		for (int i = 16; i < 32; ++i)
		{
			int newCosts = 0;

			// Assign weights to values in the direction
			// The highest value being 8 times in the positive direction
			// And the lowest value being 0 times in the opposite direction
			// The greater difference between positive direction to both sides, the lower value it is
			for (int j = -7; j < 8; ++j)
				newCosts += ((8 - std::abs(j)) * record[(i + j) & 15]);

			// Record the direction with the highest weight
			if (newCosts > costs)
			{
				dir = (i - 16);
				costs = newCosts;
			}
		}

		// If there is no threat in the surrounding area, randomly select one side
		if (!costs)
			dir = ScenarioClass::Instance->Random.RandomRanged(0, 15);

		// Reverse the direction and convert it into radians
		const double radian = -(((dir - 4) / 16.0) * Math::TwoPi);

		// Base on a location about 14 grids away
		cell.X -= static_cast<short>(14 * cos(radian));
		cell.Y += static_cast<short>(14 * sin(radian));

		// Find a location where the conyard can be deployed
		const auto newCell = MapClass::Instance.NearByLocation(cell, pType->UndeploysInto->SpeedType, -1, pType->UndeploysInto->MovementZone,
			false, (width + 2), (height + 2), false, false, false, false, CellStruct::Empty, false, false);

		// If it can find a more suitable location, go to the new one
		if (newCell != CellStruct::Empty)
			cell = newCell;
	}

	if (const auto pCell = MapClass::Instance.TryGetCellAt(cell))
		pBuilding->SetArchiveTarget(pCell);

	pBuilding->Sell(1);
}

void WarheadTypeExt::ExtData::ApplyShieldModifiers(TechnoClass* pTarget)
{
	auto const pTargetExt = TechnoExt::ExtMap.Find(pTarget);
	auto& pShield = pTargetExt->Shield;
	int shieldIndex = -1;
	double ratio = 1.0;

	// Remove shield.
	if (pShield)
	{
		const auto shieldType = pShield->GetType();
		shieldIndex = this->Shield_RemoveTypes.IndexOf(shieldType);

		if (shieldIndex >= 0 || this->Shield_RemoveAll)
		{
			ratio = pShield->GetHealthRatio();
			pTargetExt->CurrentShieldType = nullptr;
			pShield->KillAnim();
			pShield = nullptr;
		}
	}

	// Attach shield.
	if (this->Shield_AttachTypes.size() > 0)
	{
		ShieldTypeClass* shieldType = nullptr;

		if (this->Shield_ReplaceOnly)
		{
			if (shieldIndex >= 0)
				shieldType = this->Shield_AttachTypes[Math::min(shieldIndex, (signed)this->Shield_AttachTypes.size() - 1)];
			else if (this->Shield_RemoveAll)
				shieldType = this->Shield_AttachTypes[0];
		}
		else
		{
			shieldType = this->Shield_AttachTypes[0];
		}

		if (shieldType)
		{
			if (shieldType->Strength
				&& (!pShield
					|| (this->Shield_ReplaceNonRespawning
						&& pShield->IsBrokenAndNonRespawning()
						&& pShield->GetFramesSinceLastBroken() >= this->Shield_MinimumReplaceDelay)))
			{
				pTargetExt->CurrentShieldType = shieldType;
				pShield = std::make_unique<ShieldClass>(pTarget, true);
				pShield->UpdateTint();

				if (this->Shield_ReplaceOnly && this->Shield_InheritStateOnReplace)
				{
					pShield->SetHP((int)(shieldType->Strength * ratio));

					if (pShield->GetHP() == 0)
						pShield->SetRespawn(shieldType->Respawn_Rate, shieldType->Respawn, shieldType->Respawn_Rate, true);
				}
			}
		}
	}

	// Apply other modifiers.
	if (pShield)
	{
		const auto shieldType = pShield->GetType();

		auto isShieldTypeEligible = [pTargetExt, shieldType](Iterator<ShieldTypeClass*> pShieldTypeList) -> bool
			{
				return !(pShieldTypeList.size() > 0 && !pShieldTypeList.contains(shieldType));
			};

		if (this->Shield_Break && pShield->IsActive() && isShieldTypeEligible(this->Shield_Break_Types.GetElements(this->Shield_AffectTypes)))
			pShield->BreakShield(this->Shield_BreakAnim, this->Shield_BreakWeapon);

		if (this->Shield_Respawn_Duration > 0 && isShieldTypeEligible(this->Shield_Respawn_Types.GetElements(this->Shield_AffectTypes)))
		{
			const double amount = this->Shield_Respawn_Amount.Get(shieldType->Respawn);
			pShield->SetRespawn(this->Shield_Respawn_Duration, amount, this->Shield_Respawn_Rate, this->Shield_Respawn_RestartTimer);
		}

		if (this->Shield_SelfHealing_Duration > 0 && isShieldTypeEligible(this->Shield_SelfHealing_Types.GetElements(this->Shield_AffectTypes)))
		{
			const double amount = this->Shield_SelfHealing_Amount.Get(shieldType->SelfHealing);

			pShield->SetSelfHealing(this->Shield_SelfHealing_Duration, amount, this->Shield_SelfHealing_Rate,
				this->Shield_SelfHealing_RestartInCombat.Get(shieldType->SelfHealing_RestartInCombat),
				this->Shield_SelfHealing_RestartInCombatDelay, this->Shield_SelfHealing_RestartTimer);
		}
	}
}

void WarheadTypeExt::ExtData::ApplyRemoveDisguise(HouseClass* pHouse, TechnoClass* pTarget)
{
	if (pTarget->IsDisguised())
	{
		if (const auto pSpy = specific_cast<InfantryClass*, true>(pTarget))
			pSpy->Disguised = false;
		else if (const auto pMirage = specific_cast<UnitClass*, true>(pTarget))
			pMirage->ClearDisguise();
	}
}

void WarheadTypeExt::ExtData::ApplyRemoveMindControl(TechnoClass* pTarget)
{
	if (const auto pController = pTarget->MindControlledBy)
		pController->CaptureManager->FreeUnit(pTarget);
}

void WarheadTypeExt::ExtData::ApplyCrit(HouseClass* pHouse, TechnoClass* pTarget, TechnoClass* pOwner)
{
	double dice;

	if (this->Crit_ApplyChancePerTarget)
		dice = ScenarioClass::Instance->Random.RandomDouble();
	else
		dice = this->Crit_RandomBuffer;

	if (this->Crit_CurrentChance < dice)
		return;

	auto const pTargetExt = TechnoExt::ExtMap.Find(pTarget);

	if (pTargetExt->TypeExtData->ImmuneToCrit)
		return;

	auto const pSld = pTargetExt->Shield.get();

	if (pSld && pSld->IsActive() && pSld->GetType()->ImmuneToCrit)
		return;

	if (!TechnoExt::IsHealthInThreshold(pTarget, this->Crit_AffectAbovePercent, this->Crit_AffectBelowPercent))
		return;

	if (pHouse && !EnumFunctions::CanTargetHouse(this->Crit_AffectsHouses, pHouse, pTarget->Owner))
		return;

	if (!EnumFunctions::IsCellEligible(pTarget->GetCell(), this->Crit_Affects))
		return;

	if (!EnumFunctions::IsTechnoEligible(pTarget, this->Crit_Affects))
		return;

	this->Crit_Active = true;

	if (this->Crit_AnimOnAffectedTargets && this->Crit_AnimList.size())
	{
		if (!this->Crit_AnimList_CreateAll.Get(false))
		{
			const int idx = this->OwnerObject()->EMEffect || this->Crit_AnimList_PickRandom.Get(false)
				? ScenarioClass::Instance->Random.RandomRanged(0, this->Crit_AnimList.size() - 1) : 0;

			auto const pAnim = GameCreate<AnimClass>(this->Crit_AnimList[idx], pTarget->Location);
			AnimExt::SetAnimOwnerHouseKind(pAnim, pHouse, nullptr, false, true);
			AnimExt::ExtMap.Find(pAnim)->SetInvoker(pOwner, pHouse);
		}
		else
		{
			for (auto const& pType : this->Crit_AnimList)
			{
				auto const pAnim = GameCreate<AnimClass>(pType, pTarget->Location);
				AnimExt::SetAnimOwnerHouseKind(pAnim, pHouse, nullptr, false, true);
				AnimExt::ExtMap.Find(pAnim)->SetInvoker(pOwner, pHouse);
			}
		}
	}

	int damage = this->Crit_ExtraDamage.Get();

	if (this->Crit_ExtraDamage_ApplyFirepowerMult && pOwner)
		damage = static_cast<int>(damage * TechnoExt::GetCurrentFirepowerMultiplier(pOwner));

	if (this->Crit_Warhead)
	{
		if (this->Crit_Warhead_FullDetonation)
			WarheadTypeExt::DetonateAt(this->Crit_Warhead, pTarget, pOwner, damage, pHouse);
		else
			this->DamageAreaWithTarget(pTarget->GetCoords(), damage, pOwner, this->Crit_Warhead, true, pHouse, pTarget);
	}
	else
		pTarget->ReceiveDamage(&damage, 0, this->OwnerObject(), pOwner, false, false, pHouse);
}

void WarheadTypeExt::ExtData::InterceptBullets(TechnoClass* pOwner, BulletClass* pInterceptor, const CoordStruct& coords)
{
	const float cellSpread = this->OwnerObject()->CellSpread;

	if (cellSpread == 0.0)
	{
		if (const auto pBullet = abstract_cast<BulletClass*>(pInterceptor->Target))
		{
			const auto pBulletExt = BulletExt::ExtMap.Find(pBullet);

			if (!pBulletExt->TypeExtData->Interceptable)
				return;

			// 1/8th of a cell as a margin of error if not Inviso interceptor.
			if (pInterceptor->Type->Inviso || pBullet->Location.DistanceFrom(coords) <= Unsorted::LeptonsPerCell / 8.0)
				pBulletExt->InterceptBullet(pOwner, pInterceptor);
		}
	}
	else
	{
		for (const auto& pBullet : BulletClass::Array)
		{
			const auto pBulletExt = BulletExt::ExtMap.Find(pBullet);

			// Cells don't know about bullets that may or may not be located on them so it has to be this way.
			if (!pBulletExt->TypeExtData->Interceptable || pBullet->SpawnNextAnim)
				continue;

			if (pBullet->Location.DistanceFrom(coords) <= cellSpread * Unsorted::LeptonsPerCell)
				pBulletExt->InterceptBullet(pOwner, pInterceptor);
		}
	}
}

void WarheadTypeExt::ExtData::ApplyConvert(HouseClass* pHouse, TechnoClass* pTarget)
{
	auto pTargetFoot = abstract_cast<FootClass*, true>(pTarget);

	if (!pTargetFoot)
		return;

	TypeConvertGroup::Convert(pTargetFoot, this->Convert_Pairs, pHouse);
}

void WarheadTypeExt::ExtData::ApplyLocomotorInfliction(TechnoClass* pTarget)
{
	auto pTargetFoot = abstract_cast<FootClass*, true>(pTarget);

	if (!pTargetFoot)
		return;

	// same locomotor? no point to change
	CLSID targetCLSID { };
	CLSID inflictCLSID = this->OwnerObject()->Locomotor;
	IPersistPtr pLocoPersist = pTargetFoot->Locomotor;
	if (SUCCEEDED(pLocoPersist->GetClassID(&targetCLSID)) && targetCLSID == inflictCLSID)
		return;

	// prevent endless piggyback
	IPiggybackPtr pTargetPiggy = pTargetFoot->Locomotor;
	if (pTargetPiggy != nullptr && pTargetPiggy->Is_Piggybacking())
		return;

	LocomotionClass::ChangeLocomotorTo(pTargetFoot, inflictCLSID);
}

void WarheadTypeExt::ExtData::ApplyLocomotorInflictionReset(TechnoClass* pTarget)
{
	auto pTargetFoot = abstract_cast<FootClass*, true>(pTarget);

	if (!pTargetFoot)
		return;

	// remove only specific inflicted locomotor if specified
	CLSID removeCLSID = this->OwnerObject()->Locomotor;
	if (removeCLSID != CLSID())
	{
		CLSID targetCLSID { };
		IPersistPtr pLocoPersist = pTargetFoot->Locomotor;
		if (SUCCEEDED(pLocoPersist->GetClassID(&targetCLSID)) && targetCLSID != removeCLSID)
			return;
	}

	// // we don't want to remove non-ok-to-end locos
	// IPiggybackPtr pTargetPiggy = pTargetFoot->Locomotor;
	// if (pTargetPiggy != nullptr && (!pTargetPiggy->Is_Ok_To_End()))
	// 	return;

	LocomotionClass::End_Piggyback(pTargetFoot->Locomotor);
}

void WarheadTypeExt::ExtData::ApplyAttachEffects(TechnoClass* pTarget, HouseClass* pInvokerHouse, TechnoClass* pInvoker)
{
	if (!pTarget)
		return;

	std::vector<int> dummy = std::vector<int>();
	auto const& info = this->AttachEffects;
	AttachEffectClass::Attach(pTarget, pInvokerHouse, pInvoker, this->OwnerObject(), info);
	AttachEffectClass::Detach(pTarget, info);
	AttachEffectClass::DetachByGroups(pTarget, info);
}

double WarheadTypeExt::ExtData::GetCritChance(TechnoClass* pFirer) const
{
	double critChance = this->Crit_Chance;

	if (!pFirer)
		return critChance;

	auto const pExt = TechnoExt::ExtMap.Find(pFirer);
	double extraChance = 0.0;

	for (auto const& attachEffect : pExt->AttachedEffects)
	{
		if (!attachEffect->IsActive())
			continue;

		auto const pType = attachEffect->GetType();

		if (pType->Crit_Multiplier == 1.0 && pType->Crit_ExtraChance == 0.0)
			continue;

		auto const& allowWarheads = pType->Crit_AllowWarheads;
		auto const pObject = this->OwnerObject();

		if (allowWarheads.size() > 0 && !allowWarheads.Contains(pObject))
			continue;

		auto const& disallowWarheads = pType->Crit_DisallowWarheads;

		if (disallowWarheads.size() > 0 && disallowWarheads.Contains(pObject))
			continue;

		critChance = critChance * Math::max(pType->Crit_Multiplier, 0);
		extraChance += pType->Crit_ExtraChance;
	}

	return critChance + extraChance;
}

void WarheadTypeExt::ExtData::ApplyStealMoney(TechnoClass* pOwner, TechnoClass* pTarget) const
{
	const int stealAmount = this->StealMoney_Amount;

	if (stealAmount != 0 && pOwner && pTarget)
	{
		auto pOwnerHouse = pOwner->GetOwningHouse();
		auto pTargetHouse = pTarget->GetOwningHouse();

		if (pOwnerHouse && pTargetHouse && 
			!pOwnerHouse->IsNeutral() && !pTargetHouse->IsNeutral() &&
			pOwnerHouse != pTargetHouse)
		{
			if (pOwnerHouse->CanTransactMoney(stealAmount) && pTargetHouse->CanTransactMoney(-stealAmount))
			{
				pOwnerHouse->TransactMoney(stealAmount);
				pTargetHouse->TransactMoney(-stealAmount);

				if (this->StealMoney_Display)
				{
					FlyingStrings::AddMoneyString(stealAmount, pOwnerHouse, this->StealMoney_Display_Houses, pOwner->GetCoords(), this->StealMoney_Display_Offset);
					FlyingStrings::AddMoneyString(-stealAmount, pTargetHouse, this->StealMoney_Display_Houses, pTarget->GetCoords(), this->StealMoney_Display_Offset);
				}
			}
		}
	}
}
