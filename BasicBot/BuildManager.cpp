#include "BuildManager.h"

using namespace MyBot;

BuildManager::BuildManager() 
	: _enemyCloakedDetected (false),
	marginResource(std::make_pair(400, 0))
{
	setBuildOrder(StrategyManager::Instance().getOpeningBookBuildOrder());
}

void BuildManager::onStart(){
	if (InformationManager::Instance().getMapName() == 'H'){
		maxDepotWorkers = 10; //미네랄10덩어리
	}
	else if (InformationManager::Instance().getMapName() == 'F'){
		maxDepotWorkers = 9; //미네랄9덩어리
	}
	else{
		maxDepotWorkers = 8; //미네랄8덩어리
	}

}

// 빌드오더 큐에 있는 것에 대해 생산 / 건설 / 리서치 / 업그레이드를 실행한다
void BuildManager::update()
{
	/* 개요
	1. 빌드큐에서 우선순위 높은 빌드를 가지고 온다. 처리가능 하면(생산자, 미네랄, 위치 등) 처리. 처리 못하면 스킵 하거나 다음 프레임에서 처리
	1.1 건물의 경우는 건물큐로 일단 보냄(아주 간단한 정보만 확인 후). 컨스트럭트 매니저에서 데드락 관리
	2. 처리 못하는 경우는 미네랄이 부족한 경우만 있다라고 가정된 상황(필요한 건물등은 모두 만들어져 있다고 가정)
	3. 서플라이 부족한 경우는 데드락 발생하므로 예외처리 되어 있음
	4. (todo) 추가적으로 예외 상황에서 못만드는 경우가 있으므로 데드락 처리가 필요
	5. 빌드 큐가 모두 소진되면 BOSS를 통해 새로운 빌드오더 생성
	6. (todo) 디텍팅 유닛이 필요한 경우, 디텍 유닛 세팅 : 조금더 고민이 필요, 터렛 설치 장소 등
	7. (todo) 디펜스 상황에서 유닛을 빼거나 전투유닛만 만들어야 되는 상황
	*/

	/*
	// Dead Lock 을 체크해서 제거한다
	//checkBuildOrderQueueDeadlockAndAndFixIt();
	// Dead Lock 제거후 Empty 될 수 있다
	if (buildQueue.isEmpty()) {
		return;
	}
	*/

	checkErrorBuildOrderAndRemove(); //최상위 빌드1개 잘못된 빌드면 삭제 : 현재는 애드온이 다 붙어있는데 만드려고 하는것 삭제

	consumeBuildQueue(); //빌드오더 소비
	//큐가 비어 있으면 새로운 빌드오더 생성
	
	
	if (buildQueue.isEmpty()) {
		if ((buildQueue.size() == 0) && (BWAPI::Broodwar->getFrameCount() > 10))
		{
			BWAPI::Broodwar->drawTextScreen(150, 10, "Nothing left to build, new search!");

			performBuildOrderSearch();
		}
	}
	

	// 서플라이 데드락 체크
	// detect if there's a build order deadlock once per second
	if (TimeManager::Instance().isMyTurn("detectSupplyDeadlock", 24) && detectSupplyDeadlock())
	{
		//std::cout << "Supply deadlock detected, building supply!" << std::endl;

		buildQueue.queueAsHighestPriority(MetaType(BWAPI::UnitTypes::Terran_Supply_Depot), StrategyManager::Instance().getBuildSeedPositionStrategy(MetaType(BWAPI::UnitTypes::Terran_Supply_Depot)), true);
	}
	
	if (TimeManager::Instance().isMyTurn("defenceFlyingAndDetect", 12)) defenceFlyingAndDetect();

	checkBuildOrderQueueDeadlockAndRemove();

	//커맨드센터는 작업이 없으면 일꾼을 만든다.
	if (TimeManager::Instance().isMyTurn("executeWorkerTraining", 2)) executeWorkerTraining();

	//여유자원 소비 - 너무 짧은 주기로 하면 미네랄이 적은 친구들만 들어갈것이므로 적당한 주기로 한다.
	if (TimeManager::Instance().isMyTurn("consumeRemainingResource", 48)) consumeRemainingResource();
}

void BuildManager::consumeBuildQueue(){
	// the current item to be used
	if (buildQueue.isEmpty()) return;
		
	BuildOrderItem currentItem = buildQueue.getHighestPriorityItem();

	// while there is still something left in the buildQueue
	while (!buildQueue.isEmpty())
	{
		bool isOkToRemoveQueue = true;

		// v18 김유진
		// producer가 있어도 canMakeNow == false면 producer를 반환하지 않는 함수
		BWAPI::Unit producer = canMakeNowAndGetProducer(currentItem.metaType, BWAPI::Position(currentItem.seedLocation), currentItem.producerID);

		// if we can make the current item, create it
		if (producer != nullptr)
		{
			MetaType t = currentItem.metaType;

			if (t.isUnit())
			{
				if (t.getUnitType().isBuilding()) {
					// 테란 Addon 건물의 경우 (Addon 건물을 지을수 있는지는 getProducer 함수에서 이미 체크완료)
					// 모건물이 Addon 건물 짓기 전에는 canBuildAddon = true, isConstructing = false, canCommand = true 이다가 
					// Addon 건물을 짓기 시작하면 canBuildAddon = false, isConstructing = true, canCommand = true 가 되고 (Addon 건물 건설 취소는 가능하나 Train 등 커맨드는 불가능)
					// 완성되면 canBuildAddon = false, isConstructing = false 가 된다
					if (t.getUnitType().isAddon()) {
						producer->buildAddon(t.getUnitType());	

						//@도주남 애드온 못만들어도 그냥 지우기로(꼭 팩토리에 다 붙어 있을 필요는 없으므로)
						// 테란 Addon 건물의 경우 정상적으로 buildAddon 명령을 내려도 SCV가 모건물 근처에 있을 때 한동안 buildAddon 명령이 취소되는 경우가 있어서
						// 모건물이 isConstructing = true 상태로 바뀐 것을 확인한 후 buildQueue 에서 제거해야한다
						//if (producer->isConstructing() == false) {
						//	isOkToRemoveQueue = false;
						//}
					}
					// 그외 대부분 건물의 경우
					else
					{
						// ConstructionPlaceFinder 를 통해 건설 가능 위치 desiredPosition 를 알아내서
						// ConstructionManager 의 ConstructionTask Queue에 추가를 해서 desiredPosition 에 건설을 하게 한다. 
						// ConstructionManager 가 건설 도중에 해당 위치에 건설이 어려워지면 다시 ConstructionPlaceFinder 를 통해 건설 가능 위치를 desiredPosition 주위에서 찾을 것이다
						BWAPI::TilePosition desiredPosition = getDesiredPosition(t.getUnitType(), currentItem.seedLocation, currentItem.seedLocationStrategy);

						if (desiredPosition != BWAPI::TilePositions::None) {
							ConstructionManager::Instance().addConstructionTask(t.getUnitType(), desiredPosition);
						}
						else {
							// 건물 가능 위치가 없는 경우는, Protoss_Pylon 가 없거나, Creep 이 없거나, Refinery 가 이미 다 지어져있거나, 정말 지을 공간이 주위에 없는 경우인데,
							// 대부분의 경우 Pylon 이나 Hatchery가 지어지고 있는 중이므로, 다음 frame 에 건물 지을 공간을 다시 탐색하도록 한다. 
							//std::cout << "There is no place to construct " << currentItem.metaType.getUnitType().getName().c_str()
//								<< " strategy " << currentItem.seedLocationStrategy
//								<< " seedPosition " << currentItem.seedLocation.x << "," << currentItem.seedLocation.y
//								<< " desiredPosition " << desiredPosition.x << "," << desiredPosition.y << std::endl;

							isOkToRemoveQueue = false;
						}
					}
				}
				// 지상유닛 / 공중유닛의 경우
				else {
					producer->train(t.getUnitType());
				}
			}
			// if we're dealing with a tech research
			else if (t.isTech())
			{
				producer->research(t.getTechType());
			}
			else if (t.isUpgrade())
			{
				producer->upgrade(t.getUpgradeType());
			}

			// remove it from the buildQueue
			if (isOkToRemoveQueue) {
				buildQueue.removeCurrentItem();
			}

			// don't actually loop around in here
			// 한프레임에 빌드 한개 처리
			break;
		}

		// otherwise, if we can skip the current item
		// 그 빌드가 producer가 없거나 canmake가 안되면 스킵을 할수가 있음, 하지만 대부분 이미 BOSS를 통해 가능한 빌드가 들어온것이므로 미네랄 모일때까지 기다려야 하므로 블록킹 빌드로 쌓아야된다.
		// canSkipCurrentItem 에서 현재 빌드가 blocking 인지보고 스킵여부 결정함
		// 스킵해도 가져올 빌드가 없으면 스킵 불가
		// 현재 빌드가 블록킹 빌드이면 다음으로 진행 안함
		else if (buildQueue.canSkipCurrentItem())
		{
			// skip it and get the next one
			buildQueue.skipCurrentItem();
			currentItem = buildQueue.getNextItem();
		}
		else
		{
			//자원이 충분한데 처리가 안되면 일단 블로킹을 해제해서 자원수급을 원활하게 돌림 
			//v18 김유진
			//무조건 스킵처리 하면 계속 밀릴수가 있음, 그리고 가스만 부족하고 미네랄이 남는 경우가 있으므로 미네랄로만 할 수 있는 일을 먼저 할 수 있어야함
			//해결방안
			//스킵할때 스킵된 미네랄을 기록해서 현재 미네랄에 반영할 것인가?

			//일단 미네랄만 남는 경우, 하위빌드 중에서 1-미네랄only 2-미네랄충분 3-현재만들수있음 인 경우 
			if (currentItem.blocking){
				if (hasEnoughResources(currentItem.metaType)){
					buildQueue.setCurrentItemBlocking(false);
				}
				else if (currentItem.metaType.mineralPrice() <= getAvailableMinerals()){
					for (auto &n : buildQueue.viewNextItems()){
						if (n.metaType.gasPrice() > 0) 
							continue;

						if (n.metaType.mineralPrice() > getAvailableMinerals() - currentItem.metaType.mineralPrice())
							continue;

						BWAPI::Unit producer = getProducer(n.metaType, BWAPI::Position(n.seedLocation), n.producerID);

						if (producer != nullptr) {
							if (canMake(producer, n.metaType)){
								buildQueue.setCurrentItemBlocking(false);
							}
						}
					}
				}
			}

			break;
		}
	}
}

void BuildManager::performBuildOrderSearch()
{
	/*
	if (!Config::Modules::UsingBuildOrderSearch || !canPlanBuildOrderNow())
	{
		return;
	}
	*/
	////std::cout << "buildOrder.size():";
	BuildOrder & buildOrder = BOSSManager::Instance().getBuildOrder();
	////std::cout << buildOrder.size() << std::endl;

	if (buildOrder.size() > 0)
	{
		/*
		//std::cout << "Finished BOSS - ";
		for (size_t i(0); i < buildOrder.size(); ++i){
			if (buildOrder[i].getName().find("Terran_")==0)
				//std::cout << buildOrder[i].getName().replace(0, 7, "") << " ";
			else
				//std::cout << buildOrder[i].getName() << " ";
		}
		//std::cout << std::endl;
		*/
		setBuildOrder(buildOrder);
		BOSSManager::Instance().reset();
	}
	else
	{
		if (!BOSSManager::Instance().isSearchInProgress())
		{
			const std::vector<MetaPair> & goalUnits = StrategyManager::Instance().getBuildOrderGoal();
			if (goalUnits.empty()){
				return;
			}

			/*
			//std::cout << "Start BOSS" << std::endl;

			if (!buildQueue.isEmpty()){
				//std::cout << "Queue list - ";
				for (int i = buildQueue.size() - 1; i >= 0; i--){
					if (buildQueue[i].metaType.getName().find("Terran_") == 0)
						//std::cout << buildQueue[i].metaType.getName().replace(0, 7, "") << " ";
					else
						//std::cout << buildQueue[i].metaType.getName() << " ";
				}
				//std::cout << std::endl;
			}

			//std::cout << "Goal list - ";
			for (auto &i : goalUnits){
				if (i.first.getName().find("Terran_") == 0)
					//std::cout << i.first.getName().replace(0, 7, "") << "(" << i.second << ") ";
				else
					//std::cout << i.first.getName() << "(" << i.second << ") ";
			}
			//std::cout << std::endl;
			*/
			BOSSManager::Instance().startNewSearch(goalUnits);
		}
	}
}

BWAPI::Unit BuildManager::getProducer(MetaType t, BWAPI::Position closestTo, int producerID)
{
	// get the type of unit that builds this
	//지을수 있는 타입을 고른다. 시즈면 팩토리
	BWAPI::UnitType producerType = t.whatBuilds();

	// make a set of all candidate producers
	//위에서 찾은 타입(팩토리)을 내 유닛중에 있는지 본다. 
	BWAPI::Unitset candidateProducers;
	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		if (unit == nullptr) continue;

		// reasons a unit can not train the desired type
		if (unit->getType() != producerType)                    { continue; }
		if (!unit->exists())	                                { continue; }
		if (!unit->isCompleted())                               { continue; }
		if (unit->isTraining())                                 { continue; }
		if (unit->isResearching())                              { continue; }
		if (unit->isUpgrading())                                { continue; }
		// if unit is lifted, unit should land first
		if (unit->isLifted())                                   { continue; }

		//생산자를 선택한 경우, 그 생산자로 지정
		if (producerID != -1 && unit->getID() != producerID)	{ continue; }

		// if the type is an addon 
		// 애드온을 지으려면, 생산자에 이미 달려있는지, 애드온 짓는 지역이 비어 있는지 추가적으로 조사해야 됨
		if (t.getUnitType().isAddon())
		{
			/* 아주 예외사항
			1. (검증필요) 모건물은 건설되고 있는 중에는 isCompleted = false, isConstructing = true, canBuildAddon = false 이다가 건설이 완성된 후 몇 프레임동안은 isCompleted = true 이지만, canBuildAddon = false 인 경우가 있다
			2. if we just told this unit to build an addon, then it will not be building another one
			this deals with the frame-delay of telling a unit to build an addon and it actually starting to build
			빌드온 짓는 명령한지 10프레임이 안됐다면 제외, 딴 명령으로 바뀔수도 있나보다.
			*/
			//1. (검증필요) if (!unit->canBuildAddon()) { continue; }
			//2. 프레임 딜레이 및 애드온 가지고 있는지 판단
			if (hasAddon(unit)) continue;

			bool isBlocked = false;

			// if the unit doesn't have space to build an addon, it can't make one
			BWAPI::TilePosition addonPosition(
				unit->getTilePosition().x + unit->getType().tileWidth(),
				unit->getTilePosition().y + unit->getType().tileHeight() - t.getUnitType().tileHeight());

			for (int i = 0; i < t.getUnitType().tileWidth(); ++i)
			{
				for (int j = 0; j < t.getUnitType().tileHeight(); ++j)
				{
					BWAPI::TilePosition tilePos(addonPosition.x + i, addonPosition.y + j);

					// if the map won't let you build here, we can't build it.  
					// 맵 타일 자체가 건설 불가능한 타일인 경우 + 기존 건물이 해당 타일에 이미 있는경우
					if (!BWAPI::Broodwar->isBuildable(tilePos, true))
					{
						isBlocked = true;
					}

					// if there are any units on the addon tile, we can't build it
					// 아군 유닛은 Addon 지을 위치에 있어도 괜찮음. (적군 유닛은 Addon 지을 위치에 있으면 건설 안되는지는 아직 불확실함)
					BWAPI::Unitset uot = BWAPI::Broodwar->getUnitsOnTile(tilePos.x, tilePos.y);
					for (auto & u : uot) {
						if (u->getPlayer() != InformationManager::Instance().selfPlayer) {
							isBlocked = true;
							break;
						}
					}
				}
			}

			if (isBlocked)
			{
				continue;
			}
		}



		// if the type requires an addon and the producer doesn't have one
		//짓는 건물 외에 필요정보들을 가지고 와서 필요한 애드온 및 건물 확인
		//고스트 -> whatbuild는 배럭, requiredUnits은 아카데미와 코벌트옵스
		//타겟유닛(시즈)을 만드는데 필요한 추가 정보를 읽어와서 필요한 것이 애드온인 경우 애드온이 있는지 확인.
		//생산자와 애드온이 연결되어 잇어야 되는 경우와, 그냥 있기만 하면 되는 경우로 나뉨
		bool except_candidates = false;
		typedef std::pair<BWAPI::UnitType, int> ReqPair;
		for (const ReqPair & pair : t.getUnitType().requiredUnits())
		{
			BWAPI::UnitType requiredType = pair.first;
			//필요타입이 애드온 이면서 생산자와 연결되어 있어야 되는 경우(탱크, 드랍쉽, 사배 등)
			if (requiredType.isAddon() && requiredType.whatBuilds().first == unit->getType()){
				//생산자에 에드온이 없는 경우
				//생산자에 달린 애드온이 다른 애드온인 경우(테란에는 애드온이 2개씩 있는 건물이 있음)
				if (!unit->getAddon() || unit->getAddon() == nullptr || unit->getAddon()->getType() != requiredType){
					except_candidates = true;
					break;
				}
			}
		}

		if (except_candidates) continue;

		//애드온이 지어지고 있는 경우... 판단하는 api없는듯
		//애드온을 짓는 경우에는 hasAddon에서 판단할수 있지만... 시즈 같은 것은 같이 판단할 수 없으므로
		if (isConstructingAddon(unit)) continue;

        // if we haven't cut it, add it to the set of candidates
        candidateProducers.insert(unit);
    }

	//애드온이 필요없는 유닛은 애드온 없는 건물 우선으로 배정
	BWAPI::Unitset candidateProducers_no_addon;
	if (t.getUnitType() == BWAPI::UnitTypes::Terran_Vulture ||
		t.getUnitType() == BWAPI::UnitTypes::Terran_Goliath ||
		t.getUnitType() == BWAPI::UnitTypes::Terran_Wraith){
		for (auto u : candidateProducers){
			if (!hasAddon(u)){
				candidateProducers_no_addon.insert(u);
			}
		}
	}


	return getClosestUnitToPosition(candidateProducers_no_addon.size()>0 ? candidateProducers_no_addon : candidateProducers, closestTo);
}

BWAPI::Unit BuildManager::canMakeNowAndGetProducer(MetaType t, BWAPI::Position closestTo, int producerID){
	// v18 김유진
	//producer가 있어도 canMakeNow == false면 producer를 반환하지 않는 함수
	// this is the unit which can produce the currentItem
	BWAPI::Unit producer = getProducer(t, closestTo, producerID);

	// 건물을 만들수 있는 유닛(일꾼)이나, 유닛을 만들수 있는 유닛(건물 or 유닛)이 있으면
	if (producer != nullptr) {
		// check to see if we can make it right now
		// 지금 해당 유닛을 건설/생산 할 수 있는지에 대해 자원, 서플라이, 테크 트리, producer 만을 갖고 판단한다
		if (!canMakeNow(producer, t)){
			producer = nullptr;
		}
	}

	return producer;
}

BWAPI::Unit BuildManager::getClosestUnitToPosition(const BWAPI::Unitset & units, BWAPI::Position closestTo)
{
    if (units.size() == 0)
    {
        return nullptr;
    }

    // if we don't care where the unit is return the first one we have
	if (closestTo == BWAPI::Positions::None)
    {
        return *(units.begin());
    }

    BWAPI::Unit closestUnit = nullptr;
    double minDist(1000000000);

	for (auto & unit : units) 
    {
		if (unit == nullptr) continue;

		double distance = unit->getDistance(closestTo);
		if (!closestUnit || distance < minDist) 
        {
			closestUnit = unit;
			minDist = distance;
		}
	}

    return closestUnit;
}

// 지금 해당 유닛을 건설/생산 할 수 있는지에 대해 자원, 서플라이, 테크 트리, producer 만을 갖고 판단한다
// 해당 유닛이 건물일 경우 건물 지을 위치의 적절 여부 (탐색했었던 타일인지, 건설 가능한 타일인지, 주위에 Pylon이 있는지, Creep이 있는 곳인지 등) 는 판단하지 않는다
bool BuildManager::canMakeNow(BWAPI::Unit producer, MetaType t)
{
	if (producer == nullptr) {
		return false;
	}

	bool b_canMake = hasEnoughResources(t);

	if (b_canMake)
	{
		b_canMake = canMake(producer, t);
	}

	return b_canMake;
}

bool BuildManager::canMake(BWAPI::Unit producer, MetaType t)
{
	if (producer == nullptr) {
		return false;
	}

	bool b_canMake = false;

	if (t.isUnit())
	{
		// BWAPI::Broodwar->canMake : Checks all the requirements include resources, supply, technology tree, availability, and required units
		b_canMake = BWAPI::Broodwar->canMake(t.getUnitType(), producer); //컴셋 지으려고 하는데 아카데미 없는 경우, producer는 SCV로 지정되었다고 해도 false
	}
	else if (t.isTech())
	{
		b_canMake = BWAPI::Broodwar->canResearch(t.getTechType(), producer);
	}
	else if (t.isUpgrade())
	{
		b_canMake = BWAPI::Broodwar->canUpgrade(t.getUpgradeType(), producer);
	}

	return b_canMake;
}

// 건설 가능 위치를 찾는다
// seedLocationStrategy 가 SeedPositionSpecified 인 경우에는 그 근처만 찾아보고, SeedPositionSpecified 이 아닌 경우에는 seedLocationStrategy 를 조금씩 바꿔가며 계속 찾아본다.
// (MainBase -> MainBase 주위 -> MainBase 길목 -> MainBase 가까운 앞마당 -> MainBase 가까운 앞마당의 길목 -> 다른 멀티 위치 -> 탐색 종료)
BWAPI::TilePosition BuildManager::getDesiredPosition(BWAPI::UnitType unitType, BWAPI::TilePosition seedPosition, BuildOrderItem::SeedPositionStrategy seedPositionStrategy)
{
	BWAPI::TilePosition desiredPosition = ConstructionPlaceFinder::Instance().getBuildLocationWithSeedPositionAndStrategy(unitType, seedPosition, seedPositionStrategy);

	/*
	 //std::cout << "ConstructionPlaceFinder getBuildLocationWithSeedPositionAndStrategy "
		<< unitType.getName().c_str()
		<< " strategy " << seedPositionStrategy
		<< " seedPosition " << seedPosition.x << "," << seedPosition.y
		<< " desiredPosition " << desiredPosition.x << "," << desiredPosition.y << std::endl;
	*/

	// desiredPosition 을 찾을 수 없는 경우
	bool findAnotherPlace = true;
	while (desiredPosition == BWAPI::TilePositions::None) {

		switch (seedPositionStrategy) {
		case BuildOrderItem::SeedPositionStrategy::MainBaseLocation:
			seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::FirstChokePoint;
			break;
		case BuildOrderItem::SeedPositionStrategy::MainBaseBackYard:
			seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::FirstChokePoint;
			break;
		case BuildOrderItem::SeedPositionStrategy::FirstChokePoint:
			seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation;
			break;
		case BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation:
			seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::SecondChokePoint;
			break;
		case BuildOrderItem::SeedPositionStrategy::SecondChokePoint:
			seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation;
			break;
		case BuildOrderItem::SeedPositionStrategy::LowComplexityExpansionLocation:
			seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::MainBaseLocation;
			break;
		case BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation:
		case BuildOrderItem::SeedPositionStrategy::SeedPositionSpecified:
		case BuildOrderItem::SeedPositionStrategy::MainBaseOppositeChock:
		default:
			findAnotherPlace = false;
			break;
		}

		// 다른 곳을 더 찾아본다
		if (findAnotherPlace) {
			desiredPosition = ConstructionPlaceFinder::Instance().getBuildLocationWithSeedPositionAndStrategy(unitType, seedPosition, seedPositionStrategy);
			/*
			 //std::cout << "ConstructionPlaceFinder getBuildLocationWithSeedPositionAndStrategy "
				<< unitType.getName().c_str()
				<< " strategy " << seedPositionStrategy
				<< " seedPosition " << seedPosition.x << "," << seedPosition.y
				<< " desiredPosition " << desiredPosition.x << "," << desiredPosition.y << std::endl;
			*/
		}
		// 다른 곳을 더 찾아보지 않고, 끝낸다
		else {
			break;
		}
	}

	//std::cout << "desiredPosition : " << desiredPosition.x << ", " << desiredPosition.y << std::endl;
	return desiredPosition;
}

// 사용가능 미네랄 = 현재 보유 미네랄 - 사용하기로 예약되어있는 미네랄
int BuildManager::getAvailableMinerals()
{
	return BWAPI::Broodwar->self()->minerals() - ConstructionManager::Instance().getReservedMinerals();
}

// 사용가능 가스 = 현재 보유 가스 - 사용하기로 예약되어있는 가스
int BuildManager::getAvailableGas()
{
	return BWAPI::Broodwar->self()->gas() - ConstructionManager::Instance().getReservedGas();
}

// 큐 전체 미네랄, 가스 
std::pair<int, int> BuildManager::getQueueResource()
{
	std::pair<int, int> rst(0,0);

	for (size_t i = 0; i <buildQueue.size(); i++){
		rst.first += buildQueue[i].metaType.mineralPrice();
		rst.second += buildQueue[i].metaType.gasPrice();
	}

	return rst;
}

int BuildManager::getQueueSupplyRequired(bool subtract_supply_depot = true, bool stop_supply_depot = false)
{
	int rst = 0;

	for (size_t i = 0; i <buildQueue.size(); i++){
		if (buildQueue[i].metaType.getUnitType() == BWAPI::UnitTypes::Terran_Supply_Depot){
			if (stop_supply_depot){
				continue;
			}
			else if (subtract_supply_depot){
				rst -= buildQueue[i].metaType.getUnitType().supplyProvided();
			}
		}
		rst += buildQueue[i].metaType.supplyRequired();
	}

	return rst;
}

// return whether or not we meet resources, including building reserves
bool BuildManager::hasEnoughResources(MetaType type) 
{
	// return whether or not we meet the resources
	return (type.mineralPrice() <= getAvailableMinerals()) && (type.gasPrice() <= getAvailableGas());
}


// selects a unit of a given type
BWAPI::Unit BuildManager::selectUnitOfType(BWAPI::UnitType type, BWAPI::Position closestTo) 
{
	// if we have none of the unit type, return nullptr right away
	if (BWAPI::Broodwar->self()->completedUnitCount(type) == 0) 
	{
		return nullptr;
	}

	BWAPI::Unit unit = nullptr;

	// if we are concerned about the position of the unit, that takes priority
    if (closestTo != BWAPI::Positions::None) 
    {
		double minDist(1000000000);

		for (auto & u : BWAPI::Broodwar->self()->getUnits()) 
        {
			if (u->getType() == type) 
            {
				double distance = u->getDistance(closestTo);
				if (!unit || distance < minDist) {
					unit = u;
					minDist = distance;
				}
			}
		}

	// if it is a building and we are worried about selecting the unit with the least
	// amount of training time remaining
	} 
    else if (type.isBuilding()) 
    {
		for (auto & u : BWAPI::Broodwar->self()->getUnits()) 
        {
			if (u->getType() == type && u->isCompleted() && !u->isTraining() && !u->isLifted() &&u->isPowered()) {

				return u;
			}
		}
		// otherwise just return the first unit we come across
	} 
    else 
    {
		for (auto & u : BWAPI::Broodwar->self()->getUnits()) 
		{
			if (u->getType() == type && u->isCompleted() && u->getHitPoints() > 0 && !u->isLifted() &&u->isPowered()) 
			{
				return u;
			}
		}
	}

	// return what we've found so far
	return nullptr;
}


BuildManager & BuildManager::Instance()
{
	static BuildManager instance;
	return instance;
}

BuildOrderQueue * BuildManager::getBuildQueue()
{
	return & buildQueue;
}

bool BuildManager::isProducerWillExist(BWAPI::UnitType producerType)
{
	bool isProducerWillExist = true;

	if (BWAPI::Broodwar->self()->completedUnitCount(producerType) == 0
		&& BWAPI::Broodwar->self()->incompleteUnitCount(producerType) == 0)
	{
		// producer 가 건물 인 경우 : 건물이 건설 중인지 추가 파악
		if (producerType.isBuilding()) {
			if (ConstructionManager::Instance().getConstructionQueueItemCount(producerType) == 0) {
				
				// producerType이 Addon 건물인 경우, Addon 건물 건설이 명령 내려졌지만 시작되기 직전에는 getUnits, completedUnitCount, incompleteUnitCount 에서 확인할 수 없다
				// producerType의 producerType 건물에 의해 Addon 건물 건설의 명령이 들어갔는지까지 확인해야 한다
				if (producerType.isAddon()) {
					bool isAddonConstructing = false;
					BWAPI::UnitType producerTypeOfProducerType = producerType.whatBuilds().first;
					for (auto & unit : BWAPI::Broodwar->self()->getUnits())
					{
						if (unit->getType() == producerTypeOfProducerType){
							// 모건물이 완성되어있고, 모건물이 해당 Addon 건물을 건설중인지 확인한다
							if (hasAddon(unit)) {
								isAddonConstructing = true;
								break;
							}
						}
					}

					//필요한 애드온 안지어 지고 있음
					if (isAddonConstructing == false) {
						isProducerWillExist = false;
					}
				}
				else {
					isProducerWillExist = false;
				}
			}
		}
		// producer 가 건물이 아닌 경우 : producer 가 생성될 예정인지 추가 파악
		// producerType : 일꾼
		else {
			isProducerWillExist = false;
		}
	}

	return isProducerWillExist;
}

void BuildManager::checkBuildOrderQueueDeadlockAndRemove()
{
	/*
		getProducer에서 대부분 로직 체크. 다만, 기다려야 되는 상황이 있기 때문에 당장 처리하지 못하면 non block빌드로 전환하여 다음 기회에 수행할 수 있도록 한다.
		모두 non block 빌드가 된 경우에는 빌드큐에 있는 다른 빌드가 수행된 이후에도 이 빌드를 처리할 수 없는 상태이므로(즉, 순서 잘못이 아니다. 아에 빌드큐에 있는 것으로 해결이 안되는 상황)
		이 때는 필요한 건물이 현재 건설중인 건물이 있지 않으면 삭제처리한다.
	*/
	//큐를 lowest부터 돌면서 non block 빌드만 남았는지 체크 -> 모두다 non block이면 뭔가 문제들이 있었던 빌드이므로 아래 로직 체크해서 하나씩 지운다.
	for (size_t i = 0; i <buildQueue.size(); i++){
		if (buildQueue[i].blocking) return;
	}

	/*
	//std::cout << "every build is non block" << std::endl;
	for (int i = 0; i <buildQueue.size() - 1; i++){
		//std::cout << buildQueue[i].metaType.getName() << " ";
	}
	//std::cout << std::endl;
	*/

	for (int i = buildQueue.size()-1; i >=0 ; i--){
		bool isDeadlockCase = false;
		BuildOrderItem &currentItem = buildQueue[i];

		// producerType을 먼저 알아낸다
		BWAPI::UnitType producerType = currentItem.metaType.whatBuilds();

		////std::cout << "target:" << currentItem.metaType.getName() << "/producer:" << producerType;

		// 건물이나 유닛의 경우
		if (currentItem.metaType.isUnit())
		{
			BWAPI::UnitType unitType = currentItem.metaType.getUnitType();
			const std::map< BWAPI::UnitType, int >& requiredUnits = unitType.requiredUnits();

			// 건물을 생산하는 유닛이나, 유닛을 생산하는 건물이 존재하지 않고, 건설 예정이지도 않으면 dead lock
			if (!isProducerWillExist(producerType)) {
				isDeadlockCase = true;
			}

			// Refinery 건물의 경우, Refinery 가 건설되지 않은 Geyser가 있는 경우에만 가능
			// TODO TODO TODO TODO

			// 선행 건물/애드온이 있는데 
			if (!isDeadlockCase)
			{
				for (auto & u : requiredUnits)
				{
					// 선행 건물/애드온이 존재하지 않고, 건설 예정이지도 않으면 dead lock
					if (!isProducerWillExist(u.first)) {
						isDeadlockCase = true;
					}
				}
			}
		}


		// 테크의 경우, 해당 리서치를 이미 했거나, 이미 하고있거나, 리서치를 하는 건물 및 선행건물이 존재하지않고 건설예정이지도 않으면 dead lock
		else if (currentItem.metaType.isTech())
		{
			BWAPI::TechType techType = currentItem.metaType.getTechType();
			BWAPI::UnitType requiredUnitType = techType.requiredUnit(); //실제 테크에서 다른유닛을 필요로 하는 것은 러커변이만 레어를 필요로하고 나머지는 다 None

			if (BWAPI::Broodwar->self()->hasResearched(techType) || BWAPI::Broodwar->self()->isResearching(techType)) {
				isDeadlockCase = true;
			}
			else if (!isProducerWillExist(producerType))
			{
				isDeadlockCase = true;
			}
			else if (requiredUnitType != BWAPI::UnitTypes::None) {
				if (!isProducerWillExist(requiredUnitType))
					isDeadlockCase = true;
			}
		}
		// 업그레이드의 경우, 해당 업그레이드를 이미 했거나, 이미 하고있거나, 업그레이드를 하는 건물 및 선행건물이 존재하지도 않고 건설예정이지도 않으면 dead lock
		else if (currentItem.metaType.isUpgrade())
		{
			BWAPI::UpgradeType upgradeType = currentItem.metaType.getUpgradeType();
			int maxLevel = BWAPI::Broodwar->self()->getMaxUpgradeLevel(upgradeType);
			int currentLevel = BWAPI::Broodwar->self()->getUpgradeLevel(upgradeType);
			BWAPI::UnitType requiredUnitType = upgradeType.whatsRequired(currentLevel+1);

			if (currentLevel >= maxLevel || BWAPI::Broodwar->self()->isUpgrading(upgradeType)) {
				isDeadlockCase = true;
			}
			else if (!isProducerWillExist(producerType))
			{
				isDeadlockCase = true;
			}
			else if (requiredUnitType != BWAPI::UnitTypes::None) {
				if (!isProducerWillExist(requiredUnitType))
					isDeadlockCase = true;
			}
		}

		if (isDeadlockCase) {
			//std::cout << std::endl << "Build Order Dead lock case -> remove " << currentItem.metaType.getName() << std::endl;

			buildQueue.removeByIndex(i);
		}

	}
}

//BOSS에서 빌드가 나오면 세팅해주는 함수
//BOSS.getBuildOrder 해서 비어 있으면 그때까지는 계속 빌드큐가 비어있다.
//@도주남 김유진 설치전략 추가
void BuildManager::setBuildOrder(const BuildOrder & buildOrder)
{
	//buildQueue.clearAll(); //중간중간 빌드 추가되므로 삭제

	//현재 큐까지 값을 반영하여 limit 수치를 계산해야 하므로 카운트맵을 하나 만듬
	std::map<std::string, int> buildItemCnt;

	for (size_t i(0); i < buildOrder.size(); ++i){
		if (buildItemCnt.find(buildOrder[i].getName()) == buildItemCnt.end())
			buildItemCnt[buildOrder[i].getName()] = 0;
		else
			buildItemCnt[buildOrder[i].getName()]++;
	}

	for (size_t i(0); i<buildOrder.size(); ++i)
	{
		//유닛 최대값을 관리하여 그 이상은 안만들도록 한다.
		if (buildOrder[i].isUnit()){
			int unitLimit = StrategyManager::Instance().getUnitLimit(buildOrder[i]);
			int currentUnitNum = BWAPI::Broodwar->self()->completedUnitCount(buildOrder[i].getUnitType()) + BWAPI::Broodwar->self()->incompleteUnitCount(buildOrder[i].getUnitType()) + buildItemCnt[buildOrder[i].getName()];

			if (unitLimit != -1 && currentUnitNum > unitLimit){
				//std::cout << "[unit limit]-" << buildOrder[i].getName() << "is only created " << unitLimit << std::endl;
				buildItemCnt[buildOrder[i].getName()]--;
				continue;
			}
		}

		////std::cout << "before : " << buildOrder.size() << std::endl;

		if (buildOrder[i].isBuilding()){
			buildQueue.queueAsLowestPriority(buildOrder[i], StrategyManager::Instance().getBuildSeedPositionStrategy(buildOrder[i]), true);
		}
		else{
			buildQueue.queueAsLowestPriority(buildOrder[i], true);
		}
		////std::cout << "after : " << buildOrder.size() << std::endl;

	}
}

void BuildManager::addBuildOrderOneItem(MetaType buildOrder, BWAPI::TilePosition position, BuildOrderItem::SeedPositionStrategy seedPositionStrategy)
{
	/*
		1. 포지션을 안 준 경우 - 전략을 따로 안주면 메인베이스 건설전략으로 세팅, 전략 주면 전략 우선
		2. 포지션 준 경우 - 포지션 우선
	*/
	if (position == BWAPI::TilePositions::None){
		if (seedPositionStrategy == BuildOrderItem::SeedPositionStrategy::MainBaseLocation){
			buildQueue.queueAsHighestPriority(buildOrder, true);
		}
		else{
			buildQueue.queueAsHighestPriority(buildOrder, seedPositionStrategy, true);
		}
		
	}
	else{
		buildQueue.queueAsHighestPriority(buildOrder, position, true);
	}
}

//dhj ssh
void BuildManager::queueGasSteal()
{
	buildQueue.queueAsHighestPriority(MetaType(BWAPI::Broodwar->self()->getRace().getRefinery()), true, true);
}

void BuildManager::onUnitComplete(BWAPI::Unit unit){
	return;
}

//큐 제일 위에 있는 것을 서플라이제한으로 못만드는 경우
bool BuildManager::detectSupplyDeadlock()
{
	// 빌드큐 비어 있으면 추가하지 않음, 보스에서 해결함
	if (buildQueue.isEmpty()){
		return false;
	}

	// 서플라이 최대량에 가까우면 체크하지 않음
	if (BWAPI::Broodwar->self()->supplyTotal() >= 390)
	{
		return false;
	}

	// 서플라이 건설 중에는 고려하지 않음
	// 건설큐에 있으면 건설할 것으로 판단
	// 건물은 isComplete 될때까지 건설큐에 있음
	if (ConstructionManager::Instance().getConstructionQueueItemCount(BWAPI::UnitTypes::Terran_Supply_Depot) > 0){
		return false;
	}

	// 큐 최상위 유닛판단
	// v18 김유진
	// 큐 전체를 보고 서플라이 부족하면 추가(큐에 서플라이디팟이 있으면 그 직전까지만 계산)
	int supplyCost = getQueueSupplyRequired(true, false);
	int supplyAvailable = std::max(0, BWAPI::Broodwar->self()->supplyTotal() - BWAPI::Broodwar->self()->supplyUsed());

	// 최상위 큐의 코스트보다 적으면 서플라이 건설(건물은 해당없음)
	if (supplyAvailable < supplyCost)
	{
		return true;
	}

	return false;
}

//아주 예외적임
//매프레임 빈 커맨드센터 1개씩 돌면서 일꾼생산
//단, BOSS매니저 계산중에는 동작하지 않음
//단, 초기빌드 수행시는 동작하지 않음
//멀티 공격받고 있으면 동작하지 않음
//해당 멀티 수급할 정도만 생산 
void BuildManager::executeWorkerTraining(){
	if (!StrategyManager::Instance().isInitialBuildOrderFinished){
		return;
	}

	if (buildQueue.isEmpty()){
		return;
	}
	
	int totalWorkerCnt = WorkerManager::Instance().getWorkerData().getNumWorkers();
	//InformationManager::Instance().selfExpansions 사용안함
	//WorkerManager::Instance().getWorkerData().getDepots() 사용 %%%% 두개가 비슷한것으로 판단되나.. 각자 연관된 모듈이 다르므로 합치지 않고 간다.
	for (Expansion e : ExpansionManager::Instance().getExpansions()){
		if (!e.cc->isIdle()) continue;
		if (e.cc->isUnderAttack()) continue;
		if (!verifyBuildAddonCommand(e.cc)) continue; //빌드애드온 시작하자마자 다른 오더를 바로 내리면 안됨
		
		int tmpWorkerCnt = WorkerManager::Instance().getWorkerData().getDepotWorkerCount(e.cc);

		//최대생산량 = 현재남은 미네랄덩어리수 * 1.5 * 멀티개수별가중치
		if (tmpWorkerCnt > -1){
			double weight = 1.0;
			if(ExpansionManager::Instance().getExpansions().size() == 1){
				weight = 1.5;
			}
			else if(ExpansionManager::Instance().getExpansions().size() <= 2){
				weight = 1.3;
			}
			else {
				weight = 1.1;
			}
			// 일꾼 최대치 
			if ((tmpWorkerCnt < (int)(WorkerManager::Instance().getWorkerData().getMineralsNearDepot(e.cc) * 2 * weight)) && (totalWorkerCnt < 71)){
				e.cc->train(BWAPI::UnitTypes::Terran_SCV);
				return;
			}
		}
	}
}

void BuildManager::executeCombatUnitTraining(std::pair<int, int> availableResource){
	if (!StrategyManager::Instance().isInitialBuildOrderFinished){
		return;
	}

	if (buildQueue.isEmpty()){
		return;
	}

	BWAPI::Unitset idle_barracks;
	BWAPI::Unitset idle_Factory;
	BWAPI::Unitset idle_Factory_Machine_shop;

	for (auto u : BWAPI::Broodwar->self()->getUnits()){
		if (u->getType() == BWAPI::UnitTypes::Terran_Barracks){
			if (u->isIdle() && !u->isLifted()){
				idle_barracks.insert(u);
			}
		}

		if (u->getType() == BWAPI::UnitTypes::Terran_Factory){
			if (u->isIdle() && !isConstructingAddon(u) && !u->isLifted()){
				if (hasAddon(u)){
					idle_Factory_Machine_shop.insert(u);
				}
				else{
					idle_Factory.insert(u);
				}
			}
		}
	}

	BWAPI::UnitType marine(BWAPI::UnitTypes::Terran_Marine);
	for (auto b : idle_barracks){
		if (availableResource.first > marine.mineralPrice() && (BWAPI::Broodwar->self()->supplyTotal() - BWAPI::Broodwar->self()->supplyUsed()) >= marine.supplyRequired()){
			b->train(marine);
			availableResource.first -= marine.mineralPrice();
			//std::cout << "executeCombatUnitTraining - marine(remainingResource:" << availableResource.first << "," << availableResource.second << ")" << std::endl;
		}
		else{
			break;
		}
	}

	return; //벌쳐는 생산하지 않음
	
	BWAPI::UnitType vulture(BWAPI::UnitTypes::Terran_Vulture);
	for (auto b : idle_Factory){
		if (availableResource.first > vulture.mineralPrice() && (BWAPI::Broodwar->self()->supplyTotal() - BWAPI::Broodwar->self()->supplyUsed()) >= vulture.supplyRequired()){
			b->train(vulture);
			availableResource.first -= vulture.mineralPrice();
			//std::cout << "executeCombatUnitTraining - vulture(remainingResource:" << availableResource.first << "," << availableResource.second << ")" << std::endl;
		}
		else{
			break;
		}
	}
	for (auto b : idle_Factory_Machine_shop){
		if (availableResource.first > vulture.mineralPrice() && (BWAPI::Broodwar->self()->supplyTotal() - BWAPI::Broodwar->self()->supplyUsed()) >= vulture.supplyRequired()){
			b->train(vulture);
			availableResource.first -= vulture.mineralPrice();
			//std::cout << "executeCombatUnitTraining - vulture(remainingResource:" << availableResource.first << "," << availableResource.second << ")" << std::endl;
		}
		else{
			break;
		}
	}
}


bool BuildManager::verifyBuildAddonCommand(BWAPI::Unit u){
	//브루드워를 통과하는데 시간이 좀 걸림, 이 체크 안하고 isidle 같은걸로 확인이 안되서 명령이 덮어써짐
	if (u->getLastCommand().getType() == BWAPI::UnitCommandTypes::Build_Addon
		&& (BWAPI::Broodwar->getFrameCount() - u->getLastCommandFrame() < 10))
	{
		return false; 
	}
	else{
		return true;
	}
}

bool BuildManager::hasAddon(BWAPI::Unit u){
	//프레임 딜레이 체크하여 딜레이 동안은 다른명령을 내리면 안됨(즉, 애드온이 있는것처럼 판단)
	if (!verifyBuildAddonCommand(u))
		return true;
	// if the unit already has an addon, it can't make one
	else if (u->getAddon() || u->getAddon() != nullptr)
		return true;
	else
		return false;
}

bool BuildManager::isConstructingAddon(BWAPI::Unit u){
	//달려있는 경우에는 완료여부만 검사
	if (u->getAddon() || u->getAddon() != nullptr){
		return !u->getAddon()->isCompleted();
	}
	//안 달려있는 경우에는 명령을 내렸으면 건설중임
	else{
		if (!verifyBuildAddonCommand(u))
			return true;
	}

	return false;
}

bool BuildManager::hasUnitInQueue(BWAPI::UnitType ut){
	for (int i = buildQueue.size() - 1; i >= 0; i--){
		BuildOrderItem &currentItem = buildQueue[i];
		if (currentItem.metaType.isUnit() &&
			currentItem.metaType.getUnitType() == ut)
			return true;
	}

	return false;
}

void BuildManager::consumeRemainingResource(){
	if (!StrategyManager::Instance().isInitialBuildOrderFinished){
		return;
	}

	std::pair<int, int> queueResource = getQueueResource();
	queueResource.first += marginResource.first; //약간의 마진을 준다. 너무 타이트하게 여유자원을 사용하지 않기 위해서
	queueResource.second += marginResource.second;

	std::pair<int, int> remainingResource(getAvailableMinerals() - queueResource.first, getAvailableGas() - queueResource.second);

	/*
		0. 미네랄 최대여유만큼 남은 경우 - 멀티를 위해 멀티로직에서 사용
		1. 상대방 비행유닛 가능하면
		 - 엔지니어링베이 없는 경우, 미네랄 125 남으면 짓기
		 - 엔지니어링베이 있고, 터렛이 커맨드센터당 3기씩 없으면 미네랄 75 남으면 짓기
		2. 상대방 비행유닛 없으면
		 - 남는 건물에서 컴뱃유닛 훈련
	*/

	if (ExpansionManager::Instance().shouldExpandNow())
	{
		//아군 유닛이 적당히 나가있는 경우에만 수행한다. 이때가 비교적 안전한 경우 이므로
		if (InformationManager::Instance().nowCombatStatus == InformationManager::combatStatus::DEFCON4 ||
			InformationManager::Instance().nowCombatStatus >= InformationManager::combatStatus::CenterAttack){
			MetaType cc(BWAPI::UnitTypes::Terran_Command_Center);
			addBuildOrderOneItem(cc);
			remainingResource.first -= cc.mineralPrice();
			//std::cout << "add command center(remainingResource:" << remainingResource.first << "," << remainingResource.second << ")" << std::endl;
		}
	}

	//완전 초반 밀릴때
	//미네랄 조금 남으면 마린뽑기
	if (InformationManager::Instance().rushState == 1 && InformationManager::Instance().getRushSquad().size() > 0){
		signed numCombat = 0;
		for (auto u : BWAPI::Broodwar->self()->getUnits()){
			if (UnitUtils::IsCombatUnit_rush(u)){
				numCombat++;
			}
		}

		if (numCombat <= InformationManager::Instance().getRushSquad().size()){
			remainingResource.first -= marginResource.first/2;
			executeCombatUnitTraining(remainingResource);
		}
	}

	//v18 김유진
	//비행유닛 대비 공략을 여유자원이 없더라도 만들수 있도록 로직을 update쪽으로 이동
	//if (InformationManager::Instance().hasFlyingUnits){
	//	if (!isProducerWillExist(BWAPI::UnitTypes::Terran_Engineering_Bay)){
	//		MetaType eb(BWAPI::UnitTypes::Terran_Engineering_Bay);
	//		if (remainingResource.first >= eb.mineralPrice()){
	//			addBuildOrderOneItem(eb);
	//			remainingResource.first -= eb.mineralPrice();
	//			//std::cout << "add command engineering(remainingResource:" << remainingResource.first << "," << remainingResource.second << ")" << std::endl;
	//		}
	//	}
	//	else{
	//		//한번에 한개씩만 건설
	//		//큐에 없을때만 추가
	//		if (hasUnitInQueue(BWAPI::UnitTypes::Terran_Missile_Turret) == 0){
//			bool existsInConstructionQueue = false;
//			for (auto task : *ConstructionManager::Instance().getConstructionQueue()){
//				if (task.type == BWAPI::UnitTypes::Terran_Missile_Turret && task.buildingUnit == nullptr){
//					existsInConstructionQueue = true;
//					break;
//				}
//			}

//			//빌드큐와 건설큐(건설 시작 전)에 없어야 한다.
//			if (existsInConstructionQueue){
//				MetaType mt(BWAPI::UnitTypes::Terran_Missile_Turret);
//				if (remainingResource.first >= mt.mineralPrice()){
//					int numTurret = UnitUtils::GetAllUnitCount(BWAPI::UnitTypes::Terran_Missile_Turret);
//					if (numTurret < StrategyManager::Instance().getUnitLimit(BWAPI::UnitTypes::Terran_Missile_Turret)){
//						BWAPI::Position p = ExpansionManager::Instance().getExpansions()[numTurret % ExpansionManager::Instance().getExpansions().size()].cc->getPosition();
//						addBuildOrderOneItem(mt, BWAPI::TilePosition(p));
//						remainingResource.first -= mt.mineralPrice();
//						//std::cout << "add turret(remainingResource:" << remainingResource.first << "," << remainingResource.second << ")" << BWAPI::TilePosition(p) << std::endl;
//					}
//				}
//			}
//		}
//	}
//}

//서플라이가 막힌 경우
//빌드큐 또는 건설큐에 서플라이 없어야 한다.
//v18 김유진
//삭제하고 detectSupplyDeadlock 로 해결
//if ((BWAPI::Broodwar->self()->supplyTotal() - BWAPI::Broodwar->self()->supplyUsed()) <= 0){
//	if (ConstructionManager::Instance().getConstructionQueueItemCount(BWAPI::UnitTypes::Terran_Supply_Depot) == 0 &&
//		buildQueue.getHighestPriorityItem().metaType.getUnitType() != BWAPI::UnitTypes::Terran_Supply_Depot){
//		MetaType sd(BWAPI::UnitTypes::Terran_Supply_Depot);
//		if (remainingResource.first >= sd.mineralPrice()){
//			addBuildOrderOneItem(sd, BWAPI::TilePositions::None, StrategyManager::Instance().getBuildSeedPositionStrategy(sd));
//			remainingResource.first -= sd.mineralPrice();
//			//std::cout << "add supply depot(remainingResource:" << remainingResource.first << "," << remainingResource.second << ")" << std::endl;
//		}
//	}
//}
}

void BuildManager::checkErrorBuildOrderAndRemove(){
	if (buildQueue.isEmpty())
		return;

	//가장 첫 빌드만 확인한다.
	BuildOrderItem currentItem = buildQueue.getHighestPriorityItem();

	BWAPI::UnitType producerType = currentItem.metaType.whatBuilds();
	// 애드온인 경우, 모든 팩토리가 애드온이 달렸거나 달리는중이면 삭제처리
	// 전제사항 : 애드온은 항상 본 건물이 먼저 큐에 들어온다.
	if (currentItem.metaType.getUnitType().isAddon()){
		bool all_producer_has_addon = true;
		for (BWAPI::Unit u : BWAPI::Broodwar->self()->getUnits()){
			if (u->getType() == producerType && !hasAddon(u)){
				all_producer_has_addon = false;
				break;
			}
		}
		if (all_producer_has_addon){
			//std::cout << "checkErrorBuildOrderAndRemove : " << currentItem.metaType.getName() << std::endl;
			buildQueue.removeCurrentItem();
		}
	}
}

bool BuildManager::addBuildings(MetaType & u, int max_num = 200, BuildOrderItem::SeedPositionStrategy seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::MainBaseLocation){
	//한번에 한개씩만 건설
	//큐에 없을때만 추가
	//전체 개수 제한
	bool isAdded = false;

	int numUnits = UnitUtils::GetAllUnitCount(u.getUnitType());
	if (numUnits >= max_num){
		return isAdded;
	}

	if (hasUnitInQueue(u.getUnitType()) == 0){
		bool existsInConstructionQueue = false;

		for (auto task : *ConstructionManager::Instance().getConstructionQueue()){
			if (task.type == u.getUnitType() && task.buildingUnit == nullptr){
				existsInConstructionQueue = true;
				break;
			}
		}

		//빌드큐와 건설큐(건설 시작 전)에 없어야 한다.
		//건설 시작하면 전체 개수로 판단해서 제한함
		//전체 개수만 가능하면 건설 시작만 하면 추가적으로 빌드큐에 들어감
		if (!existsInConstructionQueue){
			addBuildOrderOneItem(u, BWAPI::TilePositions::None, seedPositionStrategy);
			isAdded = true;
		}
	}

	return isAdded;
}

bool BuildManager::addBuildings(MetaType & u, int max_num, std::vector<BuildOrderItem::SeedPositionStrategy> seedPositionStrategies){
	//한번에 한개씩만 건설
	//큐에 없을때만 추가
	//전체 개수 제한
	bool isAdded = false;

	int numUnits = UnitUtils::GetAllUnitCount(u.getUnitType());
	if (numUnits >= max_num){
		return isAdded;
	}

	BuildOrderItem::SeedPositionStrategy seedPositionStrategy;
	if (seedPositionStrategies.empty() || seedPositionStrategies.size() <= numUnits){
		seedPositionStrategy = BuildOrderItem::SeedPositionStrategy::MainBaseLocation;
	}
	else{
		seedPositionStrategy = seedPositionStrategies[numUnits];
	}

	if (hasUnitInQueue(u.getUnitType()) == 0){
		bool existsInConstructionQueue = false;

		for (auto task : *ConstructionManager::Instance().getConstructionQueue()){
			if (task.type == u.getUnitType() && task.buildingUnit == nullptr){
				existsInConstructionQueue = true;
				break;
			}
		}

		//빌드큐와 건설큐(건설 시작 전)에 없어야 한다.
		//건설 시작하면 전체 개수로 판단해서 제한함
		//전체 개수만 가능하면 건설 시작만 하면 추가적으로 빌드큐에 들어감
		if (!existsInConstructionQueue){
			addBuildOrderOneItem(u, BWAPI::TilePositions::None, seedPositionStrategy);
			isAdded = true;
		}
	}

	return isAdded;
}

void BuildManager::defenceFlyingAndDetect(){
	int turret_warning_level = InformationManager::Instance().needTurret("basicDetect");
	bool justAddEngineeringBay = false;

	if (turret_warning_level > 0){
		//1 - 아카데미 준비 + 엔지니어링 베이 준비 + 터렛 2개
		//2 - 엔지니어링 베이 + 터렛 3개
		//3 - 엔지니어링 베이 + 터렛 5개
		MetaType academy(BWAPI::UnitTypes::Terran_Academy);
		bool is_academy_added = addBuildings(academy, 1, BuildOrderItem::SeedPositionStrategy::MainBaseLocation);

		MetaType eb(BWAPI::UnitTypes::Terran_Engineering_Bay);
		if (addBuildings(eb, 1, BuildOrderItem::SeedPositionStrategy::MainBaseLocation)){
			justAddEngineeringBay = true;
		}
	
		if (!justAddEngineeringBay){
			MetaType u(BWAPI::UnitTypes::Terran_Missile_Turret);
			if (turret_warning_level > 2){
				addBuildings(u, 2, BuildOrderItem::SeedPositionStrategy::MissileTurret);
			}
			else if (turret_warning_level > 1){
				addBuildings(u, 2, BuildOrderItem::SeedPositionStrategy::MissileTurret);
			}
			else{
				addBuildings(u, 2, BuildOrderItem::SeedPositionStrategy::MissileTurret);
			}
		}	

	}

	turret_warning_level = InformationManager::Instance().needTurret("basicFlying");

	if (turret_warning_level > 0){
		//1 - 엔지니어링 베이 준비 + 터렛 2개
		//2 - 터렛 3개
		//3 - 터렛 5개
		if (!justAddEngineeringBay){
			MetaType eb(BWAPI::UnitTypes::Terran_Engineering_Bay);
			if (addBuildings(eb, 1, BuildOrderItem::SeedPositionStrategy::MainBaseLocation)){
				justAddEngineeringBay = true;
			}
		}
		
		if (!justAddEngineeringBay){
			MetaType u(BWAPI::UnitTypes::Terran_Missile_Turret);
			int numPerExpansion = 0;
			int totalNum = 0;
			if (turret_warning_level > 2){
				numPerExpansion = 5;
			}
			else if (turret_warning_level > 1){
				numPerExpansion = 3;
			}
			else{
				numPerExpansion = 2;
			}

			std::vector<BuildOrderItem::SeedPositionStrategy> seedPositionStrategies;

			//본진에 대한 터렛 세팅
			if (ExpansionManager::Instance().getExpansions().size() > 0){
				for (int i = 0; i < numPerExpansion; i++){
					seedPositionStrategies.push_back(BuildOrderItem::SeedPositionStrategy::MainBaseLocation);
				}
				totalNum += numPerExpansion;
			}
			
			//첫번째 멀티에 대한 터렛 세팅
			if (ExpansionManager::Instance().getExpansions().size() > 1){
				for (int i = 0; i < numPerExpansion; i++){
					seedPositionStrategies.push_back(BuildOrderItem::SeedPositionStrategy::FirstExpansionLocation);
				}
				totalNum += numPerExpansion;
			}
			

			//두번째 멀티에 대한 터렛 세팅
			if (ExpansionManager::Instance().getExpansions().size() > 2){
				for (int i = 0; i < numPerExpansion; i++){
					seedPositionStrategies.push_back(BuildOrderItem::SeedPositionStrategy::SecondExpansionLocation);
				}
				totalNum += numPerExpansion;
			}

			addBuildings(u, totalNum, seedPositionStrategies);
		}
	}
}