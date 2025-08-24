#include "Body.h"

#include <Ext/Techno/Body.h>

// Contains ScriptExt::Mission_Move and its helper functions.

void ScriptExt::Mission_Move(TeamClass* pTeam, int calcThreatMode, bool pickAllies, int attackAITargetType, int idxAITargetTypeItem)
{
	bool noWaitLoop = false;
	bool bAircraftsWithoutAmmo = false;
	const auto pTeamData = TeamExt::ExtMap.Find(pTeam);
	auto& waitNoTargetCounter = pTeamData->WaitNoTargetCounter;
	auto& waitNoTargetTimer = pTeamData->WaitNoTargetTimer;
	auto& waitNoTargetAttempts = pTeamData->WaitNoTargetAttempts;

	// When the new target wasn't found it sleeps some few frames before the new attempt. This can save cycles and cycles of unnecessary executed lines.
	if (waitNoTargetCounter > 0)
	{
		if (waitNoTargetTimer.InProgress())
			return;

		waitNoTargetTimer.Stop();
		noWaitLoop = true;
		waitNoTargetCounter = 0;

		if (waitNoTargetAttempts > 0)
			waitNoTargetAttempts--;
	}

	const auto pFirstUnit = pTeam->FirstUnit;

	for (auto pFoot = pFirstUnit; pFoot; pFoot = pFoot->NextTeamMember)
	{
		if (pFoot && pFoot->IsAlive && !pFoot->InLimbo)
		{
			const auto pTechnoType = pFoot->GetTechnoType();

			if (pFoot->WhatAmI() == AbstractType::Aircraft
				&& !pFoot->IsInAir()
				&& static_cast<AircraftTypeClass*>(pTechnoType)->AirportBound
				&& pFoot->Ammo < pTechnoType->Ammo)
			{
				bAircraftsWithoutAmmo = true;
			}
		}
	}

	// Find the Leader
	auto pLeaderUnit = pTeamData->TeamLeader;

	if (!ScriptExt::IsUnitAvailable(pLeaderUnit, true))
	{
		pLeaderUnit = ScriptExt::FindTheTeamLeader(pTeam);
		pTeamData->TeamLeader = pLeaderUnit;
	}

	const auto pScript = pTeam->CurrentScript;
	const auto pScriptType = pScript->Type;
	const auto& scriptActions = pScriptType->ScriptActions;
	const auto currentMission = pScript->CurrentMission;

	auto& idxSelectedObjectFromAIList = pTeamData->IdxSelectedObjectFromAIList;
	auto& closeEnough = pTeamData->CloseEnough;

	if (!pLeaderUnit || bAircraftsWithoutAmmo)
	{
		idxSelectedObjectFromAIList = -1;

		if (closeEnough > 0)
			closeEnough = -1;

		if (waitNoTargetAttempts != 0)
		{
			waitNoTargetTimer.Stop();
			waitNoTargetCounter = 0;
			waitNoTargetAttempts = 0;
		}

		// This action finished
		pTeam->StepCompleted = true;

		const auto& node = scriptActions[currentMission];
		const int nextMission = currentMission + 1;
		const auto& nextNode = scriptActions[nextMission];
		ScriptExt::Log("AI Scripts - Move: [%s] [%s] (line: %d = %d,%d) Jump to next line: %d = %d,%d -> (Reasons: No Leader | Aircrafts without ammo)\n",
			pTeam->Type->ID,
			pScriptType->ID,
			currentMission,
			node.Action,
			node.Argument,
			nextMission,
			nextNode.Action,
			nextNode.Argument);

		return;
	}

	auto& pTeamFocus = pTeam->Focus;
	const auto pFocus = abstract_cast<TechnoClass*>(pTeamFocus);

	if (!pFocus && !bAircraftsWithoutAmmo)
	{
		// This part of the code is used for picking a new target.
		const auto& node = scriptActions[currentMission];
		const int targetMask = node.Argument; // This is the target type
		const auto pSelectedTarget = ScriptExt::FindBestObject(pLeaderUnit, targetMask, calcThreatMode, pickAllies, attackAITargetType, idxAITargetTypeItem);

		if (pSelectedTarget)
		{
			/*const auto pLeaderUnitType = pLeaderUnit->GetTechnoType();
			ScriptExt::Log("AI Scripts - Move: [%s] [%s] (line: %d = %d,%d) Leader [%s] (UID: %lu) selected [%s] (UID: %lu) as destination target.\n",
				pTeam->Type->ID,
				pScriptType->ID,
				currentMission,
				node.Action,
				node.Argument,
				pLeaderUnitType->get_ID(),
				pLeaderUnit->UniqueID,
				pSelectedTarget->GetTechnoType()->get_ID(),
				pSelectedTarget->UniqueID);*/

			pTeamFocus = pSelectedTarget;
			waitNoTargetAttempts = 0; // Disable Script Waits if there are any because a new target was selected
			waitNoTargetTimer.Stop();
			waitNoTargetCounter = 0; // Disable Script Waits if there are any because a new target was selected

			for (auto pFoot = pFirstUnit; pFoot; pFoot = pFoot->NextTeamMember)
			{
				if (!pFoot)
					continue;

				const auto pTechnoType = pFoot->GetTechnoType();

				if (ScriptExt::IsUnitAvailable(pFoot, true))
				{
					if (pTechnoType->Underwater && pTechnoType->LandTargeting == LandTargetingType::Land_Not_OK && pSelectedTarget->GetCell()->LandType != LandType::Water) // Land not OK for the Naval unit
					{
						// Naval units like Submarines are unable to target ground targets except if they have anti-ground weapons. Ignore the attack
						pFoot->SetTarget(nullptr);
						pFoot->SetDestination(nullptr, false);
						pFoot->QueueMission(Mission::Area_Guard, true);

						continue;
					}

					// Reset previous command
					pFoot->SetTarget(nullptr);
					pFoot->SetDestination(nullptr, false);
					pFoot->ForceMission(Mission::Guard);

					// Get a cell near the target
					pFoot->QueueMission(Mission::Move, false);
					CoordStruct coord = TechnoExt::PassengerKickOutLocation(pSelectedTarget, pFoot, 10);
					coord = coord != CoordStruct::Empty ? coord : pSelectedTarget->Location;
					CellClass* pCellDestination = MapClass::Instance.TryGetCellAt(coord);
					pFoot->SetDestination(pCellDestination, true);

					// Aircraft hack. I hate how this game auto-manages the aircraft missions.
					if (pFoot->WhatAmI() == AbstractType::Aircraft && pFoot->Ammo > 0 && !pFoot->IsInAir())
						pFoot->QueueMission(Mission::Move, false);
				}
			}
		}
		else
		{
			// No target was found with the specific criteria.

			if (!noWaitLoop && waitNoTargetTimer.Completed())
			{
				waitNoTargetCounter = 30;
				waitNoTargetTimer.Start(30);
			}

			if (idxSelectedObjectFromAIList >= 0)
				idxSelectedObjectFromAIList = -1;

			if (waitNoTargetAttempts != 0 && waitNoTargetTimer.Completed())
			{
				waitNoTargetCounter = 30;
				waitNoTargetTimer.Start(30); // No target? let's wait some frames

				return;
			}

			if (closeEnough >= 0)
				closeEnough = -1;

			// This action finished
			pTeam->StepCompleted = true;

			const int nextMission = currentMission + 1;
			const auto& nextNode = scriptActions[nextMission];
			ScriptExt::Log("AI Scripts - Move: [%s] [%s] (line: %d = %d,%d) Jump to next line: %d = %d,%d (new target NOT FOUND)\n",
				pTeam->Type->ID,
				pScriptType->ID,
				currentMission,
				node.Action,
				node.Argument,
				nextMission,
				nextNode.Action,
				nextNode.Argument);

			return;
		}
	}
	else
	{
		auto& moveMissionEndMode = pTeamData->MoveMissionEndMode;

		// This part of the code is used for updating the "Move" mission in each team unit
		if (ScriptExt::MoveMissionEndStatus(pTeam, pFocus, pLeaderUnit, moveMissionEndMode))
		{
			moveMissionEndMode = 0;
			idxSelectedObjectFromAIList = -1;

			if (closeEnough >= 0)
				closeEnough = -1;

			// This action finished
			pTeam->StepCompleted = true;

			/*const auto& node = pScriptType->ScriptActions[pScript->CurrentMission];
			const int nextMission = pScript->CurrentMission + 1;
			const auto& nextNode = pScriptType->ScriptActions[nextMission];
			ScriptExt::Log("AI Scripts - Move: [%s] [%s] (line: %d = %d,%d) Jump to next line: %d = %d,%d (Reason: Reached destination)\n",
				pTeam->Type->ID,
				pScriptType->ID,
				currentMission,
				node.Action,
				node.Argument,
				nextMission,
				nextNode.Action,
				nextNode.Argument);*/

			return;
		}
	}
}

TechnoClass* ScriptExt::FindBestObject(TechnoClass* pTechno, int method, int calcThreatMode, bool pickAllies, int attackAITargetType, int idxAITargetTypeItem)
{
	TechnoClass* pBestObject = nullptr;
	double bestVal = -1;
	HouseClass* pEnemyHouse = nullptr;
	const auto pTechnoType = pTechno->GetTechnoType();
	const auto pTechnoOwner = pTechno->Owner;

	// Favorite Enemy House case. If set, AI will focus against that House
	if (!pickAllies && pTechno->BelongsToATeam())
	{
		if (const auto pFoot = abstract_cast<FootClass*>(pTechno))
		{
			const auto pTeam = pFoot->Team;
			const int enemyHouseIndex = pTeam->FirstUnit->Owner->EnemyHouseIndex;

			if (pTeam->Type->OnlyTargetHouseEnemy && enemyHouseIndex >= 0)
				pEnemyHouse = HouseClass::Array.GetItem(enemyHouseIndex);
		}
	}

	// Generic method for targeting
	for (int i = 0; i < TechnoClass::Array.Count; i++)
	{
		const auto pTarget = TechnoClass::Array.GetItem(i);
		const auto pTargetOwner = pTarget->Owner;

		if (pickAllies != pTechnoOwner->IsAlliedWith(pTargetOwner))
			continue;

		if (pEnemyHouse && pEnemyHouse != pTargetOwner)
			continue;

		// Exclude most of invalid target first
		if (!ScriptExt::EvaluateObjectWithMask(pTarget, method, attackAITargetType, idxAITargetTypeItem, pTechno))
			continue;

		const auto pTargetType = pTarget->GetTechnoType();

		// Discard invisible structures
		const auto pTargetBuildingType = abstract_cast<BuildingTypeClass*, true>(pTargetType);

		if (pTargetBuildingType && pTargetBuildingType->InvisibleInGame)
			continue;

		if (pTarget == pTechno)
			continue;

		const auto cloakState = pTarget->CloakState;

		if (pTargetType->Naval)
		{
			// Submarines aren't a valid target
			if (cloakState == CloakState::Cloaked
				&& pTargetType->Underwater)
			{
				const auto navalTargeting = pTechnoType->NavalTargeting;

				if (navalTargeting == NavalTargetingType::Underwater_Never
					|| navalTargeting == NavalTargetingType::Naval_None)
				{
					continue;
				}
			}

			// Land not OK for the Naval unit
			if (pTechnoType->LandTargeting == LandTargetingType::Land_Not_OK
				&& (pTarget->GetCell()->LandType != LandType::Water))
			{
				continue;
			}
		}

		// Stealth check.
		if (cloakState == CloakState::Cloaked)
		{
			const auto pCell = pTarget->GetCell();

			if (!pCell->Sensors_InclHouse(pTechnoOwner->ArrayIndex))
				continue;
		}

		if (!ScriptExt::IsUnitAvailable(pTarget, true))
			continue;

		double value = 0;
		bool isGoodTarget = false;

		switch (calcThreatMode)
		{
		case 0:
		case 1:
		{
			// Threat affected by distance
			double threatMultiplier = 128.0;
			double objectThreatValue = pTargetType->ThreatPosed;

			if (pTargetType->SpecialThreatValue > 0)
			{
				double const& TargetSpecialThreatCoefficientDefault = RulesClass::Instance->TargetSpecialThreatCoefficientDefault;
				objectThreatValue += pTargetType->SpecialThreatValue * TargetSpecialThreatCoefficientDefault;
			}

			// Is Defender house targeting Attacker House? if "yes" then more Threat
			if (pTechnoOwner == HouseClass::Array.GetItem(pTargetOwner->EnemyHouseIndex))
			{
				double const& EnemyHouseThreatBonus = RulesClass::Instance->EnemyHouseThreatBonus;
				objectThreatValue += EnemyHouseThreatBonus;
			}

			// Extra threat based on current health. More damaged == More threat (almost destroyed objects gets more priority)
			objectThreatValue += pTarget->Health * (1 - pTarget->GetHealthPercentage());
			value = (objectThreatValue * threatMultiplier) / ((pTechno->DistanceFrom(pTarget) / (double)Unsorted::LeptonsPerCell) + 1.0);

			if (calcThreatMode == 0)
			{
				// Is this object very FAR? then LESS THREAT against pTechno.
				// More CLOSER? MORE THREAT for pTechno.
				if (value > bestVal || bestVal < 0)
					isGoodTarget = true;
			}
			else
			{
				// Is this object very FAR? then MORE THREAT against pTechno.
				// More CLOSER? LESS THREAT for pTechno.
				if (value < bestVal || bestVal < 0)
					isGoodTarget = true;
			}

			break;
		}
		case 2:
		case 3:
		{
			// Selection affected by distance
			value = pTechno->DistanceFrom(pTarget); // Note: distance is in leptons (*256)

			if (calcThreatMode == 2)
			{
				// Is this object very FAR? then LESS THREAT against pTechno.
				// More CLOSER? MORE THREAT for pTechno.
				if (value < bestVal || bestVal < 0)
					isGoodTarget = true;
			}
			else
			{
				// Is this object very FAR? then MORE THREAT against pTechno.
				// More CLOSER? LESS THREAT for pTechno.
				if (value > bestVal || bestVal < 0)
					isGoodTarget = true;
			}

			break;
		}
		default:
		{
			break;
		}
		}

		if (isGoodTarget)
		{
			pBestObject = pTarget;
			bestVal = value;
		}
	}

	return pBestObject;
}

void ScriptExt::Mission_Move_List(TeamClass* pTeam, int calcThreatMode, bool pickAllies, int attackAITargetType)
{
	const auto pTeamData = TeamExt::ExtMap.Find(pTeam);
	pTeamData->IdxSelectedObjectFromAIList = -1;

	if (attackAITargetType < 0)
	{
		const auto pScript = pTeam->CurrentScript;
		attackAITargetType = pScript->Type->ScriptActions[pScript->CurrentMission].Argument;
	}

	if (RulesExt::Global()->AITargetTypesLists.size() > 0
		&& RulesExt::Global()->AITargetTypesLists[attackAITargetType].size() > 0)
	{
		ScriptExt::Mission_Move(pTeam, calcThreatMode, pickAllies, attackAITargetType, -1);
	}
}

void ScriptExt::Mission_Move_List1Random(TeamClass* pTeam, int calcThreatMode, bool pickAllies, int attackAITargetType, int idxAITargetTypeItem)
{
	bool selected = false;
	int idxSelectedObject = -1;
	std::vector<int> validIndexes;
	const auto pTeamData = TeamExt::ExtMap.Find(pTeam);

	if (pTeamData->IdxSelectedObjectFromAIList >= 0)
	{
		idxSelectedObject = pTeamData->IdxSelectedObjectFromAIList;
		selected = true;
	}

	const auto pScript = pTeam->CurrentScript;
	const auto pScriptType = pScript->Type;

	if (attackAITargetType < 0)
		attackAITargetType = pScriptType->ScriptActions[pScript->CurrentMission].Argument;

	if (attackAITargetType >= 0
		&& (size_t)attackAITargetType < RulesExt::Global()->AITargetTypesLists.size())
	{
		const auto& objectsList = RulesExt::Global()->AITargetTypesLists[attackAITargetType];

		// Still no random target selected
		if (idxSelectedObject < 0 && objectsList.size() > 0 && !selected)
		{
			const auto pFirstUnitOwner = pTeam->FirstUnit->Owner;
			validIndexes.reserve(TechnoClass::Array.Count * objectsList.size());

			// Finding the objects from the list that actually exists in the map
			for (int i = 0; i < TechnoClass::Array.Count; i++)
			{
				const auto pTechno = TechnoClass::Array.GetItem(i);
				const auto pTechnoType = pTechno->GetTechnoType();
				bool found = false;

				for (auto j = 0u; j < objectsList.size() && !found; j++)
				{
					if (pTechnoType == objectsList[j]
						&& ScriptExt::IsUnitAvailable(pTechno, true)
						&& pickAllies == pFirstUnitOwner->IsAlliedWith(pTechno->Owner))
					{
						validIndexes.push_back(j);
						found = true;
					}
				}
			}

			if (validIndexes.size() > 0)
			{
				idxSelectedObject = validIndexes[ScenarioClass::Instance->Random.RandomRanged(0, validIndexes.size() - 1)];
				selected = true;

				/*const auto& node = pScriptType->ScriptActions[pScript->CurrentMission];
				ScriptExt::Log("AI Scripts - Move: [%s] [%s] (line: %d = %d,%d) Picked a random Techno from the list index [AITargetTypes][%d][%d] = %s\n",
					pTeam->Type->ID,
					pScriptType->ID,
					pScript->CurrentMission,
					node.Action,
					node.Argument,
					attackAITargetType,
					idxSelectedObject,
					objectsList[idxSelectedObject]->ID);*/
			}
		}

		if (selected)
			pTeamData->IdxSelectedObjectFromAIList = idxSelectedObject;

		ScriptExt::Mission_Move(pTeam, calcThreatMode, pickAllies, attackAITargetType, idxSelectedObject);
	}

	// This action finished
	if (!selected)
	{
		pTeam->StepCompleted = true;

		const auto& node = pScriptType->ScriptActions[pScript->CurrentMission];
		ScriptExt::Log("AI Scripts - Move: [%s] [%s] (line: %d = %d,%d) Failed to pick a random Techno from the list index [AITargetTypes][%d]! Valid Technos in the list: %d\n",
			pTeam->Type->ID,
			pScriptType->ID,
			pScript->CurrentMission,
			node.Action,
			node.Argument,
			attackAITargetType,
			validIndexes.size());
	}
}
