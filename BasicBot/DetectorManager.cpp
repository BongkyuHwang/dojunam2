#include "DetectorManager.h"
using namespace MyBot;
#include "MapTools.h"

DetectorManager::DetectorManager() : unitClosestToEnemy(nullptr) { }

void DetectorManager::executeMicro(const BWAPI::Unitset & targets) 
{
	const BWAPI::Unitset & detectorUnits = getUnits();

	if (detectorUnits.empty())
	{
		return;
	}

	for (size_t i(0); i<targets.size(); ++i)
	{
		// do something here if there's targets
	}

	cloakedUnitMap.clear();
	BWAPI::Unitset cloakedUnits;

	// figure out targets
	for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
	{
		// conditions for targeting
		if (unit->getType() == BWAPI::UnitTypes::Zerg_Lurker ||
			unit->getType() == BWAPI::UnitTypes::Protoss_Dark_Templar ||
			unit->getType() == BWAPI::UnitTypes::Terran_Wraith) 
		{
			cloakedUnits.insert(unit);
			cloakedUnitMap[unit] = false;
		}
	}

	bool detectorUnitInBattle = false;
	
	// for each detectorUnit
	for (auto & detectorUnit : detectorUnits)
	{
		BWAPI::Unit target = closestCloakedUnit(cloakedUnits, detectorUnit);

		if (detectorUnit->isUnderAttack())
			detectorUnitInBattle = true;

		if (target)
		{
			double nearLTD = UnitUtils::getNearByLTD(BWAPI::Broodwar->enemy(), detectorUnit, target->getType().airWeapon().maxRange());
			if (nearLTD >= detectorUnit->getHitPoints())
			{
				detectorUnit->move(order.getCenterPosition());
				//Micro::SmartMove(detectorUnit, unitClosestToEnemy->getPosition());
				continue;
			}
		}

		if (target && target->getDistance(detectorUnit->getPosition()) > target->getType().airWeapon().maxRange() - 32 && target->getPosition().isValid())
		{
			detectorUnit->move(target->getPosition());
			continue;
		}
		else if (target)
		{
			if (detectorUnit->canUseTechUnit(BWAPI::TechTypes::EMP_Shockwave, target) &&
				BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Protoss)
			{
				detectorUnit->useTech(BWAPI::TechTypes::EMP_Shockwave, target);
				continue;
			}
			else if (detectorUnit->canUseTechUnit(BWAPI::TechTypes::Irradiate, target)
				&& BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Zerg)
			{
				detectorUnit->useTech(BWAPI::TechTypes::Irradiate, target);
				continue;
			}
		}
	
		// if we need to regroup, move the detectorUnit to that location
		if (!detectorUnitInBattle && unitClosestToEnemy && unitClosestToEnemy->getPosition().isValid())
		{
			Micro::SmartMove(detectorUnit, unitClosestToEnemy->getPosition());
			detectorUnitInBattle = true;
			continue;
		}
		// otherwise there is no battle or no closest to enemy so we don't want our detectorUnit to die
		// send him to scout around the map
		else
		{
			BWAPI::Position explorePosition = MyBot::MapGrid::Instance().getLeastExplored();
			Micro::SmartMove(detectorUnit, explorePosition);
			continue;
		}

		if (unitClosestToEnemy && detectorUnit->canUseTechUnit(BWAPI::TechTypes::Defensive_Matrix, unitClosestToEnemy) 
			&& (unitClosestToEnemy->isAttackFrame() || unitClosestToEnemy->isUnderAttack() || detectorUnit->isUnderAttack()))
		{
			if (!detectorUnit->useTech(BWAPI::TechTypes::Defensive_Matrix, unitClosestToEnemy))
			{
				detectorUnit->move(order.getClosestUnit()->getPosition());
				continue;
			}
			else
			{
				detectorUnit->useTech(BWAPI::TechTypes::Defensive_Matrix, unitClosestToEnemy);
				continue;
			}
		}
		if (detectorUnitInBattle && detectorUnit->getPosition().getDistance(order.getPosition()) < 30)
		{
			detectorUnit->move(order.getCenterPosition());
			continue;
		}
	}
}


BWAPI::Unit DetectorManager::closestCloakedUnit(const BWAPI::Unitset & cloakedUnits, BWAPI::Unit detectorUnit)
{
	BWAPI::Unit closestCloaked = nullptr;
	double closestDist = 100000;
	double closestDist_normal = 100000;
	BWAPI::Unit closestTarget = nullptr;

	for (auto & unit : cloakedUnits)
	{
		// if we haven't already assigned an detectorUnit to this cloaked unit
		if (!cloakedUnitMap[unit])
		{
			double dist = unit->getDistance(detectorUnit);

			if (!closestCloaked || (dist < closestDist))
			{
				closestCloaked = unit;
				closestDist = dist;
			}
		}
		else
		{
			double dist = unit->getDistance(detectorUnit);

			if (!closestTarget || (dist < closestDist_normal))
			{
				closestTarget = unit;
				closestDist_normal = dist;
			}
		}
	}

	if (closestCloaked == nullptr)
		return closestTarget;

	return closestCloaked;
}