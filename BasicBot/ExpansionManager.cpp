#include "ExpansionManager.h"

using namespace MyBot;

ExpansionManager::ExpansionManager()
	: startPositionDestroyed(false)
{

}

ExpansionManager & ExpansionManager::Instance()
{
	static ExpansionManager instance;
	return instance;
}
void ExpansionManager::onSendText(std::string text){

}

const std::vector<Expansion> & ExpansionManager::getExpansions(){
	return expansions;
}

// 유닛이 파괴/사망한 경우, 해당 유닛 정보를 삭제한다
void ExpansionManager::onUnitDestroy(BWAPI::Unit unit)
{
	if (unit->getPlayer() == BWAPI::Broodwar->self())
	{
		//본진 및 멀티 커맨드센터 관리
		if (unit->getType() == BWAPI::UnitTypes::Terran_Command_Center){
			for (size_t i = 0; i < expansions.size(); i++){
				if (unit->getID() == expansions[i].cc->getID()){
					expansions.erase(expansions.begin() + i);
					WorkerManager::Instance().getWorkerData().removeDepot(unit);
					
					if (i == 0){
						startPositionDestroyed = true;
						//std::cout << "Self start location is destroyed " << std::endl;
					}

					//std::cout << "onUnitDestroy numExpansion:" << expansions.size() << std::endl;
					break;
				}
			}
		}

		//아군 건물은 혼잡도 계산을 한다. (단, 커맨드센터 파괴시에는 혼잡도 자체가 삭제되므로 뺀다.)
		if (unit->getType().isBuilding()){
			changeComplexity(unit, false); //decrease complexity;
		}
	}
}

void ExpansionManager::onUnitComplete(BWAPI::Unit unit){
	if (!(unit->getHitPoints() > 0)) return;

	if (unit->getPlayer() == BWAPI::Broodwar->self()){
		if(unit->getType() == BWAPI::UnitTypes::Terran_Command_Center){
			//numExpansion는 본진 포함개수
			for (auto &unit_in_region : unit->getUnitsInRadius(400)){
				if (unit_in_region->getType() == BWAPI::UnitTypes::Resource_Mineral_Field){
					expansions.push_back(Expansion(unit));
					//std::cout << "onUnitComplete numExpansion:" << expansions.size() << std::endl;
					break;
				}
			}

			for (auto e : expansions){
				//std::cout << "current expansion:" << e.cc->getPosition() << "/" << e.complexity << std::endl;
			}
		}
		
		//아군 건물은 혼잡도 계산을 한다.
		if(unit->getType().isBuilding()){
			changeComplexity(unit); //increase complexity;
		}
	}
}

void ExpansionManager::update(){
	if (!StrategyManager::Instance().isInitialBuildOrderFinished){
		return;
	}

	//1초에 4번
	if (TimeManager::Instance().isMyTurn("ExpansionManager_update", 6)) {
		BWAPI::Unit target;

		for (auto &e : expansions){
			bool enemyExists = false;
			bool refineryExists = true;
			bool comsatExists = false;

			if (BuildManager::Instance().hasAddon(e.cc)){
				comsatExists = true;
			}

			for (auto u : e.cc->getUnitsInRadius(200)){
				if (u->getType() == BWAPI::UnitTypes::Resource_Vespene_Geyser){
					target = u;
					refineryExists = false;
				}
				else if (u->getPlayer() == InformationManager::Instance().enemyPlayer){
					enemyExists = true;
					break;
				}
			}

			//이미 큐에 있으면 제외함
			//컴셋은 아카데미 필요함
			//빌드시작하여 컨스트럭트 큐에 있는지도 확인해야됨
			if (!enemyExists){
				if (!refineryExists){
					if (!BuildManager::Instance().hasUnitInQueue(BWAPI::UnitType(BWAPI::UnitTypes::Terran_Refinery)) &&
						(ConstructionManager::Instance().getConstructionQueueItemCount(BWAPI::UnitTypes::Terran_Refinery) == 0) &&
						UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Command_Center) > UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Refinery) &&
						(StrategyManager::Instance().getMainStrategy() != Strategy::BBS && StrategyManager::Instance().getMainStrategy() != Strategy::BSB)){
						BuildManager::Instance().addBuildOrderOneItem(MetaType(BWAPI::UnitTypes::Terran_Refinery), target->getInitialTilePosition());
					}
				}
				if (!comsatExists){

					if ((BWAPI::Broodwar->self()->completedUnitCount(BWAPI::UnitTypes::Terran_Academy) > 0) &&
						!BuildManager::Instance().hasUnitInQueue(BWAPI::UnitType(BWAPI::UnitTypes::Terran_Comsat_Station)) &&
							BWAPI::Broodwar->self()->gatheredGas() > 0){
						BuildManager::Instance().addBuildOrderOneItem(MetaType(BWAPI::UnitTypes::Terran_Comsat_Station));
					}	
				}
			}
		}
	}
}


int ExpansionManager::shouldExpandNow()
{
	//@도주남 김유진 현재 커맨드센터 지어지고 있으면 그 때동안은 멀티 추가 안함
	if (BuildManager::Instance().hasUnitInQueue(BWAPI::UnitTypes::Terran_Command_Center) ||
		ConstructionManager::Instance().getConstructionQueueItemCount(BWAPI::UnitTypes::Terran_Command_Center) > 0){
		return 0;
	}

	//일꾼이 너무 많으면 멀티 안까도록
	//if (UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_SCV) > 70){
	//	return 0;
	//}


	//상대방이 멀티 숫자가 더 많은 경우(우리 멀티 숫자가 적은 경우에만 적용한다.)
	//적이 입구 막은 경우
	if (expansions.size() < 3){
		if (enemyResourceRegions.size() > expansions.size()){
			return 2;
		}

		if (InformationManager::Instance().isBlockedEnemyChoke()){
			return 2;
		}
		////std::cout << "add expansions(less than enemy expansions)" << std::endl;
	}

	//일꾼이 남는경우
	//if (WorkerManager::Instance().getNumIdleWorkers() / (float)WorkerManager::Instance().getNumMineralWorkers() > 0.5)
	//{
	//	//std::cout << "add expansions(enough workers)" << std::endl;
	//	return true;
	//}

	// 현재 큐에 있는거 만들고도 남는 미네랄이 많다면... 빌드매니저에서 사용하는 여유자원소비기준(현재는 400)
	BuildManager &tmpObj = BuildManager::Instance();
	if ((tmpObj.getAvailableMinerals() - tmpObj.getQueueResource().first) > tmpObj.marginResource.first)
	{
		////std::cout << "add expansions(enough minerals)" << std::endl;
		return 1;
	}

	/*
	int minute = BWAPI::Broodwar->getFrameCount() / (24 * 60);
	int numExpansions = ExpansionManager::Instance().expansions.size();

	std::vector<int> expansionTimes = { 5, 7, 13, 20, 40, 50 };

	for (size_t i(0); i < expansionTimes.size(); ++i){
		if (numExpansions < (i + 2) && minute > expansionTimes[i]){
			//std::cout << "add expansions(time limit)" << std::endl;
			return true;
		}
	}
	*/

	return 0;
}

void ExpansionManager::changeComplexity(BWAPI::Unit unit, bool isAdd){
	Expansion *e = getExpansion(unit);
	if (e != NULL){
		BWTA::Region *expansion_r = BWTA::getRegion(e->cc->getPosition());
		
		if (e->complexity > 0.2)
			//std::cout << "expansion " << e->cc->getID() << " compexity : " << e->complexity << " -> ";


		if (isAdd)
			e->complexity += (unit->getType().width() * unit->getType().height()) / expansion_r->getPolygon().getArea();
		else
			e->complexity -= (unit->getType().width() * unit->getType().height()) / expansion_r->getPolygon().getArea();

		//if (e->complexity > 0.2)
			//std::cout << e->complexity << std::endl;
	}
}

Expansion * ExpansionManager::getExpansion(BWAPI::Unit u){
	Expansion *tmpRst = NULL;

	for (unsigned i = 0; i < expansions.size(); i++){
		BWTA::Region *expansion_r = BWTA::getRegion(expansions[i].cc->getPosition());

		if (expansion_r->getPolygon().isInside(u->getPosition())){
			tmpRst = &expansions[i];
		}
	}

	return tmpRst;
}

Expansion::Expansion(){
	cc = nullptr;
	complexity = 0.0;
}

Expansion::Expansion(BWAPI::Unit u){
	cc = u;
	complexity = 0.0;
}

bool Expansion::isValid(){
	if (cc != nullptr){
		return true;
	}

	return false;
}
