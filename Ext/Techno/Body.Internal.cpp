#include "Body.h"

#include <AirstrikeClass.h>

#include <Utilities/EnumFunctions.h>
#include <Ext/HouseType/Body.h>
#include <Ext/House/Body.h>
#include <Ext/BuildingType/Body.h>

// Unsorted methods

void TechnoExt::ExtData::InitializeLaserTrails()
{
	if (this->LaserTrails.size())
		return;

	auto const pTypeExt = this->TypeExtData;
	this->LaserTrails.reserve(pTypeExt->LaserTrailData.size());

	for (auto const& entry : pTypeExt->LaserTrailData)
		this->LaserTrails.emplace_back(std::make_unique<LaserTrailClass>(entry.GetType(), this->OwnerObject()->Owner, entry.FLH, entry.IsOnTurret));
}

void TechnoExt::ObjectKilledBy(TechnoClass* pVictim, TechnoClass* pKiller, HouseClass* pHouseKiller)
{
	TechnoClass* pObjectKiller = nullptr;

	if (pKiller)
	{
		auto const pKillerType = pKiller->GetTechnoType();
		pObjectKiller = ((pKillerType->Spawned || pKillerType->MissileSpawn) && pKiller->SpawnOwner) ?
			pKiller->SpawnOwner : pKiller;

		if (!pObjectKiller)
			return;
	}

	if (pObjectKiller && pObjectKiller->BelongsToATeam())
	{
		if (auto const pFootKiller = generic_cast<FootClass*, true>(pObjectKiller))
		{
			auto const pKillerTechnoData = TechnoExt::ExtMap.Find(pObjectKiller);
			pKillerTechnoData->LastKillWasTeamTarget = pFootKiller->Team->Focus == pVictim;
		}
	}

	HouseClass* pHouse = pKiller ? pKiller->Owner : pHouseKiller;

	if (pHouse != pVictim->Owner)
	{
		auto pHouseExt = HouseExt::ExtMap.Find(pHouse);

		if (pHouseExt->AreBattlePointsEnabled())
		{
			int points = pHouseExt->CalculateBattlePoints(pVictim);
			pHouseExt->UpdateBattlePoints(points);
		}
	}
}

// reversed from 6F3D60
CoordStruct TechnoExt::GetFLHAbsoluteCoords(TechnoClass* pThis, CoordStruct pCoord, bool isOnTurret)
{
	auto const pType = pThis->GetTechnoType();
	auto const pFoot = abstract_cast<FootClass*, true>(pThis);
	Matrix3D mtx;

	// Step 1: get body transform matrix
	if (pFoot && pFoot->Locomotor)
		mtx = pFoot->Locomotor->Draw_Matrix(nullptr);
	else // no locomotor means no rotation or transform of any kind (f.ex. buildings) - Kerbiter
		mtx.MakeIdentity();

	// Steps 2-3: turret offset and rotation
	if (isOnTurret && (pType->Turret || !pFoot)) // If building has no turret, it's TurretFacing is TargetDirection
	{
		TechnoTypeExt::ApplyTurretOffset(pType, &mtx);

		const double turretRad = pThis->TurretFacing().GetRadian<32>();
		// For BuildingClass turret facing is equal to primary facing
		const float angle = pFoot ? (float)(turretRad - pThis->PrimaryFacing.Current().GetRadian<32>()) : (float)(turretRad);

		mtx.RotateZ(angle);
	}

	// Step 4: apply FLH offset
	mtx.Translate((float)pCoord.X, (float)pCoord.Y, (float)pCoord.Z);

	auto const result = mtx.GetTranslation();

	// Step 5: apply as an offset to global object coords
	// Resulting coords are mirrored along X axis, so we mirror it back
	auto const location = pThis->GetRenderCoords() + CoordStruct { (int)result.X, -(int)result.Y, (int)result.Z };

	return location;
}

CoordStruct TechnoExt::GetBurstFLH(TechnoClass* pThis, int weaponIndex, bool& FLHFound)
{
	FLHFound = false;
	CoordStruct FLH = CoordStruct::Empty;

	auto const pExt = TechnoExt::ExtMap.Find(pThis)->TypeExtData;

	auto const pInf = abstract_cast<InfantryClass*, true>(pThis);
	std::span<std::vector<CoordStruct>> pickedFLHs = pExt->WeaponBurstFLHs;

	if (pThis->Veterancy.IsElite())
	{
		if (pInf)
		{
			if (pInf->IsDeployed() && pExt->EliteDeployedWeaponBurstFLHs.size() > 0)
				pickedFLHs = pExt->EliteDeployedWeaponBurstFLHs;
			else if (pInf->Crawling && pExt->EliteCrouchedWeaponBurstFLHs.size() > 0)
				pickedFLHs = pExt->EliteCrouchedWeaponBurstFLHs;
			else
				pickedFLHs = pExt->EliteWeaponBurstFLHs;
		}
		else
		{
			pickedFLHs = pExt->EliteWeaponBurstFLHs;
		}
	}
	else if (pInf)
	{
		if (pInf->IsDeployed() && pExt->DeployedWeaponBurstFLHs.size() > 0)
			pickedFLHs = pExt->DeployedWeaponBurstFLHs;
		else if (pInf->Crawling && pExt->CrouchedWeaponBurstFLHs.size() > 0)
			pickedFLHs = pExt->CrouchedWeaponBurstFLHs;
	}
	if ((int)pickedFLHs[weaponIndex].size() > pThis->CurrentBurstIndex)
	{
		FLHFound = true;
		FLH = pickedFLHs[weaponIndex][pThis->CurrentBurstIndex];
	}

	return FLH;
}

CoordStruct TechnoExt::GetSimpleFLH(InfantryClass* pThis, int weaponIndex, bool& FLHFound)
{
	FLHFound = false;
	CoordStruct FLH = CoordStruct::Empty;

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type);
	Nullable<CoordStruct> pickedFLH;

	if (pThis->IsDeployed())
	{
		if (weaponIndex == 0)
			pickedFLH = pTypeExt->DeployedPrimaryFireFLH;
		else if (weaponIndex == 1)
			pickedFLH = pTypeExt->DeployedSecondaryFireFLH;
	}
	else
	{
		if (pThis->Crawling)
		{
			if (weaponIndex == 0)
				pickedFLH = pTypeExt->PronePrimaryFireFLH;
			else if (weaponIndex == 1)
				pickedFLH = pTypeExt->ProneSecondaryFireFLH;
		}
	}

	if (pickedFLH.isset())
	{
		FLH = pickedFLH.Get();
		FLHFound = true;
	}

	return FLH;
}

void TechnoExt::ExtData::InitializeDisplayInfo()
{
	const auto pThis = this->OwnerObject();
	const auto pPrimary = pThis->GetWeapon(0)->WeaponType;

	if (pPrimary && pThis->GetTechnoType()->LandTargeting != LandTargetingType::Land_Not_OK)
		pThis->RearmTimer.TimeLeft = pPrimary->ROF;
	else if (const auto pSecondary = pThis->GetWeapon(1)->WeaponType)
		pThis->RearmTimer.TimeLeft = pSecondary->ROF;

	pThis->RearmTimer.StartTime = Math::min(-2, -pThis->RearmTimer.TimeLeft);
}

void TechnoExt::ExtData::InitializeAttachEffects()
{
	auto const pTypeExt = this->TypeExtData;

	if (pTypeExt->AttachEffects.AttachTypes.size() < 1)
		return;

	auto const pThis = this->OwnerObject();
	AttachEffectClass::Attach(pThis, pThis->Owner, pThis, pThis, pTypeExt->AttachEffects);
}

// Gets tint colors for invulnerability, airstrike laser target and berserk, depending on parameters.
int TechnoExt::GetTintColor(TechnoClass* pThis, bool invulnerability, bool airstrike, bool berserk)
{
	int tintColor = 0;

	if (pThis)
	{
		if (invulnerability && pThis->IsIronCurtained())
		{
			tintColor |= pThis->ForceShielded ? RulesExt::Global()->TintColorForceShield : RulesExt::Global()->TintColorIronCurtain;
		}

		if (airstrike)
		{
			auto const pExt =  TechnoExt::ExtMap.Find(pThis);

			if (auto const pAirstrike = pExt->AirstrikeTargetingMe)
				tintColor |= pExt->TypeExtData->TintColorAirstrike;
		}

		if (berserk && pThis->Berzerk)
		{
			tintColor |= RulesExt::Global()->TintColorBerserk;
		}
	}

	return tintColor;
}

// Gets custom tint color from TechnoTypes & Warheads.
int TechnoExt::GetCustomTintColor(TechnoClass* pThis)
{
	int dummy = 0;
	int color = 0;
	TechnoExt::ApplyCustomTintValues(pThis, color, dummy);
	return color;
}

// Gets custom tint intensity from TechnoTypes & Warheads.
int TechnoExt::GetCustomTintIntensity(TechnoClass* pThis)
{
	int dummy = 0;
	int intensity = 0;
	TechnoExt::ApplyCustomTintValues(pThis, dummy, intensity);
	return intensity;
}

// Applies custom tint color and intensity from TechnoTypes and any AttachEffects and shields it might have on provided values.
void TechnoExt::ApplyCustomTintValues(TechnoClass* pThis, int& color, int& intensity)
{
	auto const pExt = TechnoExt::ExtMap.Find(pThis);
	auto const pOwner = pThis->Owner;

	if (pOwner == HouseClass::CurrentPlayer)
	{
		color |= pExt->TintColorOwner;
		intensity += pExt->TintIntensityOwner;
	}
	else if (pOwner->IsAlliedWith(HouseClass::CurrentPlayer))
	{
		color |= pExt->TintColorAllies;
		intensity += pExt->TintIntensityAllies;
	}
	else
	{
		color |= pExt->TintColorEnemies;
		intensity += pExt->TintIntensityEnemies;
	}
}

// This is still not even correct, but let's see how far this can help us
void TechnoExt::ChangeOwnerMissionFix(FootClass* pThis)
{
	pThis->ShouldScanForTarget = false;
	pThis->ShouldEnterAbsorber = false;
	pThis->ShouldEnterOccupiable = false;
	pThis->ShouldGarrisonStructure = false;
	auto const pType = pThis->GetTechnoType();

	if (pThis->HasAnyLink() || pType->ResourceGatherer) // Don't want miners to stop
		return;

	switch (pThis->GetCurrentMission())
	{
	case Mission::Harvest:
	case Mission::Sleep:
	case Mission::Harmless:
	case Mission::Repair:
		return;
	}

	pThis->Override_Mission(Mission::Guard, nullptr, nullptr); // I don't even know what this is
	pThis->ShouldLoseTargetNow = TRUE;
	pThis->QueueMission(pType->DefaultToGuardArea ? Mission::Area_Guard : Mission::Guard, true);
}

// Updates layers of all animations attached to the given techno.
void TechnoExt::UpdateAttachedAnimLayers(TechnoClass* pThis)
{
	// Skip if has no attached animations.
	if (!pThis->HasParachute)
		return;

	// Could possibly be faster to track the attached anims in TechnoExt but the profiler doesn't show this as a performance hog so whatever.
	for (auto const pAnim : AnimClass::Array)
	{
		if (pAnim->OwnerObject != pThis)
			continue;

		DisplayClass::Instance.Submit(pAnim);
	}
}
