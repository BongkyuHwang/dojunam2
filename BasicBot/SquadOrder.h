#pragma once

#include "Common.h"

namespace MyBot
{

	namespace SquadOrderTypes
	{
		enum { None, Idle, Attack, Defend, Regroup, Drop, SquadOrderTypes };
	}

	class SquadOrder
	{
		size_t              _type;
		int                 _radius;
		BWAPI::Position     _position;
		std::string         _status;
		BWAPI::Position     _centerPosition;
		BWAPI::Unit     _closestUnit;
		BWAPI::Unitset	_organicUnits;
		std::pair<BWAPI::Position, BWAPI::Position> _line;
	public:

		SquadOrder()
			: _type(SquadOrderTypes::None)
			, _radius(0)
		{
		}

		SquadOrder(int type, BWAPI::Position position, int radius, std::string status = "Default")
			: _type(type)
			, _position(position)
			, _radius(radius)
			, _status(status)
		{
			_closestUnit = nullptr;
			_organicUnits.clear();
			_line = std::make_pair(BWAPI::Positions::None, BWAPI::Positions::None);
		}

		SquadOrder(int type, BWAPI::Position position, int radius, std::pair<BWAPI::Position, BWAPI::Position> line, std::string status = "Default")
			: _type(type)
			, _position(position)
			, _radius(radius)
			, _status(status)
			, _line(line)
		{
			_closestUnit = nullptr;
			_organicUnits.clear();
		}

		const std::string & getStatus() const
		{
			return _status;
		}

		const BWAPI::Position & getPosition() const
		{
			return _position;
		}

		void setPosition(BWAPI::Position p)
		{
			_position = p;
		}

		const int & getRadius() const
		{
			return _radius;
		}

		const size_t & getType() const
		{
			return _type;
		}

		BWAPI::Position getCenterPosition()
		{
			return _centerPosition;
		}

		void setCenterPosition(BWAPI::Position in)
		{
			_centerPosition = in;
		}

		BWAPI::Unit getClosestUnit()
		{
			return _closestUnit;
		}

		void setClosestUnit(BWAPI::Unit in)
		{
			_closestUnit = in;
		}

		BWAPI::Unitset  getOrganicUnits()
		{
			return _organicUnits;
		}

		void setOrganicUnits(BWAPI::Unitset  in)
		{
			_organicUnits = in;
		}

		std::pair<BWAPI::Position, BWAPI::Position> getLine()
		{
			return _line;
		}
	};
}