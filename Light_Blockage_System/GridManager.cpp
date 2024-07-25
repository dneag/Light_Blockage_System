#include "GridManager.h"

void GridManager::newGrid(double XSIZE, double YSIZE, double ZSIZE, double UNITSIZE, MPoint BASE, double DETECTIONRANGE, double CONERANGEANGLE, double INTENSITY) {

	MSelectionList sel;
	MGlobal::getActiveSelectionList(sel);

	grids.push_back(std::make_shared<BlockPointGrid>(static_cast<int>(grids.size()), XSIZE, YSIZE, ZSIZE, UNITSIZE, BASE, DETECTIONRANGE, CONERANGEANGLE, INTENSITY));

	MGlobal::setActiveSelectionList(sel);
}

std::shared_ptr<BlockPointGrid> GridManager::getGrid(unsigned int index, MStatus& status) {

	if (grids.size() == 0) {

		MGlobal::displayInfo(MString() + "No existing grid.  Creating default grid");
		newGrid(16., 24., 16., .5, MPoint(0., -2., 0.), 3., (MH::PI / 4.), .1);
	}

	if (index >= grids.size()) {

		MGlobal::displayError(MString() + "Error:  Request for grid at index " + index + " (Out of range)");
		status = MS::kFailure;
	}

	status = MS::kSuccess;
	return grids[index];
}

MStatus GridManager::updateGridDisplay(bool d, double dist, double r, double nc, bool dbp, bool maintain, bool deleteBPs, bool dsu, bool dua, double dpt) {

	MStatus status;

	for (auto& g : grids) {

		g->displayAllBlockPoints(dbp);
		g->setDisplayPercentageThreshhold(dpt);
		g->toggleDisplayShadedUnits(dsu);
		g->toggleDisplayShadedUnitArrows(dua);

		if (deleteBPs) {
			g->deleteAllBlockPoints();
			status = g->applyShade();
			CHECK_MSTATUS_AND_RETURN_IT(status);
		}
	}

	display = d;
	displayBlockPoints = dbp;
	maintainBPs = maintain;

	return MS::kSuccess;
}