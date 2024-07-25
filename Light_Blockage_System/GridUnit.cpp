
#include <maya/MGlobal.h>
#include <maya/MFnNumericAttribute.h>

#include "GridUnit.h"

void GridUnit::updateLightDirection(double intensity, double maxShade, const MVector& unblockedLightDirection) {

	// If there is no blockage, set the light vector equal to unblockedLightDirection
	if (shadeVectorSum.length() < 0.0001) {

		lightDirection = unblockedLightDirection;
	}
	else {

		const MVector blockageVectorSumDirection = shadeVectorSum.normal();

		double shadePercentage = totalShade / maxShade;
		double angBetween = unblockedLightDirection.angle(blockageVectorSumDirection);

		// For now let's insist that the angle between the blockage direction and current light direction must be greater than 90 degrees to have any effect
		// This also protects against division by zero when calculating angleChangeFactor
		if (angBetween <= (MH::PI * .5))
			return;

		// The angle between the blockage direction and current light direction should not affect the amount of angle change in light direction
		// Also, let's say that the lightDirection will not rotate so that it is less than 90 degrees from the blockage direction
		// In other words, the desired growth direction will be, at most, perpendicular to the blockage direction - it will never face away
		double angleChange = std::min(intensity * shadePercentage, angBetween - (MH::PI * .5));
		double angleChangeFactor = (1. / angBetween) * angleChange; // Needed due to the nature of the following MQuaternion constructor
		MQuaternion lightDirRotation(unblockedLightDirection, blockageVectorSumDirection, angleChangeFactor);

		lightDirection = unblockedLightDirection.rotateBy(lightDirRotation);
	}
}

void GridUnit::makeUnitArrow(double unitSize, double maxShade, MObject& shadingGroup) {

	// Create the arrow mesh
	MVector displayVect(lightDirection.x, lightDirection.y, lightDirection.z);
	displayVect = displayVect.normal() * unitSize;
	arrowTransformNode = SimpleShapes::makeSmallArrow(center, displayVect, name, displayVect.length() * .15);
	currentMeshDirection = displayVect.normal();

	MStatus status;
	MFnDagNode nodeFn;
	nodeFn.setObject(arrowTransformNode);

	for (unsigned int i = 0; i < nodeFn.childCount(); ++i) {
		MObject child = nodeFn.child(i, &status);
		if (status == MStatus::kSuccess && child.hasFn(MFn::kMesh)) {
			arrowShapeNode = child;
			break;
		}
	}

	if (arrowShapeNode.isNull()) {
		MGlobal::displayError("Could not find shape node for unit " + name + " cube mesh");
	}

	// Create channels for Unit Density and Unit Blockage and get handles to each
	MFnDependencyNode arrowFn(arrowTransformNode);
	MFnNumericAttribute attrFn;
	MObject densityAttr = attrFn.create("Unit Density", "ud", MFnNumericData::kFloat);
	attrFn.setKeyable(true);
	attrFn.setStorable(true);
	attrFn.setWritable(true);
	attrFn.setReadable(true);
	arrowFn.addAttribute(densityAttr);

	arrowDensityPlug = arrowFn.findPlug(densityAttr, true);

	setArrowDensityPlug();

	MObject shadeAttr = attrFn.create("Unit Shade", "ub", MFnNumericData::kFloat);
	attrFn.setKeyable(true);
	attrFn.setStorable(true);
	attrFn.setWritable(true);
	attrFn.setReadable(true);
	arrowFn.addAttribute(shadeAttr);

	arrowShadePlug = arrowFn.findPlug(shadeAttr, true);

	setArrowShadePlug(maxShade);

	// Get a handle to the visibility plug for the arrow mesh
	MFnDagNode arrowDagNode(arrowTransformNode, &status);
	arrowVisibilityPlug = arrowDagNode.findPlug("visibility", true, &status);

	SimpleShapes::setObjectMaterial(arrowShapeNode, shadingGroup);
}

MStatus GridUnit::makeUnitCube(double unitSize, double maxShade, MObject& shadingGroup) {

	cubeTransformNode = SimpleShapes::makeCube(center, unitSize, name + "_box");

	MStatus status;
	MFnDagNode nodeFn;
	nodeFn.setObject(cubeTransformNode);

	for (unsigned int i = 0; i < nodeFn.childCount(); ++i) {
		MObject child = nodeFn.child(i, &status);
		if (status == MStatus::kSuccess && child.hasFn(MFn::kMesh)) {
			cubeShapeNode = child;
			break;
		}
	}

	if (cubeShapeNode.isNull()) {
		MGlobal::displayError("Could not find shape node for unit " + name + " cube mesh");
		return MS::kFailure;
	}

	// Create channel for Unit Shade and get a handle to it
	MFnDependencyNode cubeFn(cubeTransformNode);
	MFnNumericAttribute attrFn;

	MObject shadeAttr = attrFn.create("Unit Shade", "ub", MFnNumericData::kFloat);
	attrFn.setKeyable(true);
	attrFn.setStorable(true);
	attrFn.setWritable(true);
	attrFn.setReadable(true);
	cubeFn.addAttribute(shadeAttr);

	cubeShadePlug = cubeFn.findPlug(shadeAttr, true);

	setCubeShadePlug(maxShade);

	// Get a handle to the visibility plug for the cube mesh
	MFnDagNode cubeDagNode(cubeTransformNode, &status);
	cubeVisibilityPlug = cubeDagNode.findPlug("visibility", true, &status);

	SimpleShapes::setObjectMaterial(cubeShapeNode, shadingGroup);

	return MS::kSuccess;
}

MStatus GridUnit::setUVsToTile(double transparencyTileMapTileSize, double maxShade, double uvOffset) const {

	MStatus status;

	MFnMesh fnCube(cubeShapeNode, &status);

	MFloatArray uArray, vArray;
	status = fnCube.getUVs(uArray, vArray);
	if (status != MStatus::kSuccess) {
		MGlobal::displayError("Failed to retrieve UVs. " + status.errorString());
	}

	int shadePercentageAsInt = static_cast<int>((totalShade / maxShade) * 100);
	// If shadePercentageAsInt is 0 then the uv tiles won't be calculated right, so just set it to 1.  It's probably better to just not display any shaded units below
	// 1%, but it could be nice at times just to see how many units are reached.
	shadePercentageAsInt = shadePercentageAsInt == 0 ? 1 : shadePercentageAsInt;
	int uTile = (10 - (shadePercentageAsInt % 10)) % 10;
	int vTile = (shadePercentageAsInt - 1) / 10;
	double uCenter = (uTile * .1) + (transparencyTileMapTileSize * .5);
	double vCenter = (vTile * .1) + (transparencyTileMapTileSize * .5);

	//MGlobal::displayInfo(MString() + "uv tiling info for " + name + " :\tshadePercentageAsInt: " + shadePercentageAsInt + ", uTile " + uTile + ", vTile " + vTile + ", uCenter " + uCenter + ", vCenter " + vCenter);
	for (unsigned int i = 0; i < uArray.length(); i += 4) {

		// bottom left corner
		uArray[i] = uCenter - uvOffset;
		vArray[i] = vCenter - uvOffset;

		// bottom right corner
		uArray[i + 1] = uCenter + uvOffset;
		vArray[i + 1] = vCenter - uvOffset;

		// top right corner
		uArray[i + 2] = uCenter + uvOffset;
		vArray[i + 2] = vCenter + uvOffset;

		// top left corner
		uArray[i + 3] = uCenter - uvOffset;
		vArray[i + 3] = vCenter + uvOffset;
	}

	// Set the updated UV coordinates back to the mesh
	status = fnCube.setUVs(uArray, vArray);
	if (status != MStatus::kSuccess) {
		MGlobal::displayError("Failed to set UVs.");
	}

	// Update the UVs for each face to reflect the changes
	MIntArray uvCounts, uvIds;
	status = fnCube.getAssignedUVs(uvCounts, uvIds);
	if (status != MStatus::kSuccess) {
		MGlobal::displayError("Failed to get assigned UVs.");
	}

	status = fnCube.assignUVs(uvCounts, uvIds);
	if (status != MStatus::kSuccess) {
		MGlobal::displayError("Failed to assign UVs.");
	}

	return MS::kSuccess;
}

MStatus GridUnit::updateArrowMesh() {

	/*MGlobal::displayInfo(MString() + __FUNCTION__ + " unit " + name + " currentMeshDirection: " + currentMeshDirection.x + ", " + currentMeshDirection.y + ", " +
		currentMeshDirection.z + ", lightDirection: " + lightDirection.x + ", " + lightDirection.y + ", " + lightDirection.z);*/
	if (arrowTransformNode.isNull() || currentMeshDirection == lightDirection)
		return MS::kSuccess;

	//MGlobal::displayInfo(MString() + __FUNCTION__ + " unit " + name + " applying rotation");
	MStatus status;
	MQuaternion meshDirRotation(currentMeshDirection, lightDirection);
	MFnTransform arrowFn(arrowTransformNode, &status);
	SimpleShapes::unlockRotates(arrowFn.name());
	arrowFn.rotateBy(meshDirRotation, MSpace::kTransform);
	SimpleShapes::lockRotates(arrowFn.name());
	currentMeshDirection = lightDirection;

	return MS::kSuccess;
}

void GridUnit::applyShadeVector(ShadeVector& sv) {

	// If this shade index is not an ASI for this unit, insert it.  Otherwise, add to its paths
	const auto it = appliedShadeVectors.find(&sv);
	if (it == appliedShadeVectors.end()) {
		//MGlobal::displayInfo(MString() + "adding shade index " + next.toUnit.toMString() + " with " + next.convergedPaths + " to unit " + unit.name);
		appliedShadeVectors.emplace(&sv, sv.convergedPaths);
	}
	else {
		it->second += sv.convergedPaths;
	}

	shadeVectorSum += sv.combinedShadeVector;
	totalShade += sv.combinedShadeVector.length();
	//MGlobal::displayInfo(MString() + "total shade now: " + unit.totalShade + ". csv length: " + next.combinedShadeVector.length() +", base sv length: " + next.shadeVector.length());
}

MStatus GridUnit::unapplyShadeVector(ShadeVector& sv) {

	// This would assume we always have added the correct amount of converged paths for each applied shade index, and that we always remove the correct amount
	const auto it = appliedShadeVectors.find(&sv);
	if (it == appliedShadeVectors.end()) {
		MGlobal::displayError(MString() + "Attempted to remove shade index " + sv.toUnit.toMString() + " from grid unit " + name + " but it did not exist");
		return MS::kFailure;
	}

	it->second -= sv.convergedPaths;
	if (it->second < 0) {
		MGlobal::displayError(MString() + "Removed more paths than existed from applied shade index at grid unit " + name);
		return MS::kFailure;
	}
	if (it->second == 0) {
		//MGlobal::displayInfo(MString() + "Removing shade index " + it->first->toUnit.toMString() + " from unit " + unit.name);
		appliedShadeVectors.erase(it);
	}

	shadeVectorSum -= sv.combinedShadeVector;
	totalShade -= sv.combinedShadeVector.length();

	return MS::kSuccess;
}