
#include <sstream>
#include <algorithm>

#include "ModifyBlockPoints.h"
#include "GridManager.h"
#include "BlockPointGrid.h"
#include "Point_Int.h"

MStatus ModifyBlockPoints::doIt(const MArgList& argList) {

	MStatus status;
	MArgDatabase argData(syntax(), argList, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);
	MSelectionList selectionList;
	MGlobal::getActiveSelectionList(selectionList);

	if (GridManager::getInstance().gridCount() < 1) {

		MGlobal::displayInfo("There is no grid");
		return MS::kSuccess;
	}

	if (argData.isFlagSet("-c") && argData.flagArgumentBool("-c", 0)) {

		status = create(argData);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		return MS::kSuccess;
	}

	return MS::kSuccess;
}

MStatus ModifyBlockPoints::create(const MArgDatabase& argData) {

	MStatus status;
	std::vector<MPoint> locations;
	MArgList coordArgs;
	unsigned int coordCount = argData.numberOfFlagUses("-l");

	if (coordCount > 0) {

		for (unsigned int i = 0; i < coordCount; i += 3) {

			argData.getFlagArgumentList("-l", i, coordArgs);
			argData.getFlagArgumentList("-l", i + 1, coordArgs);
			argData.getFlagArgumentList("-l", i + 2, coordArgs);
			locations.push_back(MPoint(coordArgs.asDouble(i, &status), coordArgs.asDouble(i + 1, &status), coordArgs.asDouble(i + 2, &status)));
		}
	}
	else
		locations.push_back(MPoint(0., 0., 0.));

	double density = argData.flagArgumentDouble("-den", 0);
	double radius = argData.flagArgumentDouble("-rad", 0);

	std::vector<std::shared_ptr<BlockPoint>> newBPs;

	for (auto& l : locations) {

		std::shared_ptr<BlockPoint> bp = nullptr;
		GridManager::getInstance().getGrid(0, status)->addBlockPoint(l, density, radius, bp);
		newBPs.push_back(bp);
		bp->setCurrentUnit(GridManager::getInstance().getGrid(0, status)->pointToIndex(l));
		bp->setGrid(GridManager::getInstance().getGrid(0, status).get());
	}

	MSelectionList originalSelection;
	MGlobal::getActiveSelectionList(originalSelection);

	GridManager::getInstance().getGrid(0, status)->startAuxTimer();
	status = GridManager::getInstance().getGrid(0, status)->applyShade();
	CHECK_MSTATUS_AND_RETURN_IT(status);
	double applyShadeTime = GridManager::getInstance().getGrid(0, status)->getTime();
	MGlobal::displayInfo(MString() + "Apply shade time: " + applyShadeTime);
	GridManager::getInstance().getGrid(0, status)->displayBlockPoints(newBPs);
	GridManager::getInstance().getGrid(0, status)->attachBPCallbacks(newBPs);

	MGlobal::setActiveSelectionList(originalSelection);

	return MS::kSuccess;
}

MSyntax ModifyBlockPoints::newSyntax() {

	MSyntax syntax;

	syntax.addFlag("-c", "-create", MSyntax::kBoolean);
	syntax.addFlag("-e", "-edit", MSyntax::kBoolean);
	syntax.addFlag("-d", "-delete", MSyntax::kBoolean);
	syntax.addFlag("-l", "-locations", MSyntax::kDouble);
	syntax.makeFlagMultiUse("-l");
	syntax.addFlag("-den", "-density", MSyntax::kDouble);
	syntax.addFlag("-rad", "-radius", MSyntax::kDouble);

	syntax.enableEdit(false);
	syntax.enableQuery(false);

	return syntax;
}