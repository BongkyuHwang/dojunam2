#include "CommandUtil.h"

using namespace MyBot;

void CommandUtil::attackUnit(BWAPI::Unit attacker, BWAPI::Unit target)
{
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
}

void CommandUtil::attackMove(BWAPI::Unit attacker, const BWAPI::Position & targetPosition)
{
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

	// if nothing prevents it, attack the target
	attacker->attack(targetPosition);
}

void CommandUtil::move(BWAPI::Unit attacker, const BWAPI::Position & targetPosition)
{
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

	// if nothing prevents it, attack the target
	attacker->move(targetPosition);
}

void CommandUtil::rightClick(BWAPI::Unit unit, BWAPI::Unit target)
{
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
}

void CommandUtil::repair(BWAPI::Unit unit, BWAPI::Unit target)
{
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
}

bool UnitUtils::IsCombatUnit(BWAPI::Unit unit)
{
	if (!unit)
	{
		return false;
	}

	// no workers or buildings allowed
	if (unit && unit->getType().isWorker() || unit->getType().isBuilding())
	{
		return false;
	}

	// check for various types of combat units
	if (unit->getType().canAttack() ||
		unit->getType() == BWAPI::UnitTypes::Terran_Medic ||
		unit->getType() == BWAPI::UnitTypes::Protoss_High_Templar ||
		unit->getType() == BWAPI::UnitTypes::Protoss_Observer ||
		unit->isFlying() && unit->getType().spaceProvided() > 0)
	{
		return true;
	}

	return false;
}

bool UnitUtils::IsCombatUnit_rush(BWAPI::Unit unit)
{
	if (!unit)
	{
		return false;
	}

	// no workers or buildings allowed
	if (unit->getType().isWorker() || unit->getType().isBuilding())
	{
		return false;
	}

	// check for various types of combat units
	if (unit->getType().canAttack())
	{
		return true;
	}

	return false;
}

bool UnitUtils::IsValidUnit(BWAPI::Unit unit)
{
	if (!unit)
	{
		return false;
	}

	if (unit->isCompleted()
		&& unit->getHitPoints() > 0
		&& unit->exists()
		&& unit->getType() != BWAPI::UnitTypes::Unknown
		&& unit->getPosition().x != BWAPI::Positions::Unknown.x
		&& unit->getPosition().y != BWAPI::Positions::Unknown.y)
	{
		return true;
	}
	else
	{
		return false;
	}
}

double UnitUtils::GetDistanceBetweenTwoRectangles(Rect & rect1, Rect & rect2)
{
	Rect & mostLeft = rect1.x < rect2.x ? rect1 : rect2;
	Rect & mostRight = rect2.x < rect1.x ? rect1 : rect2;
	Rect & upper = rect1.y < rect2.y ? rect1 : rect2;
	Rect & lower = rect2.y < rect1.y ? rect1 : rect2;

	int diffX = std::max(0, mostLeft.x == mostRight.x ? 0 : mostRight.x - (mostLeft.x + mostLeft.width));
	int diffY = std::max(0, upper.y == lower.y ? 0 : lower.y - (upper.y + upper.height));

	return std::sqrtf(static_cast<float>(diffX*diffX + diffY*diffY));
}

bool UnitUtils::CanAttack(BWAPI::Unit attacker, BWAPI::Unit target)
{
	return GetWeapon(attacker, target) != BWAPI::UnitTypes::None; //반환타입 조심!!!! CanAttackAir과 다름
}

bool UnitUtils::CanAttackAir(BWAPI::Unit unit)
{
	return unit->getType().airWeapon() != BWAPI::WeaponTypes::None;
}

bool UnitUtils::CanAttackGround(BWAPI::Unit unit)
{
	return unit->getType().groundWeapon() != BWAPI::WeaponTypes::None;
}

double UnitUtils::CalculateLTD(BWAPI::Unit attacker, BWAPI::Unit target)
{
	BWAPI::WeaponType weapon = GetWeapon(attacker, target);

	if (weapon == BWAPI::WeaponTypes::None)
	{
		return 0;
	}

	return static_cast<double>(weapon.damageAmount()) / weapon.damageCooldown();
}

BWAPI::WeaponType UnitUtils::GetWeapon(BWAPI::Unit attacker, BWAPI::Unit target)
{
	return target->isFlying() ? attacker->getType().airWeapon() : attacker->getType().groundWeapon();
}

BWAPI::WeaponType UnitUtils::GetWeapon(BWAPI::UnitType attacker, BWAPI::UnitType target)
{
	return target.isFlyer() ? attacker.airWeapon() : attacker.groundWeapon();
}

int UnitUtils::GetAttackRange(BWAPI::Unit attacker, BWAPI::Unit target)
{
	BWAPI::WeaponType weapon = GetWeapon(attacker, target);

	if (weapon == BWAPI::WeaponTypes::None)
	{
		return 0;
	}

	int range = weapon.maxRange();

	if ((attacker->getType() == BWAPI::UnitTypes::Protoss_Dragoon)
		&& (attacker->getPlayer() == BWAPI::Broodwar->self())
		&& BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Singularity_Charge))
	{
		range = 6 * 32;
	}

	return range;
}

int UnitUtils::GetAttackRange(BWAPI::UnitType attacker, BWAPI::UnitType target)
{
	BWAPI::WeaponType weapon = GetWeapon(attacker, target);

	if (weapon == BWAPI::WeaponTypes::None)
	{
		return 0;
	}

	return weapon.maxRange();
}

size_t UnitUtils::GetAllUnitCount(BWAPI::UnitType type)
{
	size_t count = 0;
	for (const auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		// trivial case: unit which exists matches the type
		if (unit->getType() == type)
		{
			count++;
		}


		// case where a zerg egg contains the unit type
		//저글링은 2개씩 세는 로직
		if (unit->getType() == BWAPI::UnitTypes::Zerg_Egg && unit->getBuildType() == type)
		{
			count += type.isTwoUnitsInOneEgg() ? 2 : 1;
		}

		// case where a building has started constructing a unit but it doesn't yet have a unit associated with it
		// 트레이닝 중인 유닛모두 포함
		//if (unit->getRemainingTrainTime() > 0)
		//{
		//	BWAPI::UnitType trainType = unit->getLastCommand().getUnitType();

		//	if (trainType == type && unit->getRemainingTrainTime() == trainType.buildTime())
		//	{
		//		count++;
		//	}
		//}
	}

	return count;
}

// 전체 순차탐색을 하기 때문에 느리다
BWAPI::Unit UnitUtils::GetClosestUnitTypeToTarget(BWAPI::UnitType type, BWAPI::Position target)
{
	BWAPI::Unit closestUnit = nullptr;
	double closestDist = 100000000;

	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (unit->getType() == type)
		{
			double dist = unit->getDistance(target);
			if (!closestUnit || dist < closestDist)
			{
				closestUnit = unit;
				closestDist = dist;
			}
		}
	}

	return closestUnit;
}


void UnitUtils::getAllCloakUnits(BWAPI::Unitset &units){
	for (auto & unit : BWAPI::Broodwar->enemy()->getUnits()){
		if (unit->isVisible() && !unit->isDetected()){
			units.insert(unit);
		}
	}
}

// player : 주변에서 측정하고 싶은 LTD 유닛의 플레이어(주변 적정보를 얻고 싶다면 enemy)
// centerUnit : 측정에서 기준이 되는 유닛
// radius : 측정범위
// 데미지/쿨다운 : 마린(0.4), 시즈(0.81, 0.93)
double UnitUtils::getNearByLTD(BWAPI::Player player, BWAPI::Unit centerUnit, int radius){
	double totalLTD = 0.0;
	int distanceMargin = centerUnit->getType().width();
	for (auto &attacker : centerUnit->getUnitsInRadius(radius)){
		if (attacker->getPlayer() == player) {
			BWAPI::WeaponType weapon = UnitUtils::GetWeapon(attacker, centerUnit); //공격가능 여부 판단 
			int tmpDistance = attacker->getDistance(centerUnit); //거리 판단
			if (weapon != BWAPI::WeaponTypes::None && weapon != BWAPI::WeaponTypes::Unknown &&
				tmpDistance <= (weapon.maxRange() + distanceMargin))
			{
				totalLTD += (weapon.damageAmount() / (double)weapon.damageCooldown());
			}
		}
	}
	return totalLTD;
}

BWAPI::Unit UnitUtils::canIFight(BWAPI::Unit attacker)
{
	if (attacker == nullptr)
		return nullptr;
	int radius = attacker->getType().groundWeapon().maxRange();
	int enemyHP = 9999999;
	BWAPI::Unit attackedEnemy = nullptr;
	for (auto & unit : attacker->getUnitsInRadius(radius))
	{
		if (unit->getPlayer() != attacker->getPlayer() && unit->getHitPoints() > 0 && unit->getType().isWorker())
		{
			if (enemyHP > unit->getHitPoints())
			{
				enemyHP = unit->getHitPoints();
				attackedEnemy = unit;
			}
		}
		else
		{
			attackedEnemy = nullptr;
			break;
		}
	}
	
	//if (attackedEnemy == nullptr)
	//	return attackedEnemy;
	//if (attacker->getHitPoints() - CalculateLTD(attackedEnemy, attacker) <
	//	attackedEnemy->getHitPoints() - CalculateLTD(attacker, attackedEnemy))
	//{
	//	return nullptr;
	//}	
	//else
	return attackedEnemy;
}


BWAPI::Unit UnitUtils::GetFarUnitTypeToTarget(BWAPI::UnitType type, BWAPI::Position target)
{
	BWAPI::Unit farUnit = nullptr;
	double farDist = 0;

	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (unit->getType() == type)
		{
			double dist = unit->getDistance(target);
			if (!farUnit || dist > farDist)
			{
				farUnit = unit;
				farDist = dist;
			}
		}
	}

	return farUnit;
}