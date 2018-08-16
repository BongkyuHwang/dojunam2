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

	BWAPI::Unitset detectorUnitTargets;
	if(targets.size()>0)
		std::copy_if(targets.begin(), targets.end(), std::inserter(detectorUnitTargets, detectorUnitTargets.end()), [](BWAPI::Unit u){ return u->isVisible(); });

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
	}

	bool detectorUnitInBattle = false;
	
	// for each detectorUnit
	for (auto & detectorUnit : detectorUnits)
	{
		BWAPI::Unit target = closestCloakedUnit(detectorUnitTargets, detectorUnit);

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
			
		}

		if (detectorUnit->isUnderAttack())
			detectorUnitInBattle = true;

		if (target != nullptr)
		{
			double nearLTD = UnitUtils::getNearByLTD(BWAPI::Broodwar->enemy(), detectorUnit, target->getType().airWeapon().maxRange());
			if (nearLTD >= detectorUnit->getHitPoints())
			{
				//detectorUnit->move(order.getCenterPosition());
				//Micro::SmartMove(detectorUnit, unitClosestToEnemy->getPosition());
				//printf("SmartKiteTarget(detectorUnit \n");
				if (target->isVisible() && !detectorUnit->getType().isBuilding())
					Micro::SmartKiteTarget(detectorUnit, target);
				continue;
			}
		}

		if (unitClosestToEnemy && detectorUnit->canUseTechUnit(BWAPI::TechTypes::Defensive_Matrix, unitClosestToEnemy)
			&& (unitClosestToEnemy->isAttackFrame() || unitClosestToEnemy->isUnderAttack() || detectorUnit->isUnderAttack()))
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
			else if (detectorUnit->canUseTechUnit(BWAPI::TechTypes::Irradiate, target)
				&& BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Zerg
				&& (target->getType() ==  BWAPI::UnitTypes::Zerg_Lurker
				|| target->getType() == BWAPI::UnitTypes::Zerg_Ultralisk
				|| target->getType() == BWAPI::UnitTypes::Zerg_Guardian
				|| target->getType() == BWAPI::UnitTypes::Zerg_Mutalisk
				|| target->getType() == BWAPI::UnitTypes::Zerg_Defiler
				|| target->getType() == BWAPI::UnitTypes::Zerg_Queen				)
				)
			{
				//printf("Use Tech Irradiate \n");
				if (target->isVisible() && !detectorUnit->useTech(BWAPI::TechTypes::Irradiate, target))
				{
					detectorUnit->move(target->getPosition());
				}
				continue;
			}
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