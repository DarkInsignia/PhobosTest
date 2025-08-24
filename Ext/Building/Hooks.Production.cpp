#include "Body.h"

#include <Ext/House/Body.h>

DEFINE_HOOK(0x4401BB, BuildingClass_AI_PickWithFreeDocks, 0x6)
{
	GET(BuildingClass*, pBuilding, ESI);

	auto const pOwner = pBuilding->Owner;
	const int index = pOwner->ProducingAircraftTypeIndex;
	auto const pType = index >= 0 ? AircraftTypeClass::Array.GetItem(index) : nullptr;

	if (RulesExt::Global()->AllowParallelAIQueues && !RulesExt::Global()->ForbidParallelAIQueues_Aircraft && (!pType || !TechnoTypeExt::ExtMap.Find(pType)->ForbidParallelAIQueues))
		return 0;

	if (pOwner->Type->MultiplayPassive
		|| pOwner->IsCurrentPlayer()
		|| pOwner->IsNeutral())
		return 0;

	if (pBuilding->Type->Factory == AbstractType::AircraftType)
	{
		if (pBuilding->Factory
			&& !BuildingExt::HasFreeDocks(pBuilding))
		{
			if (auto const pBldExt = BuildingExt::ExtMap.TryFind(pBuilding))
				pBldExt->UpdatePrimaryFactoryAI();
		}
	}

	return 0;
}

DEFINE_HOOK(0x4502F4, BuildingClass_Update_Factory_Phobos, 0x6)
{
	GET(BuildingClass*, pThis, ESI);

	auto* const owner = pThis->Owner;
	auto* const rulesEx = RulesExt::Global();

	if (!owner->Production || !rulesEx->AllowParallelAIQueues)
		return 0;

	auto* const hx = HouseExt::ExtMap.Find(owner);
	const AbstractType facKind = pThis->Type->Factory;
	const bool naval = pThis->Type->Naval;

	// pick the correct per-type slot pointer
	BuildingClass** slot = nullptr;
	switch (facKind)
	{
	case AbstractType::BuildingType: slot = &hx->Factory_BuildingType; break;
	case AbstractType::UnitType:     slot = naval ? &hx->Factory_NavyType : &hx->Factory_VehicleType; break;
	case AbstractType::InfantryType: slot = &hx->Factory_InfantryType; break;
	case AbstractType::AircraftType: slot = &hx->Factory_AircraftType; break;
	default: break;
	}
	if (!slot) return 0;

	// initialize slot if empty
	if (!*slot)
	{
		*slot = pThis;
		return 0;
	}

	// if this isn’t the current primary, we may need to skip based on rules/type flags
	if (*slot != pThis)
	{
		enum { Skip = 0x4503CA };

		int index = -1;
		TechnoTypeClass* typ = nullptr;

		switch (facKind)
		{
		case AbstractType::BuildingType:
			if (rulesEx->ForbidParallelAIQueues_Building) return Skip;
			index = owner->ProducingBuildingTypeIndex;
			typ = (index >= 0) ? BuildingTypeClass::Array.GetItem(index) : nullptr;
			break;

		case AbstractType::InfantryType:
			if (rulesEx->ForbidParallelAIQueues_Infantry) return Skip;
			index = owner->ProducingInfantryTypeIndex;
			typ = (index >= 0) ? InfantryTypeClass::Array.GetItem(index) : nullptr;
			break;

		case AbstractType::AircraftType:
			if (rulesEx->ForbidParallelAIQueues_Aircraft) return Skip;
			index = owner->ProducingAircraftTypeIndex;
			typ = (index >= 0) ? AircraftTypeClass::Array.GetItem(index) : nullptr;
			break;

		case AbstractType::UnitType:
			if (naval ? rulesEx->ForbidParallelAIQueues_Navy
					  : rulesEx->ForbidParallelAIQueues_Vehicle) return Skip;
			index = naval ? HouseExt::ExtMap.Find(owner)->ProducingNavalUnitTypeIndex
				: owner->ProducingUnitTypeIndex;
			typ = (index >= 0) ? UnitTypeClass::Array.GetItem(index) : nullptr;
			break;

		default: break;
		}

		if (typ && TechnoTypeExt::ExtMap.Find(typ)->ForbidParallelAIQueues)
			return Skip;
	}

	return 0;
}

//const byte old_empty_log[] = { 0xC3 };
DEFINE_JUMP(CALL, 0x4CA016, 0x4CA19F); // randomly chosen 0xC3

DEFINE_HOOK(0x4CA07A, FactoryClass_AbandonProduction_Phobos, 0x8)
{
	GET(FactoryClass*, pFactory, ESI);
	GET_STACK(DWORD const, calledby, 0x18);

	auto const pTechno = pFactory->Object;

	if (calledby < 0x7F0000) // Replace the old log with this to figure out where keeps flushing the stream
	{
		Debug::LogGame("(%08x) : %s is abandoning production of %s[%s]\n"
			, calledby - 5
			, pFactory->Owner->PlainName
			, pTechno->GetType()->Name
			, pTechno->get_ID());
	}

	if (!RulesExt::Global()->AllowParallelAIQueues)
		return 0;

	auto const pOwnerExt = HouseExt::ExtMap.Find(pFactory->Owner);
	auto const pType = pTechno->GetTechnoType();
	const bool forbid = TechnoTypeExt::ExtMap.Find(pType)->ForbidParallelAIQueues;

	switch (pTechno->WhatAmI())
	{
	case AbstractType::Building:
		if (RulesExt::Global()->ForbidParallelAIQueues_Building || forbid)
			pOwnerExt->Factory_BuildingType = nullptr;
		break;
	case AbstractType::Unit:
		if (!pType->Naval)
		{
			if (RulesExt::Global()->ForbidParallelAIQueues_Vehicle || forbid)
				pOwnerExt->Factory_VehicleType = nullptr;
		}
		else
		{
			if (RulesExt::Global()->ForbidParallelAIQueues_Navy || forbid)
				pOwnerExt->Factory_NavyType = nullptr;
		}
		break;
	case AbstractType::Infantry:
		if (RulesExt::Global()->ForbidParallelAIQueues_Infantry || forbid)
			pOwnerExt->Factory_InfantryType = nullptr;
		break;
	case AbstractType::Aircraft:
		if (RulesExt::Global()->ForbidParallelAIQueues_Aircraft || forbid)
			pOwnerExt->Factory_AircraftType = nullptr;
		break;
	default:
		break;
	}

	return 0;
}

DEFINE_HOOK(0x444119, BuildingClass_KickOutUnit_UnitType_Phobos, 0x6)
{
	GET(UnitClass*, pUnit, EDI);
	GET(BuildingClass*, pFactory, ESI);

	auto const pHouseExt = HouseExt::ExtMap.Find(pFactory->Owner);

	if (pUnit->Type->Naval && pHouseExt->Factory_NavyType == pFactory)
		pHouseExt->Factory_NavyType = nullptr;
	else if (pHouseExt->Factory_VehicleType == pFactory)
		pHouseExt->Factory_VehicleType = nullptr;

	return 0;
}

DEFINE_HOOK(0x444131, BuildingClass_KickOutUnit_InfantryType_Phobos, 0x6)
{
	GET(BuildingClass*, pFactory, ESI);

	auto const pHouseExt = HouseExt::ExtMap.Find(pFactory->Owner);

	if (pHouseExt->Factory_InfantryType == pFactory)
		pHouseExt->Factory_InfantryType = nullptr;

	return 0;
}

DEFINE_HOOK(0x44531F, BuildingClass_KickOutUnit_BuildingType_Phobos, 0xA)
{
	GET(BuildingClass*, pFactory, ESI);

	auto const pHouseExt = HouseExt::ExtMap.Find(pFactory->Owner);

	if (pHouseExt->Factory_BuildingType == pFactory)
		pHouseExt->Factory_BuildingType = nullptr;

	return 0;
}

DEFINE_HOOK(0x443CCA, BuildingClass_KickOutUnit_AircraftType_Phobos, 0xA)
{
	GET(BuildingClass*, pFactory, ESI);

	auto const pHouseExt = HouseExt::ExtMap.Find(pFactory->Owner);

	if (pHouseExt->Factory_AircraftType == pFactory)
		pHouseExt->Factory_AircraftType = nullptr;

	return 0;
}
