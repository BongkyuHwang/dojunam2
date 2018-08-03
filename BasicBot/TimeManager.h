#pragma once

#include "Common.h"
#include <map>
#include <set>
#include <vector>

namespace MyBot{
	class TimeManager
	{
		TimeManager();
		std::map<int, std::set<std::string>> _registeredKeys;

	public:
		double TIME_LIMIT = 55.0;

		/// static singleton 객체를 리턴합니다
		static TimeManager & Instance();
		bool isMyTurn(std::string key, int step_size);
	};
}
