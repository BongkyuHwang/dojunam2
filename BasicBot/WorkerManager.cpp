#include "WorkerManager.h"

using namespace MyBot;

// kyj
void WorkerManager::finishedWithWorker(BWAPI::Unit unit)
{
	UAB_ASSERT(unit != nullptr, "Unit was null");

	//BWAPI::Broodwar->printf("BuildingManager finished with worker %d", unit->getID());
	if (workerData.getWorkerJob(unit) != WorkerData::Scout)
	{
		workerData.setWorkerJob(unit, WorkerData::Idle, nullptr);
	}
}

/////////////////////////////////////////////////////////////////////
///////////////////////////
WorkerManager::WorkerManager() 
{
	currentBuildingRepairWorker = nullptr;
	currentMechaRepairWorker = nullptr;
	_freeHP = 45;
}

WorkerManager & WorkerManager::Instance() 
{
	static WorkerManager instance;
	return instance;
}

void WorkerManager::update()
{
	if (TimeManager::Instance().isMyTurn("WorkerManager_update", 7))
	{
		int start = clock();
		// 아군 베이스 위치. 적군 베이스 위치. 각 유닛들의 상태정보 등을 Map 자료구조에 저장/업데이트
		int limit = 1000;
		updateWorkerStatus();
		int duration = (clock() - start) / double(CLOCKS_PER_SEC) * 1000;
		if (duration > limit) {
			std::cout << "updateWorkerStatus time: " << duration << std::endl;
		}
		start = clock();
		handleGasWorkers();
		duration = (clock() - start) / double(CLOCKS_PER_SEC) * 1000;
		if (duration > limit) {
			std::cout << "handleGasWorkers time: " << duration << std::endl;
		}

		start = clock();
		handleIdleWorkers();
		duration = (clock() - start) / double(CLOCKS_PER_SEC) * 1000;
		if (duration > limit) {
			std::cout << "handleIdleWorkers time: " << duration << std::endl;
		}

		start = clock();
		handleMoveWorkers();
		duration = (clock() - start) / double(CLOCKS_PER_SEC) * 1000;
		if (duration > limit) {
			std::cout << "handleMoveWorkers time: " << duration << std::endl;
		}

		start = clock();
		handleScoutCombatWorker();
		duration = (clock() - start) / double(CLOCKS_PER_SEC) * 1000;
		if (duration > limit) {
			std::cout << "handleScoutCombatWorker time: " << duration << std::endl;
		}

		start = clock();
		handleRepairWorkers();
		duration = (clock() - start) / double(CLOCKS_PER_SEC) * 1000;
		if (duration > limit) {
			std::cout << "handleRepairWorkers time: " << duration << std::endl;
		}
		
		start = clock();
		handleCombatWorkers();
		duration = (clock() - start) / double(CLOCKS_PER_SEC) * 1000;
		if (duration > limit) {
			std::cout << "handleCombatWorkers time: " << duration << std::endl;
		}
		
		start = clock();
		handleBunkderRepairWorkers();
		duration = (clock() - start) / double(CLOCKS_PER_SEC) * 1000;
		if (duration > limit) {
			std::cout << "handleBunkderRepairWorkers time: " << duration << std::endl;
		}
	}	
}
void WorkerManager::updateWorkerStatus() 
{
	// Drone 은 건설을 위해 isConstructing = true 상태로 건설장소까지 이동한 후, 
	// 잠깐 getBuildType() == none 가 되었다가, isConstructing = true, isMorphing = true 가 된 후, 건설을 시작한다

	// for each of our Workers
	for (auto & worker : workerData.getWorkers())
	{
		if (!worker) continue;

		if (!worker->isCompleted())
		{
			continue;
		}

		// 게임상에서 worker가 isIdle 상태가 되었으면 (새로 탄생했거나, 그전 임무가 끝난 경우), WorkerData 도 Idle 로 맞춘 후, handleGasWorkers, handleIdleWorkers 등에서 새 임무를 지정한다 
		if ( worker->isIdle() )
		{
			/*
			if ((workerData.getWorkerJob(worker) == WorkerData::Build)
				|| (workerData.getWorkerJob(worker) == WorkerData::Move)
				|| (workerData.getWorkerJob(worker) == WorkerData::Scout)) {

				//std::cout << "idle worker " << worker->getID()C
					<< " job: " << workerData.getWorkerJob(worker)
					<< " exists " << worker->exists()
					<< " isConstructing " << worker->isConstructing()
					<< " isMorphing " << worker->isMorphing()
					<< " isMoving " << worker->isMoving()
					<< " isBeingConstructed " << worker->isBeingConstructed()
					<< " isStuck " << worker->isStuck()
					<< std::endl;
			}
			*/

			// workerData 에서 Build / Move / Scout 로 임무지정한 경우, worker 는 즉 임무 수행 도중 (임무 완료 전) 에 일시적으로 isIdle 상태가 될 수 있다 
			if ((workerData.getWorkerJob(worker) != WorkerData::Build)
				&& (workerData.getWorkerJob(worker) != WorkerData::Move)
				&& (workerData.getWorkerJob(worker) != WorkerData::Scout)
				&& (workerData.getWorkerJob(worker) != WorkerData::BunkerReapir)
				&& (workerData.getWorkerJob(worker) != WorkerData::ScoutCombat)
				)
			{
				workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
			}
		}

		/*
		if (worker->isGatheringGas() && workerData.getWorkerJob(worker) != WorkerData::Gas) {
			workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
		}
		*/

		// if its job is gas
		if (workerData.getWorkerJob(worker) == WorkerData::Gas)
		{
			BWAPI::Unit refinery = workerData.getWorkerResource(worker);

			// if the refinery doesn't exist anymore (파괴되었을 경우)
			if (!refinery || !refinery->exists() ||	refinery->getHitPoints() <= 0)
			{
				workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
			}
		}

		// if its job is repair
		if (workerData.getWorkerJob(worker) == WorkerData::Repair)
		{
			BWAPI::Unit repairTargetUnit = workerData.getWorkerRepairUnit(worker);
						
			// 대상이 파괴되었거나, 수리가 다 끝난 경우
			if (!repairTargetUnit || !repairTargetUnit->exists() || repairTargetUnit->getHitPoints() <= 0 || repairTargetUnit->getHitPoints() == repairTargetUnit->getType().maxHitPoints())
			{
				workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
			}
		}
	}
}

void WorkerManager::handleGasWorkers()
{ 
	// for each unit we have
	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		// refinery 가 건설 completed 되었으면,
		if (unit->getType().isRefinery() && unit->isCompleted() )
		{
			// get the number of workers currently assigned to it
			int numAssigned = workerData.getNumAssignedWorkers(unit);

			int targetNumGasWorker = 0;

			if (StrategyManager::Instance().getMainStrategy() == Strategy::One_Fac && BWAPI::Broodwar->self()->gatheredGas() >= 150) {
				targetNumGasWorker = 2;
			}
			else if (StrategyManager::Instance().getMainStrategy() == Strategy::BSB || StrategyManager::Instance().getMainStrategy() == Strategy::BBS) {
				targetNumGasWorker = 0;
			}
			else if (StrategyManager::Instance().getMainStrategy() == Strategy::Bionic) {
				if (BWAPI::Broodwar->self()->gas() >= 300) {
					targetNumGasWorker = 0;
				}
				else if (BWAPI::Broodwar->self()->gas() >= 150 && BWAPI::Broodwar->self()->gas() < 300) {
					targetNumGasWorker = 1;
				}
				else {
					targetNumGasWorker = 3;
				}
				
			}
			else if (StrategyManager::Instance().getMainStrategy() == Strategy::Bionic_Tank) {
				if (BWAPI::Broodwar->self()->gas() >= 300) {
					targetNumGasWorker = 2;
				}
				else {
					targetNumGasWorker = 3;
				}
			}
			else {
				targetNumGasWorker = Config::Macro::WorkersPerRefinery;
			}
			/*
			int numMineralWorkers = getNumMineralWorkers();
			if (numMineralWorkers < 7) {
				targetNumGasWorker = 0;
			}
			else if (numMineralWorkers == 7) {
				targetNumGasWorker = 1;
			}
			else if (numMineralWorkers == 8){
				targetNumGasWorker = 2;
			}
			else {
				targetNumGasWorker = 3;
			}
			*/

			if (numAssigned > targetNumGasWorker) {
				for (int i = 0; i<(numAssigned - targetNumGasWorker); ++i)
				{
					BWAPI::Unit mineralWorker = chooseMineralWorkerFromGasWorkers(unit);

					if (mineralWorker)
					{
						setMineralWorker(mineralWorker);
					}
				}
			}
			else if (numAssigned < targetNumGasWorker) {
				for (int i = 0; i<(targetNumGasWorker - numAssigned); ++i)
				{
					BWAPI::Unit gasWorker = chooseGasWorkerFromMineralWorkers(unit);

					if (gasWorker)
					{
						////std::cout << "set gasWorker " << gasWorker->getID() << std::endl;
						workerData.setWorkerJob(gasWorker, WorkerData::Gas, unit);
					}
				}
			}
		}
	}
}


BWAPI::Unit WorkerManager::chooseMineralWorkerFromGasWorkers(BWAPI::Unit refinery){
	if (!refinery) return nullptr;

	double closestDistance = 0;

	for (auto & unit : workerData.getWorkers())
	{
		if (!unit) continue;
		
		if (unit->isCompleted() && !unit->isCarryingGas() && workerData.getWorkerJob(unit) == WorkerData::Gas)
		{
			return unit;
		}
	}
	return nullptr;
}

void WorkerManager::handleIdleWorkers() 
{
	// for each of our workers
	for (auto & worker : workerData.getWorkers())
	{
		if (!worker) continue;

		// if worker's job is idle 
		if (workerData.getWorkerJob(worker) == WorkerData::Idle || workerData.getWorkerJob(worker) == WorkerData::Default )
		{
			// send it to the nearest mineral patch
			setMineralWorker(worker);
		}
	}
	
}

void WorkerManager::handleMoveWorkers()
{
	// for each of our workers
	for (auto & worker : workerData.getWorkers())
	{
		if (!worker) continue;
		BWAPI::Position targetPosition = worker->getTargetPosition();
		if (targetPosition.x < 80 && targetPosition.y < 80 && worker->getType().isWorker())
		{
			/*printf("WM 280 worker [%d] move to ( %d, %d )  \n", worker->getID(), targetPosition.x, targetPosition.y);
			printf("WM 280 worker [%d] move to ( %d, %d )  \n", worker->getID(), targetPosition.x, targetPosition.y);
			;
			BWAPI::UnitCommand currentCommand(worker->getLastCommand());
			printf("WM 280 worker Command ?  \n");
			printf("[%d]%s\n", worker->getLastCommandFrame(), currentCommand.getType().c_str());
			printf("==================================\n");
			*/
			worker->stop();
		}
		// if it is a move worker
		if (workerData.getWorkerJob(worker) == WorkerData::Move)
		{
			WorkerMoveData data = workerData.getWorkerMoveData(worker);

			// 목적지에 도착한 경우 이동 명령을 해제한다
			//if (worker->getPosition().getDistance(data.position) < 100) {
			// 지상거리
			if (getGroundDistance(worker->getPosition(), data.position) < 100) {
				setIdleWorker(worker);
			}
			else {
				CommandUtil::move(worker, data.position);
			}
		}
	}
}

// 719
// 일꾼동원 로직 체크필요
void WorkerManager::handleCombatWorkers()
{//ssh

	if (InformationManager::Instance().nowCombatStatus == InformationManager::combatStatus::DEFCON5
		&& (InformationManager::Instance().lastCombatStatus == InformationManager::combatStatus::DEFCON1 || InformationManager::Instance().lastCombatStatus == InformationManager::combatStatus::DEFCON2)
		)
	{
		bool findEnemy = false;
		//질럿저글링 처리
		for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
		{
			if (!unit) {
				continue;
			}

			if (!unit->getType().isWorker() && !unit->getType().isFlyer() && !InformationManager::Instance().getWallStatus())
			{
				if (BWTA::getRegion(BWAPI::TilePosition(unit->getPosition())) != InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getRegion()) {
					continue;
				}
				findEnemy = true;

				int maxCombatWorker = 6;
				if (InformationManager::Instance().nowCombatStatus == InformationManager::combatStatus::DEFCON2)
					maxCombatWorker = 4;
				for (auto & worker : workerData.getWorkers())
				{
					if (!worker) continue;

					if (WorkerManager::Instance().getWorkerData().getWorkerJob(worker) == WorkerData::Combat) {

						if (worker->getHitPoints() <= _freeHP) {
							setMineralWorker(worker);
						}
						else {
							maxCombatWorker--;
							Micro::SmartAttackMove(worker, unit->getPosition());
							//CommandUtil::attackUnit(worker, unit);
						}
					}

					if (maxCombatWorker == 0) {
						return;
					}
				}

				for (auto & worker : workerData.getWorkers())
				{
					if (!worker) continue;

					if (WorkerManager::Instance().getWorkerData().getWorkerJob(worker) == WorkerData::Minerals){
						if (worker->getHitPoints() > _freeHP) {
							workerData.setWorkerJob(worker, WorkerData::Combat, nullptr);
							Micro::SmartAttackMove(worker, unit->getPosition());
							//CommandUtil::attackUnit(worker, unit);
							maxCombatWorker--;
						}
					}

					if (maxCombatWorker == 0) {
						return;
					}
				}
			}
		}
		if (!findEnemy) {
			stopCombat();
		}

	}
}
void WorkerManager::handleScoutCombatWorker() {
	BWTA::BaseLocation * selfBaseLocation = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self());
	BWAPI::Unitset currentEnemyWorker;

	//일꾼처리 
	for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
	{
		if (unit->getType().isWorker())
		{
			if (_enemyworkerUnits.contains(unit)) {
				currentEnemyWorker.insert(unit);
				continue;
			}
			
			if (BWTA::getRegion(BWAPI::TilePosition(unit->getPosition())) == InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getRegion())
			{
				for (auto & worker : workerData.getWorkers())
				{
					if (!worker) continue;

					if (WorkerManager::Instance().getWorkerData().getWorkerJob(worker) == WorkerData::Minerals && worker->isCompleted() == true)
					{
						_enemyworkerUnits.insert(unit);
						currentEnemyWorker.insert(unit);
						setScoutCombatWorker(worker, unit);
						
						break;
					}
				}
			}
		}
	}
	for (auto it = _enemyworkerUnits.begin(); it != _enemyworkerUnits.end();) {
		
		if (*it == nullptr) {
			it = _enemyworkerUnits.erase(it);
		}
		else {
			bool flag = false;
			for (auto it2 = currentEnemyWorker.begin(); it2 != currentEnemyWorker.end();) {
				if (*it2 == nullptr) {
					it2++;
					continue;
				}
				if ((*it)->getID() == (*it2)->getID()) {
					flag = true;
					break;
				}
				else {
					it2++;
				}
			}
			BWAPI::Unit worker = workerData.getScoutCombatWorkerAssignedUnit(*it);
			if (flag == false || worker->getHitPoints() < 20) {
				
				if (worker != nullptr) {
					setMineralWorker(worker);
				}
				
				it = _enemyworkerUnits.erase(it);
			}
			else {
				it++;
			}
		}
	}
	
}


BWAPI::Unit WorkerManager::getClosestEnemyUnitFromWorker(BWAPI::Unit worker)
{
	//ssh
	if (!worker) return nullptr;

	BWAPI::Unit closestUnit = nullptr;
	double closestDist = 10000;

	for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
	{
		// 지상거리
		//double dist = unit->getDistance(worker);
		double dist = getGroundDistance(unit->getPosition(), worker->getPosition());
		if ((dist < 300) && (!closestUnit || (dist < closestDist)))
		{
			closestUnit = unit;
			closestDist = dist;
		}
	}

	return closestUnit;
}

void WorkerManager::stopCombat()
{
	for (auto & worker : workerData.getWorkers())
	{
		if (!worker) continue;

		if (workerData.getWorkerJob(worker) == WorkerData::Combat)
		{
			setMineralWorker(worker);
		}
	}
}

void WorkerManager::handleRepairWorkers()
{
	if (BWAPI::Broodwar->self()->getRace() != BWAPI::Races::Terran)
	{
		return;
	}

	if (workerData.getNumRepairWorkers() > 6) {
		return;
	}

	/*
	if (InformationManager::Instance().getWallStatus() == true && InformationManager::Instance().getWallPositions().size() > 0) {
		for (auto position : InformationManager::Instance().getWallPositions()) {
			// wall
			for (auto & unit : BWAPI::Broodwar->self()->getUnits())
			{
				// 건물의 경우 아무리 멀어도 무조건 수리. 일꾼 한명이 순서대로 수리
				if (unit->getType().isBuilding() && unit->isCompleted() == true && unit->getTilePosition() == position && unit->getHitPoints() < unit->getType().maxHitPoints())
				{
					//std::cout << "wall repair : " << unit->getID() << std::endl;
					int maxRepairWorker = 1;

					for (auto & worker : workerData.getWorkers())
					{
						if (WorkerManager::Instance().getWorkerData().getWorkerJob(worker) == WorkerData::Combat) {

							if (worker->getHitPoints() <= _freeHP) {
								workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
							}
							else {
								maxRepairWorker--;
								setRepairWorker(worker, unit);
							}
						}

						if (maxRepairWorker == 0) {
							return;
						}
					}

					for (auto & worker : workerData.getWorkers())
					{
						if (WorkerManager::Instance().getWorkerData().getWorkerJob(worker) == WorkerData::Minerals){
							if (worker->getHitPoints() > _freeHP) {
								setRepairWorker(worker, unit);
								//CommandUtil::attackUnit(worker, unit);
								maxRepairWorker--;
							}
						}

						if (maxRepairWorker == 0) {
							return;
						}
					}
					break;
				}
			}
		}
	}
	*/
	// building
	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (unit->getType().isBuilding() && unit->isCompleted() == true)
		{
			bool flag = false;
			if (InformationManager::Instance().getWallStatus() == true) {
				
				for (auto wallUnit : InformationManager::Instance().getWallUnits()) {
					if (wallUnit->getID() == unit->getID()) {
						flag = true;
						break;
					}
				}
			}
			
			if (flag) {
				if (unit->getHitPoints() < unit->getType().maxHitPoints() && workerData.getRepairUnitCountOneTarget(unit) < 3) {
					BWAPI::Unit repairWorker = chooseRepairWorkerClosestTo(unit->getPosition(), 30 * TILE_SIZE, true);
					
					setRepairWorker(repairWorker, unit);
				}
			}
			else {
				if (unit->getHitPoints() < (unit->getType().maxHitPoints() - int(unit->getType().maxHitPoints() * 0.4)) && workerData.getRepairUnitCountOneTarget(unit) == 0) {
					BWAPI::Unit repairWorker = chooseRepairWorkerClosestTo(unit->getPosition(), 30 * TILE_SIZE, true);
					setRepairWorker(repairWorker, unit);

					//std::cout << "building : " << repairWorker->getID() << ", target : " << unit->getID() << std::endl;
				}
			}
		}
		// tank
		else if ((unit->getType() == BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode || unit->getType() == BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode) && unit->isCompleted() == true && unit->getHitPoints() < unit->getType().maxHitPoints() && workerData.getRepairUnitCountOneTarget(unit) == 0)
		{
			// SCV 는 수리 대상에서 제외. 전투 유닛만 수리하도록 한다
			if (unit->getType() != BWAPI::UnitTypes::Terran_SCV) {
				BWAPI::Unit repairWorker = chooseRepairWorkerClosestTo(unit->getPosition(), 30 * TILE_SIZE, false);
				setRepairWorker(repairWorker, unit);

				break;
			}
		}
	}
}

BWAPI::Unit WorkerManager::chooseRepairWorkerClosestTo(BWAPI::Position p, int maxRange, bool isBuilding)
{
	if (!p.isValid()) return nullptr;

    BWAPI::Unit closestWorker = nullptr;
	double closestDist = 1000000000;
	/*
	if (isBuilding == true) {
		if (currentBuildingRepairWorker != nullptr && currentBuildingRepairWorker->exists() && currentBuildingRepairWorker->getHitPoints() > 40)
		{
			return currentBuildingRepairWorker;
		}
	}
	else {
		if (currentMechaRepairWorker != nullptr && currentMechaRepairWorker->exists() && currentMechaRepairWorker->getHitPoints() > 40)
		{
			return currentMechaRepairWorker;
		}
	}
	*/
    // for each of our workers
	for (auto & worker : workerData.getWorkers())
	{
		if (!worker)
		{
			continue;
		}

		if (worker->isCompleted() && !worker->isCarryingGas() && !worker->isCarryingMinerals()
			&& (workerData.getWorkerJob(worker) == WorkerData::Minerals || workerData.getWorkerJob(worker) == WorkerData::Idle || workerData.getWorkerJob(worker) == WorkerData::Move))
		{
			// 지상거리
			//double dist = worker->getDistance(p);
			double dist = getGroundDistance(worker->getPosition(), p);

			if (!closestWorker || dist < closestDist)
            {
				closestWorker = worker;
                dist = closestDist;
            }
		}
	}
	/*
	if (isBuilding == true) {
		if (currentBuildingRepairWorker == nullptr || currentBuildingRepairWorker->exists() == false || currentBuildingRepairWorker->getHitPoints() <= 40)
		{
			currentBuildingRepairWorker = closestWorker;
		}
	}
	else {
		if (currentMechaRepairWorker == nullptr || currentMechaRepairWorker->exists() == false || currentMechaRepairWorker->getHitPoints() <= 40)
		{
			currentMechaRepairWorker = closestWorker;
		}
	}
	*/
	return closestWorker;
}

void WorkerManager::handleBunkderRepairWorkers()
{
	if (InformationManager::Instance().nowCombatStatus == InformationManager::DEFCON2 || InformationManager::Instance().nowCombatStatus == InformationManager::DEFCON3 || InformationManager::Instance().nowCombatStatus == InformationManager::DEFCON5) {
		
		bool bunkerReapir = false;

		if (InformationManager::Instance().rushState == 1 && InformationManager::Instance().getRushSquad().size() > 0){
			bunkerReapir = true;
		}

		if (bunkerReapir) 
		{
			for (auto & unit : BWAPI::Broodwar->self()->getUnits())
			{
				// 건물의 경우 아무리 멀어도 무조건 수리. 일꾼 한명이 순서대로 수리
				if (unit->getType() == BWAPI::UnitTypes::Terran_Bunker && unit->isCompleted())
				{
					int maxBunkerRepairWorker = 5;
					for (auto & worker : workerData.getWorkers())
					{
						if (WorkerManager::Instance().getWorkerData().getWorkerJob(worker) == WorkerData::BunkerReapir) {

							if (worker->getHitPoints() <= _freeHP) {
								setMineralWorker(worker);
							}
							else {
								maxBunkerRepairWorker--;

								setBunkerRepairWorker(worker, unit);
								////std::cout << "worker : " << worker->getID() << "bunker : " << unit->getID() << std::endl;
								//CommandUtil::attackUnit(worker, unit);
							}
						}

						if (maxBunkerRepairWorker <= 0) {
							return;
						}
					}

					for (auto & worker : workerData.getWorkers())
					{
						if (WorkerManager::Instance().getWorkerData().getWorkerJob(worker) == WorkerData::Minerals){
							if (worker->getHitPoints() > _freeHP) {
								maxBunkerRepairWorker--;
								setBunkerRepairWorker(worker, unit);

								////std::cout << "worker : " << worker->getID() << "bunker : " << unit->getID() << std::endl;
							}
						}

						if (maxBunkerRepairWorker <= 0) {
							return;
						}
					}
				}
			}
		}
		else
		{
			stopBunkerRepair();
		}
	}
}

void WorkerManager::stopBunkerRepair()
{
	for (auto & worker : workerData.getWorkers())
	{
		if (!worker) continue;

		if (workerData.getWorkerJob(worker) == WorkerData::BunkerReapir)
		{
			setMineralWorker(worker);
		}
	}
}


BWAPI::Unit WorkerManager::getScoutWorker()
{
    // for each of our workers
	for (auto & worker : workerData.getWorkers())
	{
		if (!worker)
		{
			continue;
		}
		// if it is a scout worker
        if (workerData.getWorkerJob(worker) == WorkerData::Scout) 
		{
			return worker;
		}
	}

    return nullptr;
}

// set a worker to mine minerals
void WorkerManager::setMineralWorker(BWAPI::Unit unit)
{
	if (!unit) return;

	// check if there is a mineral available to send the worker to
	BWAPI::Unit depot = getClosestResourceDepotFromWorker(unit);
	
	// if there is a valid ResourceDepot (Command Center, Nexus, Hatchery)
	if (depot)
	{
		// update workerData with the new job
		workerData.setWorkerJob(unit, WorkerData::Minerals, depot);
	}
}
BWAPI::Unit WorkerManager::getClosestMineralWorkerTo(BWAPI::Position target)
{
	BWAPI::Unit closestUnit = nullptr;
	double closestDist = 1000000000;
	
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
			// 지상거리
			//double dist = unit->getDistance(target);
			double dist = getGroundDistance(unit->getPosition(), target);

			if (!closestUnit || dist < closestDist)
			{
				closestUnit = unit;
				closestDist = dist;
			}
		}
	}

	return closestUnit;
}

BWAPI::Unit WorkerManager::getClosestResourceDepotFromWorker(BWAPI::Unit worker)
{
	// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
	// 멀티 기지간 일꾼 숫자 리밸런싱이 잘 일어나도록 버그 수정

	if (!worker) return nullptr;

	BWAPI::Unit closestDepot = nullptr;
	double closestDistance = 1000000000;

	// 완성된, 공중에 떠있지 않고 땅에 정착해있는, ResourceDepot 혹은 Lair 나 Hive로 변형중인 Hatchery 중에서
	// 첫째로 미네랄 일꾼수가 꽉 차지않은 곳
	// 둘째로 가까운 곳을 찾는다
	//for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	for (auto e : ExpansionManager::Instance().getExpansions())
	{
		BWAPI::Unit unit = e.cc;
		
		if (!unit) continue;

		// 가장 가까운, 일꾼수가 꽉 차지않은, 완성된 ResourceDepot (혹은 Lair 나 Hive로 변형중인 건물)을 찾는다
		if (unit->getType().isResourceDepot()
			&& (unit->isCompleted() || unit->getType() == BWAPI::UnitTypes::Zerg_Lair || unit->getType() == BWAPI::UnitTypes::Zerg_Hive)
			&& unit->isLifted() == false)
		{
			// 지상거리
			//double dist = unit->getDistance(worker);
			double dist = getGroundDistance(unit->getPosition(), worker->getPosition());
			if (closestDistance > dist) {
				closestDepot = unit;
				closestDistance = dist;
			}
		}
	}

	// 모든 ResourceDepot 이 다 일꾼수가 꽉 차있거나, 완성된 ResourceDepot 이 하나도 없고 건설중이라면, 
	// ResourceDepot 주위에 미네랄이 남아있는 곳 중에서 가까운 곳이 선택되도록 한다
	if (closestDepot == nullptr) {
		//for (auto & unit : BWAPI::Broodwar->self()->getUnits())
		for (auto e : ExpansionManager::Instance().getExpansions())
		{
			BWAPI::Unit unit = e.cc;
			if (!unit) continue;

			if (unit->getType().isResourceDepot())
			{
				if (workerData.getMineralsNearDepot(unit) > 0) {
					
					// 지상거리
					//double dist = unit->getDistance(worker);
					double dist = getGroundDistance(unit->getPosition(), worker->getPosition());
					if (closestDistance > dist) {
						closestDepot = unit;
						closestDistance = dist;
					}
				}
			}
		}
	}

	// 모든 ResourceDepot 주위에 미네랄이 하나도 없다면, 일꾼에게 가장 가까운 곳을 선택한다  
	if (closestDepot == nullptr) {
		//for (auto & unit : BWAPI::Broodwar->self()->getUnits())
		for (auto e : ExpansionManager::Instance().getExpansions())
		{
			BWAPI::Unit unit = e.cc;

			if (!unit) continue;
			if (unit->getType().isResourceDepot())
			{
				// 지상거리
				//double dist = unit->getDistance(worker);
				double dist = getGroundDistance(unit->getPosition(), worker->getPosition());

				if (closestDistance > dist) {
					closestDepot = unit;
					closestDistance = dist;
				}
			}
		}
	}

	return closestDepot;

	// BasicBot 1.1 Patch End //////////////////////////////////////////////////
}


// other managers that need workers call this when they're done with a unit
void WorkerManager::setIdleWorker(BWAPI::Unit unit)
{
	if (!unit) return;

	workerData.setWorkerJob(unit, WorkerData::Idle, nullptr);
}

// 해당 refinery 로부터 가장 가까운, Mineral 캐고있던 일꾼을 리턴한다
BWAPI::Unit WorkerManager::chooseGasWorkerFromMineralWorkers(BWAPI::Unit refinery)
{
	if (!refinery) return nullptr;

	BWAPI::Unit closestWorker = nullptr;
	// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
	// 변수 기본값 수정

	double closestDistance = 1000000000;

	// BasicBot 1.1 Patch End //////////////////////////////////////////////////

	for (auto & unit : workerData.getWorkers())
	{
		if (!unit) continue;
		
		if (unit->isCompleted() && !unit->isCarryingMinerals() && (workerData.getWorkerJob(unit) == WorkerData::Minerals))
		{
			// 지상거리
			//double dist = unit->getDistance(refinery);
			double dist = getGroundDistance(unit->getPosition(), refinery->getPosition());
			if (!closestWorker || dist < closestDistance)
			{
				closestWorker = unit;
				closestDistance = dist;
			}
		}
	}

	return closestWorker;
}

void WorkerManager::setConstructionWorker(BWAPI::Unit worker, BWAPI::UnitType buildingType)
{
	if (!worker) return;

	workerData.setWorkerJob(worker, WorkerData::Build, buildingType);
}

BWAPI::Unit WorkerManager::chooseConstuctionWorkerClosestTo(BWAPI::UnitType buildingType, BWAPI::TilePosition buildingPosition, bool setJobAsConstructionWorker, int avoidWorkerID)
{
	// variables to hold the closest worker of each type to the building
	BWAPI::Unit closestMovingWorker = nullptr;
	BWAPI::Unit closestMiningWorker = nullptr;
	// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
	// 변수 기본값 수정

	double closestMovingWorkerDistance = 1000000000;
	double closestMiningWorkerDistance = 1000000000;

	// BasicBot 1.1 Patch End //////////////////////////////////////////////////

	// look through each worker that had moved there first
	for (auto & unit : workerData.getWorkers())
	{
		if (!unit) continue;

		// worker 가 2개 이상이면, avoidWorkerID 는 피한다
		if (workerData.getWorkers().size() >= 2 && avoidWorkerID != 0 && unit->getID() == avoidWorkerID) continue;

		// Move / Idle Worker
		//if (unit->isCompleted() && (workerData.getWorkerJob(unit) == WorkerData::Move || workerData.getWorkerJob(unit) == WorkerData::Idle))
		/*
		if (unit->isCompleted() && (workerData.getWorkerJob(unit) == WorkerData::Move || workerData.getWorkerJob(unit) == WorkerData::Idle) && workerData.getWorkerJob(unit) != WorkerData::Scout)
		{
			// if it is a new closest distance, set the pointer
			//double distance = unit->getDistance(BWAPI::Position(buildingPosition));
			double dist = getGroundDistance(unit->getPosition(), BWAPI::Position(buildingPosition));
			if (!closestMovingWorker || dist < closestMovingWorkerDistance)
			{
				if (BWTA::isConnected(unit->getTilePosition(), buildingPosition)) {
					closestMovingWorker = unit;
					closestMovingWorkerDistance = dist;
				}
			}
		}
        */
		// Move / Idle Worker 가 없을때, 다른 Worker 중에서 차출한다 
		//if (unit->isCompleted() && !unit->isCarryingGas() && !unit->isCarryingMinerals() && workerData.getWorkerJob(unit) == WorkerData::Minerals)
		if (unit->isCompleted() && !unit->isCarryingGas() && workerData.getWorkerJob(unit) == WorkerData::Minerals && workerData.getWorkerJob(unit) != WorkerData::Scout)
		{
			// if it is a new closest distance, set the pointer
			// 지상거리
			//double dist = unit->getDistance(BWAPI::Position(buildingPosition));
			double dist = getGroundDistance(unit->getPosition(), BWAPI::Position(buildingPosition));
			if (!closestMiningWorker || dist < closestMiningWorkerDistance)
			{
				if (BWTA::isConnected(unit->getTilePosition(), buildingPosition)) {
					closestMiningWorker = unit;
					closestMiningWorkerDistance = dist;
				}
			}
		}
	}
	
	/*
	if (closestMiningWorker)
		//std::cout << "closestMiningWorker " << closestMiningWorker->getID() << std::endl;
	if (closestMovingWorker)
		//std::cout << "closestMovingWorker " << closestMovingWorker->getID() << std::endl;
	*/
	
	//BWAPI::Unit chosenWorker = closestMovingWorker ? closestMovingWorker : closestMiningWorker;
    BWAPI::Unit chosenWorker = closestMiningWorker;

	// if the worker exists (one may not have been found in rare cases)
	if (chosenWorker && setJobAsConstructionWorker)
	{
		workerData.setWorkerJob(chosenWorker, WorkerData::Build, buildingType);
	}

	return chosenWorker;
}

// sets a worker as a scout
void WorkerManager::setScoutWorker(BWAPI::Unit worker)
{
	if (!worker) return;

	workerData.setWorkerJob(worker, WorkerData::Scout, nullptr);
}

// get a worker which will move to a current location
BWAPI::Unit WorkerManager::chooseMoveWorkerClosestTo(BWAPI::Position p)
{
	// set up the pointer
	BWAPI::Unit closestWorker = nullptr;
	// BasicBot 1.1 Patch Start /////////////////////////setMoveWorker///////////////////////
	// 변수 기본값 수정

	double closestDistance = 1000000000;

	// BasicBot 1.1 Patch End //////////////////////////////////////////////////

	// for each worker we currently have
	for (auto & unit : workerData.getWorkers())
	{
		if (!unit) continue;

		// only consider it if it's a mineral worker
		if (unit->isCompleted() && !unit->isCarryingGas() && !unit->isCarryingMinerals() && (workerData.getWorkerJob(unit) == WorkerData::Minerals || workerData.getWorkerJob(unit) == WorkerData::Idle))
		{
			// if it is a new closest distance, set the pointer
			// 지상거리
			// double dist = unit->getDistance(p);
			double dist = getGroundDistance(unit->getPosition(), p);

			if (!closestWorker || dist < closestDistance)
			{
				closestWorker = unit;
				closestDistance = dist;
			}
		}
	}

	// return the worker
	return closestWorker;
}

// sets a worker to move to a given location
void WorkerManager::setMoveWorker(BWAPI::Unit worker, int mineralsNeeded, int gasNeeded, BWAPI::Position p)
{
	// set up the pointer
	BWAPI::Unit closestWorker = nullptr;
	// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
	// 변수 기본값 수정

	double closestDistance = 1000000000;

	// BasicBot 1.1 Patch End //////////////////////////////////////////////////

	// for each worker we currently have
	for (auto & unit : workerData.getWorkers())
	{
		if (!unit) continue;
		
		// only consider it if it's a mineral worker or idle worker
		if (unit->isCompleted() && (workerData.getWorkerJob(unit) == WorkerData::Minerals || workerData.getWorkerJob(unit) == WorkerData::Idle))
		{
			// if it is a new closest distance, set the pointer
			// 지상거리
			//double distance = unit->getDistance(p);
			double dist = getGroundDistance(unit->getPosition(), p);
			if (!closestWorker || dist < closestDistance)
			{
				closestWorker = unit;
				closestDistance = dist;
			}
		}
	}

	if (closestWorker)
	{
		workerData.setWorkerJob(closestWorker, WorkerData::Move, WorkerMoveData(mineralsNeeded, gasNeeded, p));
	}
	else
	{
		//BWAPI::Broodwar->printf("Error, no worker found");
	}
}

void WorkerManager::setCombatWorker(BWAPI::Unit worker)
{
	if (!worker) return;

	workerData.setWorkerJob(worker, WorkerData::Combat, nullptr);
}

void WorkerManager::setScoutCombatWorker(BWAPI::Unit worker, BWAPI::Unit unitToAttack)
{
	if (!worker) return;

	workerData.setWorkerJob(worker, WorkerData::ScoutCombat, unitToAttack);
}

void WorkerManager::setRepairWorker(BWAPI::Unit worker, BWAPI::Unit unitToRepair)
{
	if (!worker) return;
	workerData.setWorkerJob(worker, WorkerData::Repair, unitToRepair);
}

void WorkerManager::setBunkerRepairWorker(BWAPI::Unit worker, BWAPI::Unit unitToRepair)
{
	if (!worker) return;
	workerData.setWorkerJob(worker, WorkerData::BunkerReapir, unitToRepair);
}

void WorkerManager::stopRepairing(BWAPI::Unit worker)
{
	if (!worker) return;
	workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
}


void WorkerManager::onUnitMorph(BWAPI::Unit unit)
{
	if (!unit) return;

	// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
	// 일꾼 탄생/파괴 등에 대한 업데이트 로직 버그 수정

	// onUnitComplete 에서 처리하도록 수정
	// if something morphs into a worker, add it
	//if (unit->getType().isWorker() && unit->getPlayer() == BWAPI::Broodwar->self() && unit->getHitPoints() >= 0)
	//{
	//	workerData.addWorker(unit);
	//}

	// if something morphs into a building, it was a worker (Zerg Drone)
	if (unit->getType().isBuilding() && unit->getPlayer() == BWAPI::Broodwar->self() && unit->getPlayer()->getRace() == BWAPI::Races::Zerg)
	{
		// 해당 worker 를 workerData 에서 삭제한다
		workerData.workerDestroyed(unit);
		rebalanceWorkers();
	}

	// BasicBot 1.1 Patch End //////////////////////////////////////////////////
}

// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
// 일꾼 탄생/파괴 등에 대한 업데이트 로직 버그 수정 : onUnitShow 가 아니라 onUnitComplete 에서 처리하도록 수정
// onUnitShow 메소드 제거
/*
void WorkerManager::onUnitShow(BWAPI::Unit unit)
{
}
*/

void WorkerManager::onUnitComplete(BWAPI::Unit unit){
	if (!unit) return;
	
	// 커맨드 센터 신규 추가의 경우 - 신승호 로직
	// 일단 신승호 버전으로 진행 - 무조건 멀티 생기면 엔빵하는것 같음
	
	if (unit->getType().isResourceDepot() && unit->getPlayer()->isNeutral() == false && unit->getPlayer() == BWAPI::Broodwar->self())
	{
		int checkscv = 0;

		for (auto & unit__ : workerData.getWorkers())
		{
			if (!unit__) continue;

			if (unit__->isCompleted() && workerData.getWorkerJob(unit__) == WorkerData::Minerals)
			{
				if ((checkscv++ % ExpansionManager::Instance().getExpansions().size()) == 0) {
					
					workerData.setWorkerJob(unit__, WorkerData::Minerals, unit);
				}
			}
		}
	}
	
	// 커맨드 센터 신규 추가의 경우 - 배포버전 로직
	/*
	// ResourceDepot 건물이 신규 생성되면, 자료구조 추가 처리를 한 후, rebalanceWorkers 를 한다
	if (unit->getType().isResourceDepot() && unit->getPlayer() == BWAPI::Broodwar->self())
	{
		workerData.addDepot(unit);
		rebalanceWorkers();
	}
	*/

	// 일꾼이 신규 생성되면, 자료구조 추가 처리를 한다. 
	if (unit->getType().isWorker() && unit->getPlayer() == BWAPI::Broodwar->self() && unit->getHitPoints() >= 0)
	{
		workerData.addWorker(unit);
		rebalanceWorkers();
	}
}

void WorkerManager::handleBlockWorkers() {

}

// 일하고있는 resource depot 에 충분한 수의 mineral worker 들이 지정되어 있다면, idle 상태로 만든다
// idle worker 에게 mineral job 을 부여할 때, mineral worker 가 부족한 resource depot 으로 이동하게 된다  
void WorkerManager::rebalanceWorkers()
{
	for (auto & worker : workerData.getWorkers())
	{
		if (!worker) continue;

		if (!workerData.getWorkerJob(worker) == WorkerData::Minerals)
		{
			continue;
		}

		BWAPI::Unit depot = workerData.getWorkerDepot(worker);

		if (depot && workerData.depotHasEnoughMineralWorkers(depot))
		{
			workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
		}
		else if (!depot)
		{
			workerData.setWorkerJob(worker, WorkerData::Idle, nullptr);
		}
	}
}

void WorkerManager::onUnitDestroy(BWAPI::Unit unit) 
{
	if (!unit) return;

	/*
	if (unit->getType().isResourceDepot() && unit->getPlayer() == BWAPI::Broodwar->self())
	{
		workerData.removeDepot(unit);
	}
	*/

	if (unit->getType().isWorker() && unit->getPlayer() == BWAPI::Broodwar->self()) 
	{
		workerData.workerDestroyed(unit);
	}

	if (unit->getType() == BWAPI::UnitTypes::Resource_Mineral_Field)
	{
		rebalanceWorkers();
	}
}

bool WorkerManager::isMineralWorker(BWAPI::Unit worker)
{
	if (!worker) return false;

//	return workerData.getWorkerJob(worker) == WorkerData::Minerals || workerData.getWorkerJob(worker) == WorkerData::Idle;
	return (workerData.getWorkerJob(worker) == WorkerData::Minerals || workerData.getWorkerJob(worker) == WorkerData::Idle) && (workerData.getWorkerJob(worker) != WorkerData::Build);
}

bool WorkerManager::isScoutWorker(BWAPI::Unit worker)
{
	if (!worker) return false;

	return (workerData.getWorkerJob(worker) == WorkerData::Scout);
}

bool WorkerManager::isConstructionWorker(BWAPI::Unit worker)
{
	if (!worker) return false;
	
	return (workerData.getWorkerJob(worker) == WorkerData::Build);
}

int WorkerManager::getNumMineralWorkers() 
{
	return workerData.getNumMineralWorkers();	
}

int WorkerManager::getNumIdleWorkers() 
{
	return workerData.getNumIdleWorkers();	
}

int WorkerManager::getNumGasWorkers() 
{
	return workerData.getNumGasWorkers();
}

int WorkerManager::getNumBunkerRepairWorkers()
{
	return workerData.getNumBunkerRepairWorkers();
}

WorkerData  WorkerManager::getWorkerData()
{
	return workerData;
}

double WorkerManager::getGroundDistance(BWAPI::Position from, BWAPI::Position to) {
	
	double dist = MapTools::Instance().getGroundDistance(from, to);
	// dist = BWTA::getGroundDistance(BWAPI::TilePosition(from.x / 32, from.y / 32), BWAPI::TilePosition(to.x / 32, to.y / 32));
	if (dist < 0 || dist > 1000000000) {
		dist = 1000000000;
	}
	return dist;
}