#pragma once

#include "Common.h"
#include "UnitData.h"
#include "ExpansionManager.h"
#include <queue>

namespace MyBot
{
	/// 게임 상황정보 중 일부를 자체 자료구조 및 변수들에 저장하고 업데이트하는 class
	/// 현재 게임 상황정보는 BWAPI::Broodwar 를 조회하여 파악할 수 있지만, 과거 게임 상황정보는 BWAPI::Broodwar 를 통해 조회가 불가능하기 때문에 InformationManager에서 별도 관리하도록 합니다
	/// 또한, BWAPI::Broodwar 나 BWTA 등을 통해 조회할 수 있는 정보이지만 전처리 / 별도 관리하는 것이 유용한 것도 InformationManager에서 별도 관리하도록 합니다
	class InformationManager 
	{
		InformationManager();

		/// Player - UnitData(각 Unit 과 그 Unit의 UnitInfo 를 Map 형태로 저장하는 자료구조) 를 저장하는 자료구조 객체
		std::map<BWAPI::Player, UnitData>							_unitData;

		/// 해당 Player의 주요 건물들이 있는 BaseLocation. 
		/// 처음에는 StartLocation 으로 지정. mainBaseLocation 내 모든 건물이 파괴될 경우 재지정
		/// 건물 여부를 기준으로 파악하기 때문에 부적절하게 판단할수도 있습니다 
		std::map<BWAPI::Player, BWTA::BaseLocation * >				_mainBaseLocations;

		/// 해당 Player의 mainBaseLocation 이 변경되었는가 (firstChokePoint, secondChokePoint, firstExpansionLocation 를 재지정 했는가)
		std::map<BWAPI::Player, bool>								_mainBaseLocationChanged;

		/// 해당 Player가 점령하고 있는 Region 이 있는 BaseLocation
		/// 건물 여부를 기준으로 파악하기 때문에 부적절하게 판단할수도 있습니다 
		std::map<BWAPI::Player, std::list<BWTA::BaseLocation *> >	_occupiedBaseLocations;

		/// 해당 Player가 점령하고 있는 Region
		/// 건물 여부를 기준으로 파악하기 때문에 부적절하게 판단할수도 있습니다 
		std::map<BWAPI::Player, std::set<BWTA::Region *> >			_occupiedRegions;

		/// 해당 Player의 mainBaseLocation 에서 가장 가까운 ChokePoint
		std::map<BWAPI::Player, BWTA::Chokepoint *>					_firstChokePoint;
		/// 해당 Player의 mainBaseLocation 에서 가장 가까운 BaseLocation
		std::map<BWAPI::Player, BWTA::BaseLocation *>				_firstExpansionLocation;
		/// 해당 Player의 mainBaseLocation 에서 두번째로 가까운 (firstChokePoint가 아닌) ChokePoint
		/// 게임 맵에 따라서, secondChokePoint 는 일반 상식과 다른 지점이 될 수도 있습니다
		std::map<BWAPI::Player, BWTA::Chokepoint *>					_secondChokePoint;

		std::queue<BWAPI::TilePosition> _supPositionsForWall;
		BWAPI::TilePosition _barPositionForWall;
		std::vector<BWAPI::TilePosition> _wallPositions;
		BWAPI::TilePosition _wallBarackPosition;
		//std::vector<BWAPI::Unit> _wallUnits;
		BWAPI::Unitset _wallUnits;
		bool _wallStatus;
		// 고정된위치에 서플라이 건설은 위한 자료구조 추가
		std::queue<BWAPI::TilePosition> _reservedSupPositions;

		// 다템, 럴커 상대 터렛 위치
		BWAPI::TilePosition _turretPosition;
		bool _turretStatus;

		/// 전체 unit 의 정보를 업데이트 합니다 (UnitType, lastPosition, HitPoint 등)
		void                    updateUnitsInfo();

		/// 해당 unit 의 정보를 업데이트 합니다 (UnitType, lastPosition, HitPoint 등)
		void                    updateUnitInfo(BWAPI::Unit unit);
		void                    updateBaseLocationInfo();
		void					updateChokePointAndExpansionLocation();
		void                    updateOccupiedRegions(BWTA::Region * region, BWAPI::Player player);
		char mapName;

		

		std::set<UnitInfo> rushSquad;

	public:

		//kyj
		UIMap &           getUnitInfo(BWAPI::Player player);

		/// static singleton 객체를 리턴합니다
		static InformationManager & Instance();
		void onStart();
			
		BWAPI::Player       selfPlayer;		///< 아군 Player		
		BWAPI::Race			selfRace;		///< 아군 Player의 종족		
		BWAPI::Player       enemyPlayer;	///< 적군 Player		
		BWAPI::Race			enemyRace;		///< 적군 Player의 종족  
		BWAPI::UnitType     enemyResourceDepotType;

		bool hasCloakedUnits;
		bool hasFlyingUnits;
		
		/// Unit 및 BaseLocation, ChokePoint 등에 대한 정보를 업데이트합니다
		void                    update();

		/// Unit 에 대한 정보를 업데이트합니다
		void					onUnitShow(BWAPI::Unit unit)        { updateUnitInfo(unit); }
		/// Unit 에 대한 정보를 업데이트합니다
		void					onUnitHide(BWAPI::Unit unit)        { updateUnitInfo(unit); }
		/// Unit 에 대한 정보를 업데이트합니다
		void					onUnitCreate(BWAPI::Unit unit)		{ updateUnitInfo(unit); }
		/// Unit 에 대한 정보를 업데이트합니다
		void					onUnitComplete(BWAPI::Unit unit)    { updateUnitInfo(unit); }
		/// Unit 에 대한 정보를 업데이트합니다
		void					onUnitMorph(BWAPI::Unit unit)       { updateUnitInfo(unit); }
		/// Unit 에 대한 정보를 업데이트합니다
		void					onUnitRenegade(BWAPI::Unit unit)    { updateUnitInfo(unit); }
		/// Unit 에 대한 정보를 업데이트합니다 
		/// 유닛이 파괴/사망한 경우, 해당 유닛 정보를 삭제합니다
		void					onUnitDestroy(BWAPI::Unit unit);
			
		
		/// 해당 BaseLocation 에 player의 건물이 존재하는지 리턴합니다
		/// @param baseLocation 대상 BaseLocation
		/// @param player 아군 / 적군
		/// @param radius TilePosition 단위
		bool					hasBuildingAroundBaseLocation(BWTA::BaseLocation * baseLocation, BWAPI::Player player, int radius = 10);
		
		/// 해당 Region 에 해당 Player의 건물이 존재하는지 리턴합니다
		bool					existsPlayerBuildingInRegion(BWTA::Region * region, BWAPI::Player player);		

		/// 해당 Player (아군 or 적군) 가 건물을 건설해서 점령한 Region 목록을 리턴합니다
		std::set<BWTA::Region *> &  getOccupiedRegions(BWAPI::Player player);

		/// 해당 Player (아군 or 적군) 의 건물을 건설해서 점령한 BaseLocation 목록을 리턴합니다		 
		std::list<BWTA::BaseLocation *> & getOccupiedBaseLocations(BWAPI::Player player);

		/// 해당 Player (아군 or 적군) 의 Main BaseLocation 을 리턴합니다		 
		BWTA::BaseLocation *	getMainBaseLocation(BWAPI::Player player);

		/// 해당 Player (아군 or 적군) 의 Main BaseLocation 에서 가장 가까운 ChokePoint 를 리턴합니다		 
		BWTA::Chokepoint *      getFirstChokePoint(BWAPI::Player player);

		/// 해당 Player (아군 or 적군) 의 Main BaseLocation 에서 가장 가까운 Expansion BaseLocation 를 리턴합니다		 
		BWTA::BaseLocation *    getFirstExpansionLocation(BWAPI::Player player);

		/// 해당 Player (아군 or 적군) 의 Main BaseLocation 에서 두번째로 가까운 ChokePoint 를 리턴합니다		 
		/// 게임 맵에 따라서, secondChokePoint 는 일반 상식과 다른 지점이 될 수도 있습니다
		BWTA::Chokepoint *      getSecondChokePoint(BWAPI::Player player);

		enum combatStatus {
			Idle,
			DEFCON1, //최초 방어
			DEFCON2, //1st 쵸크 안		    
			DEFCON3, //1st 쵸크 밖
			DEFCON4, //2st 쵸크
			DEFCON5, //메인 방어
			CenterAttack, // 센터 공격
			MainAttack, // 제일 약한 지역 공격
			EnemyBaseAttack, // 적 본진 공격
			WorkerCombat,
			WorkerBlcock
		};
		combatStatus nowCombatStatus;
		combatStatus lastCombatStatus;
		int changeConmgeStatusFrame;
		combatStatus workerCombatStatus;
		void setCombatStatus(combatStatus cs);

		BWAPI::Position currentCombatOrderPosition;
		BWAPI::Position getCurrentCombatOrderPosition();

		/// 해당 Player (아군 or 적군) 의 모든 유닛 목록 (가장 최근값) UnitAndUnitInfoMap 을 리턴합니다		 
		/// 파악된 정보만을 리턴하기 때문에 적군의 정보는 틀린 값일 수 있습니다
		UnitAndUnitInfoMap &           getUnitAndUnitInfoMap(BWAPI::Player player);
		/// 해당 Player (아군 or 적군) 의 모든 유닛 통계 UnitData 을 리턴합니다		 
		UnitData &        getUnitData(BWAPI::Player player);


		/// 해당 Player (아군 or 적군) 의 해당 UnitType 유닛 숫자를 리턴합니다 (훈련/건설 중인 유닛 숫자까지 포함)
		int						getNumUnits(BWAPI::UnitType type, BWAPI::Player player);

		/// 해당 Player (아군 or 적군) 의 position 주위의 유닛 목록을 unitInfo 에 저장합니다		 
		void                    getNearbyForce(std::vector<UnitInfo> & unitInfo, BWAPI::Position p, BWAPI::Player player, int radius);

		/// 해당 UnitType 이 전투 유닛인지 리턴합니다
		bool					isCombatUnitType(BWAPI::UnitType type) const;



		// 해당 종족의 UnitType 중 ResourceDepot 기능을 하는 UnitType을 리턴합니다
		BWAPI::UnitType			getBasicResourceDepotBuildingType(BWAPI::Race race = BWAPI::Races::None);

		// 해당 종족의 UnitType 중 Refinery 기능을 하는 UnitType을 리턴합니다
		BWAPI::UnitType			getRefineryBuildingType(BWAPI::Race race = BWAPI::Races::None);

		// 해당 종족의 UnitType 중 SupplyProvider 기능을 하는 UnitType을 리턴합니다
		BWAPI::UnitType			getBasicSupplyProviderUnitType(BWAPI::Race race = BWAPI::Races::None);

		// 해당 종족의 UnitType 중 Worker 에 해당하는 UnitType을 리턴합니다
		BWAPI::UnitType			getWorkerType(BWAPI::Race race = BWAPI::Races::None);

		// 해당 종족의 UnitType 중 Basic Combat Unit 에 해당하는 UnitType을 리턴합니다
		BWAPI::UnitType			getBasicCombatUnitType(BWAPI::Race race = BWAPI::Races::None);

		// 해당 종족의 UnitType 중 Basic Combat Unit 을 생산하기 위해 건설해야하는 UnitType을 리턴합니다
		BWAPI::UnitType			getBasicCombatBuildingType(BWAPI::Race race = BWAPI::Races::None);

		// 해당 종족의 UnitType 중 Advanced Combat Unit 에 해당하는 UnitType을 리턴합니다
		BWAPI::UnitType			getAdvancedCombatUnitType(BWAPI::Race race = BWAPI::Races::None);

		// 해당 종족의 UnitType 중 Observer 에 해당하는 UnitType을 리턴합니다
		BWAPI::UnitType			getObserverUnitType(BWAPI::Race race = BWAPI::Races::None);

		// 해당 종족의 UnitType 중 Basic Depense 기능을 하는 UnitType을 리턴합니다
		BWAPI::UnitType			getBasicDefenseBuildingType(BWAPI::Race race = BWAPI::Races::None);

		// 해당 종족의 UnitType 중 Advanced Depense 기능을 하는 UnitType을 리턴합니다
		BWAPI::UnitType			getAdvancedDefenseBuildingType(BWAPI::Race race = BWAPI::Races::None);

		void                    enemyHasCloakedUnits(BWAPI::Unit u);
		void                    enemyHasFlyingUnits(BWAPI::Unit u);

		char getMapName();

		int rushState;
		std::set<UnitInfo> & getRushSquad();

		int getPostionAtHunter();
		BWAPI::Position  getPostionAtHunterfirst();
		BWAPI::Position  getPostionAtHuntersecond();

		BWTA::Chokepoint * enemyBlockChoke;
		bool isBlockedEnemyChoke();

		std::queue<BWAPI::TilePosition>& getSupPostionsForWall();
		BWAPI::TilePosition getBarPostionsForWall();
		std::queue<BWAPI::TilePosition>& getReservedSupPositions();

		void initPositionsForWall();
		BWAPI::Position dropTo;
		BWAPI::Position getDropPosition();
		void setWallPosition(BWAPI::TilePosition position);
		void updateWallStatus();
		bool getWallStatus();
		BWAPI::Unitset getWallUnits();
		double getEnemyUnitRatio(BWAPI::UnitType baseUnitType);

		std::vector<BWAPI::TilePosition>& getWallPositions();
		BWAPI::TilePosition getWallBarackPosition();

		int needTurret(std::string mode);

		// 다템, 럴커용 터렛위치 리턴
		BWAPI::TilePosition getTurretPosition();
		bool getTurretStatus();
		void setTurretStatus(bool flag);

		//drop 한번에 갈 dropship 갯수 : 지정된 갯수의 드롭쉽이 가득 차면 출발
		int bombDropNum;
		BWAPI::Unitset buildingDetectors;

		std::string unitTypeString(int ut){
			if (ut == BWAPI::UnitTypes::Terran_Marine) return"Terran_Marine";
			if (ut == BWAPI::UnitTypes::Terran_Ghost) return"Terran_Ghost";
			if (ut == BWAPI::UnitTypes::Terran_Vulture) return"Terran_Vulture";
			if (ut == BWAPI::UnitTypes::Terran_Goliath) return"Terran_Goliath";
			if (ut == BWAPI::UnitTypes::Terran_Siege_Tank_Tank_Mode) return"Terran_Siege_Tank_Tank_Mode";
			if (ut == BWAPI::UnitTypes::Terran_SCV) return"Terran_SCV";
			if (ut == BWAPI::UnitTypes::Terran_Wraith) return"Terran_Wraith";
			if (ut == BWAPI::UnitTypes::Terran_Science_Vessel) return"Terran_Science_Vessel";
			if (ut == BWAPI::UnitTypes::Hero_Gui_Montag) return"Hero_Gui_Montag";
			if (ut == BWAPI::UnitTypes::Terran_Dropship) return"Terran_Dropship";
			if (ut == BWAPI::UnitTypes::Terran_Battlecruiser) return"Terran_Battlecruiser";
			if (ut == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine) return"Terran_Vulture_Spider_Mine";
			if (ut == BWAPI::UnitTypes::Terran_Nuclear_Missile) return"Terran_Nuclear_Missile";
			if (ut == BWAPI::UnitTypes::Terran_Civilian) return"Terran_Civilian";
			if (ut == BWAPI::UnitTypes::Hero_Sarah_Kerrigan) return"Hero_Sarah_Kerrigan";
			if (ut == BWAPI::UnitTypes::Hero_Alan_Schezar) return"Hero_Alan_Schezar";
			if (ut == BWAPI::UnitTypes::Hero_Jim_Raynor_Vulture) return"Hero_Jim_Raynor_Vulture";
			if (ut == BWAPI::UnitTypes::Hero_Jim_Raynor_Marine) return"Hero_Jim_Raynor_Marine";
			if (ut == BWAPI::UnitTypes::Hero_Tom_Kazansky) return"Hero_Tom_Kazansky";
			if (ut == BWAPI::UnitTypes::Hero_Magellan) return"Hero_Magellan";
			if (ut == BWAPI::UnitTypes::Hero_Edmund_Duke_Tank_Mode) return"Hero_Edmund_Duke_Tank_Mode";
			if (ut == BWAPI::UnitTypes::Hero_Edmund_Duke_Siege_Mode) return"Hero_Edmund_Duke_Siege_Mode";
			if (ut == BWAPI::UnitTypes::Hero_Arcturus_Mengsk) return"Hero_Arcturus_Mengsk";
			if (ut == BWAPI::UnitTypes::Hero_Hyperion) return"Hero_Hyperion";
			if (ut == BWAPI::UnitTypes::Hero_Norad_II) return"Hero_Norad_II";
			if (ut == BWAPI::UnitTypes::Terran_Siege_Tank_Siege_Mode) return"Terran_Siege_Tank_Siege_Mode";
			if (ut == BWAPI::UnitTypes::Terran_Firebat) return"Terran_Firebat";
			if (ut == BWAPI::UnitTypes::Spell_Scanner_Sweep) return"Spell_Scanner_Sweep";
			if (ut == BWAPI::UnitTypes::Terran_Medic) return"Terran_Medic";
			if (ut == BWAPI::UnitTypes::Zerg_Larva) return"Zerg_Larva";
			if (ut == BWAPI::UnitTypes::Zerg_Egg) return"Zerg_Egg";
			if (ut == BWAPI::UnitTypes::Zerg_Zergling) return"Zerg_Zergling";
			if (ut == BWAPI::UnitTypes::Zerg_Hydralisk) return"Zerg_Hydralisk";
			if (ut == BWAPI::UnitTypes::Zerg_Ultralisk) return"Zerg_Ultralisk";
			if (ut == BWAPI::UnitTypes::Zerg_Broodling) return"Zerg_Broodling";
			if (ut == BWAPI::UnitTypes::Zerg_Drone) return"Zerg_Drone";
			if (ut == BWAPI::UnitTypes::Zerg_Overlord) return"Zerg_Overlord";
			if (ut == BWAPI::UnitTypes::Zerg_Mutalisk) return"Zerg_Mutalisk";
			if (ut == BWAPI::UnitTypes::Zerg_Guardian) return"Zerg_Guardian";
			if (ut == BWAPI::UnitTypes::Zerg_Queen) return"Zerg_Queen";
			if (ut == BWAPI::UnitTypes::Zerg_Defiler) return"Zerg_Defiler";
			if (ut == BWAPI::UnitTypes::Zerg_Scourge) return"Zerg_Scourge";
			if (ut == BWAPI::UnitTypes::Hero_Torrasque) return"Hero_Torrasque";
			if (ut == BWAPI::UnitTypes::Hero_Matriarch) return"Hero_Matriarch";
			if (ut == BWAPI::UnitTypes::Zerg_Infested_Terran) return"Zerg_Infested_Terran";
			if (ut == BWAPI::UnitTypes::Hero_Infested_Kerrigan) return"Hero_Infested_Kerrigan";
			if (ut == BWAPI::UnitTypes::Hero_Unclean_One) return"Hero_Unclean_One";
			if (ut == BWAPI::UnitTypes::Hero_Hunter_Killer) return"Hero_Hunter_Killer";
			if (ut == BWAPI::UnitTypes::Hero_Devouring_One) return"Hero_Devouring_One";
			if (ut == BWAPI::UnitTypes::Hero_Kukulza_Mutalisk) return"Hero_Kukulza_Mutalisk";
			if (ut == BWAPI::UnitTypes::Hero_Kukulza_Guardian) return"Hero_Kukulza_Guardian";
			if (ut == BWAPI::UnitTypes::Hero_Yggdrasill) return"Hero_Yggdrasill";
			if (ut == BWAPI::UnitTypes::Terran_Valkyrie) return"Terran_Valkyrie";
			if (ut == BWAPI::UnitTypes::Zerg_Cocoon) return"Zerg_Cocoon";
			if (ut == BWAPI::UnitTypes::Protoss_Corsair) return"Protoss_Corsair";
			if (ut == BWAPI::UnitTypes::Protoss_Dark_Templar) return"Protoss_Dark_Templar";
			if (ut == BWAPI::UnitTypes::Zerg_Devourer) return"Zerg_Devourer";
			if (ut == BWAPI::UnitTypes::Protoss_Dark_Archon) return"Protoss_Dark_Archon";
			if (ut == BWAPI::UnitTypes::Protoss_Probe) return"Protoss_Probe";
			if (ut == BWAPI::UnitTypes::Protoss_Zealot) return"Protoss_Zealot";
			if (ut == BWAPI::UnitTypes::Protoss_Dragoon) return"Protoss_Dragoon";
			if (ut == BWAPI::UnitTypes::Protoss_High_Templar) return"Protoss_High_Templar";
			if (ut == BWAPI::UnitTypes::Protoss_Archon) return"Protoss_Archon";
			if (ut == BWAPI::UnitTypes::Protoss_Shuttle) return"Protoss_Shuttle";
			if (ut == BWAPI::UnitTypes::Protoss_Scout) return"Protoss_Scout";
			if (ut == BWAPI::UnitTypes::Protoss_Arbiter) return"Protoss_Arbiter";
			if (ut == BWAPI::UnitTypes::Protoss_Carrier) return"Protoss_Carrier";
			if (ut == BWAPI::UnitTypes::Protoss_Interceptor) return"Protoss_Interceptor";
			if (ut == BWAPI::UnitTypes::Hero_Dark_Templar) return"Hero_Dark_Templar";
			if (ut == BWAPI::UnitTypes::Hero_Zeratul) return"Hero_Zeratul";
			if (ut == BWAPI::UnitTypes::Hero_Tassadar_Zeratul_Archon) return"Hero_Tassadar_Zeratul_Archon";
			if (ut == BWAPI::UnitTypes::Hero_Fenix_Zealot) return"Hero_Fenix_Zealot";
			if (ut == BWAPI::UnitTypes::Hero_Fenix_Dragoon) return"Hero_Fenix_Dragoon";
			if (ut == BWAPI::UnitTypes::Hero_Tassadar) return"Hero_Tassadar";
			if (ut == BWAPI::UnitTypes::Hero_Mojo) return"Hero_Mojo";
			if (ut == BWAPI::UnitTypes::Hero_Warbringer) return"Hero_Warbringer";
			if (ut == BWAPI::UnitTypes::Hero_Gantrithor) return"Hero_Gantrithor";
			if (ut == BWAPI::UnitTypes::Protoss_Reaver) return"Protoss_Reaver";
			if (ut == BWAPI::UnitTypes::Protoss_Observer) return"Protoss_Observer";
			if (ut == BWAPI::UnitTypes::Protoss_Scarab) return"Protoss_Scarab";
			if (ut == BWAPI::UnitTypes::Hero_Danimoth) return"Hero_Danimoth";
			if (ut == BWAPI::UnitTypes::Hero_Aldaris) return"Hero_Aldaris";
			if (ut == BWAPI::UnitTypes::Hero_Artanis) return"Hero_Artanis";
			if (ut == BWAPI::UnitTypes::Critter_Rhynadon) return"Critter_Rhynadon";
			if (ut == BWAPI::UnitTypes::Critter_Bengalaas) return"Critter_Bengalaas";
			if (ut == BWAPI::UnitTypes::Special_Cargo_Ship) return"Special_Cargo_Ship";
			if (ut == BWAPI::UnitTypes::Special_Mercenary_Gunship) return"Special_Mercenary_Gunship";
			if (ut == BWAPI::UnitTypes::Critter_Scantid) return"Critter_Scantid";
			if (ut == BWAPI::UnitTypes::Critter_Kakaru) return"Critter_Kakaru";
			if (ut == BWAPI::UnitTypes::Critter_Ragnasaur) return"Critter_Ragnasaur";
			if (ut == BWAPI::UnitTypes::Critter_Ursadon) return"Critter_Ursadon";
			if (ut == BWAPI::UnitTypes::Zerg_Lurker_Egg) return"Zerg_Lurker_Egg";
			if (ut == BWAPI::UnitTypes::Hero_Raszagal) return"Hero_Raszagal";
			if (ut == BWAPI::UnitTypes::Hero_Samir_Duran) return"Hero_Samir_Duran";
			if (ut == BWAPI::UnitTypes::Hero_Alexei_Stukov) return"Hero_Alexei_Stukov";
			if (ut == BWAPI::UnitTypes::Special_Map_Revealer) return"Special_Map_Revealer";
			if (ut == BWAPI::UnitTypes::Hero_Gerard_DuGalle) return"Hero_Gerard_DuGalle";
			if (ut == BWAPI::UnitTypes::Zerg_Lurker) return"Zerg_Lurker";
			if (ut == BWAPI::UnitTypes::Hero_Infested_Duran) return"Hero_Infested_Duran";
			if (ut == BWAPI::UnitTypes::Spell_Disruption_Web) return"Spell_Disruption_Web";
			if (ut == BWAPI::UnitTypes::Terran_Command_Center) return"Terran_Command_Center";
			if (ut == BWAPI::UnitTypes::Terran_Comsat_Station) return"Terran_Comsat_Station";
			if (ut == BWAPI::UnitTypes::Terran_Nuclear_Silo) return"Terran_Nuclear_Silo";
			if (ut == BWAPI::UnitTypes::Terran_Supply_Depot) return"Terran_Supply_Depot";
			if (ut == BWAPI::UnitTypes::Terran_Refinery) return"Terran_Refinery";
			if (ut == BWAPI::UnitTypes::Terran_Barracks) return"Terran_Barracks";
			if (ut == BWAPI::UnitTypes::Terran_Academy) return"Terran_Academy";
			if (ut == BWAPI::UnitTypes::Terran_Factory) return"Terran_Factory";
			if (ut == BWAPI::UnitTypes::Terran_Starport) return"Terran_Starport";
			if (ut == BWAPI::UnitTypes::Terran_Control_Tower) return"Terran_Control_Tower";
			if (ut == BWAPI::UnitTypes::Terran_Science_Facility) return"Terran_Science_Facility";
			if (ut == BWAPI::UnitTypes::Terran_Covert_Ops) return"Terran_Covert_Ops";
			if (ut == BWAPI::UnitTypes::Terran_Physics_Lab) return"Terran_Physics_Lab";
			if (ut == BWAPI::UnitTypes::Terran_Machine_Shop) return"Terran_Machine_Shop";
			if (ut == BWAPI::UnitTypes::Terran_Engineering_Bay) return"Terran_Engineering_Bay";
			if (ut == BWAPI::UnitTypes::Terran_Armory) return"Terran_Armory";
			if (ut == BWAPI::UnitTypes::Terran_Missile_Turret) return"Terran_Missile_Turret";
			if (ut == BWAPI::UnitTypes::Terran_Bunker) return"Terran_Bunker";
			if (ut == BWAPI::UnitTypes::Special_Crashed_Norad_II) return"Special_Crashed_Norad_II";
			if (ut == BWAPI::UnitTypes::Special_Ion_Cannon) return"Special_Ion_Cannon";
			if (ut == BWAPI::UnitTypes::Powerup_Uraj_Crystal) return"Powerup_Uraj_Crystal";
			if (ut == BWAPI::UnitTypes::Powerup_Khalis_Crystal) return"Powerup_Khalis_Crystal";
			if (ut == BWAPI::UnitTypes::Zerg_Infested_Command_Center) return"Zerg_Infested_Command_Center";
			if (ut == BWAPI::UnitTypes::Zerg_Hatchery) return"Zerg_Hatchery";
			if (ut == BWAPI::UnitTypes::Zerg_Lair) return"Zerg_Lair";
			if (ut == BWAPI::UnitTypes::Zerg_Hive) return"Zerg_Hive";
			if (ut == BWAPI::UnitTypes::Zerg_Nydus_Canal) return"Zerg_Nydus_Canal";
			if (ut == BWAPI::UnitTypes::Zerg_Hydralisk_Den) return"Zerg_Hydralisk_Den";
			if (ut == BWAPI::UnitTypes::Zerg_Defiler_Mound) return"Zerg_Defiler_Mound";
			if (ut == BWAPI::UnitTypes::Zerg_Greater_Spire) return"Zerg_Greater_Spire";
			if (ut == BWAPI::UnitTypes::Zerg_Queens_Nest) return"Zerg_Queens_Nest";
			if (ut == BWAPI::UnitTypes::Zerg_Evolution_Chamber) return"Zerg_Evolution_Chamber";
			if (ut == BWAPI::UnitTypes::Zerg_Ultralisk_Cavern) return"Zerg_Ultralisk_Cavern";
			if (ut == BWAPI::UnitTypes::Zerg_Spire) return"Zerg_Spire";
			if (ut == BWAPI::UnitTypes::Zerg_Spawning_Pool) return"Zerg_Spawning_Pool";
			if (ut == BWAPI::UnitTypes::Zerg_Creep_Colony) return"Zerg_Creep_Colony";
			if (ut == BWAPI::UnitTypes::Zerg_Spore_Colony) return"Zerg_Spore_Colony";
			if (ut == BWAPI::UnitTypes::Zerg_Sunken_Colony) return"Zerg_Sunken_Colony";
			if (ut == BWAPI::UnitTypes::Special_Overmind_With_Shell) return"Special_Overmind_With_Shell";
			if (ut == BWAPI::UnitTypes::Special_Overmind) return"Special_Overmind";
			if (ut == BWAPI::UnitTypes::Zerg_Extractor) return"Zerg_Extractor";
			if (ut == BWAPI::UnitTypes::Special_Mature_Chrysalis) return"Special_Mature_Chrysalis";
			if (ut == BWAPI::UnitTypes::Special_Cerebrate) return"Special_Cerebrate";
			if (ut == BWAPI::UnitTypes::Special_Cerebrate_Daggoth) return"Special_Cerebrate_Daggoth";
			if (ut == BWAPI::UnitTypes::Protoss_Nexus) return"Protoss_Nexus";
			if (ut == BWAPI::UnitTypes::Protoss_Robotics_Facility) return"Protoss_Robotics_Facility";
			if (ut == BWAPI::UnitTypes::Protoss_Pylon) return"Protoss_Pylon";
			if (ut == BWAPI::UnitTypes::Protoss_Assimilator) return"Protoss_Assimilator";
			if (ut == BWAPI::UnitTypes::Protoss_Observatory) return"Protoss_Observatory";
			if (ut == BWAPI::UnitTypes::Protoss_Gateway) return"Protoss_Gateway";
			if (ut == BWAPI::UnitTypes::Protoss_Photon_Cannon) return"Protoss_Photon_Cannon";
			if (ut == BWAPI::UnitTypes::Protoss_Citadel_of_Adun) return"Protoss_Citadel_of_Adun";
			if (ut == BWAPI::UnitTypes::Protoss_Cybernetics_Core) return"Protoss_Cybernetics_Core";
			if (ut == BWAPI::UnitTypes::Protoss_Templar_Archives) return"Protoss_Templar_Archives";
			if (ut == BWAPI::UnitTypes::Protoss_Forge) return"Protoss_Forge";
			if (ut == BWAPI::UnitTypes::Protoss_Stargate) return"Protoss_Stargate";
			if (ut == BWAPI::UnitTypes::Special_Stasis_Cell_Prison) return"Special_Stasis_Cell_Prison";
			if (ut == BWAPI::UnitTypes::Protoss_Fleet_Beacon) return"Protoss_Fleet_Beacon";
			if (ut == BWAPI::UnitTypes::Protoss_Arbiter_Tribunal) return"Protoss_Arbiter_Tribunal";
			if (ut == BWAPI::UnitTypes::Protoss_Robotics_Support_Bay) return"Protoss_Robotics_Support_Bay";
			if (ut == BWAPI::UnitTypes::Protoss_Shield_Battery) return"Protoss_Shield_Battery";
			if (ut == BWAPI::UnitTypes::Special_Khaydarin_Crystal_Form) return"Special_Khaydarin_Crystal_Form";
			if (ut == BWAPI::UnitTypes::Special_Protoss_Temple) return"Special_Protoss_Temple";
			if (ut == BWAPI::UnitTypes::Special_XelNaga_Temple) return"Special_XelNaga_Temple";
			if (ut == BWAPI::UnitTypes::Resource_Mineral_Field) return"Resource_Mineral_Field";
			if (ut == BWAPI::UnitTypes::Resource_Mineral_Field_Type_2) return"Resource_Mineral_Field_Type_2";
			if (ut == BWAPI::UnitTypes::Resource_Mineral_Field_Type_3) return"Resource_Mineral_Field_Type_3";
			if (ut == BWAPI::UnitTypes::Special_Independant_Starport) return"Special_Independant_Starport";
			if (ut == BWAPI::UnitTypes::Resource_Vespene_Geyser) return"Resource_Vespene_Geyser";
			if (ut == BWAPI::UnitTypes::Special_Warp_Gate) return"Special_Warp_Gate";
			if (ut == BWAPI::UnitTypes::Special_Psi_Disrupter) return"Special_Psi_Disrupter";
			if (ut == BWAPI::UnitTypes::Special_Zerg_Beacon) return"Special_Zerg_Beacon";
			if (ut == BWAPI::UnitTypes::Special_Terran_Beacon) return"Special_Terran_Beacon";
			if (ut == BWAPI::UnitTypes::Special_Protoss_Beacon) return"Special_Protoss_Beacon";
			if (ut == BWAPI::UnitTypes::Special_Zerg_Flag_Beacon) return"Special_Zerg_Flag_Beacon";
			if (ut == BWAPI::UnitTypes::Special_Terran_Flag_Beacon) return"Special_Terran_Flag_Beacon";
			if (ut == BWAPI::UnitTypes::Special_Protoss_Flag_Beacon) return"Special_Protoss_Flag_Beacon";
			if (ut == BWAPI::UnitTypes::Special_Power_Generator) return"Special_Power_Generator";
			if (ut == BWAPI::UnitTypes::Special_Overmind_Cocoon) return"Special_Overmind_Cocoon";
			if (ut == BWAPI::UnitTypes::Spell_Dark_Swarm) return"Spell_Dark_Swarm";
			if (ut == BWAPI::UnitTypes::Special_Floor_Missile_Trap) return"Special_Floor_Missile_Trap";
			if (ut == BWAPI::UnitTypes::Special_Floor_Hatch) return"Special_Floor_Hatch";
			if (ut == BWAPI::UnitTypes::Special_Upper_Level_Door) return"Special_Upper_Level_Door";
			if (ut == BWAPI::UnitTypes::Special_Right_Upper_Level_Door) return"Special_Right_Upper_Level_Door";
			if (ut == BWAPI::UnitTypes::Special_Pit_Door) return"Special_Pit_Door";
			if (ut == BWAPI::UnitTypes::Special_Right_Pit_Door) return"Special_Right_Pit_Door";
			if (ut == BWAPI::UnitTypes::Special_Floor_Gun_Trap) return"Special_Floor_Gun_Trap";
			if (ut == BWAPI::UnitTypes::Special_Wall_Missile_Trap) return"Special_Wall_Missile_Trap";
			if (ut == BWAPI::UnitTypes::Special_Wall_Flame_Trap) return"Special_Wall_Flame_Trap";
			if (ut == BWAPI::UnitTypes::Special_Right_Wall_Missile_Trap) return"Special_Right_Wall_Missile_Trap";
			if (ut == BWAPI::UnitTypes::Special_Right_Wall_Flame_Trap) return"Special_Right_Wall_Flame_Trap";
			if (ut == BWAPI::UnitTypes::Special_Start_Location) return"Special_Start_Location";
			if (ut == BWAPI::UnitTypes::Powerup_Flag) return"Powerup_Flag";
			if (ut == BWAPI::UnitTypes::Powerup_Young_Chrysalis) return"Powerup_Young_Chrysalis";
			if (ut == BWAPI::UnitTypes::Powerup_Psi_Emitter) return"Powerup_Psi_Emitter";
			if (ut == BWAPI::UnitTypes::Powerup_Data_Disk) return"Powerup_Data_Disk";
			if (ut == BWAPI::UnitTypes::Powerup_Khaydarin_Crystal) return"Powerup_Khaydarin_Crystal";
			if (ut == BWAPI::UnitTypes::Powerup_Mineral_Cluster_Type_1) return"Powerup_Mineral_Cluster_Type_1";
			if (ut == BWAPI::UnitTypes::Powerup_Mineral_Cluster_Type_2) return"Powerup_Mineral_Cluster_Type_2";
			if (ut == BWAPI::UnitTypes::Powerup_Protoss_Gas_Orb_Type_1) return"Powerup_Protoss_Gas_Orb_Type_1";
			if (ut == BWAPI::UnitTypes::Powerup_Protoss_Gas_Orb_Type_2) return"Powerup_Protoss_Gas_Orb_Type_2";
			if (ut == BWAPI::UnitTypes::Powerup_Zerg_Gas_Sac_Type_1) return"Powerup_Zerg_Gas_Sac_Type_1";
			if (ut == BWAPI::UnitTypes::Powerup_Zerg_Gas_Sac_Type_2) return"Powerup_Zerg_Gas_Sac_Type_2";
			if (ut == BWAPI::UnitTypes::Powerup_Terran_Gas_Tank_Type_1) return"Powerup_Terran_Gas_Tank_Type_1";
			if (ut == BWAPI::UnitTypes::Powerup_Terran_Gas_Tank_Type_2) return"Powerup_Terran_Gas_Tank_Type_2";

			return std::to_string(ut);
		}
	};
}
