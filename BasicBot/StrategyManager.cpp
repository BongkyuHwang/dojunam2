#include "StrategyManager.h"
#include "CommandUtil.h"

using namespace MyBot;

StrategyManager & StrategyManager::Instance()
{
	static StrategyManager instance;
	return instance;
}

StrategyManager::StrategyManager()
{
	isFullScaleAttackStarted = false;
	isInitialBuildOrderFinished = false;
	firstChokeBunker = false;
	buildSeedPositionStrategy = BuildOrderItem::SeedPositionStrategy::MainBaseLocation;
}

void StrategyManager::onStart()
{
	if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Terran)
	{
		setOpeningBookBuildOrder();
	}
}

void StrategyManager::setOpeningBookBuildOrder(){
	//전체 전략 세팅
	initStrategies();

	//초기 전략 선택
	if (InformationManager::Instance().enemyRace == BWAPI::Races::Terran){
		_main_strategy = Strategy::main_strategies::One_Fac;
	}
	else if (InformationManager::Instance().enemyRace == BWAPI::Races::Zerg){
		_main_strategy = Strategy::main_strategies::Bionic;
	}
	else if (InformationManager::Instance().enemyRace == BWAPI::Races::Protoss){
		//_main_strategy = Strategy::main_strategies::Mechanic;
		//_main_strategy = Strategy::main_strategies::Two_Fac;
		//_main_strategy = Strategy::main_strategies::One_Fac_Tank;
		_main_strategy = Strategy::main_strategies::One_Fac;
	}
	else{
		_main_strategy = Strategy::main_strategies::Bionic;
	}

	//초기 빌드 큐에 세팅
	std::istringstream iss(_strategies[_main_strategy].opening_build_order);
	std::vector<std::string> vec_build_order{ std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{} };
	for (auto &i : vec_build_order){
		_openingBuildOrder.add(MetaType(i));
	}

	//@도주남 김유진 유닛비율 테이블 - 마린대메딕 비율
	initUnitRatioTable();
}

void StrategyManager::onEnd(bool isWinner)
{
}

void StrategyManager::update()
{
	//1. 초기 빌드는 onstart에서 세팅
	//2. todo) update에서 초기빌드 깨졌는지 확인
	//3. 초기빌드가 깨지거나 초기빌드를 다 사용하면 isInitialBuildOrderFinished 세팅하여 다이나믹 빌드오더 체제로 전환
	if (BuildManager::Instance().buildQueue.isEmpty()) {
		isInitialBuildOrderFinished = true;
	}
	/*
	int numBunkers = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Bunker);
	if (InformationManager::Instance().nowCombatStatus == InformationManager::combatStatus::DEFCON3 && !firstChokeBunker){
		if (ConstructionManager::Instance().getConstructionQueueItemCount(BWAPI::UnitTypes::Terran_Bunker) == 0 &&
			!BuildManager::Instance().hasUnitInQueue(BWAPI::UnitTypes::Terran_Bunker)){
			MetaType bunker(BWAPI::UnitTypes::Terran_Bunker);
			BuildManager::Instance().addBuildOrderOneItem(bunker, BWAPI::TilePositions::None, getBuildSeedPositionStrategy(bunker));
			if (ExpansionManager::Instance().getExpansions()[0].cc->isTraining() &&
				ExpansionManager::Instance().getExpansions()[0].cc->getLastCommand().getUnitType() == BWAPI::UnitTypes::Terran_SCV &&
				ExpansionManager::Instance().getExpansions()[0].cc->canCancelTrain()
				){
				ExpansionManager::Instance().getExpansions()[0].cc->cancelTrain();
			}
			firstChokeBunker = true;
		}
	}
	*/
	//executeWorkerTraining();

	//executeSupplyManagement();

	//executeBasicCombatUnitTraining();

	//executeCombat();
	if (TimeManager::Instance().isMyTurn("StrategyManager_update", 24)) {
		//liftAndMoveBarrackFromWall();
		liftBarrackFromWall();
	}
}

const BuildOrder & StrategyManager::getOpeningBookBuildOrder() const
{
	return _openingBuildOrder;
}

// 일꾼 계속 추가 생산
void StrategyManager::executeWorkerTraining()
{
	// InitialBuildOrder 진행중에는 아무것도 하지 않습니다
	if (isInitialBuildOrderFinished == false) {
		return;
	}

	if (BWAPI::Broodwar->self()->minerals() >= 50) {
		// workerCount = 현재 일꾼 수 + 생산중인 일꾼 수
		int workerCount = BWAPI::Broodwar->self()->allUnitCount(InformationManager::Instance().getWorkerType());

		if (BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Zerg) {

			for (auto & unit : BWAPI::Broodwar->self()->getUnits())
			{
				if (unit->getType() == BWAPI::UnitTypes::Zerg_Egg) {
					// Zerg_Egg 에게 morph 명령을 내리면 isMorphing = true, isBeingConstructed = true, isConstructing = true 가 된다
					// Zerg_Egg 가 다른 유닛으로 바뀌면서 새로 만들어진 유닛은 잠시 isBeingConstructed = true, isConstructing = true 가 되었다가, 
					if (unit->isMorphing() && unit->getBuildType() == BWAPI::UnitTypes::Zerg_Drone) {
						workerCount++;
					}
				}
			}
		}
		else {
			for (auto & unit : BWAPI::Broodwar->self()->getUnits())
			{
				if (unit->getType().isResourceDepot())
				{
					if (unit->isTraining()) {
						workerCount += unit->getTrainingQueue().size();
					}
				}
			}
		}

		if (workerCount < 30) {
			for (auto & unit : BWAPI::Broodwar->self()->getUnits())
			{
				if (unit->getType().isResourceDepot())
				{
					if (unit->isTraining() == false || unit->getLarva().size() > 0) {

						// 빌드큐에 일꾼 생산이 1개는 있도록 한다
						if (BuildManager::Instance().buildQueue.getItemCount(InformationManager::Instance().getWorkerType()) == 0) {
							////std::cout << "worker enqueue" << std::endl;
							BuildManager::Instance().buildQueue.queueAsLowestPriority(MetaType(InformationManager::Instance().getWorkerType()), false);
						}
					}
				}
			}
		}
	}
}

const MetaPairVector StrategyManager::getBuildOrderGoal()
{
	//초기빌드 이후에만 작업가능
	if (!isInitialBuildOrderFinished) false;

	BWAPI::Race myRace = BWAPI::Broodwar->self()->getRace();

	if (myRace == BWAPI::Races::Protoss)
	{
		return MetaPairVector();
	}
	else if (myRace == BWAPI::Races::Terran)
	{
		return getTerranBuildOrderGoal();
	}
	else
	{
		return MetaPairVector();
	}
}


bool StrategyManager::changeMainStrategy(std::map<std::string, int> & numUnits){
	//@도주남 김유진 다음 빌드오더로 변환체크
	//std::cout << "main stragety:" << _main_strategy << " next_strategy:" << _strategies[_main_strategy].next_strategy << std::endl;
	//std::cout << " num_marines:" << numUnits["Marines"] << " limit_marines:" << _strategies[_main_strategy].num_unit_limit["Marines"] << std::endl;
	if (_strategies[_main_strategy].next_strategy != Strategy::main_strategies::None){
		bool _changeStrategy = true;
		for (auto &i : _strategies[_main_strategy].num_unit_limit){
			if (numUnits[i.first] < i.second){
				_changeStrategy = false;
				break;
			}
		}

		if (_changeStrategy){
			_main_strategy = _strategies[_main_strategy].next_strategy;
		}
	}

	//@도주남 김유진 이전 빌드오더로 변환체크
	if (_strategies[_main_strategy].pre_strategy != Strategy::main_strategies::None){
		bool _changeStrategy = true;
		for (auto &i : _strategies[_main_strategy].num_unit_limit){
			if (i.second != -1 && numUnits[i.first] > i.second){
				_changeStrategy = false;
				break;
			}
		}

		if (_changeStrategy){
			_main_strategy = _strategies[_main_strategy].pre_strategy;
		}
	}

	if (_main_strategy == Strategy::main_strategies::Mechanic || _main_strategy == Strategy::main_strategies::One_Fac || _main_strategy == Strategy::main_strategies::Two_Fac){
		if (InformationManager::Instance().hasFlyingUnits){
			_main_strategy = Strategy::main_strategies::Mechanic_Goliath;
			return true;
		}
	}

	if (_main_strategy == Strategy::main_strategies::Mechanic || Strategy::main_strategies::Mechanic_Goliath) {

		// 황봉규
		// 인구수 198 넘으면 배틀크루저 전략으로 넘어가는 것에서
		// mineral과 gas 모두 5000이상일 때 넘어간다
		if (BWAPI::Broodwar->self()->supplyUsed() >= 396 && BWAPI::Broodwar->self()->minerals() > 5000 && BWAPI::Broodwar->self()->gas() > 5000) {
			_main_strategy = Strategy::main_strategies::BATTLE;
			return true;
		}
	}

	if (InformationManager::Instance().enemyRace == BWAPI::Races::Terran){

	}
	else if (InformationManager::Instance().enemyRace == BWAPI::Races::Protoss){

	}
	else if (InformationManager::Instance().enemyRace == BWAPI::Races::Zerg){

	}


	return false;
}

const MetaPairVector StrategyManager::getTerranBuildOrderGoal()
{
	// the goal to return
	std::vector<MetaPair> goal;

	std::map<std::string, int> numUnits;

	numUnits["Workers"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_SCV);
	numUnits["CC"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Command_Center);
	numUnits["Marines"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Marine);
	numUnits["Medics"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Medic);
	numUnits["Firebats"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Firebat);
	numUnits["Wraith"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Wraith);
	numUnits["Vultures"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Vulture);
	numUnits["Goliath"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Goliath);
	numUnits["Tanks"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode) + UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode);
	numUnits["Bay"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Engineering_Bay);
	numUnits["Barracks"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Barracks);
	numUnits["Factorys"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Factory);
	numUnits["Academy"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Academy);
	numUnits["Science_Facility"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Science_Facility);
	numUnits["Dropships"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Dropship);
	numUnits["Armorys"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Armory);
	numUnits["Battles"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Battlecruiser);
	numUnits["Starports"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Starport);
	numUnits["Science_Vessel"] = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Science_Vessel);

	changeMainStrategy(numUnits);
	
	if (_main_strategy == Strategy::main_strategies::BSB || _main_strategy == Strategy::main_strategies::BBS)
	{
		int goal_num_marines = numUnits["Marines"] + 2;
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Marine, goal_num_marines));

	}
	else if (_main_strategy == Strategy::main_strategies::Bionic || _main_strategy == Strategy::main_strategies::Bionic_Tank)
	{
		int goal_num_marines = numUnits["Marines"] + numUnits["Barracks"];
		int goal_num_tanks = numUnits["Tanks"];
		
		if (_main_strategy == Strategy::main_strategies::Bionic_Tank) {
			goal_num_tanks += 1;
			if (!hasTech(BWAPI::TechTypes::Tank_Siege_Mode)) {
				goal.push_back(std::pair<MetaType, int>(BWAPI::TechTypes::Tank_Siege_Mode, 1));
			}
		}

		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Marine, goal_num_marines));
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Medic, int(numUnits["Marines"]/4)));
		//goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Firebat, unit_ratio_table["Medics"][goal_num_marines]));
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, goal_num_tanks));

		if (numUnits["Barracks"] > 3)
		{
			goal.push_back(std::pair<MetaType, int>(BWAPI::UpgradeTypes::Terran_Infantry_Weapons, BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Terran_Infantry_Weapons) + 1));
			goal.push_back(std::pair<MetaType, int>(BWAPI::UpgradeTypes::Terran_Infantry_Armor, BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Terran_Infantry_Armor) + 1));

			if (numUnits["Science_Facility"] == 0) {
				goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Science_Facility, 1));
			}
		}

		if (!hasTech(BWAPI::TechTypes::Stim_Packs)) {
			goal.push_back(std::pair<MetaType, int>(BWAPI::TechTypes::Stim_Packs, 1));
		}
		if (hasTech(BWAPI::TechTypes::Stim_Packs)) {
			goal.push_back(std::pair<MetaType, int>(BWAPI::UpgradeTypes::U_238_Shells, 1));
		}
	}
	else if (_main_strategy == Strategy::main_strategies::One_Fac) {
		int goal_num_marines = numUnits["Marines"];
		int goal_num_vultures = numUnits["Vultures"];
		int goal_num_tanks = numUnits["Tanks"];

		if (InformationManager::Instance().enemyRace == BWAPI::Races::Zerg) {
			if (BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Terran_Factory) == 0) {
				goal_num_marines += 2;
			}
		}
		else if (InformationManager::Instance().enemyRace == BWAPI::Races::Terran) {
			if (numUnits["Marines"] < 1) {
				goal_num_marines += 1;
			}
		}
		
		if (InformationManager::Instance().enemyRace == BWAPI::Races::Zerg || InformationManager::Instance().enemyRace == BWAPI::Races::Terran) {
			if (!hasTech(BWAPI::TechTypes::Spider_Mines)) {
				goal.push_back(std::pair<MetaType, int>(BWAPI::TechTypes::Spider_Mines, 1));
			}
		}
		else {
			if (numUnits["Tanks"] > 0 && !hasTech(BWAPI::TechTypes::Tank_Siege_Mode)) {
				goal.push_back(std::pair<MetaType, int>(BWAPI::TechTypes::Tank_Siege_Mode, 1));
			}
		}
		

		if (BWAPI::Broodwar->self()->gas() > 90) {
			goal_num_tanks += 1;
		}
		else {
			goal_num_vultures += 1;
		}

		if (InformationManager::Instance().getEnemyUnitRatio(BWAPI::UnitTypes::Protoss_Zealot) > 0.9){
			goal_num_vultures +=1 ;
		}
		else if (InformationManager::Instance().getEnemyUnitRatio(BWAPI::UnitTypes::Protoss_Zealot) > 1) {
			goal_num_vultures += 2;
		}
		
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Marine, goal_num_marines));
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Vulture, goal_num_vultures));
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, goal_num_tanks));
		
	}
	else if (_main_strategy == Strategy::main_strategies::Two_Fac)
	{
		int goal_num_vultures = numUnits["Vultures"];
		int goal_num_tanks = numUnits["Tanks"];
		
		/*
		if (numUnits["Factorys"] > 1 && numUnits["Armorys"] == 0 )
		{
			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Armory, 1));
		}

		if (numUnits["Factorys"] > 1 && numUnits["Academy"] == 0)
		{
			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Academy, 1));
		}
		*/
		if (numUnits["Vultures"] > numUnits["Tanks"] && BWAPI::Broodwar->self()->gas() > 90) {
			goal_num_tanks += 1;
			goal_num_vultures += 1;
		}
		else {
			goal_num_vultures += 2;
		}

		if (InformationManager::Instance().getEnemyUnitRatio(BWAPI::UnitTypes::Protoss_Zealot) > 0.9){
			goal_num_vultures += 1;
		}
		else if (InformationManager::Instance().getEnemyUnitRatio(BWAPI::UnitTypes::Protoss_Zealot) > 1) {
			goal_num_vultures += 2;
		}
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Vulture, goal_num_vultures));
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, goal_num_tanks));

		if (!hasTech(BWAPI::TechTypes::Spider_Mines)) {
			goal.push_back(std::pair<MetaType, int>(BWAPI::TechTypes::Spider_Mines, 1));
		}
		if (!hasTech(BWAPI::TechTypes::Tank_Siege_Mode)) {
			goal.push_back(std::pair<MetaType, int>(BWAPI::TechTypes::Tank_Siege_Mode, 1));
		}
	}
	else if (_main_strategy == Strategy::main_strategies::Mechanic || _main_strategy == Strategy::main_strategies::Mechanic_Goliath)
	{
		int goal_num_tanks = numUnits["Tanks"];
		int goal_num_vultures = numUnits["Vultures"];
		int goal_num_goliath = numUnits["Goliath"];

		/*
		if (numUnits["Factorys"] > 1 && numUnits["Academy"] == 0)
		{
			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Academy, 1));
		}

		if (numUnits["Factorys"] > 1 && numUnits["Armorys"] == 0)
		{
			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Armory, 1));
		}
		*/
		if (numUnits["Factorys"] > 3)
		{
			goal.push_back(std::pair<MetaType, int>(BWAPI::UpgradeTypes::Terran_Vehicle_Weapons, BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Terran_Vehicle_Weapons) + 1));
			goal.push_back(std::pair<MetaType, int>(BWAPI::UpgradeTypes::Terran_Vehicle_Plating, BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Terran_Vehicle_Plating) + 1));
			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Dropship, InformationManager::Instance().bombDropNum));

		}

		if (numUnits["Factorys"] > 5 && numUnits["Science_Facility"] == 0) {
			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Science_Facility, 1));
		}

		if (_main_strategy == Strategy::main_strategies::Mechanic) {
			if (BWAPI::Broodwar->self()->gas() < 500) {
				goal_num_vultures += numUnits["Factorys"];
			}
			else {
				if (goal_num_tanks > goal_num_vultures){
					goal_num_vultures += (goal_num_tanks - goal_num_vultures > numUnits["Factorys"] ? numUnits["Factorys"] : goal_num_tanks - goal_num_vultures);
					goal_num_tanks += ((numUnits["Factorys"] - (goal_num_tanks - goal_num_vultures)) > 0 ? (numUnits["Factorys"] - (goal_num_tanks - goal_num_vultures)) : 0);
				}
				else {
					goal_num_tanks += (goal_num_vultures - goal_num_tanks > numUnits["Factorys"] ? numUnits["Factorys"] : goal_num_vultures - goal_num_tanks);
					goal_num_vultures += ((numUnits["Factorys"] - (goal_num_vultures - goal_num_tanks)) > 0 ? (numUnits["Factorys"] - (goal_num_vultures - goal_num_tanks)) : 0);
				}

			}
		}
		else {
			if (BWAPI::Broodwar->self()->gas() < 500) {
				goal_num_vultures += numUnits["Factorys"];
			}
			else{
				if (BWAPI::Broodwar->enemy()->allUnitCount(BWAPI::UnitTypes::Zerg_Mutalisk) > 0 || BWAPI::Broodwar->enemy()->allUnitCount(BWAPI::UnitTypes::Protoss_Carrier) > 0) {
					goal_num_tanks += 1;
					goal_num_goliath += numUnits["Factorys"];
				}
				else {
					if (goal_num_tanks > goal_num_goliath){
						goal_num_goliath += (goal_num_tanks - goal_num_goliath > numUnits["Factorys"] ? numUnits["Factorys"] : goal_num_tanks - goal_num_goliath);
						goal_num_tanks += ((numUnits["Factorys"] - (goal_num_tanks - goal_num_goliath)) > 0 ? (numUnits["Factorys"] - (goal_num_tanks - goal_num_goliath)) : 0);
					}
					else {
						goal_num_tanks += (goal_num_goliath - goal_num_tanks > numUnits["Factorys"] ? numUnits["Factorys"] : goal_num_goliath - goal_num_tanks);
						goal_num_goliath += ((numUnits["Factorys"] - (goal_num_goliath - goal_num_tanks)) > 0 ? (numUnits["Factorys"] - (goal_num_goliath - goal_num_tanks)) : 0);
					}
				}

				goal_num_vultures += 1;
			}
		}
		goal_num_vultures += 1;

		/* 순서변경
		if (!hasTech(BWAPI::TechTypes::Tank_Siege_Mode)) {
			goal.push_back(std::pair<MetaType, int>(BWAPI::TechTypes::Tank_Siege_Mode, 1));
		}
		if (hasTech(BWAPI::TechTypes::Tank_Siege_Mode) && !hasTech(BWAPI::TechTypes::Spider_Mines)) {
			goal.push_back(std::pair<MetaType, int>(BWAPI::TechTypes::Spider_Mines, 1));
		}
		*/
		if (!hasTech(BWAPI::TechTypes::Spider_Mines)) {
			goal.push_back(std::pair<MetaType, int>(BWAPI::TechTypes::Spider_Mines, 1));
		}
		if (hasTech(BWAPI::TechTypes::Spider_Mines) && !hasTech(BWAPI::TechTypes::Tank_Siege_Mode)) {
			goal.push_back(std::pair<MetaType, int>(BWAPI::TechTypes::Tank_Siege_Mode, 1));
		}
		if (hasTech(BWAPI::TechTypes::Spider_Mines) && BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Ion_Thrusters) == 0) {
			goal.push_back(std::pair<MetaType, int>(BWAPI::UpgradeTypes::Ion_Thrusters, 1));
		}
		if (_main_strategy == Strategy::main_strategies::Mechanic_Goliath && BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Charon_Boosters) == 0) {
			goal.push_back(std::pair<MetaType, int>(BWAPI::UpgradeTypes::Charon_Boosters, 1));
		}

		if (InformationManager::Instance().getEnemyUnitRatio(BWAPI::UnitTypes::Protoss_Zealot) > 0.9){
			goal_num_vultures += 1;
		}
		else if (InformationManager::Instance().getEnemyUnitRatio(BWAPI::UnitTypes::Protoss_Zealot) > 1) {
			goal_num_vultures += 2;
		}

		//int goal_num_vultures = (goal_num_tanks - numUnits["Vultures"]) > numUnits["Factorys"] * 2 ? numUnits["Vultures"]+numUnits["Factorys"] : goal_num_tanks;
		//벌쳐는 탱크수만큼 뽑되 현재 탱크수와 벌쳐수가 많이 차이나면(팩토리의 2배만큼) 현재탱크수의 절반만 뽑는다.
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Vulture, goal_num_vultures));
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Goliath, goal_num_goliath));
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, goal_num_tanks));

		// 2018.08.16
		// 황봉규
		// 저그전일때 bionic_tank에서 Mechanic 으로 바뀌는 동안병력생산 공백을 줄이기 위한로직
		if (InformationManager::Instance().isTimingToLiftBaracks == false && InformationManager::Instance().enemyRace == BWAPI::Races::Zerg && numUnits["Factorys"] < 5) {
			int goal_num_marines = numUnits["Marines"] + numUnits["Barracks"];
			int goal_num_tanks = numUnits["Tanks"];

			goal_num_tanks += 1;

			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Marine, goal_num_marines));
			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Medic, int(numUnits["Marines"] / 4)));
			//goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Firebat, unit_ratio_table["Medics"][goal_num_marines]));
			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode, goal_num_tanks));
		}
		else {
			// 배럭드는 타이밍 설정과 배럭들 유닛셋에 insert
			if (InformationManager::Instance().isTimingToLiftBaracks == false) {
				InformationManager::Instance().isTimingToLiftBaracks = true;

				for (auto unit : BWAPI::Broodwar->self()->getUnits()) {
					if (unit == nullptr) {
						continue;
					}
					if (unit->getType() == BWAPI::UnitTypes::Terran_Barracks && unit->isCompleted() == true && unit->isLifted() == false && unit->canLift() == true) {
						unit->lift();
						InformationManager::Instance().buildingDetectors.insert(unit);
					}
				}
			}
		}
		

	}
	else if (_main_strategy == Strategy::main_strategies::BATTLE) {
		goal.push_back(std::pair<MetaType, int>(BWAPI::UpgradeTypes::Terran_Ship_Weapons, BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Terran_Ship_Weapons) + 1));
		goal.push_back(std::pair<MetaType, int>(BWAPI::UpgradeTypes::Terran_Ship_Plating, BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Terran_Ship_Plating) + 1));

		if (numUnits["Science_Facility"] >= 1) {
			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Science_Vessel, 1));
		}


		int goal_num_battles = numUnits["Battles"] + 6;
		// 황봉규
		// 기존에 배틀크루저 생산하는 로직이 없어서
		// 인구수 198 넘어간 후 전략이 BATTLE로 바뀌면
		// 아무런 전투유닛 생산 안하는 문제 발생하여 추가
		goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Battlecruiser, goal_num_battles));

	}
	else
	{
		//BWAPI::Broodwar->printf("Warning: No build order goal for Terran Strategy: %s", Config::Strategy::StrategyName.c_str());
	}

	if (_main_strategy == Strategy::main_strategies::One_Fac || _main_strategy == Strategy::main_strategies::Mechanic || _main_strategy == Strategy::main_strategies::Two_Fac || _main_strategy == Strategy::main_strategies::Mechanic_Goliath) {

		std::pair<int, int> queueResource = BuildManager::Instance().getQueueResource();
		queueResource.first += BuildManager::Instance().marginResource.first; //약간의 마진을 준다. 너무 타이트하게 여유자원을 사용하지 않기 위해서
		queueResource.second += BuildManager::Instance().marginResource.second;

		std::pair<int, int> remainingResource(BuildManager::Instance().getAvailableMinerals() - queueResource.first, BuildManager::Instance().getAvailableGas() - queueResource.second);

		if (remainingResource.first > 0 && remainingResource.second > 0) {
			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Factory, numUnits["Factorys"] + 1));
		}
	}

	if (_main_strategy == Strategy::main_strategies::BATTLE) {

		std::pair<int, int> queueResource = BuildManager::Instance().getQueueResource();
		queueResource.first += BuildManager::Instance().marginResource.first; //약간의 마진을 준다. 너무 타이트하게 여유자원을 사용하지 않기 위해서
		queueResource.second += BuildManager::Instance().marginResource.second;

		std::pair<int, int> remainingResource(BuildManager::Instance().getAvailableMinerals() - queueResource.first, BuildManager::Instance().getAvailableGas() - queueResource.second);

		if (remainingResource.first > 0 && remainingResource.second > 0) {
			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Factory, numUnits["Starports"] + 1));
		}
	}

	// 사이언스베슬 생산을 현재 전략에 따라서가 아닌 상대 종족에 따라 생산하고 기술개발 하도록 변경
	if (numUnits["Science_Facility"] >= 1) {
		if (InformationManager::Instance().enemyRace == BWAPI::Races::Zerg) {
			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Science_Vessel, numUnits["Science_Vessel"] + 1));
			goal.push_back(std::pair<MetaType, int>(BWAPI::TechTypes::Irradiate, 1));
		}
		else if (InformationManager::Instance().enemyRace == BWAPI::Races::Protoss) {
			goal.push_back(std::pair<MetaType, int>(BWAPI::UnitTypes::Terran_Science_Vessel, 1));
			goal.push_back(std::pair<MetaType, int>(BWAPI::TechTypes::EMP_Shockwave, 1));
		}

	}



	return goal;
}


//각 건물에 대한 설치전략
BuildOrderItem::SeedPositionStrategy StrategyManager::getBuildSeedPositionStrategy(MetaType type){
	BuildOrderItem::SeedPositionStrategy rst = BuildOrderItem::SeedPositionStrategy::MainBaseLocation;

	if (type.getUnitType() == BWAPI::UnitTypes::Terran_Bunker) {
		if (InformationManager::Instance().nowCombatStatus == InformationManager::combatStatus::DEFCON3) {
			return BuildOrderItem::SeedPositionStrategy::DefenceChokePoint;
		}
		else if (InformationManager::Instance().nowCombatStatus == InformationManager::combatStatus::DEFCON4) {
			return BuildOrderItem::SeedPositionStrategy::SecondChokePoint;
		}
	}

	if (InformationManager::Instance().enemyRace == BWAPI::Races::Protoss || InformationManager::Instance().enemyRace == BWAPI::Races::Terran){

		// if building is destroyed?
		if (type.getUnitType() == BWAPI::UnitTypes::Terran_Supply_Depot) {

			if (InformationManager::Instance().getSupPostionsForWall().size() > 0) {
				return BuildOrderItem::SeedPositionStrategy::SupForWall;
			}
		}
		else if (type.getUnitType() == BWAPI::UnitTypes::Terran_Barracks) {

			if (InformationManager::Instance().getBarPostionsForWall() != BWAPI::TilePositions::None && UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Supply_Depot) == 0) {
				return BuildOrderItem::SeedPositionStrategy::BarForWall;
			}
		}
	}

	if (type.getUnitType() == BWAPI::UnitTypes::Terran_Supply_Depot) {
		if (InformationManager::Instance().getReservedSupPositions().size() > 0) {
			return BuildOrderItem::SeedPositionStrategy::ReservedSupPosion;
		}
	}
	//TODO : 큐 단위 이므로 큐에 있는것까지 고려는 잘 안됨.
	//서플라이 2개까지만 본진커맨드 주변, 이후 본진과 쵸크포인트 둘다 먼곳
	//if (InformationManager::Instance().getMapName() != 'H'){
	//if (type.getUnitType() == BWAPI::UnitTypes::Terran_Supply_Depot){
	//	if (UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Supply_Depot) > 3){
	//		//std::cout << "build supply depot on MainBaseOppositeChock" << std::endl;
	//		rst = BuildOrderItem::SeedPositionStrategy::MainBaseOppositeChock;
	//	}
	//}
	//}

	return buildSeedPositionStrategy;
	/*
	//본진 포화되면 다른데가서 짓도록
	if (!ExpansionManager::Instance().getExpansions().empty() && ExpansionManager::Instance().getExpansions()[0].complexity > 0.25){
		//std::cout << "build " << type.getName() << " on LowComplexityExpansionLocation" << std::endl;
		rst = BuildOrderItem::SeedPositionStrategy::LowComplexityExpansionLocation;
	}

	return rst;
	*/
}

void StrategyManager::setBuildSeedPositionStrategy(BuildOrderItem::SeedPositionStrategy seedPositionStrategy){
	if (buildSeedPositionStrategy < seedPositionStrategy && buildSeedPositionStrategy != BuildOrderItem::SeedPositionStrategy::SupForWall && buildSeedPositionStrategy != BuildOrderItem::SeedPositionStrategy::BarForWall) {
		buildSeedPositionStrategy = seedPositionStrategy;
	}
}

int StrategyManager::getUnitLimit(MetaType type){
	if (type.getUnitType() == BWAPI::UnitTypes::Terran_Barracks) {
		if (_main_strategy == Strategy::main_strategies::BSB || _main_strategy == Strategy::main_strategies::BBS) {
			return 1;
		}
		else if (_main_strategy == Strategy::main_strategies::Bionic) {
			return 3;
		}
		else if (_main_strategy == Strategy::main_strategies::Bionic_Tank) {
			return 5;
		}
		else if (_main_strategy == Strategy::main_strategies::One_Fac || _main_strategy == Strategy::main_strategies::Two_Fac) {
			return 0;
		}
	}
	if (type.getUnitType() == BWAPI::UnitTypes::Terran_Factory){
		if (_main_strategy == Strategy::main_strategies::One_Fac) {
			return 1;
		} else if (_main_strategy == Strategy::main_strategies::Two_Fac) {
			return 3;
		}
		else if (_main_strategy == Strategy::main_strategies::Bionic_Tank) {
			return 1;
		}
		return 9;
	}
	if (type.getUnitType() == BWAPI::UnitTypes::Terran_Armory) {
		return 1;
	}
	if (type.getUnitType() == BWAPI::UnitTypes::Terran_Engineering_Bay) {
		return 1;
	}
	if (type.getUnitType() == BWAPI::UnitTypes::Terran_Science_Facility) {
		return 0;
	}

	if (type.getUnitType() == BWAPI::UnitTypes::Terran_Missile_Turret) {
		return ExpansionManager::Instance().getExpansions().size() * 3;
	}

	if (type.getUnitType() == BWAPI::UnitTypes::Terran_Starport){
		if (_main_strategy == Strategy::main_strategies::BATTLE) {
			return 7;
		}
		else {
			return 1;
		}
		
	}

	return -1;
}

void StrategyManager::initStrategies(){
	/*
	@도주남 김유진 전략구조체 세팅
	1. 전략이름정의 : enum
	2. 이전/다음 전략 세팅
	3. 초기빌드 세팅
	4. 이전/다음 전략 넘어가는 조건 세팅
	*/

	_strategies[Strategy::main_strategies::BSB] = Strategy();
	_strategies[Strategy::main_strategies::BSB].pre_strategy = Strategy::main_strategies::None;
	_strategies[Strategy::main_strategies::BSB].next_strategy = Strategy::main_strategies::Bionic;
	_strategies[Strategy::main_strategies::BSB].opening_build_order = "SCV SCV SCV SCV SCV Barracks SCV Supply_Depot Barracks";
	_strategies[Strategy::main_strategies::BSB].num_unit_limit["Marines"] = 6; //마린18이상이면 테크진화

	_strategies[Strategy::main_strategies::BBS] = Strategy();
	_strategies[Strategy::main_strategies::BBS].pre_strategy = Strategy::main_strategies::None;
	_strategies[Strategy::main_strategies::BBS].next_strategy = Strategy::main_strategies::One_Fac;
	_strategies[Strategy::main_strategies::BBS].opening_build_order = "SCV SCV SCV SCV SCV Barracks SCV Barracks Supply_Depot";
	_strategies[Strategy::main_strategies::BBS].num_unit_limit["Marines"] = 6; //마린18이상이면 테크진화

	_strategies[Strategy::main_strategies::Bionic] = Strategy();
	_strategies[Strategy::main_strategies::Bionic].pre_strategy = Strategy::main_strategies::None;
	_strategies[Strategy::main_strategies::Bionic].next_strategy = Strategy::main_strategies::Bionic_Tank;
	_strategies[Strategy::main_strategies::Bionic].opening_build_order = "SCV SCV SCV SCV SCV Supply_Depot SCV Barracks SCV Barracks SCV SCV SCV Marine Supply_Depot SCV Marine Refinery SCV Marine SCV Marine SCV Marine Supply_Depot SCV Academy";
	_strategies[Strategy::main_strategies::Bionic].num_unit_limit["Marines"] = 16; //마린18이상이면 테크진화
	_strategies[Strategy::main_strategies::Bionic].num_unit_limit["Barracks"] = 4; //-1은 테크진화에 영향없음
	_strategies[Strategy::main_strategies::Bionic].num_unit_limit["Firebats"] = -1;

	_strategies[Strategy::main_strategies::Bionic_Tank] = Strategy();
	_strategies[Strategy::main_strategies::Bionic_Tank].pre_strategy = Strategy::main_strategies::None;
	_strategies[Strategy::main_strategies::Bionic_Tank].next_strategy = Strategy::main_strategies::Mechanic;
	_strategies[Strategy::main_strategies::Bionic_Tank].opening_build_order = "SCV SCV SCV SCV SCV Supply_Depot SCV Barracks SCV Barracks SCV SCV SCV Marine Supply_Depot SCV Marine Refinery SCV Marine SCV Marine SCV Marine Supply_Depot SCV Academy";
	_strategies[Strategy::main_strategies::Bionic_Tank].num_unit_limit["Tanks"] = 6;

	_strategies[Strategy::main_strategies::One_Fac] = Strategy();
	_strategies[Strategy::main_strategies::One_Fac].pre_strategy = Strategy::main_strategies::None;
	_strategies[Strategy::main_strategies::One_Fac].next_strategy = Strategy::main_strategies::Two_Fac;
	_strategies[Strategy::main_strategies::One_Fac].opening_build_order = "SCV SCV SCV SCV SCV Supply_Depot SCV SCV Barracks Refinery SCV SCV Factory Supply_Depot";
	//_strategies[Strategy::main_strategies::One_Fac].opening_build_order = "SCV SCV SCV SCV SCV Barracks SCV Refinery Supply_Depot SCV Marine Factory SCV Marine Supply_Depot";
	_strategies[Strategy::main_strategies::One_Fac].num_unit_limit["Tanks"] = 2;

	_strategies[Strategy::main_strategies::Two_Fac] = Strategy();
	_strategies[Strategy::main_strategies::Two_Fac].pre_strategy = Strategy::main_strategies::None;
	_strategies[Strategy::main_strategies::Two_Fac].next_strategy = Strategy::main_strategies::Mechanic;
	_strategies[Strategy::main_strategies::Two_Fac].num_unit_limit["Tanks"] = 4;

	_strategies[Strategy::main_strategies::Mechanic] = Strategy();
	_strategies[Strategy::main_strategies::Mechanic].pre_strategy = Strategy::main_strategies::None;
	_strategies[Strategy::main_strategies::Mechanic].next_strategy = Strategy::main_strategies::None;
	_strategies[Strategy::main_strategies::Mechanic].opening_build_order = "SCV SCV SCV SCV SCV Supply_Depot SCV SCV Barracks Refinery SCV SCV SCV Marine Supply_Depot Factory SCV Marine SCV Marine Machine_Shop Marine SCV Marine SCV Supply_Depot Siege_Tank_Tank_Mode Tank_Siege_Mode Marine SCV Marine SCV Supply_Depot Siege_Tank_Tank_Mode SCV Engineering_Bay Siege_Tank_Tank_Mode Command_Center Siege_Tank_Tank_Mode Supply_Depot Factory SCV SCV Siege_Tank_Tank_Mode";

	_strategies[Strategy::main_strategies::Mechanic_Goliath] = Strategy();
	_strategies[Strategy::main_strategies::Mechanic_Goliath].pre_strategy = Strategy::main_strategies::None;
	_strategies[Strategy::main_strategies::Mechanic_Goliath].next_strategy = Strategy::main_strategies::None;

	_strategies[Strategy::main_strategies::BATTLE] = Strategy();
	_strategies[Strategy::main_strategies::BATTLE].pre_strategy = Strategy::main_strategies::None;
	_strategies[Strategy::main_strategies::BATTLE].next_strategy = Strategy::main_strategies::None;
}
void StrategyManager::initUnitRatioTable(){
	//@도주남 김유진 유닛비율 테이블
	//std::cout << "Medic ratio" << std::endl;
	unit_ratio_table["Medics"].push_back(0);
	unit_ratio_table["Medics"].push_back(0); //index 1 부터 시작
	for (int i = 1; i < 201; i++){
		if (i % 10 == 0)
			//std::cout << unit_ratio_table["Medics"][i - 1] << " ";

		unit_ratio_table["Medics"].push_back(int(log(i) / log(1.5)));
	}
	//std::cout << std::endl;
}

bool StrategyManager::hasTech(BWAPI::TechType tech){
	BWAPI::Player self = BWAPI::Broodwar->self();
	return self->hasResearched(tech);
}

bool StrategyManager::obtainNextUpgrade(BWAPI::UpgradeType upgType)
{
	BWAPI::Player self = BWAPI::Broodwar->self();
	int maxLvl = self->getMaxUpgradeLevel(upgType);
	int currentLvl = self->getUpgradeLevel(upgType);
	if (!self->isUpgrading(upgType) && currentLvl < maxLvl &&
		self->completedUnitCount(upgType.whatsRequired(currentLvl + 1)) > 0 &&
		self->completedUnitCount(upgType.whatUpgrades()) > 0) {

	}
	//return self->getUnits(). upgrade(upgType);
	return false;
}

double StrategyManager::weightByFrame(double max_weight, int early){
	return BWAPI::Broodwar->getFrameCount() > early ? 1.0 : ((1 - max_weight) * BWAPI::Broodwar->getFrameCount() / early) + max_weight;
}

Strategy::main_strategies StrategyManager::getMainStrategy() {
	return _main_strategy;
}

BWAPI::Position StrategyManager::getPositionForDefenceChokePoint(BWTA::Chokepoint * chokepoint, BWAPI::UnitType unit)
{
	BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
	std::vector<BWAPI::TilePosition> tpList = BWTA::getShortestPath(BWAPI::TilePosition(ourBasePosition), BWAPI::TilePosition(chokepoint->getCenter()));
	BWAPI::Position resultPosition = ourBasePosition;
	for (auto & t : tpList) {
		BWAPI::Position tp(t.x * 32, t.y * 32);
		if (!tp.isValid())
			continue;
		
		if (tp.getDistance(chokepoint->getCenter()) <= unit.groundWeapon().maxRange() + 64
			&& tp.getDistance(chokepoint->getCenter()) >= unit.groundWeapon().maxRange() + 30)
		{
			resultPosition = tp;
		}
	}
	return resultPosition;
}

void StrategyManager::liftBarrackFromWall() {

	for (auto wallUnit : InformationManager::Instance().getWallUnits()) {

		if (wallUnit == nullptr) {
			continue;
		}
		/* 드랍이 와서 본진으로 적유닛이 들어오는 경우에 
		   배럭이 닫히는 것을 막기 위한 로직 추가
		*/
		if (wallUnit->getType() == BWAPI::UnitTypes::Terran_Barracks && wallUnit->isCompleted() == true) {
			int maxDist = MapTools::Instance().getGroundDistance(wallUnit->getPosition(), InformationManager::Instance().getSecondChokePoint(BWAPI::Broodwar->self())->getCenter());
			int wallDistFromMainBase = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition().getDistance(wallUnit->getPosition());
			maxDist = maxDist < 0 ? 1000000 : maxDist;
			wallDistFromMainBase = wallDistFromMainBase < 0 ? 1000000 : wallDistFromMainBase;

			for (auto enemyUnit : BWAPI::Broodwar->enemy()->getUnits()) {

				if (enemyUnit == nullptr) {
					continue;
				}

				// 일꾼이 올때도 적용하게 변경
				//if (enemyUnit->isCompleted() && enemyUnit->getType().isFlyer() == false && enemyUnit->getType().isWorker() == false){
				if (enemyUnit->isCompleted() && enemyUnit->getType().isFlyer() == false){
					int dist = MapTools::Instance().getGroundDistance(wallUnit->getPosition(), enemyUnit->getPosition());
					dist = dist < 0 ? 1000000 : dist;

					int enemyDistFromMainBase = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition().getDistance(enemyUnit->getPosition());
					enemyDistFromMainBase = enemyDistFromMainBase < 0 ? 1000000 : enemyDistFromMainBase;

					if (dist < maxDist && enemyDistFromMainBase > wallDistFromMainBase && wallUnit->isLifted() == true && wallUnit->canLand() == true) {
						wallUnit->land(InformationManager::Instance().getWallBarackPosition());
					}
					return;
				}
			}

			if (wallUnit->isLifted() == false && wallUnit->canLift() == true) {
				wallUnit->lift();
				return;
			}
		}
	}
	
}
void StrategyManager::liftAndMoveBarrackFromWall() {
	if (InformationManager::Instance().nowCombatStatus == InformationManager::combatStatus::DEFCON4 || InformationManager::Instance().nowCombatStatus > InformationManager::combatStatus::MainAttack) {
		for (auto & unit : BWAPI::Broodwar->self()->getUnits()) {
			BWAPI::TilePosition position = unit->getTilePosition();
			if ((unit->getType() == BWAPI::UnitTypes::Terran_Barracks) && _main_strategy != Strategy::BBS && _main_strategy != Strategy::BSB && _main_strategy != Strategy::Bionic && _main_strategy != Strategy::Bionic_Tank && _main_strategy != Strategy::One_Fac) {
				if (unit->canLift()) {
					unit->lift();
				}
			}				
		}
	}
	else if (InformationManager::Instance().nowCombatStatus == InformationManager::combatStatus::DEFCON5 || InformationManager::Instance().nowCombatStatus > InformationManager::combatStatus::MainAttack) {
		for (auto & unit : BWAPI::Broodwar->self()->getUnits()) {
			BWAPI::TilePosition position = unit->getTilePosition();
			if (unit->getType() == BWAPI::UnitTypes::Terran_Barracks && unit->isLifted()) {
				CommandUtil::move(unit, InformationManager::Instance().getSecondChokePoint(BWAPI::Broodwar->self())->getCenter());
			}
		}
	}
}

BWAPI::UnitType StrategyManager::getUnitTypeFromMainStrategy(){
	if (_main_strategy == Strategy::BBS || _main_strategy == Strategy::BSB || _main_strategy == Strategy::Bionic || _main_strategy == Strategy::Bionic_Tank) {
		return BWAPI::UnitTypes::Terran_Marine;
	}
	else if (_main_strategy == Strategy::Mechanic) {
		return BWAPI::UnitTypes::Terran_Vulture;
	}
	else if (_main_strategy == Strategy::Mechanic_Goliath) {
		return BWAPI::UnitTypes::Terran_Goliath;
	}
	else if (_main_strategy == Strategy::BATTLE) {
		return BWAPI::UnitTypes::Terran_Battlecruiser;
	}
	else {
		BWAPI::UnitTypes::None;
	}
}