#pragma once

#include "Common.h"
//#include "Squad.h"
#include "SquadData.h"
#include "InformationManager.h"
#include "StrategyManager.h"

namespace MyBot
{
class CombatCommander
{
	SquadData       _squadData;
    BWAPI::Unitset  _combatUnits;
	BWAPI::Unitset  _dropUnits;
    bool            _initialized;
	

    void            updateScoutDefenseSquad();
	void            updateDefenseSquads();
	void            updateAttackSquads();
    void            updateDropSquads();
	void            updateScoutSquads();
	void            updateBunkertSquads();
	void            updateIdleSquad();
	bool            isSquadUpdateFrame();
	int             getNumType(BWAPI::Unitset & units, BWAPI::UnitType type);

	BWAPI::Unit     findClosestDefender(const Squad & defenseSquad, BWAPI::Position pos, bool flyingDefender);
    BWAPI::Unit     findClosestWorkerToTarget(BWAPI::Unitset & unitsToAssign, BWAPI::Unit target);

	BWAPI::Position getIdleSquadLastOrderLocation();
	BWAPI::Position getDefendLocation();
    BWAPI::Position getMainAttackLocation();
	BWAPI::Position getPositionForDefenceChokePoint(BWTA::Chokepoint * chokepoint);
	
	//@도주남 김지훈
	BWAPI::Position getMainAttackLocationForCombat();
	BWAPI::Position getFirstChokePoint_OrderPosition();
	BWAPI::Position getPoint_DEFCON4();
	void			updateSmallAttackSquad();
	void            saveAllSquadSecondChokePoint();
	int				indexFirstChokePoint_OrderPosition;
	std::vector<BWAPI::Position> firstChokePoint_OrderPositionPath;
	bool			initMainAttackPath;
	std::vector<BWAPI::Position> mainAttackPath;
	int				curIndex;

    void            initializeSquads();
    void            verifySquadUniqueMembership();
    void            assignFlyingDefender(Squad & squad);
    void            emptySquad(Squad & squad, BWAPI::Unitset & unitsToAssign);
    int             getNumGroundDefendersInSquad(Squad & squad);
    int             getNumAirDefendersInSquad(Squad & squad);

    void            updateDefenseSquadUnits(Squad & defenseSquad, const size_t & flyingDefendersNeeded, const size_t & groundDefendersNeeded);
    int             defendWithWorkers();

    int             numZerglingsInOurBase();
    bool            beingBuildingRushed();
public:
	void            updateComBatStatus(const BWAPI::Unitset & combatUnits);
	BWAPI::Position rDefence_OrderPosition;
	BWAPI::Position wFirstChokePoint_OrderPosition;
	static CombatCommander &	Instance();
	CombatCommander();

	void update();
    
	void drawSquadInformation(int x, int y);

	std::vector<BWTA::Region *> defenceRegions;
	std::map<BWTA::Region *, BWAPI::Unitset> unitNumInDefenceRegion;

	void supplementSquad();

	bool notDraw;

	//에러위치 찾는 용도
	std::ofstream log_file;
	std::string log_file_path;

	template<typename T>
	void log_write(T s, bool end = false){
		//if (!Config::Debug::createTrackingLog) return;
		return;
		log_file.open(log_file_path, std::ofstream::out | std::ofstream::app);

		if (log_file.is_open()){
			log_file << s;
			if (end) log_file << std::endl;
		}
		log_file.close();
	}

};
}
