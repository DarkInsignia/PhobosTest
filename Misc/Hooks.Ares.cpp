#include <BuildingClass.h>
#include <FootClass.h>

#include <Utilities/Macro.h>
#include <Utilities/AresHelper.h>
#include <Utilities/Helpers.Alex.h>

#include <Ext/Sidebar/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/EBolt/Body.h>

// Remember that we still don't fix Ares "issues" a priori. Extensions as well.
// Patches presented here are exceptions rather that the rule. They must be short, concise and correct.
// DO NOT POLLUTE ISSUEs and PRs.

ObjectClass* __fastcall CreateInitialPayload(TechnoTypeClass* type, void*, HouseClass* owner)
{
	// temporarily reset the mutex since it's not part of the design
	const int mutex_old = std::exchange(Unsorted::ScenarioInit, 0);
	const auto instance = type->CreateObject(owner);
	Unsorted::ScenarioInit = mutex_old;
	return instance;
}

void __fastcall LetGo(TemporalClass* pTemporal)
{
	pTemporal->LetGo();
}

bool __stdcall ConvertToType(TechnoClass* pThis, TechnoTypeClass* pToType)
{
	if (const auto pFoot = abstract_cast<FootClass*, true>(pThis))
		return TechnoExt::ConvertToType(pFoot, pToType);

	return false;
}

EBolt* __stdcall CreateEBolt(WeaponTypeClass** pWeaponData)
{
	return EBoltExt::CreateEBolt(*pWeaponData);
}

EBolt* __stdcall CreateEBolt2(WeaponTypeClass* pWeapon)
{
	return EBoltExt::CreateEBolt(pWeapon);
}

void Apply_Ares3_0_Patches()
{
	// Abductor fix:
	Patch::Apply_LJMP(AresHelper::AresBaseAddress + 0x54CDF, AresHelper::AresBaseAddress + 0x54D3C);

	// Amphibious enter fix:
	Patch::Apply_LJMP(AresHelper::AresBaseAddress + 0x17536, AresHelper::AresBaseAddress + 0x1754D);

	// Redirect Ares' getCellSpreadItems to our implementation:
	Patch::Apply_CALL(AresHelper::AresBaseAddress + 0x62267, &Helpers::Alex::getCellSpreadItems);
	Patch::Apply_CALL(AresHelper::AresBaseAddress + 0x528C8, &Helpers::Alex::getCellSpreadItems);
	Patch::Apply_CALL(AresHelper::AresBaseAddress + 0x5273A, &Helpers::Alex::getCellSpreadItems);

	// Redirect Ares's RemoveCameo to our implementation:
	Patch::Apply_LJMP(AresHelper::AresBaseAddress + 0x02BDD0, GET_OFFSET(SidebarExt::AresTabCameo_RemoveCameo));

	// InitialPayload creation:
	Patch::Apply_CALL6(AresHelper::AresBaseAddress + 0x43D5D, &CreateInitialPayload);

	// Replace the TemporalClass::Detach call by LetGo in convert function:
	Patch::Apply_CALL(AresHelper::AresBaseAddress + 0x436DA, &LetGo);

	// SuperClass_Launch_SkipRelatedTags:
	Patch::Apply_LJMP(AresHelper::AresBaseAddress + 0x3207C, AresHelper::AresBaseAddress + 0x320DF);

	// Convert ManagerFix:
	Patch::Apply_CALL(AresHelper::AresBaseAddress + 0x039DAE, &ConvertToType);
	Patch::Apply_CALL(AresHelper::AresBaseAddress + 0x046C6D, &ConvertToType);
	Patch::Apply_CALL(AresHelper::AresBaseAddress + 0x04B397, &ConvertToType);
	Patch::Apply_CALL(AresHelper::AresBaseAddress + 0x04C099, &ConvertToType);

	// EBolt reimpl:
	Patch::Apply_LJMP(AresHelper::AresBaseAddress + 0x550A0, GET_OFFSET(CreateEBolt));
	Patch::Apply_LJMP(AresHelper::AresBaseAddress + 0x550F0, GET_OFFSET(CreateEBolt2));
	Patch::Apply_LJMP(AresHelper::AresBaseAddress + 0x561F0, GET_OFFSET(EBoltExt::_EBolt_Draw_Colors));

	// Unit simple deployer fix:
	Patch::Apply_RAW(AresHelper::AresBaseAddress + 0x4C0C6, { 0x5E }); // pop esi
	Patch::Apply_RAW(AresHelper::AresBaseAddress + 0x4C0C7, { 0x33, 0xC0 }); // xor eax, eax
	Patch::Apply_LJMP(AresHelper::AresBaseAddress + 0x4C0A9, AresHelper::AresBaseAddress + 0x4C0C6);
}

void Apply_Ares3_0p1_Patches()
{
	// Abductor fix:
	// Issue: moving vehicles leave permanent occupation stats on terrain
	// What's done here: Skip Mark_Occupation_Bits cuz pFoot->Remove/Limbo() will do it.
	Patch::Apply_LJMP(AresHelper::AresBaseAddress + 0x5598F, AresHelper::AresBaseAddress + 0x559EC);

	// Amphibious enter fix:
	Patch::Apply_LJMP(AresHelper::AresBaseAddress + 0x17C26, AresHelper::AresBaseAddress + 0x17C3D);

	// Redirect Ares' getCellSpreadItems to our implementation:
	Patch::Apply_CALL(AresHelper::AresBaseAddress + 0x62FB7, &Helpers::Alex::getCellSpreadItems);
	Patch::Apply_CALL(AresHelper::AresBaseAddress + 0x53578, &Helpers::Alex::getCellSpreadItems);
	Patch::Apply_CALL(AresHelper::AresBaseAddress + 0x533EA, &Helpers::Alex::getCellSpreadItems);

	// Redirect Ares's RemoveCameo to our implementation:
	Patch::Apply_LJMP(AresHelper::AresBaseAddress + 0x02C910, GET_OFFSET(SidebarExt::AresTabCameo_RemoveCameo));

	// InitialPayload creation:
	Patch::Apply_CALL6(AresHelper::AresBaseAddress + 0x4483D, &CreateInitialPayload);

	// Replace the TemporalClass::Detach call by LetGo in convert function:
	Patch::Apply_CALL(AresHelper::AresBaseAddress + 0x441BA, &LetGo);

	// SuperClass_Launch_SkipRelatedTags:
	Patch::Apply_LJMP(AresHelper::AresBaseAddress + 0x32A5C, AresHelper::AresBaseAddress + 0x32ABF);

	// Convert ManagerFix:
	Patch::Apply_CALL(AresHelper::AresBaseAddress + 0x3A82E, &ConvertToType);
	Patch::Apply_CALL(AresHelper::AresBaseAddress + 0x4780D, &ConvertToType);
	Patch::Apply_CALL(AresHelper::AresBaseAddress + 0x4BFF7, &ConvertToType);
	Patch::Apply_CALL(AresHelper::AresBaseAddress + 0x4CCF9, &ConvertToType);

	// EBolt reimpl:
	Patch::Apply_LJMP(AresHelper::AresBaseAddress + 0x55D50, GET_OFFSET(CreateEBolt));
	Patch::Apply_LJMP(AresHelper::AresBaseAddress + 0x55DA0, GET_OFFSET(CreateEBolt2));
	Patch::Apply_LJMP(AresHelper::AresBaseAddress + 0x56EA0, GET_OFFSET(EBoltExt::_EBolt_Draw_Colors));

	// Unit simple deployer fix:
	Patch::Apply_RAW(AresHelper::AresBaseAddress + 0x4CD26, { 0x5E }); // pop esi
	Patch::Apply_RAW(AresHelper::AresBaseAddress + 0x4CD27, { 0x33, 0xC0 }); // xor eax, eax
	Patch::Apply_LJMP(AresHelper::AresBaseAddress + 0x4CD09, AresHelper::AresBaseAddress + 0x4CD26);
}
