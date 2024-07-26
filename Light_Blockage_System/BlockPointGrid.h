/*
BlockPointGrid.h

This system is designed to simulate a 3D space in which physical objects block the light from above, influencing
the growth rate and direction of any trees below them.

*/

#pragma once

#include <cmath>
#include <vector>
#include <unordered_set>
#include <set>
#include <memory>
#include <queue>
#include <functional>
#include <ctime>
#include <time.h>

#include <maya/MStreamUtils.h>
#include <maya/MStatus.h>
#include <maya/MDagPath.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MDGModifier.h>
#include <maya/MNodeMessage.h>
#include <maya/MCallbackIdArray.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MPlugArray.h>
#include <maya/MFnTransform.h>
#include <maya/MSelectionList.h>

#include "GridUnit.h"
#include "ShadeVector.h"
#include "BlockPoint.h"
#include "MathHelper.h"
#include "SimpleShapes.h"

class BlockPointGrid {

	MStatus bpgStatus;

	//TreeMakerTimer timer;
	clock_t auxiliaryTimer;

	// Parent group for all grid meshes
	MObject gridGroup;

	// Group to keep unit arrow meshes
	MObject unitArrowMeshGroup;

	// Group to keep unit cube meshes
	MObject unitCubeMeshGroup;

	// Group to keep block point meshes
	MObject bpMeshGroup;

	MCallbackIdArray bpCallbackIds;

	std::vector<Point_Int> unitsOnDisplayByCameraMove;

	std::vector<Point_Int> unitsOnDisplayNearPoints;

	int id = -1;

	enum adjustment { add = 1, subtract = -1 };

	// We want the grid to be represented as centered on the Maya grid.  This means that x and z elements must always be an odd
	// number.  E.g. xSize / xUnitSize is always an odd number.  Also, this means that the center element itself is centered on
	// the Maya grid.  E.g. the x and z coordinates at the center of the center element are 0. and 0.
	std::vector< std::vector< std::vector<GridUnit> > > grid;

	bool displayShadedUnits = false;
	bool displayShadedUnitArrows = false;
	double displayPercentageThreshhold = .01;

	// Hardcoding these for now
	double transparencyTileMapTileSize = .1;
	double uvPadding = transparencyTileMapTileSize * .2;
	double uvOffSet = (transparencyTileMapTileSize * .5) - uvPadding;
	MObject transparencyMaterialShadingGroup;
	MObject defaultShadingGroup;

	double unitSize = 1.;
	int xElements = 0;
	int yElements = 0;
	int zElements = 0;
	MPoint base = { 0.,0.,0. };
	double xIndexOffset = unitSize / 2.;
	double yIndexOffset = 0.;
	double zIndexOffset = unitSize / 2.;

	// The range through which BlockPoints are effective
	double shadeRange = 0.;

	// halfConeAngle is the angle between straight down and the border of the cone in which BlockPoints affect units
	double halfConeAngle = (MH::PI / 4.) + .4;

	double intensity = 0.;

	// GridUnits whose light conditions have changed.  This is checked, handled, and cleared after all blockpoint / segment adjustments have been made for 
	// all trees for a given time loop or after post deformers
	std::unordered_set<GridUnit*> dirtyUnits;

	// Units whose densityIncludingExcess has been modified this iteration
	std::unordered_set<GridUnit*> dirtyDensityUnits;

	// Note that the contactPathIndex and shadeVector for the root ShadeVector should never be used
	std::shared_ptr<ShadeVector> shadeRoot = std::make_shared<ShadeVector>(Point_Int(0, 0, 0));

	std::vector<std::shared_ptr<ShadeVector>> toUnitsInShade;

	// The magnitude of the sum of all vectors in CONTACT_VECTORS that are within the shaded sector, times unit size
	double maximumShade = 0.1;

	double attenuationRate = .1;

	std::vector<std::shared_ptr<BlockPoint>> blockPoints;

	// This vector represents the direction of light in the absence of block points.  Useful when a meristem
	// is ignoring block points
	MVector unblockedDirection = { 0., 1., 0. };

	const MVector unblockedLightDirection = MVector(0., 1., 0.);

	MStatus initiateGrid();

	// Creates a tree data structure where each node is a ShadeVector.  The root is the shadeRoot member.
	void createIndexVectorTree();

	// Propagate from the given shade index, either adding or removing shade.  If add is false, then remove.
	MStatus propagateFrom(ShadeVector* startShadeIndex, std::size_t convergedPaths, Point_Int startUnitIndex, bool add);

	double getIntersectionWithShadeRange(const MVector& vectorToUnit, int timesToDivide) const;

	static void divideCubeToEighths(const MVector& cubeCenter, double size, std::vector<MVector>& subdivisions);

	// Checks that each index is within the range of the grid and output an error message if not
	inline bool indicesAreInRange_showError(int x, int y, int z) const;

	// Adds moveVector to each index in indicesInRadius to form a new set of indices in radius.  Also finds the set difference with respect to both new and old index sets
	MStatus addMoveVectorToBP(BlockPoint& bp, const Point_Int& moveVector, std::vector<Point_Int>& newSetDiff, std::vector<Point_Int>& oldSetDiff);

	// Performs a BFS, radiating from bpUnitIndex to any units whose center's distance from bpLoc is less than radius
	// Consider further optimizing this.  Since the radius doesn't change for a given order, it seems like maybe we can do this only once for each order and store a
	// list of vectors to units within the radius.  Note that bpLoc does change, however, which might mean this optimization could only at best be an approximation.
	std::unordered_set<Point_Int, Point_Int::HashFunction> getIndicesInRadius(const MPoint& bpLoc, const Point_Int bpUnitIndex, const double radius);

	// A list of integer vectors to adjacent units.  Can be used to optimize finding units within a BlockPoint's radius
	const std::vector<Point_Int> UNIT_NEIGHBOR_DIRECTIONS{ {-1,0,0 }, {1,0,0}, {0,-1,0}, {0,1,0}, {0,0,-1}, {0,0,1} };

public:

	BlockPointGrid() {}

	// If x, y, or z size doesn't divide evenly by unit size they will be increased to accomodate
	BlockPointGrid(int id, double XSIZE, double YSIZE, double ZSIZE, double UNITSIZE, const MPoint base, double DETECTIONRANGE, double CONERANGEANGLE, double INTENSITY);

	~BlockPointGrid();

	void setID(int id) { this->id = id; }

	int getID() const { return id; }

	static double HCA_DEFAULT() { return (MH::PI / 3.) + .1; }

	static double INTENSITY_DEFAULT() { return .1; }

	void startAuxTimer() { auxiliaryTimer = clock(); }

	double getTime() const {

		return double(clock() - auxiliaryTimer) / CLOCKS_PER_SEC;
	}

	// Return the index of the grid unit corresponding to the point
	Point_Int pointToIndex(const MPoint& p) const;

	// Checks that each index is within the range of the grid
	inline bool indicesAreOnGrid(int x, int y, int z) const;

	// Creates a new BlockPoint and adjusts any affected units.  
	// The pointer reference is for Segments' pointers to their BlockPoints - they are the only handles to BlockPoints that exist
	// outside of the BlockPointGrid
	MStatus addBlockPoint(const MPoint loc, double bpDensity, double bpRadius, std::shared_ptr<BlockPoint>& ptrForSeg);

	// Moves the passed BlockPoint to the new location.  Subtracts its effects from previously affected units and adds its effects to newly affected ones
	MStatus moveBlockPoint(BlockPoint& bp, const MPoint newLoc);

	// Removes one block point from the grid's blockPoints list and adjusts the density of the unit it occupied
	void deleteBlockPoint(std::shared_ptr<BlockPoint> bp);

	// Calls deleteBlockPoint for all block points in blockPoints. Also deletes the block point's mesh if it has one
	MStatus deleteAllBlockPoints();

	bool hasBlockPoint(std::shared_ptr<BlockPoint>& bp) {

		return std::find(blockPoints.begin(), blockPoints.end(), bp) != blockPoints.end();
	}

	void updateAllUnitsLightDirection();

	// Applies or unapplies shade from any grid units that have had block points added or removed.
	MStatus applyShade();

	// Visit every grid unit and execute `func`, which takes a GridUnit reference as argument
	// startInd will be the first 3 dimensional index
	// range represents the number of units from that index in each dimension
	void traverseRange(Point_Int startInd, Point_Int endInd, std::function<void(GridUnit&)> func);

	template <typename Func>
	void traverse(Func func, MStatus& status) {

		Point_Int start(0, 0, 0);
		Point_Int end(xElements, yElements, zElements);
		this->traverseRange(start, end, func, status);
	}

	// Adds callbacks to the bps passed.  These will alter the grid state when block points are moved or removed
	void attachBPCallbacks(std::vector<std::shared_ptr<BlockPoint>> bps);

	// Triggers when blockpoints are moved in the Maya viewport. 
	static void updateGridFromBPChange(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData);

	// Triggers when blockpoints are deleted in the Maya viewport. 
	static void updateGridAfterBPRemoval(MObject& node, void* clientData);

	// Display the block points passed.
	void displayBlockPoints(std::vector<std::shared_ptr<BlockPoint>> bpsToDisplay);

	// Can be used as a toggle.  If d is true, displays block point meshes, otherwise hides them
	void displayAllBlockPoints(bool d);

	static MVector getObjectTranslation(MObject obj, MStatus& status);

	void setDisplayPercentageThreshhold(double value) { displayPercentageThreshhold = value; }
	void toggleDisplayShadedUnits(bool display);
	void toggleDisplayShadedUnitArrows(bool display);
	void displayShadedUnitIf(GridUnit& unit);
	void displayAffectedUnitArrowIf(GridUnit& unit);

	void makeUnitCubeMesh(GridUnit& unit) {

		unit.makeUnitCube(unitSize, maximumShade, transparencyMaterialShadingGroup);
		MFnDagNode gridGroupDagNodeFn;
		gridGroupDagNodeFn.setObject(unitCubeMeshGroup);
		MObject cubeTransform = unit.getCubeTransformNode();
		gridGroupDagNodeFn.addChild(cubeTransform);
	}

	void makeUnitArrowMesh(GridUnit& unit) {

		unit.makeUnitArrow(unitSize, maximumShade, defaultShadingGroup);
		MFnDagNode gridGroupDagNodeFn;
		gridGroupDagNodeFn.setObject(unitArrowMeshGroup);
		MObject arrowTransform = unit.getArrowTransformNode();
		gridGroupDagNodeFn.addChild(arrowTransform);
	}

	void createTransform(std::string name, MObject& handleForNewTransform, MFnDagNode& parent, MStatus& status) {

		// Create unit arrow mesh group as a transform and add it to the grid group
		MFnDagNode newTransformDagNodeFn;
		handleForNewTransform = newTransformDagNodeFn.create("transform", MObject::kNullObj, &status);
		newTransformDagNodeFn.setName(name.c_str());
		SimpleShapes::lockTransforms(name.c_str());
		parent.addChild(handleForNewTransform);
		CHECK_MSTATUS(status);
	}

};
