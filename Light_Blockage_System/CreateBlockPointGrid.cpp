
#include "CreateBlockPointGrid.h"

MStatus CreateBlockPointGrid::doIt(const MArgList& argList) {

	MStatus status;

	MArgDatabase argData(syntax(), argList, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	if (!argData.isFlagSet("-un")) {

		MGlobal::displayInfo("Error creating bpg: unit size must be set");
		return MS::kFailure;
	}

	double unitSize = argData.flagArgumentDouble("-un", 0);

	if (unitSize <= 0.) {

		MGlobal::displayInfo("Error creating bpg: unit size must be greater than zero");
		return MS::kFailure;
	}

	double xSize;
	double ySize;
	double zSize;

	if (argData.isFlagSet("-gr")) {

		double gridSize = argData.flagArgumentDouble("-gr", 0);
		if (gridSize < unitSize) {

			MGlobal::displayInfo("Error creating bpg: grid size must be >= unit size");
			return MS::kFailure;
		}

		xSize = gridSize;
		ySize = gridSize;
		zSize = gridSize;
	}
	else {

		if (!argData.isFlagSet("-xs") || !argData.isFlagSet("-ys") || !argData.isFlagSet("-zs")) {

			MGlobal::displayInfo("Error creating bpg: if grid size is not set, then x, y, and z sizes must all be set");
			return MS::kFailure;
		}

		xSize = argData.flagArgumentDouble("-xs", 0);
		ySize = argData.flagArgumentDouble("-ys", 0);
		zSize = argData.flagArgumentDouble("-zs", 0);

		if ((xSize <= unitSize) || (ySize <= unitSize) || (zSize <= unitSize)) {

			MGlobal::displayInfo("Error creating bpg: x, y, and z size must be greater than unit size");
			return MS::kFailure;
		}

	}

	MPoint base(0., 0., 0.);
	if (argData.isFlagSet("-b")) {

		MArgList baseCoords;

		status = argData.getFlagArgumentList("-b", 0, baseCoords);
		status = argData.getFlagArgumentList("-b", 1, baseCoords);
		status = argData.getFlagArgumentList("-b", 2, baseCoords);
		status = argData.getFlagArgumentList("-b", 3, baseCoords); // Check for excess arg
		if (baseCoords.length() != 3) {

			MGlobal::displayInfo("Error creating bpg: -b (-base) flag does not have 3 elements");
			return MS::kFailure;
		}

		base.x = baseCoords.asDouble(0, &status);
		base.y = baseCoords.asDouble(1, &status);
		base.z = baseCoords.asDouble(2, &status);
	}

	double shadeRange = argData.isFlagSet("-r") ? argData.flagArgumentDouble("-r", 0) : unitSize * 2.;
	if (shadeRange < unitSize * 2.) {

		MGlobal::displayInfo("Error creating bpg: -dr (-detection range) must be at least unitsize * 2");
		return MS::kFailure;
	}

	double halfConeAngle = argData.isFlagSet("-hca") ? argData.flagArgumentDouble("-hca", 0) : BlockPointGrid::HCA_DEFAULT();
	double intensity = argData.isFlagSet("-i") ? argData.flagArgumentDouble("-i", 0) : BlockPointGrid::INTENSITY_DEFAULT();

	if (GridManager::getInstance().gridCount() == 0) {

		MSelectionList sel;
		MGlobal::getActiveSelectionList(sel);
		GridManager::getInstance().newGrid(xSize, ySize, zSize, unitSize, base, shadeRange, halfConeAngle, intensity);
		MGlobal::setActiveSelectionList(sel);
	}
	else {
		MGlobal::displayError("Grid already exists!");
	}

	return MS::kSuccess;
}


MSyntax CreateBlockPointGrid::newSyntax() {

	MSyntax syntax;

	syntax.addFlag("-un", "-unit size", MSyntax::kDouble);
	syntax.addFlag("-xs", "-x size", MSyntax::kDouble);
	syntax.addFlag("-ys", "-y size", MSyntax::kDouble);
	syntax.addFlag("-zs", "-z size", MSyntax::kDouble);
	syntax.addFlag("-gr", "-grid size", MSyntax::kDouble);
	syntax.addFlag("-b", "-base", MSyntax::kDouble);
	syntax.makeFlagMultiUse("-b");

	// The radius of the spherical sector that is the shade range
	syntax.addFlag("-r", "-radius", MSyntax::kDouble);

	// Half of the angle of the spherical sector that is the shade range
	syntax.addFlag("-hca", "-half cone angle", MSyntax::kDouble);
	syntax.addFlag("-i", "-intensity", MSyntax::kDouble);

	syntax.enableEdit(false);
	syntax.enableQuery(false);

	return syntax;
}