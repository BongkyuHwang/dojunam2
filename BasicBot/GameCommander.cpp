#include "GameCommander.h"
#include <direct.h>

using namespace MyBot;

GameCommander::GameCommander(){
	isToFindError = true;
	//@도주남 김지훈
	_initialScoutSet = false;

	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	//tstruct = *localtime(&now);
	localtime_s(&tstruct, &now);
	strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", &tstruct);

	log_file_path = Config::Strategy::WriteDir + std::string(buf) + ".log";
	
	//std::cout << "log_file_path:" << log_file_path << std::endl;
	
}
GameCommander::~GameCommander(){
}

void GameCommander::onStart() 
{
	BWAPI::TilePosition startLocation = BWAPI::Broodwar->self()->getStartLocation();
	if (startLocation == BWAPI::TilePositions::None || startLocation == BWAPI::TilePositions::Unknown) {
		return;
	}

	//맵정보 세팅
	//InformationManager::Instance().onStart();

	//초기화용
	BOSS::init();

	//초기빌드 세팅
	StrategyManager::Instance().onStart();

	//맵정보에 따른 resourcedepot 당 일꾼 최대수 결정
	BuildManager::Instance().onStart();

	//맵정보 초기세팅(리젼별 경계세팅)
	MapGrid::Instance().onStart();

	initHistory();

	//리젼 정보 확인용
	//BWAPI의 region과 BWTA의 region은 완전 다름
	/*
	for (auto &r : BWTA::getRegions()){
		//std::cout << r->getCenter() << "/" << r->getPolygon().getArea() << std::endl;
	}

	for (auto &u : BWAPI::Broodwar->self()->getUnits()){
		BWTA::Region *r = BWTA::getRegion(u->getPosition());
		//std::cout << "my base-" << r->getCenter() << "/" << r->getPolygon().getArea() << std::endl;
	}
	for (auto &u : BWAPI::Broodwar->self()->getUnits()){
		//std::cout << u->getType() << ":" << u->getType().width() << "," << u->getType().height() << "/" << u->getPosition() << std::endl;
	}
	*/
}
void GameCommander::onEnd(bool isWinner)
{
	closeHistory(isWinner);
	StrategyManager::Instance().onEnd(isWinner);
}

void GameCommander::onFrame()
{
	if (BWAPI::Broodwar->isPaused() 
		|| BWAPI::Broodwar->self() == nullptr || BWAPI::Broodwar->self()->isDefeated() || BWAPI::Broodwar->self()->leftGame()
		|| BWAPI::Broodwar->enemy() == nullptr || BWAPI::Broodwar->enemy()->isDefeated() || BWAPI::Broodwar->enemy()->leftGame()) {
		return;
	}
	log_write(BWAPI::Broodwar->getFrameCount());


	double total_duration = 0;
	std::vector<double> durations;
	std::string manager[12] = {"Information", "MapGrid", "Boss", "Worker", "Build", "Construction", "Scout", "Strategy", "handleUnitAssignments", "Combat", "Comsat", "Expansion"};
	int start = clock();
	// 아군 베이스 위치. 적군 베이스 위치. 각 유닛들의 상태정보 등을 Map 자료구조에 저장/업데이트
	InformationManager::Instance().update();
	durations.push_back((clock() - start) / double(CLOCKS_PER_SEC) * 1000);
	total_duration += durations.back();
	log_write(":InformationManager, ");
	std::cout << "InformationManager" << std::endl;

	// 각 유닛의 위치를 자체 MapGrid 자료구조에 저장
	start = clock();
	MapGrid::Instance().update();
	durations.push_back((clock() - start) / double(CLOCKS_PER_SEC) * 1000); 
	total_duration += durations.back();
	log_write("MapGrid, ");
	std::cout << "MapGrid" << std::endl;

	start = clock();
	BOSSManager::Instance().update(30.0); //순서가 중요?
	durations.push_back((clock() - start) / double(CLOCKS_PER_SEC) * 1000);
	total_duration += durations.back();
	log_write("BOSSManager, ");
	std::cout << "BOSSManager" << std::endl;

	// economy and base managers
	// 일꾼 유닛에 대한 명령 (자원 채취, 이동 정도) 지시 및 정리
	start = clock();
	WorkerManager::Instance().update();
	durations.push_back((clock() - start) / double(CLOCKS_PER_SEC) * 1000);
	total_duration += durations.back();
	log_write("WorkerManager, ");
	std::cout << "WorkerManager" << std::endl;

	// 빌드오더큐를 관리하며, 빌드오더에 따라 실제 실행(유닛 훈련, 테크 업그레이드 등)을 지시한다.
	start = clock();
	BuildManager::Instance().update();
	durations.push_back((clock() - start) / double(CLOCKS_PER_SEC) * 1000);
	total_duration += durations.back();
	log_write("BuildManager, ");
	std::cout << "BuildManager" << std::endl;

	// 빌드오더 중 건물 빌드에 대해서는, 일꾼유닛 선정, 위치선정, 건설 실시, 중단된 건물 빌드 재개를 지시한다
	start = clock();
	ConstructionManager::Instance().update();
	durations.push_back((clock() - start) / double(CLOCKS_PER_SEC) * 1000);
	total_duration += durations.back();
	log_write("ConstructionManager, ");
	std::cout << "ConstructionManager" << std::endl;

	// 게임 초기 정찰 유닛 지정 및 정찰 유닛 컨트롤을 실행한다
	start = clock();
	ScoutManager::Instance().update();
	durations.push_back((clock() - start) / double(CLOCKS_PER_SEC) * 1000);
	total_duration += durations.back();
	log_write("ScoutManager, ");
	std::cout << "ScoutManager" << std::endl;

	// 전략적 판단 및 유닛 컨트롤
	start = clock();
	StrategyManager::Instance().update();
	durations.push_back((clock() - start) / double(CLOCKS_PER_SEC) * 1000);
	total_duration += durations.back();
	log_write("StrategyManager, ");
	std::cout << "StrategyManager" << std::endl;

	//@도주남 김지훈 전투유닛 셋팅
	start = clock();
	handleUnitAssignments();
	durations.push_back((clock() - start) / double(CLOCKS_PER_SEC) * 1000);
	total_duration += durations.back();

	start = clock();
	CombatCommander::Instance().updateComBatStatus(_combatUnits);

	if (InformationManager::Instance().changeConmgeStatusFrame == BWAPI::Broodwar->getFrameCount()){
		//std::cout << "combat status:" << InformationManager::Instance().nowCombatStatus << "/" << InformationManager::Instance().lastCombatStatus
//			<< "/" << InformationManager::Instance().changeConmgeStatusFrame 
//			<< "(size:" << _combatUnits.size() << ")"
//			<< std::endl;
	}

	CombatCommander::Instance().update();
	durations.push_back((clock() - start) / double(CLOCKS_PER_SEC) * 1000);
	total_duration += durations.back();

	log_write("CombatCommander, ");
	std::cout << "CombatCommander" << std::endl;

	start = clock();
	ComsatManager::Instance().update();
	durations.push_back((clock() - start) / double(CLOCKS_PER_SEC) * 1000);
	total_duration += durations.back();

	log_write("ComsatManager, ");
	std::cout << "ComsatManager" << std::endl;

	start = clock();
	ExpansionManager::Instance().update(); //본진 및 확장정보 저장, 가스/컴셋 주기적으로 생성
	durations.push_back((clock() - start) / double(CLOCKS_PER_SEC) * 1000);
	total_duration += durations.back();

	log_write("ExpansionManager");
	std::cout << "ExpansionManager" << std::endl;

	if (total_duration > 55) {
		std::cout << "frame : " << BWAPI::Broodwar->getFrameCount();
		int i = 0;
		for (double duration : durations) {
			std::cout << manager[i++] << " : " << duration << "|";
		}
		std::cout << std::endl;
	}
}

// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
// 일꾼 탄생/파괴 등에 대한 업데이트 로직 버그 수정 : onUnitShow 가 아니라 onUnitComplete 에서 처리하도록 수정
void GameCommander::onUnitShow(BWAPI::Unit unit)			
{ 
	InformationManager::Instance().onUnitShow(unit); 

	// ResourceDepot 및 Worker 에 대한 처리
	//WorkerManager::Instance().onUnitShow(unit);
}

// BasicBot 1.1 Patch End //////////////////////////////////////////////////

void GameCommander::onUnitHide(BWAPI::Unit unit)			
{
	InformationManager::Instance().onUnitHide(unit); 
}

void GameCommander::onUnitCreate(BWAPI::Unit unit)
{ 
	InformationManager::Instance().onUnitCreate(unit);
}

// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
// 일꾼 탄생/파괴 등에 대한 업데이트 로직 버그 수정 : onUnitShow 가 아니라 onUnitComplete 에서 처리하도록 수정
void GameCommander::onUnitComplete(BWAPI::Unit unit)
{
	InformationManager::Instance().onUnitComplete(unit);
	ExpansionManager::Instance().onUnitComplete(unit); //본진 및 확장정보 저장, 가스/컴셋 주기적으로 생성
	BuildManager::Instance().onUnitComplete(unit);

	// ResourceDepot 및 Worker 에 대한 처리
	WorkerManager::Instance().onUnitComplete(unit);
}

// BasicBot 1.1 Patch End //////////////////////////////////////////////////

void GameCommander::onUnitDestroy(BWAPI::Unit unit)
{
	InformationManager::Instance().onUnitDestroy(unit);
	ExpansionManager::Instance().onUnitDestroy(unit); //본진 및 확장정보 저장, 가스/컴셋 주기적으로 생성
	// ResourceDepot 및 Worker 에 대한 처리
	WorkerManager::Instance().onUnitDestroy(unit);

	if (_scoutUnits.contains(unit)) { _scoutUnits.erase(unit); }
}

void GameCommander::onUnitRenegade(BWAPI::Unit unit)
{
	// Vespene_Geyser (가스 광산) 에 누군가가 건설을 했을 경우
	//BWAPI::Broodwar->sendText("A %s [%p] has renegaded. It is now owned by %s", unit->getType().c_str(), unit, unit->getPlayer()->getName().c_str());

	InformationManager::Instance().onUnitRenegade(unit);
}

void GameCommander::onUnitMorph(BWAPI::Unit unit)
{ 
	InformationManager::Instance().onUnitMorph(unit);

	// Zerg 종족 Worker 의 Morph 에 대한 처리
	WorkerManager::Instance().onUnitMorph(unit);
}

void GameCommander::onUnitDiscover(BWAPI::Unit unit)
{
}

void GameCommander::onUnitEvade(BWAPI::Unit unit)
{
}

// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
// onNukeDetect, onPlayerLeft, onSaveGame 이벤트를 처리할 수 있도록 메소드 추가

void GameCommander::onNukeDetect(BWAPI::Position target)
{
}

void GameCommander::onPlayerLeft(BWAPI::Player player)
{
}

void GameCommander::onSaveGame(std::string gameName)
{
}

// BasicBot 1.1 Patch End //////////////////////////////////////////////////

void GameCommander::onSendText(std::string text)
{
	Kyj::Instance().onSendText(text);
}

void GameCommander::onReceiveText(BWAPI::Player player, std::string text)
{	
}

//@도주남 김지훈 
void GameCommander::handleUnitAssignments()
{
	//_validUnits.clear();
	_combatUnits.clear();

	// filter our units for those which are valid and usable
	//setValidUnits();

	// set each type of unit
	//djn ssh
	setScoutUnits();
	setCombatUnits();
}
//djn ssh

void GameCommander::setScoutUnits()
{
	// if we haven't set a scout unit, do it
	if (_scoutUnits.empty() && !_initialScoutSet)
	{
		BWAPI::Unit supplyProvider;// = getFirstSupplyProvider();

		bool flag = false;
		for (auto & unit : BWAPI::Broodwar->self()->getUnits())
		{
			if (unit->getType().isBuilding() == true && unit->getType().isResourceDepot() == false)
			{
				supplyProvider = unit;
				flag = true;
				break;
			}
		}
		// if it exists
		////std::cout << "error??" << std::endl;
		if (flag)
		{
			// grab the closest worker to the supply provider to send to scout
			BWAPI::Unit workerScout = WorkerManager::Instance().getClosestMineralWorkerTo(supplyProvider->getPosition());
			//	getClosestWorkerToTarget(supplyProvider->getPosition());

			// if we find a worker (which we should) add it to the scout units
			if (workerScout)
			{
				ScoutManager::Instance().setWorkerScout(workerScout);
				assignUnit(workerScout, _scoutUnits);
				_initialScoutSet = true;
			}
		}
		////std::cout << "Lucky??" << std::endl;
	}
	else if (_scoutUnits.empty() && _initialScoutSet == true && ScoutManager::Instance().second_scout == false && !InformationManager::Instance().getWallStatus())
	{
		BWAPI::Unit workerScout = nullptr;


		for (auto & unit : BWAPI::Broodwar->self()->getUnits())
		{
			if (!unit)
			{
				continue;
			}

			if (unit->isCompleted()
				&& unit->getHitPoints() > 0
				&& unit->exists()
				&& unit->getType().isWorker()
				&& !unit->isCarryingMinerals()
				&& WorkerManager::Instance().isMineralWorker(unit))
			{
				workerScout = unit;
				break;
			}
		}
		//	getClosestWorkerToTarget(supplyProvider->getPosition());

		// if we find a worker (which we should) add it to the scout units
		if (workerScout)
		{
			ScoutManager::Instance().setWorkerScout(workerScout);
			assignUnit(workerScout, _scoutUnits);
			ScoutManager::Instance().second_scout = true;
		}
	}
}

//@도주남 김지훈 // 전투유닛을 setting 해주는 부분 기존 로직과 다르게 적용함.
void GameCommander::setCombatUnits()
{
	int combatunitCount = 0;
	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (_scoutUnits.contains(unit) || unit->getType() == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine)
			continue;
		
		if (UnitUtils::IsValidUnit(unit))
			if ((unit->getType().isBuilding() && unit->isFlying() )
				|| (UnitUtils::IsCombatUnit(unit) && !unit->getType().isWorker()) || unit->getType() == BWAPI::UnitTypes::Terran_Science_Vessel )
			{
				if ((unit->getType().isBuilding() && unit->isFlying()) && unit->getType() == BWAPI::UnitTypes::Terran_Barracks)
					continue;
				//unit->getOrder
				BWAPI::UnitCommand currentCommand(unit->getLastCommand());

				//@도주남 김지훈 일꾼이 아니면 넣는다.
				{
					assignUnit(unit, _combatUnits);
					combatunitCount++;
				}
			}
	}
	//if (combatunitCount!=0)
	//	//std::cout << "공격 유닛 [" << combatunitCount  <<"]명 셋팅 !" << std::endl;
}


void GameCommander::assignUnit(BWAPI::Unit unit, BWAPI::Unitset & set)
{
	//@도주남 김지훈 다른 유닛set에 포함되였는지를 확인하고 제거해주는 로직이 존재 하지만, 지금 전투유닛만 쓸꺼니까 필요없음;
	//    if (_scoutUnits.contains(unit)) { _scoutUnits.erase(unit); }
	//    else if (_combatUnits.contains(unit)) { _combatUnits.erase(unit); }

	set.insert(unit);
}

void GameCommander::initHistory(){
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	char       buf2[80];
	//tstruct = *localtime(&now);
	localtime_s(&tstruct, &now);
	strftime(buf, sizeof(buf), "%Y-%m-%d-%H-%M-%S", &tstruct);
	strftime(buf2, sizeof(buf2), "_%m%d-%H%M", &tstruct);

	history_file_path = Config::Strategy::WriteDir + BWAPI::Broodwar->enemy()->getName() + std::string(buf2) + ".dat";

	history_write("start_time:" + std::string(buf), true);
	history_write("enemy_name:" + BWAPI::Broodwar->enemy()->getName(), true);

	if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Terran)
	{
		history_write("start_enemy_race:Terran", true);
	}
	else if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Protoss)
	{
		history_write("start_enemy_race:Protoss", true);
	}
	else if (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Zerg)
	{
		history_write("start_enemy_race:Zerg", true);
	}
	else
	{
		history_write("start_enemy_race:Random", true);
	}
	
	history_write("map_name:" + BWAPI::Broodwar->mapFileName(), true);
}

void GameCommander::closeHistory(bool isWinner){
	if (isWinner)
	{
		history_write("am_i_winner:true", true);
	}
	else{
		history_write("am_i_winner:false", true);
	}

	for (const BWAPI::UnitType & t : BWAPI::UnitTypes::allUnitTypes())
	{
		int num = InformationManager::Instance().getUnitData(BWAPI::Broodwar->self()).getNumCreatedUnits(t);
		if (num > 0){
			history_write("self_" + InformationManager::Instance().unitTypeString(t.getID()) + ":" + std::to_string(num), true);
		}
	}

	history_write("self_mineralsLost:" + std::to_string(InformationManager::Instance().getUnitData(BWAPI::Broodwar->self()).getMineralsLost()), true);
	history_write("self_gasLost:" + std::to_string(InformationManager::Instance().getUnitData(BWAPI::Broodwar->self()).getGasLost()), true);
	
	
	if (InformationManager::Instance().enemyRace == BWAPI::Races::Terran)
	{
		history_write("end_enemy_race:Terran", true);
	}
	else if (InformationManager::Instance().enemyRace == BWAPI::Races::Protoss)
	{
		history_write("end_enemy_race:Protoss", true);
	}
	else if (InformationManager::Instance().enemyRace == BWAPI::Races::Zerg)
	{
		history_write("end_enemy_race:Zerg", true);
	}
	else
	{
		history_write("end_enemy_race:Random", true);
	}

	for (const BWAPI::UnitType & t : BWAPI::UnitTypes::allUnitTypes())
	{
		int num = InformationManager::Instance().getUnitData(BWAPI::Broodwar->enemy()).getNumCreatedUnits(t);
		if (num > 0){
			history_write("enemy_" + InformationManager::Instance().unitTypeString(t.getID()) + ":" + std::to_string(num), true);
		}
	}

	history_write("enemy_mineralsLost:" + std::to_string(InformationManager::Instance().getUnitData(BWAPI::Broodwar->enemy()).getMineralsLost()), true);
	history_write("enemy_gasLost:" + std::to_string(InformationManager::Instance().getUnitData(BWAPI::Broodwar->enemy()).getGasLost()), true);

	history_write("end:true", true);
}
