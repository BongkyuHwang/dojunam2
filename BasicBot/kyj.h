//kyj.h
#pragma once
#include "Common.h"
#include <exception>
#include "ExpansionManager.h"
namespace MyBot{
	class Kyj{

	public:
		bool mouseEnable = false;
		static Kyj & Instance();
		void onSendText(std::string text);
	};
}
