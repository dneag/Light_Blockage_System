#include "UpdateGridDisplay.h"
#include "GridManager.h"

MStatus UpdateGridDisplay::doIt(const MArgList& argList) {

	MStatus status;

	MArgDatabase argData(syntax(), argList, &status);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	if (!argData.isFlagSet("-d") || !argData.isFlagSet("-dis")) {

		MGlobal::displayInfo("Error setting grid display: -d and -dis flags must be set");
		return MS::kFailure;
	}

	bool disp = argData.flagArgumentBool("-d", 0);
	double distance = argData.flagArgumentDouble("-dis", 0);
	double range = argData.flagArgumentDouble("-r", 0);
	double nearClip = argData.flagArgumentDouble("-nc", 0);
	bool displayBPs = argData.flagArgumentBool("-dbp", 0);
	bool maintainBPs = argData.flagArgumentBool("-mtn", 0);
	bool deleteBPs = argData.flagArgumentBool("-dlb", 0);
	bool displayShadedUnits = argData.flagArgumentBool("-dsu", 0);
	bool displayShadedUnitArrows = argData.flagArgumentBool("-dua", 0);
	double displayPercentageThresh = argData.flagArgumentDouble("-dpt", 0);

	MGlobal::displayInfo(MString() + "Updating grid display.\n\tdisplay: " + disp + "\n\tdistance to poi: " + distance + "\n\trange: " + range + "\n\tnear clip: " + nearClip
		+ "\n\tdisplay block points: " + displayBPs + "\n\tmaintain: " + maintainBPs + "\n\tdelete: " + deleteBPs + "\n\tdisplay shaded units: " + displayShadedUnits
		+ "\n\tdisplayShadedUnitArrows: " + displayShadedUnitArrows + "\n\tdisplayPercentageThresh: " + displayPercentageThresh);

	// Newly created objects will be automatically selected if we harden edges, so we have to take steps to preserve the current selection just in case
	MSelectionList originalSelection;
	MGlobal::getActiveSelectionList(originalSelection);

	GridManager::getInstance().updateGridDisplay(disp, distance, range, nearClip, displayBPs, maintainBPs, deleteBPs, displayShadedUnits, displayShadedUnitArrows, displayPercentageThresh);

	MGlobal::setActiveSelectionList(originalSelection);

	return MS::kSuccess;
}

MSyntax UpdateGridDisplay::newSyntax() {

	MSyntax syntax;

	syntax.addFlag("-d", "-display", MSyntax::kBoolean);
	syntax.addFlag("-dis", "-display distance", MSyntax::kDouble);
	syntax.addFlag("-r", "-range", MSyntax::kDouble);
	syntax.addFlag("-nc", "-near clip plane", MSyntax::kDouble);
	syntax.addFlag("-dbp", "-display block points", MSyntax::kBoolean);
	syntax.addFlag("-mtn", "-maintain block points", MSyntax::kBoolean); // If true, block points created by trees will remain on the grid after the tree mesh is created
	syntax.addFlag("-dlb", "-delete block points", MSyntax::kBoolean);
	syntax.addFlag("-dsu", "-display shaded units", MSyntax::kBoolean);
	syntax.addFlag("-dua", "-display shaded unit arrows", MSyntax::kBoolean);
	syntax.addFlag("-dpt", "-display percentage threshhold", MSyntax::kDouble);

	syntax.enableEdit(false);
	syntax.enableQuery(false);

	return syntax;
}