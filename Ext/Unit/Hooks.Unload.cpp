#include <AnimClass.h>
#include <UnitClass.h>
#include <TechnoClass.h>
#include <TunnelLocomotionClass.h>

#include <Ext/Anim/Body.h>
#include <Ext/Building/Body.h>
#include <Ext/Techno/Body.h>
#include <Utilities/EnumFunctions.h>
#include <Utilities/GeneralUtils.h>
#include <Utilities/Macro.h>

namespace UnitDeployConvertHelpers
{
	void RemoveDeploying(REGISTERS* R);
	void ChangeAmmo(REGISTERS* R);
	void ChangeAmmoOnUnloading(REGISTERS* R);
}

void UnitDeployConvertHelpers::RemoveDeploying(REGISTERS* R)
{
	GET(TechnoClass*, pThis, ESI);
	const auto pTypeExt = TechnoExt::ExtMap.Find(pThis)->TypeExtData;

	const bool canDeploy = pThis->CanDeploySlashUnload();
	R->AL(canDeploy);

	if (!canDeploy || pThis->BunkerLinkedItem)
		return;

	const int skipMinimum = pTypeExt->Ammo_DeployUnlockMinimumAmount;
	const int skipMaximum = pTypeExt->Ammo_DeployUnlockMaximumAmount;

	if (skipMinimum < 0 && skipMaximum < 0)
		return;

	const int ammo = pThis->Ammo;
	const bool moreThanMinimum = ammo >= skipMinimum;
	const bool lessThanMaximum = ammo <= skipMaximum;

	if ((skipMinimum || moreThanMinimum) && (skipMaximum || lessThanMaximum))
		return;

	R->AL(false);
}

void UnitDeployConvertHelpers::ChangeAmmo(REGISTERS* R)
{
	GET(UnitClass*, pThis, ECX);
	auto const pType = pThis->Type;

	if (pThis->Deployed && !pThis->BunkerLinkedItem && !pThis->Deploying)
	{
		if (const bool addOnDeploy = TechnoTypeExt::ExtMap.Find(pType)->Ammo_AddOnDeploy)
		{
			const int ammoCalc = std::max(pThis->Ammo + addOnDeploy, 0);
			pThis->Ammo = std::min(pType->Ammo, ammoCalc);
		}
	}

	R->EAX(pType);
}

void UnitDeployConvertHelpers::ChangeAmmoOnUnloading(REGISTERS* R)
{
	GET(UnitClass*, pThis, ESI);
	auto const pType = pThis->Type;

	if (pType->IsSimpleDeployer && !pThis->BunkerLinkedItem && pType->UnloadingClass == nullptr)
	{
		if (const bool addOnDeploy = TechnoTypeExt::ExtMap.Find(pType)->Ammo_AddOnDeploy)
		{
			const int ammoCalc = std::max(pThis->Ammo + addOnDeploy, 0);
			pThis->Ammo = std::min(pType->Ammo, ammoCalc);
		}
	}

	R->AL(pThis->Deployed);
}

DEFINE_HOOK(0x7396D2, UnitClass_TryToDeploy_Transfer, 0x5)
{
	GET(UnitClass*, pUnit, EBP);
	GET(BuildingClass*, pStructure, EBX);

	if (pUnit->Type->DeployToFire && pUnit->Target)
		pStructure->LastTarget = pUnit->Target;

	const auto pStructureExt = BuildingExt::ExtMap.Find(pStructure);
	pStructureExt->DeployedTechno = true;

	return 0;
}

DEFINE_HOOK(0x73FFE6, UnitClass_WhatAction_RemoveDeploying, 0xA)
{
	enum { Continue = 0x73FFF0 };
	UnitDeployConvertHelpers::RemoveDeploying(R);
	return Continue;
}

DEFINE_HOOK(0x730C70, DeployClass_Execute_RemoveDeploying, 0xA)
{
	enum { Continue = 0x730C7A };
	GET(TechnoClass*, pThis, ESI);

	if (abstract_cast<UnitClass*>(pThis))
		UnitDeployConvertHelpers::RemoveDeploying(R);
	else
		R->AL(pThis->CanDeploySlashUnload());

	return Continue;
}

DEFINE_HOOK(0x739C74, UnitClass_ToggleDeployState_ChangeAmmo, 0x6) // deploying
{
	enum { Continue = 0x739C7A };
	UnitDeployConvertHelpers::ChangeAmmo(R);
	return Continue;
}

DEFINE_HOOK(0x739E5A, UnitClass_ToggleSimpleDeploy_ChangeAmmo, 0x6) // undeploying
{
	enum { Continue = 0x739E60 };
	UnitDeployConvertHelpers::ChangeAmmo(R);
	return Continue;
}

DEFINE_HOOK(0x73DE78, UnitClass_Unload_ChangeAmmo, 0x6) // converters
{
	enum { Continue = 0x73DE7E };
	UnitDeployConvertHelpers::ChangeAmmoOnUnloading(R);
	return Continue;
}

// TODO: Replace all of Ares SimpleDeployer code.
#pragma region SimpleDeployer

namespace SimpleDeployerTemp
{
	bool HoverDeployedToLand = false;
	AnimTypeClass* DeployingAnim = nullptr;
}

DEFINE_HOOK(0x73CF46, UnitClass_Draw_It_KeepUnitVisible, 0x6)
{
	enum { KeepUnitVisible = 0x73CF62 };

	GET(UnitClass*, pThis, ESI);

	if (pThis->Deploying || pThis->Undeploying)
	{
		const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type);

		if (pTypeExt->DeployingAnim_KeepUnitVisible)
			return KeepUnitVisible;
	}

	return 0;
}

DEFINE_HOOK(0x73D6E6, UnitClass_Unload_Subterranean, 0x6)
{
	enum { ReturnFromFunction = 0x73DFB0 };

	GET(UnitClass*, pThis, ESI);

	if (pThis->Type->Locomotor == LocomotionClass::CLSIDs::Tunnel)
	{
		auto const pLoco = static_cast<TunnelLocomotionClass*>(pThis->Locomotor.GetInterfacePtr());

		if (pLoco->State != TunnelLocomotionClass::State::Idle)
			return ReturnFromFunction;
	}

	return 0;
}

// Ares hooks in at 739B8A, this goes before it and skips it if needed.
DEFINE_HOOK(0x739B7C, UnitClass_Deploy_DeployDir, 0x6)
{
	enum { SkipAnim = 0x739C70, PlayAnim = 0x739B9E };

	GET(UnitClass*, pThis, ESI);

	if (!pThis->InAir)
	{
		const auto pType = pThis->Type;

		if (pType->DeployingAnim)
		{
			if (TechnoTypeExt::ExtMap.Find(pType)->DeployingAnim_AllowAnyDirection)
				return PlayAnim;

			return 0;
		}

		pThis->Deployed = true;
	}

	return SkipAnim;
}

DEFINE_HOOK_AGAIN(0x739D8B, UnitClass_DeployUndeploy_DeployAnim, 0x5)
DEFINE_HOOK(0x739BA8, UnitClass_DeployUndeploy_DeployAnim, 0x5)
{
	enum { Deploy = 0x739C20, DeployUseUnitDrawer = 0x739C0A, Undeploy = 0x739E04, UndeployUseUnitDrawer = 0x739DEE };

	GET(UnitClass*, pThis, ESI);

	const bool isDeploying = R->Origin() == 0x739BA8;

	auto const pType = pThis->Type;
	auto const pExt = TechnoTypeExt::ExtMap.Find(pType);
	auto const pAnim = GameCreate<AnimClass>(pType->DeployingAnim,
		pThis->Location, 0, 1, 0x600, 0,
		!isDeploying ? pExt->DeployingAnim_ReverseForUndeploy : false);

	pThis->DeployAnim = pAnim;
	pAnim->SetOwnerObject(pThis);
	AnimExt::SetAnimOwnerHouseKind(pAnim, pThis->Owner, nullptr, false, true);
	AnimExt::ExtMap.Find(pAnim)->SetInvoker(pThis);

	if (pExt->DeployingAnim_UseUnitDrawer)
		return isDeploying ? DeployUseUnitDrawer : UndeployUseUnitDrawer;

	return isDeploying ? Deploy : Undeploy;
}

DEFINE_HOOK_AGAIN(0x739E81, UnitClass_DeployUndeploy_DeploySound, 0x6)
DEFINE_HOOK(0x739C86, UnitClass_DeployUndeploy_DeploySound, 0x6)
{
	enum { DeployReturn = 0x739CBF, UndeployReturn = 0x739EB8 };

	GET(UnitClass*, pThis, ESI);

	const bool isDeploying = R->Origin() == 0x739C86;
	const bool isDoneWithDeployUndeploy = isDeploying ? pThis->Deployed : !pThis->Deployed;

	if (isDoneWithDeployUndeploy)
		return 0; // Only play sound when done with deploying or undeploying.

	return isDeploying ? DeployReturn : UndeployReturn;
}

DEFINE_HOOK(0x739CBF, UnitClass_Deploy_DeployToLandHover, 0x5)
{
	GET(UnitClass*, pThis, ESI);

	if (pThis->Deployed)
	{
		const auto pType = pThis->Type;

		if (pType->DeployToLand && pType->Locomotor == LocomotionClass::CLSIDs::Hover)
			SimpleDeployerTemp::HoverDeployedToLand = true;
	}

	return 0;
}

DEFINE_HOOK_AGAIN(0x73DED8, UnitClass_Unload_DeployToLandHover, 0x7)
DEFINE_HOOK(0x73E5B1, UnitClass_Unload_DeployToLandHover, 0x8)
{
	if (SimpleDeployerTemp::HoverDeployedToLand)
	{
		GET(UnitClass*, pThis, ESI);

		// Ares' DeployToLand 'fix' for Hover IsSimpleDeployer vehicles does not set/reset certain values
		// and has a chance to get stuck in Unload mission as a result, following should remedy that.
		SimpleDeployerTemp::HoverDeployedToLand = false;
		pThis->SetHeight(0);
		pThis->InAir = false;
		pThis->ForceMission(Mission::Guard);
	}

	return 0;
}

// Do not display hover bobbing when landed during deploying.
DEFINE_HOOK(0x513D2C, HoverLocomotionClass_ProcessBobbing_DeployToLand, 0x6)
{
	enum { SkipBobbing = 0x513F2A };

	GET(LocomotionClass*, pThis, ECX);

	if (auto const pUnit = abstract_cast<UnitClass*>(pThis->Owner))
	{
		if (pUnit->Deploying && pUnit->Type->DeployToLand)
			return SkipBobbing;
	}

	return 0;
}

// Trick Ares into thinking it can deploy in any direction if anim does not constrain it by temporarily removing the anim.
DEFINE_HOOK(0x514325, HoverLocomotionClass_Process_DeployingAnim1, 0x8)
{
	GET(ILocomotion*, iLoco, ESI);
	GET(bool, isMoving, EAX);

	auto const pLinkedTo = static_cast<LocomotionClass*>(iLoco)->LinkedTo;
	auto const pType = pLinkedTo->GetTechnoType();

	if (pType->DeployToLand && pType->DeployingAnim)
	{
		auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

		if (pTypeExt->DeployingAnim_AllowAnyDirection)
		{
			SimpleDeployerTemp::DeployingAnim = pType->DeployingAnim;
			pType->DeployingAnim = nullptr;
		}
	}

	return isMoving ? 0x51432D : 0x514A21;
}

// Restore the DeployingAnim to normal after.
DEFINE_HOOK(0x514AD0, HoverLocomotionClass_Process_DeployingAnim2, 0x5)
{
	GET(ILocomotion*, iLoco, ESI);

	if (SimpleDeployerTemp::DeployingAnim)
	{
		auto const pLinkedTo = static_cast<LocomotionClass*>(iLoco)->LinkedTo;
		auto const pType = pLinkedTo->GetTechnoType();
		pType->DeployingAnim = SimpleDeployerTemp::DeployingAnim;
		SimpleDeployerTemp::DeployingAnim = nullptr;
	}

	return 0;
}

// DeployToLand units increment WalkingFramesSoFar on every frame, on hover units this causes weird behaviour with move sounds etc.
DEFINE_HOOK(0x4DA9C1, FootClass_AI_DeployToLand, 0x6)
{
	enum { SkipGameCode = 0x4DAA01 };

	GET(FootClass*, pThis, ESI);

	if (pThis->GetTechnoType()->Locomotor == LocomotionClass::CLSIDs::Hover)
		return SkipGameCode;

	return 0;
}

#pragma endregion
