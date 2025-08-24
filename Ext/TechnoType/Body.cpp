#include "Body.h"

#include <AircraftTrackerClass.h>
#include <AnimClass.h>
#include <FlyLocomotionClass.h>
#include <JumpjetLocomotionClass.h>
#include <TechnoTypeClass.h>
#include <StringTable.h>
#include <ScenarioClass.h>
#include <VocClass.h>

#include <Ext/Anim/Body.h>
#include <Ext/BuildingType/Body.h>
#include <Ext/BulletType/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/WeaponType/Body.h>
#include <New/Type/InsigniaTypeClass.h>

#include <Utilities/GeneralUtils.h>

TechnoTypeExt::ExtContainer TechnoTypeExt::ExtMap;
bool TechnoTypeExt::SelectWeaponMutex = false;
bool TechnoTypeExt::ExtData::ExtraFireInProgress = false;

void TechnoTypeExt::ExtData::ApplyTurretOffset(Matrix3D* mtx, double factor)
{
	// Does not verify if the offset actually has all values parsed as it makes no difference, it will be 0 for the unparsed ones either way.
	const auto offset = this->TurretOffset.GetEx();
	const float x = static_cast<float>(offset->X * factor);
	const float y = static_cast<float>(offset->Y * factor);
	const float z = static_cast<float>(offset->Z * factor);

	mtx->Translate(x, y, z);
}

int TechnoTypeExt::ExtData::SelectForceWeapon(TechnoClass* pThis, AbstractClass* pTarget)
{
	if (TechnoTypeExt::SelectWeaponMutex || !this->ForceWeapon_Check || !pTarget) // In theory, pTarget must exist
		return -1;

	int forceWeaponIndex = -1;
	const auto pTargetTechno = abstract_cast<TechnoClass*>(pTarget);
	TechnoTypeClass* pTargetType = nullptr;

	if (pTargetTechno)
	{
		pTargetType = pTargetTechno->GetTechnoType();

		if (this->ForceWeapon_Naval_Decloaked >= 0
			&& pTargetType->Cloakable
			&& pTargetType->Naval
			&& pTargetTechno->CloakState == CloakState::Uncloaked)
		{
			forceWeaponIndex = this->ForceWeapon_Naval_Decloaked;
		}
		else if (this->ForceWeapon_Cloaked >= 0
			&& pTargetTechno->CloakState == CloakState::Cloaked)
		{
			forceWeaponIndex = this->ForceWeapon_Cloaked;
		}
		else if (this->ForceWeapon_Disguised >= 0
			&& pTargetTechno->IsDisguised())
		{
			forceWeaponIndex = this->ForceWeapon_Disguised;
		}
		else if (this->ForceWeapon_UnderEMP >= 0
			&& pTargetTechno->IsUnderEMP())
		{
			forceWeaponIndex = this->ForceWeapon_UnderEMP;
		}
	}

	if (forceWeaponIndex == -1
		&& (pTargetTechno || !this->ForceWeapon_InRange_TechnoOnly)
		&& (!this->ForceWeapon_InRange.empty() || !this->ForceAAWeapon_InRange.empty()))
	{
		TechnoTypeExt::SelectWeaponMutex = true;
		forceWeaponIndex = TechnoExt::ExtMap.Find(pThis)->ApplyForceWeaponInRange(pTarget);
		TechnoTypeExt::SelectWeaponMutex = false;
	}

	if (forceWeaponIndex == -1 && pTargetType)
	{
		switch (pTarget->WhatAmI())
		{
			case AbstractType::Building:
			{
				forceWeaponIndex = this->ForceWeapon_Buildings;

				if (this->ForceWeapon_Defenses >= 0)
				{
					auto const pBuildingType = static_cast<BuildingTypeClass*>(pTargetType);

					if (pBuildingType->BuildCat == BuildCat::Combat)
						forceWeaponIndex = this->ForceWeapon_Defenses;
				}

				break;
			}
			case AbstractType::Infantry:
			{
				forceWeaponIndex = (this->ForceAAWeapon_Infantry >= 0 && pTarget->IsInAir())
					? this->ForceAAWeapon_Infantry : this->ForceWeapon_Infantry;

				break;
			}
			case AbstractType::Unit:
			{
				forceWeaponIndex = (this->ForceAAWeapon_Units >= 0 && pTarget->IsInAir())
					? this->ForceAAWeapon_Units : ((this->ForceWeapon_Naval_Units >= 0 && pTargetType->Naval)
					? this->ForceWeapon_Naval_Units : this->ForceWeapon_Units);

				break;
			}
			case AbstractType::Aircraft:
			{
				forceWeaponIndex = (this->ForceAAWeapon_Aircraft >= 0 && pTarget->IsInAir())
					? this->ForceAAWeapon_Aircraft : this->ForceWeapon_Aircraft;

				break;
			}
		}
	}

	return forceWeaponIndex;
}

bool TechnoTypeExt::ExtData::IsSecondary(int nWeaponIndex)
{
	const auto pThis = this->OwnerObject();

	if (pThis->IsGattling)
		return nWeaponIndex != 0 && nWeaponIndex % 2 != 0;

	if (this->MultiWeapon.Get() && nWeaponIndex >= 0
		&& !this->MultiWeapon_IsSecondary.empty())
	{
		return this->MultiWeapon_IsSecondary[nWeaponIndex];
	}

	return nWeaponIndex != 0;
}

int TechnoTypeExt::ExtData::SelectMultiWeapon(TechnoClass* const pThis, AbstractClass* const pTarget)
{
	// Keep original early exits. :contentReference[oaicite:1]{index=1}
	if (!pTarget || !this->MultiWeapon.Get())
		return -1;

	const auto pType = this->OwnerObject();
	if (pType->IsGattling || (pType->HasMultipleTurrets() && pType->Gunner))
		return -1;

	const int weaponCount = Math::min(pType->WeaponCount, this->MultiWeapon_SelectCount.Get());
	if (weaponCount < 2)  return 0;
	if (weaponCount == 2) return -1;

	const bool isElite = pThis->Veterancy.IsElite();
	const bool noSecondary = this->NoSecondaryWeaponFallback.Get();

	// --- perf: stack bitmask instead of heap vector<bool> ---
	// bit i == 1  -> "skip index i in the primary pass" (same meaning as 'secondaryCanTargets[i] == true'). :contentReference[oaicite:2]{index=2}
	uint64_t skipMask64 = 0ull;
	auto markSkip = [&](int idx)
		{
			if (static_cast<unsigned>(idx) < 64u) skipMask64 |= (1ull << idx);
		};
	auto isSkipped = [&](int idx) -> bool
		{
			return (static_cast<unsigned>(idx) < 64u) ? ((skipMask64 >> idx) & 1ull) != 0ull : false;
		};

	// cache the target as Techno, if any (original logic) :contentReference[oaicite:3]{index=3}
	if (const auto pTargetTechno = abstract_cast<TechnoClass*, true>(pTarget))
	{
		// dead/invalid → default to primary (original) :contentReference[oaicite:4]{index=4}
		if (pTargetTechno->Health <= 0 || !pTargetTechno->IsAlive)
			return 0;

		bool getNavalTargeting = false;

		// perf: avoid duplicate GetWeaponType calls by caching when we touch an index
		// (not strictly necessary, but keeps the hot path tight)
		auto checkSecondary = [&](int weaponIndex, WeaponTypeClass* pWeaponCached = nullptr) -> bool
			{
				const auto pWeapon = pWeaponCached ? pWeaponCached : TechnoTypeExt::GetWeaponType(pType, weaponIndex, isElite);

				if (!pWeapon || pWeapon->NeverUse)
				{
					markSkip(weaponIndex);
					return false;
				}

				bool secondaryPriority = getNavalTargeting;

				if (!getNavalTargeting)
				{
					const auto pWH = pWeapon->Warhead;
					const bool isAllies = pThis->Owner->IsAlliedWith(pTargetTechno->Owner);
					const bool isInAir = pTargetTechno->IsInAir();

					if (pWH->Airstrike)
					{
						// Can it attack the air force now? (preserve exact fallback flags) :contentReference[oaicite:5]{index=5}
						secondaryPriority = isInAir ? !this->NoSecondaryWeaponFallback_AllowAA.Get() : !noSecondary;
					}
					else if (isInAir)
					{
						secondaryPriority = !this->NoSecondaryWeaponFallback_AllowAA.Get();
					}
					else if (pWeapon->DrainWeapon
						&& pTargetTechno->GetTechnoType()->Drainable
						&& !pThis->DrainTarget && !isAllies)
					{
						secondaryPriority = !noSecondary;
					}
					else if (pWH->ElectricAssault && isAllies
						&& pTargetTechno->WhatAmI() == AbstractType::Building
						&& static_cast<BuildingClass*>(pTargetTechno)->Type->Overpowerable)
					{
						secondaryPriority = !noSecondary;
					}
				}

				if (secondaryPriority)
				{
					// try secondary first; if it *can’t* fire, mark to skip in the primary pass (original semantics) :contentReference[oaicite:6]{index=6}
					if (TechnoExt::MultiWeaponCanFire(pThis, pTargetTechno, pWeapon))
						return true;

					markSkip(weaponIndex);
					return false;
				}

				return false;
			};

		// Determine naval targeting once (original flow) — guard GetCell() for safety. :contentReference[oaicite:7]{index=7}
		if (auto* const pCell = pTargetTechno->GetCell())
		{
			const LandType landType = pCell->LandType;
			const bool targetOnWater = (landType == LandType::Water) || (landType == LandType::Beach);

			if (!pTargetTechno->OnBridge && targetOnWater)
			{
				const int result = pThis->SelectNavalTargeting(pTargetTechno);
				if (result != -1)
					getNavalTargeting = (result == 1);
			}
		}

		const bool hasSecondaryList = !this->MultiWeapon_IsSecondary.empty();

		// original iteration order preserved (either [0..N) filtered by list, or [1..N) when no list) :contentReference[oaicite:8]{index=8}
		for (int idx = hasSecondaryList ? 0 : 1; idx < weaponCount; ++idx)
		{
			if (hasSecondaryList && !this->MultiWeapon_IsSecondary[idx])
				continue;

			if (checkSecondary(idx))
				return idx;
		}
	}
	else if (noSecondary)
	{
		// non-techno targets with no-secondary fallback → primary (unchanged) :contentReference[oaicite:9]{index=9}
		return 0;
	}

	// Primary pass: try any index that wasn't proven “skip” above. Keep order & conditions identical. :contentReference[oaicite:10]{index=10}
	for (int idx = 0; idx < weaponCount; ++idx)
	{
		if (isSkipped(idx))
			continue;

		if (TechnoExt::MultiWeaponCanFire(pThis, pTarget, TechnoTypeExt::GetWeaponType(pType, idx, isElite)))
			return idx;
	}

	return 0;
}

// Ares 0.A source
const char* TechnoTypeExt::ExtData::GetSelectionGroupID() const
{
	return GeneralUtils::IsValidString(this->GroupAs) ? this->GroupAs : this->OwnerObject()->ID;
}

const char* TechnoTypeExt::GetSelectionGroupID(ObjectTypeClass* pType)
{
	if (const auto pExt = TechnoTypeExt::ExtMap.TryFind(static_cast<TechnoTypeClass*>(pType)))
		return pExt->GetSelectionGroupID();

	return pType->ID;
}

bool TechnoTypeExt::HasSelectionGroupID(ObjectTypeClass* pType, const char* pID)
{
	const auto id = TechnoTypeExt::GetSelectionGroupID(pType);

	return (_strcmpi(id, pID) == 0);
}

void TechnoTypeExt::ExtData::ParseBurstFLHs(INI_EX& exArtINI, const char* pArtSection,
	std::vector<std::vector<CoordStruct>>& nFLH, std::vector<std::vector<CoordStruct>>& nEFlh, const char* pPrefixTag)
{
	char tempBuffer[32];
	char tempBufferFLH[48];
	auto pThis = this->OwnerObject();
	bool parseMultiWeapons = this->MultiWeapon.Get() || (pThis->TurretCount > 0 && pThis->WeaponCount > 0);
	auto weaponCount = parseMultiWeapons ? pThis->WeaponCount : 2;
	nFLH.resize(weaponCount);
	nEFlh.resize(weaponCount);

	for (int i = 0; i < weaponCount; i++)
	{
		for (int j = 0; j < INT_MAX; j++)
		{
			_snprintf_s(tempBuffer, sizeof(tempBuffer), "%sWeapon%d", pPrefixTag, i + 1);
			auto prefix = parseMultiWeapons ? tempBuffer : i > 0 ? "%sSecondaryFire" : "%sPrimaryFire";
			_snprintf_s(tempBuffer, sizeof(tempBuffer), prefix, pPrefixTag);

			_snprintf_s(tempBufferFLH, sizeof(tempBufferFLH), "%sFLH.Burst%d", tempBuffer, j);
			Nullable<CoordStruct> FLH;
			FLH.Read(exArtINI, pArtSection, tempBufferFLH);

			_snprintf_s(tempBufferFLH, sizeof(tempBufferFLH), "Elite%sFLH.Burst%d", tempBuffer, j);
			Nullable<CoordStruct> eliteFLH;
			eliteFLH.Read(exArtINI, pArtSection, tempBufferFLH);

			if (FLH.isset() && !eliteFLH.isset())
				eliteFLH = FLH;
			else if (!FLH.isset() && !eliteFLH.isset())
				break;

			nFLH[i].emplace_back(FLH.Get());
			nEFlh[i].emplace_back(eliteFLH.Get());
		}
	}
}

void TechnoTypeExt::ExtData::ParseVoiceWeaponAttacks(INI_EX& exINI, const char* pSection, ValueableVector<int>& voice, ValueableVector<int>& voiceElite)
{
	if (!this->ReadMultiWeapon)
	{
		voice.clear();
		voiceElite.clear();
		return;
	}

	const auto pThis = this->OwnerObject();
	const auto weaponCount = Math::max(pThis->WeaponCount, 0);

	while (int(voice.size()) > weaponCount)
	{
		voice.erase(voice.begin() + int(voice.size()) - 1);
	}

	while (int(voiceElite.size()) > weaponCount)
	{
		voiceElite.erase(voiceElite.begin() + int(voiceElite.size()) - 1);
	}

	char tempBuff[64];
	for (int index = 0; index < weaponCount; index++)
	{
		NullableIdx<VocClass> VoiceAttack;
		_snprintf_s(tempBuff, sizeof(tempBuff), "VoiceWeapon%dAttack", index + 1);
		VoiceAttack.Read(exINI, pSection, tempBuff);

		NullableIdx<VocClass> VoiceEliteAttack;
		_snprintf_s(tempBuff, sizeof(tempBuff), "VoiceEliteWeapon%dAttack", index + 1);
		VoiceAttack.Read(exINI, pSection, tempBuff);

		if (int(voice.size()) > index)
		{
			voice[index] = VoiceAttack.Get(voice[index]);
			voiceElite[index] = VoiceEliteAttack.Get(voiceElite[index]);
		}
		else
		{
			const int voiceAttack = VoiceAttack.Get(-1);
			voice.push_back(voiceAttack);
			voiceElite.push_back(VoiceEliteAttack.Get(voiceAttack));
		}
	}
}

void TechnoTypeExt::ExtData::CalculateSpawnerRange()
{
	const auto pThis = this->OwnerObject();
	const int weaponRangeExtra = this->Spawner_ExtraLimitRange * Unsorted::LeptonsPerCell;
	this->SpawnerRange = 0;
	this->EliteSpawnerRange = 0;

	auto setWeaponRange = [](int& range, WeaponTypeClass* pWeapon)
		{
			if (pWeapon && pWeapon->Spawner && pWeapon->Range > range)
				range = pWeapon->Range;
		};

	const bool multiweapon = this->MultiWeapon.Get() || (pThis->TurretCount > 0 && pThis->WeaponCount > 0);
	const int weaponCount = multiweapon ? pThis->WeaponCount : 2;

	for (int i = 0; i < weaponCount; ++i)
	{
		setWeaponRange(this->SpawnerRange, TechnoTypeExt::GetWeaponType(pThis, i, false));
		setWeaponRange(this->EliteSpawnerRange, TechnoTypeExt::GetWeaponType(pThis, i, true));
	}

	this->SpawnerRange += weaponRangeExtra;
	this->EliteSpawnerRange += weaponRangeExtra;
}

//TODO: YRpp this with proper casting
TechnoTypeClass* TechnoTypeExt::GetTechnoType(ObjectTypeClass* pType)
{
	enum class IUnknownVtbl : DWORD
	{
		AircraftType = 0x7E2868,
		BuildingType = 0x7E4570,
		InfantryType = 0x7EB610,
		UnitType = 0x7F6218,
	};
	auto const vtThis = static_cast<IUnknownVtbl>(VTable::Get(pType));
	if (vtThis == IUnknownVtbl::InfantryType
		|| vtThis == IUnknownVtbl::UnitType
		|| vtThis == IUnknownVtbl::AircraftType
		|| vtThis == IUnknownVtbl::BuildingType)
	{
		return static_cast<TechnoTypeClass*>(pType);
	}

	return nullptr;
}

TechnoClass* TechnoTypeExt::CreateUnit(CreateUnitTypeClass* pCreateUnit, DirType facing, DirType* secondaryFacing,
	CoordStruct location, HouseClass* pOwner, TechnoClass* pInvoker, HouseClass* pInvokerHouse)
{
	auto const pType = pCreateUnit->Type.Get();
	auto const rtti = pType->WhatAmI();

	if (rtti == AbstractType::BuildingType)
		return nullptr;

	HouseClass* decidedOwner = pOwner;

	if (!pOwner || pOwner->Defeated)
	{
		if (pCreateUnit->RequireOwner)
			return nullptr;

		decidedOwner = HouseClass::FindCivilianSide();
	}

	auto pCell = MapClass::Instance.TryGetCellAt(location);
	auto const speedType = rtti != AbstractType::AircraftType ? pType->SpeedType : SpeedType::Wheel;
	auto const mZone = rtti != AbstractType::AircraftType ? pType->MovementZone : MovementZone::Normal;
	const bool allowBridges = GroundType::Array[static_cast<int>(LandType::Clear)].Cost[static_cast<int>(speedType)] > 0.0;
	bool isBridge = allowBridges && pCell && pCell->ContainsBridge();
	const int baseHeight = location.Z;
	bool inAir = location.Z >= Unsorted::CellHeight * 2;

	if (pCreateUnit->ConsiderPathfinding && (!pCell || !pCell->IsClearToMove(speedType, false, false, -1, mZone, -1, isBridge)))
	{
		auto nCell = MapClass::Instance.NearByLocation(CellClass::Coord2Cell(location), speedType, -1, mZone,
			isBridge, 1, 1, true, false, false, isBridge, CellStruct::Empty, false, false);

		if (nCell != CellStruct::Empty)
		{
			pCell = MapClass::Instance.TryGetCellAt(nCell);

			if (pCell)
				location = pCell->GetCoords();
		}
	}

	if (pCell)
	{
		isBridge = allowBridges && pCell->ContainsBridge();
		const int bridgeZ = isBridge ? CellClass::BridgeHeight : 0;
		const int zCoord = pCreateUnit->AlwaysSpawnOnGround ? INT32_MIN : baseHeight;
		const int cellFloorHeight = MapClass::Instance.GetCellFloorHeight(location) + bridgeZ;

		if (!pCreateUnit->AlwaysSpawnOnGround && pCreateUnit->SpawnHeight >= 0)
			location.Z = cellFloorHeight + pCreateUnit->SpawnHeight;
		else
			location.Z = Math::max(cellFloorHeight, zCoord);

		if (auto const pTechno = static_cast<FootClass*>(pType->CreateObject(decidedOwner)))
		{
			bool success = false;
			bool parachuted = false;
			pTechno->OnBridge = isBridge;

			if (rtti != AbstractType::AircraftType && pCreateUnit->SpawnParachutedInAir && !pCreateUnit->AlwaysSpawnOnGround && inAir)
			{
				parachuted = true;
				success = pTechno->SpawnParachuted(location);
			}
			else if (!pCell->GetBuilding() || !pCreateUnit->ConsiderPathfinding)
			{
				++Unsorted::ScenarioInit;
				success = pTechno->Unlimbo(location, facing);
				--Unsorted::ScenarioInit;
			}
			else
			{
				success = pTechno->Unlimbo(location, facing);
			}

			if (success)
			{
				if (secondaryFacing)
					pTechno->SecondaryFacing.SetCurrent(DirStruct(*secondaryFacing));

				if (pCreateUnit->SpawnAnim)
				{
					auto const pAnim = GameCreate<AnimClass>(pCreateUnit->SpawnAnim, location);
					AnimExt::SetAnimOwnerHouseKind(pAnim, pInvokerHouse, nullptr, false, true);
					AnimExt::ExtMap.Find(pAnim)->SetInvoker(pInvoker, pInvokerHouse);
				}

				if (!pTechno->InLimbo)
				{
					if (!pCreateUnit->AlwaysSpawnOnGround)
					{
						inAir = pTechno->IsInAir();

						if (auto const pFlyLoco = locomotion_cast<FlyLocomotionClass*>(pTechno->Locomotor))
						{
							pTechno->SetLocation(location);
							const bool airportBound = rtti == AbstractType::AircraftType && static_cast<AircraftTypeClass*>(pType)->AirportBound;

							if (pCell->GetContent() || airportBound)
								pTechno->EnterIdleMode(false, true);
							else
								pFlyLoco->Move_To(pCell->GetCoordsWithBridge());
						}
						else if (auto const pJJLoco = locomotion_cast<JumpjetLocomotionClass*>(pTechno->Locomotor))
						{
							pJJLoco->LocomotionFacing.SetCurrent(DirStruct(facing));

							if (pType->BalloonHover)
							{
								// Makes the jumpjet think it is hovering without actually moving.
								pJJLoco->State = JumpjetLocomotionClass::State::Hovering;
								pJJLoco->IsMoving = true;
								pJJLoco->DestinationCoords = location;
								pJJLoco->CurrentHeight = pType->JumpjetHeight;

								if (!inAir)
									AircraftTrackerClass::Instance.Add(pTechno);
							}
							else if (inAir)
							{
								// Order non-BalloonHover jumpjets to land.
								pJJLoco->Move_To(location);
							}
						}
						else if (inAir && !parachuted)
						{
							pTechno->IsFallingDown = true;
						}
					}

					auto newMission = pCreateUnit->UnitMission;

					if (!decidedOwner->IsControlledByHuman() && pCreateUnit->AIUnitMission.isset())
						newMission = pCreateUnit->AIUnitMission;

					pTechno->QueueMission(newMission, false);
				}

				if (!decidedOwner->Type->MultiplayPassive)
					decidedOwner->RecheckTechTree = true;
			}
			else
			{
				if (pTechno)
					pTechno->UnInit();
			}

			return pTechno;
		}
	}

	return nullptr;
}

// used for more WeaponX added by Ares.
WeaponTypeClass* TechnoTypeExt::GetWeaponType(TechnoTypeClass* pThis, int weaponIndex, bool isElite)
{
	if (isElite)
	{
		if (const auto pEliteStruct = pThis->GetEliteWeapon(weaponIndex))
		{
			if (const auto pEliteWeapon = pEliteStruct->WeaponType)
				return pEliteWeapon;
		}
	}

	const auto pWeaponStruct = pThis->GetWeapon(weaponIndex);

	return pWeaponStruct ? pWeaponStruct->WeaponType : nullptr;
}

// =============================
// load / save

void TechnoTypeExt::ExtData::LoadFromINIFile(CCINIClass* const pINI)
{
	auto pThis = this->OwnerObject();
	const char* pSection = pThis->ID;
	INI_EX exINI(pINI);

	this->HealthBar_Hide.Read(exINI, pSection, "HealthBar.Hide");
	this->UIDescription.Read(exINI, pSection, "UIDescription");
	this->LowSelectionPriority.Read(exINI, pSection, "LowSelectionPriority");
	this->MindControlRangeLimit.Read(exINI, pSection, "MindControlRangeLimit");
	this->MindControlLink_VisibleToHouse.Read(exINI, pSection, "MindControlLink.VisibleToHouse");
	this->FactoryPlant_Multiplier.Read(exINI, pSection, "FactoryPlant.Multiplier");
	
	// Read custom armor name for HitAnim system
	char armorBuffer[128] = "";
	if (pINI->ReadString(pSection, "Armor", "", armorBuffer, sizeof(armorBuffer)) > 0)
	{
		std::string armorStr = armorBuffer;
		std::transform(armorStr.begin(), armorStr.end(), armorStr.begin(), 
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		this->CustomArmorName = armorStr;
	}
	else
	{
		this->CustomArmorName.clear();
	}

	this->Spawner_LimitRange.Read(exINI, pSection, "Spawner.LimitRange");
	this->Spawner_ExtraLimitRange.Read(exINI, pSection, "Spawner.ExtraLimitRange");
	this->Spawner_DelayFrames.Read(exINI, pSection, "Spawner.DelayFrames");
	this->Spawner_AttackImmediately.Read(exINI, pSection, "Spawner.AttackImmediately");
	this->Spawner_UseTurretFacing.Read(exINI, pSection, "Spawner.UseTurretFacing");

	this->Harvester_Counted.Read(exINI, pSection, "Harvester.Counted");
	if (!this->Harvester_Counted.isset() && pThis->Enslaves)
		this->Harvester_Counted = true;

	this->Promote_IncludeSpawns.Read(exINI, pSection, "Promote.IncludeSpawns");
	this->ImmuneToCrit.Read(exINI, pSection, "ImmuneToCrit");
	this->MultiMindControl_ReleaseVictim.Read(exINI, pSection, "MultiMindControl.ReleaseVictim");
	this->NoManualMove.Read(exINI, pSection, "NoManualMove");
	this->AllowFire_IroncurtainedTarget.Read(exINI, pSection, "Firing.AllowICedTargetForAI");
	this->IsDummy.Read(exINI, pSection, "Dummy");
	this->InitialStrength.Read(exINI, pSection, "InitialStrength");
	if (this->InitialStrength.isset())
		this->InitialStrength = Math::clamp(this->InitialStrength, 1, pThis->Strength);

	this->ReloadInTransport.Read(exINI, pSection, "ReloadInTransport");
	this->ForbidParallelAIQueues.Read(exINI, pSection, "ForbidParallelAIQueues");

	this->LaserTargetColor.Read(exINI, pSection, "LaserTargetColor");
	this->AirstrikeLineColor.Read(exINI, pSection, "AirstrikeLineColor");

	this->ShieldType.Read<true>(exINI, pSection, "ShieldType");

	this->HarvesterDumpAmount.Read(exINI, pSection, "HarvesterDumpAmount");

	this->Ammo_AddOnDeploy.Read(exINI, pSection, "Ammo.AddOnDeploy");
	this->Ammo_AutoDeployMinimumAmount.Read(exINI, pSection, "Ammo.AutoDeployMinimumAmount");
	this->Ammo_AutoDeployMaximumAmount.Read(exINI, pSection, "Ammo.AutoDeployMaximumAmount");
	this->Ammo_DeployUnlockMinimumAmount.Read(exINI, pSection, "Ammo.DeployUnlockMinimumAmount");
	this->Ammo_DeployUnlockMaximumAmount.Read(exINI, pSection, "Ammo.DeployUnlockMaximumAmount");

	this->AutoDeath_Behavior.Read(exINI, pSection, "AutoDeath.Behavior");
	this->AutoDeath_VanishAnimation.Read(exINI, pSection, "AutoDeath.VanishAnimation");
	this->AutoDeath_OnAmmoDepletion.Read(exINI, pSection, "AutoDeath.OnAmmoDepletion");
	this->AutoDeath_AfterDelay.Read(exINI, pSection, "AutoDeath.AfterDelay");
	this->AutoDeath_TechnosDontExist.Read(exINI, pSection, "AutoDeath.TechnosDontExist");
	this->AutoDeath_TechnosDontExist_Any.Read(exINI, pSection, "AutoDeath.TechnosDontExist.Any");
	this->AutoDeath_TechnosDontExist_AllowLimboed.Read(exINI, pSection, "AutoDeath.TechnosDontExist.AllowLimboed");
	this->AutoDeath_TechnosDontExist_Houses.Read(exINI, pSection, "AutoDeath.TechnosDontExist.Houses");
	this->AutoDeath_TechnosExist.Read(exINI, pSection, "AutoDeath.TechnosExist");
	this->AutoDeath_TechnosExist_Any.Read(exINI, pSection, "AutoDeath.TechnosExist.Any");
	this->AutoDeath_TechnosExist_AllowLimboed.Read(exINI, pSection, "AutoDeath.TechnosExist.AllowLimboed");
	this->AutoDeath_TechnosExist_Houses.Read(exINI, pSection, "AutoDeath.TechnosExist.Houses");

	this->Slaved_OwnerWhenMasterKilled.Read(exINI, pSection, "Slaved.OwnerWhenMasterKilled");
	this->SlavesFreeSound.Read(exINI, pSection, "SlavesFreeSound");
	this->SellSound.Read(exINI, pSection, "SellSound");
	this->EVA_Sold.Read(exINI, pSection, "EVA.Sold");

	this->CombatAlert.Read(exINI, pSection, "CombatAlert");
	this->CombatAlert_NotBuilding.Read(exINI, pSection, "CombatAlert.NotBuilding");
	this->CombatAlert_UseFeedbackVoice.Read(exINI, pSection, "CombatAlert.UseFeedbackVoice");
	this->CombatAlert_UseAttackVoice.Read(exINI, pSection, "CombatAlert.UseAttackVoice");
	this->CombatAlert_UseEVA.Read(exINI, pSection, "CombatAlert.UseEVA");
	this->CombatAlert_EVA.Read(exINI, pSection, "CombatAlert.EVA");

	this->VoiceCreated.Read(exINI, pSection, "VoiceCreated");
	this->VoicePickup.Read(exINI, pSection, "VoicePickup");

	this->CameoPriority.Read(exINI, pSection, "CameoPriority");

	this->WarpOut.Read(exINI, pSection, "WarpOut");
	this->WarpIn.Read(exINI, pSection, "WarpIn");
	this->WarpAway.Read(exINI, pSection, "WarpAway");
	this->ChronoTrigger.Read(exINI, pSection, "ChronoTrigger");
	this->ChronoDistanceFactor.Read(exINI, pSection, "ChronoDistanceFactor");
	this->ChronoMinimumDelay.Read(exINI, pSection, "ChronoMinimumDelay");
	this->ChronoRangeMinimum.Read(exINI, pSection, "ChronoRangeMinimum");
	this->ChronoDelay.Read(exINI, pSection, "ChronoDelay");
	this->ChronoSpherePreDelay.Read(exINI, pSection, "ChronoSpherePreDelay");
	this->ChronoSphereDelay.Read(exINI, pSection, "ChronoSphereDelay");

	this->WarpInWeapon.Read<true>(exINI, pSection, "WarpInWeapon");
	this->WarpInMinRangeWeapon.Read<true>(exINI, pSection, "WarpInMinRangeWeapon");
	this->WarpOutWeapon.Read<true>(exINI, pSection, "WarpOutWeapon");
	this->WarpInWeapon_UseDistanceAsDamage.Read(exINI, pSection, "WarpInWeapon.UseDistanceAsDamage");

	exINI.ReadSpeed(pSection, "SubterraneanSpeed", &this->SubterraneanSpeed);
	this->SubterraneanHeight.Read(exINI, pSection, "SubterraneanHeight");

	this->OreGathering_Anims.Read(exINI, pSection, "OreGathering.Anims");
	this->OreGathering_Tiberiums.Read(exINI, pSection, "OreGathering.Tiberiums");
	this->OreGathering_FramesPerDir.Read(exINI, pSection, "OreGathering.FramesPerDir");

	this->DestroyAnim_Random.Read(exINI, pSection, "DestroyAnim.Random");
	this->NotHuman_RandomDeathSequence.Read(exINI, pSection, "NotHuman.RandomDeathSequence");

	this->DefaultDisguise.Read(exINI, pSection, "DefaultDisguise");
	this->UseDisguiseMovementSpeed.Read(exINI, pSection, "UseDisguiseMovementSpeed");

	this->OpenTopped_RangeBonus.Read(exINI, pSection, "OpenTopped.RangeBonus");
	this->OpenTopped_DamageMultiplier.Read(exINI, pSection, "OpenTopped.DamageMultiplier");
	this->OpenTopped_WarpDistance.Read(exINI, pSection, "OpenTopped.WarpDistance");
	this->OpenTopped_IgnoreRangefinding.Read(exINI, pSection, "OpenTopped.IgnoreRangefinding");
	this->OpenTopped_AllowFiringIfDeactivated.Read(exINI, pSection, "OpenTopped.AllowFiringIfDeactivated");
	this->OpenTopped_ShareTransportTarget.Read(exINI, pSection, "OpenTopped.ShareTransportTarget");
	this->OpenTopped_UseTransportRangeModifiers.Read(exINI, pSection, "OpenTopped.UseTransportRangeModifiers");
	this->OpenTopped_CheckTransportDisableWeapons.Read(exINI, pSection, "OpenTopped.CheckTransportDisableWeapons");

	this->AutoFire.Read(exINI, pSection, "AutoFire");
	this->AutoFire_TargetSelf.Read(exINI, pSection, "AutoFire.TargetSelf");

	this->NoSecondaryWeaponFallback.Read(exINI, pSection, "NoSecondaryWeaponFallback");
	this->NoSecondaryWeaponFallback_AllowAA.Read(exINI, pSection, "NoSecondaryWeaponFallback.AllowAA");

	this->JumpjetRotateOnCrash.Read(exINI, pSection, "JumpjetRotateOnCrash");
	this->ShadowSizeCharacteristicHeight.Read(exINI, pSection, "ShadowSizeCharacteristicHeight");

	this->DeployingAnim_AllowAnyDirection.Read(exINI, pSection, "DeployingAnim.AllowAnyDirection");
	this->DeployingAnim_KeepUnitVisible.Read(exINI, pSection, "DeployingAnim.KeepUnitVisible");
	this->DeployingAnim_ReverseForUndeploy.Read(exINI, pSection, "DeployingAnim.ReverseForUndeploy");
	this->DeployingAnim_UseUnitDrawer.Read(exINI, pSection, "DeployingAnim.UseUnitDrawer");

	this->EnemyUIName.Read(exINI, pSection, "EnemyUIName");

	this->ForceWeapon_Naval_Decloaked.Read(exINI, pSection, "ForceWeapon.Naval.Decloaked");
	this->ForceWeapon_Cloaked.Read(exINI, pSection, "ForceWeapon.Cloaked");
	this->ForceWeapon_Disguised.Read(exINI, pSection, "ForceWeapon.Disguised");
	this->ForceWeapon_UnderEMP.Read(exINI, pSection, "ForceWeapon.UnderEMP");
	this->ForceWeapon_InRange_TechnoOnly.Read(exINI, pSection, "ForceWeapon.InRange.TechnoOnly");
	this->ForceWeapon_InRange.Read(exINI, pSection, "ForceWeapon.InRange");
	this->ForceWeapon_InRange_Overrides.Read(exINI, pSection, "ForceWeapon.InRange.Overrides");
	this->ForceWeapon_InRange_ApplyRangeModifiers.Read(exINI, pSection, "ForceWeapon.InRange.ApplyRangeModifiers");
	this->ForceAAWeapon_InRange.Read(exINI, pSection, "ForceAAWeapon.InRange");
	this->ForceAAWeapon_InRange_Overrides.Read(exINI, pSection, "ForceAAWeapon.InRange.Overrides");
	this->ForceAAWeapon_InRange_ApplyRangeModifiers.Read(exINI, pSection, "ForceAAWeapon.InRange.ApplyRangeModifiers");
	this->ForceWeapon_Buildings.Read(exINI, pSection, "ForceWeapon.Buildings");
	this->ForceWeapon_Defenses.Read(exINI, pSection, "ForceWeapon.Defenses");
	this->ForceWeapon_Infantry.Read(exINI, pSection, "ForceWeapon.Infantry");
	this->ForceWeapon_Naval_Units.Read(exINI, pSection, "ForceWeapon.Naval.Units");
	this->ForceWeapon_Units.Read(exINI, pSection, "ForceWeapon.Units");
	this->ForceWeapon_Aircraft.Read(exINI, pSection, "ForceWeapon.Aircraft");
	this->ForceAAWeapon_Infantry.Read(exINI, pSection, "ForceAAWeapon.Infantry");
	this->ForceAAWeapon_Units.Read(exINI, pSection, "ForceAAWeapon.Units");
	this->ForceAAWeapon_Aircraft.Read(exINI, pSection, "ForceAAWeapon.Aircraft");

	this->ForceWeapon_Check = (
		this->ForceWeapon_Naval_Decloaked >= 0
		|| this->ForceWeapon_Cloaked >= 0
		|| this->ForceWeapon_Disguised >= 0
		|| this->ForceWeapon_UnderEMP >= 0
		|| !this->ForceWeapon_InRange.empty()
		|| !this->ForceAAWeapon_InRange.empty()
		|| this->ForceWeapon_Buildings >= 0
		|| this->ForceWeapon_Defenses >= 0
		|| this->ForceWeapon_Infantry >= 0
		|| this->ForceWeapon_Naval_Units >= 0
		|| this->ForceWeapon_Units >= 0
		|| this->ForceWeapon_Aircraft >= 0
		|| this->ForceAAWeapon_Infantry >= 0
		|| this->ForceAAWeapon_Units >= 0
		|| this->ForceAAWeapon_Aircraft >= 0
	);

	this->Ammo_Shared.Read(exINI, pSection, "Ammo.Shared");
	this->Ammo_Shared_Group.Read(exINI, pSection, "Ammo.Shared.Group");
	this->SelfHealGainType.Read(exINI, pSection, "SelfHealGainType");
	this->Passengers_SyncOwner.Read(exINI, pSection, "Passengers.SyncOwner");
	this->Passengers_SyncOwner_RevertOnExit.Read(exINI, pSection, "Passengers.SyncOwner.RevertOnExit");

	this->IronCurtain_KeptOnDeploy.Read(exINI, pSection, "IronCurtain.KeptOnDeploy");
	this->IronCurtain_Effect.Read(exINI, pSection, "IronCurtain.Effect");
	this->IronCurtain_KillWarhead.Read<true>(exINI, pSection, "IronCurtain.KillWarhead");
	this->ForceShield_KeptOnDeploy.Read(exINI, pSection, "ForceShield.KeptOnDeploy");
	this->ForceShield_Effect.Read(exINI, pSection, "ForceShield.Effect");
	this->ForceShield_KillWarhead.Read<true>(exINI, pSection, "ForceShield.KillWarhead");

	this->Explodes_KillPassengers.Read(exINI, pSection, "Explodes.KillPassengers");
	this->Explodes_DuringBuildup.Read(exINI, pSection, "Explodes.DuringBuildup");
	this->DeployFireWeapon.Read(exINI, pSection, "DeployFireWeapon");
	this->TargetZoneScanType.Read(exINI, pSection, "TargetZoneScanType");

		// insignia type
	Nullable<InsigniaTypeClass*> InsigniaType;
	InsigniaType.Read(exINI, pSection, "InsigniaType");

	if (InsigniaType.isset())
	{
		this->Insignia = InsigniaType.Get()->Insignia;
		this->InsigniaFrame = InsigniaType.Get()->InsigniaFrame;
		this->InsigniaFrames = Vector3D<int>(-1, -1, -1); // override it so only InsigniaFrame will be used
	}
	else
	{
		this->Insignia.Read(exINI, pSection, "Insignia.%s");
		this->InsigniaFrames.Read(exINI, pSection, "InsigniaFrames");
		this->InsigniaFrame.Read(exINI, pSection, "InsigniaFrame.%s");
	}

	this->Insignia_ShowEnemy.Read(exINI, pSection, "Insignia.ShowEnemy");

	this->JumpjetTilt.Read(exINI, pSection, "JumpjetTilt");
	this->JumpjetTilt_ForwardAccelFactor.Read(exINI, pSection, "JumpjetTilt.ForwardAccelFactor");
	this->JumpjetTilt_ForwardSpeedFactor.Read(exINI, pSection, "JumpjetTilt.ForwardSpeedFactor");
	this->JumpjetTilt_SidewaysRotationFactor.Read(exINI, pSection, "JumpjetTilt.SidewaysRotationFactor");
	this->JumpjetTilt_SidewaysSpeedFactor.Read(exINI, pSection, "JumpjetTilt.SidewaysSpeedFactor");

	this->TiltsWhenCrushes_Vehicles.Read(exINI, pSection, "TiltsWhenCrushes.Vehicles");
	this->TiltsWhenCrushes_Overlays.Read(exINI, pSection, "TiltsWhenCrushes.Overlays");
	this->CrushForwardTiltPerFrame.Read(exINI, pSection, "CrushForwardTiltPerFrame");
	this->CrushOverlayExtraForwardTilt.Read(exINI, pSection, "CrushOverlayExtraForwardTilt");
	this->CrushSlowdownMultiplier.Read(exINI, pSection, "CrushSlowdownMultiplier");
	this->SkipCrushSlowdown.Read(exINI, pSection, "SkipCrushSlowdown");

	this->DigitalDisplay_Disable.Read(exINI, pSection, "DigitalDisplay.Disable");
	this->DigitalDisplayTypes.Read(exINI, pSection, "DigitalDisplayTypes");

	this->SelectBox.Read(exINI, pSection, "SelectBox");
	this->HideSelectBox.Read(exINI, pSection, "HideSelectBox");

	this->AmmoPipFrame.Read(exINI, pSection, "AmmoPipFrame");
	this->EmptyAmmoPipFrame.Read(exINI, pSection, "EmptyAmmoPipFrame");
	this->AmmoPipWrapStartFrame.Read(exINI, pSection, "AmmoPipWrapStartFrame");
	this->AmmoPipSize.Read(exINI, pSection, "AmmoPipSize");
	this->AmmoPipOffset.Read(exINI, pSection, "AmmoPipOffset");

	this->ShowSpawnsPips.Read(exINI, pSection, "ShowSpawnsPips");
	this->SpawnsPipFrame.Read(exINI, pSection, "SpawnsPipFrame");
	this->EmptySpawnsPipFrame.Read(exINI, pSection, "EmptySpawnsPipFrame");
	this->SpawnsPipSize.Read(exINI, pSection, "SpawnsPipSize");
	this->SpawnsPipOffset.Read(exINI, pSection, "SpawnsPipOffset");

	this->SpawnDistanceFromTarget.Read(exINI, pSection, "SpawnDistanceFromTarget");
	this->SpawnHeight.Read(exINI, pSection, "SpawnHeight");
	this->LandingDir.Read(exINI, pSection, "LandingDir");

	this->Convert_HumanToComputer.Read(exINI, pSection, "Convert.HumanToComputer");
	this->Convert_ComputerToHuman.Read(exINI, pSection, "Convert.ComputerToHuman");
	this->Convert_ResetMindControl.Read(exINI, pSection, "Convert.ResetMindControl");

	this->CrateGoodie_RerollChance.Read(exINI, pSection, "CrateGoodie.RerollChance");

	this->Tint_Color.Read(exINI, pSection, "Tint.Color");
	this->Tint_Intensity.Read(exINI, pSection, "Tint.Intensity");
	this->Tint_VisibleToHouses.Read(exINI, pSection, "Tint.VisibleToHouses");

	this->RevengeWeapon.Read<true>(exINI, pSection, "RevengeWeapon");
	this->RevengeWeapon_AffectsHouses.Read(exINI, pSection, "RevengeWeapon.AffectsHouses");

	this->RecountBurst.Read(exINI, pSection, "RecountBurst");

	this->BuildLimitGroup_Types.Read(exINI, pSection, "BuildLimitGroup.Types");
	this->BuildLimitGroup_Nums.Read(exINI, pSection, "BuildLimitGroup.Nums");
	this->BuildLimitGroup_Factor.Read(exINI, pSection, "BuildLimitGroup.Factor");
	this->BuildLimitGroup_ContentIfAnyMatch.Read(exINI, pSection, "BuildLimitGroup.ContentIfAnyMatch");
	this->BuildLimitGroup_NotBuildableIfQueueMatch.Read(exINI, pSection, "BuildLimitGroup.NotBuildableIfQueueMatch");
	this->BuildLimitGroup_ExtraLimit_Types.Read(exINI, pSection, "BuildLimitGroup.ExtraLimit.Types");
	this->BuildLimitGroup_ExtraLimit_Nums.Read(exINI, pSection, "BuildLimitGroup.ExtraLimit.Nums");
	this->BuildLimitGroup_ExtraLimit_MaxCount.Read(exINI, pSection, "BuildLimitGroup.ExtraLimit.MaxCount");
	this->BuildLimitGroup_ExtraLimit_MaxNum.Read(exINI, pSection, "BuildLimitGroup.ExtraLimit.MaxNum");

	this->AmphibiousEnter.Read(exINI, pSection, "AmphibiousEnter");
	this->AmphibiousUnload.Read(exINI, pSection, "AmphibiousUnload");
	this->NoQueueUpToEnter.Read(exINI, pSection, "NoQueueUpToEnter");
	this->NoQueueUpToUnload.Read(exINI, pSection, "NoQueueUpToUnload");

	this->RateDown_Delay.Read(exINI, pSection, "RateDown.Delay");
	this->RateDown_Reset.Read(exINI, pSection, "RateDown.Reset");
	this->RateDown_Cover_Value.Read(exINI, pSection, "RateDown.Cover.Value");
	this->RateDown_Cover_AmmoBelow.Read(exINI, pSection, "RateDown.Cover.AmmoBelow");

	this->NoRearm_UnderEMP.Read(exINI, pSection, "NoRearm.UnderEMP");
	this->NoRearm_Temporal.Read(exINI, pSection, "NoRearm.Temporal");
	this->NoReload_UnderEMP.Read(exINI, pSection, "NoReload.UnderEMP");
	this->NoReload_Temporal.Read(exINI, pSection, "NoReload.Temporal");
	this->NoTurret_TrackTarget.Read(exINI, pSection, "NoTurret.TrackTarget");

	this->Wake.Read(exINI, pSection, "Wake");
	this->Wake_Grapple.Read(exINI, pSection, "Wake.Grapple");
	this->Wake_Sinking.Read(exINI, pSection, "Wake.Sinking");

	this->AINormalTargetingDelay.Read(exINI, pSection, "AINormalTargetingDelay");
	this->PlayerNormalTargetingDelay.Read(exINI, pSection, "PlayerNormalTargetingDelay");
	this->AIGuardAreaTargetingDelay.Read(exINI, pSection, "AIGuardAreaTargetingDelay");
	this->PlayerGuardAreaTargetingDelay.Read(exINI, pSection, "PlayerGuardAreaTargetingDelay");
	this->AIAttackMoveTargetingDelay.Read(exINI, pSection, "AIAttackMoveTargetingDelay");
	this->PlayerAttackMoveTargetingDelay.Read(exINI, pSection, "PlayerAttackMoveTargetingDelay");
	this->DistributeTargetingFrame.Read(exINI, pSection, "DistributeTargetingFrame");

	this->BunkerableAnyway.Read(exINI, pSection, "BunkerableAnyway");

	this->DigitalDisplay_Health_FakeAtDisguise.Read(exINI, pSection, "DigitalDisplay.Health.FakeAtDisguise");

	this->AttackMove_Aggressive.Read(exINI, pSection, "AttackMove.Aggressive");
	this->AttackMove_UpdateTarget.Read(exINI, pSection, "AttackMove.UpdateTarget");

	this->KeepTargetOnMove.Read(exINI, pSection, "KeepTargetOnMove");
	this->KeepTargetOnMove_NoMorePursuit.Read(exINI, pSection, "KeepTargetOnMove.NoMorePursuit");
	this->KeepTargetOnMove_ExtraDistance.Read(exINI, pSection, "KeepTargetOnMove.ExtraDistance");

	this->Power.Read(exINI, pSection, "Power");

	this->AllowAirstrike.Read(exINI, pSection, "AllowAirstrike");

	this->Image_ConditionYellow.Read(exINI, pSection, "Image.ConditionYellow");
	this->Image_ConditionRed.Read(exINI, pSection, "Image.ConditionRed");
	this->WaterImage_ConditionYellow.Read(exINI, pSection, "WaterImage.ConditionYellow");
	this->WaterImage_ConditionRed.Read(exINI, pSection, "WaterImage.ConditionRed");

	this->InitialSpawnsNumber.Read(exINI, pSection, "InitialSpawnsNumber");
	this->Spawns_Queue.Read(exINI, pSection, "Spawns.Queue");

	this->Spawner_RecycleRange.Read(exINI, pSection, "Spawner.RecycleRange");
	this->Spawner_RecycleAnim.Read(exINI, pSection, "Spawner.RecycleAnim");
	this->Spawner_RecycleCoord.Read(exINI, pSection, "Spawner.RecycleCoord");
	this->Spawner_RecycleOnTurret.Read(exINI, pSection, "Spawner.RecycleOnTurret");

	this->Sinkable.Read(exINI, pSection, "Sinkable");
	this->Sinkable_SquidGrab.Read(exINI, pSection, "Sinkable.SquidGrab");
	this->SinkSpeed.Read(exINI, pSection, "SinkSpeed");

	this->DamagedSpeed.Read(exINI, pSection, "DamagedSpeed");
	this->ProneSpeed.Read(exINI, pSection, "ProneSpeed");

	this->SuppressKillWeapons.Read(exINI, pSection, "SuppressKillWeapons");
	this->SuppressKillWeapons_Types.Read(exINI, pSection, "SuppressKillWeapons.Types");

	this->Promote_VeteranAnimation.Read(exINI, pSection, "Promote.VeteranAnimation");
	this->Promote_EliteAnimation.Read(exINI, pSection, "Promote.EliteAnimation");

	this->RadarInvisibleToHouse.Read(exINI, pSection, "RadarInvisibleToHouse");

	this->Overload_Count.Read(exINI, pSection, "Overload.Count");
	this->Overload_Damage.Read(exINI, pSection, "Overload.Damage");
	this->Overload_Frames.Read(exINI, pSection, "Overload.Frames");
	this->Overload_DeathSound.Read(exINI, pSection, "Overload.DeathSound");
	this->Overload_ParticleSys.Read(exINI, pSection, "Overload.ParticleSys");
	this->Overload_ParticleSysCount.Read(exINI, pSection, "Overload.ParticleSysCount");

	this->Harvester_CanGuardArea.Read(exINI, pSection, "Harvester.CanGuardArea");
	this->HarvesterScanAfterUnload.Read(exINI, pSection, "HarvesterScanAfterUnload");

	this->ExtendedAircraftMissions_SmoothMoving.Read(exINI, pSection, "ExtendedAircraftMissions.SmoothMoving");
	this->ExtendedAircraftMissions_EarlyDescend.Read(exINI, pSection, "ExtendedAircraftMissions.EarlyDescend");
	this->ExtendedAircraftMissions_RearApproach.Read(exINI, pSection, "ExtendedAircraftMissions.RearApproach");

	this->FallingDownDamage.Read(exINI, pSection, "FallingDownDamage");
	this->FallingDownDamage_Water.Read(exINI, pSection, "FallingDownDamage.Water");

	this->FiringForceScatter.Read(exINI, pSection, "FiringForceScatter");

	this->EngineerRepairAmount.Read(exINI, pSection, "EngineerRepairAmount");

	this->DebrisTypes_Limit.Read(exINI, pSection, "DebrisTypes.Limit");
	this->DebrisMinimums.Read(exINI, pSection, "DebrisMinimums");

	this->AttackMove_Follow.Read(exINI, pSection, "AttackMove.Follow");
	this->AttackMove_Follow_IncludeAir.Read(exINI, pSection, "AttackMove.Follow.IncludeAir");
	this->AttackMove_Follow_IfMindControlIsFull.Read(exINI, pSection, "AttackMove.Follow.IfMindControlIsFull");
	this->AttackMove_StopWhenTargetAcquired.Read(exINI, pSection, "AttackMove.StopWhenTargetAcquired");
	this->AttackMove_PursuitTarget.Read(exINI, pSection, "AttackMove.PursuitTarget");

	this->BattlePoints.Read(exINI, pSection, "BattlePoints");

	this->InfantryAutoDeploy.Read(exINI, pSection, "InfantryAutoDeploy");

	// Ares 0.2
	this->RadarJamRadius.Read(exINI, pSection, "RadarJamRadius");

	// Ares 0.9
	this->InhibitorRange.Read(exINI, pSection, "InhibitorRange");
	this->DesignatorRange.Read(exINI, pSection, "DesignatorRange");

	// Ares 0.A
	this->GroupAs.Read(pINI, pSection, "GroupAs");

	// Ares 0.C
	this->NoAmmoWeapon.Read(exINI, pSection, "NoAmmoWeapon");
	this->NoAmmoAmount.Read(exINI, pSection, "NoAmmoAmount");

	// ExtraFire weapons
	this->ExtraFire_Primary.Read(exINI, pSection, "ExtraFire.Primary");
	this->ExtraFire_Secondary.Read(exINI, pSection, "ExtraFire.Secondary");
	this->ExtraFire_ElitePrimary.Read(exINI, pSection, "ExtraFire.ElitePrimary");
	this->ExtraFire_EliteSecondary.Read(exINI, pSection, "ExtraFire.EliteSecondary");

	// Ares 2.0
	this->Passengers_BySize.Read(exINI, pSection, "Passengers.BySize");

	char tempBuffer[40];

	if (pThis->Gunner)
	{
		size_t weaponCount = pThis->WeaponCount;

		if (this->Insignia_Weapon.empty() || this->Insignia_Weapon.size() != weaponCount)
		{
			this->Insignia_Weapon.resize(weaponCount);
			this->InsigniaFrame_Weapon.resize(weaponCount, Promotable<int>(-1));
			Valueable<Vector3D<int>> frames;
			frames = Vector3D<int>(-1, -1, -1);
			this->InsigniaFrames_Weapon.resize(weaponCount, frames);
		}

		for (size_t i = 0; i < weaponCount; i++)
		{
			Nullable<InsigniaTypeClass*> InsigniaType_Weapon;
			_snprintf_s(tempBuffer, sizeof(tempBuffer), "InsigniaType.Weapon%d", i + 1);
			InsigniaType_Weapon.Read(exINI, pSection, tempBuffer);

			if (InsigniaType_Weapon.isset())
			{
				this->Insignia_Weapon[i] = InsigniaType_Weapon.Get()->Insignia;
				this->InsigniaFrame_Weapon[i] = InsigniaType_Weapon.Get()->InsigniaFrame;
				this->InsigniaFrames_Weapon[i] = Vector3D<int>(-1, -1, -1); // override it so only InsigniaFrame will be used
			}
			else
			{
				_snprintf_s(tempBuffer, sizeof(tempBuffer), "Insignia.Weapon%d.%s", i + 1, "%s");
				this->Insignia_Weapon[i].Read(exINI, pSection, tempBuffer);

				_snprintf_s(tempBuffer, sizeof(tempBuffer), "InsigniaFrame.Weapon%d.%s", i + 1, "%s");
				this->InsigniaFrame_Weapon[i].Read(exINI, pSection, tempBuffer);

				_snprintf_s(tempBuffer, sizeof(tempBuffer), "InsigniaFrames.Weapon%d", i + 1);
				this->InsigniaFrames_Weapon[i].Read(exINI, pSection, tempBuffer);
			}
		}
	}

	if (pThis->Passengers > 0)
	{
		size_t passengers = pThis->Passengers + 1;

		if (this->Insignia_Passengers.empty() || this->Insignia_Passengers.size() != passengers)
		{
			this->Insignia_Passengers.resize(passengers);
			this->InsigniaFrame_Passengers.resize(passengers, Promotable<int>(-1));
			Valueable<Vector3D<int>> frames;
			frames = Vector3D<int>(-1, -1, -1);
			this->InsigniaFrames_Passengers.resize(passengers, frames);
		}

		for (size_t i = 0; i < passengers; i++)
		{
			Nullable<InsigniaTypeClass*> InsigniaType_Passengers;
			_snprintf_s(tempBuffer, sizeof(tempBuffer), "InsigniaType.Passengers%d", i);
			InsigniaType_Passengers.Read(exINI, pSection, tempBuffer);

			if (InsigniaType_Passengers.isset())
			{
				this->Insignia_Passengers[i] = InsigniaType_Passengers.Get()->Insignia;
				this->InsigniaFrame_Passengers[i] = InsigniaType_Passengers.Get()->InsigniaFrame;
				this->InsigniaFrames_Passengers[i] = Vector3D<int>(-1, -1, -1); // override it so only InsigniaFrame will be used
			}
			else
			{
				_snprintf_s(tempBuffer, sizeof(tempBuffer), "Insignia.Passengers%d.%s", i, "%s");
				this->Insignia_Passengers[i].Read(exINI, pSection, tempBuffer);

				_snprintf_s(tempBuffer, sizeof(tempBuffer), "InsigniaFrame.Passengers%d.%s", i, "%s");
				this->InsigniaFrame_Passengers[i].Read(exINI, pSection, tempBuffer);

				_snprintf_s(tempBuffer, sizeof(tempBuffer), "InsigniaFrames.Passengers%d", i);
				this->InsigniaFrames_Passengers[i].Read(exINI, pSection, tempBuffer);
			}
		}
	}

	// Airstrike tint color
	this->TintColorAirstrike = GeneralUtils::GetColorFromColorAdd(this->LaserTargetColor.Get(RulesClass::Instance->LaserTargetColor));

	// Art tags
	INI_EX exArtINI(CCINIClass::INI_Art);
	auto pArtSection = pThis->ImageFile;

	this->TurretOffset.Read(exArtINI, pArtSection, "TurretOffset");
	this->TurretShadow.Read(exArtINI, pArtSection, "TurretShadow");
	ValueableVector<int> shadow_indices;
	shadow_indices.Read(exArtINI, pArtSection, "ShadowIndices");
	ValueableVector<int> shadow_indices_frame;
	shadow_indices_frame.Read(exArtINI, pArtSection, "ShadowIndices.Frame");
	if (shadow_indices_frame.size() != shadow_indices.size())
	{
		if (!shadow_indices_frame.empty())
			Debug::LogGame("[Developer warning] %s ShadowIndices.Frame size (%d) does not match ShadowIndices size (%d) \n"
				, pSection, shadow_indices_frame.size(), shadow_indices.size());
		shadow_indices_frame.resize(shadow_indices.size(), -1);
	}
	for (size_t i = 0; i < shadow_indices.size(); i++)
		this->ShadowIndices[shadow_indices[i]] = shadow_indices_frame[i];

	this->ShadowIndex_Frame.Read(exArtINI, pArtSection, "ShadowIndex.Frame");

	this->LaserTrailData.clear();
	for (size_t i = 0; ; ++i)
	{
		NullableIdx<LaserTrailTypeClass> trail;
		_snprintf_s(tempBuffer, sizeof(tempBuffer), "LaserTrail%d.Type", i);
		trail.Read(exArtINI, pArtSection, tempBuffer);

		if (!trail.isset())
			break;

		Valueable<CoordStruct> flh;
		_snprintf_s(tempBuffer, sizeof(tempBuffer), "LaserTrail%d.FLH", i);
		flh.Read(exArtINI, pArtSection, tempBuffer);

		Valueable<bool> isOnTurret;
		_snprintf_s(tempBuffer, sizeof(tempBuffer), "LaserTrail%d.IsOnTurret", i);
		isOnTurret.Read(exArtINI, pArtSection, tempBuffer);

		this->LaserTrailData.push_back({ ValueableIdx<LaserTrailTypeClass>(trail), flh, isOnTurret });
	}

	this->ParseBurstFLHs(exArtINI, pArtSection, this->WeaponBurstFLHs, this->EliteWeaponBurstFLHs, "");
	this->ParseBurstFLHs(exArtINI, pArtSection, this->DeployedWeaponBurstFLHs, this->EliteDeployedWeaponBurstFLHs, "Deployed");
	this->ParseBurstFLHs(exArtINI, pArtSection, this->CrouchedWeaponBurstFLHs, this->EliteCrouchedWeaponBurstFLHs, "Prone");

	this->OnlyUseLandSequences.Read(exArtINI, pArtSection, "OnlyUseLandSequences");

	this->PronePrimaryFireFLH.Read(exArtINI, pArtSection, "PronePrimaryFireFLH");
	this->ProneSecondaryFireFLH.Read(exArtINI, pArtSection, "ProneSecondaryFireFLH");
	this->DeployedPrimaryFireFLH.Read(exArtINI, pArtSection, "DeployedPrimaryFireFLH");
	this->DeployedSecondaryFireFLH.Read(exArtINI, pArtSection, "DeployedSecondaryFireFLH");
	this->AlternateFLH_OnTurret.Read(exArtINI, pArtSection, "AlternateFLH.OnTurret");
	
	// ExtraFire FLH coordinates
	this->ExtraFire_PrimaryFLH.Read(exArtINI, pArtSection, "ExtraFire.PrimaryFLH");
	this->ExtraFire_SecondaryFLH.Read(exArtINI, pArtSection, "ExtraFire.SecondaryFLH");
	this->ExtraFire_ElitePrimaryFLH.Read(exArtINI, pArtSection, "ExtraFire.ElitePrimaryFLH");
	this->ExtraFire_EliteSecondaryFLH.Read(exArtINI, pArtSection, "ExtraFire.EliteSecondaryFLH");

	for (size_t i = 0; ; i++)
	{
		Nullable<CoordStruct> alternateFLH;
		_snprintf_s(tempBuffer, sizeof(tempBuffer), "AlternateFLH%u", i);
		alternateFLH.Read(exArtINI, pArtSection, tempBuffer);

		// ww always read all of AlternateFLH0-5
		if (i >= 5U && !alternateFLH.isset())
			break;
		else if (!alternateFLH.isset())
			alternateFLH = pThis->Weapon[0].FLH; // Game defaults to this for AlternateFLH, not 0,0,0

		if (this->AlternateFLHs.size() < i)
			this->AlternateFLHs[i] = alternateFLH;
		else
			this->AlternateFLHs.emplace_back(alternateFLH);
	}

	// Parasitic types
	this->AttachEffects.LoadFromINI(pINI, pSection);

	auto [canParse, resetValue] = PassengerDeletionTypeClass::CanParse(exINI, pSection);

	if (canParse && !this->PassengerDeletionType)
		this->PassengerDeletionType = std::make_unique<PassengerDeletionTypeClass>(pThis);

	if (this->PassengerDeletionType)
	{
		if (resetValue)
			this->PassengerDeletionType.reset();
		else
			this->PassengerDeletionType->LoadFromINI(pINI, pSection);
	}

	Nullable<int> transDelay;
	transDelay.Read(exINI, pSection, "TiberiumEater.TransDelay");

	if (transDelay.Get(-1) >= 0 && !this->TiberiumEaterType)
		this->TiberiumEaterType = std::make_unique<TiberiumEaterTypeClass>();

	if (this->TiberiumEaterType)
	{
		if (transDelay.isset() && transDelay.Get() < 0)
			this->TiberiumEaterType.reset();
		else
			this->TiberiumEaterType->LoadFromINI(pINI, pSection);
	}

	Nullable<bool> isInterceptor;
	isInterceptor.Read(exINI, pSection, "Interceptor");

	if (isInterceptor)
	{
		if (this->InterceptorType == nullptr)
			this->InterceptorType = std::make_unique<InterceptorTypeClass>(pThis);

		this->InterceptorType->LoadFromINI(pINI, pSection);
	}
	else if (isInterceptor.isset())
	{
		this->InterceptorType.reset();
	}

	if (pThis->WhatAmI() != AbstractType::BuildingType)
	{
		if (this->DroppodType == nullptr)
			this->DroppodType = std::make_unique<DroppodTypeClass>();

		this->DroppodType->LoadFromINI(pINI, pSection);
	}
	else
	{
		this->DroppodType.reset();
	}

	if (GeneralUtils::IsValidString(pThis->PaletteFile) && !pThis->Palette)
		Debug::Log("[Developer warning] [%s] has Palette=%s set but no palette file was loaded (missing file or wrong filename). Missing palettes cause issues with lighting recalculations.\n", pArtSection, pThis->PaletteFile);

	this->LoadFromINIByWhatAmI(exArtINI, pArtSection);

	// VoiceIFVRepair from Ares 0.2
	this->VoiceIFVRepair.Read(exINI, pSection, "VoiceIFVRepair");
	this->ParseVoiceWeaponAttacks(exINI, pSection, this->VoiceWeaponAttacks, this->VoiceEliteWeaponAttacks);
}

void TechnoTypeExt::ExtData::LoadFromINIByWhatAmI(INI_EX& exArtINI, const char* pArtSection)
{
	AbstractType abs = this->OwnerObject()->WhatAmI();

	switch (abs)
	{
	case AbstractType::UnitType:
	{
		this->FireUp.Read(exArtINI, pArtSection, "FireUp");
		this->FireUp_ResetInRetarget.Read(exArtINI, pArtSection, "FireUp.ResetInRetarget");
		//this->SecondaryFire.Read(exArtINI, pArtSection, "SecondaryFire");
		break;
	}
	default:
		break;
	}
}

void TechnoTypeExt::ExtData::FireExtraWeapons(TechnoClass* pThis, AbstractClass* pTarget, int weaponIndex) const
{
	if (!pThis || !pTarget || ExtraFireInProgress)
		return;


	// Set guard to prevent recursion
	ExtraFireInProgress = true;

	const bool isElite = pThis->Veterancy.IsElite();
	std::vector<WeaponTypeClass*> extraWeapons;

	// Determine which ExtraFire weapons to use based on weapon index and elite status
	if (weaponIndex == 0) // Primary weapon
	{
		if (isElite && !this->ExtraFire_ElitePrimary.empty())
			extraWeapons = this->ExtraFire_ElitePrimary;
		else if (!this->ExtraFire_Primary.empty())
			extraWeapons = this->ExtraFire_Primary;
	}
	else if (weaponIndex == 1) // Secondary weapon  
	{
		if (isElite && !this->ExtraFire_EliteSecondary.empty())
			extraWeapons = this->ExtraFire_EliteSecondary;
		else if (!this->ExtraFire_Secondary.empty())
			extraWeapons = this->ExtraFire_Secondary;
	}

	// Fire each ExtraFire weapon
	auto pTechnoExt = TechnoExt::ExtMap.Find(pThis);
	for (auto pWeapon : extraWeapons)
	{
		if (pWeapon)
		{
			// Check ROF timer for this specific ExtraFire weapon
			auto& timer = pTechnoExt->ExtraFireTimers[pWeapon];
			if (!timer.Expired())
				continue; // Skip this weapon if ROF hasn't elapsed
			
			// Start ROF timer for this weapon
			timer.Start(pWeapon->ROF);
			
			// Temporarily store the original weapon and FLH
			auto originalWeapon = pThis->GetWeapon(weaponIndex);
			WeaponTypeClass* originalWeaponType = originalWeapon ? originalWeapon->WeaponType : nullptr;
			CoordStruct originalFLH = originalWeapon ? originalWeapon->FLH : CoordStruct{0,0,0};
			
			// Get the appropriate ExtraFire FLH
			CoordStruct extraFireFLH = {0,0,0};
			if (weaponIndex == 0) // Primary weapon
			{
				if (isElite && this->ExtraFire_ElitePrimaryFLH.isset())
					extraFireFLH = this->ExtraFire_ElitePrimaryFLH.Get();
				else
					extraFireFLH = this->ExtraFire_PrimaryFLH.Get();
			}
			else if (weaponIndex == 1) // Secondary weapon
			{
				if (isElite && this->ExtraFire_EliteSecondaryFLH.isset())
					extraFireFLH = this->ExtraFire_EliteSecondaryFLH.Get();
				else
					extraFireFLH = this->ExtraFire_SecondaryFLH.Get();
			}
			
			// Temporarily replace the weapon and FLH with the ExtraFire weapon
			if (originalWeapon)
			{
				originalWeapon->WeaponType = pWeapon;
				originalWeapon->FLH = extraFireFLH;
				
				// Use the game's built-in firing mechanism
				pThis->Fire(pTarget, weaponIndex);
				
				// Restore the original weapon and FLH
				originalWeapon->WeaponType = originalWeaponType;
				originalWeapon->FLH = originalFLH;
			}
		}
	}
	
	// Clear guard
	ExtraFireInProgress = false;
}

template <typename T>
void TechnoTypeExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->HealthBar_Hide)
		.Process(this->UIDescription)
		.Process(this->LowSelectionPriority)
		.Process(this->MindControlRangeLimit)
		.Process(this->MindControlLink_VisibleToHouse)
		.Process(this->FactoryPlant_Multiplier)
		.Process(this->CustomArmorName)
		.Process(this->InterceptorType)

		.Process(this->GroupAs)
		.Process(this->RadarJamRadius)
		.Process(this->InhibitorRange)
		.Process(this->DesignatorRange)
		.Process(this->TurretOffset)
		.Process(this->TurretShadow)
		.Process(this->ShadowIndices)
		.Process(this->ShadowIndex_Frame)
		.Process(this->Spawner_LimitRange)
		//.Process(this->Spawner_ExtraLimitRange)
		.Process(this->SpawnerRange)
		.Process(this->EliteSpawnerRange)
		.Process(this->Spawner_DelayFrames)
		.Process(this->Spawner_AttackImmediately)
		.Process(this->Spawner_UseTurretFacing)
		.Process(this->Harvester_Counted)
		.Process(this->Promote_IncludeSpawns)
		.Process(this->ImmuneToCrit)
		.Process(this->MultiMindControl_ReleaseVictim)
		.Process(this->CameoPriority)
		.Process(this->NoManualMove)
		.Process(this->AllowFire_IroncurtainedTarget)
		.Process(this->IsDummy)
		.Process(this->InitialStrength)
		.Process(this->ReloadInTransport)
		.Process(this->ForbidParallelAIQueues)
		.Process(this->TintColorAirstrike)
		.Process(this->LaserTargetColor)
		.Process(this->AirstrikeLineColor)
		.Process(this->ShieldType)
		.Process(this->PassengerDeletionType)

		.Process(this->HarvesterDumpAmount)

		.Process(this->Ammo_AddOnDeploy)
		.Process(this->Ammo_AutoDeployMinimumAmount)
		.Process(this->Ammo_AutoDeployMaximumAmount)
		.Process(this->Ammo_DeployUnlockMinimumAmount)
		.Process(this->Ammo_DeployUnlockMaximumAmount)

		.Process(this->AutoDeath_Behavior)
		.Process(this->AutoDeath_VanishAnimation)
		.Process(this->AutoDeath_OnAmmoDepletion)
		.Process(this->AutoDeath_AfterDelay)
		.Process(this->AutoDeath_TechnosDontExist)
		.Process(this->AutoDeath_TechnosDontExist_Any)
		.Process(this->AutoDeath_TechnosDontExist_AllowLimboed)
		.Process(this->AutoDeath_TechnosDontExist_Houses)
		.Process(this->AutoDeath_TechnosExist)
		.Process(this->AutoDeath_TechnosExist_Any)
		.Process(this->AutoDeath_TechnosExist_AllowLimboed)
		.Process(this->AutoDeath_TechnosExist_Houses)

		.Process(this->Slaved_OwnerWhenMasterKilled)
		.Process(this->SlavesFreeSound)
		.Process(this->SellSound)
		.Process(this->EVA_Sold)

		.Process(this->CombatAlert)
		.Process(this->CombatAlert_NotBuilding)
		.Process(this->CombatAlert_UseFeedbackVoice)
		.Process(this->CombatAlert_UseAttackVoice)
		.Process(this->CombatAlert_UseEVA)
		.Process(this->CombatAlert_EVA)

		.Process(this->VoiceCreated)
		.Process(this->VoicePickup)

		.Process(this->WarpOut)
		.Process(this->WarpIn)
		.Process(this->WarpAway)
		.Process(this->ChronoTrigger)
		.Process(this->ChronoDistanceFactor)
		.Process(this->ChronoMinimumDelay)
		.Process(this->ChronoRangeMinimum)
		.Process(this->ChronoDelay)
		.Process(this->ChronoSpherePreDelay)
		.Process(this->ChronoSphereDelay)
		.Process(this->WarpInWeapon)
		.Process(this->WarpInMinRangeWeapon)
		.Process(this->WarpOutWeapon)
		.Process(this->WarpInWeapon_UseDistanceAsDamage)

		.Process(this->SubterraneanSpeed)
		.Process(this->SubterraneanHeight)

		.Process(this->OreGathering_Anims)
		.Process(this->OreGathering_Tiberiums)
		.Process(this->OreGathering_FramesPerDir)
		.Process(this->LaserTrailData)
		.Process(this->DestroyAnim_Random)
		.Process(this->NotHuman_RandomDeathSequence)
		.Process(this->DefaultDisguise)
		.Process(this->UseDisguiseMovementSpeed)
		.Process(this->WeaponBurstFLHs)
		.Process(this->EliteWeaponBurstFLHs)
		.Process(this->AlternateFLHs)
		.Process(this->AlternateFLH_OnTurret)

		.Process(this->OpenTopped_RangeBonus)
		.Process(this->OpenTopped_DamageMultiplier)
		.Process(this->OpenTopped_WarpDistance)
		.Process(this->OpenTopped_IgnoreRangefinding)
		.Process(this->OpenTopped_AllowFiringIfDeactivated)
		.Process(this->OpenTopped_ShareTransportTarget)
		.Process(this->OpenTopped_UseTransportRangeModifiers)
		.Process(this->OpenTopped_CheckTransportDisableWeapons)

		.Process(this->AutoFire)
		.Process(this->AutoFire_TargetSelf)
		.Process(this->NoSecondaryWeaponFallback)
		.Process(this->NoSecondaryWeaponFallback_AllowAA)
		.Process(this->NoAmmoWeapon)
		.Process(this->NoAmmoAmount)
		
		// ExtraFire serialization
		.Process(this->ExtraFire_Primary)
		.Process(this->ExtraFire_Secondary)
		.Process(this->ExtraFire_ElitePrimary)
		.Process(this->ExtraFire_EliteSecondary)
		.Process(this->ExtraFire_PrimaryFLH)
		.Process(this->ExtraFire_SecondaryFLH)
		.Process(this->ExtraFire_ElitePrimaryFLH)
		.Process(this->ExtraFire_EliteSecondaryFLH)
		
		.Process(this->JumpjetRotateOnCrash)
		.Process(this->ShadowSizeCharacteristicHeight)
		.Process(this->DeployingAnim_AllowAnyDirection)
		.Process(this->DeployingAnim_KeepUnitVisible)
		.Process(this->DeployingAnim_ReverseForUndeploy)
		.Process(this->DeployingAnim_UseUnitDrawer)

		.Process(this->EnemyUIName)

		.Process(this->ForceWeapon_Check)
		.Process(this->ForceWeapon_Naval_Decloaked)
		.Process(this->ForceWeapon_Cloaked)
		.Process(this->ForceWeapon_Disguised)
		.Process(this->ForceWeapon_UnderEMP)
		.Process(this->ForceWeapon_InRange_TechnoOnly)
		.Process(this->ForceWeapon_InRange)
		.Process(this->ForceWeapon_InRange_Overrides)
		.Process(this->ForceWeapon_InRange_ApplyRangeModifiers)
		.Process(this->ForceAAWeapon_InRange)
		.Process(this->ForceAAWeapon_InRange_Overrides)
		.Process(this->ForceAAWeapon_InRange_ApplyRangeModifiers)
		.Process(this->ForceWeapon_Buildings)
		.Process(this->ForceWeapon_Defenses)
		.Process(this->ForceWeapon_Infantry)
		.Process(this->ForceWeapon_Naval_Units)
		.Process(this->ForceWeapon_Units)
		.Process(this->ForceWeapon_Aircraft)
		.Process(this->ForceAAWeapon_Infantry)
		.Process(this->ForceAAWeapon_Units)
		.Process(this->ForceAAWeapon_Aircraft)

		.Process(this->Ammo_Shared)
		.Process(this->Ammo_Shared_Group)
		.Process(this->SelfHealGainType)
		.Process(this->Passengers_SyncOwner)
		.Process(this->Passengers_SyncOwner_RevertOnExit)

		.Process(this->OnlyUseLandSequences)

		.Process(this->PronePrimaryFireFLH)
		.Process(this->ProneSecondaryFireFLH)
		.Process(this->DeployedPrimaryFireFLH)
		.Process(this->DeployedSecondaryFireFLH)
		.Process(this->CrouchedWeaponBurstFLHs)
		.Process(this->EliteCrouchedWeaponBurstFLHs)
		.Process(this->DeployedWeaponBurstFLHs)
		.Process(this->EliteDeployedWeaponBurstFLHs)

		.Process(this->IronCurtain_KeptOnDeploy)
		.Process(this->IronCurtain_Effect)
		.Process(this->IronCurtain_KillWarhead)
		.Process(this->ForceShield_KeptOnDeploy)
		.Process(this->ForceShield_Effect)
		.Process(this->ForceShield_KillWarhead)

		.Process(this->Explodes_KillPassengers)
		.Process(this->Explodes_DuringBuildup)
		.Process(this->DeployFireWeapon)
		.Process(this->TargetZoneScanType)

		.Process(this->Insignia)
		.Process(this->InsigniaFrames)
		.Process(this->InsigniaFrame)
		.Process(this->Insignia_ShowEnemy)
		.Process(this->Insignia_Weapon)
		.Process(this->InsigniaFrame_Weapon)
		.Process(this->InsigniaFrames_Weapon)
		.Process(this->Insignia_Passengers)
		.Process(this->InsigniaFrame_Passengers)
		.Process(this->InsigniaFrames_Passengers)

		.Process(this->JumpjetTilt)
		.Process(this->JumpjetTilt_ForwardAccelFactor)
		.Process(this->JumpjetTilt_ForwardSpeedFactor)
		.Process(this->JumpjetTilt_SidewaysRotationFactor)
		.Process(this->JumpjetTilt_SidewaysSpeedFactor)

		.Process(this->TiltsWhenCrushes_Vehicles)
		.Process(this->TiltsWhenCrushes_Overlays)
		.Process(this->CrushForwardTiltPerFrame)
		.Process(this->CrushOverlayExtraForwardTilt)
		.Process(this->CrushSlowdownMultiplier)
		.Process(this->SkipCrushSlowdown)

		.Process(this->DigitalDisplay_Disable)
		.Process(this->DigitalDisplayTypes)

		.Process(this->SelectBox)
		.Process(this->HideSelectBox)

		.Process(this->AmmoPipFrame)
		.Process(this->EmptyAmmoPipFrame)
		.Process(this->AmmoPipWrapStartFrame)
		.Process(this->AmmoPipSize)
		.Process(this->AmmoPipOffset)

		.Process(this->ShowSpawnsPips)
		.Process(this->SpawnsPipFrame)
		.Process(this->EmptySpawnsPipFrame)
		.Process(this->SpawnsPipSize)
		.Process(this->SpawnsPipOffset)

		.Process(this->SpawnDistanceFromTarget)
		.Process(this->SpawnHeight)
		.Process(this->LandingDir)
		.Process(this->DroppodType)

		.Process(this->TiberiumEaterType)

		.Process(this->Convert_HumanToComputer)
		.Process(this->Convert_ComputerToHuman)
		.Process(this->Convert_ResetMindControl)

		.Process(this->CrateGoodie_RerollChance)

		.Process(this->Tint_Color)
		.Process(this->Tint_Intensity)
		.Process(this->Tint_VisibleToHouses)

		.Process(this->RevengeWeapon)
		.Process(this->RevengeWeapon_AffectsHouses)

		.Process(this->AttachEffects)

		.Process(this->RecountBurst)

		.Process(this->BuildLimitGroup_Types)
		.Process(this->BuildLimitGroup_Nums)
		.Process(this->BuildLimitGroup_Factor)
		.Process(this->BuildLimitGroup_ContentIfAnyMatch)
		.Process(this->BuildLimitGroup_NotBuildableIfQueueMatch)
		.Process(this->BuildLimitGroup_ExtraLimit_Types)
		.Process(this->BuildLimitGroup_ExtraLimit_Nums)
		.Process(this->BuildLimitGroup_ExtraLimit_MaxCount)
		.Process(this->BuildLimitGroup_ExtraLimit_MaxNum)

		.Process(this->AmphibiousEnter)
		.Process(this->AmphibiousUnload)
		.Process(this->NoQueueUpToEnter)
		.Process(this->NoQueueUpToUnload)
		.Process(this->Passengers_BySize)

		.Process(this->RateDown_Delay)
		.Process(this->RateDown_Reset)
		.Process(this->RateDown_Cover_Value)
		.Process(this->RateDown_Cover_AmmoBelow)

		.Process(this->NoRearm_UnderEMP)
		.Process(this->NoRearm_Temporal)
		.Process(this->NoReload_UnderEMP)
		.Process(this->NoReload_Temporal)
		.Process(this->NoTurret_TrackTarget)

		.Process(this->Wake)
		.Process(this->Wake_Grapple)
		.Process(this->Wake_Sinking)

		.Process(this->AINormalTargetingDelay)
		.Process(this->PlayerNormalTargetingDelay)
		.Process(this->AIGuardAreaTargetingDelay)
		.Process(this->PlayerGuardAreaTargetingDelay)
		.Process(this->AIAttackMoveTargetingDelay)
		.Process(this->PlayerAttackMoveTargetingDelay)
		.Process(this->DistributeTargetingFrame)

		.Process(this->DigitalDisplay_Health_FakeAtDisguise)

		.Process(this->AttackMove_Aggressive)
		.Process(this->AttackMove_UpdateTarget)

		.Process(this->BunkerableAnyway)
		.Process(this->KeepTargetOnMove)
		.Process(this->KeepTargetOnMove_NoMorePursuit)
		.Process(this->KeepTargetOnMove_ExtraDistance)

		.Process(this->Power)

		.Process(this->AllowAirstrike)

		.Process(this->Image_ConditionYellow)
		.Process(this->Image_ConditionRed)
		.Process(this->WaterImage_ConditionYellow)
		.Process(this->WaterImage_ConditionRed)

		.Process(this->InitialSpawnsNumber)
		.Process(this->Spawns_Queue)

		.Process(this->Spawner_RecycleRange)
		.Process(this->Spawner_RecycleAnim)
		.Process(this->Spawner_RecycleCoord)
		.Process(this->Spawner_RecycleOnTurret)

		.Process(this->Sinkable)
		.Process(this->Sinkable_SquidGrab)
		.Process(this->SinkSpeed)

		.Process(this->DamagedSpeed)
		.Process(this->ProneSpeed)

		.Process(this->SuppressKillWeapons)
		.Process(this->SuppressKillWeapons_Types)

		.Process(this->Promote_VeteranAnimation)
		.Process(this->Promote_EliteAnimation)

		.Process(this->RadarInvisibleToHouse)

		.Process(this->Overload_Count)
		.Process(this->Overload_Damage)
		.Process(this->Overload_Frames)
		.Process(this->Overload_DeathSound)
		.Process(this->Overload_ParticleSys)
		.Process(this->Overload_ParticleSysCount)

		.Process(this->Harvester_CanGuardArea)
		.Process(this->HarvesterScanAfterUnload)

		.Process(this->ExtendedAircraftMissions_SmoothMoving)
		.Process(this->ExtendedAircraftMissions_EarlyDescend)
		.Process(this->ExtendedAircraftMissions_RearApproach)

		.Process(this->FallingDownDamage)
		.Process(this->FallingDownDamage_Water)

		.Process(this->FiringForceScatter)

		.Process(this->FireUp)
		.Process(this->FireUp_ResetInRetarget)
		//.Process(this->SecondaryFire)

		.Process(this->DebrisTypes_Limit)
		.Process(this->DebrisMinimums)

		.Process(this->EngineerRepairAmount)

		.Process(this->AttackMove_Follow)
		.Process(this->AttackMove_Follow_IncludeAir)
		.Process(this->AttackMove_Follow_IfMindControlIsFull)
		.Process(this->AttackMove_StopWhenTargetAcquired)
		.Process(this->AttackMove_PursuitTarget)

		.Process(this->MultiWeapon)
		.Process(this->MultiWeapon_IsSecondary)
		.Process(this->MultiWeapon_SelectCount)
		.Process(this->ReadMultiWeapon)

		.Process(this->VoiceIFVRepair)
		.Process(this->VoiceWeaponAttacks)
		.Process(this->VoiceEliteWeaponAttacks)

		.Process(this->BattlePoints)

		.Process(this->InfantryAutoDeploy)

		.Process(this->Suspicious)
		.Process(this->AttackIfSuspicious)
		;
}
void TechnoTypeExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<TechnoTypeClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TechnoTypeExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<TechnoTypeClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

// =============================
// container

TechnoTypeExt::ExtContainer::ExtContainer() : Container("TechnoTypeClass") { }
TechnoTypeExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

DEFINE_HOOK(0x711835, TechnoTypeClass_CTOR, 0x5)
{
	GET(TechnoTypeClass*, pItem, ESI);

	TechnoTypeExt::ExtMap.TryAllocate(pItem);

	return 0;
}

DEFINE_HOOK(0x711AE0, TechnoTypeClass_DTOR, 0x5)
{
	GET(TechnoTypeClass*, pItem, ECX);

	TechnoTypeExt::ExtMap.Remove(pItem);

	return 0;
}

DEFINE_HOOK_AGAIN(0x716DC0, TechnoTypeClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x7162F0, TechnoTypeClass_SaveLoad_Prefix, 0x6)
{
	GET_STACK(TechnoTypeClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	TechnoTypeExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x716DAC, TechnoTypeClass_Load_Suffix, 0xA)
{
	TechnoTypeExt::ExtMap.LoadStatic();

	return 0;
}

DEFINE_HOOK(0x717094, TechnoTypeClass_Save_Suffix, 0x5)
{
	TechnoTypeExt::ExtMap.SaveStatic();

	return 0;
}

//DEFINE_HOOK_AGAIN(0x716132, TechnoTypeClass_LoadFromINI, 0x5)// Section dont exist!
DEFINE_HOOK(0x716123, TechnoTypeClass_LoadFromINI, 0x5)
{
	GET(TechnoTypeClass*, pItem, EBP);
	GET_STACK(CCINIClass*, pINI, 0x380);

	TechnoTypeExt::ExtMap.LoadFromINI(pItem, pINI);

	return 0;
}

#if ANYONE_ACTUALLY_USE_THIS
DEFINE_HOOK(0x679CAF, RulesClass_LoadAfterTypeData_CompleteInitialization, 0x5)
{
	//GET(CCINIClass*, pINI, ESI);

	for (auto const& [pType, pExt] : BuildingTypeExt::ExtMap)
	{
		pExt->CompleteInitialization();
	}

	return 0;
}
#endif

DEFINE_HOOK(0x747E90, UnitTypeClass_LoadFromINI, 0x5)
{
	GET(UnitTypeClass*, pItem, ESI);

	if (auto pTypeExt = TechnoTypeExt::ExtMap.Find(pItem))
	{
		if (!pTypeExt->Harvester_Counted.isset() && pItem->Harvester)
			pTypeExt->Harvester_Counted = true;
	}

	return 0;
}
