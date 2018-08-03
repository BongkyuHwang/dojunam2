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
		if (unit->getHitPoints() < unit->getType().maxHitPoints())
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
        // otherwise we didn't find a medic which means they're all in use so break
        else
        {
            break;
        }
    }
	
    // the remaining medics should head to the squad order position
    for (auto & medic : availableMedics)
    {
		//@도주남 김지훈 노는 메딕을 마린혹은 파벳 중심으로 보내준다.  아 안되겠다 싶으면 본진쪽 초크포인트로 돌아온다
		if (order.getOrganicUnits().size() == 0)
		{			
			//보류
			//if (InformationManager::combatStatus::wSecondChokePoint <= InformationManager::Instance().nowCombatStatus)
			//	Micro::SmartMove(medic, InformationManager::Instance().getSecondChokePoint(BWAPI::Broodwar->self())->getCenter());
			//else if (InformationManager::combatStatus::wFirstChokePoint <= InformationManager::Instance().nowCombatStatus)
			//	Micro::SmartMove(medic, InformationManager::Instance().getFirstChokePoint(BWAPI::Broodwar->self())->getCenter());
			//else
			//	Micro::SmartMove(medic, InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->self())->getPosition());
		}
		else
		{
			if (medic->getDistance(order.getPosition()) > 100)
			//{
			//	if (!medic->isHoldingPosition())
			//	{
			//		medic->holdPosition();
			//		//BWAPI::Broodwar->drawTextMap(medic->getPosition() + BWAPI::Position(0, 30), "%s", "Hold on Position ");
			//	}
			//}
			//else
			{
				Micro::SmartMove(medic, order.getPosition());
			}
		}
    }
}