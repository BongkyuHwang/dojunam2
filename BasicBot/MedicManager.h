#pragma once;

#include "Common.h"
#include "MicroManager.h"

namespace MyBot
{
class MedicManager : public MicroManager
{
public:
	//@���ֳ� ������ melee ���� �߰��� ��� �޵��� ������ ���� ����
	//BWAPI::Position meleeUnitsetCenterP;
	MedicManager();
	void executeMicro(const BWAPI::Unitset & targets);
};
}