#pragma once

#include "Common.h"
//#include "MapGrid.h"
#include "SquadOrder.h"
#include "MapTools.h"
#include "Micro.h"
#include "InformationManager.h"

namespace MyBot
{
struct AirThreat
{
	BWAPI::Unit	unit;
	double			weight;
};

struct GroundThreat
{
	BWAPI::Unit	unit;
	double			weight;
};

class MicroManager
{
	BWAPI::Unitset  _units;

protected:
	
	SquadOrder			order;

	virtual void        executeMicro(const BWAPI::Unitset & targets) = 0;
	bool                checkPositionWalkable(BWAPI::Position pos);
	void                drawOrderText();
	bool                unitNearEnemy(BWAPI::Unit unit);
	bool                unitNearChokepoint(BWAPI::Unit unit) const;
	void                trainSubUnits(BWAPI::Unit unit) const;
    

public:
						MicroManager();
    virtual				~MicroManager(){}

	const BWAPI::Unitset & getUnits() const;
	BWAPI::Position     calcCenter() const;

	void				setUnits(const BWAPI::Unitset & u);
	void				execute(const SquadOrder & order);
	void				regroup(const BWAPI::Position & regroupPosition) const;

};
}