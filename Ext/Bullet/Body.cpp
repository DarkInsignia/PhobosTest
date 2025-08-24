#include "Body.h"

#include <Ext/Anim/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/RadSite/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Ext/Cell/Body.h>
#include <Ext/EBolt/Body.h>
#include <Utilities/EnumFunctions.h>
#include <Utilities/AresFunctions.h>
#include <Misc/FlyingStrings.h>

BulletExt::ExtContainer BulletExt::ExtMap;

void BulletExt::ExtData::InterceptBullet(TechnoClass* pSource, BulletClass* pInterceptor)
{
	const auto pThis = this->OwnerObject();
	auto pTypeExt = this->TypeExtData;
	const auto pInterceptorType = BulletExt::ExtMap.Find(pInterceptor)->InterceptorTechnoType->InterceptorType.get();

	if (!pTypeExt->Armor.isset())
	{
		if (!pInterceptorType->KeepIntact)
			this->InterceptedStatus |= InterceptedStatus::Intercepted;
	}
	else
	{
		const double versus = GeneralUtils::GetWarheadVersusArmor(pInterceptor->WH, pTypeExt->Armor.Get());

		if (versus == 0.0)
			return;

		const int damage = static_cast<int>(pInterceptor->Health * versus);
		this->CurrentStrength -= damage;

		if (Phobos::DisplayDamageNumbers && damage != 0)
			GeneralUtils::DisplayDamageNumberString(damage, DamageDisplayType::Intercept, pThis->GetRenderCoords(), this->DamageNumberOffset);

		if (this->CurrentStrength <= 0)
		{
			this->CurrentStrength = 0;

			if (!pInterceptorType->KeepIntact)
				this->InterceptedStatus |= InterceptedStatus::Intercepted;
		}
	}

	this->DetonateOnInterception = !pInterceptorType->DeleteOnIntercept.Get(pTypeExt->Interceptable_DeleteOnIntercept);

	if (const auto pWeaponOverride = pInterceptorType->WeaponOverride.Get(pTypeExt->Interceptable_WeaponOverride))
	{
		pThis->WeaponType = pWeaponOverride;
		pThis->Health = pInterceptorType->WeaponCumulativeDamage.Get() ? pThis->Health + pWeaponOverride->Damage : pWeaponOverride->Damage;
		pThis->WH = pWeaponOverride->Warhead;
		pThis->Bright = pWeaponOverride->Bright;

		if (pInterceptorType->WeaponReplaceProjectile
			&& pWeaponOverride->Projectile
			&& pWeaponOverride->Projectile != pThis->Type)
		{
			pThis->Speed = pWeaponOverride->Speed;
			pThis->Type = pWeaponOverride->Projectile;
			pTypeExt = BulletTypeExt::ExtMap.Find(pThis->Type);
			this->TypeExtData = pTypeExt;

			if (this->LaserTrails.size())
				this->LaserTrails.clear();

			if (!pThis->Type->Inviso)
				this->InitializeLaserTrails();

			// Lose target if the current bullet is no longer interceptable.
			if (pSource && (!pTypeExt->Interceptable || (pTypeExt->Armor.isset() && GeneralUtils::GetWarheadVersusArmor(pInterceptor->WH, pTypeExt->Armor.Get()) == 0.0)))
				pSource->SetTarget(nullptr);
		}
	}
}

void BulletExt::ExtData::ApplyRadiationToCell(CellStruct cell, int spread, int radLevel)
{
	const auto pCell = MapClass::Instance.TryGetCellAt(cell);

	if (!pCell)
		return;

	const auto pThis = this->OwnerObject();
	const auto pWeapon = pThis->GetWeaponType();
	const auto pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon);
	const auto pRadType = pWeaponExt->RadType;
	const auto pCellExt = CellExt::ExtMap.Find(pCell);

	const auto it = std::find_if(pCellExt->RadSites.cbegin(), pCellExt->RadSites.cend(),
		[=](const auto pSite)
		{
			const auto pRadExt = RadSiteExt::ExtMap.Find(pSite);

			if (pRadExt->Type != pRadType || spread != pSite->Spread)
				return false;

			if (pRadExt->RadInvoker && pThis->Owner)
				return pRadExt->RadInvoker == pThis->Owner;

			return true;
		}
	);

	if (it != pCellExt->RadSites.cend())
	{
		const auto pRadExt = RadSiteExt::ExtMap.Find(*it);
		// Handle It
		pRadExt->Add(std::min(radLevel, pRadType->GetLevelMax() - (*it)->GetRadLevel()));
		return;
	}

	const auto pThisHouse = pThis->Owner ? pThis->Owner->Owner : this->FirerHouse;
	RadSiteExt::CreateInstance(cell, spread, radLevel, pWeaponExt, pThisHouse, pThis->Owner);
}

void BulletExt::ExtData::InitializeLaserTrails()
{
	if (this->LaserTrails.size())
		return;

	auto const pThis = this->OwnerObject();
	auto const pTypeExt = BulletTypeExt::ExtMap.Find(pThis->Type);
	auto const pOwner = pThis->Owner ? pThis->Owner->Owner : nullptr;
	this->LaserTrails.reserve(pTypeExt->LaserTrail_Types.size());

	for (auto const& idxTrail : pTypeExt->LaserTrail_Types)
		this->LaserTrails.emplace_back(std::make_unique<LaserTrailClass>(LaserTrailTypeClass::Array[idxTrail].get(), pOwner));
}

static inline int SetBuildingFireAnimZAdjust(BuildingClass* pBuilding, int animY)
{
	if (pBuilding->GetOccupantCount() > 0)
		return -200;

	const auto renderCoords = pBuilding->GetRenderCoords();
	const auto zAdj = (animY - renderCoords.Y) / -4;
	return (zAdj >= 0) ? 0 : zAdj;
}

// Make sure pBullet and pBullet->WeaponType is not empty before call
inline void BulletExt::SimulatedFiringAnim(BulletClass* pBullet, HouseClass* pHouse, ObjectClass* pAttach)
{
	const auto pWeapon = pBullet->WeaponType;
	const auto animCounts = pWeapon->Anim.Count;

	if (animCounts <= 0)
		return;

	const auto pFirer = pBullet->Owner;
	const auto pAnimType = pWeapon->Anim[(animCounts % 8 == 0) // Have direction
		? (static_cast<int>((Math::atan2(pBullet->Velocity.Y , pBullet->Velocity.X) / Math::TwoPi + 1.5) * animCounts - (animCounts / 8) + 0.5) % animCounts) // Calculate direction
		: ScenarioClass::Instance->Random.RandomRanged(0 , animCounts - 1)]; // Simple random;
/*
	const auto velocityRadian = Math::atan2(pBullet->Velocity.Y , pBullet->Velocity.X);
	const auto ratioOfRotateAngle = velocityRadian / Math::TwoPi;
	const auto correctRatioOfRotateAngle = ratioOfRotateAngle + 1.5; // Correct the Y-axis in reverse and ensure that the ratio is a positive number
	const auto animIndex = correctRatioOfRotateAngle * animCounts;
	const auto correctAnimIndex = animIndex - (animCounts / 8); // A multiple of 8 greater than 8 will have an additional offset
	const auto trueAnimIndex = static_cast<int>(correctAnimIndex + 0.5) % animCounts; // Round down and prevent exceeding the scope
*/

	if (!pAnimType)
		return;

	const auto pAnim = GameCreate<AnimClass>(pAnimType, pBullet->SourceCoords);

	AnimExt::SetAnimOwnerHouseKind(pAnim, pHouse, nullptr, false, true);
	AnimExt::ExtMap.Find(pAnim)->SetInvoker(pFirer, pHouse);

	if (pAttach)
	{
		if (const auto pBuilding = abstract_cast<BuildingClass*, true>(pAttach))
			pAnim->ZAdjust = SetBuildingFireAnimZAdjust(pBuilding, pBullet->SourceCoords.Y);
		else
			pAnim->SetOwnerObject(pAttach);
	}
	else if (const auto pBuilding = abstract_cast<BuildingClass*>(pFirer))
	{
		pAnim->ZAdjust = SetBuildingFireAnimZAdjust(pBuilding, pBullet->SourceCoords.Y);
	}
}

// Make sure pBullet and pBullet->WeaponType is not empty before call
inline void BulletExt::SimulatedFiringReport(BulletClass* pBullet)
{
	const auto pWeapon = pBullet->WeaponType;

	if (pWeapon->Report.Count <= 0)
		return;

	const auto pFirer = pBullet->Owner;
	const auto reportIndex = pWeapon->Report[(pFirer ? pFirer->unknown_short_3C8 : ScenarioClass::Instance->Random.Random()) % pWeapon->Report.Count];
	VocClass::PlayAt(reportIndex, pBullet->Location, nullptr);
}

// Make sure pBullet and pBullet->WeaponType is not empty before call
inline void BulletExt::SimulatedFiringLaser(BulletClass* pBullet, HouseClass* pHouse)
{
	// Can not use 0x6FD210 because the firer may die
	const auto pWeapon = pBullet->WeaponType;

	if (!pWeapon->IsLaser)
		return;

	const auto pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon);

	if (pWeapon->IsHouseColor || pWeaponExt->Laser_IsSingleColor)
	{
		const auto black = ColorStruct { 0, 0, 0 };
		const auto pLaser = GameCreate<LaserDrawClass>(pBullet->SourceCoords, (pBullet->Type->Inviso ? pBullet->Location : pBullet->TargetCoords),
			((pWeapon->IsHouseColor && pHouse) ? pHouse->LaserColor : pWeapon->LaserInnerColor), black, black, pWeapon->LaserDuration);

		pLaser->IsHouseColor = true;
		pLaser->Thickness = pWeaponExt->LaserThickness;
		pLaser->IsSupported = (pLaser->Thickness > 3);
	}
	else
	{
		const auto pLaser = GameCreate<LaserDrawClass>(pBullet->SourceCoords, (pBullet->Type->Inviso ? pBullet->Location : pBullet->TargetCoords),
			pWeapon->LaserInnerColor, pWeapon->LaserOuterColor, pWeapon->LaserOuterSpread, pWeapon->LaserDuration);

		pLaser->IsHouseColor = false;
		pLaser->Thickness = 3;
		pLaser->IsSupported = false;
	}
}

// Make sure pBullet and pBullet->WeaponType is not empty before call
inline void BulletExt::SimulatedFiringElectricBolt(BulletClass* pBullet)
{
	// Can not use 0x6FD460 because the firer may die
	const auto pWeapon = pBullet->WeaponType;

	if (!pWeapon->IsElectricBolt)
		return;

	const auto pBolt = EBoltExt::CreateEBolt(pWeapon);
	pBolt->AlternateColor = pWeapon->IsAlternateColor;

	const auto targetCoords = pBullet->Type->Inviso ? pBullet->Location : pBullet->TargetCoords;
	pBolt->Fire(pBullet->SourceCoords, targetCoords, 0);

	if (const auto particle = WeaponTypeExt::ExtMap.Find(pWeapon)->Bolt_ParticleSystem.Get(RulesClass::Instance->DefaultSparkSystem))
		GameCreate<ParticleSystemClass>(particle, targetCoords, nullptr, nullptr, CoordStruct::Empty, nullptr);
}

// Make sure pBullet and pBullet->WeaponType is not empty before call
inline void BulletExt::SimulatedFiringRadBeam(BulletClass* pBullet, HouseClass* pHouse)
{
	const auto pWeapon = pBullet->WeaponType;

	if (!pWeapon->IsRadBeam)
		return;

	const auto pWH = pWeapon->Warhead;
	const bool isTemporal = pWH && pWH->Temporal;
	const auto pRadBeam = RadBeam::Allocate(isTemporal ? RadBeamType::Temporal : RadBeamType::RadBeam);

	pRadBeam->SetCoordsSource(pBullet->SourceCoords);
	pRadBeam->SetCoordsTarget((pBullet->Type->Inviso ? pBullet->Location : pBullet->TargetCoords));

	const auto pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon);

	pRadBeam->Color = (pWeaponExt->Beam_IsHouseColor && pHouse) ? pHouse->LaserColor
		: pWeaponExt->Beam_Color.Get(isTemporal ? RulesClass::Instance->ChronoBeamColor : RulesClass::Instance->RadColor);

	pRadBeam->Period = pWeaponExt->Beam_Duration;
	pRadBeam->Amplitude = pWeaponExt->Beam_Amplitude;
}

// Make sure pBullet and pBullet->WeaponType is not empty before call
inline void BulletExt::SimulatedFiringParticleSystem(BulletClass* pBullet, HouseClass* pHouse)
{
	if (const auto pPSType = pBullet->WeaponType->AttachedParticleSystem)
	{
		GameCreate<ParticleSystemClass>(pPSType, pBullet->SourceCoords, pBullet->Target, pBullet->Owner,
			(pBullet->Type->Inviso ? pBullet->Location : pBullet->TargetCoords), pHouse);
	}
}

// Make sure pBullet is not empty before call
void BulletExt::SimulatedFiringUnlimbo(BulletClass* pBullet, HouseClass* pHouse, WeaponTypeClass* pWeapon, const CoordStruct& sourceCoords, bool randomVelocity)
{
	// Weapon
	pBullet->WeaponType = pWeapon;

	// Range
	const int projectileRange = WeaponTypeExt::ExtMap.Find(pWeapon)->ProjectileRange.Get();
	pBullet->Range = projectileRange;

	// House
	BulletExt::ExtMap.Find(pBullet)->FirerHouse = pHouse;

	const auto pType = pBullet->Type;

	// Palette
	if (pType->FirersPalette)
		pBullet->InheritedColor = pHouse->ColorSchemeIndex;

	// Velocity
	auto velocity = BulletVelocity::Empty;

	// If someone asks me, I would say Arcing is just a piece of shit
	// But there are still people who like to use it, so anyway, it has been fixed
	if (pType->Arcing)
	{
		// The target must exist during launch
		const auto targetCoords = pBullet->Target->GetCenterCoords();
		const auto gravity = BulletTypeExt::GetAdjustedGravity(pType);
		const auto distanceCoords = targetCoords - sourceCoords;
		const auto horizontalDistance = Point2D { distanceCoords.X, distanceCoords.Y }.Magnitude();
		const bool lobber = pWeapon->Lobber || static_cast<int>(horizontalDistance) < distanceCoords.Z; // 0x70D590
		// The lower the horizontal velocity, the higher the trajectory
		// WW calculates the launch angle (and limits it) before calculating the velocity
		// Here, some magic numbers are used to directly simulate its calculation
		const auto speedMult = (lobber ? 0.45 : (distanceCoords.Z > 0 ? 0.68 : 1.0)); // Simulated 0x48A9D0
		const auto speed = speedMult * sqrt(horizontalDistance * gravity * 1.2); // 0x48AB90

		// Simulate firing Arcing bullet
		if (horizontalDistance < 1e-10 || speed < 1e-10)
		{
			// No solution
			velocity.Z = speed;
		}
		else
		{
			const auto mult = speed / horizontalDistance;
			velocity.X = static_cast<double>(distanceCoords.X) * mult;
			velocity.Y = static_cast<double>(distanceCoords.Y) * mult;
			velocity.Z = static_cast<double>(distanceCoords.Z) * mult + (gravity * horizontalDistance) / (2 * speed);
		}
	}
	else if (randomVelocity)
	{
		DirStruct dir;
		dir.SetValue<5>(ScenarioClass::Instance->Random.RandomRanged(0, 31));

		const auto cos_factor = -2.44921270764e-16; // cos(1.5 * Math::Pi * 1.00001)
		const auto flatSpeed = cos_factor * pBullet->Speed;

		const auto radians = dir.GetRadian<32>();
		velocity = BulletVelocity { Math::cos(radians) * flatSpeed, Math::sin(radians) * flatSpeed, static_cast<double>(-pBullet->Speed) };
	}

	// Unlimbo
	pBullet->MoveTo(sourceCoords, velocity);
}

// Make sure pBullet and pBullet->WeaponType is not empty before call
void BulletExt::SimulatedFiringEffects(BulletClass* pBullet, HouseClass* pHouse, ObjectClass* pAttach, bool firingEffect, bool visualEffect)
{
	if (firingEffect)
	{
		BulletExt::SimulatedFiringAnim(pBullet, pHouse, pAttach);
		BulletExt::SimulatedFiringReport(pBullet);
	}

	if (visualEffect)
	{
		BulletExt::SimulatedFiringLaser(pBullet, pHouse);
		BulletExt::SimulatedFiringElectricBolt(pBullet);
		BulletExt::SimulatedFiringRadBeam(pBullet, pHouse);
		BulletExt::SimulatedFiringParticleSystem(pBullet, pHouse);
	}
}

void BulletExt::ApplyArcingFix(BulletClass* pThis, const CoordStruct& sourceCoords, const CoordStruct& targetCoords, BulletVelocity& velocity)
{
	const auto distanceCoords = targetCoords - sourceCoords;
	const auto horizontalDistance = Point2D { distanceCoords.X, distanceCoords.Y }.Magnitude();
	const bool lobber = pThis->WeaponType->Lobber || static_cast<int>(horizontalDistance) < distanceCoords.Z; // 0x70D590
	// The lower the horizontal velocity, the higher the trajectory
	// WW calculates the launch angle (and limits it) before calculating the velocity
	// Here, some magic numbers are used to directly simulate its calculation
	const auto speedMult = (lobber ? 0.45 : (distanceCoords.Z > 0 ? 0.68 : 1.0)); // Simulated 0x48A9D0
	const auto gravity = BulletTypeExt::GetAdjustedGravity(pThis->Type);
	const auto speed = speedMult * sqrt(horizontalDistance * gravity * 1.2); // 0x48AB90

	if (horizontalDistance < 1e-10 || speed < 1e-10)
	{
		// No solution
		velocity.Z = speed;
	}
	else
	{
		const auto mult = speed / horizontalDistance;
		velocity.X = static_cast<double>(distanceCoords.X) * mult;
		velocity.Y = static_cast<double>(distanceCoords.Y) * mult;
		velocity.Z = (static_cast<double>(distanceCoords.Z) + velocity.Z) * mult + (gravity * horizontalDistance) / (2 * speed);
	}
}

// =============================
// load / save

template <typename T>
void BulletExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->TypeExtData)
		.Process(this->FirerHouse)
		.Process(this->CurrentStrength)
		.Process(this->InterceptorTechnoType)
		.Process(this->InterceptedStatus)
		.Process(this->DetonateOnInterception)
		.Process(this->LaserTrails)
		.Process(this->SnappedToTarget)
		.Process(this->DamageNumberOffset)
		.Process(this->ParabombFallRate)

		.Process(this->Trajectory) // Keep this shit at last
		;
}

void BulletExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<BulletClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void BulletExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<BulletClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

// =============================
// container

BulletExt::ExtContainer::ExtContainer() : Container("BulletClass") { }

BulletExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

DEFINE_HOOK(0x4664BA, BulletClass_CTOR, 0x5)
{
	GET(BulletClass*, pItem, ESI);

	BulletExt::ExtMap.TryAllocate(pItem);

	return 0;
}

DEFINE_HOOK(0x4665E9, BulletClass_DTOR, 0xA)
{
	GET(BulletClass*, pItem, ESI);
	BulletExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x46AFB0, BulletClass_SaveLoad_Prefix, 0x8)
DEFINE_HOOK(0x46AE70, BulletClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(BulletClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	BulletExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK_AGAIN(0x46AF97, BulletClass_Load_Suffix, 0x7)
DEFINE_HOOK(0x46AF9E, BulletClass_Load_Suffix, 0x7)
{
	BulletExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x46AFC4, BulletClass_Save_Suffix, 0x3)
{
	BulletExt::ExtMap.SaveStatic();
	return 0;
}
