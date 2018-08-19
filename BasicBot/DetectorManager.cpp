#include "DetectorManager.h"
using namespace MyBot;
#include "MapTools.h"

DetectorManager::DetectorManager() : unitClosestToEnemy(nullptr) { }

void DetectorManager::calWayPosition()
{
	_waypoints.clear();
	//printf("calWayPosition \n");
	for (int tx = 1; tx < BWAPI::Broodwar->mapWidth(); tx += 17)
	{
		if (tx%2 == 0)
			for (int ty = 1; ty < BWAPI::Broodwar->mapHeight(); ty += 15)
			{
				if (BWAPI::TilePosition(tx, ty).isValid())
					_waypoints.push_back(BWAPI::Position(BWAPI::TilePosition(tx, ty)));
			}
		else
		{
			for (int ty = BWAPI::Broodwar->mapHeight(); ty > 1; ty -= 15)
			{
				if (BWAPI::TilePosition(tx, ty).isValid())
					_waypoints.push_back(BWAPI::Position(BWAPI::TilePosition(tx, ty)));
			}
		}
	}
}

BWAPI::Position DetectorManager::doFullscan()
{
	//printf("doFullscan \n");
	if (_waypoints.size() <= 0)
	{
		calWayPosition();
		return _waypoints[_waypoints.size() - 1];
	}
	else
	{
		BWAPI::Position gogo = _waypoints[_waypoints.size() - 1];
		_waypoints.pop_back();
		return gogo;
	}
}

void DetectorManager::executeMicro(const BWAPI::Unitset & targets) 
{
	const BWAPI::Unitset & detectorUnits = getUnits();

	if (detectorUnits.empty())
	{
		return;
	}

	BWAPI::Unitset detectorUnitTargets;
	if(targets.size()>0)
		std::copy_if(targets.begin(), targets.end(), std::inserter(detectorUnitTargets, detectorUnitTargets.end()), [](BWAPI::Unit u){ return u->isVisible(); });
	BWAPI::Unitset candidateIrradiating;
	cloakedUnitMap.clear();
	BWAPI::Unitset cloakedUnits;

	// figure out targets
	for (auto & unit : detectorUnitTargets)
	{
		// conditions for targeting
		if (unit->getType() == BWAPI::UnitTypes::Zerg_Lurker ||
			unit->getType() == BWAPI::UnitTypes::Protoss_Dark_Templar ||
			unit->getType() == BWAPI::UnitTypes::Terran_Wraith) 
		{
			cloakedUnits.insert(unit);
			cloakedUnitMap[unit] = false;
		}

		if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Zerg
			&& (unit->getType() == BWAPI::UnitTypes::Zerg_Lurker
			|| unit->getType() == BWAPI::UnitTypes::Zerg_Ultralisk
			|| unit->getType() == BWAPI::UnitTypes::Zerg_Guardian
			|| unit->getType() == BWAPI::UnitTypes::Zerg_Mutalisk
			|| unit->getType() == BWAPI::UnitTypes::Zerg_Defiler
			|| unit->getType() == BWAPI::UnitTypes::Zerg_Queen
			))
		{
			if (!unit->isIrradiated() < candidateIrradiating.size() < detectorUnits.size())
				candidateIrradiating.insert(unit);
		}

	}

	bool detectorUnitInBattle = false;
	
	// for each detectorUnit
	for (auto & detectorUnit : detectorUnits)
	{
		
		BWAPI::Unit target = closestCloakedUnit(detectorUnitTargets, detectorUnit);
		if (BWAPI::Broodwar->getFrameCount() > 25000 && (target == nullptr || detectorUnitTargets.size() <= 0))
		{
			if (toGo.isValid())
			{
				if (detectorUnit->getPosition().getDistance(toGo) >= 70)
				{
				
					detectorUnit->move(toGo);
				}
				else
				{
					toGo = doFullscan();
					detectorUnit->move(toGo);
				
				}
			}
			else
			{
				toGo = doFullscan();
		
			}
			continue;
		}

		if (target != nullptr && detectorUnit->getType().isBuilding() )
		{
			if (target->getPosition().getDistance(detectorUnit->getPosition()) - detectorUnit->getType().width() - 30 < BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange())
			{
				if (order.getCenterPosition().isValid())
				{
					detectorUnit->move(order.getCenterPosition());
				}
				else
				{
					detectorUnit->move(BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation()));
				}
			}
			else
			{
				detectorUnit->move(target->getPosition());
			}
			continue;
		}

		if (detectorUnit->isUnderAttack())
			detectorUnitInBattle = true;

		if (target != nullptr)
		{
			double nearLTD = UnitUtils::getNearByLTD(BWAPI::Broodwar->enemy(), detectorUnit, target->getType().airWeapon().maxRange());
			if (nearLTD >= detectorUnit->getHitPoints())
			{
				if (target->isVisible() && !detectorUnit->getType().isBuilding())
					Micro::SmartKiteTarget(detectorUnit, target);
				continue;
			}
		}

		if (unitClosestToEnemy && detectorUnit->canUseTechUnit(BWAPI::TechTypes::Defensive_Matrix, unitClosestToEnemy)
			&& (unitClosestToEnemy->isAttackFrame() || unitClosestToEnemy->isUnderAttack() || detectorUnit->isUnderAttack())
			&& BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Terran	)
		{
			//printf("Use Tech Defensive_Matrix \n");
			if (!detectorUnit->useTech(BWAPI::TechTypes::Defensive_Matrix, unitClosestToEnemy))
			{
				detectorUnit->move(order.getClosestUnit()->getPosition());
				//printf("go ClosestUnit UnitID[%d] \n", detectorUnit->getID());
				continue;
			}
			else
			{
				detectorUnit->useTech(BWAPI::TechTypes::Defensive_Matrix, unitClosestToEnemy);
				continue;
			}
		}

		if (target!=nullptr)
		{
			if (detectorUnit->canUseTechUnit(BWAPI::TechTypes::EMP_Shockwave, target) &&
				BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Protoss)
			{
				if (target->isVisible() && !detectorUnit->useTech(BWAPI::TechTypes::EMP_Shockwave, target))
				{
					detectorUnit->move(target->getPosition());
				}
				continue;
			}
			bool useTech = false;
			for (auto &itarget : candidateIrradiating)
			{
				if (!itarget->isIrradiated() && itarget->exists()
				&& BWAPI::TechTypes::Irradiate.getWeapon().maxRange() <= detectorUnit->getDistance(itarget->getPosition()))
				{
					detectorUnit->useTech(BWAPI::TechTypes::Irradiate, itarget);
					useTech = true;
					candidateIrradiating.erase(itarget);
					break;
				}
			}
			if (useTech)
				continue;
		}
		else if (target && target->getDistance(detectorUnit->getPosition()) > target->getType().airWeapon().maxRange() - 32
			&& target->getPosition().isValid()
			&& UnitUtils::GetWeapon(target, detectorUnit) != BWAPI::WeaponTypes::None
			)
		{

			Micro::SmartKiteTarget(detectorUnit, target);
			continue;
		}
	
		// if we need to regroup, move the detectorUnit to that location
		if (unitClosestToEnemy && unitClosestToEnemy->getPosition().isValid())
		{
			Micro::SmartMove(detectorUnit, unitClosestToEnemy->getPosition());
			//printf("unitClosestToEnemy UnitID[%d] \n", detectorUnit->getID());
			detectorUnitInBattle = true;
			continue;
		}
		// otherwise there is no battle or no closest to enemy so we don't want our detectorUnit to die
		// send him to scout around the map
		else
		{
			BWAPI::Position explorePosition = MyBot::MapGrid::Instance().getLeastExplored();
			if (explorePosition.isValid())
			{

				Micro::SmartMove(detectorUnit, explorePosition);
				//printf("explorePosition UnitID[%d] \n", detectorUnit->getID());
				continue;
			}
		}
		//printf("getCenterPosition UnitID[%d] \n", detectorUnit->getID());
		detectorUnit->move(order.getCenterPosition());
		
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
		//if (!cloakedUnitMap[unit])
		//{
		//	double dist = unit->getDistance(detectorUnit);
		//
		//	if (!closestCloaked || (dist < closestDist))
		//	{
		//		closestCloaked = unit;
		//		closestDist = dist;
		//	}
		//}
		//else
		{
			double dist = unit->getDistance(detectorUnit);

			if (!closestTarget || (dist < closestDist_normal))
			{
				closestTarget = unit;
				closestDist_normal = dist;
			}
		}
	}

	//if (closestCloaked == nullptr)
	return closestTarget;

	//return closestCloaked;
}