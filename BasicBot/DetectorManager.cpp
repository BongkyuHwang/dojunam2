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
		if (target)
		{
			double nearLTD = UnitUtils::getNearByLTD(BWAPI::Broodwar->enemy(), detectorUnit, target->getType().airWeapon().maxRange());
			if (nearLTD >= detectorUnit->getHitPoints())
			{
				detectorUnit->move(order.getPosition());
				continue;
			}
		}

		if (target && target->getDistance(order.getPosition()) > order.getRadius() + target->getType().airWeapon().maxRange() - 32 && target->getPosition().isValid())
		{
			detectorUnit->move(order.getPosition());
			continue;
		}
		else if (target)
		{
			if (detectorUnit->canUseTechUnit(BWAPI::TechTypes::EMP_Shockwave, target) &&
				BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Protoss)
			{
				detectorUnit->useTech(BWAPI::TechTypes::EMP_Shockwave, target);
			}
			else if (detectorUnit->canUseTechUnit(BWAPI::TechTypes::Irradiate, target)
				&& BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Zerg)
			{
				detectorUnit->useTech(BWAPI::TechTypes::Irradiate, target);
			}
		}
	
		// if we need to regroup, move the detectorUnit to that location
		if (!detectorUnitInBattle && unitClosestToEnemy && unitClosestToEnemy->getPosition().isValid())
		{
			Micro::SmartMove(detectorUnit, order.getPosition());
			detectorUnitInBattle = true;
		}
		// otherwise there is no battle or no closest to enemy so we don't want our detectorUnit to die
		// send him to scout around the map
		else
		{
			BWAPI::Position explorePosition = MyBot::MapGrid::Instance().getLeastExplored();
			Micro::SmartMove(detectorUnit, explorePosition);
		}
		if (detectorUnitInBattle && unitClosestToEnemy && detectorUnit->canUseTechUnit(BWAPI::TechTypes::Defensive_Matrix, unitClosestToEnemy) && unitClosestToEnemy->isAttackFrame())
		{
			if (!detectorUnit->useTech(BWAPI::TechTypes::Defensive_Matrix, unitClosestToEnemy))
				detectorUnit->move(order.getClosestUnit()->getPosition());
			else
			{
				detectorUnit->useTech(BWAPI::TechTypes::Defensive_Matrix, unitClosestToEnemy);
			}
		}
		if (detectorUnit->getPosition().getDistance(order.getPosition()) < 30)
		{
			continue;
		}
	}
}


BWAPI::Unit DetectorManager::closestCloakedUnit(const BWAPI::Unitset & cloakedUnits, BWAPI::Unit detectorUnit)
{
	BWAPI::Unit closestCloaked = nullptr;
	double closestDist = 100000;

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
	}

	return closestCloaked;
}