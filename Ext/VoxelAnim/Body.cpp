#include "Body.h"
#include <Ext/VoxelAnimType/Body.h>
#include <New/Entity/LaserTrailClass.h>
#include <Utilities/Macro.h>

VoxelAnimExt::ExtContainer VoxelAnimExt::ExtMap;

void VoxelAnimExt::InitializeLaserTrails(VoxelAnimClass* pThis)
{
	auto* const pThisExt = VoxelAnimExt::ExtMap.Find(pThis);
	if (!pThisExt || !pThis || !pThis->Type)
		return;

	if (!pThisExt->LaserTrails.empty())
		return;

	auto* const pTypeExt = VoxelAnimTypeExt::ExtMap.Find(pThis->Type);
	const auto& typeIdxs = pTypeExt->LaserTrail_Types;
	if (typeIdxs.empty())
		return;

	pThisExt->LaserTrails.reserve(typeIdxs.size());

	// assume Array is a std::vector<unique_ptr<...>> as used elsewhere
	const auto total = static_cast<int>(LaserTrailTypeClass::Array.size());

	for (const int idxTrail : typeIdxs)
	{
		if (idxTrail < 0 || idxTrail >= total)
		{
			Debug::Log("VoxelAnimExt: invalid LaserTrail type index %d on %s\n",
					   idxTrail, pThis->Type->ID);
			continue;
		}

		auto* const ltType = LaserTrailTypeClass::Array[idxTrail].get();
		if (!ltType)
			continue;

		pThisExt->LaserTrails.emplace_back(
			std::make_unique<LaserTrailClass>(ltType, pThis->OwnerHouse));
	}
}

void VoxelAnimExt::ExtData::Initialize() { }

// =============================
// load / save
template <typename T>
void VoxelAnimExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->LaserTrails)
		.Process(this->TrailerSpawnTimer)
		;
}

void VoxelAnimExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<VoxelAnimClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void VoxelAnimExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<VoxelAnimClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool VoxelAnimExt::LoadGlobals(PhobosStreamReader& Stm)
{
	return Stm
		.Success();
}

bool VoxelAnimExt::SaveGlobals(PhobosStreamWriter& Stm)
{
	return Stm
		.Success();
}

// =============================
// container

VoxelAnimExt::ExtContainer::ExtContainer() : Container("VoxelAnimClass") { }
VoxelAnimExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

//DEFINE_HOOK(0x749951, VoxelAnimClass_CTOR, 0xC)
DEFINE_HOOK(0x74942E, VoxelAnimClass_CTOR, 0xC)
{
	GET(VoxelAnimClass*, pItem, ESI);

	VoxelAnimExt::ExtMap.TryAllocate(pItem);
	VoxelAnimExt::InitializeLaserTrails(pItem);

	return 0;
}

DEFINE_HOOK(0x7499F1, VoxelAnimClass_DTOR, 0x5)
{
	GET(VoxelAnimClass*, pItem, ECX);

	VoxelAnimExt::ExtMap.Remove(pItem);

	return 0;
}

DEFINE_HOOK_AGAIN(0x74A970, VoxelAnimClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x74AA10, VoxelAnimClass_SaveLoad_Prefix, 0x8)
{
	GET_STACK(VoxelAnimClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	VoxelAnimExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x74A9FB, VoxelAnimClass_Load_Suffix, 0x7)
{
	VoxelAnimExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x74AA24, VoxelAnimClass_Save_Suffix, 0x5)
{
	VoxelAnimExt::ExtMap.SaveStatic();
	return 0;
}
