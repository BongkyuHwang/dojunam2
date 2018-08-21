#include "RangedManager.h"
#include "CommandUtil.h"

using namespace MyBot;

RangedManager::RangedManager()
{
	bunkerNum = 0; marinInBunkerNum = 0;
}

RangedManager & RangedManager::Instance()
{
	static RangedManager instance;
	return instance;
}

void RangedManager::executeMicro(const BWAPI::Unitset & targets)
{
	//checkBunkerNum();
	assignTargetsOld(targets);
}

void RangedManager::checkBunkerNum()
{
	bunkerNum = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Bunker);
	if (bunkerNum > 0)
	{
		bunkerUnit = UnitUtils::GetClosestUnitTypeToTarget(BWAPI::UnitTypes::Terran_Bunker, BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation()));
	}
}

void RangedManager::assignTargetsOld(const BWAPI::Unitset & targets)
{
	const BWAPI::Unitset & rangedUnits = getUnits();
	
	// figure out targets
	BWAPI::Unitset rangedUnitTargets;
	if (targets.size() > 0)
		std::copy_if(targets.begin(), targets.end(), std::inserter(rangedUnitTargets, rangedUnitTargets.end()), [](BWAPI::Unit u){ return u->isVisible(); });
	
	for (auto & rangedUnit : rangedUnits)
	{
		//bool goHome = false;
		//BWAPI::Position keepOnPosition = (order.getPosition() + order.getCenterPosition())/2;
		//if (keepOnPosition.getDistance(rangedUnit->getPosition()) > BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange() )
		//	goHome = true;
		//if (order.getType() == SquadOrderTypes::Defend || order.getType() == SquadOrderTypes::Drop)
		//	goHome = false;
		
		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend || order.getType() == SquadOrderTypes::Idle)
		{
			// if there are targets
			if (!rangedUnitTargets.empty())
			{
				// find the best target for this zealot
				BWAPI::Unit target = getTarget(rangedUnit, rangedUnitTargets);

				if (target->getType() == BWAPI::UnitTypes::Zerg_Lurker 
					&& target->isBurrowed()
					&& rangedUnit->getType() == BWAPI::UnitTypes::Terran_Marine
					&& target->getDistance(rangedUnit->getPosition()) < BWAPI::UnitTypes::Zerg_Lurker.groundWeapon().maxRange()+36 )
				{
					BWAPI::Position fleePosition(rangedUnit->getPosition() - target->getPosition() + rangedUnit->getPosition());
					if (fleePosition.isValid())
						Micro::SmartMove(rangedUnit, fleePosition);
					else
						Micro::SmartMove(rangedUnit, InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition());
					continue;
				}

				if (rangedUnit->getType() == BWAPI::UnitTypes::Terran_Battlecruiser)
				{
					if (BWAPI::Broodwar->self()->hasResearched(BWAPI::TechTypes::Yamato_Gun) && rangedUnit->canUseTech(BWAPI::TechTypes::Yamato_Gun)){
						if (rangedUnit->canUseTechUnit(BWAPI::TechTypes::Yamato_Gun, target) && target->getHitPoints() >= BWAPI::TechTypes::Yamato_Gun.getWeapon().damageAmount() )
						{
							rangedUnit->useTech(BWAPI::TechTypes::Yamato_Gun, target);
							continue;
						}
					}
				}

				if (rangedUnit->getStimTimer() == 0
					&& rangedUnit->getType() == BWAPI::UnitTypes::Terran_Marine
					&& rangedUnit->getHitPoints() == rangedUnit->getType().maxHitPoints()
					&& //@도주남 김지훈 상대가 공격 범위에 들어오면  스팀팩을 사용한다.
					target->getPosition().getDistance(rangedUnit->getPosition()) < rangedUnit->getType().groundWeapon().maxRange() + 32)
				{
					rangedUnit->useTech(BWAPI::TechTypes::Stim_Packs);
				}

				//방어부대가 아니면서 부대의 중심이 유효 할때
				if (order.getCenterPosition().isValid() && order.getType() != SquadOrderTypes::Defend && rangedUnit->getStimTimer()==0)
				{
					// target이 탱크 사정거리 밖에 있고 공격 받고 있지 않으면
					if (!rangedUnit->isUnderAttack())
					{
						// 부대의 중심으로 이동한다.
						if (target->getPosition().getDistance(order.getCenterPosition()) > BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().minRange())
							Micro::SmartAttackMove(rangedUnit, order.getCenterPosition());
						else if (target->getPosition().getDistance(order.getCenterPosition()) 
					> BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange() + rangedUnit->getType().groundWeapon().maxRange())
							Micro::SmartMove(rangedUnit, order.getCenterPosition());

						BWAPI::Broodwar->drawLineMap(rangedUnit->getPosition(), order.getCenterPosition() , BWAPI::Colors::Yellow);
						continue;
					}					
				}			

				// attack it
				Micro::SmartKiteTarget(rangedUnit, target);
			}
			// if there are no targets
			else
			{
				//if (order.getCenterPosition().isValid())
				//{
				//	Micro::SmartAttackMove(rangedUnit, order.getCenterPosition());
				//}
				//// if we're not near the order position
				//else
				//{
				//	BWAPI::Position fleeVec(InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition() - order.getPosition());
				//	double fleeAngle = atan2(fleeVec.y, fleeVec.x);
				//	int dist = order.getRadius() - BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().minRange();
				//	if (dist <= 0)
				//		dist = order.getRadius() *0.8;
				//	fleeVec = BWAPI::Position(static_cast<int>(dist * cos(fleeAngle)), static_cast<int>(dist * sin(fleeAngle)));
				//	fleeVec -= order.getPosition();
				//
				//	if (fleeVec.isValid())
				//		Micro::SmartAttackMove(rangedUnit, fleeVec);
				//}
				BWAPI::Position fleeVec(order.getPosition() - InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition());
				double fleeAngle = atan2(fleeVec.y, fleeVec.x);
				int dist = -BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange()*0.75;
				fleeVec = BWAPI::Position(static_cast<int>(dist * cos(fleeAngle)), static_cast<int>(dist * sin(fleeAngle)));
				BWAPI::Position newDest = order.getPosition() + fleeVec;
				if (newDest.getDistance(rangedUnit->getPosition()) > BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange()*0.8)
				{
					Micro::SmartAttackMove(rangedUnit, newDest);
				}
			}
		}
		else if (order.getType() == SquadOrderTypes::Drop)
		{
			if (BWTA::getRegion(BWAPI::TilePosition(rangedUnit->getPosition()))
				== BWTA::getRegion(InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy())->getTilePosition())
				|| BWTA::getRegion(BWAPI::TilePosition(rangedUnit->getPosition()))
				!= BWTA::getRegion(InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getTilePosition()))
			{
				Micro::SmartAttackMove(rangedUnit, InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy())->getPosition());
			}
			else
			{
				if (rangedUnit->getDistance(order.getPosition()) > order.getRadius())
				{
					// move to it
					Micro::SmartAttackMove(rangedUnit, order.getPosition());
				}
			}
		}
	}
}

std::pair<BWAPI::Unit, BWAPI::Unit> RangedManager::findClosestUnitPair(const BWAPI::Unitset & attackers, const BWAPI::Unitset & targets)
{
	std::pair<BWAPI::Unit, BWAPI::Unit> closestPair(nullptr, nullptr);
	double closestDistance = std::numeric_limits<double>::max();

	for (auto & attacker : attackers)
	{
		BWAPI::Unit target = getTarget(attacker, targets);
		double dist = attacker->getDistance(attacker);

		if (!closestPair.first || (dist < closestDistance))
		{
			closestPair.first = attacker;
			closestPair.second = target;
			closestDistance = dist;
		}
	}

	return closestPair;
}

// get a target for the zealot to attack
BWAPI::Unit RangedManager::getTarget(BWAPI::Unit rangedUnit, const BWAPI::Unitset & targets)
{
	int bestPriorityDistance = 1000000;
	int bestPriority = 0;

	double bestLTD = 0;

	int highPriority = 0;
	double closestDist = std::numeric_limits<double>::infinity();
	BWAPI::Unit closestTarget = nullptr;

	for (const auto & target : targets)
	{
		double distance = rangedUnit->getDistance(target);
		double LTD = UnitUtils::CalculateLTD(target, rangedUnit);
		int priority = getAttackPriority(rangedUnit, target);
		bool targetIsThreat = LTD > 0;
		double t_LTD = UnitUtils::CalculateLTD(rangedUnit, target); // 한방에 죽일 수 있을 경우를 체크 하기 위한 로직
		bool CK = target->getHitPoints() <= t_LTD;

		if (!closestTarget || (priority > highPriority) || (priority == highPriority && distance < closestDist) 
			|| (priority >= highPriority && CK))
		{
			closestDist = distance;
			highPriority = priority;
			closestTarget = target;
		}
	}

	return closestTarget;
}

// get the attack priority of a type in relation to a zergling
int RangedManager::getAttackPriority(BWAPI::Unit rangedUnit, BWAPI::Unit target)
{
	BWAPI::UnitType rangedType = rangedUnit->getType();
	BWAPI::UnitType targetType = target->getType();


	if (rangedUnit->getType() == BWAPI::UnitTypes::Zerg_Scourge)
	{
		if (target->getType() == BWAPI::UnitTypes::Protoss_Carrier)
		{

			return 100;
		}

		if (target->getType() == BWAPI::UnitTypes::Protoss_Corsair)
		{
			return 90;
		}
	}

	bool isThreat = rangedType.isFlyer() ? targetType.airWeapon() != BWAPI::WeaponTypes::None : targetType.groundWeapon() != BWAPI::WeaponTypes::None;

	if (target->getType().isWorker())
	{
		isThreat = false;
	}

	if (target->getType() == BWAPI::UnitTypes::Zerg_Larva || target->getType() == BWAPI::UnitTypes::Zerg_Egg)
	{
		return 0;
	}

	if (target->getType() == BWAPI::UnitTypes::Protoss_Carrier && rangedUnit->canAttack(target))
	{
		return 101;
	}

	if (rangedType == BWAPI::UnitTypes::Terran_Goliath && target->isFlying())
	{
		return 90;
	}

	// if the target is building something near our base something is fishy
	BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
	if (target->getType().isWorker() && (target->isConstructing() || target->isRepairing()) && target->getDistance(ourBasePosition) < 1200)
	{
		return 100;
	}

	if (target->getType().isBuilding() && (target->isCompleted() || target->isBeingConstructed()) && target->getDistance(ourBasePosition) < 1200)
	{
		return 90;
	}

	// highest priority is something that can attack us or aid in combat
	if (targetType == BWAPI::UnitTypes::Terran_Bunker || isThreat)
	{
		return 11;
	}
	// next priority is worker
	else if (targetType.isWorker())
	{
		if (rangedUnit->getType() == BWAPI::UnitTypes::Terran_Vulture)
		{
			return 11;
		}

		return 11;
	}
	// next is special buildings
	else if (targetType == BWAPI::UnitTypes::Zerg_Spawning_Pool)
	{
		return 5;
	}
	// next is special buildings
	else if (targetType == BWAPI::UnitTypes::Protoss_Pylon)
	{
		return 5;
	}
	// next is buildings that cost gas
	else if (targetType.gasPrice() > 0)
	{
		return 4;
	}
	else if (targetType.mineralPrice() > 0)
	{
		return 3;
	}
	// then everything else
	else
	{
		return 1;
	}
}

BWAPI::Unit RangedManager::closestrangedUnit(BWAPI::Unit target, std::set<BWAPI::Unit> & rangedUnitsToAssign)
{
	double minDistance = 0;
	BWAPI::Unit closest = nullptr;

	for (auto & rangedUnit : rangedUnitsToAssign)
	{
		double distance = rangedUnit->getDistance(target);
		if (!closest || distance < minDistance)
		{
			minDistance = distance;
			closest = rangedUnit;
		}
	}

	return closest;
}


// still has bug in it somewhere, use Old version
void RangedManager::assignTargetsNew(const BWAPI::Unitset & targets)
{
	const BWAPI::Unitset & rangedUnits = getUnits();

	// figure out targets
	BWAPI::Unitset rangedUnitTargets;
	std::copy_if(targets.begin(), targets.end(), std::inserter(rangedUnitTargets, rangedUnitTargets.end()), [](BWAPI::Unit u){ return u->isVisible(); });

	BWAPI::Unitset rangedUnitsToAssign(rangedUnits);
	std::map<BWAPI::Unit, int> attackersAssigned;

	for (auto & unit : rangedUnitTargets)
	{
		attackersAssigned[unit] = 0;
	}

	// keep assigning targets while we have attackers and targets remaining
	while (!rangedUnitsToAssign.empty() && !rangedUnitTargets.empty())
	{
		auto attackerAssignment = findClosestUnitPair(rangedUnitsToAssign, rangedUnitTargets);
		BWAPI::Unit & attacker = attackerAssignment.first;
		BWAPI::Unit & target = attackerAssignment.second;

		UAB_ASSERT_WARNING(attacker, "We should have chosen an attacker!");

		if (!attacker)
		{
			break;
		}

		if (!target)
		{
			Micro::SmartAttackMove(attacker, order.getPosition());
			continue;
		}

		if (Config::Micro::KiteWithRangedUnits)
		{
			if (attacker->getType() == BWAPI::UnitTypes::Zerg_Mutalisk || attacker->getType() == BWAPI::UnitTypes::Terran_Vulture)
			{
				Micro::MutaDanceTarget(attacker, target);
			}
			else
			{
				Micro::SmartKiteTarget(attacker, target);
			}
		}
		else
		{
			Micro::SmartAttackUnit(attacker, target);
		}

		// update the number of units assigned to attack the target we found
		int & assigned = attackersAssigned[attackerAssignment.second];
		assigned++;

		// if it's a small / fast unit and there's more than 2 things attacking it already, don't assign more
		if ((target->getType().isWorker() || target->getType() == BWAPI::UnitTypes::Zerg_Zergling) && (assigned > 2))
		{
			rangedUnitTargets.erase(target);
		}
		// if it's a building and there's more than 10 things assigned to it already, don't assign more
		else if (target->getType().isBuilding() && (assigned > 10))
		{
			rangedUnitTargets.erase(target);
		}

		rangedUnitsToAssign.erase(attacker);
	}

	// if there's no targets left, attack move to the order destination
	if (rangedUnitTargets.empty())
	{
		for (auto & unit : rangedUnitsToAssign)
		{
			if (unit->getDistance(order.getPosition()) > 100)
			{
				// move to it
				Micro::SmartAttackMove(unit, order.getPosition());
			}
		}
	}
}
