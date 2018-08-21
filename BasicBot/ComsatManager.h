#pragma once

#include "Common.h"
#include "MapTools.h"
#include "CommandUtil.h"

namespace MyBot
{
	class ComsatManager
	{
		BWAPI::Position _scan_position;
		int _next_enable_frame;

		const double _scan_dps_offset= 1.0; //min dps (a marine is 6/15)
		const int _scan_radius_offset = 224; //marine sight

		ComsatManager();
		void setScanPosition();
		void setCommand();
		void setCommandForScout();
		void clearScanPosition();
		void setNextEnableFrame(size_t delay_frame);

		BWAPI::Position getScanPositionForScout();

		BWAPI::Position getScanPositionForCombat();

		void setCommandForCombat();

		BWAPI::Position _request_position;
		std::queue<BWAPI::Position> _full_scan_positions;

		int energy_buffer = 200;

		public:
			void update();
			static ComsatManager &	Instance();

			void setRequestPosition(BWAPI::Position pos);
	};
}
