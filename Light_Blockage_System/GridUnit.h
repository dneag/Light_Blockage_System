/*
	A GridUnit is the unit of the BlockPointGrid.  Each unit stores information representing the light conditions in its volume.
*/

#pragma once

#include <algorithm>
#include <unordered_map>

#include <maya/MPlug.h>
#include <maya/MTypes.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnSet.h>
#include <maya/MQuaternion.h>
#include <maya/MFnMesh.h>
#include <maya/MFnTransform.h>

#include "Point_Int.h"
#include "MathHelper.h"
#include "ShadeVector.h"
#include "SimpleShapes.h"

class GridUnit {

	MString name;

	MPoint center = { 0.,0.,0. };

	Point_Int gridIndex = { 0, 0, 0 };

	double totalShade = 0.;

	// Unit vector representing the direction towards the most light
	MVector lightDirection = MVector(0., 1., 0.);

	// The sum of all shade vectors affecting this unit.  
	MVector shadeVectorSum = MVector(0., 0., 0.);

	// These represent the shade indices applied to this unit.  The keys are the shade vectors and the values
	// represent the number of the shade vector's paths that have reached the unit
	std::unordered_map<ShadeVector*, std::size_t> appliedShadeVectors;

	// The sum of all block points' densities within this unit.  This value can fall outside of the 0 - 1 range, however, when it is used
	// to block other units it is always clamped between 0 - 1.
	int densityIncludingExcess = 0;

	// The density of the unit, capped at 1.
	int effectiveDensity = 0;

	bool blocked = false;

	/*** Members for debugging ***/

	// The current direction that this unit's arrow mesh points.  This only needs to match light direction when the mesh is displayed,
	// so it is updated every time the mesh is rotated, not necessarily every time light direction changes
	MVector currentMeshDirection = MVector(0., 1., 0.);

	// Handle to the arrow mesh for the unit
	MObject arrowTransformNode;
	MObject arrowShapeNode;

	MObject cubeTransformNode;
	MObject cubeShapeNode;

	// Plugs are used to access and modify channels of the unit's mesh which we will use to display the unit and its density and shade values
	MPlug arrowDensityPlug;
	MPlug arrowShadePlug;
	MPlug arrowVisibilityPlug;
	MPlug cubeShadePlug;
	MPlug cubeVisibilityPlug;


public:

	GridUnit(MString name, double cX, double cY, double cZ, Point_Int index) {

		this->name = name;
		center.x = cX;
		center.y = cY;
		center.z = cZ;
		gridIndex = index;
	}

	Point_Int getGridIndex() const { return gridIndex; }

	// Set the x, y, z values to the index values of the resulting grid unit
	void getIndexAtUnit(const Point_Int& toUnit, int& x, int& y, int& z) const {

		Point_Int unit = gridIndex + toUnit;
		x = unit.x, y = unit.y, z = unit.z;
	}

	MVector getLightDirection() const { return lightDirection; }

	// Must only be used after blockpoints have been updated for all trees per time loop iteration or after post deformers
	void updateLightDirection(double intensity, double maxBlockage, const MVector& unblockedLightDirection);

	MPoint getCenter() const { return center; }
	void setCenter(MPoint c) { center = c; }

	double getTotalShade() const { return totalShade; }
	void setTotalShade(double s) { totalShade = s; }

	std::unordered_map<ShadeVector*, std::size_t>& getAppliedShadeVectors() { return appliedShadeVectors; }
	void applyShadeVector(ShadeVector& sv);

	MStatus unapplyShadeVector(ShadeVector& sv);

	bool isBlocked() const { return blocked; }
	void setBlocked(bool b) { blocked = b; }

	void adjustDensityIncludingExcess(int adj) { densityIncludingExcess += adj; }
	void checkDensity(MStatus& status) const {

		if (densityIncludingExcess < 0) {
			MGlobal::displayError(MString() + "Error: unit " + name + " has densityIncludingExcess less than 0: " + densityIncludingExcess);
			status = MS::kFailure;
		}
	}

	// Update the effectiveDensity and return the difference from the previous value
	double updateDensity() {

		int newEffectiveDensity = std::min(densityIncludingExcess, 1);
		int densityChange = newEffectiveDensity - effectiveDensity;
		effectiveDensity = newEffectiveDensity;

		return densityChange;
	}

	MObject getCubeTransformNode() const { return cubeTransformNode; }
	void setCubeVisibility(bool v) {

		if (!cubeTransformNode.isNull())
			cubeVisibilityPlug.setValue(v);
	}

	void setCubeShadePlug(double maxShade) {

		cubeShadePlug.setLocked(false);
		cubeShadePlug.setValue(totalShade / maxShade);
		cubeShadePlug.setLocked(true);
	}

	void setArrowDensityPlug() {

		if (!arrowDensityPlug.isNull()) {

			arrowDensityPlug.setLocked(false);
			arrowDensityPlug.setValue(std::min(densityIncludingExcess, 1));
			arrowDensityPlug.setLocked(true);
		}
	}

	void setArrowShadePlug(double maxShade) {

		arrowShadePlug.setLocked(false);
		arrowShadePlug.setValue(totalShade / maxShade);
		arrowShadePlug.setLocked(true);
	}

	MObject getArrowTransformNode() const { return arrowTransformNode; }
	void setArrowVisibility(bool v) {

		if (!arrowTransformNode.isNull())
			arrowVisibilityPlug.setValue(v);
	}

	bool arrowMeshIsVisible() const {

		if (!arrowVisibilityPlug.isNull()) {

			bool visible;
			arrowVisibilityPlug.getValue(visible);
			return visible;
		}
		else
			return false;
	}

	void makeUnitArrow(double unitSize, double maxBlockage, MObject& shadingGroup);

	// Check if currentMeshDirection is different than lightDirection and update it if necessary
	MStatus updateArrowMesh();

	MStatus makeUnitCube(double unitSize, double maxBlockage, MObject& shadingGroup);

	MStatus setUVsToTile(double transparencyTileMapTileSize, double maxShade, double uvOffset) const;
};