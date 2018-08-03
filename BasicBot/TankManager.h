#pragma once;

#include "Common.h"
#include "MicroManager.h"

namespace MyBot
{
class TankManager : public MicroManager
{
public:

	TankManager();
	void executeMicro(const BWAPI::Unitset & targets);

	BWAPI::Unit chooseTarget(BWAPI::Unit rangedUnit, const BWAPI::Unitset & targets, std::map<BWAPI::Unit, int> & numTargeting);
	BWAPI::Unit closestrangedUnit(BWAPI::Unit target, std::set<BWAPI::Unit> & rangedUnitsToAssign);
	BWAPI::Unit closestrangedUnit_kjh(BWAPI::Unit target, BWAPI::Unitset & rangedUnitsToAssign);


	int getAttackPriority(BWAPI::Unit rangedUnit, BWAPI::Unit target);
	BWAPI::Unit getTarget(BWAPI::Unit rangedUnit, const BWAPI::Unitset & targets);
	BWAPI::Unit cadidateRemoveTarget;
};
}