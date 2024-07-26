#pragma once

#include <chrono>
#include <thread>

#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MCallbackIdArray.h>
#include <maya/MFnCamera.h>
#include <maya/MPlugArray.h>
#include <maya/MFnAttribute.h>
#include <maya/M3dView.h>

#include "BlockPointGrid.h"
#include "SimpleShapes.h"

/*
	Once a BlockPointGrid is created, the GridManager keeps it alive as long as the plugin is loaded.  This is a singleton
	which is instantiated whenever one of our MPxCommands is called.
*/
class GridManager {

	std::vector<std::shared_ptr<BlockPointGrid>> grids;

	bool display = false;
	bool displayBlockPoints = false;

	GridManager() {

		MGlobal::displayInfo("GridManager created");
	}

public:

	static GridManager& getInstance() {

		static GridManager instance;

		return instance;
	}

	~GridManager() {

		MGlobal::displayInfo("GridManager and grids destroyed");
	}

	void newGrid(double XSIZE, double YSIZE, double ZSIZE, double UNITSIZE, MPoint BASE, double DETECTIONRANGE, double CONERANGEANGLE, double INTENSITY);

	std::size_t gridCount() { return grids.size(); }

	std::shared_ptr<BlockPointGrid> getGrid(unsigned int index, MStatus& status);

	MStatus updateGridDisplay(bool d, double dist, double r, double nc, bool dbp, bool maintain, bool deleteBPs, bool dsu, bool dua, double dpt);

};