#include "Body.h"

#include <Ext/WeaponType/Body.h>
#include <Ext/TechnoType/Body.h>
#include <Utilities/Macro.h>

// Ares reimplements the bullet obstacle logic so need to get creative to add any new functionality for that in Phobos.
// Not named PhobosTrajectoryHelper to avoid confusion with actual custom trajectory logic.
class BulletObstacleHelper
{
public:

	static CellClass* GetObstacle(CellClass* pSourceCell, CellClass* pTargetCell, CellClass* pCurrentCell, CoordStruct currentCoords, AbstractClass const* const pSource,
		AbstractClass const* const pTarget, HouseClass* pOwner, BulletTypeClass* pBulletType, BulletTypeExt::ExtData*& pBulletTypeExt, bool isTargetingCheck = false)
	{
		CellClass* pObstacleCell = nullptr;

		if (SubjectToObstacles(pBulletType, pBulletTypeExt))
		{
			if (SubjectToTerrain(pCurrentCell, pBulletType, pBulletTypeExt, isTargetingCheck))
				pObstacleCell = pCurrentCell;
		}

		return pObstacleCell;
	}

	static CellClass* FindFirstObstacle(CoordStruct const& pSourceCoords, CoordStruct const& pTargetCoords, AbstractClass const* const pSource,
		AbstractClass const* const pTarget, HouseClass* pOwner, BulletTypeClass* pBulletType, bool isTargetingCheck = false, bool subjectToGround = false)
	{
		BulletTypeExt::ExtData* pBulletTypeExt = BulletTypeExt::ExtMap.Find(pBulletType);

		if (SubjectToObstacles(pBulletType, pBulletTypeExt) || subjectToGround)
		{
			auto const sourceCell = CellClass::Coord2Cell(pSourceCoords);
			auto const pSourceCell = MapClass::Instance.GetCellAt(sourceCell);
			auto const targetCell = CellClass::Coord2Cell(pTargetCoords);
			auto const pTargetCell = MapClass::Instance.GetCellAt(targetCell);

			auto const sub = sourceCell - targetCell;
			auto const delta = CellStruct { (short)std::abs(sub.X), (short)std::abs(sub.Y) };
			auto const maxDelta = static_cast<size_t>(std::max(delta.X, delta.Y));
			auto const step = !maxDelta ? CoordStruct::Empty : (pTargetCoords - pSourceCoords) * (1.0 / maxDelta);
			CoordStruct crdCur = pSourceCoords;
			auto pCellCur = pSourceCell;

			for (size_t i = 0; i < maxDelta + isTargetingCheck; ++i)
			{
				if (auto const pCell = GetObstacle(pSourceCell, pTargetCell, pCellCur, crdCur, pSource, pTarget, pOwner, pBulletType, pBulletTypeExt, isTargetingCheck))
					return pCell;

				if (subjectToGround && crdCur.Z < MapClass::Instance.GetCellFloorHeight(crdCur))
					return pCellCur;

				crdCur += step;
				pCellCur = MapClass::Instance.GetCellAt(crdCur);
			}
		}

		return nullptr;
	}

	static CellClass* FindFirstImpenetrableObstacle(CoordStruct const& pSourceCoords, CoordStruct const& pTargetCoords, AbstractClass const* const pSource,
		AbstractClass const* const pTarget, HouseClass* pOwner, WeaponTypeClass* pWeapon, bool isTargetingCheck = false, bool subjectToGround = false)
	{
		// Does not currently need further checks.
		return FindFirstObstacle(pSourceCoords, pTargetCoords, pSource, pTarget, pOwner, pWeapon->Projectile, isTargetingCheck, subjectToGround);
	}

	static bool SubjectToObstacles(BulletTypeClass* pBulletType, BulletTypeExt::ExtData*& pBulletTypeExt)
	{
		bool subjectToTerrain = pBulletTypeExt->SubjectToLand.isset() || pBulletTypeExt->SubjectToWater.isset();

		return subjectToTerrain ? true : pBulletType->Level;
	}

	static bool SubjectToTerrain(CellClass* pCurrentCell, BulletTypeClass* pBulletType, BulletTypeExt::ExtData*& pBulletTypeExt, bool isTargetingCheck)
	{
		const bool isCellWater = pCurrentCell->LandType == LandType::Water || pCurrentCell->LandType == LandType::Beach;
		const bool isLevel = pBulletType->Level ? pCurrentCell->IsOnFloor() : false;

		if (!isTargetingCheck && isLevel && !pBulletTypeExt->SubjectToLand.isset() && !pBulletTypeExt->SubjectToWater.isset())
			return true;
		else if (!isCellWater && pBulletTypeExt->SubjectToLand.Get(false))
			return !isTargetingCheck ? pBulletTypeExt->SubjectToLand_Detonate : true;
		else if (isCellWater && pBulletTypeExt->SubjectToWater.Get(false))
			return !isTargetingCheck ? pBulletTypeExt->SubjectToWater_Detonate : true;

		return false;
	}

	static CoordStruct AddFLHToSourceCoords(const CoordStruct& sourceCoords, const CoordStruct& targetCoords, TechnoClass* const pTechno, AbstractClass* const pTarget,
		WeaponTypeClass* pWeapon, bool& subjectToGround)
	{
		// Buildings, air force, and passengers are not allowed, because they don't even know how to find a suitable location
		if (((pTechno->AbstractFlags & AbstractFlags::Foot) == AbstractFlags::None) || pTechno->IsInAir() || pTarget->IsInAir() || pTechno->Transporter)
		{
			// Turn off the switch for subsequent checks
			subjectToGround = false;
			return sourceCoords;
		}
		// Predicting the firing position of weapons
		// Unable to predict the degree of inclination of the techno yet
		Matrix3D mtx = Matrix3D::GetIdentity();
		// Position on the ground or bridge
		const auto pCell = MapClass::Instance.GetCellAt(sourceCoords);
		const auto source = pTechno->OnBridge ? pCell->GetCoordsWithBridge() : pCell->GetCoords();
		// Predicted orientation
		const float radian = (float)(-Math::atan2(targetCoords.Y - source.Y, targetCoords.X - source.X));
		mtx.RotateZ(radian);
		// Offset of turret, directly substitute because it is impossible to predict the orientation of the techno when it reaches this position
		// Only predict the situation when the techno is facing the target directly
		if (pTechno->HasTurret())
			TechnoTypeExt::ApplyTurretOffset(pTechno->GetTechnoType(), &mtx);
		// FLH of weapon, not use independent firing positions
		// Because this will result in different results due to the current burst, causing the techno to constantly move
		const auto pWeaponStruct = pTechno->GetWeapon(pTechno->SelectWeapon(pTarget));
		const auto& flh = pWeaponStruct->FLH;
		// If the weapon's burst is greater than 1, use the center firing position for calculation
		// This can avoid constantly searching for position and pacing back and forth
		mtx.Translate((float)flh.X, ((pWeaponStruct->WeaponType == pWeapon && pWeapon->Burst != 1) ? 0 : ((float)flh.Y)), (float)flh.Z);
		const auto result = mtx.GetTranslation();
		// Starting from the center position of the cell and adding the offset value
		return source + CoordStruct { (int)result.X, -(int)result.Y, (int)result.Z };
	}
};

// Hooks

DEFINE_HOOK(0x4688A9, BulletClass_Unlimbo_Obstacles, 0x6)
{
	enum { SkipGameCode = 0x468A3F, Continue = 0x4688BD };

	GET(BulletClass*, pThis, EBX);
	GET(CoordStruct const* const, sourceCoords, EDI);
	REF_STACK(CoordStruct const, targetCoords, STACK_OFFSET(0x54, -0x10));

	// Jul 5, 2025 - Starkku: Borrowing this hook for a parabomb check instead of adding a new one.
	if (pThis->HasParachute)
	{
		pThis->Velocity = BulletVelocity::Empty;
		return SkipGameCode;
	}

	auto const pType = pThis->Type;

	if (pType->Inviso)
	{
		auto const pThisOwner = pThis->Owner;
		auto const pOwner = pThisOwner ? pThisOwner->Owner : BulletExt::ExtMap.Find(pThis)->FirerHouse;
		const auto pObstacleCell = BulletObstacleHelper::FindFirstObstacle(*sourceCoords, targetCoords, pThisOwner, pThis->Target, pOwner, pType, false, false);

		if (pObstacleCell)
		{
			// TODO Accurate coords
			pThis->SetLocation(pObstacleCell->GetCoords());
			pThis->Speed = 0;
			pThis->Velocity = BulletVelocity::Empty;

			return SkipGameCode;
		}

		return Continue;
	}

	return 0;
}

DEFINE_HOOK(0x468C86, BulletClass_ShouldExplode_Obstacles, 0xA)
{
	enum { SkipGameCode = 0x468C90, Explode = 0x468C9F };

	GET(BulletClass*, pThis, ESI);

	auto const pType = pThis->Type;
	auto pBulletTypeExt = BulletTypeExt::ExtMap.Find(pType);

	if (BulletObstacleHelper::SubjectToObstacles(pType, pBulletTypeExt))
	{
		auto const pCellSource = MapClass::Instance.GetCellAt(pThis->SourceCoords);
		auto const pCellTarget = MapClass::Instance.GetCellAt(pThis->TargetCoords);
		auto const pCellCurrent = MapClass::Instance.GetCellAt(pThis->LastMapCoords);
		auto const pOwner = pThis->Owner ? pThis->Owner->Owner : BulletExt::ExtMap.Find(pThis)->FirerHouse;
		auto const pObstacleCell = BulletObstacleHelper::GetObstacle(pCellSource, pCellTarget, pCellCurrent, pThis->Location, pThis->Owner, pThis->Target, pOwner, pType, pBulletTypeExt, false);

		if (pObstacleCell)
			return Explode;
	}

	// Restore overridden instructions.
	R->EAX(pThis->GetHeight());
	return SkipGameCode;
}

namespace InRangeTemp
{
	TechnoClass* Techno = nullptr;
}

DEFINE_HOOK(0x6F7261, TechnoClass_InRange_SetContext, 0x5)
{
	GET(TechnoClass*, pThis, ESI);

	InRangeTemp::Techno = pThis;

	return 0;
}

DEFINE_HOOK(0x6F737F, TechnoClass_InRange_WeaponMinimumRange, 0x6)
{
	enum { SkipGameCode = 0x6F7385 };

	GET(WeaponTypeClass*, pWeapon, EDX);

	auto pTechno = InRangeTemp::Techno;

	if (const auto keepRange = WeaponTypeExt::GetTechnoKeepRange(pWeapon, pTechno, true))
		R->ECX(keepRange);
	else
		return 0;

	return SkipGameCode;
}

DEFINE_HOOK(0x6F7647, TechnoClass_InRange_Obstacles, 0x5)
{
	GET_BASE(WeaponTypeClass*, pWeapon, 0x10);
	GET(CoordStruct const* const, pSourceCoords, ESI);
	REF_STACK(CoordStruct const, targetCoords, STACK_OFFSET(0x3C, -0x1C));
	GET_BASE(AbstractClass* const, pTarget, 0xC);
	GET(CellClass*, pResult, EAX);

	auto pObstacleCell = pResult;
	const auto pTechno = InRangeTemp::Techno;

	if (!pObstacleCell)
	{
		bool subjectToGround = BulletTypeExt::ExtMap.Find(pWeapon->Projectile)->SubjectToGround.Get();
		const auto newSourceCoords = subjectToGround ? BulletObstacleHelper::AddFLHToSourceCoords(*pSourceCoords, targetCoords, pTechno, pTarget, pWeapon, subjectToGround) : *pSourceCoords;
		pObstacleCell = BulletObstacleHelper::FindFirstImpenetrableObstacle(newSourceCoords, targetCoords, pTechno, pTarget, pTechno->Owner, pWeapon, true, subjectToGround);
	}

	// Enable aircraft carriers to search for suitable attack positions on their own
	if (!pObstacleCell && pTechno->SpawnManager && (pTechno->AbstractFlags & AbstractFlags::Foot) && !pTechno->IsInAir())
	{
		const auto coords = pTechno->Location;
		pTechno->Location = *pSourceCoords; // Temporarily adjust the coordinates based on the path finding

		if (reinterpret_cast<bool(__thiscall*)(const TechnoClass*)>(0x703B10)(pTechno)) // Near by elevated bridge
			pObstacleCell = MapClass::Instance.GetCellAt(*pSourceCoords);

		pTechno->Location = coords;
	}

	InRangeTemp::Techno = nullptr;

	R->EAX(pObstacleCell);
	return 0;
}
