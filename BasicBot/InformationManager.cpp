#include "InformationManager.h"

using namespace MyBot;


InformationManager::InformationManager()
{
	bombDropNum = 2;
	selfPlayer = BWAPI::Broodwar->self();
	enemyPlayer = BWAPI::Broodwar->enemy();

	selfRace = selfPlayer->getRace();
	enemyRace = enemyPlayer->getRace();

	_unitData[selfPlayer] = UnitData();
	_unitData[enemyPlayer] = UnitData();

	_mainBaseLocations[selfPlayer] = BWTA::getStartLocation(BWAPI::Broodwar->self());
	_mainBaseLocationChanged[selfPlayer] = true;
	_occupiedBaseLocations[selfPlayer] = std::list<BWTA::BaseLocation *>();
	_occupiedBaseLocations[selfPlayer].push_back(_mainBaseLocations[selfPlayer]);
	updateOccupiedRegions(BWTA::getRegion(_mainBaseLocations[selfPlayer]->getTilePosition()), BWAPI::Broodwar->self());

	_mainBaseLocations[enemyPlayer] = nullptr;
	_mainBaseLocationChanged[enemyPlayer] = false;
	_occupiedBaseLocations[enemyPlayer] = std::list<BWTA::BaseLocation *>();
	enemyResourceDepotType = enemyRace == BWAPI::Races::Unknown ? BWAPI::UnitTypes::None : getBasicResourceDepotBuildingType(enemyRace);

	_firstChokePoint[selfPlayer] = nullptr;
	_firstChokePoint[enemyPlayer] = nullptr;
	_firstExpansionLocation[selfPlayer] = nullptr;
	_firstExpansionLocation[enemyPlayer] = nullptr;
	_secondChokePoint[selfPlayer] = nullptr;
	_secondChokePoint[enemyPlayer] = nullptr;

	mapName = 'N';
	onStart();
	updateChokePointAndExpansionLocation();

	hasCloakedUnits = false;
	hasFlyingUnits = false;

	//0 러쉬예상, 1 러쉬공격받음
	rushState = 0;
	enemyBlockChoke = nullptr;

	// wall position
	initPositionsForWall();
	_wallStatus = false;
	// 터렛위치 설치여부
	_turretStatus = false;

	isTimingToLiftBaracks = false;
}

//kyj
UIMap & InformationManager::getUnitInfo(BWAPI::Player player)
{
	return getUnitData(player).getUnitAndUnitInfoMap();
}

void InformationManager::onStart(){
	std::string mapFileName = BWAPI::Broodwar->mapFileName();
	for (auto & c : mapFileName) c = std::toupper(c);

	if (mapFileName.find("HUNTER") != std::string::npos){
		mapName = 'H';
	}
	else if (mapFileName.find("SPIRIT") != std::string::npos){
		mapName = 'F';
	}
	else if (mapFileName.find("FIGHT") != std::string::npos){
		mapName = 'F';
	}
	else if (mapFileName.find("LOST") != std::string::npos){
		mapName = 'L';
	}
	else if (mapFileName.find("CIRCUIT") != std::string::npos){
		mapName = 'C';
	}
}


void InformationManager::update() 
{
	updateUnitsInfo();

	// occupiedBaseLocation 이나 occupiedRegion 은 거의 안바뀌므로 자주 안해도 된다
	if (TimeManager::Instance().isMyTurn("InformationManager_update", 120)) {
		updateBaseLocationInfo();
		updateWallStatus();
	}
}

void InformationManager::updateUnitsInfo() 
{
	// update units info
	int numCombatUnitInSelfRegion = 0;

	for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
	{
		//한번만 체크!
		//적 전략 중에 공중 전략 또는 클로킹 전략이 있는지 판단하여 저장
		if (!hasCloakedUnits) enemyHasCloakedUnits(unit);
		if (!hasFlyingUnits) enemyHasFlyingUnits(unit);


		//적이 점령했던 지역을 저장함. 단 업데이트는 어려움.(해처리를 여러개 지을수도 있어서)
		if (unit->getType() == InformationManager::Instance().enemyResourceDepotType){
			BWTA::Region *p_region = BWTA::getRegion(unit->getPosition());
			auto & it = ExpansionManager::Instance().enemyResourceRegions.find(p_region);
			if (it == ExpansionManager::Instance().enemyResourceRegions.end())
			{
				//std::cout << "insert the enemy resource region:" << ExpansionManager::Instance().enemyResourceRegions.size() << std::endl;
				ExpansionManager::Instance().enemyResourceRegions.insert(p_region);
			}
		}

		//매 프레임 적 유닛 위치를 파악하여 우리 지역에 들어왔는지 판단
		if (UnitUtils::IsCombatUnit_rush(unit)){
			if (getMainBaseLocation(selfPlayer)->getRegion()->getPolygon().isInside(unit->getPosition()) ||
				getFirstExpansionLocation(selfPlayer)->getRegion()->getPolygon().isInside(unit->getPosition())){
				numCombatUnitInSelfRegion++;
			}
		}

		updateUnitInfo(unit);
	}

	//적 유닛이 본진+앞마당에 들어오면 러쉬공격받음 으로 세팅
	if (numCombatUnitInSelfRegion > 0){
		rushState = 1;
	}
	else{
		rushState = 0;
	}

	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		updateUnitInfo(unit);
	}

	// remove bad enemy units
	_unitData[enemyPlayer].removeBadUnits();
	_unitData[selfPlayer].removeBadUnits();

	//러쉬 스쿼드 세팅
	rushSquad.clear();
	for (auto & ui : getUnitAndUnitInfoMap(enemyPlayer)){
		if (ui.second.isRushSquad){
			rushSquad.insert(ui.second);
		}
	}
}

// 해당 unit 의 정보를 업데이트 한다 (UnitType, lastPosition, HitPoint 등)
void InformationManager::updateUnitInfo(BWAPI::Unit unit)
{
    if (unit->getPlayer() != selfPlayer && unit->getPlayer() != enemyPlayer) {
        return;
    }

	if (enemyRace == BWAPI::Races::Unknown && unit->getPlayer() == enemyPlayer) {
		enemyRace = unit->getType().getRace();
		enemyResourceDepotType = getBasicResourceDepotBuildingType(enemyRace);
	}

	//적 유닛에 대해서만 판단
	bool isRushSquad = false;
	if (unit->getPlayer() == enemyPlayer && UnitUtils::IsCombatUnit_rush(unit)){
		// 적진 찾기 전에면 false
		// 적 앞마당 이후에는 본진과 앞마당을 제외한 곳에서 발견된 유닛이 러쉬스쿼드에 들어간다.
		if (getMainBaseLocation(enemyPlayer) != nullptr){
			if (ExpansionManager::Instance().enemyResourceRegions.size() > 1 || getFirstExpansionLocation(enemyPlayer) != nullptr){
				if (!getMainBaseLocation(enemyPlayer)->getRegion()->getPolygon().isInside(unit->getPosition()) &&
					!getFirstExpansionLocation(enemyPlayer)->getRegion()->getPolygon().isInside(unit->getPosition())){
					isRushSquad = true;
				}
			}
			else{
				if (!getMainBaseLocation(enemyPlayer)->getRegion()->getPolygon().isInside(unit->getPosition())){
					isRushSquad = true;
				}
			}
		}
	}

    _unitData[unit->getPlayer()].updateUnitInfo(unit, isRushSquad);
}

// 유닛이 파괴/사망한 경우, 해당 유닛 정보를 삭제한다
void InformationManager::onUnitDestroy(BWAPI::Unit unit)
{
	if (unit == nullptr) { 
		return; 
	}

    if (unit->getType().isNeutral())
    {
        return;
    }

    _unitData[unit->getPlayer()].removeUnit(unit);

	// wall 구성 unit이 제거 되었을 때 해당 unit을 unitset에서 삭제
	bool wallFlag = false;

	if ((unit->getType() == BWAPI::UnitTypes::Terran_Supply_Depot || unit->getType() == BWAPI::UnitTypes::Terran_Barracks)) {
		for (auto wallUnit : _wallUnits) {
			if (wallUnit->getID() == unit->getID()) {
				wallFlag = true;
				break;
			}
		}
	}

	if (wallFlag == true) {
		for (auto it = _wallUnits.begin(); it != _wallUnits.end();) {
			BWAPI::Unit wallUnit = *it;

			if (wallUnit == nullptr) {
				it = _wallUnits.erase(it);
			}
			else {
				if (wallUnit->getType().isBuilding() == false || wallUnit->getPlayer() != BWAPI::Broodwar->self() || wallUnit->exists() == false || wallUnit->getHitPoints() == 0) {
					it = _wallUnits.erase(it);
				}
				else {
					it++;
				}
			}
		}

		_wallUnits.erase(unit);
	}
}

bool InformationManager::isCombatUnitType(BWAPI::UnitType type) const
{
	if (type == BWAPI::UnitTypes::Zerg_Lurker/* || type == BWAPI::UnitTypes::Protoss_Dark_Templar*/)
	{
		return false;
	}

	// check for various types of combat units
	if (type.canAttack() || 
		type == BWAPI::UnitTypes::Terran_Medic || 
		type == BWAPI::UnitTypes::Protoss_Observer ||
        type == BWAPI::UnitTypes::Terran_Bunker)
	{
		return true;
	}
		
	return false;
}

void InformationManager::getNearbyForce(std::vector<UnitInfo> & unitInfo, BWAPI::Position p, BWAPI::Player player, int radius) 
{
	// for each unit we know about for that player
	for (const auto & kv : getUnitData(player).getUnitAndUnitInfoMap())
	{
		const UnitInfo & ui(kv.second);

		// if it's a combat unit we care about
		// and it's finished! 
		if (isCombatUnitType(ui.type) && ui.completed)
		{
			// determine its attack range
			int range = 0;
			if (ui.type.groundWeapon() != BWAPI::WeaponTypes::None)
			{
				range = ui.type.groundWeapon().maxRange() + 40;
			}

			// if it can attack into the radius we care about
			if (ui.lastPosition.getDistance(p) <= (radius + range))
			{
				// add it to the vector
				unitInfo.push_back(ui);
			}
		}
		else if (ui.type.isDetector() && ui.lastPosition.getDistance(p) <= (radius + 250))
        {
			// add it to the vector
			unitInfo.push_back(ui);
        }
	}
}


int InformationManager::getNumUnits(BWAPI::UnitType t, BWAPI::Player player)
{
	return getUnitData(player).getNumUnits(t);
}


UnitData & InformationManager::getUnitData(BWAPI::Player player)
{
    return _unitData.find(player)->second;
}


InformationManager & InformationManager::Instance()
{
	static InformationManager instance;
	return instance;
}


void InformationManager::updateBaseLocationInfo()
{
	_occupiedRegions[selfPlayer].clear();
	_occupiedRegions[enemyPlayer].clear();
	_occupiedBaseLocations[selfPlayer].clear();
	_occupiedBaseLocations[enemyPlayer].clear();

	// enemy 의 startLocation을 아직 모르는 경우
	if (_mainBaseLocations[enemyPlayer] == nullptr) {

		// how many start locations have we explored
		int exploredStartLocations = 0;
		bool enemyStartLocationFound = false;

		// an unexplored base location holder
		BWTA::BaseLocation * unexplored = nullptr;

		for (BWTA::BaseLocation * startLocation : BWTA::getStartLocations())
		{
			if (existsPlayerBuildingInRegion(BWTA::getRegion(startLocation->getTilePosition()), enemyPlayer))
			{
				if (enemyStartLocationFound == false) {
					enemyStartLocationFound = true;
					_mainBaseLocations[enemyPlayer] = startLocation;
					_mainBaseLocationChanged[enemyPlayer] = true;
				}
			}

			// if it's explored, increment
			if (BWAPI::Broodwar->isExplored(startLocation->getTilePosition()))
			{
				exploredStartLocations++;

			}
			// otherwise set it as unexplored base
			else
			{
				unexplored = startLocation;
			}
		}

		// if we've explored every start location except one, it's the enemy
		if (!enemyStartLocationFound && exploredStartLocations == ((int)BWTA::getStartLocations().size() - 1))
		{
			enemyStartLocationFound = true;
			_mainBaseLocations[enemyPlayer] = unexplored;
			_mainBaseLocationChanged[enemyPlayer] = true;
			_occupiedBaseLocations[enemyPlayer].push_back(unexplored);
		}
	}

	// update occupied base location
	// 어떤 Base Location 에는 아군 건물, 적군 건물 모두 혼재해있어서 동시에 여러 Player 가 Occupy 하고 있는 것으로 판정될 수 있다
	for (BWTA::BaseLocation * baseLocation : BWTA::getBaseLocations())
	{
		if (hasBuildingAroundBaseLocation(baseLocation, enemyPlayer))
		{
			_occupiedBaseLocations[enemyPlayer].push_back(baseLocation);
		}

		if (hasBuildingAroundBaseLocation(baseLocation, selfPlayer))
		{
			_occupiedBaseLocations[selfPlayer].push_back(baseLocation);
		}
	}

	// enemy의 mainBaseLocations을 발견한 후, 그곳에 있는 건물을 모두 파괴한 경우 _occupiedBaseLocations 중에서 _mainBaseLocations 를 선정한다
	if (_mainBaseLocations[enemyPlayer] != nullptr) {
		if (existsPlayerBuildingInRegion(BWTA::getRegion(_mainBaseLocations[enemyPlayer]->getTilePosition()), enemyPlayer) == false)
		{
			for (std::list<BWTA::BaseLocation*>::const_iterator iterator = _occupiedBaseLocations[enemyPlayer].begin(), end = _occupiedBaseLocations[enemyPlayer].end(); iterator != end; ++iterator) {
				if (existsPlayerBuildingInRegion(BWTA::getRegion((*iterator)->getTilePosition()), enemyPlayer) == true) {
					_mainBaseLocations[enemyPlayer] = *iterator;
					_mainBaseLocationChanged[enemyPlayer] = true;
					break;
				}
			}
		}
	}

	// self의 mainBaseLocations에 대해, 그곳에 있는 건물이 모두 파괴된 경우 _occupiedBaseLocations 중에서 _mainBaseLocations 를 선정한다
	if (_mainBaseLocations[selfPlayer] != nullptr) {
		if (existsPlayerBuildingInRegion(BWTA::getRegion(_mainBaseLocations[selfPlayer]->getTilePosition()), selfPlayer) == false)
		{
			for (std::list<BWTA::BaseLocation*>::const_iterator iterator = _occupiedBaseLocations[selfPlayer].begin(), end = _occupiedBaseLocations[selfPlayer].end(); iterator != end; ++iterator) {
				if (existsPlayerBuildingInRegion(BWTA::getRegion((*iterator)->getTilePosition()), selfPlayer) == true) {
					_mainBaseLocations[selfPlayer] = *iterator;
					_mainBaseLocationChanged[selfPlayer] = true;
					break;
				}
			}
		}
	}

	// for each enemy building unit we know about
	for (const auto & kv : _unitData[enemyPlayer].getUnitAndUnitInfoMap())
	{
		const UnitInfo & ui(kv.second);
		if (ui.type.isBuilding())
		{
			updateOccupiedRegions(BWTA::getRegion(BWAPI::TilePosition(ui.lastPosition)), BWAPI::Broodwar->enemy());
		}
	}
	// for each of our building units
	for (const auto & kv : _unitData[selfPlayer].getUnitAndUnitInfoMap())
	{
		const UnitInfo & ui(kv.second);
		if (ui.type.isBuilding())
		{
			updateOccupiedRegions(BWTA::getRegion(BWAPI::TilePosition(ui.lastPosition)), BWAPI::Broodwar->self());
		}
	}

	updateChokePointAndExpansionLocation();
}

void InformationManager::updateChokePointAndExpansionLocation()
{
	if (_mainBaseLocationChanged[selfPlayer] == true) {

		if (_mainBaseLocations[selfPlayer]) {

			BWTA::BaseLocation* sourceBaseLocation = _mainBaseLocations[selfPlayer];

			_firstChokePoint[selfPlayer] = BWTA::getNearestChokepoint(sourceBaseLocation->getTilePosition());

			double tempDistance;
			double closestDistance = 1000000000;
			for (BWTA::BaseLocation * targetBaseLocation : BWTA::getBaseLocations())
			{
				if (targetBaseLocation == _mainBaseLocations[selfPlayer]) continue;

				// BWTA::getGroundDistance 연산량이 많아 교체
				//tempDistance = BWTA::getGroundDistance(sourceBaseLocation->getTilePosition(), targetBaseLocation->getTilePosition());
				tempDistance = MapTools::Instance().getGroundDistance(sourceBaseLocation->getPosition(), targetBaseLocation->getPosition());
				
				if (tempDistance < closestDistance && tempDistance > 0) {
					closestDistance = tempDistance;
					_firstExpansionLocation[selfPlayer] = targetBaseLocation;
				}
			}

			closestDistance = 1000000000;
			for (BWTA::Chokepoint * chokepoint : BWTA::getChokepoints())
			{
				if (chokepoint == _firstChokePoint[selfPlayer]) continue;
				// BWTA::getGroundDistance 연산량이 많아 교체
				//tempDistance = BWTA::getGroundDistance(sourceBaseLocation->getTilePosition(), BWAPI::TilePosition(chokepoint->getCenter()));
				tempDistance = MapTools::Instance().getGroundDistance(sourceBaseLocation->getPosition(), chokepoint->getCenter());

				if (tempDistance < closestDistance && tempDistance > 0) {
					closestDistance = tempDistance;
					_secondChokePoint[selfPlayer] = chokepoint;
				}
			}
			if (getMapName() == 'H' && 3 == getPostionAtHunter())
			{
				closestDistance = 1000000000;
				BWTA::Chokepoint *temp;
				for (BWTA::Chokepoint * chokepoint : BWTA::getChokepoints())
				{
					if (chokepoint == _firstChokePoint[selfPlayer]) continue;
					if (chokepoint == _secondChokePoint[selfPlayer]) continue;

					// BWTA::getGroundDistance 연산량이 많아 교체
					tempDistance = MapTools::Instance().getGroundDistance(sourceBaseLocation->getPosition(), chokepoint->getCenter());
					if (tempDistance < closestDistance && tempDistance > 0) {
						closestDistance = tempDistance;
						temp = chokepoint;
					}
				}
				_firstChokePoint[selfPlayer] = _secondChokePoint[selfPlayer];
				_secondChokePoint[selfPlayer] = temp;

			}
		}
		_mainBaseLocationChanged[selfPlayer] = false;
	}
	
	if (_mainBaseLocationChanged[enemyPlayer] == true) {
		if (_mainBaseLocations[enemyPlayer]) {

			BWTA::BaseLocation* sourceBaseLocation = _mainBaseLocations[enemyPlayer];

			_firstChokePoint[enemyPlayer] = BWTA::getNearestChokepoint(sourceBaseLocation->getTilePosition());

			double tempDistance;
			double closestDistance = 1000000000;
			for (BWTA::BaseLocation * targetBaseLocation : BWTA::getBaseLocations())
			{
				if (targetBaseLocation == _mainBaseLocations[enemyPlayer]) continue;

				// 연산량을 줄이기위해 교체
				//tempDistance = BWTA::getGroundDistance(sourceBaseLocation->getTilePosition(), targetBaseLocation->getTilePosition());
				tempDistance = MapTools::Instance().getGroundDistance(sourceBaseLocation->getPosition(), targetBaseLocation->getPosition());
				if (tempDistance < closestDistance && tempDistance > 0) {
					closestDistance = tempDistance;
					_firstExpansionLocation[enemyPlayer] = targetBaseLocation;
				}
			}

			closestDistance = 1000000000;
			for (BWTA::Chokepoint * chokepoint : BWTA::getChokepoints())
			{
				if (chokepoint == _firstChokePoint[enemyPlayer]) continue;

				// 연산량 줄이기 위해 교체
				//tempDistance = BWTA::getGroundDistance(sourceBaseLocation->getTilePosition(), BWAPI::TilePosition(chokepoint->getCenter()));
				tempDistance = MapTools::Instance().getGroundDistance(sourceBaseLocation->getPosition(), chokepoint->getCenter());

				if (tempDistance < closestDistance && tempDistance > 0) {
					closestDistance = tempDistance;
					_secondChokePoint[enemyPlayer] = chokepoint;
				}
			}
		}
		_mainBaseLocationChanged[enemyPlayer] = false;
	}
}


void InformationManager::updateOccupiedRegions(BWTA::Region * region, BWAPI::Player player)
{
	// if the region is valid (flying buildings may be in nullptr regions)
	if (region)
	{
		// add it to the list of occupied regions
		_occupiedRegions[player].insert(region);
	}
}

// BaseLocation 주위 원 안에 player의 건물이 있으면 true 를 반환한다
bool InformationManager::hasBuildingAroundBaseLocation(BWTA::BaseLocation * baseLocation, BWAPI::Player player, int radius)
{
	// invalid regions aren't considered the same, but they will both be null
	if (!baseLocation)
	{
		return false;
	}

	for (const auto & kv : _unitData[player].getUnitAndUnitInfoMap())
	{
		const UnitInfo & ui(kv.second);
		if (ui.type.isBuilding())
		{
			BWAPI::TilePosition buildingPosition(ui.lastPosition);
			if (BWTA::getRegion(buildingPosition) != BWTA::getRegion(baseLocation->getTilePosition())) continue;
			if (buildingPosition.x >= baseLocation->getTilePosition().x - radius && buildingPosition.x <= baseLocation->getTilePosition().x + radius
				&& buildingPosition.y >= baseLocation->getTilePosition().y - radius && buildingPosition.y <= baseLocation->getTilePosition().y + radius)
			{
				return true;
			}
		}
	}
	return false;
}

bool InformationManager::existsPlayerBuildingInRegion(BWTA::Region * region, BWAPI::Player player)
{
	// invalid regions aren't considered the same, but they will both be null
	if (region == nullptr || player == nullptr)
	{
		return false;
	}

	for (const auto & kv : _unitData[player].getUnitAndUnitInfoMap())
	{
		const UnitInfo & ui(kv.second);
		if (ui.type.isBuilding())
		{
			if (BWTA::getRegion(BWAPI::TilePosition(ui.lastPosition)) == region)
			{
				return true;
			}
		}
	}

	return false;
}

// 해당 Player 의 UnitAndUnitInfoMap 을 갖고온다
UnitAndUnitInfoMap & InformationManager::getUnitAndUnitInfoMap(BWAPI::Player player)
{
	return getUnitData(player).getUnitAndUnitInfoMap();
}

std::set<BWTA::Region *> & InformationManager::getOccupiedRegions(BWAPI::Player player)
{
	return _occupiedRegions[player];
}

std::list<BWTA::BaseLocation *> & InformationManager::getOccupiedBaseLocations(BWAPI::Player player)
{
	return _occupiedBaseLocations[player];
}

BWTA::BaseLocation * InformationManager::getMainBaseLocation(BWAPI::Player player)
{
	return _mainBaseLocations[player];
}

BWTA::Chokepoint * InformationManager::getFirstChokePoint(BWAPI::Player player)
{
	return _firstChokePoint[player];
}
BWTA::BaseLocation * InformationManager::getFirstExpansionLocation(BWAPI::Player player)
{
	return _firstExpansionLocation[player];
}

BWTA::Chokepoint * InformationManager::getSecondChokePoint(BWAPI::Player player)
{
	return _secondChokePoint[player];
}



BWAPI::UnitType InformationManager::getBasicCombatUnitType(BWAPI::Race race)
{
	if (race == BWAPI::Races::None) {
		race = selfRace;
	}

	if (race == BWAPI::Races::Protoss) {
		return BWAPI::UnitTypes::Protoss_Zealot;
	}
	else if (race == BWAPI::Races::Terran) {
		return BWAPI::UnitTypes::Terran_Marine;
	}
	else if (race == BWAPI::Races::Zerg) {
		return BWAPI::UnitTypes::Zerg_Zergling;
	}
	else {
		return BWAPI::UnitTypes::None;
	}
}

BWAPI::UnitType InformationManager::getAdvancedCombatUnitType(BWAPI::Race race)
{
	if (race == BWAPI::Races::None) {
		race = selfRace;
	}

	if (race == BWAPI::Races::Protoss) {
		return BWAPI::UnitTypes::Protoss_Dragoon;
	}
	else if (race == BWAPI::Races::Terran) {
		return BWAPI::UnitTypes::Terran_Medic;
	}
	else if (race == BWAPI::Races::Zerg) {
		return BWAPI::UnitTypes::Zerg_Hydralisk;
	}
	else {
		return BWAPI::UnitTypes::None;
	}
}

BWAPI::UnitType InformationManager::getBasicCombatBuildingType(BWAPI::Race race)
{
	if (race == BWAPI::Races::None) {
		race = selfRace;
	}

	if (race == BWAPI::Races::Protoss) {
		return BWAPI::UnitTypes::Protoss_Gateway;
	}
	else if (race == BWAPI::Races::Terran) {
		return BWAPI::UnitTypes::Terran_Barracks;
	}
	else if (race == BWAPI::Races::Zerg) {
		return BWAPI::UnitTypes::Zerg_Hatchery;
	}
	else {
		return BWAPI::UnitTypes::None;
	}
}

BWAPI::UnitType InformationManager::getObserverUnitType(BWAPI::Race race)
{
	if (race == BWAPI::Races::None) {
		race = selfRace;
	}

	if (race == BWAPI::Races::Protoss) {
		return BWAPI::UnitTypes::Protoss_Observer;
	}
	else if (race == BWAPI::Races::Terran) {
		return BWAPI::UnitTypes::Terran_Science_Vessel;
	}
	else if (race == BWAPI::Races::Zerg) {
		return BWAPI::UnitTypes::Zerg_Overlord;
	}
	else {
		return BWAPI::UnitTypes::None;
	}
}

BWAPI::UnitType	InformationManager::getBasicResourceDepotBuildingType(BWAPI::Race race)
{
	if (race == BWAPI::Races::None) {
		race = selfRace;
	}

	if (race == BWAPI::Races::Protoss) {
		return BWAPI::UnitTypes::Protoss_Nexus;
	}
	else if (race == BWAPI::Races::Terran) {
		return BWAPI::UnitTypes::Terran_Command_Center;
	}
	else if (race == BWAPI::Races::Zerg) {
		return BWAPI::UnitTypes::Zerg_Hatchery;
	}
	else {
		return BWAPI::UnitTypes::None;
	}
}
BWAPI::UnitType InformationManager::getRefineryBuildingType(BWAPI::Race race)
{
	if (race == BWAPI::Races::None) {
		race = selfRace;
	}

	if (race == BWAPI::Races::Protoss) {
		return BWAPI::UnitTypes::Protoss_Assimilator;
	}
	else if (race == BWAPI::Races::Terran) {
		return BWAPI::UnitTypes::Terran_Refinery;
	}
	else if (race == BWAPI::Races::Zerg) {
		return BWAPI::UnitTypes::Zerg_Extractor;
	}
	else {
		return BWAPI::UnitTypes::None;
	}

}

BWAPI::UnitType	InformationManager::getWorkerType(BWAPI::Race race)
{
	if (race == BWAPI::Races::None) {
		race = selfRace;
	}

	if (race == BWAPI::Races::Protoss) {
		return BWAPI::UnitTypes::Protoss_Probe;
	}
	else if (race == BWAPI::Races::Terran) {
		return BWAPI::UnitTypes::Terran_SCV;
	}
	else if (race == BWAPI::Races::Zerg) {
		return BWAPI::UnitTypes::Zerg_Drone;
	}
	else {
		return BWAPI::UnitTypes::None;
	}
}

BWAPI::UnitType InformationManager::getBasicSupplyProviderUnitType(BWAPI::Race race)
{
	if (race == BWAPI::Races::None) {
		race = selfRace;
	}

	if (race == BWAPI::Races::Protoss) {
		return BWAPI::UnitTypes::Protoss_Pylon;
	}
	else if (race == BWAPI::Races::Terran) {
		return BWAPI::UnitTypes::Terran_Supply_Depot;
	}
	else if (race == BWAPI::Races::Zerg) {
		return BWAPI::UnitTypes::Zerg_Overlord;
	}
	else {
		return BWAPI::UnitTypes::None;
	}
}

BWAPI::UnitType InformationManager::getBasicDefenseBuildingType(BWAPI::Race race)
{
	if (race == BWAPI::Races::None) {
		race = selfRace;
	}

	if (race == BWAPI::Races::Protoss) {
		return BWAPI::UnitTypes::Protoss_Pylon;
	}
	else if (race == BWAPI::Races::Terran) {
		return BWAPI::UnitTypes::Terran_Bunker;
	}
	else if (race == BWAPI::Races::Zerg) {
		return BWAPI::UnitTypes::Zerg_Creep_Colony;
	}
	else {
		return BWAPI::UnitTypes::None;
	}
}

BWAPI::UnitType InformationManager::getAdvancedDefenseBuildingType(BWAPI::Race race)
{
	if (race == BWAPI::Races::None) {
		race = selfRace;
	}

	if (race == BWAPI::Races::Protoss) {
		return BWAPI::UnitTypes::Protoss_Photon_Cannon;
	}
	else if (race == BWAPI::Races::Terran) {
		return BWAPI::UnitTypes::Terran_Missile_Turret;
	}
	else if (race == BWAPI::Races::Zerg) {
		return BWAPI::UnitTypes::Zerg_Sunken_Colony;
	}
	else {
		return BWAPI::UnitTypes::None;
	}
}

void InformationManager::enemyHasCloakedUnits(BWAPI::Unit u)
{
	//@도주남 kyj 레이스, 고스트
	//@도주남 kyj 닥템, 옵저버
	//@도주남 kyj lurker
	if (u->getType().isCloakable() || 
		u->getType().hasPermanentCloak() ||
		u->getType() == BWAPI::UnitTypes::Zerg_Lurker)
	{
		hasCloakedUnits = true;
		return;
	}


	// assume they're going dts
	if (u->getType() == BWAPI::UnitTypes::Protoss_Citadel_of_Adun ||
		u->getType() == BWAPI::UnitTypes::Protoss_Observatory ||
		u->getType() == BWAPI::UnitTypes::Protoss_Templar_Archives ||
		u->getType() == BWAPI::UnitTypes::Protoss_Arbiter_Tribunal)
	{
		hasCloakedUnits = true;
		return;
	}

	if (u->getType() == BWAPI::UnitTypes::Terran_Control_Tower ||
		u->getType() == BWAPI::UnitTypes::Terran_Covert_Ops)
	{
		hasCloakedUnits = true;
		return;
	}
}

void InformationManager::enemyHasFlyingUnits(BWAPI::Unit u){
	if (u->getType().isFlyer() &&
		u->getType() != BWAPI::UnitTypes::Zerg_Overlord){
		hasFlyingUnits = true;
		return;
	}

	if (u->getType() == BWAPI::UnitTypes::Zerg_Mutalisk ||
		u->getType() == BWAPI::UnitTypes::Zerg_Spire){
		hasFlyingUnits = true;
		return;
	}

	if (u->getType() == BWAPI::UnitTypes::Protoss_Stargate ||
		u->getType() == BWAPI::UnitTypes::Protoss_Scout ||
		u->getType() == BWAPI::UnitTypes::Protoss_Carrier){
		hasFlyingUnits = true;
		return;
	}
}

char InformationManager::getMapName(){
	return mapName;
}

std::set<UnitInfo> & InformationManager::getRushSquad(){
	return rushSquad;
}


void InformationManager::setCombatStatus(combatStatus cs){
	if (cs != nowCombatStatus){
		changeConmgeStatusFrame = BWAPI::Broodwar->getFrameCount();
		lastCombatStatus = nowCombatStatus;
		nowCombatStatus = cs;
	}
}

BWAPI::Position InformationManager::getCurrentCombatOrderPosition()
{
	return currentCombatOrderPosition;
}

int InformationManager::getPostionAtHunter(){
	BWAPI::Position p_11 = BWAPI::Position(384, 240);
	BWAPI::Position p_12 = BWAPI::Position(2304, 304);
	BWAPI::Position p_1 = BWAPI::Position(3680, 304);
    BWAPI::Position p_3 = BWAPI::Position(3712, 2608);	
	BWAPI::Position p_5 = BWAPI::Position(3712, 3760);
	BWAPI::Position p_6 = BWAPI::Position(2080, 3792);
	BWAPI::Position p_7 = BWAPI::Position(384, 3728);
	BWAPI::Position p_9 = BWAPI::Position(320, 1552);

	BWAPI::Position home(BWTA::getStartLocation(InformationManager::Instance().selfPlayer)->getPosition());
	if (p_11 == home)
		return 11;
	if (p_12 == home)
		return 12;
	if (p_1 == home)
		return 1;
	if (p_3 == home)
		return 3;
	if (p_5 == home)
		return 5;
	if (p_6 == home)
		return 6;
	if (p_7 == home)
		return 7;
	if (p_9 == home)
		return 9;

	return 0;
}

BWAPI::Position InformationManager::getPostionAtHuntersecond(){

	int pos_t = getPostionAtHunter();
	if (11 == pos_t)
		return  BWAPI::Position(52 * 32, 28 * 32);
	else if (12 == pos_t)
		return BWAPI::Position(52 * 32, 28 * 32);
	else if (1 == pos_t)
		return BWAPI::Position(94 * 32, 32 * 32);
	else if (3 == pos_t)
		return BWAPI::Position(85 * 32, 60 * 32);
	else if (5 == pos_t)
		return BWAPI::Position(84 * 32, 83 * 32);
	else if (6 == pos_t)
		return BWAPI::Position(44 * 32, 82 * 32);
	else if (7 == pos_t)
		return BWAPI::Position(34 * 32, 77 * 32);
	else if (9 == pos_t)
		return BWAPI::Position(43 * 32, 54 * 32);
}

BWAPI::Position InformationManager::getPostionAtHunterfirst(){

	int pos_t = getPostionAtHunter();
	if (11 == pos_t)
		return  BWAPI::Position(29 * 32, 20 * 32);
	else if (12 == pos_t)
		return BWAPI::Position(56 * 32, 21 * 32);
	else if (1 == pos_t)
		return BWAPI::Position(98 * 32, 23 * 32);
	else if (3 == pos_t)
		return BWAPI::Position(104 * 32, 62 * 32);
	else if (5 == pos_t)
		return BWAPI::Position(99 * 32, 102 * 32);
	else if (6 == pos_t)
		return BWAPI::Position(57 * 32, 102 * 32);
	else if (7 == pos_t)
		return BWAPI::Position(15 * 32, 98 * 32);
	else if (9 == pos_t)
		return BWAPI::Position(23 * 32, 56 * 32);
}

bool InformationManager::isBlockedEnemyChoke(){
	if (getMainBaseLocation(enemyPlayer) == nullptr){
		enemyBlockChoke = nullptr;
	}
	else{
		if (enemyBlockChoke == nullptr){
			if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Terran){
				int num1stChokeUnit_supply = 0;
				int num1stChokeUnit_barrack = 0;

				for (auto u : getUnitAndUnitInfoMap(enemyPlayer)){
					if (u.second.type.isBuilding()){
						if (u.second.type == BWAPI::UnitTypes::Terran_Supply_Depot){
							if (u.second.lastPosition.getDistance(getFirstChokePoint(enemyPlayer)->getCenter()) < 120){
								num1stChokeUnit_supply++;
							}
						}
						else if (u.second.type == BWAPI::UnitTypes::Terran_Barracks){
							if (u.second.lastPosition.getDistance(getFirstChokePoint(enemyPlayer)->getCenter()) < 120){
								num1stChokeUnit_barrack++;
							}
						}

						//포지1+캐논1 || 캐논3 || 캐논1+게이트1
						if (num1stChokeUnit_supply > 0 || num1stChokeUnit_barrack > 0){
							enemyBlockChoke = getFirstChokePoint(enemyPlayer);
						}
					}
				}
			}
			else if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Zerg){

			}
			else if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Protoss){
				int num1stChokeUnit = 0;
				int num2stChokeUnit = 0;
				int num1stChokeUnit_gateway = 0;
				int num2stChokeUnit_gateway = 0;
				int numOther = 0;

				for (auto u : getUnitAndUnitInfoMap(enemyPlayer)){
					if (u.second.type.isBuilding()){
						if (u.second.type == BWAPI::UnitTypes::Protoss_Photon_Cannon){
							if (u.second.lastPosition.getDistance(getFirstChokePoint(enemyPlayer)->getCenter()) < 160){
								num1stChokeUnit++;
							}
							else if (u.second.lastPosition.getDistance(getSecondChokePoint(enemyPlayer)->getCenter()) < 160){
								num2stChokeUnit++;
							}
							else{
								numOther++;
							}
						}
						else if (u.second.type == BWAPI::UnitTypes::Protoss_Forge){
							if (u.second.lastPosition.getDistance(getFirstChokePoint(enemyPlayer)->getCenter()) < 160){
								num1stChokeUnit+=2;
							}
							if (u.second.lastPosition.getDistance(getSecondChokePoint(enemyPlayer)->getCenter()) < 160){
								num2stChokeUnit+=2;
							}
						}
						else if (u.second.type == BWAPI::UnitTypes::Protoss_Gateway){
							if (u.second.lastPosition.getDistance(getFirstChokePoint(enemyPlayer)->getCenter()) < 160){
								num1stChokeUnit_gateway++;
							}
							if (u.second.lastPosition.getDistance(getSecondChokePoint(enemyPlayer)->getCenter()) < 160){
								num2stChokeUnit_gateway++;
							}
						}

						//포지1+캐논1 || 캐논3 || 캐논1+게이트1
						if (num2stChokeUnit > 2 || (num2stChokeUnit > 0 && num2stChokeUnit_gateway > 0)){
							enemyBlockChoke = getFirstChokePoint(enemyPlayer);
						}
						else if (num1stChokeUnit > 2 || (num1stChokeUnit > 0 && num1stChokeUnit_gateway > 0)){
							enemyBlockChoke = getSecondChokePoint(enemyPlayer);
						}
						//포지1+캐논3 || 캐논5 -> 너무 많으면.. 방어적인 전략으로 판단
						else if(numOther > 4){
							enemyBlockChoke = getFirstChokePoint(enemyPlayer);
						}
					}
				}
			}
			else{
				enemyBlockChoke = nullptr;
			}
		}
	}

	if (enemyBlockChoke == nullptr){
		return false;
	}
	else{
		return true;
	}
}

std::queue<BWAPI::TilePosition>& InformationManager::getSupPostionsForWall(){
	return _supPositionsForWall;
}

BWAPI::TilePosition InformationManager::getBarPostionsForWall() {
	return _barPositionForWall;
}

std::queue<BWAPI::TilePosition>& InformationManager::getReservedSupPositions(){
	return _reservedSupPositions;
}

void InformationManager::initPositionsForWall() {

	BWTA::BaseLocation *base = getMainBaseLocation(BWAPI::Broodwar->self());

	// lost temple
	if (getMapName() == 'L') {
		if (base->getTilePosition().x == 27 && base->getTilePosition().y == 118) {
			// 6
			// 58, 106
			// 55, 106

			_supPositionsForWall.push(BWAPI::TilePosition(57, 106));
			_supPositionsForWall.push(BWAPI::TilePosition(54, 106));

			_barPositionForWall = BWAPI::TilePosition(52, 108);
		}
		else if (base->getTilePosition().x == 7 && base->getTilePosition().y == 87) {
			// 9
			// 20, 62
			_supPositionsForWall.push(BWAPI::TilePosition(19, 62));

			_barPositionForWall = BWAPI::TilePosition(17, 64);

		}
		else if (base->getTilePosition().x == 57 && base->getTilePosition().y == 6) {
			// 12
			// 82, 6
			// 79, 7
			_supPositionsForWall.push(BWAPI::TilePosition(81, 6));
			_supPositionsForWall.push(BWAPI::TilePosition(78, 6));

			_barPositionForWall = BWAPI::TilePosition(76, 8);

		}
		else {
			// 2
			// 114, 52
			// 117, 52
			_supPositionsForWall.push(BWAPI::TilePosition(113, 51));
			_supPositionsForWall.push(BWAPI::TilePosition(116, 51));

			_barPositionForWall = BWAPI::TilePosition(118, 53);
		}
	}
	// fighting
	// 7, 116 - 7
	// 7, 6 - 11
	// 117, 117 - 5
	// 117, 7 - 1
	else if (getMapName() == 'F') {
		if (base->getTilePosition().x == 7 && base->getTilePosition().y == 116) {
			// 7
			_supPositionsForWall.push(BWAPI::TilePosition(22, 118));
			_supPositionsForWall.push(BWAPI::TilePosition(27, 121));
			_barPositionForWall = BWAPI::TilePosition(23, 120);
			_turretPosition = BWAPI::TilePosition(21, 117);
			_bunkerPosition = BWAPI::TilePosition(34, 109);
			std::vector<BWAPI::TilePosition> startPoints;
			std::vector<int> strides;
			startPoints.push_back(BWAPI::TilePosition(0, 96));
			startPoints.push_back(BWAPI::TilePosition(3, 97));
			startPoints.push_back(BWAPI::TilePosition(6, 99));
			startPoints.push_back(BWAPI::TilePosition(9, 100));
			strides.push_back(8);
			strides.push_back(6);
			strides.push_back(5);
			strides.push_back(4);
			for (int i = 0; i < startPoints.size(); i++) {
				
				for (int j = 0; j < strides[i]; j++) {
					BWAPI::TilePosition target = startPoints[i] + BWAPI::TilePosition(0, 2 * j);
					_reservedSupPositions.push(target);
					ConstructionPlaceFinder::Instance().reserveTiles(target, 3, 2);
				}
			}
		}
		else if (base->getTilePosition().x == 7 && base->getTilePosition().y == 6) {
			// 11
			_supPositionsForWall.push(BWAPI::TilePosition(10, 26));
			_supPositionsForWall.push(BWAPI::TilePosition(7, 26));
			_barPositionForWall = BWAPI::TilePosition(4, 28);
			_turretPosition = BWAPI::TilePosition(10, 25);
			_bunkerPosition = BWAPI::TilePosition(12, 36);

			std::vector<BWAPI::TilePosition> startPoints;
			std::vector<int> strides;
			startPoints.push_back(BWAPI::TilePosition(20, 0));
			startPoints.push_back(BWAPI::TilePosition(17, 0));
			startPoints.push_back(BWAPI::TilePosition(14, 0));
			startPoints.push_back(BWAPI::TilePosition(23, 10));
			strides.push_back(6);
			strides.push_back(6);
			strides.push_back(6);
			strides.push_back(4);
			
			for (int i = 0; i < startPoints.size(); i++) {

				for (int j = 0; j < strides[i]; j++) {
					BWAPI::TilePosition target = startPoints[i] + BWAPI::TilePosition(0, 2 * j);
					_reservedSupPositions.push(target);
					ConstructionPlaceFinder::Instance().reserveTiles(target, 3, 2);
				}
			}
		}
		else if (base->getTilePosition().x == 117 && base->getTilePosition().y == 117) {
			// 5
			_supPositionsForWall.push(BWAPI::TilePosition(118, 98));
			_supPositionsForWall.push(BWAPI::TilePosition(118, 100));
			_barPositionForWall = BWAPI::TilePosition(114, 101);
			_turretPosition = BWAPI::TilePosition(119, 98);
			_bunkerPosition = BWAPI::TilePosition(108, 91);

			std::vector<BWAPI::TilePosition> startPoints;
			std::vector<int> strides;
			startPoints.push_back(BWAPI::TilePosition(104, 124));
			startPoints.push_back(BWAPI::TilePosition(104, 122));
			startPoints.push_back(BWAPI::TilePosition(104, 120));
			startPoints.push_back(BWAPI::TilePosition(104, 118));
			startPoints.push_back(BWAPI::TilePosition(104, 116));
			strides.push_back(6);
			strides.push_back(5);
			strides.push_back(3);
			strides.push_back(3);
			strides.push_back(3);
			for (int i = 0; i < startPoints.size(); i++) {

				for (int j = 0; j < strides[i]; j++) {
					BWAPI::TilePosition target = startPoints[i] + BWAPI::TilePosition(3 * j, 0);
					_reservedSupPositions.push(target);
					ConstructionPlaceFinder::Instance().reserveTiles(target, 3, 2);
				}
			}
		}
		else {
			// 1
			_supPositionsForWall.push(BWAPI::TilePosition(97, 5));
			_supPositionsForWall.push(BWAPI::TilePosition(100, 7));
			_barPositionForWall = BWAPI::TilePosition(102, 9);
			_turretPosition = BWAPI::TilePosition(97, 4);
			_bunkerPosition = BWAPI::TilePosition(90, 14);

			std::vector<BWAPI::TilePosition> startPoints;
			std::vector<int> strides;
			startPoints.push_back(BWAPI::TilePosition(125, 30));
			startPoints.push_back(BWAPI::TilePosition(122, 28));
			startPoints.push_back(BWAPI::TilePosition(119, 28));
			strides.push_back(8);
			strides.push_back(7);
			strides.push_back(7);
			for (int i = 0; i < startPoints.size(); i++) {

				for (int j = 0; j < strides[i]; j++) {
					BWAPI::TilePosition target = startPoints[i] + BWAPI::TilePosition(0, -2 * j);
					_reservedSupPositions.push(target);
					ConstructionPlaceFinder::Instance().reserveTiles(target, 3, 2);
				}
			}
		}
	}
	else if (getMapName() == 'H') {
		
		if (getPostionAtHunter() == 11) {
			//29 19 서플
			//26 21 베력
			_supPositionsForWall.push(BWAPI::TilePosition(29, 19));
			_supPositionsForWall.push(BWAPI::TilePosition(14, 5));

			_barPositionForWall = BWAPI::TilePosition(26, 21);

		}
		else if (getPostionAtHunter() == 12) {
				//52 23 서플
				//54 25 베럭
			_supPositionsForWall.push(BWAPI::TilePosition(52, 23));
			_supPositionsForWall.push(BWAPI::TilePosition(74, 7));

			_barPositionForWall = BWAPI::TilePosition(54, 25);

		}
		else if (getPostionAtHunter() == 1) {
			//96 21 서플
			//98 32 베럭
			_supPositionsForWall.push(BWAPI::TilePosition(96, 21));
			_supPositionsForWall.push(BWAPI::TilePosition(125, 33));

			_barPositionForWall = BWAPI::TilePosition(98, 23);

		}
		else if (getPostionAtHunter() == 3) {
			//101 61 서플
			//102 63 베럭
			_supPositionsForWall.push(BWAPI::TilePosition(101, 61));
			_supPositionsForWall.push(BWAPI::TilePosition(119, 44));

			_barPositionForWall = BWAPI::TilePosition(102, 63);

		}
		else if (getPostionAtHunter() == 5) {
			//93 95 서플라이
			//90 97 베럭
			_supPositionsForWall.push(BWAPI::TilePosition(93, 95));
			_supPositionsForWall.push(BWAPI::TilePosition(118, 115));

			_barPositionForWall = BWAPI::TilePosition(90, 97);

		}
		else if (getPostionAtHunter() == 6) {
			//55 94 서플라이
			//52 96 베럭
			_supPositionsForWall.push(BWAPI::TilePosition(55, 94));
			_supPositionsForWall.push(BWAPI::TilePosition(67, 116));

			_barPositionForWall = BWAPI::TilePosition(52, 96);

		}
		else if (getPostionAtHunter() == 7) {
			//14 96 서플라이
			//15 98 베럭
			_supPositionsForWall.push(BWAPI::TilePosition(14, 96));
			_supPositionsForWall.push(BWAPI::TilePosition(14, 114));

			_barPositionForWall = BWAPI::TilePosition(15, 98);

		}
		else if (getPostionAtHunter() == 9) {
			//23 54 서플
			//20 56 베럭
			_supPositionsForWall.push(BWAPI::TilePosition(23, 54));
			_supPositionsForWall.push(BWAPI::TilePosition(12, 46));

			_barPositionForWall = BWAPI::TilePosition(20, 56);

		}
		else {
			_barPositionForWall = BWAPI::TilePositions::None;
		}
	}
	else if (getMapName() == 'C') {
		// 1
		if (base->getTilePosition().x >= 64 && base->getTilePosition().y < 64) {
			_supPositionsForWall.push(BWAPI::TilePosition(122, 23));
			_supPositionsForWall.push(BWAPI::TilePosition(125, 23));
			_barPositionForWall = BWAPI::TilePosition(118, 23);
			_turretPosition = BWAPI::TilePosition(125, 22);
			_bunkerPosition = BWAPI::TilePosition(118, 32);

			std::vector<BWAPI::TilePosition> startPoints;
			std::vector<int> strides;
			startPoints.push_back(BWAPI::TilePosition(97, 0));
			startPoints.push_back(BWAPI::TilePosition(98, 2));
			startPoints.push_back(BWAPI::TilePosition(102, 4));
			startPoints.push_back(BWAPI::TilePosition(105, 6));
			strides.push_back(7);
			strides.push_back(7);
			strides.push_back(3);
			strides.push_back(2);
			for (int i = 0; i < startPoints.size(); i++) {

				for (int j = 0; j < strides[i]; j++) {
					BWAPI::TilePosition target = startPoints[i] + BWAPI::TilePosition(3 * j, 0);
					_reservedSupPositions.push(target);
					ConstructionPlaceFinder::Instance().reserveTiles(target, 3, 2);
				}
			}
		}
		// 5
		else if (base->getTilePosition().x >= 64 && base->getTilePosition().y >= 64) {
			_supPositionsForWall.push(BWAPI::TilePosition(122, 101));
			_supPositionsForWall.push(BWAPI::TilePosition(125, 101));
			_barPositionForWall = BWAPI::TilePosition(118, 102);
			_turretPosition = BWAPI::TilePosition(125, 102);
			_bunkerPosition = BWAPI::TilePosition(118, 95);

			std::vector<BWAPI::TilePosition> startPoints;
			std::vector<int> strides;
			startPoints.push_back(BWAPI::TilePosition(97, 125));
			startPoints.push_back(BWAPI::TilePosition(98, 123));
			startPoints.push_back(BWAPI::TilePosition(102, 121));
			startPoints.push_back(BWAPI::TilePosition(105, 119));
			strides.push_back(7);
			strides.push_back(7);
			strides.push_back(3);
			strides.push_back(2);
			for (int i = 0; i < startPoints.size(); i++) {

				for (int j = 0; j < strides[i]; j++) {
					BWAPI::TilePosition target = startPoints[i] + BWAPI::TilePosition(3 * j, 0);
					_reservedSupPositions.push(target);
					ConstructionPlaceFinder::Instance().reserveTiles(target, 3, 2);
				}
			}
		}
		// 7
		else if (base->getTilePosition().x < 64 && base->getTilePosition().y >= 64) {
			_supPositionsForWall.push(BWAPI::TilePosition(4, 102));
			_supPositionsForWall.push(BWAPI::TilePosition(7, 102));
			_barPositionForWall = BWAPI::TilePosition(0, 101);
			_turretPosition = BWAPI::TilePosition(7, 103);
			_bunkerPosition = BWAPI::TilePosition(6, 95);

			std::vector<BWAPI::TilePosition> startPoints;
			std::vector<int> strides;
			startPoints.push_back(BWAPI::TilePosition(27, 125));
			startPoints.push_back(BWAPI::TilePosition(27, 123));
			startPoints.push_back(BWAPI::TilePosition(23, 121));
			startPoints.push_back(BWAPI::TilePosition(20, 119));
			strides.push_back(7);
			strides.push_back(7);
			strides.push_back(3);
			strides.push_back(2);
			for (int i = 0; i < startPoints.size(); i++) {

				for (int j = 0; j < strides[i]; j++) {
					BWAPI::TilePosition target = startPoints[i] + BWAPI::TilePosition(-3 * j, 0);
					_reservedSupPositions.push(target);
					ConstructionPlaceFinder::Instance().reserveTiles(target, 3, 2);
				}
			}
		}
		// 11
		else {
			_supPositionsForWall.push(BWAPI::TilePosition(4, 24));
			_supPositionsForWall.push(BWAPI::TilePosition(7, 24));
			_barPositionForWall = BWAPI::TilePosition(0, 24);
			_turretPosition = BWAPI::TilePosition(7, 23);
			_bunkerPosition = BWAPI::TilePosition(6, 32);

			std::vector<BWAPI::TilePosition> startPoints;
			std::vector<int> strides;
			startPoints.push_back(BWAPI::TilePosition(27, 0));
			startPoints.push_back(BWAPI::TilePosition(27, 2));
			startPoints.push_back(BWAPI::TilePosition(23, 4));
			startPoints.push_back(BWAPI::TilePosition(20, 6));
			strides.push_back(7);
			strides.push_back(7);
			strides.push_back(3);
			strides.push_back(2);
			for (int i = 0; i < startPoints.size(); i++) {

				for (int j = 0; j < strides[i]; j++) {
					BWAPI::TilePosition target = startPoints[i] + BWAPI::TilePosition(-3 * j, 0);
					_reservedSupPositions.push(target);
					ConstructionPlaceFinder::Instance().reserveTiles(target, 3, 2);
				}
			}
		}
	}
}

BWAPI::Position InformationManager::getDropPosition()
{
	if (dropTo.isValid())
		return dropTo;
	else
		return BWAPI::Position(BWAPI::Broodwar->mapWidth()/16 ,BWAPI::Broodwar->mapHeight()/16);
}

void InformationManager::setWallPosition(BWAPI::TilePosition position) {
	_wallPositions.push_back(position);
}

BWAPI::TilePosition InformationManager::getWallBarackPosition() {
	return _wallBarackPosition;
}

void InformationManager::updateWallStatus() {

	for (auto it = _wallPositions.begin(); it != _wallPositions.end();) {

		bool flag = false;
		for (auto unit : BWAPI::Broodwar->self()->getUnits()) {

			if (unit == nullptr) {
				continue;
			}

			if (unit->getTilePosition() == *it &&
				(unit->getType() == BWAPI::UnitTypes::Terran_Barracks || unit->getType() == BWAPI::UnitTypes::Terran_Supply_Depot)) {

				if (unit->getType() == BWAPI::UnitTypes::Terran_Barracks) {
					_wallBarackPosition = unit->getTilePosition();
				}
				assert(_wallUnits.find(unit) == _wallUnits.end());
				_wallUnits.insert(unit);
				it = _wallPositions.erase(it);
				flag = true;
				break;
			}
		}
		if (flag == false) {
			it++;
		}
	}
	
	if (_wallUnits.size() < 3) {
		_wallStatus = false;
		return;
	}

	for (auto wallUnit : _wallUnits) {
		if (wallUnit->getType() == BWAPI::UnitTypes::Terran_Barracks && wallUnit->isLifted() == true) {
			_wallStatus = false;
			return;
		}
	}

	_wallStatus = true;
}

bool InformationManager::getWallStatus() {
	return _wallStatus;
}

BWAPI::Unitset InformationManager::getWallUnits() {
	return _wallUnits;
}
double InformationManager::getEnemyUnitRatio(BWAPI::UnitType baseUnitType){
	double rst = 0.0;

	if (enemyRace == BWAPI::Races::Unknown){
		return rst;
	}
	else{
		if (baseUnitType == BWAPI::UnitTypes::Protoss_Zealot){
			int z = 0;
			int d = 0;

			for (auto a : rushSquad){
				if (a.type == BWAPI::UnitTypes::Protoss_Zealot){
					z++;
				}

				if (a.type == BWAPI::UnitTypes::Protoss_Dragoon){
					d++;
				}
			}

			rst = d==0 ? 0.0 : z / (double)d;
		}
	}

	return rst;
}

std::vector<BWAPI::TilePosition>& InformationManager::getWallPositions() {
	return _wallPositions;
}


int InformationManager::needTurret(std::string mode){
	//v18 김유진
	//얼마나 터렛이 필요한지 결정
	int warning_level = 0;

	if (mode == "basicFlying"){
		if (hasFlyingUnits){
			warning_level = 1;

			int num_units = 0;
			for (auto u : getUnitAndUnitInfoMap(enemyPlayer)){
				if (u.second.type == BWAPI::UnitTypes::Zerg_Mutalisk){
					num_units++;
				}
			}

			if (num_units == 1){
				warning_level = 2;
			}
			else if (num_units > 1){
				warning_level = 3;
			}
		}
	}
	else if (mode == "basicDetect"){
		if (hasCloakedUnits){
			warning_level = 1;

			int num_units = 0;
			for (auto u : getUnitAndUnitInfoMap(enemyPlayer)){
				if (u.second.type == BWAPI::UnitTypes::Protoss_Dark_Templar ||
					u.second.type == BWAPI::UnitTypes::Zerg_Lurker ||
					u.second.type == BWAPI::UnitTypes::Zerg_Lurker_Egg){
					num_units++;
				}
			}

			if (num_units == 1){
				warning_level = 2;
			}
			else if (num_units > 1){
				warning_level = 3;
			}
		}
	}

	return warning_level;
}

BWAPI::TilePosition InformationManager::getTurretPosition() {
	return _turretPosition;
}

bool InformationManager::getTurretStatus() {
	return _turretStatus;
}

void InformationManager::setTurretStatus(bool flag) {
	_turretStatus = flag;
}

BWAPI::TilePosition InformationManager::getBunkerPosition() {
	return _bunkerPosition;
}