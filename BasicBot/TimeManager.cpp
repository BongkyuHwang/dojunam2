#include "TimeManager.h"

using namespace MyBot;

TimeManager::TimeManager(){

}

TimeManager & TimeManager::Instance()
{
	static TimeManager instance;
	return instance;
}


bool TimeManager::isMyTurn(std::string key, int step_size){
	std::map<int, std::set<std::string>>::iterator it_registeredKeys = _registeredKeys.find(step_size);

	if (it_registeredKeys == _registeredKeys.end()){
		std::set<std::string> obj;
		obj.insert(key);
		_registeredKeys.insert(make_pair(step_size, obj));
		return true;
	}
	else{
		if (it_registeredKeys->second.find(key) == it_registeredKeys->second.end()){
			it_registeredKeys->second.insert(key);
			return true;
		}
	}

	int target_step = 0;
	for (auto it : it_registeredKeys->second){
		if (key == it){
			break;
		}
		target_step++;
		if (target_step == step_size){
			target_step = 0;
		}
	}

	if (BWAPI::Broodwar->getFrameCount() % step_size == target_step){
		return true;
	}

	return false;
}