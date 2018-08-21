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
        else
        {
            break;
        }
    }
	
    // the remaining medics should head to the squad order position
    for (auto & medic : availableMedics)
    {
		if (order.getType() != SquadOrderTypes::Drop)
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
			continue;
		}

		//@도주남 김지훈 노는 메딕을 마린혹은 파벳 중심으로 보내준다.  아 안되겠다 싶으면 본진쪽 초크포인트로 돌아온다
		//if (goHome)
		{			
			if (order.getCenterPosition().isValid())
			{
				Micro::SmartMove(medic, order.getCenterPosition());
			}
			else if (order.getPosition().getDistance(medic->getPosition()) > order.getRadius() - BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode.groundWeapon().minRange() )
				Micro::SmartMove(medic, order.getPosition());
			continue;
		}		
    }
}