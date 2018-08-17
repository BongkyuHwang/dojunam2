#include "VultureManager.h"
#include "CommandUtil.h"

using namespace MyBot;

VultureManager::VultureManager()
{
	miningOn = false;
	miningUnit = nullptr;
	scountUnit = nullptr;
	lastMiniCalFrame = 0;
	chokePointCount = 0;
}

bool clsDist(const BWAPI::Position &mp1, const BWAPI::Position &mp2)
{
	return BWAPI::Broodwar->enemy()->getStartLocation().getDistance(BWAPI::TilePosition(mp1)) > BWAPI::Broodwar->enemy()->getStartLocation().getDistance(BWAPI::TilePosition(mp2));
}

void VultureManager::miningPositionSetting()
{
	BWTA::Chokepoint * mysecChokePoint = InformationManager::Instance().getSecondChokePoint(BWAPI::Broodwar->self());
	BWTA::Chokepoint * enemySecChokePoint = InformationManager::Instance().getSecondChokePoint(BWAPI::Broodwar->enemy());
	
	if (!BWAPI::Broodwar->self()->hasResearched(BWAPI::TechTypes::Spider_Mines))
		return;
	if (enemySecChokePoint == nullptr || getUnits().size() == 0)
		return;

	if (chokePointForVulture.size() - chokePointCount <= 0)
	{
		chokePointForVulture.clear();
		miningOn = false;
	}
	else if (miningOn)
	{
		return;
	}
	
	for (BWTA::Chokepoint * chokepoint : BWTA::getChokepoints())
	{
		if (chokepoint == InformationManager::Instance().getFirstChokePoint(BWAPI::Broodwar->self()))
			continue;
		if (chokepoint == mysecChokePoint)
			continue;
		if (chokepoint == InformationManager::Instance().getFirstChokePoint(BWAPI::Broodwar->enemy()))
			continue;
		if (chokepoint == enemySecChokePoint)
			continue;
		chokePointForVulture.push_back(chokepoint->getCenter());
	}
	
	chokePointCount = chokePointForVulture.size();
	
	float offset = 0.4f;
	BWAPI::TilePosition home = BWAPI::Broodwar->self()->getStartLocation();
	BWTA::Region * centerRegion = BWTA::getRegion(BWAPI::TilePosition(BWAPI::Broodwar->mapWidth()/2, BWAPI::Broodwar->mapHeight()/2));
	std::vector<BWAPI::TilePosition> tileList = BWTA::getShortestPath(BWAPI::TilePosition(enemySecChokePoint->getCenter()), BWAPI::TilePosition(mysecChokePoint->getCenter()));
	for (int tx = 1; tx < BWAPI::Broodwar->mapWidth(); tx+=2)
	{
		for (int ty = 1; ty < BWAPI::Broodwar->mapHeight(); ty+=3)
		{
			BWAPI::TilePosition t(tx, ty);
			BWAPI::Position pos(t);
			if (!t.isValid())
			{
				continue;
			}
			if (!BWTA::isConnected(t, home))
			{
				continue;
			}

			if (!checkPositionWalkable(pos))
			{
				continue;
			}

			if (BWAPI::Broodwar->getUnitsOnTile(t).size()>0)
				continue;
			if (centerRegion->getPolygon().isInside(pos))
				chokePointForVulture.push_back(pos);
			else if (std::find(tileList.begin(), tileList.end(), t) != tileList.end())
			{
				chokePointForVulture.push_back(pos);
			}
			
		}
	}

	for (BWTA::BaseLocation * startLocation : BWTA::getStartLocations())
	{
		bool setMine = true;
		for (BWTA::Region * myRegion : InformationManager::Instance().getOccupiedRegions(BWAPI::Broodwar->enemy()))
		{
			if (startLocation->getRegion() == myRegion)
			{
				setMine = false;
				break;
			}
		}
		if (setMine)
			chokePointForVulture.push_back(startLocation->getPosition());
	}

	chokePointForVulture.push_back(InformationManager::Instance().getFirstExpansionLocation(BWAPI::Broodwar->enemy())->getPosition());

	std::sort(chokePointForVulture.begin(), chokePointForVulture.end(), clsDist);
	chokePointForVulture.erase(unique(chokePointForVulture.begin(), chokePointForVulture.end()), chokePointForVulture.end());
	miningOn = true;
	lastMiniCalFrame = BWAPI::Broodwar->getFrameCount();
	return;
}

void VultureManager::executeMicro(const BWAPI::Unitset & targets)
{
	assignTargetsOld(targets);
}


void VultureManager::assignTargetsOld(const BWAPI::Unitset & targets)
{
	const BWAPI::Unitset & vultureUnits = getUnits();
	int spiderMineCount = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Vulture_Spider_Mine);
	if (spiderMineCount > 0 && Config::Debug::Draw)
	for (auto & a : chokePointForVulture)
	{
		BWAPI::Broodwar->drawCircleMap(a, 4, BWAPI::Colors::Green, false);
	}
	// figure out targets
	BWAPI::Unitset vultureUnitTargets;
	if (targets.size()>0)
		std::copy_if(targets.begin(), targets.end(), std::inserter(vultureUnitTargets, vultureUnitTargets.end()), [](BWAPI::Unit u){ return u->isVisible(); });	
	
	if (order.getStatus() == "scout")
	{
		setScoutRegions();
		for (auto & vultureUnit : vultureUnits){
			BWAPI::Unit target = UnitUtils::canIFight(vultureUnit);
			if (target == nullptr)
			{
				if (vultureUnit->isUnderAttack())
				{
					scoutRegions.clear();
				}
				getScoutRegions(vultureUnit);
			}			
			else
			{
				vultureUnit->attack(target);
			}
			//if (vultureUnit->getHitPoints() < vultureUnit->getType().maxHitPoints())
			//{
			//	vultureUnit->move(InformationManager::Instance().getSecondChokePoint(BWAPI::Broodwar->self())->getSides().first);
			//}
			//else
			
			//}
		}
		return;
	}

	for (auto & vultureUnit : vultureUnits)
	{
		//BWAPI::Broodwar->drawTextMap(vultureUnit->getPosition().x, vultureUnit->getPosition().y + 5, "\x07%d", vultureUnit->getID());

		bool goHome = false;
		if (InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition().getDistance(vultureUnit->getPosition())
		> InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition().getDistance(order.getPosition()) - order.getRadius()
		+ BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange() * 1.1
		+ BWAPI::UnitTypes::Terran_Firebat.groundWeapon().maxRange()
		//+ BWAPI::UnitTypes::Terran_Vulture.groundWeapon().maxRange()
		)
			goHome = true;
		if (order.getType() == SquadOrderTypes::Defend)
			goHome = false;
		if (order.getType() == SquadOrderTypes::Drop)
		{
			//if (BWTA::getRegion(BWAPI::TilePosition(vultureUnit->getPosition())) != InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getRegion())
			if (BWTA::getRegion(BWAPI::TilePosition(vultureUnit->getPosition()))
				== BWTA::getRegion(InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy())->getTilePosition())
				|| BWTA::getRegion(BWAPI::TilePosition(vultureUnit->getPosition()))
				!= BWTA::getRegion(InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getTilePosition()))
			{

				if (vultureUnit->getSpiderMineCount() == 3)
					Micro::SmartLaySpiderMine(vultureUnit, vultureUnit->getPosition());
				else
				{
					if (!vultureUnitTargets.empty())
					{
						// find the best target for this zealot
						BWAPI::Unit target = getTarget(vultureUnit, vultureUnitTargets);
						vultureUnit->attack(target);
					}
				}
			}
			else
			{
				if (vultureUnit->getDistance(order.getPosition()) > 200)
					vultureUnit->move(order.getPosition());
			}
			continue;
		}
		//else
		// if the order is to attack or defend
		if (order.getType() == SquadOrderTypes::Attack || order.getType() == SquadOrderTypes::Defend || order.getType() == SquadOrderTypes::Idle)
		{

			// if there are targets
			if (!vultureUnitTargets.empty())
			{

				if (goHome && vultureUnit->getID() %10 == 0)
				{
					if (order.getCenterPosition().isValid())
					{
						Micro::SmartMove(vultureUnit, order.getCenterPosition());
					}
					else
						Micro::SmartMove(vultureUnit, InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition());
					continue;
				}
				// find the best target for this zealot
				BWAPI::Unit target = getTarget(vultureUnit, vultureUnitTargets);

				if (order.getCenterPosition().isValid() 
					&& order.getCenterPosition().getDistance(vultureUnit->getPosition()) >BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange())
				{
					Micro::SmartMove(vultureUnit, order.getCenterPosition());
					continue;
				}

				////std::cout << " vulture target distance " << target->getDistance(order.getPosition()) << " / order Radius : " << order.getRadius() << std::endl;
				if (target->getType().isBuilding() && target->getDistance(vultureUnit) > vultureUnit->getType().groundWeapon().maxRange() *0.4)
				{	
					if (vultureUnit->getSpiderMineCount() > 0)
						Micro::SmartLaySpiderMine(vultureUnit, vultureUnit->getPosition());
					else
						vultureUnit->move(target->getPosition());
					continue;
				}
				else if (target->getType().isBuilding())
				{
					Micro::SmartAttackUnit(vultureUnit, target);
					continue;
				}
				else
				{
					Micro::MutaDanceTarget(vultureUnit, target);
					continue;
				}
			}
			// if there are no targets
			else
			{
				if (order.getType() == SquadOrderTypes::Idle || order.getType() == SquadOrderTypes::Attack)
				{
					if (order.getType() == SquadOrderTypes::Attack)
					{
						if (goHome)
						{
							if (order.getCenterPosition().isValid())
							{
								Micro::SmartAttackMove(vultureUnit, order.getCenterPosition());
							}
							else
								Micro::SmartAttackMove(vultureUnit, InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition());
							continue;
						}
						if (vultureUnit->getDistance(order.getPosition()) > order.getRadius() )
						{

							Micro::SmartAttackMove(vultureUnit, order.getPosition());
							continue;
						}
						else
						{
							if (vultureUnit->getSpiderMineCount() > 1)
							{
								Micro::SmartLaySpiderMine(vultureUnit, vultureUnit->getPosition());
								continue;
							}
						}
					}
					else if (miningOn && vultureUnit->getSpiderMineCount() > 0 && chokePointForVulture.size() > 0
						&& !vultureUnit->isStuck() && InformationManager::Instance().rushState == 0)//&& (miningUnit == nullptr || miningUnit == vultureUnit) && !vultureUnit->isStuck())
					{
						BWAPI::Position mineSetPosition = vultureUnit->getPosition();
						if (chokePointForVulture.size() > (vultureUnit->getID() % 6)+1 )
							mineSetPosition = chokePointForVulture[chokePointForVulture.size() - (vultureUnit->getID() % 6)];
						else
							mineSetPosition = chokePointForVulture[chokePointForVulture.size() - 1];
						// 벌처가 마인을 설치해야 하는 위치중 제일 마지막 위치를 가져오고, 해당위치가 유효 한 경우에만 마인을 설치한다.
						bool position_invalid = true;
						while (position_invalid)
						{
							if (chokePointForVulture.size() <= 0)
							{
								position_invalid = true;
								break;
							}
							position_invalid = false;
							for (auto & ifmine : BWAPI::Broodwar->getUnitsOnTile(BWAPI::TilePosition(mineSetPosition)))
							{
								if (ifmine->getType() == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine || ifmine->getType().isBuilding())
								{
									if (chokePointForVulture.size() > 1)
									{
										mineSetPosition = chokePointForVulture[chokePointForVulture.size() - 2];
										chokePointForVulture.pop_back();
										position_invalid = false;
										break;
									}
									else if (chokePointForVulture.size() > 0)
										chokePointForVulture.pop_back();
									position_invalid = true;
									break;
								}
							}
						}
						//만약 벌처가 공격을 받고 있으면 // 마인 설치를 포기하고 부대 위치로 복귀한다.
						if (vultureUnit->isUnderAttack())
						{
							if (chokePointForVulture.size() > 0)
								chokePointForVulture.pop_back();
							if (order.getCenterPosition().isValid())
							{
								Micro::SmartMove(vultureUnit, order.getCenterPosition());
							}
							else
								Micro::SmartMove(vultureUnit, InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition());
							position_invalid = true;
							continue;
						}
						if (!position_invalid)
						{
							Micro::SmartLaySpiderMine(vultureUnit, mineSetPosition);
							continue;
						}
					}
				}
				
				// if we're not near the order position
				if (order.getCenterPosition().isValid())
				{
					Micro::SmartMove(vultureUnit, order.getCenterPosition());
				}
				else
				{
					Micro::SmartMove(vultureUnit, order.getPosition());
				}

			}
		}
	}
}

std::pair<BWAPI::Unit, BWAPI::Unit> VultureManager::findClosestUnitPair(const BWAPI::Unitset & attackers, const BWAPI::Unitset & targets)
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
BWAPI::Unit VultureManager::getTarget(BWAPI::Unit vultureUnit, const BWAPI::Unitset & targets)
{
	int bestPriorityDistance = 1000000;
	int bestPriority = 0;

	double bestLTD = 0;

	int highPriority = 0;
	double closestDist = std::numeric_limits<double>::infinity();
	BWAPI::Unit closestTarget = nullptr;

	for (const auto & target : targets)
	{
		double distance = vultureUnit->getDistance(target);
		double LTD = UnitUtils::CalculateLTD(target, vultureUnit);
		int priority = getAttackPriority(vultureUnit, target);
		bool targetIsThreat = LTD > 0;

		if (!closestTarget || (priority > highPriority) || (priority == highPriority && distance < closestDist))
		{
			closestDist = distance;
			highPriority = priority;
			closestTarget = target;
		}
	}

	return closestTarget;
}

// get the attack priority of a type in relation to a zergling
int VultureManager::getAttackPriority(BWAPI::Unit vultureUnit, BWAPI::Unit target)
{

	if (target->getType() == BWAPI::UnitTypes::Zerg_Larva || target->getType() == BWAPI::UnitTypes::Zerg_Egg)
	{
		return 0;
	}

	if (target->getType().isFlyer())
	{
		return 0;
	}


	// next priority is worker
	if (target->getType().isWorker())
	{
		return 11;
	}
	// next is special buildings
	else if (target->getType() == BWAPI::UnitTypes::Protoss_High_Templar || target->getType() == BWAPI::UnitTypes::Zerg_Defiler)
	{
		return 10;
	}
	// next is special buildings
	else if (target->getType().size() == BWAPI::UnitSizeTypes::Small)
	{
		return 9;
	}
	// next is buildings that cost gas
	else if (target->getType().size() == BWAPI::UnitSizeTypes::Medium)
	{
		return 8;
	}
	else if (target->getType().size() == BWAPI::UnitSizeTypes::Large)
	{
		return 7;
	}
	// then everything else
	else
	{
		return 1;
	}
}

BWAPI::Unit VultureManager::closestvultureUnit(BWAPI::Unit target, std::set<BWAPI::Unit> & vultureUnitsToAssign)
{
	double minDistance = 9999999;
	BWAPI::Unit closest = nullptr;

	for (auto & vultureUnit : vultureUnitsToAssign)
	{
		double distance = vultureUnit->getDistance(target);
		if (!closest || distance < minDistance)
		{
			minDistance = distance;
			closest = vultureUnit;
		}
	}

	return closest;
}


// still has bug in it somewhere, use Old version
void VultureManager::assignTargetsNew(const BWAPI::Unitset & targets)
{
	const BWAPI::Unitset & vultureUnits = getUnits();

	// figure out targets
	BWAPI::Unitset vultureUnitTargets;
	std::copy_if(targets.begin(), targets.end(), std::inserter(vultureUnitTargets, vultureUnitTargets.end()), [](BWAPI::Unit u){ return u->isVisible(); });

	BWAPI::Unitset vultureUnitsToAssign(vultureUnits);
	std::map<BWAPI::Unit, int> attackersAssigned;

	for (auto & unit : vultureUnitTargets)
	{
		attackersAssigned[unit] = 0;
	}

	// keep assigning targets while we have attackers and targets remaining
	while (!vultureUnitsToAssign.empty() && !vultureUnitTargets.empty())
	{
		auto attackerAssignment = findClosestUnitPair(vultureUnitsToAssign, vultureUnitTargets);
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
			vultureUnitTargets.erase(target);
		}
		// if it's a building and there's more than 10 things assigned to it already, don't assign more
		else if (target->getType().isBuilding() && (assigned > 10))
		{
			vultureUnitTargets.erase(target);
		}

		vultureUnitsToAssign.erase(attacker);
	}

	// if there's no targets left, attack move to the order destination
	if (vultureUnitTargets.empty())
	{
		for (auto & unit : vultureUnitsToAssign)
		{
			if (unit->getDistance(order.getPosition()) > 100)
			{
				// move to it
				Micro::SmartAttackMove(unit, order.getPosition());
			}
		}
	}
}

void VultureManager::setScoutRegions()
{
	if (scoutRegions.size() > 0)
		return;
	////std::cout << "setScoutRegions " << std::endl;
	BWTA::Chokepoint * enemySecChokePoint = InformationManager::Instance().getSecondChokePoint(BWAPI::Broodwar->enemy());

	if (enemySecChokePoint == nullptr)
		return;

	std::list<BWTA::BaseLocation *> enemayBaseLocations = InformationManager::Instance().getOccupiedBaseLocations(BWAPI::Broodwar->enemy());
	std::list<BWTA::BaseLocation *> selfBaseLocations = InformationManager::Instance().getOccupiedBaseLocations(BWAPI::Broodwar->self());

	for (BWTA::BaseLocation * startLocation : BWTA::getBaseLocations())
	{
		bool insertable = true;
		//if (InformationManager::Instance().getFirstExpansionLocation(BWAPI::Broodwar->enemy()) == startLocation)
		//	continue;
		if (BWTA::isConnected(startLocation->getTilePosition(), InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getTilePosition()) == false)
		{
			continue;
		}
		
		for (auto & ifmine : BWAPI::Broodwar->getUnitsOnTile(startLocation->getTilePosition()))
		{
			if (ifmine->getType() == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine)
			{
				insertable = false;
				break;
			}
		}

		for (BWTA::BaseLocation * eBaseLocation : enemayBaseLocations)
		{
			if (startLocation == eBaseLocation)
			{
				insertable = false;
				break;
			}
		}
		if (insertable)
		{
			for (BWTA::BaseLocation * sBaseLocation : selfBaseLocations)
			{
				if (startLocation == sBaseLocation)
				{
					insertable = false;
					break;
				}
			}
		}
		
		if (insertable)
		{
			scoutRegions.push_back(startLocation->getPosition());
		}
	}
}

void VultureManager::getScoutRegions(BWAPI::Unit unit)
{
	if (scoutRegions.size() <= 0)
	{
		setScoutRegions();
		if (scoutRegions.size() <= 0)
		{
			unit->move(InformationManager::Instance().getFirstChokePoint(BWAPI::Broodwar->self())->getSides().first);
		}
	}

	//if (BWTA::getRegion(BWAPI::TilePosition(unitPosition)) == BWTA::getRegion(BWAPI::TilePosition(scoutRegions[scoutRegions.size()-1])))
	if (unit->getSpiderMineCount() != 0)
	{
		if (unit->getPosition().getDistance(scoutRegions[scoutRegions.size() - 1]) > 100)
			Micro::SmartLaySpiderMine(unit, scoutRegions[scoutRegions.size() - 1]);
		for (auto & ifmine : BWAPI::Broodwar->getUnitsOnTile(BWAPI::TilePosition(scoutRegions[scoutRegions.size() - 1])))
		{
			if (ifmine->getType() == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine)
			{
				scoutRegions.pop_back();
				break;
			}
		}
	}
	else if (unit->getPosition().getDistance(scoutRegions[scoutRegions.size() - 1]) < 100)
	{	
		scoutRegions.pop_back();
	}
	else
	{
		unit->move(scoutRegions[scoutRegions.size() - 1]);
	}

}