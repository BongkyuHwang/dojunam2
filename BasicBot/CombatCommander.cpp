#include "CombatCommander.h"
#include "MedicManager.h"

using namespace MyBot;

const size_t IdlePriority = 0;
const size_t AttackPriority = 0;
const size_t BaseDefensePriority = 3;
const size_t BunkerPriority = 1;
const size_t ScoutPriority = 2;
//const size_t ScoutDefensePriority = 3;
const size_t DropPriority = 4;

CombatCommander & CombatCommander::Instance()
{
	static CombatCommander instance;
	return instance;
}

CombatCommander::CombatCommander() 
    : _initialized(false),
	notDraw(false)
{

	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	//tstruct = *localtime(&now);
	localtime_s(&tstruct, &now);
	strftime(buf, sizeof(buf), "%Y%m%d%H%M%S_combat", &tstruct);

	log_file_path = Config::Strategy::WriteDir + std::string(buf) + ".log";
	//std::cout << "log_file_path:" << log_file_path << std::endl;
	/////////////////////////////////////////////////////////////////////////
	
}

void CombatCommander::initializeSquads()
{
	InformationManager & im = InformationManager::Instance();

	initMainAttackPath = false;
	curIndex = 0;

	indexFirstChokePoint_OrderPosition = -1;
	rDefence_OrderPosition = im.getMainBaseLocation(BWAPI::Broodwar->self())->getPosition();
	BWAPI::Position myFirstChokePointCenter = im.getFirstChokePoint(BWAPI::Broodwar->self())->getCenter();
	BWAPI::Unit targetDepot = nullptr;
	double maxDistance = 0;
	for (auto & munit : BWAPI::Broodwar->getAllUnits())
	{
		if (
			(munit->getType() == BWAPI::UnitTypes::Resource_Mineral_Field)
			&& myFirstChokePointCenter.getDistance(munit->getPosition()) >= maxDistance
			&& BWTA::getRegion(BWAPI::TilePosition(munit->getPosition())) 
			== im.getMainBaseLocation(BWAPI::Broodwar->self())->getRegion()
			)
		{
			maxDistance = myFirstChokePointCenter.getDistance(munit->getPosition());
			targetDepot = munit;
		}
	}
	float dx = 0, dy = 0;
	if (targetDepot == nullptr)
		return;
	//std::cout << "targetDepot " << targetDepot->getPosition().x/ 32 << " " << targetDepot->getPosition().y/32 << std::endl;
	dx = (targetDepot->getPosition().x - rDefence_OrderPosition.x)*0.7f;
	dy = (targetDepot->getPosition().y - rDefence_OrderPosition.y)*0.7f;
	rDefence_OrderPosition = rDefence_OrderPosition + BWAPI::Position(dx, dy);// (mineralPosition + closestDepot->getPosition()) / 2;
	wFirstChokePoint_OrderPosition = getPositionForDefenceChokePoint(im.getFirstChokePoint(BWAPI::Broodwar->self()));

	/*
	방어형 쵸크포인트 계산
	첫번째쵸크 10% 뒤에 있고 우리 리젼으로 확장 
	*/
	BWAPI::Position tmpBase(BWAPI::Broodwar->self()->getStartLocation());
	BWAPI::Position tmpChoke(im.getFirstChokePoint(BWAPI::Broodwar->self())->getCenter());
	
	double tmpRatio = 0.2;
	std::pair<int, int> vecBase2Choke;
	vecBase2Choke.first = tmpBase.x - tmpChoke.x;
	vecBase2Choke.second = tmpBase.y - tmpChoke.y;
	BWAPI::Position in_1st_chock_center(tmpChoke.x + (int)(vecBase2Choke.first*tmpRatio), tmpChoke.y + (int)(vecBase2Choke.second*tmpRatio));
	std::pair<BWAPI::Position, BWAPI::Position> in_1st_chock_side;
	in_1st_chock_side.first = BWAPI::Position(im.getFirstChokePoint(BWAPI::Broodwar->self())->getSides().first.x + (int)(vecBase2Choke.first*tmpRatio), im.getFirstChokePoint(BWAPI::Broodwar->self())->getSides().first.y + (int)(vecBase2Choke.second*tmpRatio));
	in_1st_chock_side.second = BWAPI::Position(im.getFirstChokePoint(BWAPI::Broodwar->self())->getSides().second.x + (int)(vecBase2Choke.first*tmpRatio), im.getFirstChokePoint(BWAPI::Broodwar->self())->getSides().second.y + (int)(vecBase2Choke.second*tmpRatio));
	
	////std::cout << "base:" << tmpBase << std::endl;
	////std::cout << "choke:" << tmpChoke << std::endl;
	////std::cout << "vecBase2Choke:" << vecBase2Choke.first << "," << vecBase2Choke.second << std::endl;
	////std::cout << "in_1st_chock_center:" << in_1st_chock_center.x / 32 << "," << in_1st_chock_center.y / 32 << std::endl;
	////std::cout << "in_1st_chock_side:" << in_1st_chock_side.first.x / 32 << "," << in_1st_chock_side.first.y / 32 << "/" << in_1st_chock_side.second.x / 32 << "," << in_1st_chock_side.second.y / 32 << std::endl;
	int dfcRadius = BWAPI::UnitTypes::Terran_Marine.groundWeapon().maxRange();

	if (BWAPI::Broodwar->enemy()->getRace() != BWAPI::Races::Zerg)
	{
		dfcRadius = rDefence_OrderPosition.getDistance(wFirstChokePoint_OrderPosition);
	}
	SquadOrder defcon1Order(SquadOrderTypes::Defend, rDefence_OrderPosition
		, dfcRadius, "DEFCON1");
	_squadData.addSquad("DEFCON1", Squad("DEFCON1", defcon1Order, IdlePriority));


	int radiusAttack = im.getMapName() == 'H' ? 50 : 30;
	int radiusScout = 10;

	RegionVertices *tmpObj = MapGrid::Instance().getRegionVertices(InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self()));
	BWAPI::Position seedPosition = BWAPI::Positions::None;
	if (tmpObj != NULL){
		seedPosition = tmpObj->getSiegeDefence();
	}

	SquadOrder defcon2Order(SquadOrderTypes::Idle, seedPosition, 150, "DEFCON2");
	_squadData.addSquad("DEFCON2", Squad("DEFCON2", defcon2Order, IdlePriority));

	//앞마당 중간에서 정찰만
	std::vector<BWAPI::TilePosition> expansion2choke = BWTA::getShortestPath(BWAPI::TilePosition(im.getFirstChokePoint(im.selfPlayer)->getCenter()), BWAPI::TilePosition(im.getSecondChokePoint(im.selfPlayer)->getCenter()));
	SquadOrder defcon3Order(SquadOrderTypes::Idle, 
		BWAPI::Position(expansion2choke[expansion2choke.size() / 2]), 
		radiusScout, 
		"DEFCON3");
	_squadData.addSquad("DEFCON3", Squad("DEFCON3", defcon3Order, IdlePriority));

	SquadOrder defcon4Order(SquadOrderTypes::Idle, 
		im.getSecondChokePoint(im.selfPlayer)->getCenter() + im.getSecondChokePoint(im.selfPlayer)->getCenter() - im.getFirstChokePoint(im.selfPlayer)->getCenter(),
		BWAPI::UnitTypes::Terran_Marine.groundWeapon().maxRange() + radiusAttack, 
		//im.getSecondChokePoint(im.selfPlayer)->getSides(), 
		"DEFCON4");
	_squadData.addSquad("DEFCON4", Squad("DEFCON4", defcon4Order, IdlePriority));
	
	SquadOrder scoutOrder(SquadOrderTypes::Idle, im.getSecondChokePoint(im.selfPlayer)->getSides().first, 70, "scout");
	_squadData.addSquad("scout", Squad("scout", scoutOrder, ScoutPriority));

    // the main attack squad that will pressure the enemy's closest base location
	SquadOrder mainAttackOrder(SquadOrderTypes::Attack, getMainAttackLocationForCombat(), 500, "AttackEnemyBase");
	_squadData.addSquad("MainAttack", Squad("MainAttack", mainAttackOrder, AttackPriority));

	SquadOrder smallAttackOrder(SquadOrderTypes::Attack, getMainAttackLocationForCombat(), 250, "AttackEnemymulti");
	_squadData.addSquad("SMALLATTACK", Squad("SMALLATTACK", smallAttackOrder, AttackPriority));


	if (InformationManager::Instance().getMapName() != 'H')
	{
		SquadOrder siegeDefenceOrder(SquadOrderTypes::Defend, seedPosition, 400, "S4D");
		_squadData.addSquad("S4D", Squad("S4D", siegeDefenceOrder, BaseDefensePriority));
	}
	

    BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());

	SquadOrder zealotDrop(SquadOrderTypes::Drop, ourBasePosition, 410, "Wait for transport");
    _squadData.addSquad("Drop", Squad("Drop", zealotDrop, DropPriority));    

    _initialized = true;
}

bool CombatCommander::isSquadUpdateFrame()
{
	return TimeManager::Instance().isMyTurn("isSquadUpdateFrame", 10);
}

void CombatCommander::update()
{
    //if (!Config::Modules::UsingCombatCommander)
    //{
    //    return;
    //}
	
    if (!_initialized)
    {
        initializeSquads();
    }

	InformationManager & im = InformationManager::Instance();
	if (im.nowCombatStatus == InformationManager::combatStatus::DEFCON1 ||
		im.nowCombatStatus == InformationManager::combatStatus::DEFCON2 ||
		im.nowCombatStatus == InformationManager::combatStatus::DEFCON3 ||
		im.nowCombatStatus == InformationManager::combatStatus::DEFCON4
		){

		if (im.changeConmgeStatusFrame == BWAPI::Broodwar->getFrameCount()){
			updateIdleSquad();
		}
		else{
			supplementSquad();
		}
		
	}
	else if (im.nowCombatStatus == InformationManager::combatStatus::CenterAttack ||
		im.nowCombatStatus == InformationManager::combatStatus::MainAttack ||
		im.nowCombatStatus == InformationManager::combatStatus::EnemyBaseAttack){
		if (im.changeConmgeStatusFrame == BWAPI::Broodwar->getFrameCount()){
			updateAttackSquads();
		}
		else{
			//추가병력 세팅
			supplementSquad();
		}
	}

	//드롭스쿼드는 별도로 운영 : 우리 전장 상태와 무관
	if (isSquadUpdateFrame())
	{
		//std::cout << "3" << std::endl;
		if (im.nowCombatStatus == InformationManager::combatStatus::DEFCON4)
		{
			Squad & defcon4Squad = _squadData.getSquad("DEFCON4");

			BWAPI::Position positionDefcon4 = getPoint_DEFCON4();
			int radius = BWAPI::Broodwar->mapWidth() * 5 + defcon4Squad.getUnits().size();
			if (BWAPI::Broodwar->enemy()->getRace() != BWAPI::Races::Terran)
			{
				radius = BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange();
			}
			if (positionDefcon4 != BWAPI::Positions::None) {
				SquadOrder defcon4Order(SquadOrderTypes::Idle, positionDefcon4,
					radius , "DEFCON4");
				defcon4Squad.setSquadOrder(defcon4Order);
			}
		}
		//어택포지션 변경
		else if (im.nowCombatStatus == InformationManager::combatStatus::EnemyBaseAttack){
			Squad & mainAttackSquad = _squadData.getSquad("MainAttack");
			SquadOrder _order(SquadOrderTypes::Attack, getMainAttackLocationForCombat(), mainAttackSquad.getSquadOrder().getRadius(), "AttackEnemybase");
			mainAttackSquad.setSquadOrder(_order);
		}
		//방어부대는 계속 편성해야함
		else if (im.nowCombatStatus == InformationManager::combatStatus::DEFCON5){
			updateDefenseSquads();

			for (auto r : defenceRegions)
			{
				std::stringstream squadName;
				squadName << "DEFCON5" << r->getCenter().x << "_" << r->getCenter().y;

				// if we don't have a squad assigned to this region already, create one
				if (_squadData.squadExists(squadName.str()))
				{
					Squad & defenseSquad = _squadData.getSquad(squadName.str());
				}
			}
		}
		updateScoutSquads();
		//if (im.nowCombatStatus < InformationManager::combatStatus::DEFCON4)//초반에만 벙커를 설치한다고 가정하고,
		updateBunkertSquads();
		updateSmallAttackSquad();
		updateDropSquads();
	}
	_squadData.update();
	drawSquadInformation(20, 200);
	Micro::drawAPM(10, 150);
}

void CombatCommander::updateIdleSquad()
{
	InformationManager & im = InformationManager::Instance();

	Squad & defcon1Squad = _squadData.getSquad("DEFCON1");
	Squad & defcon2Squad = _squadData.getSquad("DEFCON2");
	Squad & defcon3Squad = _squadData.getSquad("DEFCON3");
	Squad & defcon4Squad = _squadData.getSquad("DEFCON4");
	
	std::pair<int, BWAPI::Unit> shortDistanceUnitForDEFCON3;
	shortDistanceUnitForDEFCON3.first = 1000000;

	BWAPI::Unit candidateS4D = nullptr;
	
	//BWAPI::Position mineralPosition = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition();
	for (auto & unit : _combatUnits)
	{
		//전투 준비
		if(im.nowCombatStatus == InformationManager::combatStatus::DEFCON1)
		{
			
			if (_squadData.canAssignUnitToSquad(unit, defcon1Squad))
			{
				//idleSquad.addUnit(unit);
				_squadData.assignUnitToSquad(unit, defcon1Squad);
			}
		}
		//첫번째 초크
		else if (im.nowCombatStatus == InformationManager::combatStatus::DEFCON2)
		{
			if (_squadData.canAssignUnitToSquad(unit, defcon2Squad))
			{
				//idleSquad.addUnit(unit);
				_squadData.assignUnitToSquad(unit, defcon2Squad);
			}
		}

		//첫번째 초크 밖
		else if (im.nowCombatStatus == InformationManager::combatStatus::DEFCON3)
		{
			//첫번째 초크 안쪽에 전부 배치하고
			if (_squadData.canAssignUnitToSquad(unit, defcon2Squad))
			{
				//idleSquad.addUnit(unit);
				_squadData.assignUnitToSquad(unit, defcon2Squad);

				if ((unit->getType() != BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode || unit->getType() != BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode)){
					int tmpDistance = unit->getPosition().getDistance(im.getFirstExpansionLocation(im.selfPlayer)->getPosition());
					if (tmpDistance < shortDistanceUnitForDEFCON3.first){
						shortDistanceUnitForDEFCON3.first = tmpDistance;
						shortDistanceUnitForDEFCON3.second = unit;
					}
				}
			}
		}

		//두번째 쵸크
		else if (im.nowCombatStatus == InformationManager::combatStatus::DEFCON4)
		{
			if (_squadData.canAssignUnitToSquad(unit, defcon4Squad))
			{
				//idleSquad.addUnit(unit);
				_squadData.assignUnitToSquad(unit, defcon4Squad);
			}
		}
	}

	//1마리만 밖에서 정찰 (탱크는 제외)
	if (im.nowCombatStatus == InformationManager::combatStatus::DEFCON3){
		if (shortDistanceUnitForDEFCON3.first < 1000000)
		{
			_squadData.assignUnitToSquad(shortDistanceUnitForDEFCON3.second, defcon3Squad);
		}
	}

	if (InformationManager::Instance().getMapName() != 'H' && _squadData.squadExists("S4D"))
	{
		Squad & s4dSquad = _squadData.getSquad("S4D");
		int countTank = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode) + BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode);
		int shortDistance_siege = 1000000;
		BWAPI::Position targetP(s4dSquad.getSquadOrder().getPosition());

		if (s4dSquad.getUnits().size() < 2 && countTank > 0)
		{
			for (auto & unit : _combatUnits)
			{
				if ((unit->getType() == BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode || unit->getType() == BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode)){

					if (unit->getDistance(targetP) < shortDistance_siege && !s4dSquad.containsUnit(unit)){
						shortDistance_siege = unit->getDistance(targetP);
						candidateS4D = unit;
					}
				}
			}

			if (candidateS4D != nullptr)
			{
				if (_squadData.canAssignUnitToSquad(candidateS4D, s4dSquad)){
					////std::cout << "set S4D:" << candidateS4D->getID() << std::endl;
					_squadData.assignUnitToSquad(candidateS4D, s4dSquad);
				}
					
			}
		}
	}

}

BWAPI::Position CombatCommander::getIdleSquadLastOrderLocation()
{
	BWAPI::Position mCenter((BWAPI::Broodwar->mapWidth()*16), BWAPI::Broodwar->mapHeight()*16);
	BWTA::Chokepoint * mSec = InformationManager::Instance().getSecondChokePoint(BWAPI::Broodwar->self());
	int fIndex = 7;
	if (mainAttackPath.size() > 0)
	{
		if ((_combatUnits.size() / fIndex) + 1 < mainAttackPath.size())
			return mainAttackPath[(_combatUnits.size() / fIndex)];
	}

	return mSec->getCenter();

}

void CombatCommander::updateAttackSquads()
{
	Squad & mainAttackSquad = _squadData.getSquad("MainAttack");
	
	for (auto & unit : _combatUnits)
	{
		if (_squadData.canAssignUnitToSquad(unit, mainAttackSquad))
		{
			_squadData.assignUnitToSquad(unit, mainAttackSquad);
		}
	}

	InformationManager & im = InformationManager::Instance();
	int radi = 410;
	if (im.nowCombatStatus == InformationManager::combatStatus::CenterAttack)
	{
		std::vector<BWAPI::TilePosition> tileList = BWTA::getShortestPath(BWAPI::TilePosition(im.getSecondChokePoint(BWAPI::Broodwar->self())->getCenter()), BWAPI::TilePosition(BWAPI::Broodwar->mapWidth() / 2, BWAPI::Broodwar->mapHeight() / 2) );
		SquadOrder _order(SquadOrderTypes::Attack, BWAPI::Position(tileList[tileList.size() - 1]), radi, "AttackCenter");
		mainAttackSquad.setSquadOrder(_order);
	}
	else if (im.nowCombatStatus == InformationManager::combatStatus::EnemyBaseAttack)
	{
		int addRadi = 0;
		if (BWAPI::Broodwar->getFrameCount() > 20000)
		{
			addRadi += 200;
		}
		if (BWAPI::Broodwar->getFrameCount() > 28000)
		{
			addRadi += 200;
		}
		if (BWAPI::Broodwar->getFrameCount() > 35000)
		{
			addRadi += 400;
		}
		SquadOrder _order(SquadOrderTypes::Attack, getMainAttackLocationForCombat(), 800+addRadi, "AttackEnemybase");
		mainAttackSquad.setSquadOrder(_order);
	}
}

void CombatCommander::updateDropSquads()
{
	Squad & dropSquad = _squadData.getSquad("Drop");
	int numUnits = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Dropship);
	if (numUnits <= 0)
		return;

    // figure out how many units the drop squad needs
    bool dropSquadHasTransport = false;
    int transportSpotsRemaining = 0;
	int totalSpaceRemaining = 0;
	BWAPI::Unitset  dropUnits = dropSquad.getUnits();
	BWAPI::Unit dropShipUnit = nullptr;
	BWAPI::Unitset dropShips;

	for (auto & unit : dropUnits)
	{
		if (unit->getType() == BWAPI::UnitTypes::Terran_Dropship && unit->getHitPoints() > 0 && unit->exists() )
		{
			if (_squadData.canAssignUnitToSquad(unit, dropSquad))
				_squadData.assignUnitToSquad(unit, dropSquad);
			if (unit->getSpaceRemaining() > 0)
			{
				dropShips.insert(unit);
			}
		}
	}

    for (auto & unit : dropUnits)
    {	
		if (unit->getType() != BWAPI::UnitTypes::Terran_Dropship &&!unit->isLoaded())
		{
			if (BWTA::getRegion(BWAPI::TilePosition(unit->getPosition())) 
				== BWTA::getRegion(BWAPI::TilePosition(dropSquad.getSquadOrder().getPosition())) )
			{
				for (auto & ship : dropShips)
				{
					if (unit->getType().spaceRequired() <= ship->getSpaceRemaining())
					{
						if (!ship->load(unit))
							Micro::SmartMove(ship, unit->getPosition());
						if (!unit->load(ship))
							Micro::SmartMove(unit, ship->getPosition());
					}
				}
			}
		}
    }

	if (!dropSquadHasTransport)
	{
		dropSquad.clear();
	}
	
    // if there are still units to be added to the drop squad, do it
	//if ((totalSpaceRemaining > 0 || !dropSquadHasTransport))
    {
        // take our first amount of combat units that fill up a transport and add them to the drop squad
        for (auto & unit : _combatUnits)
        {
			if (unit->getType() == BWAPI::UnitTypes::Terran_Dropship
				&& _squadData.canAssignUnitToSquad(unit, dropSquad)
				&& !dropSquad.containsUnit(unit))
			{
				_squadData.assignUnitToSquad(unit, dropSquad);
				continue;
			}

			if (unit->getType() != BWAPI::UnitTypes::Terran_SCV
				&& !unit->getType().isBuilding()
				&& !unit->isFlying()
				&& _squadData.canAssignUnitToSquad(unit, dropSquad)
				&& !dropSquad.containsUnit(unit)
				&& BWTA::getRegion(BWAPI::TilePosition(unit->getPosition()))
				== BWTA::getRegion(InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition())
				)
			{
				for (auto & ship : dropShips)
				{
					if (unit->getType().spaceRequired() <= ship->getSpaceRemaining())
					{
						if (!ship->load(unit))
							Micro::SmartMove(ship, unit->getPosition());
						if (!unit->load(ship))
							Micro::SmartMove(unit, ship->getPosition());
						_squadData.assignUnitToSquad(unit, dropSquad);
					}
				}
			}
        }
    }
    // otherwise the drop squad is full, so execute the order
	//else if ( !dropSquadHasTransport) //totalSpaceRemaining == 0 &&
    {
		SquadOrder goingDrop(SquadOrderTypes::Drop, BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation()), 410, "Go Drop");
		dropSquad.setSquadOrder(goingDrop);
    }

}

void CombatCommander::updateScoutDefenseSquad() 
{
    if (_combatUnits.empty()) 
    { 
        return; 
    }

    // if the current squad has units in it then we can ignore this
    Squad & scoutDefenseSquad = _squadData.getSquad("ScoutDefense");
  
    // get the region that our base is located in
    BWTA::Region * myRegion = BWTA::getRegion(BWAPI::Broodwar->self()->getStartLocation());
    if (!myRegion && myRegion->getCenter().isValid())
    {
        return;
    }

    // get all of the enemy units in this region
	BWAPI::Unitset enemyUnitsInRegion;
    for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
    {
        if (BWTA::getRegion(BWAPI::TilePosition(unit->getPosition())) == myRegion)
        {
            enemyUnitsInRegion.insert(unit);
        }
    }

    // if there's an enemy worker in our region then assign someone to chase him
    bool assignScoutDefender = enemyUnitsInRegion.size() == 1 && (*enemyUnitsInRegion.begin())->getType().isWorker();

    // if our current squad is empty and we should assign a worker, do it
    if (scoutDefenseSquad.isEmpty() && assignScoutDefender)
    {
        // the enemy worker that is attacking us
        BWAPI::Unit enemyWorker = *enemyUnitsInRegion.begin();

        // get our worker unit that is mining that is closest to it
        BWAPI::Unit workerDefender = findClosestWorkerToTarget(_combatUnits, enemyWorker);

		if (enemyWorker && workerDefender)
		{
			// grab it from the worker manager and put it in the squad
            if (_squadData.canAssignUnitToSquad(workerDefender, scoutDefenseSquad))
            {
			    WorkerManager::Instance().setCombatWorker(workerDefender);
                _squadData.assignUnitToSquad(workerDefender, scoutDefenseSquad);
            }
		}
    }
    // if our squad is not empty and we shouldn't have a worker chasing then take him out of the squad
    else if (!scoutDefenseSquad.isEmpty() && !assignScoutDefender)
    {
        for (auto & unit : scoutDefenseSquad.getUnits())
        {
			printf("stoped [%d] %s\n", unit->getID(), unit->getType().c_str());
			//BWAPI::Broodwar->setScreenPosition(unit->getPosition() - BWAPI::Position(100, 100));
			//BWAPI::Broodwar->drawBoxMap(unit->getPosition() - BWAPI::Position(unit->getType().width() + 10, unit->getType().height() + 10), unit->getPosition() + BWAPI::Position(unit->getType().width() + 10, unit->getType().height() + 10), BWAPI::Colors::Black, true);
			//BWAPI::Broodwar->drawBoxMap(unit->getPosition() - BWAPI::Position(unit->getType().width(), unit->getType().height()), unit->getPosition() + BWAPI::Position(unit->getType().width(), unit->getType().height()), BWAPI::Colors::Yellow, true);

            unit->stop();
            if (unit->getType().isWorker())
            {
                WorkerManager::Instance().finishedWithWorker(unit);
            }
        }

        scoutDefenseSquad.clear();
    }
}

void CombatCommander::updateDefenseSquads() 
{
	if (_combatUnits.empty() || _combatUnits.size() == 0)
    { 
        return; 
    }

	int numDefendersPerEnemyUnit = 20;

	for(auto r : defenceRegions)
	{
		std::stringstream squadName;
		squadName << "DEFCON5" << r->getCenter().x << "_" << r->getCenter().y;

		// if we don't have a squad assigned to this region already, create one
		if (!_squadData.squadExists(squadName.str()))
		{
			SquadOrder defendRegion(SquadOrderTypes::Defend, unitNumInDefenceRegion[r].getPosition(), 500, "Defend Region!");
			_squadData.addSquad(squadName.str(), Squad(squadName.str(), defendRegion, IdlePriority));
		}

		Squad & defenseSquad = _squadData.getSquad(squadName.str());
		SquadOrder defendRegion(SquadOrderTypes::Defend, unitNumInDefenceRegion[r].getPosition(), 500, "Defend Region!");
		defenseSquad.setSquadOrder(defendRegion);

		int flyingDefendersNeeded = std::count_if(unitNumInDefenceRegion[r].begin(), unitNumInDefenceRegion[r].end(), [](BWAPI::Unit u) { return u->isFlying(); });
		int groundDefensersNeeded = std::count_if(unitNumInDefenceRegion[r].begin(), unitNumInDefenceRegion[r].end(), [](BWAPI::Unit u) { return !u->isFlying(); });
		// figure out how many units we need on defense
		flyingDefendersNeeded *= numDefendersPerEnemyUnit;
		groundDefensersNeeded *= numDefendersPerEnemyUnit;

		updateDefenseSquadUnits(defenseSquad, flyingDefendersNeeded, groundDefensersNeeded);
	}
}

void CombatCommander::updateDefenseSquadUnits(Squad & defenseSquad, const size_t & flyingDefendersNeeded, const size_t & groundDefendersNeeded)
{
	const BWAPI::Unitset & squadUnits = defenseSquad.getUnits();
    size_t flyingDefendersInSquad = std::count_if(squadUnits.begin(), squadUnits.end(), UnitUtils::CanAttackAir);
    size_t groundDefendersInSquad = std::count_if(squadUnits.begin(), squadUnits.end(), UnitUtils::CanAttackGround);
	//////std::cout << "flyingDefendersInSquad " << flyingDefendersInSquad << " groundDefendersInSquad " << groundDefendersInSquad << std::endl;

    // add flying defenders if we still need them
    size_t flyingDefendersAdded = 0;
    while (flyingDefendersNeeded > flyingDefendersInSquad + flyingDefendersAdded)
    {
        BWAPI::Unit defenderToAdd = findClosestDefender(defenseSquad, defenseSquad.getSquadOrder().getPosition(), true);

        // if we find a valid flying defender, add it to the squad
        if (defenderToAdd)
        {
            _squadData.assignUnitToSquad(defenderToAdd, defenseSquad);
            ++flyingDefendersAdded;
        }
        // otherwise we'll never find another one so break out of this loop
        else
        {
            break;
        }
    }

    // add ground defenders if we still need them
    size_t groundDefendersAdded = 0;
    while (groundDefendersNeeded > groundDefendersInSquad + groundDefendersAdded)
    {
        BWAPI::Unit defenderToAdd = findClosestDefender(defenseSquad, defenseSquad.getSquadOrder().getPosition(), false);

        // if we find a valid ground defender add it
        if (defenderToAdd)
        {
            _squadData.assignUnitToSquad(defenderToAdd, defenseSquad);
            ++groundDefendersAdded;
        }
        // otherwise we'll never find another one so break out of this loop
        else
        {
			break;
        }
    }
}

BWAPI::Unit CombatCommander::findClosestDefender(const Squad & defenseSquad, BWAPI::Position pos, bool flyingDefender) 
{
	BWAPI::Unit closestDefender = nullptr;
	double minDistance = std::numeric_limits<double>::max();

    //int zerglingsInOurBase = numZerglingsInOurBase();
    //bool zerglingRush = zerglingsInOurBase > 0 && BWAPI::Broodwar->getFrameCount() < 5000;

	for (auto & unit : _combatUnits) 
	{
		if (((flyingDefender && !UnitUtils::CanAttackAir(unit)) || (!flyingDefender && !UnitUtils::CanAttackGround(unit))) 
			&& unit->getType() != BWAPI::UnitTypes::Terran_Medic )
        {
            continue;
        }

        if (!_squadData.canAssignUnitToSquad(unit, defenseSquad))
        {
            continue;
        }

        // add workers to the defense squad if we are being rushed very quickly
        //if (!Config::Micro::WorkersDefendRush || (unit->getType().isWorker() && !zerglingRush && !beingBuildingRushed()))
        //{
        //    continue;
        //}

        double dist = unit->getDistance(pos);
        if (!closestDefender || (dist < minDistance))
        {
            closestDefender = unit;
            minDistance = dist;
        }
	}

	return closestDefender;
}

BWAPI::Position CombatCommander::getDefendLocation()
{
	return BWTA::getRegion(BWTA::getStartLocation(BWAPI::Broodwar->self())->getTilePosition())->getCenter();
}

void CombatCommander::drawSquadInformation(int x, int y)
{
	_squadData.drawSquadInformation(x, y);
}

BWAPI::Position CombatCommander::getMainAttackLocationForCombat()
{
	BWTA::BaseLocation * enemyBaseLocation = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());
	BWTA::Chokepoint * enemyfirstCP = InformationManager::Instance().getFirstChokePoint(BWAPI::Broodwar->enemy());
	BWAPI::TilePosition home = BWAPI::Broodwar->self()->getStartLocation();
	if (enemyfirstCP)
	{
		if (!initMainAttackPath)
		{
			std::vector<BWAPI::TilePosition> tileList 
				= BWTA::getShortestPath(BWAPI::TilePosition(InformationManager::Instance().getSecondChokePoint(BWAPI::Broodwar->self())->getCenter())
				, BWAPI::TilePosition(enemyfirstCP->getCenter()));
			std::vector<std::pair<double, BWAPI::Position>> candidate_pos;
			for (auto & t : tileList) {
				BWAPI::Position tp(t.x * 32, t.y * 32);
				if (!tp.isValid())
					continue;
				if (tp.getDistance(InformationManager::Instance().getSecondChokePoint(BWAPI::Broodwar->self())->getCenter()) < 50)
					continue;
				if (tp.getDistance(enemyfirstCP->getCenter()) < 100)
					continue;
				if (!BWTA::isConnected(t, home))
				{
					continue;
				}
				mainAttackPath.push_back(tp);
			}

			initMainAttackPath = true;
		}
		else{
			Squad & mainAttackSquad = _squadData.getSquad("MainAttack");
			if (curIndex < mainAttackPath.size() / 2)
				curIndex = mainAttackPath.size() / 2;
			if (curIndex < mainAttackPath.size())
			{
				//@도주남 김지훈 만약 공격 중이다가 우리의 인원수가 줄었다면 뒤로 뺀다 ?
				if (mainAttackPath[curIndex].getDistance(mainAttackSquad.calcCenter()) 
					< mainAttackSquad.getSquadOrder().getRadius() - BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange() + mainAttackSquad.getUnits().size()*1.2)
					curIndex+=1;
				else if (mainAttackPath[curIndex].getDistance(BWAPI::Position(home)) < BWAPI::Position(home).getDistance(mainAttackSquad.calcCenter()))
				{
					curIndex += 2;
				}


				if (curIndex >= mainAttackPath.size())
					return mainAttackPath[mainAttackPath.size()-1];
				else if (curIndex < mainAttackPath.size() / 2)
				{
					curIndex = mainAttackPath.size() / 2;
					mainAttackPath[curIndex];
				}
				else
					return mainAttackPath[curIndex];
			}
		}
	}
	return getMainAttackLocation();

}

BWAPI::Position CombatCommander::getMainAttackLocation()
{
    BWTA::BaseLocation * enemyBaseLocation = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());
	
	// First choice: Attack an enemy region if we can see units inside it
    if (enemyBaseLocation)
    {
        BWAPI::Position enemyBasePosition = enemyBaseLocation->getPosition();

        // get all known enemy units in the area
        BWAPI::Unitset enemyUnitsInArea;
		MapGrid::Instance().getUnitsNear(enemyUnitsInArea, enemyBasePosition, 800, false, true);

        bool onlyOverlords = true;
        for (auto & unit : enemyUnitsInArea)
        {
            if (unit->getType() != BWAPI::UnitTypes::Zerg_Overlord)
            {
                onlyOverlords = false;
            }
        }

        if (!BWAPI::Broodwar->isExplored(BWAPI::TilePosition(enemyBasePosition)) || !enemyUnitsInArea.empty())
        {
            if (!onlyOverlords)
            {
                return enemyBaseLocation->getPosition();
            }
        }
    }

    // Second choice: Attack known enemy buildings
    for (const auto & kv : InformationManager::Instance().getUnitInfo(BWAPI::Broodwar->enemy()))
    {
        const UnitInfo & ui = kv.second;

        if (ui.type.isBuilding() && ui.lastPosition != BWAPI::Positions::None)
		{
			return ui.lastPosition;	
		}
    }

    // Third choice: Attack visible enemy units that aren't overlords
    for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
	{
        if (unit->getType() == BWAPI::UnitTypes::Zerg_Overlord)
        {
            continue;
        }

		if (UnitUtils::IsValidUnit(unit) && unit->isVisible())
		{
			return unit->getPosition();
		}
	}

    // Fourth choice: We can't see anything so explore the map attacking along the way
    return MapGrid::Instance().getLeastExplored();
}

BWAPI::Unit CombatCommander::findClosestWorkerToTarget(BWAPI::Unitset & unitsToAssign, BWAPI::Unit target)
{
    UAB_ASSERT(target != nullptr, "target was null");

    if (!target)
    {
        return nullptr;
    }

    BWAPI::Unit closestMineralWorker = nullptr;
    double closestDist = 100000;
    
    // for each of our workers
	for (auto & unit : unitsToAssign)
	{
        if (!unit->getType().isWorker())
        {
            continue;
        }

		// if it is a move worker
        if (WorkerManager::Instance().isMineralWorker(unit)) 
		{
			double dist = unit->getDistance(target);

            if (!closestMineralWorker || dist < closestDist)
            {
                closestMineralWorker = unit;
                dist = closestDist;
            }
		}
	}

    return closestMineralWorker;
}
// when do we want to defend with our workers?
// this function can only be called if we have no fighters to defend with
int CombatCommander::defendWithWorkers()
{
	// our home nexus position
	BWAPI::Position homePosition = BWTA::getStartLocation(BWAPI::Broodwar->self())->getPosition();;

	// enemy units near our workers
	int enemyUnitsNearWorkers = 0;

	// defense radius of nexus
	int defenseRadius = 300;

	// fill the set with the types of units we're concerned about
	for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
	{
		// if it's a zergling or a worker we want to defend
		if (unit->getType() == BWAPI::UnitTypes::Zerg_Zergling)
		{
			if (unit->getDistance(homePosition) < defenseRadius)
			{
				enemyUnitsNearWorkers++;
			}
		}
	}

	// if there are enemy units near our workers, we want to defend
	return enemyUnitsNearWorkers;
}

int CombatCommander::numZerglingsInOurBase()
{
    int concernRadius = 600;
    int zerglings = 0;
    BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
    
    // check to see if the enemy has zerglings as the only attackers in our base
    for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
    {
        if (unit->getType() != BWAPI::UnitTypes::Zerg_Zergling)
        {
            continue;
        }

        if (unit->getDistance(ourBasePosition) < concernRadius)
        {
            zerglings++;
        }
    }

    return zerglings;
}

bool CombatCommander::beingBuildingRushed()
{
    int concernRadius = 1200;
    BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
    
    // check to see if the enemy has zerglings as the only attackers in our base
    for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
    {
        if (unit->getType().isBuilding())
        {
            return true;
        }
    }

    return false;
}

BWAPI::Position CombatCommander::getPositionForDefenceChokePoint(BWTA::Chokepoint * chokepoint)
{
	BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
	std::vector<BWAPI::TilePosition> tpList = BWTA::getShortestPath(BWAPI::TilePosition(ourBasePosition), BWAPI::TilePosition(chokepoint->getCenter()));
	BWAPI::Position resultPosition = ourBasePosition;
	for (auto & t : tpList) {
		BWAPI::Position tp(t.x * 32, t.y * 32);
		if (!tp.isValid())
			continue;
		if (tp.getDistance(chokepoint->getCenter()) <= BWAPI::UnitTypes::Terran_Marine.groundWeapon().maxRange() + 64
			&& tp.getDistance(chokepoint->getCenter()) >= BWAPI::UnitTypes::Terran_Marine.groundWeapon().maxRange()+30)
		{
			resultPosition = tp;
		}
	}
	return resultPosition;
}

void CombatCommander::updateComBatStatus(const BWAPI::Unitset & combatUnits)
{
	InformationManager::combatStatus _combatStatus = InformationManager::combatStatus::DEFCON1;
	_combatUnits = combatUnits;
	
	int totalUnits = _combatUnits.size();
	InformationManager &im = InformationManager::Instance();

	if (StrategyManager::Instance().getMainStrategy() == Strategy::main_strategies::Bionic)
	{
		if (totalUnits < 7)
			_combatStatus = InformationManager::combatStatus::DEFCON1; //방어준비
		else{
			_combatStatus = InformationManager::combatStatus::DEFCON2; //첫번째 쵸크 안쪽 이동

			if (_combatStatus == InformationManager::combatStatus::DEFCON2 && (im.rushState == 0 && im.getRushSquad().size() > 0)){
				_combatStatus = InformationManager::combatStatus::DEFCON3; // 첫번째 초크 밖 이동
			}


			int expansionStatus = ExpansionManager::Instance().shouldExpandNow();

			//적 멀티 발견시, 적 입구 막을때는 무조건 멀티 시작

			if (expansionStatus == 2){
				_combatStatus = InformationManager::combatStatus::DEFCON4; // 두번째 초크 이동
			}

			//단, 앞마당 상황이 안좋으면 안나간다.
			else if (expansionStatus > 0){
				if (im.rushState == 1 && _combatUnits.size() > im.getRushSquad().size()){
					_combatStatus = InformationManager::combatStatus::DEFCON2; // 두번째 초크 이동
				}
				else{
					_combatStatus = InformationManager::combatStatus::DEFCON4; // 두번째 초크 이동
				}
			}

			if (_combatStatus != InformationManager::combatStatus::DEFCON4){
				/*
				커맨드 센터 있으면 뒤로 안땡긴다.
				*/
				bool goCommandCenter = false;
				for (auto & isCommandCenter : BWAPI::Broodwar->getUnitsOnTile(InformationManager::Instance().getFirstExpansionLocation(BWAPI::Broodwar->self())->getTilePosition()))
				{
					if (isCommandCenter->getType() == BWAPI::UnitTypes::Terran_Command_Center)
					{
						goCommandCenter = true;
						break;
					}
				}

				if (!goCommandCenter &&
					(BuildManager::Instance().hasUnitInQueue(BWAPI::UnitTypes::Terran_Command_Center) ||
					ConstructionManager::Instance().getConstructionQueueItemCount(BWAPI::UnitTypes::Terran_Command_Center) > 0)){
					goCommandCenter = true;
				}

				if (goCommandCenter) _combatStatus = InformationManager::combatStatus::DEFCON4;
			}

			if (totalUnits > 24)
				_combatStatus = InformationManager::combatStatus::DEFCON4; // 두번째 초크 이동

			if (totalUnits > 45)
				_combatStatus = InformationManager::combatStatus::CenterAttack; // 취약 지역 공격
		}


		if (BWAPI::Broodwar->self()->supplyTotal() > 320)
		{
			_combatStatus = InformationManager::combatStatus::EnemyBaseAttack; // 적 본진 공격
		}
	}
	else{
		int countTank = im.selfPlayer->completedUnitCount(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode) + im.selfPlayer->completedUnitCount(BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode);
		if (im.enemyRace != BWAPI::Races::Unknown){
			if (im.enemyRace == BWAPI::Races::Protoss){
				_combatStatus = InformationManager::combatStatus::DEFCON2; // 첫번째 쵸크 안쪽 이동

				int expansionStatus = ExpansionManager::Instance().shouldExpandNow();

				//적 멀티 발견시, 적 입구 막을때는 무조건 멀티 시작

				if (expansionStatus == 2){
					_combatStatus = InformationManager::combatStatus::DEFCON4; // 두번째 초크 이동
				}

				//그렇지 않으면 멀티할 돈이 생기거나 탱크가 많이 쌓이면 나간다.
				//탱크가 3대 초과되고 스파이더마인이 개발된 이후에 멀티간다.
				//단, 앞마당 상황이 안좋으면 안나간다.
				else if (expansionStatus > 0 &&
					(countTank > 0 && im.selfPlayer->completedUnitCount(BWAPI::UnitTypes::Terran_Vulture) > 1 && StrategyManager::Instance().hasTech(BWAPI::TechTypes::Tank_Siege_Mode))){
					if (im.rushState == 1 && im.getRushSquad().size() > 0){
						int z = 0;
						int d = 0;
						for (auto u : im.getRushSquad()){
							if (u.type == BWAPI::UnitTypes::Protoss_Zealot){
								z++;
							}
							if (u.type == BWAPI::UnitTypes::Protoss_Dragoon){
								d++;
							}
						}

						//벌쳐가 질럿보다 많고 탱크가 드라군/2 보다 많을때
						if (z <= im.selfPlayer->completedUnitCount(BWAPI::UnitTypes::Terran_Vulture) &&
							(d / 2 <= countTank)){
							_combatStatus = InformationManager::combatStatus::DEFCON4; // 두번째 초크 이동
						}
					}
					else{
						_combatStatus = InformationManager::combatStatus::DEFCON4; // 두번째 초크 이동
					}
				}
			}
			else{
				_combatStatus = InformationManager::combatStatus::DEFCON1;

				if (im.selfPlayer->completedUnitCount(BWAPI::UnitTypes::Terran_Factory) > 0){
					_combatStatus = InformationManager::combatStatus::DEFCON2; //첫번째 쵸크 안쪽 이동

					int expansionStatus = ExpansionManager::Instance().shouldExpandNow();

					//적 멀티 발견시, 적 입구 막을때는 무조건 멀티 시작

					if (expansionStatus == 2){
						_combatStatus = InformationManager::combatStatus::DEFCON4; // 두번째 초크 이동
					}

					//단, 앞마당 상황이 안좋으면 안나간다.
					else if (expansionStatus > 0){
						if (im.rushState == 1 && im.getRushSquad().size() > 0){
							if (im.rushState == 1 && _combatUnits.size() > im.getRushSquad().size() && im.getRushSquad().size() < 3){
								_combatStatus = InformationManager::combatStatus::DEFCON4; // 두번째 초크 이동
							}
						}
						else{
							_combatStatus = InformationManager::combatStatus::DEFCON4; // 두번째 초크 이동
						}
					}
				}
				else{
					_combatStatus = InformationManager::combatStatus::DEFCON1; //방어준비
				}
			}

			if (_combatStatus != InformationManager::combatStatus::DEFCON4){
				/*
				커맨드 센터 있으면 뒤로 안땡긴다.
				*/
				bool goCommandCenter = false;
				for (auto & isCommandCenter : BWAPI::Broodwar->getUnitsOnTile(InformationManager::Instance().getFirstExpansionLocation(BWAPI::Broodwar->self())->getTilePosition()))
				{
					if (isCommandCenter->getType() == BWAPI::UnitTypes::Terran_Command_Center)
					{
						goCommandCenter = true;
						break;
					}
				}

				if (!goCommandCenter &&
					(BuildManager::Instance().hasUnitInQueue(BWAPI::UnitTypes::Terran_Command_Center) ||
					ConstructionManager::Instance().getConstructionQueueItemCount(BWAPI::UnitTypes::Terran_Command_Center) > 0)){
					goCommandCenter = true;
				}

				if (goCommandCenter) _combatStatus = InformationManager::combatStatus::DEFCON4;
			}

			if (countTank > 9){
				_combatStatus = InformationManager::combatStatus::CenterAttack; // 취약 지역 공격
			}

			if (BWAPI::Broodwar->self()->supplyTotal() > 350)
			{
				_combatStatus = InformationManager::combatStatus::EnemyBaseAttack; // 적 본진 공격
			}
		}
	}

	//디펜스 상황 판단
	std::vector<BWTA::Region *> tmp_defenceRegions;
	std::map<BWTA::Region *, BWAPI::Unitset> tmp_unitNumInDefenceRegion;

	BWTA::BaseLocation * enemyBaseLocation = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());
	BWTA::Region * enemyRegion = nullptr;
	if (enemyBaseLocation)
	{
		enemyRegion = BWTA::getRegion(enemyBaseLocation->getPosition());
	}

	// for each of our occupied regions
	for (BWTA::Region * myRegion : InformationManager::Instance().getOccupiedRegions(BWAPI::Broodwar->self()))
	{
		// don't defend inside the enemy region, this will end badly when we are stealing gas
		if (myRegion == enemyRegion) continue;

		BWAPI::Position regionCenter = myRegion->getCenter();
		if (!regionCenter.isValid()) continue;


		// all of the enemy units in this region
		BWAPI::Unitset enemyUnitsInRegion;
		int ignoreNumWorker = 2;
		for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
		{
			if (unit->getType().isWorker() && ignoreNumWorker > 0){
				ignoreNumWorker--;
				continue;
			}

			// if it's an overlord, don't worry about it for defense, we don't care what they see
			if (unit->getType() == BWAPI::UnitTypes::Zerg_Overlord) continue;
			if (unit->getType() == BWAPI::UnitTypes::Protoss_Observer) continue;

			if (BWTA::getRegion(BWAPI::TilePosition(unit->getPosition())) == myRegion)
			{
				//////std::cout << "enemyUnits In My Region " << std::endl;
				enemyUnitsInRegion.insert(unit);
			}
		}

		// if there's nothing in this region to worry about
		if (enemyUnitsInRegion.empty())
		{
			continue;
		}
		else
		{
			// we can ignore the first enemy worker in our region since we assume it is a scout
			//int numEnemyFlyingInRegion = std::count_if(enemyUnitsInRegion.begin(), enemyUnitsInRegion.end(), [](BWAPI::Unit u) { return u->isFlying(); });
			//int numEnemyGroundInRegion = std::count_if(enemyUnitsInRegion.begin(), enemyUnitsInRegion.end(), [](BWAPI::Unit u) { return !u->isFlying(); });

			tmp_defenceRegions.push_back(myRegion);
			tmp_unitNumInDefenceRegion[myRegion] = enemyUnitsInRegion;
		}
	}

	if (tmp_defenceRegions.size() > 0){
		_combatStatus = InformationManager::combatStatus::DEFCON5; // 본진 방어
		defenceRegions = tmp_defenceRegions;
		unitNumInDefenceRegion = tmp_unitNumInDefenceRegion;
	}

	InformationManager::Instance().setCombatStatus(_combatStatus);

	if (BWAPI::Broodwar->getFrameCount() >= 16800 && InformationManager::Instance().getUnitAndUnitInfoMap(InformationManager::Instance().enemyPlayer).size() == 0){
		notDraw = true;
	}
}


BWAPI::Position CombatCommander::getFirstChokePoint_OrderPosition()
{
	BWTA::Chokepoint * startCP = InformationManager::Instance().getFirstChokePoint(BWAPI::Broodwar->self());
	BWTA::Chokepoint * endCP = InformationManager::Instance().getSecondChokePoint(BWAPI::Broodwar->self());
	if (indexFirstChokePoint_OrderPosition == -1)
	{
		std::vector<BWAPI::TilePosition> tileList = 
			BWTA::getShortestPath(BWAPI::TilePosition(startCP->getCenter())
			, BWAPI::TilePosition(endCP->getCenter()));
		for (auto & t : tileList) {
			BWAPI::Position tp(t.x * 32, t.y * 32);
			if (!tp.isValid())
				continue;
			firstChokePoint_OrderPositionPath.push_back(tp);
		}
		if (firstChokePoint_OrderPositionPath.size() > 1)
		{
			indexFirstChokePoint_OrderPosition = firstChokePoint_OrderPositionPath.size()/2;
			return firstChokePoint_OrderPositionPath[indexFirstChokePoint_OrderPosition];
		}
	}
	else{
		Squad & idleSquad = _squadData.getSquad("DEFCON4");
		if (indexFirstChokePoint_OrderPosition < firstChokePoint_OrderPositionPath.size()-1)
		{
			if (firstChokePoint_OrderPositionPath[indexFirstChokePoint_OrderPosition].getDistance(idleSquad.calcCenter()) 
				< idleSquad.getUnits().size() * 15)
				indexFirstChokePoint_OrderPosition+=2;
			
			return firstChokePoint_OrderPositionPath[indexFirstChokePoint_OrderPosition];
		}
	}
	return endCP->getCenter();
}


BWAPI::Position CombatCommander::getPoint_DEFCON4()
{
	BWTA::Chokepoint * startCP = InformationManager::Instance().getSecondChokePoint(BWAPI::Broodwar->self());
	BWTA::Chokepoint * endCP = InformationManager::Instance().getSecondChokePoint(BWAPI::Broodwar->enemy());
	if (endCP == nullptr || !endCP->getCenter().isValid())
	{
		return startCP->getCenter();
	}

	BWAPI::Position newTargetPosition(
		(endCP->getCenter()
		+ startCP->getCenter()
		+ BWAPI::Position(BWAPI::Broodwar->mapWidth()*16, BWAPI::Broodwar->mapHeight()*16)
		+ BWAPI::Position(BWAPI::Broodwar->mapWidth() * 16, BWAPI::Broodwar->mapHeight() * 16))
		/ 4);
	if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Terran)
	{
		newTargetPosition = BWAPI::Position(BWAPI::Broodwar->mapWidth() * 16, BWAPI::Broodwar->mapHeight() * 16);
	}

	if (indexFirstChokePoint_OrderPosition == -1)
	{
		BWTA::Chokepoint * startCP = InformationManager::Instance().getSecondChokePoint(BWAPI::Broodwar->self());
		BWTA::Chokepoint * endCP = InformationManager::Instance().getSecondChokePoint(BWAPI::Broodwar->enemy());
		//BWAPI::Position newTargetPosition(
		//	(endCP->getCenter()
		//	+ startCP->getCenter()
		//	+ InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition()
		//	+ InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy())->getPosition())
		//	/ 4);

		std::vector<BWAPI::TilePosition> tileList =
			BWTA::getShortestPath(BWAPI::TilePosition(startCP->getCenter())
			, BWAPI::TilePosition(newTargetPosition));
		// index 문제로 주석처리
		//indexFirstChokePoint_OrderPosition = 0;
		for (auto & t : tileList) {
			BWAPI::Position tp(t.x * 32, t.y * 32);
			if (!tp.isValid())
				continue;
			if (startCP->getRegions().first->getPolygon().isInside(tp) || startCP->getRegions().second->getPolygon().isInside(tp))
				continue;
			indexFirstChokePoint_OrderPosition++;
			firstChokePoint_OrderPositionPath.push_back(tp);
		}
		if (firstChokePoint_OrderPositionPath.size() > 1)
		{			
			if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Terran)
				return firstChokePoint_OrderPositionPath[firstChokePoint_OrderPositionPath.size() - 1];
			else
				return firstChokePoint_OrderPositionPath[0];
		}
		else
		{
			return newTargetPosition;
		}
	}
	else{
		if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Terran)
		{
			return firstChokePoint_OrderPositionPath[firstChokePoint_OrderPositionPath.size() - 1];
		}
		else
		{
			return firstChokePoint_OrderPositionPath[0];
		}
	}

	if (firstChokePoint_OrderPositionPath.size() == 0) {
		return newTargetPosition;
	}
	else {
		if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Terran)
		{
			return firstChokePoint_OrderPositionPath[firstChokePoint_OrderPositionPath.size() - 1];
		}
		else
		{
			return firstChokePoint_OrderPositionPath[0];
		}
	}
	
}

void CombatCommander::supplementSquad(){
	InformationManager & im = InformationManager::Instance();

	Squad & defcon1Squad = _squadData.getSquad("DEFCON1");
	Squad & defcon2Squad = _squadData.getSquad("DEFCON2");
	Squad & defcon3Squad = _squadData.getSquad("DEFCON3");
	Squad & defcon4Squad = _squadData.getSquad("DEFCON4");
	Squad & mainAttackSquad = _squadData.getSquad("MainAttack");

	//추가병력만 세팅
	for (auto & unit : _combatUnits){
		if (_squadData.getUnitSquad(unit) == nullptr){
			if (im.nowCombatStatus == InformationManager::combatStatus::DEFCON1){
				if (_squadData.canAssignUnitToSquad(unit, defcon1Squad))
					_squadData.assignUnitToSquad(unit, defcon1Squad);
			}
			else if (im.nowCombatStatus == InformationManager::combatStatus::DEFCON2){
				if (_squadData.canAssignUnitToSquad(unit, defcon2Squad))
					_squadData.assignUnitToSquad(unit, defcon2Squad);
			}
			else if (im.nowCombatStatus == InformationManager::combatStatus::DEFCON3){
				if (_squadData.canAssignUnitToSquad(unit, defcon3Squad))
					_squadData.assignUnitToSquad(unit, defcon3Squad); // 오타 존재
			}
			else if (im.nowCombatStatus == InformationManager::combatStatus::DEFCON4){
				if (_squadData.canAssignUnitToSquad(unit, defcon4Squad))
					_squadData.assignUnitToSquad(unit, defcon4Squad);
			}
			else if (im.nowCombatStatus == InformationManager::combatStatus::DEFCON5){
				//디펜스지역 중 가장 가까운 지역으로 이동
				std::pair<int, BWAPI::Position> minDistRegion;
				minDistRegion.first = 100000000;
				for (auto r : defenceRegions)
				{
					int tmpDist = unit->getPosition().getDistance(r->getCenter());
					if (tmpDist < minDistRegion.first){
						minDistRegion.first = tmpDist;
						minDistRegion.second = BWAPI::Position(r->getCenter());
					}
				}

				if (minDistRegion.first < 100000000){
					std::stringstream squadName;
					squadName << "DEFCON5" << minDistRegion.second.x << "_" << minDistRegion.second.y;

					if (_squadData.squadExists(squadName.str()))
					{
						_squadData.assignUnitToSquad(unit, _squadData.getSquad(squadName.str()));
					}
				}
			}
			else if (im.nowCombatStatus == InformationManager::combatStatus::CenterAttack){
				if (_squadData.canAssignUnitToSquad(unit, mainAttackSquad))
					_squadData.assignUnitToSquad(unit, mainAttackSquad);
			}
			else if (im.nowCombatStatus == InformationManager::combatStatus::EnemyBaseAttack){
				_squadData.assignUnitToSquad(unit, mainAttackSquad);
			}
		}
	}
}

void CombatCommander::updateScoutSquads()
{
	if (!BWAPI::Broodwar->self()->hasResearched(BWAPI::TechTypes::Spider_Mines))
		return;

	Squad & scoutSquad = _squadData.getSquad("scout");
	if (BWAPI::Broodwar->getFrameCount() % 14000 == 0)
		scoutSquad.clear();
	if (scoutSquad.getUnits().size() == 0 && BWAPI::Broodwar->getFrameCount() % 1440 != 0)
		return;
	if (InformationManager::Instance().rushState == 1)
	{
		if (scoutSquad.getUnits().size() >= 1)
			scoutSquad.clear();
		return;
	}
	if (scoutSquad.getUnits().size() >= 1)
		return;
	else
	{
		for (auto & unit : _combatUnits){
			if (unit->getType() == BWAPI::UnitTypes::Terran_Vulture && scoutSquad.getUnits().size() < 1)
			{
				if (_squadData.canAssignUnitToSquad(unit, scoutSquad))
					_squadData.assignUnitToSquad(unit, scoutSquad);
			}
		}
	}
}

void CombatCommander::updateBunkertSquads()
{
	int bunkerNum = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Terran_Bunker);
	if (bunkerNum == 0)
	{
		if (_squadData.squadExists("bunker"))
		{
			Squad & bunkerSquad = _squadData.getSquad("bunker");
			bunkerSquad.clear();
		}
		return;
	}
	BWAPI::Unit bunker =
		UnitUtils::GetFarUnitTypeToTarget(BWAPI::UnitTypes::Terran_Bunker, BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation()));
	if (lastBunker == nullptr || !lastBunker->exists())
	{
		lastBunker = bunker;
	}
	else if (lastBunker != bunker)
	{
		for (auto & inUnit : lastBunker->getLoadedUnits())
		{
			inUnit->unload(lastBunker);
		}
	}

	SquadOrder bunkerOrder(SquadOrderTypes::Idle, bunker->getPosition(), 300, std::make_pair(bunker->getPosition(), bunker->getPosition()), "bunker");
	if (!_squadData.squadExists("bunker"))
		_squadData.addSquad("bunker", Squad("bunker", bunkerOrder, BunkerPriority));

	Squad & bunkerSquad = _squadData.getSquad("bunker");
	bunkerSquad.setSquadOrder(bunkerOrder);


	Squad & defcon1Squad = _squadData.getSquad("DEFCON1");
	Squad & defcon2Squad = _squadData.getSquad("DEFCON2");


	//SquadOrder saveBunker(SquadOrderTypes::Idle, bunker->getPosition()
	//	, BWAPI::UnitTypes::Terran_Marine.groundWeapon().maxRange() + 10, "SAVEBUNKER");
	//
	//defcon1Squad.setSquadOrder(saveBunker);
	//defcon2Squad.setSquadOrder(saveBunker);
	//defcon3Squad.setSquadOrder(saveBunker);
	//defcon4Squad.setSquadOrder(saveBunker);

	if (bunkerSquad.getUnits().size() >= 4)
		return;
	else
	{
		for (auto & unit : _combatUnits){
			if (unit->getType() == BWAPI::UnitTypes::Terran_Marine && bunkerSquad.getUnits().size() < 4)
			{
				if(_squadData.canAssignUnitToSquad(unit, bunkerSquad))
					_squadData.assignUnitToSquad(unit, bunkerSquad);
			}
		}
	}
}

void CombatCommander::saveAllSquadSecondChokePoint()
{
	//Squad & defcon1Squad = _squadData.getSquad("DEFCON1");
	//Squad & defcon2Squad = _squadData.getSquad("DEFCON2");
	//Squad & defcon3Squad = _squadData.getSquad("DEFCON3");
	//Squad & defcon4Squad = _squadData.getSquad("DEFCON4");
	//
	//bool goCommandCenter = false;
	//for (auto & isCommandCenter : BWAPI::Broodwar->getUnitsOnTile(InformationManager::Instance().getFirstExpansionLocation(BWAPI::Broodwar->self())->getTilePosition()))
	//{
	//	if (isCommandCenter->getType() == BWAPI::UnitTypes::Terran_Command_Center)
	//	{
	//		goCommandCenter = true;
	//		break;
	//	}
	//}
	//
	//if (goCommandCenter)
	//{
	//
	//	InformationManager & im = InformationManager::Instance();
	//
	//	SquadOrder defcon4Order(SquadOrderTypes::Idle, im.getSecondChokePoint(im.selfPlayer)->getCenter(),
	//		BWAPI::UnitTypes::Terran_Marine.groundWeapon().maxRange() + 30, im.getSecondChokePoint(im.selfPlayer)->getSides(), "DEFCON4");
	//
	//	defcon1Squad.setSquadOrder(defcon4Order);
	//	defcon2Squad.setSquadOrder(defcon4Order);
	//	defcon3Squad.setSquadOrder(defcon4Order);
	//	defcon4Squad.setSquadOrder(defcon4Order);
	//}
	//else
	//	return;
}

void CombatCommander::updateSmallAttackSquad()
{
	std::list<BWTA::BaseLocation *> enemayBaseLocations = InformationManager::Instance().getOccupiedBaseLocations(BWAPI::Broodwar->enemy());
	
	BWTA::BaseLocation * eBase = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());
	BWTA::BaseLocation * eFirstMulti = InformationManager::Instance().getFirstExpansionLocation(BWAPI::Broodwar->enemy());
	BWTA::BaseLocation * attackBaseLocation = nullptr;
	for (BWTA::BaseLocation * startLocation : enemayBaseLocations)
	{
		if (startLocation == eBase || startLocation == eFirstMulti)
			continue;
		attackBaseLocation = startLocation;
		break;
	}

	if (attackBaseLocation == nullptr)
	{
		if (_squadData.squadExists("SMALLATTACK"))
		{
			Squad & smallAttackSquad = _squadData.getSquad("SMALLATTACK");
			smallAttackSquad.clear();
		}
		return;
	}

	Squad & smallAttackSquad = _squadData.getSquad("SMALLATTACK");
	if (smallAttackSquad.getUnits().size() > 11 && attackBaseLocation->getPosition() == smallAttackSquad.getSquadOrder().getPosition())
		return;
	
	int vultureCount = BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Terran_Vulture)
		- BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode)
		- BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode) - smallAttackSquad.getUnits().size();
	if (vultureCount <= 0)
		return;

	for (auto & unit : _combatUnits)
	{
		if(smallAttackSquad.getUnits().size() > 11)
			break;

		if (_squadData.canAssignUnitToSquad(unit, smallAttackSquad) && unit->getType() == BWAPI::UnitTypes::Terran_Vulture)
		{

			_squadData.assignUnitToSquad(unit, smallAttackSquad);
		}
	}

	SquadOrder smallAttackOrder(SquadOrderTypes::Attack, attackBaseLocation->getPosition(), 200, "Attack Enemy multi");
	smallAttackSquad.setSquadOrder(smallAttackOrder);
}
