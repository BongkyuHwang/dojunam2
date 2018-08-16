#include "TankManager.h"
#include "CommandUtil.h"

using namespace MyBot;

TankManager::TankManager() 
{ 
}

void TankManager::executeMicro(const BWAPI::Unitset & targets) 
{
	const BWAPI::Unitset & tanks = getUnits();

	// figure out targets
	BWAPI::Unitset tankTargets;	
	cadidateRemoveTarget = nullptr;
	if (targets.size() > 0 )
		std::copy_if(targets.begin(), targets.end(), std::inserter(tankTargets, tankTargets.end()), 
                 [](BWAPI::Unit u){ return u->isVisible() && !u->isFlying(); });

    int siegeTankRange = BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange() - 32;
	int  tankTankRange = BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode.groundWeapon().maxRange() / 2;
    bool haveSiege = BWAPI::Broodwar->self()->hasResearched(BWAPI::TechTypes::Tank_Siege_Mode);
	BWAPI::Position mbase = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition();
	BWAPI::Position fchokePoint = InformationManager::Instance().getFirstChokePoint(BWAPI::Broodwar->self())->getCenter();
	
	int mbToFcP = mbase.getDistance(fchokePoint);
	
	if (order.getStatus() == "S4D")
	{
		for (auto & tank : tanks)
		{
			if (tank->getDistance(order.getPosition()) > 32)
			{
				if (tank->isSieged())
					tank->unsiege();
				else
					tank->move(order.getPosition());
			}
			else
			{
				if (tank->canSiege())
				{
					tank->siege();
				}
			}
		}
		return;
	}

	for (auto & tank : tanks)
	{
		bool goHome = false;
		if (InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition().getDistance(tank->getPosition())
		> InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition().getDistance(order.getPosition()) - order.getRadius() 
		+ BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange()*1.2)
			goHome = true;
		if (order.getType() == SquadOrderTypes::Defend)
			goHome = false;
		bool tankNearChokepoint = false;
		for (auto & choke : BWTA::getChokepoints())
		{
			//@도주남 김지훈 64 라는 절대적인 수치 기준으로 , choke point 진입여부를 판단하고 있음 , 다른 getDistance 기준 64 미만의 경우
			// 근접해있다고 판단해도 무방할 것으로 보임
			if (choke->getCenter().getDistance(tank->getPosition()) < 80)
			{
				////std::cout << "choke->getWidth() Tank In Choke Point half " << std::endl;
				//if (Config::Debug::Draw) BWAPI::Broodwar->drawTextMap(tank->getPosition() + BWAPI::Position(0, 50), "%s", "In Choke Point");
				tankNearChokepoint = true;
				break;
			}
		}
		// train sub units such as scarabs or interceptors
		if (order.getType() == SquadOrderTypes::Drop)
		{
			if ( tank->canSiege()
				&& (BWTA::getRegion(BWAPI::TilePosition(tank->getPosition()))
				!= BWTA::getRegion(InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition()))
				)
			{
				if (!tankNearChokepoint)
					tank->siege();
			}
			else 
			{
				if (tank->isSieged())
					tank->unsiege();
			}
			continue;
		}


		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend || order.getType() == SquadOrderTypes::Idle)
        {
			// if there are targets
			if (!tankTargets.empty())
			{
				// find the best target for this zealot
				BWAPI::Unit target = getTarget(tank, tankTargets);

				BWAPI::Unit target_unSiege_Unit = closestrangedUnit_kjh(tank, tankTargets);
                
				if (tank->getDistance(target_unSiege_Unit) < tankTankRange )
				{
					tank->unsiege();
				}
				else if (tank->getDistance(target) < siegeTankRange && tank->canSiege())
				{
					tank->siege();
				}
				// otherwise unsiege and move in
				//else if ((!target || (tank->getDistance(target) > siegeTankRange) && tank->canUnsiege()))
				else if ((nullptr == target || (tank->getDistance(target) > siegeTankRange) && tank->canUnsiege()))
				{
					tank->unsiege();
				}
				
                // if we're in siege mode just attack the target
                if (tank->isSieged())
                {
                    Micro::SmartAttackUnit(tank, target);
                }
                // if we're not in siege mode kite the target
                else
                {
					//@도주남 김지훈 처음 보는 부분이였음, smartKite 라는 것이 발동되어야 소위말하는 kiting 이 되는 듯
                    Micro::SmartKiteTarget(tank, target);
                }
				if (tank->isAttackFrame())
					if (Config::Debug::Draw) BWAPI::Broodwar->drawCircleMap(tank->getPosition(), 3, BWAPI::Colors::White, false);
				if (Config::Debug::Draw) BWAPI::Broodwar->drawLineMap(tank->getPosition(), target->getPosition(), BWAPI::Colors::White);

				if (cadidateRemoveTarget)
					tankTargets.erase(cadidateRemoveTarget);
			}
			// if there are no targets
			else
			{				
				if (goHome)
				{
					if (tank->canUnsiege())
					{
						tank->unsiege();
					}
					else
					{
						Micro::SmartAttackMove(tank, InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition());
					}
					continue;
				}
				else if (tank->getDistance(order.getPosition()) > order.getRadius() + BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.width() )
				{
					if (tank->canUnsiege())
					{
						tank->unsiege();
					}
					else
					{
						Micro::SmartAttackMove(tank, order.getPosition());
					}
					continue;
				}
				if (tank->canSiege() && !tankNearChokepoint)
				{
					tank->siege();
				}
			}
		}
	}
}

// get a target for the zealot to attack
BWAPI::Unit TankManager::getTarget(BWAPI::Unit tank, const BWAPI::Unitset & targets)
{
	int bestPriorityDistance = 1000000;
    int bestPriority = 0;
    
    double bestLTD = 0;

	BWAPI::Unit bestTargetThreatInRange = nullptr;
    double bestTargetThreatInRangeLTD = 0;
    
    int highPriority = 0;
	double closestDist = std::numeric_limits<double>::infinity();
	BWAPI::Unit closestTarget = nullptr;

    int siegeTankRange = BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange() - 32;
    BWAPI::Unitset targetsInSiegeRange;
    for (auto & target : targets)
    {
        if (target->getDistance(tank) < siegeTankRange && UnitUtils::CanAttack(tank, target))
        {
            targetsInSiegeRange.insert(target);
        }
    }

    const BWAPI::Unitset & newTargets = targetsInSiegeRange.empty() ? targets : targetsInSiegeRange;

    // check first for units that are in range of our attack that can cause damage
    // choose the highest priority one from them at the lowest health
    for (const auto & target : newTargets)
    {
        if (!UnitUtils::CanAttack(tank, target))
        {
            continue;
        }

        double distance         = tank->getDistance(target);
        double LTD              = UnitUtils::CalculateLTD(target, tank);
        int priority            = getAttackPriority(tank, target);
        bool targetIsThreat     = LTD > 0;        		
		double t_LTD = UnitUtils::CalculateLTD(tank, target); // 한방에 죽일 수 있을 경우를 체크 하기 위한 로직
		bool CK = target->getHitPoints() <= t_LTD;

		if (!closestTarget || (priority > highPriority) || (priority == highPriority && distance < closestDist) )
		{
			closestDist = distance;
			highPriority = priority;
			closestTarget = target;
			if (priority >= highPriority && CK)
			{
				bestTargetThreatInRange = target;
			}
		}
    }

    if (bestTargetThreatInRange)
    {	
		cadidateRemoveTarget = bestTargetThreatInRange;
        return bestTargetThreatInRange;
    }
	cadidateRemoveTarget = nullptr;
    return closestTarget;
}

//@도주남 김지훈 시즈모드 풀기위한 가장 가까운 유닛찾기

// get the attack priority of a type in relation to a zergling
int TankManager::getAttackPriority(BWAPI::Unit rangedUnit, BWAPI::Unit target) 
{
	BWAPI::UnitType rangedType = rangedUnit->getType();
	BWAPI::UnitType targetType = target->getType();

	bool isThreat = rangedType.isFlyer() ? targetType.airWeapon() != BWAPI::WeaponTypes::None : targetType.groundWeapon() != BWAPI::WeaponTypes::None;

    if (target->getType().isWorker())
    {
        isThreat = false;
    }

    if (target->getType() == BWAPI::UnitTypes::Zerg_Larva || target->getType() == BWAPI::UnitTypes::Zerg_Egg)
    {
        return 0;
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

	if (rangedUnit->isSieged() && targetType == BWAPI::UnitTypes::Protoss_Dragoon)
	{
		return 20;
	}

	// highest priority is something that can attack us or aid in combat
    if (targetType ==  BWAPI::UnitTypes::Terran_Bunker || isThreat)
    {
        return 11;
    }
	// next priority is worker
	else if (targetType.isWorker()) 
	{
  		return 9;
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

BWAPI::Unit TankManager::closestrangedUnit(BWAPI::Unit target, std::set<BWAPI::Unit> & rangedUnitsToAssign)
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


BWAPI::Unit TankManager::closestrangedUnit_kjh(BWAPI::Unit target, BWAPI::Unitset & rangedUnitsToAssign)
{
	double minDistance = 999999;
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
