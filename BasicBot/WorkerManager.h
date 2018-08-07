#pragma once


#include "Common.h"
#include "WorkerData.h"
#include "ConstructionTask.h"
#include "ConstructionManager.h"
#include "InformationManager.h"
#include "UABAssert.h"


namespace MyBot
{

	/// 일꾼 유닛들의 상태를 관리하고 컨트롤하는 class
	class WorkerManager
	{
		//719
		//ssh
		BWAPI::Unitset          _enemyworkerUnits;
		int			_freeHP;
	public:
		/// 각 Worker 에 대한 WorkerJob 상황을 저장하는 자료구조 객체
		WorkerData  workerData;
		bool initial_attack;


		/// 일꾼 중 한명을 Repair Worker 로 정해서, 전체 수리 대상을 하나씩 순서대로 수리합니다
		BWAPI::Unit currentBuildingRepairWorker;
		BWAPI::Unit currentMechaRepairWorker;

				
		void        updateWorkerStatus();
		//719
		void		onUnitComplete(BWAPI::Unit unit);
		/// Idle 일꾼을 Mineral 일꾼으로 만듭니다
		void        handleIdleWorkers();

		void        handleGasWorkers();
		void        handleMoveWorkers();
		void        handleCombatWorkers();
		void        handleRepairWorkers();
		void		handleBunkderRepairWorkers();
		void		handleScoutCombatWorker();
		void        rebalanceWorkers();
		void		handleBlockWorkers();
		WorkerManager();


		// kyj
		void        finishedWithWorker(BWAPI::Unit unit);
		//////////////////////////////////////////////////////////////////////
		/// static singleton 객체를 리턴합니다
		static WorkerManager &  Instance();

		/// 일꾼 유닛들의 상태를 저장하는 workerData 객체를 업데이트하고, 일꾼 유닛들이 자원 채취 등 임무 수행을 하도록 합니다
		void        update();

		/// 일꾼 유닛들의 상태를 저장하는 workerData 객체를 업데이트합니다
		void        onUnitDestroy(BWAPI::Unit unit);

		/// 일꾼 유닛들의 상태를 저장하는 workerData 객체를 업데이트합니다
		void        onUnitMorph(BWAPI::Unit unit);

		/// 일꾼 유닛들의 상태를 저장하는 workerData 객체를 업데이트합니다
		//void        onUnitShow(BWAPI::Unit unit);
		
		
		/// 일꾼 유닛들의 상태를 저장하는 workerData 객체를 리턴합니다
		WorkerData  getWorkerData();



		/// 해당 일꾼 유닛 unit 의 WorkerJob 값를 Idle 로 변경합니다
		void        setIdleWorker(BWAPI::Unit unit);
		
		/// idle 상태인 일꾼 유닛 unit 의 숫자를 리턴합니다
		int         getNumIdleWorkers();



		/// 해당 일꾼 유닛 unit 의 WorkerJob 값를 Mineral 로 변경합니다
		void        setMineralWorker(BWAPI::Unit unit);
		int         getNumMineralWorkers();
		bool        isMineralWorker(BWAPI::Unit worker);
		BWAPI::Unit chooseMineralWorkerFromGasWorkers(BWAPI::Unit refinery);

		/// target 으로부터 가장 가까운 Mineral 일꾼 유닛을 리턴합니다
		BWAPI::Unit getClosestMineralWorkerTo(BWAPI::Position target);

		/// 해당 일꾼 유닛 unit 으로부터 가장 가까운 ResourceDepot 건물을 리턴합니다
		BWAPI::Unit getClosestResourceDepotFromWorker(BWAPI::Unit worker);


		/// Mineral 일꾼 유닛들 중에서 Gas 임무를 수행할 일꾼 유닛을 정해서 리턴합니다
		/// Idle 일꾼은 Build, Repair, Scout 등 다른 임무에 먼저 투입되어야 하기 때문에 Mineral 일꾼 중에서만 정합니다
		BWAPI::Unit chooseGasWorkerFromMineralWorkers(BWAPI::Unit refinery);
		int         getNumGasWorkers();

		/// Mineral 혹은 Idle 일꾼 유닛들 중에서 Scout 임무를 수행할 일꾼 유닛을 정해서 리턴합니다
		BWAPI::Unit getScoutWorker();
		void        setScoutWorker(BWAPI::Unit worker);
		bool        isScoutWorker(BWAPI::Unit worker);

		/// buildingPosition 에서 가장 가까운 Move 혹은 Idle 혹은 Mineral 일꾼 유닛들 중에서 Construction 임무를 수행할 일꾼 유닛을 정해서 리턴합니다
		/// Move / Idle Worker 중에서 먼저 선정하고, 없으면 Mineral Worker 중에서 선정합니다
		/// 일꾼 유닛이 2개 이상이면, avoidWorkerID 에 해당하는 worker 는 선정하지 않도록 합니다
		/// if setJobAsConstructionWorker is true (default), it will be flagged as a builder unit
		/// if setJobAsConstructionWorker is false, we just want to see which worker will build a building
		BWAPI::Unit chooseConstuctionWorkerClosestTo(BWAPI::UnitType buildingType, BWAPI::TilePosition buildingPosition, bool setJobAsConstructionWorker = true, int avoidWorkerID = 0);
		void        setConstructionWorker(BWAPI::Unit worker, BWAPI::UnitType buildingType);
		bool        isConstructionWorker(BWAPI::Unit worker);

		/// position 에서 가장 가까운 Mineral 혹은 Idle 혹은 Move 일꾼 유닛들 중에서 Repair 임무를 수행할 일꾼 유닛을 정해서 리턴합니다
		BWAPI::Unit chooseRepairWorkerClosestTo(BWAPI::Position p, int maxRange = 100000000, bool isBuilding = true);
		void        setRepairWorker(BWAPI::Unit worker, BWAPI::Unit unitToRepair);
		void		setBunkerRepairWorker(BWAPI::Unit worker, BWAPI::Unit unitToRepair);
		void		stopBunkerRepair();
		void        stopRepairing(BWAPI::Unit worker);
		int			getNumBunkerRepairWorkers();
		/// position 에서 가장 가까운 Mineral 혹은 Idle 일꾼 유닛들 중에서 Move 임무를 수행할 일꾼 유닛을 정해서 리턴합니다
		void        setMoveWorker(BWAPI::Unit worker, int m, int g, BWAPI::Position p);
		BWAPI::Unit chooseMoveWorkerClosestTo(BWAPI::Position p);

		/// 해당 일꾼 유닛으로부터 가장 가까운 적군 유닛을 리턴합니다
		BWAPI::Unit getClosestEnemyUnitFromWorker(BWAPI::Unit worker);

		/// 해당 일꾼 유닛에게 Combat 임무를 부여합니다
		void        setCombatWorker(BWAPI::Unit worker);
		/// 모든 Combat 일꾼 유닛에 대해 임무를 해제합니다
		void        stopCombat();
	
		void		setScoutCombatWorker(BWAPI::Unit worker, BWAPI::Unit unitToAttack);

		double		getGroundDistance(BWAPI::Position from, BWAPI::Position to);
	};
};
