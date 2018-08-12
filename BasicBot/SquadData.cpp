#include "SquadData.h"

using namespace MyBot;

SquadData::SquadData() 
{
	
}

void SquadData::update()
{	
	updateAllSquads();	
	verifySquadUniqueMembership();	
}

void SquadData::clearSquadData()
{
    // give back workers who were in squads
    for (auto & kv : _squads)
	{
        Squad & squad = kv.second;

        const BWAPI::Unitset & units = squad.getUnits();

        for (auto & unit : units)
        {
            if (unit->getType().isWorker())
            {
                WorkerManager::Instance().finishedWithWorker(unit);
            }
        }
	}

	_squads.clear();
}

void SquadData::removeSquad(const std::string & squadName)
{
    auto & squadPtr = _squads.find(squadName);

    UAB_ASSERT_WARNING(squadPtr != _squads.end(), "Trying to clear a squad that didn't exist: %s", squadName.c_str());
    if (squadPtr == _squads.end())
    {
        return;
    }

    for (auto & unit : squadPtr->second.getUnits())
    {
        if (unit->getType().isWorker())
        {
            WorkerManager::Instance().finishedWithWorker(unit);
        }
    }

    _squads.erase(squadName);
}

const std::map<std::string, Squad> & SquadData::getSquads() const
{
    return _squads;
}

bool SquadData::squadExists(const std::string & squadName)
{
    return _squads.find(squadName) != _squads.end();
}

void SquadData::addSquad(const std::string & squadName, const Squad & squad)
{
	_squads[squadName] = squad;
}

void SquadData::updateAllSquads()
{
	for (auto & kv : _squads)
	{
		//std::cout << kv.second.getName() <<  " start " << std::endl;
		kv.second.update();
		//std::cout << kv.second.getName() << " done " << std::endl; // idle
	}
}

void SquadData::drawSquadInformation(int x, int y) 
{
    //if (!Config::Debug::DrawSquadInfo)
    //{
    //    return;
    //}

	if (Config::Debug::Draw) BWAPI::Broodwar->drawTextScreen(x, y, "\x04Squads");
	if (Config::Debug::Draw) BWAPI::Broodwar->drawTextScreen(x, y+20, "\x04NAME");
	if (Config::Debug::Draw) BWAPI::Broodwar->drawTextScreen(x+150, y+20, "\x04SIZE");
	if (Config::Debug::Draw) BWAPI::Broodwar->drawTextScreen(x+200, y+20, "\x04LOCATION");

	int yspace = 0;

	for (auto & kv : _squads) 
	{
        const Squad & squad = kv.second;

		const BWAPI::Unitset & units = squad.getUnits();
		const SquadOrder & order = squad.getSquadOrder();

		if (Config::Debug::Draw) BWAPI::Broodwar->drawTextScreen(x, y+40+((yspace)*10), "\x03%s", squad.getName().c_str());
		if (Config::Debug::Draw) BWAPI::Broodwar->drawTextScreen(x+150, y+40+((yspace)*10), "\x03%d", units.size());
		if (Config::Debug::Draw) BWAPI::Broodwar->drawTextScreen(x+200, y+40+((yspace++)*10), "\x03(%d,%d)", order.getPosition().x, order.getPosition().y);

		if (Config::Debug::Draw) BWAPI::Broodwar->drawCircleMap(order.getPosition(), 10, BWAPI::Colors::Green, true);
        if (Config::Debug::Draw) BWAPI::Broodwar->drawCircleMap(order.getPosition(), order.getRadius(), BWAPI::Colors::Red, false);
        if (Config::Debug::Draw) BWAPI::Broodwar->drawTextMap(order.getPosition() + BWAPI::Position(0, 20), "%s", squad.getName().c_str());

        for (const BWAPI::Unit unit : units)
        {
            if (Config::Debug::Draw) BWAPI::Broodwar->drawTextMap(unit->getPosition() + BWAPI::Position(0, 30), "%s", squad.getName().c_str());
        }
	}
}

void SquadData::verifySquadUniqueMembership()
{
    BWAPI::Unitset assigned;

    for (const auto & kv : _squads)
    {
        for (auto & unit : kv.second.getUnits())
        {
            if (assigned.contains(unit))
            {
                BWAPI::Broodwar->printf("Unit is in at least two squads: %s", unit->getType().getName().c_str());
            }

            assigned.insert(unit);
        }
    }
}

bool SquadData::unitIsInSquad(BWAPI::Unit unit) const
{
    return getUnitSquad(unit) != nullptr;
}

const Squad * SquadData::getUnitSquad(BWAPI::Unit unit) const
{
    for (const auto & kv : _squads)
    {
        if (kv.second.getUnits().contains(unit))
        {
            return &kv.second;
        }
    }

    return nullptr;
}

Squad * SquadData::getUnitSquad(BWAPI::Unit unit)
{
    for (auto & kv : _squads)
    {
        if (kv.second.getUnits().contains(unit))
        {
            return &kv.second;
        }
    }

    return nullptr;
}

void SquadData::assignUnitToSquad(BWAPI::Unit unit, Squad & squad)
{
    UAB_ASSERT_WARNING(canAssignUnitToSquad(unit, squad), "We shouldn't be re-assigning this unit!");

    Squad * previousSquad = getUnitSquad(unit);

    if (previousSquad)
    {
        previousSquad->removeUnit(unit);
    }

    squad.addUnit(unit);
}

bool SquadData::canAssignUnitToSquad(BWAPI::Unit unit, const Squad & squad) const
{
    const Squad * unitSquad = getUnitSquad(unit);

	if (unitSquad && unitSquad->getName() == squad.getName()) return false;

    // make sure strictly less than so we don't reassign to the same squad etc
	return !unitSquad || (unitSquad->getPriority() <= squad.getPriority());
}

Squad & SquadData::getSquad(const std::string & squadName)
{
    UAB_ASSERT_WARNING(squadExists(squadName), "Trying to access squad that doesn't exist: %s", squadName);
    if (!squadExists(squadName))
    {
        int a = 10;
    }

    return _squads[squadName];
}