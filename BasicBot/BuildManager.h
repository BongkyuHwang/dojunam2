#pragma once

#include "Common.h"
#include "BuildOrderQueue.h"
#include "ConstructionManager.h"
#include "BuildOrder.h"
#include "BOSSManager.h"
#include "ExpansionManager.h"
#include "TimeManager.h"

namespace MyBot
{
	/// 빌드(건물 건설 / 유닛 훈련 / 테크 리서치 / 업그레이드) 명령을 순차적으로 실행하기 위해 빌드 큐를 관리하고, 빌드 큐에 있는 명령을 하나씩 실행하는 class
	/// 빌드 명령 중 건물 건설 명령은 ConstructionManager로 전달합니다
	/// @see ConstructionManager
	class BuildManager
	{

		BuildManager();

		/// 해당 MetaType 을 build 할 수 있는 producer 를 찾아 반환합니다
		/// @param t 빌드하려는 대상의 타입
		/// @param closestTo 파라메타 입력 시 producer 후보들 중 해당 position 에서 가장 가까운 producer 를 리턴합니다
		/// @param producerID 파라메타 입력 시 해당 ID의 unit 만 producer 후보가 될 수 있습니다
		BWAPI::Unit			getProducer(MetaType t, BWAPI::Position closestTo = BWAPI::Positions::None, int producerID = -1);

		BWAPI::Unit			canMakeNowAndGetProducer(MetaType t, BWAPI::Position closestTo = BWAPI::Positions::None, int producerID = -1);

		BWAPI::Unit         getClosestUnitToPosition(const BWAPI::Unitset & units, BWAPI::Position closestTo);
		BWAPI::Unit         selectUnitOfType(BWAPI::UnitType type, BWAPI::Position closestTo = BWAPI::Position(0, 0));

		bool                hasEnoughResources(MetaType type);
		bool                hasNumCompletedUnitType(BWAPI::UnitType type, int num);

		bool                canMakeNow(BWAPI::Unit producer, MetaType t);
		bool                canMake(BWAPI::Unit producer, MetaType t);

		BWAPI::TilePosition getDesiredPosition(BWAPI::UnitType unitType, BWAPI::TilePosition seedPosition, BuildOrderItem::SeedPositionStrategy seedPositionStrategy);

		void				checkBuildOrderQueueDeadlockAndRemove();
		//잘못된 빌드 삭제
		void                checkErrorBuildOrderAndRemove();

		//addon 관련 : 예외처리판단함수
		bool verifyBuildAddonCommand(BWAPI::Unit u);

		bool				_enemyCloakedDetected;
		bool detectSupplyDeadlock();
		
		int maxDepotWorkers;

	public:
		/// static singleton 객체를 리턴합니다
		static BuildManager &	Instance();

		void onStart();

		/// buildQueue 에 대해 Dead lock 이 있으면 제거하고, 가장 우선순위가 높은 BuildOrderItem 를 실행되도록 시도합니다
		void				update();
		void                consumeBuildQueue();
		void                consumeRemainingResource();
		/// BuildOrderItem 들의 목록을 저장하는 buildQueue 
		BuildOrderQueue     buildQueue;

		/// BuildOrderItem 들의 목록을 저장하는 buildQueue 를 리턴합니다
		BuildOrderQueue *	getBuildQueue();

		/// buildQueue 의 Dead lock 여부를 판단하기 위해, 가장 우선순위가 높은 BuildOrderItem 의 producer 가 존재하게될 것인지 여부를 리턴합니다
		bool				isProducerWillExist(BWAPI::UnitType producerType);
		void			    performBuildOrderSearch();
				//@djn ssh
		void        queueGasSteal();
		void		onUnitComplete(BWAPI::Unit unit);

		void                setBuildOrder(const BuildOrder & buildOrder);
		void                addBuildOrderOneItem(MetaType buildOrder, BWAPI::TilePosition position = BWAPI::TilePositions::None, BuildOrderItem::SeedPositionStrategy seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::MainBaseLocation);
		void executeWorkerTraining();
		void executeCombatUnitTraining(std::pair<int, int> availableResource);

		//addon 달려있는지 확인하는 함수
		bool hasAddon(BWAPI::Unit u);
		bool isConstructingAddon(BWAPI::Unit u);

		bool hasUnitInQueue(BWAPI::UnitType ut);

		//자원확인
		int                 getAvailableMinerals();
		int                 getAvailableGas();
		std::pair<int, int> getQueueResource();
		int getQueueSupplyRequired(bool subtract_supply_depot, bool stop_supply_depot);
		const std::pair<int, int> marginResource;

		void defenceFlyingAndDetect();
		bool addBuildings(MetaType & ut, int max_num, BuildOrderItem::SeedPositionStrategy seedPositionStrategy);
		bool addBuildings(MetaType & ut, int max_num, std::vector<BuildOrderItem::SeedPositionStrategy> seedPositionStrategies);
	};


}
