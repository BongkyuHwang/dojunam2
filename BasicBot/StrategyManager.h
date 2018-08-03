#pragma once

#include "Common.h"

#include "UnitData.h"
#include "BuildOrderQueue.h"
#include "InformationManager.h"
#include "WorkerManager.h"
#include "BuildManager.h"
#include "ConstructionManager.h"
#include "ScoutManager.h"
#include "BuildOrder.h"
#include "MetaType.h"

namespace MyBot
{

	typedef std::pair<MetaType, size_t> MetaPair;
	typedef std::vector<MetaPair> MetaPairVector;

	struct Strategy
	{
		enum main_strategies{
			None,
			Bionic,
			Bionic_Tank,
			Mechanic,
			One_Fac,
			Two_Fac,
			Mechanic_Goliath,
			Mechanic_Vessel,
			BSB,
			BBS,
			BATTLE
		};

		main_strategies next_strategy;
		main_strategies pre_strategy;
		std::map<std::string, int> num_unit_limit;
		std::string opening_build_order;
	};

	/// 상황을 판단하여, 정찰, 빌드, 공격, 방어 등을 수행하도록 총괄 지휘를 하는 class
	/// InformationManager 에 있는 정보들로부터 상황을 판단하고, 
	/// BuildManager 의 buildQueue에 빌드 (건물 건설 / 유닛 훈련 / 테크 리서치 / 업그레이드) 명령을 입력합니다.
	/// 정찰, 빌드, 공격, 방어 등을 수행하는 코드가 들어가는 class
	class StrategyManager
	{
		StrategyManager();

		void executeWorkerTraining();
		void executeSupplyManagement();
		void executeBasicCombatUnitTraining();

		std::map<Strategy::main_strategies, Strategy> _strategies;
		BuildOrder _openingBuildOrder;
		Strategy::main_strategies _main_strategy;
		const BuildOrder                _emptyBuildOrder;
		const MetaPairVector getTerranBuildOrderGoal();
		void setOpeningBookBuildOrder();
		bool changeMainStrategy(std::map<std::string, int> & numUnits);
		bool checkStrategyLimit(std::string &name, std::map<std::string, int> & numUnits);

		std::map<std::string, std::vector<int>> unit_ratio_table;

		void initStrategies();
		void initUnitRatioTable();
		bool obtainNextUpgrade(BWAPI::UpgradeType upgType);

	public:
		bool isInitialBuildOrderFinished;
		bool isFullScaleAttackStarted;

		/// static singleton 객체를 리턴합니다
		static StrategyManager &	Instance();

		/// 경기가 시작될 때 일회적으로 전략 초기 세팅 관련 로직을 실행합니다
		void onStart();

		///  경기가 종료될 때 일회적으로 전략 결과 정리 관련 로직을 실행합니다
		void onEnd(bool isWinner);

		/// 경기 진행 중 매 프레임마다 경기 전략 관련 로직을 실행합니다
		void update();

		const BuildOrder & getOpeningBookBuildOrder() const;
		const MetaPairVector getBuildOrderGoal();
		BuildOrderItem::SeedPositionStrategy getBuildSeedPositionStrategy(MetaType type);
		int getUnitLimit(MetaType type);
		double weightByFrame(double max_weight, int early = 7200); //7200 = 5분

		bool hasTech(BWAPI::TechType tech);
		Strategy::main_strategies getMainStrategy();
		BWAPI::Position getPositionForDefenceChokePoint(BWTA::Chokepoint * chokepoint, BWAPI::UnitType unit);

		bool firstChokeBunker;
		void liftBarrackFromWall();
		void liftAndMoveBarrackFromWall();
		BWAPI::UnitType getUnitTypeFromMainStrategy();
	};
}
