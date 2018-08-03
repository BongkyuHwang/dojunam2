#pragma once

#include "Common.h"
#include "InformationManager.h"

#include "BuildManager.h"
#include "ConstructionManager.h"
#include "ScoutManager.h"
#include "StrategyManager.h"
#include "UXManager.h"
#include "kyj.h"
#include "CombatCommander.h"
#include "ComsatManager.h"
#include "WorkerManager.h"
#include "ExpansionManager.h"
#include "TimeManager.h"

namespace MyBot
{
	/// 실제 봇프로그램의 본체가 되는 class
	/// 스타크래프트 경기 도중 발생하는 이벤트들이 적절하게 처리되도록 해당 Manager 객체에게 이벤트를 전달하는 관리자 Controller 역할을 합니다
	class GameCommander 
	{
		/// 디버깅용 플래그 : 어느 Manager 가 에러를 일으키는지 알기위한 플래그
		bool isToFindError;

		//@도주남 김지훈 // 전투유닛들의 유닛 셋을 가지고 있는다.
		BWAPI::Unitset          _combatUnits;
		//djn ssh
		BWAPI::Unitset          _scoutUnits;
		//djn ssh
		bool                    _initialScoutSet;

	public:
		//@도주남 김지훈 .// 기존 알버타에선 정찰유닛및 전투유닛 셋팅해 주는 부분이지만 일단 전투유닛만 넣는 것으로 한다.
		void handleUnitAssignments();
		//@도주남 김지훈 .// 전투 플레그가 뜨면 전투 유닛을 셋팅해준다.
		void setCombatUnits();
		//@도주남 김지훈 .//유닛셋에 유닛을 넣어준다.
		void assignUnit(BWAPI::Unit unit, BWAPI::Unitset & set);

		GameCommander();
		~GameCommander();
		
		/// 경기가 시작될 때 일회적으로 발생하는 이벤트를 처리합니다
		void onStart();
		/// 경기가 종료될 때 일회적으로 발생하는 이벤트를 처리합니다
		void onEnd(bool isWinner);
		/// 경기 진행 중 매 프레임마다 발생하는 이벤트를 처리합니다
		void onFrame();

		/// 텍스트를 입력 후 엔터를 하여 다른 플레이어들에게 텍스트를 전달하려 할 때 발생하는 이벤트를 처리합니다
		void onSendText(std::string text);
		/// 다른 플레이어로부터 텍스트를 전달받았을 때 발생하는 이벤트를 처리합니다
		void onReceiveText(BWAPI::Player player, std::string text);

		/// 유닛(건물/지상유닛/공중유닛)이 Create 될 때 발생하는 이벤트를 처리합니다
		void onUnitCreate(BWAPI::Unit unit);
		///  유닛(건물/지상유닛/공중유닛)이 Destroy 될 때 발생하는 이벤트를 처리합니다
		void onUnitDestroy(BWAPI::Unit unit);

		/// 유닛(건물/지상유닛/공중유닛)이 Morph 될 때 발생하는 이벤트를 처리합니다
		/// Zerg 종족의 유닛은 건물 건설이나 지상유닛/공중유닛 생산에서 거의 대부분 Morph 형태로 진행됩니다
		void onUnitMorph(BWAPI::Unit unit);

		/// 유닛(건물/지상유닛/공중유닛)의 소속 플레이어가 바뀔 때 발생하는 이벤트를 처리합니다
		/// Gas Geyser에 어떤 플레이어가 Refinery 건물을 건설했을 때, Refinery 건물이 파괴되었을 때, Protoss 종족 Dark Archon 의 Mind Control 에 의해 소속 플레이어가 바뀔 때 발생합니다
		void onUnitRenegade(BWAPI::Unit unit);
		/// 유닛(건물/지상유닛/공중유닛)의 하던 일 (건물 건설, 업그레이드, 지상유닛 훈련 등)이 끝났을 때 발생하는 이벤트를 처리합니다
		void onUnitComplete(BWAPI::Unit unit);

		/// 유닛(건물/지상유닛/공중유닛)이 Discover 될 때 발생하는 이벤트를 처리합니다
		/// 아군 유닛이 Create 되었을 때 라든가, 적군 유닛이 Discover 되었을 때 발생합니다
		void onUnitDiscover(BWAPI::Unit unit);
		/// 유닛(건물/지상유닛/공중유닛)이 Evade 될 때 발생하는 이벤트를 처리합니다
		/// 유닛이 Destroy 될 때 발생합니다
		void onUnitEvade(BWAPI::Unit unit);

		/// 유닛(건물/지상유닛/공중유닛)이 Show 될 때 발생하는 이벤트를 처리합니다
		/// 아군 유닛이 Create 되었을 때 라든가, 적군 유닛이 Discover 되었을 때 발생합니다
		void onUnitShow(BWAPI::Unit unit);
		/// 유닛(건물/지상유닛/공중유닛)이 Hide 될 때 발생하는 이벤트를 처리합니다
		/// 보이던 유닛이 Hide 될 때 발생합니다
		void onUnitHide(BWAPI::Unit unit);
				//djn ssh
		void setScoutUnits();

		// BasicBot 1.1 Patch Start ////////////////////////////////////////////////
		// onNukeDetect, onPlayerLeft, onSaveGame 이벤트를 처리할 수 있도록 메소드 추가

		/// 핵미사일 발사가 감지되었을 때 발생하는 이벤트를 처리합니다
		void onNukeDetect(BWAPI::Position target);

		/// 다른 플레이어가 대결을 나갔을 때 발생하는 이벤트를 처리합니다
		void onPlayerLeft(BWAPI::Player player);

		/// 게임을 저장할 때 발생하는 이벤트를 처리합니다
		void onSaveGame(std::string gameName);

		//에러위치 찾는 용도
		std::ofstream log_file;
		std::string log_file_path;

		template<typename T>
		void log_write(T s, bool end=false){
			if (!Config::Debug::createTrackingLog) return;

			log_file.open(log_file_path, std::ofstream::out | std::ofstream::app);

			if (log_file.is_open()){
				log_file << s;
				if (end) log_file << std::endl;
			}
			log_file.close();
		}

		std::string history_file_path;
		
		template<typename T>
		void history_write(T s, bool end = false){
			std::ofstream history_file;

			history_file.open(history_file_path, std::ofstream::out | std::ofstream::app);

			if (history_file.is_open()){
				history_file << s;
				if (end) history_file << std::endl;
			}
			history_file.close();
		}

		void initHistory();
		void closeHistory(bool isWinner);
	};

}