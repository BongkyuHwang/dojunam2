#include "Micro.h"
#include "CommandUtil.h"
#include "MapTools.h"

using namespace MyBot;

size_t TotalCommands = 0;

const int dotRadius = 2;

void Micro::drawAPM(int x, int y)
{
	int bwapiAPM = BWAPI::Broodwar->getAPM();
	int myAPM = (int)(TotalCommands / ((double)BWAPI::Broodwar->getFrameCount() / (24 * 60)));
	if (Config::Debug::Draw) BWAPI::Broodwar->drawTextScreen(x, y, "%d %d", bwapiAPM, myAPM);
}

void Micro::SmartAttackUnit(BWAPI::Unit attacker, BWAPI::Unit target)
{
	UAB_ASSERT(attacker, "SmartAttackUnit: Attacker not valid");
	UAB_ASSERT(target, "SmartAttackUnit: Target not valid");

	if (!attacker || !target)
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (attacker->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount() || attacker->isAttackFrame())
	{
		return;
	}

	// get the unit's current command
	BWAPI::UnitCommand currentCommand(attacker->getLastCommand());

	// if we've already told this unit to attack this target, ignore this command
	if (currentCommand.getType() == BWAPI::UnitCommandTypes::Attack_Unit &&	currentCommand.getTarget() == target)
	{
		return;
	}

	// if nothing prevents it, attack the target
	attacker->attack(target);
	TotalCommands++;

	if (Config::Debug::DrawUnitTargetInfo)
	{
		if (Config::Debug::Draw) BWAPI::Broodwar->drawCircleMap(attacker->getPosition(), dotRadius, BWAPI::Colors::Red, true);
		if (Config::Debug::Draw) BWAPI::Broodwar->drawCircleMap(target->getPosition(), dotRadius, BWAPI::Colors::Red, true);
		if (Config::Debug::Draw) BWAPI::Broodwar->drawLineMap(attacker->getPosition(), target->getPosition(), BWAPI::Colors::Red);
	}
}

void Micro::SmartAttackMove(BWAPI::Unit attacker, const BWAPI::Position & targetPosition)
{
	//UAB_ASSERT(attacker, "SmartAttackMove: Attacker not valid");
	//UAB_ASSERT(targetPosition.isValid(), "SmartAttackMove: targetPosition not valid");

	if (!attacker || !targetPosition.isValid())
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (attacker->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount() || attacker->isAttackFrame())
	{
		return;
	}

	// get the unit's current command
	BWAPI::UnitCommand currentCommand(attacker->getLastCommand());

	// if we've already told this unit to attack this target, ignore this command
	if (currentCommand.getType() == BWAPI::UnitCommandTypes::Attack_Move &&	currentCommand.getTargetPosition() == targetPosition)
	{
		return;
	}

	// if nothing prevents it, attack the targ
	attacker->attack(targetPosition);
	TotalCommands++;

	if (Config::Debug::DrawUnitTargetInfo)
	{
		if (Config::Debug::Draw) BWAPI::Broodwar->drawCircleMap(attacker->getPosition(), dotRadius, BWAPI::Colors::Orange, true);
		if (Config::Debug::Draw) BWAPI::Broodwar->drawCircleMap(targetPosition, dotRadius, BWAPI::Colors::Orange, true);
		if (Config::Debug::Draw) BWAPI::Broodwar->drawLineMap(attacker->getPosition(), targetPosition, BWAPI::Colors::Orange);
	}
}

void Micro::SmartMove(BWAPI::Unit attacker, const BWAPI::Position & targetPosition)
{
	//UAB_ASSERT(attacker, "SmartAttackMove: Attacker not valid");
	//UAB_ASSERT(targetPosition.isValid(), "SmartAttackMove: targetPosition not valid");

	if (!attacker || !targetPosition.isValid())
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (attacker->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount() || attacker->isAttackFrame())
	{
		return;
	}

	// get the unit's current command
	BWAPI::UnitCommand currentCommand(attacker->getLastCommand());

	// if we've already told this unit to move to this position, ignore this command
	if ((currentCommand.getType() == BWAPI::UnitCommandTypes::Move) && (currentCommand.getTargetPosition() == targetPosition) && attacker->isMoving())
	{
		return;
	}

	//if (attacker->getDistance(targetPosition) < 30)
	//{
	//	if (attacker->canPatrol() && !attacker->isMoving())
	//	{
	//		attacker->patrol(targetPosition);
	//	}
	//	else
	//	{
	//		// if nothing prevents it, attack the target
	//	}
	//}
	//
	attacker->move(targetPosition);
	TotalCommands++;

	if (Config::Debug::DrawUnitTargetInfo)
	{
		if (Config::Debug::Draw) BWAPI::Broodwar->drawCircleMap(attacker->getPosition(), dotRadius, BWAPI::Colors::White, true);
		if (Config::Debug::Draw) BWAPI::Broodwar->drawCircleMap(targetPosition, dotRadius, BWAPI::Colors::White, true);
		if (Config::Debug::Draw) BWAPI::Broodwar->drawLineMap(attacker->getPosition(), targetPosition, BWAPI::Colors::White);
	}
}

void Micro::SmartRightClick(BWAPI::Unit unit, BWAPI::Unit target)
{
	UAB_ASSERT(unit, "SmartRightClick: Unit not valid");
	UAB_ASSERT(target, "SmartRightClick: Target not valid");

	if (!unit || !target)
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (unit->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount() || unit->isAttackFrame())
	{
		return;
	}

	// get the unit's current command
	BWAPI::UnitCommand currentCommand(unit->getLastCommand());

	// if we've already told this unit to move to this position, ignore this command
	if ((currentCommand.getType() == BWAPI::UnitCommandTypes::Right_Click_Unit) && (currentCommand.getTargetPosition() == target->getPosition()))
	{
		return;
	}

	// if nothing prevents it, attack the target
	unit->rightClick(target);
	TotalCommands++;

	if (Config::Debug::DrawUnitTargetInfo)
	{
		if (Config::Debug::Draw) BWAPI::Broodwar->drawCircleMap(unit->getPosition(), dotRadius, BWAPI::Colors::Cyan, true);
		if (Config::Debug::Draw) BWAPI::Broodwar->drawCircleMap(target->getPosition(), dotRadius, BWAPI::Colors::Cyan, true);
		if (Config::Debug::Draw) BWAPI::Broodwar->drawLineMap(unit->getPosition(), target->getPosition(), BWAPI::Colors::Cyan);
	}
}

void Micro::SmartLaySpiderMine(BWAPI::Unit unit, BWAPI::Position pos)
{
	if (!unit)
	{
		return;
	}


	if (!unit->canUseTech(BWAPI::TechTypes::Spider_Mines, pos))
	{
		//if (Config::Debug::Draw) BWAPI::Broodwar->drawTextMap(unit->getPosition() + BWAPI::Position(0, 50), "%s", "I can't mining");
		if (pos.isValid())		
			return;
	}

	if (BWAPI::Broodwar->getFrameCount() - unit->getLastCommandFrame() < 50)
	{
		return;
	}

	BWAPI::UnitCommand currentCommand(unit->getLastCommand());

	// if we've already told this unit to move to this position, ignore this command
	if ((currentCommand.getType() == BWAPI::UnitCommandTypes::Use_Tech_Position) && (currentCommand.getTargetPosition() == pos))// && unit->isMoving())
	{
		//if (Config::Debug::Draw) BWAPI::Broodwar->drawTextMap(unit->getPosition() + BWAPI::Position(0, 50), "%s", "I'm Going for mining");
		//if (Config::Debug::Draw) BWAPI::Broodwar->drawTextMap(unit->getPosition() + BWAPI::Position(0, 60), "%d", BWAPI::Broodwar->getFrameCount() - unit->getLastCommandFrame());
		int count = 0;
		if ((BWAPI::Broodwar->getFrameCount() - unit->getLastCommandFrame() > 100) && !unit->isMoving())
		{
			pos = pos + BWAPI::Position(20, 0);
			while (!pos.isValid())
			{
				pos = pos + BWAPI::Position(-1, 0);
				if (count == 6)
					pos = pos + BWAPI::Position(0, 7);
				if (count == 12)
					pos = pos + BWAPI::Position(0, -17);
				count++;
				if (count > 20)
				{
					unit->useTech(BWAPI::TechTypes::Spider_Mines, unit->getPosition());
					TotalCommands++;
					return;					
				}
			}
			unit->useTech(BWAPI::TechTypes::Spider_Mines, pos);
			TotalCommands++;
		}

		return;
	}

	{
		unit->useTech(BWAPI::TechTypes::Spider_Mines, pos);
		TotalCommands++;
	}

}

void Micro::SmartRepair(BWAPI::Unit unit, BWAPI::Unit target)
{
	UAB_ASSERT(unit, "SmartRightClick: Unit not valid");
	UAB_ASSERT(target, "SmartRightClick: Target not valid");

	if (!unit || !target)
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (unit->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount() || unit->isAttackFrame())
	{
		return;
	}

	// get the unit's current command
	BWAPI::UnitCommand currentCommand(unit->getLastCommand());

	// if we've already told this unit to move to this position, ignore this command
	if ((currentCommand.getType() == BWAPI::UnitCommandTypes::Repair) && (currentCommand.getTarget() == target))
	{
		return;
	}

	// if nothing prevents it, attack the target
	unit->repair(target);
	TotalCommands++;

	if (Config::Debug::DrawUnitTargetInfo)
	{
		if (Config::Debug::Draw) BWAPI::Broodwar->drawCircleMap(unit->getPosition(), dotRadius, BWAPI::Colors::Cyan, true);
		if (Config::Debug::Draw) BWAPI::Broodwar->drawCircleMap(target->getPosition(), dotRadius, BWAPI::Colors::Cyan, true);
		if (Config::Debug::Draw) BWAPI::Broodwar->drawLineMap(unit->getPosition(), target->getPosition(), BWAPI::Colors::Cyan);
	}
}


void Micro::SmartKiteTarget(BWAPI::Unit rangedUnit, BWAPI::Unit target)
{
	UAB_ASSERT(rangedUnit, "SmartKiteTarget: Unit not valid");
	UAB_ASSERT(target, "SmartKiteTarget: Target not valid");

	if (!rangedUnit || !target)
	{
		return;
	}

	double range(rangedUnit->getType().groundWeapon().maxRange());
	if (rangedUnit->getType() == BWAPI::UnitTypes::Protoss_Dragoon && BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Singularity_Charge))
	{
		range = 6 * 32;
	}

	// determine whether the target can be kited
	bool kiteLonger = Config::Micro::KiteLongerRangedUnits.find(rangedUnit->getType()) != Config::Micro::KiteLongerRangedUnits.end();
	if (!kiteLonger && (range <= target->getType().groundWeapon().maxRange()))
	{
		// if we can't kite it, there's no point
		Micro::SmartAttackUnit(rangedUnit, target);
		return;
	}

	bool    kite(true);
	double  dist(rangedUnit->getDistance(target));
	double  speed(rangedUnit->getType().topSpeed());


	// if the unit can't attack back don't kite
	if ((rangedUnit->isFlying() && !UnitUtils::CanAttackAir(target)) || (!rangedUnit->isFlying() && !UnitUtils::CanAttackGround(target)))
	{
		//kite = false;
	}

	double timeToEnter = std::max(0.0, (dist - range) / speed);
	if ((timeToEnter >= rangedUnit->getGroundWeaponCooldown()))
	{
		kite = false;
	}

	if (target->getType().isBuilding())
	{
		kite = false;
	}

	// if we can't shoot, run away
	if (kite)
	{
		BWAPI::Position fleePosition(rangedUnit->getPosition() - target->getPosition() + rangedUnit->getPosition());
		//if (Config::Debug::Draw) BWAPI::Broodwar->drawLineMap(rangedUnit->getPosition(), fleePosition, BWAPI::Colors::Cyan);

		//@µµÁÖ³² ±èÁöÈÆ
		if (!fleePosition.isValid())
		{
			BWAPI::Position ourBasePosition = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition();
			fleePosition = ourBasePosition;
		}
		rangedUnit->move(fleePosition);
		Micro::SmartMove(rangedUnit, fleePosition);
	}
	// otherwise shoot
	else
	{
		Micro::SmartAttackUnit(rangedUnit, target);
	}
}


void Micro::MutaDanceTarget(BWAPI::Unit muta, BWAPI::Unit target)
{
	UAB_ASSERT(muta, "MutaDanceTarget: Muta not valid");
	UAB_ASSERT(target, "MutaDanceTarget: Target not valid");

	if (!muta || !target)
	{
		return;
	}

	const int cooldown = muta->getType().groundWeapon().damageCooldown();
	const int latency = BWAPI::Broodwar->getLatency();
	const double speed = muta->getType().topSpeed();
	const double range = muta->getType().groundWeapon().maxRange();
	const double distanceToTarget = muta->getDistance(target);
	const double distanceToFiringRange = std::max(distanceToTarget - range, 0.0);
	const double timeToEnterFiringRange = distanceToFiringRange / speed;
	const int framesToAttack = static_cast<int>(timeToEnterFiringRange)+2 * latency;

	// How many frames are left before we can attack?
	const int currentCooldown = muta->isStartingAttack() ? cooldown : muta->getGroundWeaponCooldown();

	BWAPI::Position fleeVector = GetKiteVector(target, muta);
	BWAPI::Position moveToPosition(muta->getPosition() + fleeVector);

	//@µµÁÖ³² ±èÁöÈÆ
	BWAPI::Position ourBasePosition = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition();

	// If we can attack by the time we reach our firing range
	if (currentCooldown <= framesToAttack)
	{
		// Move towards and attack the target
		muta->attack(target);
	}
	else if (muta->getSpiderMineCount() > 0 && muta->canUseTech(BWAPI::TechTypes::Spider_Mines, muta->getPosition()))// Otherwise we cannot attack and should temporarily back off
	{
		Micro::SmartLaySpiderMine(muta, muta->getPosition());
	}
	else
	{
		// Determine direction to flee
		// Determine point to flee to		
		if (moveToPosition.isValid())
		{
			muta->rightClick(moveToPosition);
		}
		else
		{//@µµÁÖ³² ±èÁöÈÆ ¸¸¾à µÚ¿¡°¡ º®ÀÌ¸é °Á Áý ¹æÇâÀ¸·Î ¤¼¤¼
			muta->move(ourBasePosition);
			Micro::SmartMove(muta, ourBasePosition);
		}
	}
}

BWAPI::Position Micro::GetKiteVector(BWAPI::Unit unit, BWAPI::Unit target)
{
	BWAPI::Position fleeVec(target->getPosition() - unit->getPosition());
	double fleeAngle = atan2(fleeVec.y, fleeVec.x);
	fleeVec = BWAPI::Position(static_cast<int>(64 * cos(fleeAngle)), static_cast<int>(64 * sin(fleeAngle)));
	return fleeVec;
}

const double PI = 3.14159265;
void Micro::Rotate(double &x, double &y, double angle)
{
	angle = angle*PI / 180.0;
	x = (x * cos(angle)) - (y * sin(angle));
	y = (y * cos(angle)) + (x * sin(angle));
}

void Micro::Normalize(double &x, double &y)
{
	double length = sqrt((x * x) + (y * y));
	if (length != 0)
	{
		x = (x / length);
		y = (y / length);
	}
}

BWAPI::Position GetVectorToPosition(BWAPI::Unit unit, BWAPI::Position targetPosition)
{
	BWAPI::Position fleeVec(targetPosition - unit->getPosition());
	double fleeAngle = atan2(fleeVec.y, fleeVec.x);
	if (unit->getType() == BWAPI::UnitTypes::Terran_Firebat)
		fleeVec = BWAPI::Position(static_cast<int>(74 * cos(fleeAngle)), static_cast<int>(74 * sin(fleeAngle)));
	if (unit->getType() == BWAPI::UnitTypes::Terran_Marine)
		fleeVec = BWAPI::Position(static_cast<int>(64 * cos(fleeAngle)), static_cast<int>(64 * sin(fleeAngle)));
	return fleeVec;
}


void Micro::SmartAttackMove2(BWAPI::Unit attacker, BWAPI::Position orderCenterPosition, const BWAPI::Position & targetPosition)
{
	//UAB_ASSERT(attacker, "SmartAttackMove: Attacker not valid");
	//UAB_ASSERT(targetPosition.isValid(), "SmartAttackMove: targetPosition not valid");
	if (orderCenterPosition == BWAPI::Position(0, 0))
	{
		SmartAttackMove(attacker, targetPosition);
		return;
	}
	BWAPI::Position newTargetPosition(targetPosition + GetVectorToPosition(attacker, orderCenterPosition));


	if (!attacker || !newTargetPosition.isValid())
	{
		return;
	}

	// if we have issued a command to this unit already this frame, ignore this one
	if (attacker->getLastCommandFrame() >= BWAPI::Broodwar->getFrameCount() || attacker->isAttackFrame())
	{
		return;
	}

	// get the unit's current command
	BWAPI::UnitCommand currentCommand(attacker->getLastCommand());

	// if we've already told this unit to attack this target, ignore this command
	if (currentCommand.getType() == BWAPI::UnitCommandTypes::Attack_Move &&	currentCommand.getTargetPosition() == newTargetPosition)
	{
		return;
	}

	// if nothing prevents it, attack the target
	attacker->attack(newTargetPosition);
	TotalCommands++;

	if (Config::Debug::DrawUnitTargetInfo)
	{
		if (Config::Debug::Draw) BWAPI::Broodwar->drawCircleMap(attacker->getPosition(), dotRadius, BWAPI::Colors::Orange, true);
		if (Config::Debug::Draw) BWAPI::Broodwar->drawCircleMap(targetPosition, dotRadius, BWAPI::Colors::Orange, true);
		if (Config::Debug::Draw) BWAPI::Broodwar->drawLineMap(attacker->getPosition(), targetPosition, BWAPI::Colors::Orange);
	}
}
