#pragma once
#include "Common.h"
#include "WorkerManager.h"

namespace MyBot
{
	class Expansion{
	public:
		BWAPI::Unit cc;
		double complexity;
		Expansion();
		Expansion(BWAPI::Unit u);
		bool isValid();
	};

	class ExpansionManager{
		ExpansionManager();
		std::vector<Expansion> expansions;
		void changeComplexity(BWAPI::Unit unit, bool isAdd=true);
		bool startPositionDestroyed;

	public:
		static ExpansionManager & Instance();
		void onSendText(std::string text);
		void update();

		const std::vector<Expansion> & getExpansions();
		Expansion * getExpansion(BWAPI::Unit u);
		void ExpansionManager::onUnitDestroy(BWAPI::Unit unit);
		void ExpansionManager::onUnitComplete(BWAPI::Unit unit);
		int shouldExpandNow();

		std::set<BWTA::Region *> enemyResourceRegions;
	};
}
