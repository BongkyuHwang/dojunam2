#include "Squad.h"
#include "CommandUtil.h"

using namespace MyBot;

Squad::Squad()
	: _lastRetreatSwitch(0)
	, _lastRetreatSwitchVal(false)
	, _priority(0)
	, _name("Default")
{
	int a = 10;
}

Squad::Squad(const std::string & name, SquadOrder order, size_t priority)
	: _name(name)
	, _order(order)
	, _lastRetreatSwitch(0)
	, _lastRetreatSwitchVal(false)
	, _priority(priority)
{
}

Squad::~Squad()
{
	clear();
}

void Squad::update()
{
	if (_name == "bunker"){
		//if (order.getStatus() == "bunker")
		{
			BWAPI::Unit maybeBunker = nullptr;
			for (auto & unit : BWAPI::Broodwar->getUnitsOnTile(BWAPI::TilePosition(_order.getPosition())))
			{
				if (unit->getType() == BWAPI::UnitTypes::Terran_Bunker)
				{
					maybeBunker = unit;
					break;
				}
			}
			for (auto & rangedUnit : _units)
			{
				if (maybeBunker != nullptr)
				{
					if (!rangedUnit->isLoaded())
						rangedUnit->load(maybeBunker);
					if (_order.getPosition().getDistance(rangedUnit->getPosition()) > 100 && rangedUnit->isLoaded())
					{
						for (auto & unit : BWAPI::Broodwar->getUnitsOnTile(BWAPI::TilePosition(rangedUnit->getPosition())))
						{
							if (unit->getType() == BWAPI::UnitTypes::Terran_Bunker)
							{
								unit->unloadAll();
								return;
							}
						}
					}
				}
			}
			return;
		}
	}
	else{
		
		// update all necessary unit information within this squad
		_order.setCenterPosition(BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation()));
		updateUnits();

		// determine whether or not we should regroup

		// draw some debug info
		/*if (Config::Debug::DrawSquadInfo && _order.getType() == SquadOrderTypes::Attack)
		{
			if (Config::Debug::Draw) BWAPI::Broodwar->drawTextScreen(200, 350, "%s", _regroupStatus.c_str());

			BWAPI::Unit closest = unitClosestToEnemy();
		}*/

		// if we do need to regroup, do it
		//BWAPI::Unit cloesetUnit = unitClosestToEnemy();
		//if (cloesetUnit != nullptr) {
		//	_order.setClosestUnit(cloesetUnit);
		//}
		_meleeManager.execute(_order);
		_rangedManager.execute(_order);
		_vultureManager.execute(_order);
		_medicManager.execute(_order);
		_tankManager.execute(_order);
		_transportManager.update();
		//std::cout << "s else 3" << std::endl;
		// unitClosestToEnemy nullptr 리턴할경우 예외처리
		BWAPI::Unit cloesetUnit = unitClosestToEnemy();
		if (cloesetUnit != nullptr) {		
			_detectorManager.setUnitClosestToEnemy(cloesetUnit);
			_detectorManager.execute(_order);
		}
		_detectorManager.execute(_order);
		//std::cout << "s else 4" << std::endl;
	}
}

bool Squad::isEmpty() const
{
	return _units.empty();
}

size_t Squad::getPriority() const
{
	return _priority;
}

void Squad::setPriority(const size_t & priority)
{
	_priority = priority;
}

void Squad::updateUnits()
{
	setAllUnits();
	if (_vultureManager.miningOn)
	{
		if (BWAPI::Broodwar->getFrameCount() - _vultureManager.lastMiniCalFrame > 10000)
			if (_vultureManager.chokePointCount != 0 && _vultureManager.chokePointForVulture.size() - _vultureManager.chokePointCount <= 0)
				_vultureManager.miningOn = false;
	}
	if (_vultureManager.miningOn == false)
	{
		_vultureManager.lastMiniCalFrame = BWAPI::Broodwar->getFrameCount();
		_vultureManager.miningPositionSetting();
	}
	setNearEnemyUnits();
	addUnitsToMicroManagers();
	BWAPI::Position centerPosition = calcCenter();
	//BWAPI::Broodwar->drawCircleMap(centerPosition, 20, BWAPI::Colors::Purple, true);
	//BWAPI::Broodwar->drawLineMap(BWAPI::Position(0,0) ,centerPosition, BWAPI::Colors::Purple);
	//BWAPI::Broodwar->setScreenPosition(centerPosition - BWAPI::Position(10, 10));	
	//BWAPI::Broodwar->drawBoxMap(centerPosition.x - 10, centerPosition.y - 10, centerPosition.x + 10, centerPosition.y + 10, BWAPI::Colors::Yellow, true);
	if (centerPosition != BWAPI::Positions::None)
	{
		_order.setCenterPosition(centerPosition);
	}
	else
		_order.setCenterPosition(_order.getPosition());
}

void Squad::setAllUnits()
{
	// clean up the _units vector just in case one of them died
	BWAPI::Unitset goodUnits;

	//@도주남 김지훈 메딕이 힐링이 가능한 캐릭터가 몇명인지 확인한다.  의견을 들어보고 적용 여부 결정
	BWAPI::Unitset organicUnits;

	for (auto & unit : _units)
	{

		if (unit->isCompleted() &&
			unit->getHitPoints() > 0 &&
			unit->exists() &&
			unit->getType() != BWAPI::UnitTypes::Terran_Vulture_Spider_Mine &&
			unit->getPosition().isValid() &&
			unit->getType() != BWAPI::UnitTypes::Unknown)
		{

			if (unit->isStuck())//|| unit->isIdle())
			{
				//printf("isStuck [%d] %s\n", unit->getID(), unit->getType().c_str());
				//BWAPI::Broodwar->setScreenPosition(unit->getPosition() - BWAPI::Position(100, 100));
				//BWAPI::Broodwar->drawBoxMap(unit->getPosition() - BWAPI::Position(unit->getType().width() + 10, unit->getType().height() + 10), unit->getPosition() + BWAPI::Position(unit->getType().width() + 10, unit->getType().height() + 10), BWAPI::Colors::Black, true);
				//BWAPI::Broodwar->drawBoxMap(unit->getPosition() - BWAPI::Position(unit->getType().width(), unit->getType().height()), unit->getPosition() + BWAPI::Position(unit->getType().width(), unit->getType().height()), BWAPI::Colors::Yellow, true);
				//unit->setRallyPoint(_order.getPosition());
				unit->holdPosition();
				for (BWTA::Chokepoint * chokepoint : BWTA::getChokepoints())
				{
					// BWTA::getGroundDistance 연산량이 많아 교체
					if (MapTools::Instance().getGroundDistance(unit->getPosition(), chokepoint->getCenter()) <= chokepoint->getWidth())
					{
						Micro::SmartMove(unit, BWAPI::Position(BWAPI::Broodwar->mapWidth() * 16, BWAPI::Broodwar->mapHeight() * 16));
						break;
					}
				}
				continue;
			}
			//else
			//	if (Config::Debug::Draw) BWAPI::Broodwar->drawCircleMap(unit->getPosition(), 5, BWAPI::Colors::Blue, true);
			goodUnits.insert(unit);

			if (unit->getType().isOrganic() && unit->getType() != BWAPI::UnitTypes::Terran_Medic)
			{
				organicUnits.insert(unit);
			}
		}
	}
	_units.clear();
	_units = goodUnits;
	
	if (organicUnits.size() > 0)
		_order.setOrganicUnits(organicUnits);

	if (organicUnits.size() != 0)
		_order.setClosestUnit(unitClosestToEnemy());
	if (_order.getClosestUnit() == nullptr)
		_order.setClosestUnit(nullptr);
}

void Squad::setNearEnemyUnits()
{
	_nearEnemy.clear();
	for (auto & unit : _units)
	{
		int x = unit->getPosition().x;
		int y = unit->getPosition().y;

		int left = unit->getType().dimensionLeft();
		int right = unit->getType().dimensionRight();
		int top = unit->getType().dimensionUp();
		int bottom = unit->getType().dimensionDown();

		_nearEnemy[unit] = unitNearEnemy(unit);
		if (_nearEnemy[unit])
		{
			if (Config::Debug::DrawSquadInfo) if (Config::Debug::Draw) BWAPI::Broodwar->drawBoxMap(x - left, y - top, x + right, y + bottom, Config::Debug::ColorUnitNearEnemy);
		}
		else
		{
			if (Config::Debug::DrawSquadInfo) if (Config::Debug::Draw) BWAPI::Broodwar->drawBoxMap(x - left, y - top, x + right, y + bottom, Config::Debug::ColorUnitNotNearEnemy);
		}
	}
}

std::vector<BWAPI::Unitset> Squad::_units_divided(int num){
	//BWAPI::Unitset meleeUnits;
	//BWAPI::Unitset rangedUnits;
	//BWAPI::Unitset detectorUnits;
	//BWAPI::Unitset transportUnits;
	//BWAPI::Unitset tankUnits;
	//BWAPI::Unitset medicUnits;
	//BWAPI::Unitset vultureUnits;
	//// add _units to micro managers
	//for (auto & unit : _units)
	//{
	//	if (unit->isCompleted() && unit->getHitPoints() > 0 && unit->exists())
	//	{
	//		// select dector _units
	//		if (unit->getType() == BWAPI::UnitTypes::Terran_Medic)
	//		{
	//			medicUnits.insert(unit);
	//		}
	//		else if (unit->getType() == BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode || unit->getType() == BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode)
	//		{
	//			tankUnits.insert(unit);
	//		}
	//		//@도주남 김지훈
	//		else if (unit->getType() == BWAPI::UnitTypes::Terran_Vulture)
	//		{
	//			vultureUnits.insert(unit);
	//		}
	//		else if (unit->getType().isDetector() && !unit->getType().isBuilding())
	//		{
	//			detectorUnits.insert(unit);
	//		}
	//		// select transport _units
	//		else if (unit->getType() == BWAPI::UnitTypes::Protoss_Shuttle || unit->getType() == BWAPI::UnitTypes::Terran_Dropship)
	//		{
	//			transportUnits.insert(unit);
	//		}
	//		// select ranged _units
	//		else if ((unit->getType().groundWeapon().maxRange() > 32) || (unit->getType() == BWAPI::UnitTypes::Protoss_Reaver) || (unit->getType() == BWAPI::UnitTypes::Zerg_Scourge))
	//		{
	//			rangedUnits.insert(unit);
	//		}
	//		// select melee _units
	//		else if (unit->getType().groundWeapon().maxRange() <= 32)
	//		{
	//			meleeUnits.insert(unit);
	//		}
	//	}
	//}

	//std::vector<BWAPI::Unitset> rst;
	//if (num == 2){
	//	BWAPI::Unitset half_first;
	//	BWAPI::Unitset half_second;

	//	auto lamb_divide = [](BWAPI::Unitset &u, BWAPI::Unitset & half_first, BWAPI::Unitset &half_second){
	//		int half_idx = u.size() / 2;
	//		int cnt = 0;
	//		for (auto tmpUnit : u){
	//			if (cnt < half_idx){
	//				half_first.insert(tmpUnit);
	//			}
	//			else{
	//				half_second.insert(tmpUnit);
	//			}
	//			cnt++;
	//		}
	//	};

	//	lamb_divide(meleeUnits, half_first, half_second);
	//	lamb_divide(rangedUnits, half_first, half_second);
	//	lamb_divide(detectorUnits, half_first, half_second);
	//	lamb_divide(transportUnits, half_first, half_second);
	//	lamb_divide(tankUnits, half_first, half_second);
	//	lamb_divide(medicUnits, half_first, half_second);
	//	lamb_divide(vultureUnits, half_first, half_second);

	//	rst.push_back(half_first);
	//	rst.push_back(half_second);
	//}

	//return rst;
}

void Squad::addUnitsToMicroManagers()
{
	BWAPI::Unitset meleeUnits;
	BWAPI::Unitset rangedUnits;
	BWAPI::Unitset detectorUnits;
	BWAPI::Unitset transportUnits;
	BWAPI::Unitset tankUnits;
	BWAPI::Unitset medicUnits;
	BWAPI::Unitset vultureUnits;

	// add _units to micro managers
	for (auto & unit : _units)
	{
		if (unit->isCompleted() && unit->getHitPoints() > 0 && unit->exists())
		{
			// select dector _units
			if (unit->getType() == BWAPI::UnitTypes::Terran_Medic)
			{
				medicUnits.insert(unit);
			}
			else if (unit->getType() == BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode || unit->getType() == BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode)
			{
				tankUnits.insert(unit);
			}
			//@도주남 김지훈
			else if (unit->getType() == BWAPI::UnitTypes::Terran_Vulture)
			{
				vultureUnits.insert(unit);
			}
			else if (unit->getType().isDetector() && !unit->getType().isBuilding())
			{
				detectorUnits.insert(unit);
			}
			// select transport _units
			else if (unit->getType() == BWAPI::UnitTypes::Protoss_Shuttle || unit->getType() == BWAPI::UnitTypes::Terran_Dropship)
			{
				transportUnits.insert(unit);
			}
			// select ranged _units
			else if ((unit->getType().groundWeapon().maxRange() > 32) || (unit->getType() == BWAPI::UnitTypes::Protoss_Reaver) || (unit->getType() == BWAPI::UnitTypes::Zerg_Scourge))
			{
				rangedUnits.insert(unit);
			}
			// select melee _units
			else if (unit->getType().groundWeapon().maxRange() <= 32)
			{
				meleeUnits.insert(unit);
			}
		}
	}
	for (auto & bD : InformationManager::Instance().buildingDetectors)
	{
		// load 된 유닛 들어오면 유닛셋을 그냥 밀어 넣는다.
		if (bD->isFlying() && bD->getType().isBuilding() )
			detectorUnits.insert(bD);
	}
	if (BWAPI::Broodwar->getFrameCount() > 30000 && detectorUnits.size() <= 0)
	{
		for (auto & bD : transportUnits)
		{
			detectorUnits.insert(bD);
			transportUnits.erase(bD);
			break;
		}
		if (detectorUnits.size() <= 0)
			for (auto & bD : rangedUnits)
			{
				detectorUnits.insert(bD);
				rangedUnits.erase(bD);
				break;
			}
	}


	_meleeManager.setUnits(meleeUnits);
	_rangedManager.setUnits(rangedUnits);
	_detectorManager.setUnits(detectorUnits);
	_transportManager.setUnits(transportUnits);
	_tankManager.setUnits(tankUnits);
	_vultureManager.setUnits(vultureUnits);
	_medicManager.setUnits(medicUnits);

}

void Squad::setSquadOrder(const SquadOrder & so)
{
	_order = so;
}

bool Squad::containsUnit(BWAPI::Unit u) const
{
	return _units.contains(u);
}

void Squad::clear()
{
	for (auto & unit : getUnits())
	{
		if (unit->getType().isWorker())
		{
			WorkerManager::Instance().finishedWithWorker(unit);
		}
	}

	_units.clear();
}

bool Squad::unitNearEnemy(BWAPI::Unit unit)
{
	assert(unit);

	BWAPI::Unitset enemyNear;

	MapGrid::Instance().getUnitsNear(enemyNear, unit->getPosition(), 400, false, true);

	return enemyNear.size() > 0;
}

BWAPI::Position Squad::calcCenter()
{
	if (_units.empty())
	{
		if (Config::Debug::DrawSquadInfo)
		{
			BWAPI::Broodwar->printf("Squad::calcCenter() called on empty squad");
		}
		return BWAPI::Positions::None;
	}

	BWAPI::Position accum(0, 0);
	int sizeUnits = 0;
	accum += _order.getPosition();
	sizeUnits++;
	for (auto & unit : _units)
	{
		if (_order.getPosition().getDistance(unit->getPosition()) > _order.getRadius())
		{
			continue;
		}
		if (unit->isFlying())
			continue;

		if (unit->getType() == (BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode) || unit->getType() == (BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode))
		{
			accum += unit->getPosition();
			sizeUnits++;
		}
	}
	if (sizeUnits == 0)
		return BWAPI::Positions::None;
	if (accum == _order.getPosition())
	{
		BWAPI::Position fleeVec(InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition() - accum);
		double fleeAngle = atan2(fleeVec.y, fleeVec.x);
		int dist = _order.getRadius() - BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange();
		if (dist <= 0)
			dist = _order.getRadius() *0.8;
		fleeVec = BWAPI::Position(static_cast<int>(dist * cos(fleeAngle)), static_cast<int>(dist * sin(fleeAngle)));
		return accum + fleeVec;
	}
	//else
	////if (accum != _order.getPosition())
	//{
	//	BWAPI::Position fleeVec(InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition() - accum);
	//	double fleeAngle = atan2(fleeVec.y, fleeVec.x);
	//	int dist = BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().minRange();
	//	
	//	//if (dist <= 0)
	//	//	dist = _order.getRadius();
	//	fleeVec = BWAPI::Position(static_cast<int>(dist * cos(fleeAngle)), static_cast<int>(dist * sin(fleeAngle)));
	//	return accum + fleeVec;
	//}
	return BWAPI::Position(accum.x / sizeUnits, accum.y / sizeUnits);
}



BWAPI::Unit Squad::unitClosestToEnemy()
{
	BWAPI::Unit closest = nullptr;
	int closestDist = 100000;

	//if (!closest)
	if (BWAPI::Broodwar->enemy()->getStartLocation() == BWAPI::TilePositions::Invalid
		|| BWAPI::Broodwar->enemy()->getStartLocation() == BWAPI::TilePositions::None
		|| BWAPI::Broodwar->enemy()->getStartLocation() == BWAPI::TilePositions::Unknown)
	{
		closest = nullptr;
	}
	else
	{

		for (auto & unit : _units)
		{
			if (unit->getType() == BWAPI::UnitTypes::Terran_Science_Vessel)
			{
				continue;
			}

			// the distance to the order position
			int dist = unit->getDistance(BWAPI::Position(BWAPI::Broodwar->enemy()->getStartLocation()));

			if (dist != -1 && (nullptr==closest || dist < closestDist))
			{
				closest = unit;
				closestDist = dist;
			}
		}
	}
	if (closest == nullptr)
	{
		for (auto & unit : _units)
		{
			if (unit->getType() == BWAPI::UnitTypes::Terran_Science_Vessel)
			{
				continue;
			}

			// the distance to the order position
			int dist = MapTools::Instance().getGroundDistance(unit->getPosition(), _order.getPosition());

			if (dist != -1 && (nullptr == closest || dist < closestDist))
			{
				closest = unit;
				closestDist = dist;
			}
		}
	}

	return closest;
}

int Squad::squadUnitsNear(BWAPI::Position p)
{
	int numUnits = 0;

	for (auto & unit : _units)
	{
		if (unit->getDistance(p) < 600)
		{
			numUnits++;
		}
	}

	return numUnits;
}

const BWAPI::Unitset & Squad::getUnits() const
{
	return _units;
}

const SquadOrder & Squad::getSquadOrder()	const
{
	return _order;
}

void Squad::addUnit(BWAPI::Unit u)
{
	_units.insert(u);
}

void Squad::removeUnit(BWAPI::Unit u)
{
	_units.erase(u);
}

const std::string & Squad::getName() const
{
	return _name;
}


BWAPI::Unit Squad::unitClosestToEnemyForOrder()
{
	BWAPI::Unit closest = nullptr;
	int closestDist = 100000;

	for (auto & unit : _units)
	{
		if (unit->getType() != BWAPI::UnitTypes::Terran_Medic)
		{
			continue;
		}
		if (unit->getHitPoints() <= 0)
			continue;
		// the distance to the order position
		int dist = MapTools::Instance().getGroundDistance(unit->getPosition(), _order.getPosition());

		if (dist != -1 && (!closest || dist < closestDist))
		{
			closest = unit;
			closestDist = dist;
		}
	}

	if (!closest)
	{
		for (auto & unit : _units)
		{
			if (unit->getType() != BWAPI::UnitTypes::Terran_Medic)
			{
				continue;
			}
			if (unit->getHitPoints() <= 0)
				continue;

			// the distance to the order position
			int dist = unit->getDistance(BWAPI::Position(BWAPI::Broodwar->enemy()->getStartLocation()));

			if (dist != -1 && (!closest || dist < closestDist))
			{
				closest = unit;
				closestDist = dist;
			}
		}
	}
	return closest;
}
