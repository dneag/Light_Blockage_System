#pragma once

#include <memory>
#include <map>
#include <unordered_set>
#include <time.h>

#include <maya/MString.h>
#include <maya/MPoint.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>

#include "Point_Int.h"
#include "SimpleShapes.h"

class BlockPointGrid;

class BlockPoint : public std::enable_shared_from_this<BlockPoint> {

	MString name;
	MPoint loc;
	int density = 1;
	double radius = 1.;
	Point_Int gridIndex;
	MObject bpTransformNode;
	std::unordered_set<Point_Int, Point_Int::HashFunction> indicesInRadius;
	// For debugging only. Used to trigger moveBlockPoint in callback
	clock_t timeSinceLastMoved = 0;
	BlockPointGrid* grid = nullptr;
	Point_Int currentUnit = Point_Int(0, 0, 0);

public:

	BlockPoint(const MPoint& LOC, int DENSITY, double RADIUS, Point_Int GRIDINDEX, int number) : loc(LOC), density(DENSITY), radius(RADIUS), gridIndex(GRIDINDEX) {

		std::string n = "bp_" + std::to_string(number);
		name = n.c_str();
	}

	MObject getTransformNode() { return bpTransformNode; }

	Point_Int getGridIndex() const { return gridIndex; }
	void setGridIndex(Point_Int index) { gridIndex = index; }

	std::unordered_set<Point_Int, Point_Int::HashFunction> getIndicesInRadius() { return indicesInRadius; }
	void setIndicesInRadius(std::unordered_set<Point_Int, Point_Int::HashFunction> iir) { indicesInRadius = iir; }

	Point_Int getCurrentUnit() const { return currentUnit; }
	void setCurrentUnit(Point_Int u) { currentUnit = u; }

	double getDensity() const { return density; }
	void setLoc(MPoint p) { loc = p; }

	BlockPointGrid* getGrid() const { return grid; }
	void setGrid(BlockPointGrid* g) { grid = g; }

	std::shared_ptr<BlockPoint> getSharedFromThis() { return shared_from_this(); }

	/*** Display / Debug Tools ***/

	// Create a mesh for the block point and add it as a child of the bp mesh group
	MObject createBPMesh(MFnDagNode& bpMeshGroupDagNodeFn, MObject& shadingGroup);
};