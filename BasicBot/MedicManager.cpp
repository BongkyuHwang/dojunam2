#include "MedicManager.h"
#include "CommandUtil.h"

using namespace MyBot;

MedicManager::MedicManager() 
{ 
}

void MedicManager::executeMicro(const BWAPI::Unitset & targets) 
{
	const BWAPI::Unitset & medics = getUnits();

	// create a set of all medic targets
	BWAPI::Unitset medicTargets;	
	
	for (auto & unit : order.getOrganicUnits())
    {		
		if (unit->exists() && unit->getHitPoints() < unit->getType().maxHitPoints())
		{
            medicTargets.insert(unit);
        }
    }
	
	BWAPI::Unitset availableMedics(medics);

    // for each target, send the closest medic to heal it
    for (auto & target : medicTargets)
    {
		bool goHome = false;
		if (InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition().getDistance(target->getPosition())
		> InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition().getDistance(order.getPosition()) - order.getRadius()
		+ BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange() * 1.2
		+ BWAPI::UnitTypes::Terran_Firebat.groundWeapon().maxRange()
		+ BWAPI::UnitTypes::Terran_Vulture.groundWeapon().maxRange())
			goHome = true;

		if (goHome)
			continue;

        // only one medic can heal a target at a time
		if (target->isBeingHealed() )//&& countCB > 0)
        {
            continue;
        }

        double closestMedicDist = std::numeric_limits<double>::infinity();
        BWAPI::Unit closestMedic = nullptr;
		
        for (auto & medic : availableMedics)
        {
            double dist = medic->getDistance(target);
            if (!closestMedic || (dist < closestMedicDist))
            {
                closestMedic = medic;
                closestMedicDist = dist;
            }
        }

        // if we found a medic, send it to heal the target
        if (closestMedic)
        {
            closestMedic->useTech(BWAPI::TechTypes::Healing, target);
            availableMedics.erase(closestMedic);
        }
        // otherwise we didn't find a medic which means they're all in use so break
        else
        {
            break;
        }
    }
	
    // the remaining medics should head to the squad order position
    for (auto & medic : availableMedics)
    {
		bool goHome = false;
		if (InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition().getDistance(medic->getPosition())
			> InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition().getDistance(order.getPosition()) - order.getRadius()
			+ BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().maxRange() * 1.2
			+ BWAPI::UnitTypes::Terran_Firebat.groundWeapon().maxRange()
			+ BWAPI::UnitTypes::Terran_Vulture.groundWeapon().maxRange())
			goHome = true;
		if (order.getType() == SquadOrderTypes::Defend || order.getType() == SquadOrderTypes::Drop)
			goHome = false;
		//@도주남 김지훈 노는 메딕을 마린혹은 파벳 중심으로 보내준다.  아 안되겠다 싶으면 본진쪽 초크포인트로 돌아온다
		if (goHome)
		{			
			if (order.getCenterPosition().isValid())
			{
				Micro::SmartMove(medic, order.getCenterPosition());
			}
			else
				Micro::SmartMove(medic, InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition());
			continue;
		}
		else if (order.getType() != SquadOrderTypes::Drop)
		{
			if (!(BWTA::getRegion(BWAPI::TilePosition(medic->getPosition()))
				== BWTA::getRegion(InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy())->getTilePosition())
				|| BWTA::getRegion(BWAPI::TilePosition(medic->getPosition()))
				!= BWTA::getRegion(InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getTilePosition())))
			{
				if (medic->getDistance(order.getPosition()) > order.getRadius())
				{
					// move to it
					Micro::SmartMove(medic, order.getPosition());
				}
			}

		}
    }
}