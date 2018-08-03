#pragma once

#include "Common.h"
#include <BWAPI.h>
#include "UABAssert.h"

namespace MyBot
{
	namespace Micro
	{
		void SmartAttackUnit(BWAPI::Unit attacker, BWAPI::Unit target);
		void SmartAttackMove(BWAPI::Unit attacker, const BWAPI::Position & targetPosition);
		void SmartAttackMove2(BWAPI::Unit attacker, BWAPI::Position oderCenterPosition, const BWAPI::Position & targetPosition);
		void SmartMove(BWAPI::Unit attacker, const BWAPI::Position & targetPosition);
		void SmartRightClick(BWAPI::Unit unit, BWAPI::Unit target);
		void SmartLaySpiderMine(BWAPI::Unit unit, BWAPI::Position pos);
		void SmartRepair(BWAPI::Unit unit, BWAPI::Unit target);
		void SmartKiteTarget(BWAPI::Unit rangedUnit, BWAPI::Unit target);
		void MutaDanceTarget(BWAPI::Unit muta, BWAPI::Unit target);
		BWAPI::Position GetKiteVector(BWAPI::Unit unit, BWAPI::Unit target);
		BWAPI::Position GetKiteVectorToPosition(BWAPI::Unit unit, BWAPI::Position targetPosition);

		void Rotate(double &x, double &y, double angle);
		void Normalize(double &x, double &y);

		void drawAPM(int x, int y);
	};
}