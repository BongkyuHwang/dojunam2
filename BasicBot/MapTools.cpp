#include "MapTools.h"

using namespace MyBot;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MapGrid & MapGrid::Instance()
{
	static MapGrid instance(BWAPI::Broodwar->mapWidth() * 32, BWAPI::Broodwar->mapHeight() * 32, Config::Tools::MAP_GRID_SIZE);
	return instance;
}

MapGrid::MapGrid() 
{
}

MapGrid::MapGrid(int mapWidth, int mapHeight, int cellSize)
	: mapWidth(mapWidth)
	, mapHeight(mapHeight)
	, cellSize(cellSize)
	, cols((mapWidth + cellSize - 1) / cellSize)
	, rows((mapHeight + cellSize - 1) / cellSize)
	, cells(rows * cols)
	, lastUpdated(0)
{
	calculateCellCenters();
}

BWAPI::Position MapGrid::getLeastExplored()
{
	int minSeen = 1000000;
	double minSeenDist = 0;
	int leastRow(0), leastCol(0);

	for (int r = 0; r<rows; ++r)
	{
		for (int c = 0; c<cols; ++c)
		{
			// get the center of this cell
			BWAPI::Position cellCenter = getCellCenter(r, c);

			// don't worry about places that aren't connected to our start locatin
			if (!BWTA::isConnected(BWAPI::TilePosition(cellCenter), BWAPI::Broodwar->self()->getStartLocation()))
			{
				continue;
			}

			BWAPI::Position home(BWAPI::Broodwar->self()->getStartLocation());
			double dist = home.getDistance(getCellByIndex(r, c).center);
			int lastVisited = getCellByIndex(r, c).timeLastVisited;
			if (lastVisited < minSeen || ((lastVisited == minSeen) && (dist > minSeenDist)))
			{
				leastRow = r;
				leastCol = c;
				minSeen = getCellByIndex(r, c).timeLastVisited;
				minSeenDist = dist;
			}
		}
	}

	return getCellCenter(leastRow, leastCol);
}

void MapGrid::calculateCellCenters()
{
	for (int r = 0; r < rows; ++r)
	{
		for (int c = 0; c < cols; ++c)
		{
			GridCell & cell = getCellByIndex(r, c);

			int centerX = (c * cellSize) + (cellSize / 2);
			int centerY = (r * cellSize) + (cellSize / 2);

			// if the x position goes past the end of the map
			if (centerX > mapWidth)
			{
				// when did the last cell start
				int lastCellStart = c * cellSize;

				// how wide did we go
				int tooWide = mapWidth - lastCellStart;

				// go half the distance between the last start and how wide we were
				centerX = lastCellStart + (tooWide / 2);
			}
			else if (centerX == mapWidth)
			{
				centerX -= 50;
			}

			if (centerY > mapHeight)
			{
				// when did the last cell start
				int lastCellStart = r * cellSize;

				// how wide did we go
				int tooHigh = mapHeight - lastCellStart;

				// go half the distance between the last start and how wide we were
				centerY = lastCellStart + (tooHigh / 2);
			}
			else if (centerY == mapHeight)
			{
				centerY -= 50;
			}

			cell.center = BWAPI::Position(centerX, centerY);
			assert(cell.center.isValid());
		}
	}
}

BWAPI::Position MapGrid::getCellCenter(int row, int col)
{
	return getCellByIndex(row, col).center;
}

// clear the vectors in the grid
void MapGrid::clearGrid() {

	for (size_t i(0); i<cells.size(); ++i)
	{
		cells[i].ourUnits.clear();
		cells[i].oppUnits.clear();
	}
}

void MapGrid::onStart(){
	for (BWTA::BaseLocation *i : BWTA::getBaseLocations()){
		_arr_regionVertices.push_back(RegionVertices());
		_arr_regionVertices.back().init(i);
	}
}


// populate the grid with units
void MapGrid::update()
{
	//vertics 표시
	for (auto j : _arr_regionVertices){
		std::vector<BWAPI::Position> _enemyRegionVertices = j.getRegionVertices();
		
		for (size_t i(0); i < _enemyRegionVertices.size(); ++i)
		{
			if (j.getOppositeChock() == _enemyRegionVertices[i]){
				BWAPI::Broodwar->drawCircleMap(_enemyRegionVertices[i], 8, BWAPI::Colors::Red, true);
			}
			else if (j.getSiegeDefence() == _enemyRegionVertices[i]){
				BWAPI::Broodwar->drawCircleMap(_enemyRegionVertices[i], 8, BWAPI::Colors::Blue, true);
			}
			else{
				BWAPI::Broodwar->drawCircleMap(_enemyRegionVertices[i], 4, BWAPI::Colors::Green, false);
			}

			BWAPI::Broodwar->drawTextMap(_enemyRegionVertices[i], "%d", i);
		}
	}

	// clear the grid
	clearGrid();

	//BWAPI::Broodwar->printf("MapGrid info: WH(%d, %d)  CS(%d)  RC(%d, %d)  C(%d)", mapWidth, mapHeight, cellSize, rows, cols, cells.size());

	// add our units to the appropriate cell
	for (auto & unit : BWAPI::Broodwar->self()->getUnits())
	{
		getCell(unit).ourUnits.insert(unit);
		getCell(unit).timeLastVisited = BWAPI::Broodwar->getFrameCount();
	}

	// add enemy units to the appropriate cell
	for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
	{
		if (unit->getHitPoints() > 0)
		{
			getCell(unit).oppUnits.insert(unit);
			getCell(unit).timeLastOpponentSeen = BWAPI::Broodwar->getFrameCount();
		}
	}
}

void MapGrid::getUnitsNear(BWAPI::Unitset & units, BWAPI::Position center, int radius, bool ourUnits, bool oppUnits)
{
	const int x0(std::max((center.x - radius) / cellSize, 0));
	const int x1(std::min((center.x + radius) / cellSize, cols - 1));
	const int y0(std::max((center.y - radius) / cellSize, 0));
	const int y1(std::min((center.y + radius) / cellSize, rows - 1));
	const int radiusSq(radius * radius);
	for (int y(y0); y <= y1; ++y)
	{
		for (int x(x0); x <= x1; ++x)
		{
			int row = y;
			int col = x;

			GridCell & cell(getCellByIndex(row, col));
			if (ourUnits)
			{
				for (auto & unit : cell.ourUnits)
				{
					BWAPI::Position d(unit->getPosition() - center);
					if (d.x * d.x + d.y * d.y <= radiusSq)
					{
						if (!units.contains(unit))
						{
							units.insert(unit);
						}
					}
				}
			}
			if (oppUnits)
			{
				for (auto & unit : cell.oppUnits) if (unit->getType() != BWAPI::UnitTypes::Unknown && unit->isVisible())
				{
					BWAPI::Position d(unit->getPosition() - center);
					if (d.x * d.x + d.y * d.y <= radiusSq)
					{
						if (!units.contains(unit))
						{
							units.insert(unit);
						}
					}
				}
			}
		}
	}
}


int	MapGrid::getCellSize()
{
	return cellSize;
}

int MapGrid::getMapWidth(){
	return mapWidth;
}

int MapGrid::getMapHeight(){
	return mapHeight;
}

int MapGrid::getRows()
{
	return rows;
}

int MapGrid::getCols()
{
	return cols;
}

RegionVertices * MapGrid::getRegionVertices(BWTA::BaseLocation *p_baseLocation){
	RegionVertices *rst = NULL;
	for (unsigned i = 0; i < _arr_regionVertices.size(); i++){
		if (_arr_regionVertices[i].getBaseLocation()->getPosition() == p_baseLocation->getPosition()){
			rst = &_arr_regionVertices[i];
		}
	}
	return rst;
}

MapTools & MapTools::Instance()
{
    static MapTools instance;
    return instance;
}

// constructor for MapTools
MapTools::MapTools()
    : _rows(BWAPI::Broodwar->mapHeight())
    , _cols(BWAPI::Broodwar->mapWidth())
{
    _map    = std::vector<bool>(_rows*_cols,false);
    _units  = std::vector<bool>(_rows*_cols,false);
    _fringe = std::vector<int>(_rows*_cols,0);

    setBWAPIMapData();
}

// return the index of the 1D array from (row,col)
inline int MapTools::getIndex(int row,int col)
{
    return row * _cols + col;
}

bool MapTools::unexplored(DistanceMap & dmap,const int index) const
{
    return (index != -1) && dmap[index] == -1 && _map[index];
}

// resets the distance and fringe vectors, call before each search
void MapTools::reset()
{
    std::fill(_fringe.begin(),_fringe.end(),0);
}

// reads in the map data from bwapi and stores it in our map format
void MapTools::setBWAPIMapData()
{
    // for each row and column
    for (int r(0); r < _rows; ++r)
    {
        for (int c(0); c < _cols; ++c)
        {
            bool clear = true;

            // check each walk tile within this TilePosition
            for (int i=0; i<4; ++i)
            {
                for (int j=0; j<4; ++j)
                {
                    if (!BWAPI::Broodwar->isWalkable(c*4 + i,r*4 + j))
                    {
                        clear = false;
                        break;
                    }

                    if (clear)
                    {
                        break;
                    }
                }
            }

            // set the map as binary clear or not
            _map[getIndex(r,c)] = clear;
        }
    }
}

void MapTools::resetFringe()
{
    std::fill(_fringe.begin(),_fringe.end(),0);
}

int MapTools::getGroundDistance(BWAPI::Position origin,BWAPI::Position destination)
{
    // if we have too many maps, reset our stored maps in case we run out of memory
    if (_allMaps.size() > 20)
    {
        _allMaps.clear();

        //BWAPI::Broodwar->printf("Cleared stored distance map cache");
    }

    // if we haven't yet computed the distance map to the destination
    if (_allMaps.find(destination) == _allMaps.end())
    {
		////std::cout << "we haven't yet" << std::endl;
		
		// if we have computed the opposite direction, we can use that too
        if (_allMaps.find(origin) != _allMaps.end())
        {
			////std::cout << "we have opposite" << std::endl;
			
			return _allMaps[origin][destination];
        }

		////std::cout << "compute it" << std::endl;

        // add the map and compute it
        _allMaps.insert(std::pair<BWAPI::Position,DistanceMap>(destination,DistanceMap()));
        computeDistance(_allMaps[destination],destination);
    }

	////std::cout << "get it" << std::endl;

    // get the distance from the map
    return _allMaps[destination][origin];
}


// computes walk distance from Position P to all other points on the map
void MapTools::computeDistance(DistanceMap & dmap,const BWAPI::Position p)
{
    search(dmap,p.y / 32,p.x / 32);
}

// does the dynamic programming search
void MapTools::search(DistanceMap & dmap,const int sR,const int sC)
{
    // reset the internal variables
    resetFringe();

    // set the starting position for this search
    dmap.setStartPosition(sR,sC);

    // set the distance of the start cell to zero
    dmap[getIndex(sR,sC)] = 0;

    // set the fringe variables accordingly
    int fringeSize(1);
    int fringeIndex(0);
    _fringe[0] = getIndex(sR,sC);
    dmap.addSorted(getTilePosition(_fringe[0]));

    // temporary variables used in search loop
    int currentIndex,nextIndex;
    int newDist;

    // the size of the map
    int size = _rows*_cols;

    // while we still have things left to expand
    while (fringeIndex < fringeSize)
    {
        // grab the current index to expand from the fringe
        currentIndex = _fringe[fringeIndex++];
        newDist = dmap[currentIndex] + 1;

        // search up
        nextIndex = (currentIndex > _cols) ? (currentIndex - _cols) : -1;
        if (unexplored(dmap,nextIndex))
        {
            // set the distance based on distance to current cell
            dmap.setDistance(nextIndex,newDist);
            dmap.setMoveTo(nextIndex,'D');
            dmap.addSorted(getTilePosition(nextIndex));

            // put it in the fringe
            _fringe[fringeSize++] = nextIndex;
        }

        // search down
        nextIndex = (currentIndex + _cols < size) ? (currentIndex + _cols) : -1;
        if (unexplored(dmap,nextIndex))
        {
            // set the distance based on distance to current cell
            dmap.setDistance(nextIndex,newDist);
            dmap.setMoveTo(nextIndex,'U');
            dmap.addSorted(getTilePosition(nextIndex));

            // put it in the fringe
            _fringe[fringeSize++] = nextIndex;
        }

        // search left
        nextIndex = (currentIndex % _cols > 0) ? (currentIndex - 1) : -1;
        if (unexplored(dmap,nextIndex))
        {
            // set the distance based on distance to current cell
            dmap.setDistance(nextIndex,newDist);
            dmap.setMoveTo(nextIndex,'R');
            dmap.addSorted(getTilePosition(nextIndex));

            // put it in the fringe
            _fringe[fringeSize++] = nextIndex;
        }

        // search right
        nextIndex = (currentIndex % _cols < _cols - 1) ? (currentIndex + 1) : -1;
        if (unexplored(dmap,nextIndex))
        {
            // set the distance based on distance to current cell
            dmap.setDistance(nextIndex,newDist);
            dmap.setMoveTo(nextIndex,'L');
            dmap.addSorted(getTilePosition(nextIndex));

            // put it in the fringe
            _fringe[fringeSize++] = nextIndex;
        }
    }
}

const std::vector<BWAPI::TilePosition> & MapTools::getClosestTilesTo(BWAPI::Position pos)
{
    // make sure the distance map is calculated with pos as a destination
    int a = getGroundDistance(BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation()),pos);

    return _allMaps[pos].getSortedTiles();
}

BWAPI::TilePosition MapTools::getTilePosition(int index)
{
    return BWAPI::TilePosition(index % _cols,index / _cols);
}

void MapTools::parseMap()
{
    //BWAPI::Broodwar->printf("Parsing Map Information");
    std::ofstream mapFile;
    std::string file = "c:\\scmaps\\" + BWAPI::Broodwar->mapName() + ".txt";
    mapFile.open(file.c_str());

    mapFile << BWAPI::Broodwar->mapWidth()*4 << "\n";
    mapFile << BWAPI::Broodwar->mapHeight()*4 << "\n";

    for (int j=0; j<BWAPI::Broodwar->mapHeight()*4; j++) 
    {
        for (int i=0; i<BWAPI::Broodwar->mapWidth()*4; i++) 
        {
            if (BWAPI::Broodwar->isWalkable(i,j)) 
            {
                mapFile << "0";
            }
            else 
            {
                mapFile << "1";
            }
        }

        mapFile << "\n";
    }

    //BWAPI::Broodwar->printf(file.c_str());

    mapFile.close();
}

BWAPI::TilePosition MapTools::getNextExpansion()
{
	return _selectNextExpansion(getNextExpansions());
}

BWAPI::TilePosition MapTools::getNextExpansion(BWAPI::TilePosition &exceptPosition)
{
	std::vector<BWAPI::TilePosition> &expansions = getNextExpansions();
	for (size_t i = 0; i < expansions.size(); i++){
		if (expansions[i] == exceptPosition){
			expansions.erase(expansions.begin() + i);
			break;
		}
	}
	return _selectNextExpansion(expansions);
}

std::vector<BWAPI::TilePosition> MapTools::getNextExpansions()
{
	BWTA::BaseLocation * closestBase = nullptr;
	double minDistance = 100000;

	std::vector<std::pair<double, BWAPI::TilePosition>> candidate_pos;
	BWAPI::Player player = InformationManager::Instance().selfPlayer;
	BWAPI::Player enemy = InformationManager::Instance().enemyPlayer;
	BWAPI::TilePosition homeTile = player->getStartLocation();

	// for each base location
	for (BWTA::BaseLocation * base : BWTA::getBaseLocations())
	{
		//우리본진, 적진, 적 앞마당은 멀티에서 제외
		if (base == BWTA::getStartLocation(player)) continue;

		if (InformationManager::Instance().getMainBaseLocation(enemy) != nullptr &&
			InformationManager::Instance().getMainBaseLocation(enemy)->getTilePosition() == base->getTilePosition()){
			continue;
		}

		if (InformationManager::Instance().getFirstExpansionLocation(enemy) != nullptr &&
			InformationManager::Instance().getFirstExpansionLocation(enemy)->getTilePosition() == base->getTilePosition()){
			continue;
		}

		// if the base has gas
		//if (!base->isMineralOnly())
		if (1)
		{
			// get the tile position of the base
			BWAPI::TilePosition tile = base->getTilePosition();
			bool buildingInTheWay = false;

			for (int x = 0; x < BWAPI::Broodwar->self()->getRace().getCenter().tileWidth(); ++x)
			{
				for (int y = 0; y < BWAPI::Broodwar->self()->getRace().getCenter().tileHeight(); ++y)
				{
					BWAPI::TilePosition tp(tile.x + x, tile.y + y);

					for (auto & unit : BWAPI::Broodwar->getUnitsOnTile(tp))
					{
						if (unit->getType().isBuilding() && !unit->isFlying())
						{
							buildingInTheWay = true;
							break;
						}
					}
				}
			}

			//베이스 중에서 유닛이 있거나 건물이 막고 있으면 못지음
			if (buildingInTheWay)
			{
				continue;
			}

			// the base's distance from our main nexus
			BWAPI::Position myBasePosition(player->getStartLocation());
			BWAPI::Position thisTile = BWAPI::Position(tile);
			double distanceFromHome = MapTools::Instance().getGroundDistance(thisTile, myBasePosition);

			// if it is not connected, continue
			// 섬에는 못지음(내 생각)
			if (!BWTA::isConnected(homeTile, tile) || distanceFromHome < 0)
			{
				continue;
			}

			//가능한 베이스지역 중에서 최소값을 구한다.
			/*
			if (!closestBase || distanceFromHome < minDistance)
			{
				closestBase = base;
				minDistance = distanceFromHome;
				candidate_pos.push_front(base->getTilePosition());
			}
			else{
				candidate_pos.push_back(base->getTilePosition());
			}
			*/
			candidate_pos.push_back(std::make_pair(distanceFromHome, base->getTilePosition()));
		}
	}

	//최소거리로 정렬
	std::sort(candidate_pos.begin(), candidate_pos.end(), 
		[](std::pair<double, BWAPI::TilePosition> &a, std::pair<double, BWAPI::TilePosition> &b){ return a.first < b.first; });
	std::vector<BWAPI::TilePosition> result;
	for (auto &i : candidate_pos) result.push_back(i.second);
	return result;
}

bool MapTools::isStartLocation(BWAPI::TilePosition &tp){
	for (auto start : BWTA::getStartLocations()){
		if (tp == start->getTilePosition()){
			return true;
		}
	}
	return false;
}

BWAPI::TilePosition MapTools::_selectNextExpansion(std::vector<BWAPI::TilePosition> &positions){
	/*
	//std::cout << "start pt:" << BWTA::getStartLocation(InformationManager::Instance().selfPlayer)->getTilePosition() << std::endl;
	if (InformationManager::Instance().getFirstExpansionLocation(InformationManager::Instance().selfPlayer) != nullptr)
		//std::cout << "first multi:" << InformationManager::Instance().getFirstExpansionLocation(InformationManager::Instance().selfPlayer)->getTilePosition() << std::endl;

	//std::cout << "multi point candiates:";
	for (auto &tmp : positions){
		//std::cout << "(" << tmp.x << "," << tmp.y << "," << isStartLocation(tmp) << ") / ";
	}
	//std::cout << std::endl;
	*/

	if (!positions.empty()){
		BWAPI::TilePosition rst = BWAPI::TilePositions::None;

		//@도주남 김유진 두번째 멀티할때는 스타트포인트에서 찾는다. 헌터만 적용
		if (InformationManager::Instance().getMapName() == 'H'){
			int tmpSize = ExpansionManager::Instance().getExpansions().size();
			if (tmpSize > 1 && (tmpSize % 2 == 0)){
				for (auto &expansion_position : positions){
					if (isStartLocation(expansion_position)){
						rst = expansion_position;
						break;
					}
					else{
						continue;
					}
				}
			}
		}
		else{
			int tmpSize = ExpansionManager::Instance().getExpansions().size();
			if (tmpSize > 2 && ((tmpSize-1) % 2 == 0)){
				for (auto &expansion_position : positions){
					if (isStartLocation(expansion_position)){
						rst = expansion_position;
						break;
					}
					else{
						continue;
					}
				}
			}
		}

		if (rst == BWAPI::TilePosition(25, 89) && InformationManager::Instance().getMapName() == 'H')
			rst = BWAPI::TilePosition(24, 89);

		//일반적인 경우는 제일 가까운 멀티선정
		if (rst == BWAPI::TilePositions::None){
			if (positions.front() == BWAPI::TilePosition(25, 89) && InformationManager::Instance().getMapName() == 'H')
			{
				//std::cout << "TilePosition(24, 89)" << std::endl;
				return BWAPI::TilePosition(24, 89);
			}
			else
				return positions.front();
		}
		else{
			return rst;
		}

	}
	else{
		return BWAPI::TilePositions::None;
	}
}

RegionVertices::RegionVertices() :
	_thisBaseLocation(NULL), 
	_oppositeChock(BWAPI::Positions::None),
	_siegeDefence(BWAPI::Positions::None)
{
}


void RegionVertices::init(BWTA::BaseLocation *baseLocation)
{
	_thisBaseLocation = baseLocation;

	BWTA::Region * baseRegion = baseLocation->getRegion();

	if (!baseRegion)
	{
		//std::cout << "return!!!" << std::endl;
		return;
	}

	const BWAPI::Position basePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
	const std::vector<BWAPI::TilePosition> & closestTobase = MapTools::Instance().getClosestTilesTo(basePosition);

	std::set<BWAPI::Position> unsortedVertices;

	// check each tile position
	for (size_t i(0); i < closestTobase.size(); ++i)
	{
		const BWAPI::TilePosition & tp = closestTobase[i];

		if (BWTA::getRegion(tp) != baseRegion)
		{
			continue;
		}

		// a tile is 'surrounded' if
		// 1) in all 4 directions there's a tile position in the current region
		// 2) in all 4 directions there's a buildable tile
		bool surrounded = true;
		if (BWTA::getRegion(BWAPI::TilePosition(tp.x + 1, tp.y)) != baseRegion || !BWAPI::Broodwar->isBuildable(BWAPI::TilePosition(tp.x + 1, tp.y))
			|| BWTA::getRegion(BWAPI::TilePosition(tp.x, tp.y + 1)) != baseRegion || !BWAPI::Broodwar->isBuildable(BWAPI::TilePosition(tp.x, tp.y + 1))
			|| BWTA::getRegion(BWAPI::TilePosition(tp.x - 1, tp.y)) != baseRegion || !BWAPI::Broodwar->isBuildable(BWAPI::TilePosition(tp.x - 1, tp.y))
			|| BWTA::getRegion(BWAPI::TilePosition(tp.x, tp.y - 1)) != baseRegion || !BWAPI::Broodwar->isBuildable(BWAPI::TilePosition(tp.x, tp.y - 1)))
		{
			surrounded = false;
		}

		// push the tiles that aren't surrounded
		if (!surrounded && BWAPI::Broodwar->isBuildable(tp))
		{
			if (Config::Debug::DrawScoutInfo)
			{
				int x1 = tp.x * 32 + 2;
				int y1 = tp.y * 32 + 2;
				int x2 = (tp.x + 1) * 32 - 2;
				int y2 = (tp.y + 1) * 32 - 2;

				//BWAPI::Broodwar->drawTextMap(x1 + 3, y1 + 2, "%d", MapTools::Instance().getGroundDistance(BWAPI::Position(tp), basePosition));
				//BWAPI::Broodwar->drawBoxMap(x1, y1, x2, y2, BWAPI::Colors::Green, false);
			}

			unsortedVertices.insert(BWAPI::Position(tp) + BWAPI::Position(16, 16));
		}
	}


	std::vector<BWAPI::Position> sortedVertices;
	BWAPI::Position current;
	for (auto i : unsortedVertices){
		current = i;
		break;
	}

	_regionVertices.push_back(current);
	unsortedVertices.erase(current);

	// while we still have unsorted vertices left, find the closest one remaining to current
	while (!unsortedVertices.empty())
	{
		double bestDist = 1000000;
		BWAPI::Position bestPos;

		for (const BWAPI::Position & pos : unsortedVertices)
		{
			double dist = pos.getDistance(current);

			if (dist < bestDist)
			{
				bestDist = dist;
				bestPos = pos;
			}
		}

		current = bestPos;
		sortedVertices.push_back(bestPos);
		unsortedVertices.erase(bestPos);
	}

	// let's close loops on a threshold, eliminating death grooves
	int distanceThreshold = 100;

	while (true)
	{
		// find the largest index difference whose distance is less than the threshold
		int maxFarthest = 0;
		int maxFarthestStart = 0;
		int maxFarthestEnd = 0;

		// for each starting vertex
		for (int i(0); i < (int)sortedVertices.size(); ++i)
		{
			int farthest = 0;
			int farthestIndex = 0;

			// only test half way around because we'll find the other one on the way back
			for (size_t j(1); j < sortedVertices.size() / 2; ++j)
			{
				int jindex = (i + j) % sortedVertices.size();

				if (sortedVertices[i].getDistance(sortedVertices[jindex]) < distanceThreshold)
				{
					farthest = j;
					farthestIndex = jindex;
				}
			}

			if (farthest > maxFarthest)
			{
				maxFarthest = farthest;
				maxFarthestStart = i;
				maxFarthestEnd = farthestIndex;
			}
		}

		// stop when we have no long chains within the threshold
		if (maxFarthest < 4)
		{
			break;
		}

		double dist = sortedVertices[maxFarthestStart].getDistance(sortedVertices[maxFarthestEnd]);

		std::vector<BWAPI::Position> temp;

		for (size_t s(maxFarthestEnd); s != maxFarthestStart; s = (s + 1) % sortedVertices.size())
		{
			temp.push_back(sortedVertices[s]);
		}

		sortedVertices = temp;
	}

	_regionVertices = sortedVertices;

	double tmpMaxDist = 0.0;
	double tmpMinDist = 10000000.0;
	double offsetChoke = 1.0;
	double offsetBase = 1.0;
	BWAPI::Position specPosition(BWAPI::Positions::None);

	if (InformationManager::Instance().getMapName() == 'F'){
		offsetChoke = 1.5;
	}
	else if (InformationManager::Instance().getMapName() == 'L'){
		if (_thisBaseLocation->getPosition().x == 928 && _thisBaseLocation->getPosition().y == 3824){
			specPosition.x = 1408;
			specPosition.y = 3232;
		}
		else if (_thisBaseLocation->getPosition().x == 1888 && _thisBaseLocation->getPosition().y == 240){
			offsetBase = 1.6;
		}
		else{
			offsetChoke = 1.3;
		}
		//std::cout << _thisBaseLocation->getPosition() << std::endl;
	}

	BWTA::Chokepoint * nearestChoke = BWTA::getNearestChokepoint(_thisBaseLocation->getPosition());

	for (auto i : _regionVertices){
		if (specPosition != BWAPI::Positions::None){
			double distToSpec = i.getDistance(specPosition);
			if (distToSpec < tmpMinDist){
				tmpMinDist = distToSpec;
				_oppositeChock = i;
			}
		}
		else{
			double distToBase = i.getDistance(_thisBaseLocation->getPosition());

			double distToChock = 0.0;
			if (nearestChoke != NULL){
				distToChock = i.getDistance(nearestChoke->getCenter());
			}

			if (tmpMaxDist < distToBase*offsetBase + distToChock*offsetChoke){
				tmpMaxDist = distToBase*offsetBase + distToChock*offsetChoke;
				_oppositeChock = i;
			}
		}
	}

	if (BWTA::getStartLocation(BWAPI::Broodwar->self())->getPosition() == _thisBaseLocation->getPosition()){
		BWAPI::Position secondChoke(InformationManager::Instance().getSecondChokePoint(InformationManager::Instance().selfPlayer)->getCenter());

		double tmpMinDist = 100000000.0;
		for (auto i : _regionVertices){
			double distTo2nd = i.getDistance(secondChoke);

			if (tmpMinDist > distTo2nd){
				tmpMinDist = distTo2nd;
				_siegeDefence = i;
			}
		}
	}


	//std::cout << "$ finished to calculate vertices - center:" << _thisBaseLocation->getPosition() << ", size:" << _regionVertices.size() << std::endl;
}

void RegionVertices::getRegionVertices(std::vector<BWAPI::Position> &rv){
	rv = _regionVertices;
	return;
}

std::vector<BWAPI::Position> & RegionVertices::getRegionVertices(){
	return _regionVertices;
}

BWTA::BaseLocation * RegionVertices::getBaseLocation(){
	return _thisBaseLocation;
}

BWAPI::Position RegionVertices::getOppositeChock(){
	return _oppositeChock;
}

BWAPI::Position RegionVertices::getSiegeDefence(){
	return _siegeDefence;
}