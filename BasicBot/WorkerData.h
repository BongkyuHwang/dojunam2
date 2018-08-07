#pragma once

#include "Common.h"

namespace MyBot
{
	class WorkerMoveData
	{
	public:

		int mineralsNeeded;
		int gasNeeded;
		BWAPI::Position position;

		WorkerMoveData(int m, int g, BWAPI::Position p)
			: mineralsNeeded(m)
			, gasNeeded(g)
			, position(p) {
		}

		WorkerMoveData() {}
	};

	class WorkerData 
	{

	public:
		/// 일꾼 유닛에게 지정하는 임무의 종류
		enum WorkerJob { 
			Minerals, 		///< 미네랄 채취 
			Gas,			///< 가스 채취
			Build,			///< 건물 건설
			Combat, 		///< 전투
			Idle,			///< 하는 일 없음. 대기 상태. 
			Repair,			///< 수리. Terran_SCV 만 가능
			BunkerReapir,
			WallReapir,
			Move,			///< 이동
			Scout, 			///< 정찰. Move와 다름. Mineral / Gas / Build 등의 다른 임무로 차출되지 않게 됨.
			Block,                  /// for enemy blocking
			ScoutCombat,           /// for combat with enmey scout worker
			Default 		///< 기본. 미설정 상태. 
		};

	private:

		/// 일꾼 목록
		BWAPI::Unitset								  workers;

		/// ResourceDepot 목록
		//BWAPI::Unitset								  depots;

		std::map<BWAPI::Unit, enum WorkerJob>         workerJobMap;
		std::map<BWAPI::Unit, BWAPI::Unit>			  workerMineralMap;
		std::map<BWAPI::Unit, BWAPI::Unit>			  workerDepotMap;
		std::map<BWAPI::Unit, BWAPI::Unit>			  workerRefineryMap;
		std::map<BWAPI::Unit, BWAPI::Unit>			  workerRepairMap;
		std::map<BWAPI::Unit, BWAPI::Unit>			  workerBunkerRepairMap;
		std::map<BWAPI::Unit, BWAPI::Unit>			  workerScoutCombatMap;
		std::map<BWAPI::Unit, WorkerMoveData>         workerMoveMap;
		std::map<BWAPI::Unit, BWAPI::UnitType>        workerBuildingTypeMap;
		
		std::map<BWAPI::Unit, int>                    depotWorkerCount;
		std::map<BWAPI::Unit, int>                    refineryWorkerCount;

		std::map<BWAPI::Unit, int>                    workersOnMineralPatch;
		std::map<BWAPI::Unit, BWAPI::Unit>			  workerMineralAssignment;

		void clearPreviousJob(BWAPI::Unit unit);

	public:

		WorkerData();

		const BWAPI::Unitset &  getWorkers() const { return workers; }

		void					printWorkerJob();

		void					addWorker(BWAPI::Unit unit);
		void					addWorker(BWAPI::Unit unit, WorkerJob job, BWAPI::Unit jobUnit);
		void					addWorker(BWAPI::Unit unit, WorkerJob job, BWAPI::UnitType jobUnitType);
		void					workerDestroyed(BWAPI::Unit unit);

		void					addDepot(BWAPI::Unit unit);
		void					removeDepot(BWAPI::Unit unit);
		BWAPI::Unitset			getDepots();

		void					setWorkerJob(BWAPI::Unit unit, WorkerJob job, BWAPI::Unit jobUnit);
		void					setWorkerJob(BWAPI::Unit unit, WorkerJob job, WorkerMoveData wmd);
		void					setWorkerJob(BWAPI::Unit unit, WorkerJob job, BWAPI::UnitType jobUnitType);

		int						getNumWorkers() const;
		int						getNumMineralWorkers() const;
		int						getNumGasWorkers() const;
		int						getNumIdleWorkers() const;
		int						getNumCombatWorkers() const;
		int						getNumBunkerRepairWorkers() const;
		int						getNumRepairWorkers() const;

		char					getJobCode(BWAPI::Unit unit);

		void					getMineralWorkers(std::set<BWAPI::Unit> & mw);
		void					getGasWorkers(std::set<BWAPI::Unit> & mw);
		void					getBuildingWorkers(std::set<BWAPI::Unit> & mw);
		void					getRepairWorkers(std::set<BWAPI::Unit> & mw);
	
		double					mineralAndMineralWorkerRatio;

		bool					depotHasEnoughMineralWorkers(BWAPI::Unit depot);
		int						getMineralsNearDepot(BWAPI::Unit depot);

		int						getNumAssignedWorkers(BWAPI::Unit unit);
		BWAPI::Unit             getMineralToMine(BWAPI::Unit worker);

		enum WorkerJob			getWorkerJob(BWAPI::Unit unit);
		BWAPI::Unit             getWorkerResource(BWAPI::Unit unit);
		BWAPI::Unit             getWorkerDepot(BWAPI::Unit unit);
		BWAPI::Unit             getWorkerRepairUnit(BWAPI::Unit unit);
		BWAPI::UnitType			getWorkerBuildingType(BWAPI::Unit unit);
		WorkerMoveData			getWorkerMoveData(BWAPI::Unit unit);

		BWAPI::Unitset          getMineralPatchesNearDepot(BWAPI::Unit depot);
		void                    addToMineralPatch(BWAPI::Unit unit, int num);
		int getDepotWorkerCount(BWAPI::Unit &u);
		int getRepairUnitCountOneTarget(BWAPI::Unit unit);
		BWAPI::Unit WorkerData::getScoutCombatWorkerAssignedUnit(BWAPI::Unit enemy);
	};
}
