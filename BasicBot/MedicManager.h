#pragma once;

#include "Common.h"
#include "MicroManager.h"

namespace MyBot
{
class MedicManager : public MicroManager
{
public:
	//@도주남 김지훈 melee 유닛 중간에 노는 메딕을 보내기 위한 변수
	//BWAPI::Position meleeUnitsetCenterP;
	MedicManager();
	void executeMicro(const BWAPI::Unitset & targets);
};
}