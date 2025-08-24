#include "Body.h"

#include <MessageListClass.h>

#include <Ext/Scenario/Body.h>
#include <Ext/SWType/Body.h>

#include <New/Entity/BannerClass.h>

#include <New/Type/BannerTypeClass.h>

#include <Utilities/SavegameDef.h>
#include <Utilities/SpawnerHelper.h>
#include <Ext/House/Body.h>

//Static init
TActionExt::ExtContainer TActionExt::ExtMap;

// =============================
// load / save

template <typename T>
void TActionExt::ExtData::Serialize(T& Stm)
{
	//Stm;
}

void TActionExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<TActionClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TActionExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<TActionClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool TActionExt::Execute(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject,
	TriggerClass* pTrigger, CellStruct const& location, bool& bHandled)
{
	bHandled = true;

	// Vanilla
	switch (pThis->ActionKind)
	{
	case TriggerAction::PlaySoundEffectRandom:
		return TActionExt::PlayAudioAtRandomWP(pThis, pHouse, pObject, pTrigger, location);
	default:
		break;
	};

	// Phobos
	switch (static_cast<PhobosTriggerAction>(pThis->ActionKind))
	{
	case PhobosTriggerAction::SaveGame:
		return TActionExt::SaveGame(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::EditVariable:
		return TActionExt::EditVariable(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::GenerateRandomNumber:
		return TActionExt::GenerateRandomNumber(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::PrintVariableValue:
		return TActionExt::PrintVariableValue(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::BinaryOperation:
		return TActionExt::BinaryOperation(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::RunSuperWeaponAtLocation:
		return TActionExt::RunSuperWeaponAtLocation(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::RunSuperWeaponAtWaypoint:
		return TActionExt::RunSuperWeaponAtWaypoint(pThis, pHouse, pObject, pTrigger, location);

	case PhobosTriggerAction::ToggleMCVRedeploy:
		return TActionExt::ToggleMCVRedeploy(pThis, pHouse, pObject, pTrigger, location);

	case PhobosTriggerAction::EditAngerNode:
		return TActionExt::EditAngerNode(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::ClearAngerNode:
		return TActionExt::ClearAngerNode(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::SetForceEnemy:
		return TActionExt::SetForceEnemy(pThis, pHouse, pObject, pTrigger, location);

	case PhobosTriggerAction::CreateBannerLocal:
		return TActionExt::CreateBannerLocal(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::CreateBannerGlobal:
		return TActionExt::CreateBannerGlobal(pThis, pHouse, pObject, pTrigger, location);
	case PhobosTriggerAction::DeleteBanner:
		return TActionExt::DeleteBanner(pThis, pHouse, pObject, pTrigger, location);

	default:
		bHandled = false;
		return true;
	}
}

bool TActionExt::PlayAudioAtRandomWP(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	std::vector<int> waypoints;
	waypoints.reserve(ScenarioExt::Global()->Waypoints.size());

	auto const pScen = ScenarioClass::Instance;

	for (auto const pair : ScenarioExt::Global()->Waypoints)
		if (pScen->IsDefinedWaypoint(pair.first))
			waypoints.push_back(pair.first);

	if (waypoints.size() > 0)
	{
		auto const index = pScen->Random.RandomRanged(0, waypoints.size() - 1);
		auto const luckyWP = waypoints[index];
		auto const cell = pScen->GetWaypointCoords(luckyWP);
		auto const coords = CellClass::Cell2Coord(cell);
		VocClass::PlayIndexAtPos(pThis->Value, coords);
	}

	return true;
}

bool TActionExt::SaveGame(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	if (SessionClass::IsSingleplayer() || SpawnerHelper::IsSaveGameEventHooked())
		Phobos::ScheduleGameSave(StringTable::LoadString(pThis->Text));

	return true;
}

bool TActionExt::EditVariable(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	// Variable Index
	// holds by pThis->Value

	// Operations:
	// 0 : set value - operator=
	// 1 : add value - operator+
	// 2 : minus value - operator-
	// 3 : multiply value - operator*
	// 4 : divide value - operator/
	// 5 : mod value - operator%
	// 6 : <<
	// 7 : >>
	// 8 : ~ (no second param being used)
	// 9 : ^
	// 10 : |
	// 11 : &
	// holds by pThis->Param3

	// Params:
	// The second value
	// holds by pThis->Param4

	// Global Variable or Local
	// 0 for local and 1 for global
	// holds by pThis->Param5

	// uses !pThis->Param5 to ensure Param5 is 0 or 1
	auto& variables = ScenarioExt::Global()->Variables[pThis->Param5 != 0];
	auto itr = variables.find(pThis->Value);
	if (itr != variables.end())
	{
		auto& nCurrentValue = itr->second.Value;
		// variable being found
		switch (pThis->Param3)
		{
		case 0: { nCurrentValue = pThis->Param4; break; }
		case 1: { nCurrentValue += pThis->Param4; break; }
		case 2: { nCurrentValue -= pThis->Param4; break; }
		case 3: { nCurrentValue *= pThis->Param4; break; }
		case 4: { nCurrentValue /= pThis->Param4; break; }
		case 5: { nCurrentValue %= pThis->Param4; break; }
		case 6: { nCurrentValue <<= pThis->Param4; break; }
		case 7: { nCurrentValue >>= pThis->Param4; break; }
		case 8: { nCurrentValue = ~nCurrentValue; break; }
		case 9: { nCurrentValue ^= pThis->Param4; break; }
		case 10: { nCurrentValue |= pThis->Param4; break; }
		case 11: { nCurrentValue &= pThis->Param4; break; }
		default:
			return true;
		}

		if (!pThis->Param5)
			TagClass::NotifyLocalChanged(pThis->Value);
		else
			TagClass::NotifyGlobalChanged(pThis->Value);
	}
	return true;
}

bool TActionExt::GenerateRandomNumber(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	auto& variables = ScenarioExt::Global()->Variables[pThis->Param5 != 0];
	auto itr = variables.find(pThis->Value);
	if (itr != variables.end())
	{
		itr->second.Value = ScenarioClass::Instance->Random.RandomRanged(pThis->Param3, pThis->Param4);
		if (!pThis->Param5)
			TagClass::NotifyLocalChanged(pThis->Value);
		else
			TagClass::NotifyGlobalChanged(pThis->Value);
	}

	return true;
}

bool TActionExt::PrintVariableValue(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	auto& variables = ScenarioExt::Global()->Variables[pThis->Param3 != 0];
	auto itr = variables.find(pThis->Value);
	if (itr != variables.end())
	{
		CRT::swprintf(Phobos::wideBuffer, L"%d", itr->second.Value);
		MessageListClass::Instance.PrintMessage(Phobos::wideBuffer);
	}

	return true;
}

bool TActionExt::BinaryOperation(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	auto& variables1 = ScenarioExt::Global()->Variables[pThis->Param5 != 0];
	auto itr1 = variables1.find(pThis->Value);
	auto& variables2 = ScenarioExt::Global()->Variables[pThis->Param6 != 0];
	auto itr2 = variables2.find(pThis->Param4);

	if (itr1 != variables1.end() && itr2 != variables2.end())
	{
		auto& nCurrentValue = itr1->second.Value;
		auto& nOptValue = itr2->second.Value;
		switch (pThis->Param3)
		{
		case 0: { nCurrentValue = nOptValue; break; }
		case 1: { nCurrentValue += nOptValue; break; }
		case 2: { nCurrentValue -= nOptValue; break; }
		case 3: { nCurrentValue *= nOptValue; break; }
		case 4: { nCurrentValue /= nOptValue; break; }
		case 5: { nCurrentValue %= nOptValue; break; }
		case 6: { nCurrentValue <<= nOptValue; break; }
		case 7: { nCurrentValue >>= nOptValue; break; }
		case 8: { nCurrentValue = nOptValue; break; }
		case 9: { nCurrentValue ^= nOptValue; break; }
		case 10: { nCurrentValue |= nOptValue; break; }
		case 11: { nCurrentValue &= nOptValue; break; }
		default:
			return true;
		}

		if (!pThis->Param5)
			TagClass::NotifyLocalChanged(pThis->Value);
		else
			TagClass::NotifyGlobalChanged(pThis->Value);
	}
	return true;
}

bool TActionExt::RunSuperWeaponAtLocation(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	if (!pThis)
		return true;

	TActionExt::RunSuperWeaponAt(pThis, pThis->Param5, pThis->Param6);

	return true;
}

bool TActionExt::RunSuperWeaponAtWaypoint(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	if (!pThis)
		return true;

	auto& waypoints = ScenarioExt::Global()->Waypoints;
	const int nWaypoint = pThis->Param5;

	// Check if is a valid Waypoint
	if (nWaypoint >= 0 && waypoints.find(nWaypoint) != waypoints.end() && waypoints[nWaypoint].X && waypoints[nWaypoint].Y)
	{
		auto const selectedWP = waypoints[nWaypoint];
		TActionExt::RunSuperWeaponAt(pThis, selectedWP.X, selectedWP.Y);
	}

	return true;
}

bool TActionExt::RunSuperWeaponAt(TActionClass* pThis, int X, int Y)
{
	if (SuperWeaponTypeClass::Array.Count > 0)
	{
		HouseClass* pExecuteHouse = nullptr;  // House who will fire the SW.
		std::vector<HouseClass*> housesList;
		CellStruct targetLocation = { (short)X, (short)Y };
		auto& random = ScenarioClass::Instance->Random;

		do
		{
			if (X < 0)
				targetLocation.X = (short)random.RandomRanged(0, MapClass::Instance.MapCoordBounds.Right);

			if (Y < 0)
				targetLocation.Y = (short)random.RandomRanged(0, MapClass::Instance.MapCoordBounds.Bottom);
		}
		while (!MapClass::Instance.IsWithinUsableArea(targetLocation, false));

		switch (pThis->Param4)
		{
		case -1:
			housesList.reserve(HouseClass::Array.Count);

			// Random non-neutral
			for (auto const pHouse : HouseClass::Array)
			{
				if (!pHouse->Defeated
					&& !pHouse->IsObserver()
					&& !pHouse->Type->MultiplayPassive)
				{
					housesList.push_back(pHouse);
				}
			}

			if (housesList.size() > 0)
				pExecuteHouse = housesList[random.RandomRanged(0, housesList.size() - 1)];
			else
				return true;

			break;

		case -2:
			// Find first Neutral
			for (auto const pHouseNeutral : HouseClass::Array)
			{
				if (pHouseNeutral->IsNeutral())
				{
					pExecuteHouse = pHouseNeutral;
					break;
				}
			}

			break;

		case -3:
			housesList.reserve(HouseClass::Array.Count);

			// Random Human Player
			for (auto const pHouse : HouseClass::Array)
			{
				if (pHouse->IsControlledByHuman()
					&& !pHouse->Defeated
					&& !pHouse->IsObserver())
				{
					housesList.push_back(pHouse);
				}
			}

			if (housesList.size() > 0)
				pExecuteHouse = housesList[random.RandomRanged(0, housesList.size() - 1)];
			else
				return true;

			break;

		default:
			if (pThis->Param4 >= 0)
				pExecuteHouse = HouseClass::Index_IsMP(pThis->Param4) ? HouseClass::FindByIndex(pThis->Param4) : HouseClass::FindByCountryIndex(pThis->Param4);
			else
				return true;

			break;
		}

		if (pExecuteHouse)
		{
			auto const pSuper = pExecuteHouse->Supers.Items[pThis->Param3];

			const CDTimerClass old_timer = pSuper->RechargeTimer;
			pSuper->SetReadiness(true);
			pSuper->Launch(targetLocation, false);
			pSuper->Reset();
			pSuper->RechargeTimer = old_timer;
		}
	}

	return true;
}

bool TActionExt::ToggleMCVRedeploy(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	GameModeOptionsClass::Instance.MCVRedeploy = pThis->Param3 != 0;
	return true;
}

bool TActionExt::EditAngerNode(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	if (pHouse->AngerNodes.Count <= 0)
		return true;

	auto setValue = [pThis, pHouse](HouseClass* pTargetHouse)
	{
		if (!pTargetHouse || pHouse == pTargetHouse || pHouse->IsAlliedWith(pTargetHouse))
			return;

		for (auto& pAngerNode : pHouse->AngerNodes)
		{
			if (pAngerNode.House != pTargetHouse)
				continue;

			switch (pThis->Param3)
			{
			case 0: { pAngerNode.AngerLevel = pThis->Param4; break; }
			case 1: { pAngerNode.AngerLevel += pThis->Param4; break; }
			case 2: { pAngerNode.AngerLevel -= pThis->Param4; break; }
			case 3: { pAngerNode.AngerLevel *= pThis->Param4; break; }
			case 4: { pAngerNode.AngerLevel /= pThis->Param4; break; }
			case 5: { pAngerNode.AngerLevel %= pThis->Param4; break; }
			case 6: { pAngerNode.AngerLevel <<= pThis->Param4; break; }
			case 7: { pAngerNode.AngerLevel >>= pThis->Param4; break; }
			case 8: { pAngerNode.AngerLevel = ~pAngerNode.AngerLevel; break; }
			case 9: { pAngerNode.AngerLevel ^= pThis->Param4; break; }
			case 10: { pAngerNode.AngerLevel |= pThis->Param4; break; }
			case 11: { pAngerNode.AngerLevel &= pThis->Param4; break; }
			default:break;
			}

			break;
		}
	};

	const int value = pThis->Value;

	if (value >= 0)
	{
		HouseClass* pTargetHouse = HouseClass::Index_IsMP(value)
			? HouseClass::FindByIndex(value)
			: HouseClass::FindByCountryIndex(value);

		setValue(pTargetHouse);
		pHouse->UpdateAngerNodes(0, pHouse);
	}
	else if (value == -1)
	{
		for (auto const pTargetHouse : HouseClass::Array)
		{
			setValue(pTargetHouse);
		}

		pHouse->UpdateAngerNodes(0, pHouse);
	}

	return true;
}

bool TActionExt::ClearAngerNode(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	if (pHouse->AngerNodes.Count <= 0)
		return true;

	const int value = pThis->Value;

	if (value >= 0)
	{
		const HouseClass* pTargetHouse = HouseClass::Index_IsMP(value)
			? HouseClass::FindByIndex(value)
			: HouseClass::FindByCountryIndex(value);

		if (pTargetHouse)
		{
			for (auto& pAngerNode : pHouse->AngerNodes)
			{
				if (pAngerNode.House != pTargetHouse)
					continue;

				pAngerNode.AngerLevel = 0;
				pHouse->UpdateAngerNodes(0, pHouse);
				break;
			}
		}
	}
	else if (value == -1)
	{
		for (auto& pAngerNode : pHouse->AngerNodes)
		{
			pAngerNode.AngerLevel = 0;
		}

		pHouse->UpdateAngerNodes(0, pHouse);
	}

	return true;
}

bool TActionExt::SetForceEnemy(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	auto const pHouseExt = HouseExt::ExtMap.Find(pHouse);
	const int value = pThis->Param3;

	if (value >= 0 || value == -2)
	{
		if (value != -2)
		{
			const HouseClass* pTargetHouse = HouseClass::Index_IsMP(value)
				? HouseClass::FindByIndex(value)
				: HouseClass::FindByCountryIndex(value);

			if (pTargetHouse
				&& pHouse != pTargetHouse
				&& !pHouse->IsAlliedWith(pTargetHouse))
			{
				pHouseExt->SetForceEnemyIndex(pTargetHouse->GetArrayIndex());
				pHouse->UpdateAngerNodes(0, pHouse);
			}
		}
		else
		{
			pHouseExt->SetForceEnemyIndex(-2);
			pHouse->UpdateAngerNodes(0, pHouse);
		}
	}
	else if (value == -1)
	{
		pHouseExt->SetForceEnemyIndex(-1);
		pHouse->UpdateAngerNodes(0, pHouse);
	}

	return true;
}

static void CreateOrReplaceBanner(TActionClass* pTAction, bool isGlobal)
{
	const auto pBannerType = BannerTypeClass::Find(pTAction->Text);

	if (!pBannerType)
		return;

	auto& banners = BannerClass::Array;

	const auto it = std::find_if(banners.begin(), banners.end(),
		[pTAction](const std::unique_ptr<BannerClass>& pBanner)
		{
			return pBanner->ID == pTAction->Param3;
		});

	if (it != banners.end())
	{
		auto& pBanner = *it;
		pBanner->Type = pBannerType;
		pBanner->Position = { static_cast<int>(pTAction->Param4 / 100.0 * DSurface::ViewBounds.Width), static_cast<int>(pTAction->Param5 / 100.0 * DSurface::ViewBounds.Height) };
		pBanner->Variable = pTAction->Param6;
		pBanner->IsGlobalVariable = isGlobal;
	}
	else
	{
		banners.emplace_back(
			std::make_unique<BannerClass>(pBannerType, pTAction->Param3, Point2D { pTAction->Param4, pTAction->Param5 }, pTAction->Param6, isGlobal)
		);
	}
}

bool TActionExt::CreateBannerLocal(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	CreateOrReplaceBanner(pThis, false);
	return true;
}

bool TActionExt::CreateBannerGlobal(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	CreateOrReplaceBanner(pThis, true);
	return true;
}

bool TActionExt::DeleteBanner(TActionClass* pThis, HouseClass* pHouse, ObjectClass* pObject, TriggerClass* pTrigger, CellStruct const& location)
{
	const auto it = std::find_if(BannerClass::Array.cbegin(), BannerClass::Array.cend(),
		[pThis](const std::unique_ptr<BannerClass>& pBanner)
		{
			return pBanner->ID == pThis->Value;
		});

	if (it != BannerClass::Array.cend())
		BannerClass::Array.erase(it);

	return true;
}


// =============================
// container

TActionExt::ExtContainer::ExtContainer() : Container("TActionClass") { }

TActionExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

#ifdef MAKE_GAME_SLOWER_FOR_NO_REASON
DEFINE_HOOK(0x6DD176, TActionClass_CTOR, 0x5)
{
	GET(TActionClass*, pItem, ESI);

	TActionExt::ExtMap.TryAllocate(pItem);
	return 0;
}

DEFINE_HOOK(0x6E4761, TActionClass_SDDTOR, 0x6)
{
	GET(TActionClass*, pItem, ESI);

	TActionExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x6E3E30, TActionClass_SaveLoad_Prefix, 0x8)
DEFINE_HOOK(0x6E3DB0, TActionClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(TActionClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	TActionExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x6E3E29, TActionClass_Load_Suffix, 0x4)
{
	TActionExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x6E3E4A, TActionClass_Save_Suffix, 0x3)
{
	TActionExt::ExtMap.SaveStatic();
	return 0;
}
#endif
