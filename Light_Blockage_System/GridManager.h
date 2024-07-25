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

class GridManager {

	inline static MStatus gridStatus;

	std::vector<std::shared_ptr<BlockPointGrid>> grids;

	std::vector<MObject> cameraTransforms;

	MCallbackIdArray cameraCallbacksIds;

	bool display = false;
	bool displayBlockPoints = false;

	// If true, block points created by a tree will not be deleted after its mesh is created
	bool maintainBPs = false;

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

		MMessage::removeCallbacks(cameraCallbacksIds);
	}

	void newGrid(double XSIZE, double YSIZE, double ZSIZE, double UNITSIZE, MPoint BASE, double DETECTIONRANGE, double CONERANGEANGLE, double INTENSITY);

	std::size_t gridCount() { return grids.size(); }

	std::shared_ptr<BlockPointGrid> getGrid(unsigned int index, MStatus& status);

	bool shouldMaintainBPs() const { return maintainBPs; }

	bool isDisplayingBlockPoints() const { return displayBlockPoints; }

	MStatus updateGridDisplay(bool d, double dist, double r, double nc, bool dbp, bool maintain, bool deleteBPs, bool dsu, bool dua, double dpt);

};