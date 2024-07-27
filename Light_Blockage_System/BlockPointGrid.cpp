/*
BlockPointGrid.cpp
*/

#include <math.h>
#include <map>
#include "BlockPointGrid.h"

void BlockPointGrid::createIndexVectorTree() {

	// Index vectors to neighboring units on sides and below, including diagonals.  Note that the order of these is significant as it is the order that
	// we will search for units blocked by another unit.
	std::vector<Point_Int> VECTORS_TO_NEIGHBORS = {
		{0,-1,0},
		{-1,0,-1},
		{-1,0,1},
		{1,0,-1},
		{1,0,1},
		{-1,-1,-1},
		{-1,-1,1},
		{1,-1,-1},
		{1,-1,1},
		{-1,0,0},
		{0,0,-1},
		{0,0,1},
		{1,0,0},
		{-1,-1,0},
		{0,-1,-1},
		{0,-1,1},
		{1,-1,0}
	};

	/*
	* Before creating the ShadeVectors, we define PathInfo as a utility for precalculating some of their attributes.
	* 
	* The strength of a ShadeVector (shadeStrength) is intended to represent the volume within shade range beyond the unit it points to.
	* However, the full strength is not always applied, because each ShadeVector can be reached via multiple shortest paths through the tree from shadeRoot,
	* and, during runtime, some of these paths may be obstructed by other occupied grid units. The shade applied by a ShadeVector should be proportional
	* to the number of unobstructed paths. To make this happen, we set the length of their shadeVector as shadeStrength divided by the number of shortest
	* paths. Then, when propagating shade, each of these paths is followed separately, and the shadeVector is only applied if it is clear.
	* 
	* Each PathInfo is associated with an individual ShadeVector, and is used to determine the length and number of shortest paths to it so that we can 
	* precalculate ShadeVectors' shadeVectors accordingly. Once the paths are established, we use computeShadeStrength() to set the shadeStrength and
	* shadeVector.
	*/
	struct PathInfo {

		std::shared_ptr<ShadeVector> sv;
		double shortestPathLength = std::numeric_limits<double>::infinity();

		// The number of paths to the ShadeVector, including paths with overlapping segments
		int totalPaths = 0;
		std::vector<std::shared_ptr<ShadeVector>> parents;

		// The volume of the unit pointed to by this ShadeVector that lies within the shade range.
		// This is needed to get an accurate measure of those units that only partially intersect with the shade range.
		double shadedVolume = 0.;

		PathInfo() {}

		PathInfo(std::shared_ptr<ShadeVector> sv, double shortestPathLength, int totalPaths, std::shared_ptr<ShadeVector> incomingFrom, double shadedVolume)
			: sv(sv), shortestPathLength(shortestPathLength), totalPaths(totalPaths), shadedVolume(shadedVolume) {

			parents.push_back(incomingFrom);
		}

		void replaceIncoming(double newShortestPathLength, std::shared_ptr<ShadeVector> newBlocker, int totalPaths) {

			for (auto& si : parents) {

				si->eraseBlockee(sv);
			}

			this->totalPaths = totalPaths;
			parents.clear();
			parents.push_back(newBlocker);
			shortestPathLength = newShortestPathLength;
		}

		/*
		* Computes the final values of the shadeStrength and shadeVector for each ShadeVector.
		* 
		* Each ShadeVector points to a unit that lies between the shadeRoot and some sub-sector within shade range.  The strength of a ShadeVector (shadeStrength)
		* is intended to represent the volume of this sub-sector, including the volume of the unit it points to.  This is the volume that it 'blocks'. We could
		* use the formula for the volume of a spherical sector to get a precise value for this, however, when propagating shade, applying more or less of this value
		* depending on the grid state becomes more complicated. Instead, we will get an approximation to this value by visiting descendant nodes from each ShadeVector
		* and summing their volumes.  We also consider whether descendants have multiple parents, in which case, only the appropriate fraction of that descendant's
		* volume is added.
		*/
		static void computeShadeStrength(PathInfo* p, double& shadeBelow, std::unordered_map<Point_Int, PathInfo, Point_Int::HashFunction>& pathInfos,
			std::unordered_set<std::shared_ptr<ShadeVector>>& computed) {

			for (auto& blockee : p->sv->blockedShadeVectors) {

				computeShadeStrength(&pathInfos[blockee->toUnit], shadeBelow, pathInfos, computed);

				if (computed.find(p->sv) == computed.end())
					p->sv->shadeStrength += shadeBelow;

				shadeBelow = 0.;
			}

			if (computed.find(p->sv) == computed.end()) {

				p->sv->shadeStrength += p->shadedVolume;
				MVector shadeVector = MVector(p->sv->toUnit.x, p->sv->toUnit.y, p->sv->toUnit.z).normal();
				p->sv->setShadeVectors(shadeVector * (p->sv->shadeStrength / p->totalPaths));
				computed.insert(p->sv);
			}

			shadeBelow = p->sv->shadeStrength / p->parents.size();
		}
	};

	/*
	* With PathInfo established, we can now create the ShadeVectors
	* 
	* This is essentially a breadth-first-search.  Starting with shadeRoot, we use VECTORS_TO_NEIGHBORS to check which neighboring
	* units lie within shade range, create ShadeVectors which point to those units, and add them to the queue.
	*/
	std::queue<std::shared_ptr<ShadeVector>> shadeVectors;
	shadeVectors.push(shadeRoot);
	std::unordered_map<Point_Int, PathInfo, Point_Int::HashFunction> pathInfos;
	pathInfos[Point_Int(0, 0, 0)] = PathInfo(shadeRoot, 0., 1, nullptr, 0.);

	while (!shadeVectors.empty()) {

		std::shared_ptr<ShadeVector> next = shadeVectors.front();
		shadeVectors.pop();

		for (const auto& toNeighbor : VECTORS_TO_NEIGHBORS) {

			Point_Int neighborIndex = next->toUnit + toNeighbor;
			double fullPathLength = pathInfos[next->toUnit].shortestPathLength + (toNeighbor.toMVector() * unitSize).length();

			if (pathInfos.find(neighborIndex) != pathInfos.end()) {

				if (almostEqual(fullPathLength, pathInfos[neighborIndex].shortestPathLength)) {

					next->blockedShadeVectors.push_back(pathInfos[neighborIndex].sv);
					pathInfos[neighborIndex].parents.push_back(next);
					pathInfos[neighborIndex].totalPaths += pathInfos[next->toUnit].totalPaths;

				}
				else if (fullPathLength < pathInfos[neighborIndex].shortestPathLength) {

					pathInfos[neighborIndex].replaceIncoming(fullPathLength, next, pathInfos[next->toUnit].totalPaths);
					next->blockedShadeVectors.push_back(pathInfos[neighborIndex].sv);
				}
			}
			else {

				MVector fullVectorToNeighbor = neighborIndex.toMVector() * unitSize;
				double intersectionVolume = getIntersectionWithShadeRange(fullVectorToNeighbor, 3);
				if (intersectionVolume > 0.) {

					std::shared_ptr<ShadeVector> newShadeIndex = std::make_shared<ShadeVector>(neighborIndex);
					pathInfos[neighborIndex] = PathInfo(newShadeIndex, fullPathLength, pathInfos[next->toUnit].totalPaths, next, intersectionVolume);
					next->blockedShadeVectors.push_back(newShadeIndex);
					shadeVectors.push(newShadeIndex);
					maximumShade += intersectionVolume;
				}
			}
		}
	}

	double shadeBelow = 0.;
	std::unordered_set<std::shared_ptr<ShadeVector>> computed;
	PathInfo::computeShadeStrength(&pathInfos[Point_Int(0, 0, 0)], shadeBelow, pathInfos, computed);
}

MStatus BlockPointGrid::initiateGrid() {

	MStatus status;

	double xCoord = base.x - (unitSize * (xElements / 2.)) + (unitSize * .5);

	for (int xI = 0; xI < xElements; ++xI) {

		double yCoord = base.y + (unitSize * .5);

		grid.push_back(std::vector< std::vector<GridUnit> >());

		for (int yI = 0; yI < yElements; ++yI) {

			double zCoord = base.z - (unitSize * (zElements / 2.)) + (unitSize * .5);

			grid.back().push_back(std::vector<GridUnit>());

			for (int zI = 0; zI < zElements; ++zI) {

				std::string unitName = "g_" + std::to_string(id) + "_unit_" + std::to_string(xI) + "_" + std::to_string(yI) + "_" + std::to_string(zI);
				Point_Int newUnitIndex = pointToIndex(MPoint(xCoord, yCoord, zCoord));
				GridUnit newUnit(unitName.c_str(), xCoord, yCoord, zCoord, newUnitIndex);
				grid.back().back().push_back(newUnit);

				zCoord += unitSize;
			}

			yCoord += unitSize;
		}

		xCoord += unitSize;

		MStreamUtils::stdOutStream() << "Layer " << xI << " created\n";
	}

	grid.back().back().shrink_to_fit();
	grid.back().shrink_to_fit();
	grid.shrink_to_fit();

	// Create grid group as a transform
	MFnDagNode gridGroupDagNodeFn;
	gridGroup = gridGroupDagNodeFn.create("transform", MObject::kNullObj, &status);
	std::string gridName = "grid_" + std::to_string(id);
	gridGroupDagNodeFn.setName(gridName.c_str());
	SimpleShapes::lockTransforms(gridName.c_str());
	CHECK_MSTATUS_AND_RETURN_IT(status);

	// Create mesh sub-groups as transforms and add them to the grid group
	createTransform("unit_arrow_meshes_grid_" + std::to_string(id), unitArrowMeshGroup, gridGroupDagNodeFn, status);
	createTransform("unit_cube_meshes_grid_" + std::to_string(id), unitCubeMeshGroup, gridGroupDagNodeFn, status);
	createTransform("bp_meshes_grid_" + std::to_string(id), bpMeshGroup, gridGroupDagNodeFn, status);

	// Create the grid border mesh and add it to the grid group
	MPoint gridCenter_Point = base + MVector(0., (unitSize * yElements) / 2., 0.);
	MPoint gridCenter(gridCenter_Point.x, gridCenter_Point.y, gridCenter_Point.z);
	std::string borderName = "grid_" + std::to_string(id) + "_border";
	MObject gridBorder = SimpleShapes::makeBox(gridCenter, unitSize * xElements, unitSize * yElements, unitSize * zElements, borderName.c_str());
	gridGroupDagNodeFn.addChild(gridBorder);

	// Display the border as a template
	MFnDagNode gridBorderFn(gridBorder);
	gridBorderFn.setObject(gridBorderFn.child(0));
	MPlug templateDisplayPlug = gridBorderFn.findPlug("template", true, &status);
	templateDisplayPlug.setValue(true);

	// Find any Maya materials (shading groups) we have set up and assign them to their corresponding handles
	std::map<MObject*, MString> expectedShadingGroups;
	expectedShadingGroups[&transparencyMaterialShadingGroup] = "shadePercentageMat";
	expectedShadingGroups[&defaultShadingGroup] = "lambert1";
	MItDependencyNodes it(MFn::kShadingEngine);
	for (; !it.isDone(); it.next()) {
		MFnDependencyNode shadingGroup(it.thisNode());
		MPlug surfaceShaderPlug = shadingGroup.findPlug("surfaceShader", true);
		MPlugArray connectedPlugs;
		surfaceShaderPlug.connectedTo(connectedPlugs, true, false);
		for (unsigned int i = 0; i < connectedPlugs.length(); ++i) {

			MFnDependencyNode materialNode(connectedPlugs[i].node());

			for (auto& [sg, materialName] : expectedShadingGroups) {

				if (materialNode.name() == materialName)
					*sg = shadingGroup.object();
			}
		}
	}

	for (auto& [sg, materialName] : expectedShadingGroups) {

		if ((*sg).isNull())
			MGlobal::displayError("Shading group not found for material: " + materialName);
	}

	return MS::kSuccess;
}

BlockPointGrid::BlockPointGrid(int id, double XSIZE, double YSIZE, double ZSIZE, double UNITSIZE, const MPoint BASE, double DETECTIONRANGE, double CONERANGEANGLE,
	double INTENSITY) {

	//timer.start(clock());
	this->id = id;
	unitSize = UNITSIZE;

	// Note to self: after dividing two doubles that divide evenly in reality, the result is represented internally as
	// ~ .00000000001 less than its integer counterpart.  So truncating will effectively reduce by 1.  Thus the ceil here.

	xElements = static_cast<int>(std::ceil(XSIZE / UNITSIZE));
	yElements = static_cast<int>(std::ceil(YSIZE / UNITSIZE));
	zElements = static_cast<int>(std::ceil(ZSIZE / UNITSIZE));
	base = BASE;
	xIndexOffset = (unitSize * (xElements / 2.)) - base.x;
	yIndexOffset = -base.y;
	zIndexOffset = (unitSize * (zElements / 2.)) - base.z;
	shadeRange = DETECTIONRANGE;
	halfConeAngle = CONERANGEANGLE;
	intensity = INTENSITY;

	createIndexVectorTree();
	initiateGrid();

	MGlobal::displayInfo(MString() + "Grid created with " + (xElements * yElements * zElements) + " units.");
	MGlobal::displayInfo(MString() + "	xElements: " + xElements + ", yElements: " + yElements + ", zElements: " + zElements);
	MGlobal::displayInfo(MString() + "	unitSize: " + unitSize);
	MGlobal::displayInfo(MString() + "	base: (" + base.x + ", " + base.y + ", " + base.z + ")");
	MGlobal::displayInfo(MString() + "	xIndexOffset: " + xIndexOffset + ", yIndexOffset: " + yIndexOffset + ", zIndexOffset: " + zIndexOffset);
	MGlobal::displayInfo(MString() + "	shadeRange: " + shadeRange);
	MGlobal::displayInfo(MString() + "	halfConeAngle: " + halfConeAngle);
	MGlobal::displayInfo(MString() + "	intensity: " + intensity);
	MGlobal::displayInfo(MString() + "	maximumShade: " + maximumShade);

	// Clear any selection
	MGlobal::executeCommand(MString() + "select -cl -sym");
}

BlockPointGrid::~BlockPointGrid() {

	MMessage::removeCallbacks(bpCallbackIds);
}

Point_Int BlockPointGrid::pointToIndex(const MPoint& p) const {

	int xInd = static_cast<int>((p.x + xIndexOffset) / unitSize);
	int yInd = static_cast<int>((p.y + yIndexOffset) / unitSize);
	int zInd = static_cast<int>((p.z + zIndexOffset) / unitSize);

	return Point_Int(xInd, yInd, zInd);
}

std::unordered_set<Point_Int, Point_Int::HashFunction> BlockPointGrid::getIndicesInRadius(const MPoint& loc, const Point_Int bpUnitIndex, double radius) {

	std::queue<Point_Int> unitQueue;
	unitQueue.push(bpUnitIndex);
	std::unordered_set<Point_Int, Point_Int::HashFunction> unitsInRange;
	unitsInRange.insert(bpUnitIndex);

	while (!unitQueue.empty()) {

		Point_Int currentIndex = unitQueue.front();
		unitQueue.pop();
		for (auto& d : UNIT_NEIGHBOR_DIRECTIONS) {

			Point_Int neighbor = currentIndex + d;

			if (unitsInRange.find(neighbor) == unitsInRange.end()) {

				if (indicesAreOnGrid(neighbor.x, neighbor.y, neighbor.z)) {

					double proximity = (grid[neighbor.x][neighbor.y][neighbor.z].getCenter() - loc).length();
					if (proximity < radius) {

						unitQueue.push(neighbor);
						unitsInRange.insert(neighbor);
					}
				}
			}
		}
	}

	return unitsInRange;
}

MStatus BlockPointGrid::addBlockPoint(const MPoint loc, double bpDensity, double bpRadius, std::shared_ptr<BlockPoint>& ptrForSeg) {

	Point_Int unitIndex = pointToIndex(loc);

	if (!this->indicesAreInRange_showError(unitIndex.x, unitIndex.y, unitIndex.z))
		return MS::kFailure;

	// Only BlockPointGrid creates new BlockPoints, however there are two handles to each BlockPoint - one for the bpg and one for the Segment that the bp sits on
	std::shared_ptr<BlockPoint> newBP = std::make_shared<BlockPoint>(loc, static_cast<int>(std::round(bpDensity)), bpRadius, unitIndex, static_cast<int>(blockPoints.size()));

	blockPoints.push_back(newBP);
	ptrForSeg = newBP;

	auto indicesInRadius = getIndicesInRadius(loc, newBP->getGridIndex(), bpRadius);
	newBP->setIndicesInRadius(indicesInRadius);

	for (const auto& i : indicesInRadius) {

		GridUnit* unit = &grid[i.x][i.y][i.z];
		unit->adjustDensityIncludingExcess(add * static_cast<int>(std::round(bpDensity)));
		dirtyDensityUnits.insert(unit);
	}

	return MS::kSuccess;
}

MStatus BlockPointGrid::moveBlockPoint(BlockPoint& bp, const MPoint newLoc) {

	MStatus status;

	Point_Int newUnitIndex = pointToIndex(newLoc);

	if (!indicesAreInRange_showError(newUnitIndex.x, newUnitIndex.y, newUnitIndex.z))
		return MS::kFailure;

	Point_Int bpGridIndex = bp.getGridIndex();
	if (newUnitIndex != bpGridIndex) {

		// Since BlockPoints' radius allows them to affect many units, we must add its movement to the set of indices in its radius, then figure out which of the resulting
		// indices were not in the original set (this results in newSetDiff), as well as which ones in the original set are not in the new set (this results in oldSetDiff).
		// Only the units within these set differences have changed densities, so only they are used to adjust the grid.

		std::vector<Point_Int> newSetDiff;
		std::vector<Point_Int> oldSetDiff;
		Point_Int moveVector = newUnitIndex - bpGridIndex;
		status = addMoveVectorToBP(bp, moveVector, newSetDiff, oldSetDiff);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		bp.setGridIndex(newUnitIndex);

		for (auto& i : oldSetDiff) {

			GridUnit* unit = &grid[i.x][i.y][i.z];
			unit->adjustDensityIncludingExcess(subtract * bp.getDensity());
			dirtyDensityUnits.insert(unit);
		}

		for (auto& i : newSetDiff) {

			GridUnit* unit = &grid[i.x][i.y][i.z];
			unit->adjustDensityIncludingExcess(add * bp.getDensity());
			dirtyDensityUnits.insert(unit);
		}
	}

	// set the new location for the block point
	bp.setLoc(newLoc);

	return MS::kSuccess;
}

void BlockPointGrid::deleteBlockPoint(std::shared_ptr<BlockPoint> bp) {

	// remove the bp's effect on the grid
	for (const auto& i : bp->getIndicesInRadius()) {

		GridUnit* unit = &grid[i.x][i.y][i.z];
		unit->adjustDensityIncludingExcess(subtract * bp->getDensity());
		dirtyDensityUnits.insert(unit);
	}

	// remove the bpg's handle to the bp
	blockPoints.erase(remove(blockPoints.begin(), blockPoints.end(), bp), blockPoints.end());
}

MStatus BlockPointGrid::deleteAllBlockPoints() {

	MStatus status;

	// Delete the bp objects and adjust the grid units they were affecting
	// Since deleteBlockPoint will change the size of blockPoints, loop through a copy
	std::vector<std::shared_ptr<BlockPoint>> bpsCopy = blockPoints;
	for (auto& bp : bpsCopy)

		deleteBlockPoint(bp);


	// Delete all child objects of the bpMeshGroup
	MFnDagNode bpMeshGroupDagNodeFn(bpMeshGroup);

	for (int i = static_cast<int>(bpMeshGroupDagNodeFn.childCount()); i >= 0; --i) {
		MObject childObj = bpMeshGroupDagNodeFn.child(i);
		MDagPath childDagPath;
		MFnDagNode(childObj).getPath(childDagPath);

		MObject childNode = childDagPath.node();
		MGlobal::deleteNode(childNode);
	}


	return MS::kSuccess;
}

MStatus BlockPointGrid::applyShade() {

	MStatus status;

	for (auto& u : dirtyDensityUnits) {

		u->checkDensity(status);

		int densityChange = u->updateDensity();
		if (std::abs(densityChange) == 0)
			continue;

		bool add = densityChange > 0;
		Point_Int dirtyUnitIndex = u->getGridIndex();

		// A change in density made to this unit affects the shade travelling through it, which is represented by the ShadeVectors in appliedShadeIndices.
		// Adjust this shade accordingly.  In general, if the current unit has become obstructed (density added), then shade that had been travelling through will be removed
		// and replaced with that emitted from the new shadeRoot.  If it has become unobstructed (density subtracted), then shade that it was blocking is put back.
		for (const auto& [sv, paths] : u->getAppliedShadeVectors()) {

			status = propagateFrom(sv, paths, dirtyUnitIndex - sv->toUnit, !add);
			CHECK_MSTATUS_AND_RETURN_IT(status);
		}

		status = propagateFrom(shadeRoot.get(), 1, dirtyUnitIndex, add);
		CHECK_MSTATUS_AND_RETURN_IT(status);
		u->setBlocked(add);
	}

	dirtyDensityUnits.clear();
	updateAllUnitsLightDirection();

	return MS::kSuccess;
}

MStatus BlockPointGrid::propagateFrom(ShadeVector* startingShadeIndex, std::size_t convergedPaths, Point_Int blockerIndex, bool add) {

	std::queue<std::shared_ptr<ShadeVector>> svs;
	std::unordered_set<std::shared_ptr<ShadeVector>> encountered;

	// Note that startingShadeIndex is not applied.  We start with its children.
	startingShadeIndex->getBlocked(svs, encountered, convergedPaths);

	while (!svs.empty()) {

		ShadeVector& next = *svs.front();

		// Get the index of the grid unit pointed to by this sv
		int X = blockerIndex.x + next.toUnit.x;
		int Y = blockerIndex.y + next.toUnit.y;
		int Z = blockerIndex.z + next.toUnit.z;

		if (indicesAreOnGrid(X, Y, Z)) {

			GridUnit& unit = grid[X][Y][Z];

			if (add) {

				unit.applyShadeVector(next);
			}
			else {

				unit.unapplyShadeVector(next);
			}

			if (!unit.isBlocked())
				next.getBlocked(svs, encountered, next.convergedPaths);

			dirtyUnits.insert(&unit);
		}

		svs.pop();
	}

	return MS::kSuccess;
}

void BlockPointGrid::updateAllUnitsLightDirection() {

	for (auto& unit : dirtyUnits) {

		unit->updateLightDirection(intensity, maximumShade, unblockedLightDirection);

		displayAffectedUnitArrowIf(*unit);

		if (!displayShadedUnitArrows && unit->arrowMeshIsVisible()) {

			unit->updateArrowMesh();
			unit->setArrowShadePlug(maximumShade);
		}

		displayShadedUnitIf(*unit);
	}

	dirtyUnits.clear();
}

inline bool BlockPointGrid::indicesAreOnGrid(int x, int y, int z) const {

	if (x >= xElements || x < 0)
		return false;
	else if (y >= yElements || y < 0)
		return false;
	else if (z >= zElements || z < 0)
		return false;

	return true;
}

inline bool BlockPointGrid::indicesAreInRange_showError(int x, int y, int z) const {

	if (x >= xElements || x < 0) {

		MStreamUtils::stdOutStream() << "Error. x index outside of grid.  Aborting";
		MGlobal::displayInfo(MString() + "Error. x index outside of grid.  Aborting\n");
		return false;
	}
	else if (y >= yElements || y < 0) {

		MStreamUtils::stdOutStream() << "Error. y index outside of grid.  Aborting";
		MGlobal::displayInfo(MString() + "Error. y index outside of grid.  Aborting\n");
		return false;
	}
	else if (z >= zElements || z < 0) {

		MStreamUtils::stdOutStream() << "Error. z index outside of grid.  Aborting";
		MGlobal::displayInfo(MString() + "Error. z index outside of grid.  Aborting\n");
		return false;
	}

	return true;
}

MStatus BlockPointGrid::addMoveVectorToBP(BlockPoint& bp, const Point_Int& moveVector, std::vector<Point_Int>& newSetDiff, std::vector<Point_Int>& oldSetDiff) {

	std::unordered_set<Point_Int, Point_Int::HashFunction> newIndicesInRadius;
	auto oldIndicesInRadius = bp.getIndicesInRadius();
	for (const auto& i : oldIndicesInRadius) {
		newIndicesInRadius.insert(i + moveVector);
	}

	for (const auto& i : oldIndicesInRadius) {

		if (newIndicesInRadius.find(i) == newIndicesInRadius.end()) {

			oldSetDiff.push_back(i);
		}
	}

	for (auto& i : newIndicesInRadius) {

		if (oldIndicesInRadius.find(i) == oldIndicesInRadius.end()) {

			if (!indicesAreInRange_showError(i.x, i.y, i.z))
				return MS::kFailure;

			newSetDiff.push_back(i);
		}
	}

	bp.setIndicesInRadius(newIndicesInRadius);

	return MS::kSuccess;
}

void BlockPointGrid::traverseRange(Point_Int startInd, Point_Int endInd, std::function<void(GridUnit&)> func) {

	for (int x = startInd.x; x < endInd.x; x++) {
		for (int y = startInd.y; y < endInd.y; y++) {
			for (int z = startInd.z; z < endInd.z; z++) {

				if (!indicesAreOnGrid(x, y, z))
					continue;

				func(grid[x][y][z]);
			}
		}
	}
}

double BlockPointGrid::getIntersectionWithShadeRange(const MVector& vectorToUnit, int timesToDivide) const {

	double intersectionVolume = 0.;
	std::vector<MVector> subdivisions = { vectorToUnit };
	std::vector<MVector> cubesToDivide = subdivisions;
	double subDivisionSize = unitSize;

	// This loop will yield a list of centers of cubic subdivisions of the unit.  The number of subdivisions is 8^timesToDivide
	for (int i = 0; i < timesToDivide; ++i) {

		subdivisions.clear();

		for (const auto& c : cubesToDivide) {

			divideCubeToEighths(c, subDivisionSize, subdivisions);
		}

		subDivisionSize *= .5;
		cubesToDivide = subdivisions;
	}

	double subDivisionVolume = std::pow(subDivisionSize, 3);

	for (const auto& s : subdivisions) {

		if (s.length() < shadeRange && s.angle(MVector(0., -1., 0.)) <= halfConeAngle) {

			intersectionVolume += subDivisionVolume;
		}
	}

	return intersectionVolume;
}

void BlockPointGrid::divideCubeToEighths(const MVector& cubeCenter, double size, std::vector<MVector>& subdivisions) {

	double q = size * .25;

	subdivisions.push_back(MPoint(cubeCenter.x - q, cubeCenter.y - q, cubeCenter.z - q));
	subdivisions.push_back(MPoint(cubeCenter.x - q, cubeCenter.y - q, cubeCenter.z + q));
	subdivisions.push_back(MPoint(cubeCenter.x + q, cubeCenter.y - q, cubeCenter.z + q));
	subdivisions.push_back(MPoint(cubeCenter.x + q, cubeCenter.y - q, cubeCenter.z - q));
	subdivisions.push_back(MPoint(cubeCenter.x - q, cubeCenter.y + q, cubeCenter.z - q));
	subdivisions.push_back(MPoint(cubeCenter.x - q, cubeCenter.y + q, cubeCenter.z + q));
	subdivisions.push_back(MPoint(cubeCenter.x + q, cubeCenter.y + q, cubeCenter.z + q));
	subdivisions.push_back(MPoint(cubeCenter.x + q, cubeCenter.y + q, cubeCenter.z - q));
}

void BlockPointGrid::attachBPCallbacks(std::vector<std::shared_ptr<BlockPoint>> bps) {
	MGlobal::displayInfo(MString() + "Attaching");
	MStatus status;

	for (auto& bp : bps) {

		MObject bpTransformNode = bp->getTransformNode();
		MFnTransform bpTransformFn(bpTransformNode);
		bpCallbackIds.append(MNodeMessage::addAttributeChangedCallback(bpTransformNode, BlockPointGrid::updateGridFromBPChange, static_cast<void*>(bp.get()), &status));
		bpCallbackIds.append(MNodeMessage::addNodePreRemovalCallback(bpTransformNode, BlockPointGrid::updateGridAfterBPRemoval, static_cast<void*>(bp.get()), &status));
	}
}

void BlockPointGrid::updateGridFromBPChange(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug& otherPlug, void* clientData) {

	MSelectionList originalSelection;
	MGlobal::getActiveSelectionList(originalSelection); // only needed if we are creating meshes and hardening edges as they will be auto selected

	if (msg & MNodeMessage::kAttributeSet) {

		MStatus status;

		BlockPoint* bp = static_cast<BlockPoint*>(clientData);
		MVector bpTranslation = getObjectTranslation(plug.node(&status), status);
		MPoint currentLoc(bpTranslation);
		Point_Int meshUnit = bp->getGrid()->pointToIndex(currentLoc);
		if (bp->getCurrentUnit() != meshUnit) {

			bp->getGrid()->moveBlockPoint(*bp, currentLoc);
			bp->getGrid()->applyShade();
			bp->setCurrentUnit(meshUnit);
		}
	}

	MGlobal::setActiveSelectionList(originalSelection);
}

void BlockPointGrid::updateGridAfterBPRemoval(MObject& node, void* clientData) {

	BlockPoint* bp = static_cast<BlockPoint*>(clientData);
	std::shared_ptr<BlockPoint> temp = bp->getSharedFromThis();

	// Check whether the bp has already been removed.  This would happen if we triggered this callback via deleteAllBlockPoints
	if (!bp->getGrid()->hasBlockPoint(temp))
		return;

	bp->getGrid()->deleteBlockPoint(temp);
	bp->getGrid()->applyShade();
}

void BlockPointGrid::displayAllBlockPoints(bool d) {

	MStatus status;
	MFnDagNode bpMeshGroupDagNodeFn;
	bpMeshGroupDagNodeFn.setObject(bpMeshGroup);

	if (d) {

		for (auto& bp : blockPoints) {

			MObject transformNode = bp->getTransformNode();
			if (transformNode.isNull()) {

				bp->createBPMesh(bpMeshGroupDagNodeFn, defaultShadingGroup);
			}

			MFnDagNode bpDagNode(transformNode, &status);

			MPlug arrowVisibilityPlug = bpDagNode.findPlug("visibility", true, &status);
			arrowVisibilityPlug.setValue(true);
		}
	}
	else {

		for (auto& bp : blockPoints) {

			MObject transformNode = bp->getTransformNode();
			if (!transformNode.isNull()) {

				MFnDagNode bpDagNode(transformNode, &status);

				MPlug arrowVisibilityPlug = bpDagNode.findPlug("visibility", true, &status);
				arrowVisibilityPlug.setValue(false);
			}
		}
	}
}

void BlockPointGrid::displayBlockPoints(std::vector<std::shared_ptr<BlockPoint>> bpsToDisplay) {

	MStatus status;
	MFnDagNode bpMeshGroupDagNodeFn;
	bpMeshGroupDagNodeFn.setObject(bpMeshGroup);

	for (auto& bp : bpsToDisplay) {

		MObject transformNode = bp->getTransformNode();
		if (transformNode.isNull()) {

			bp->createBPMesh(bpMeshGroupDagNodeFn, defaultShadingGroup);
		}

		MFnDagNode bpDagNode(transformNode, &status);

		MPlug arrowVisibilityPlug = bpDagNode.findPlug("visibility", true, &status);
		arrowVisibilityPlug.setValue(true);
	}
}

void BlockPointGrid::toggleDisplayShadedUnits(bool display) {

	displayShadedUnits = display;
	traverseRange(Point_Int(0, 0, 0), Point_Int(xElements, yElements, zElements), [this](GridUnit& unit) { displayShadedUnitIf(unit); });
}

void BlockPointGrid::toggleDisplayShadedUnitArrows(bool display) {

	displayShadedUnitArrows = display;
	traverseRange(Point_Int(0, 0, 0), Point_Int(xElements, yElements, zElements), [this](GridUnit& unit) { displayAffectedUnitArrowIf(unit); });
}

void BlockPointGrid::displayShadedUnitIf(GridUnit& unit) {

	if (displayShadedUnits) {

		if (unit.getTotalShade() / maximumShade >= displayPercentageThreshhold) {

			if (unit.getCubeTransformNode().isNull()) 
				makeUnitCubeMesh(unit);

			unit.setCubeShadePlug(maximumShade);
			unit.setUVsToTile(transparencyTileMapTileSize, maximumShade, uvOffSet);
			unit.setCubeVisibility(true);
		}
		else {
			unit.setCubeVisibility(false);
		}
	}
	else {

		unit.setCubeVisibility(false);
	}
}

void BlockPointGrid::displayAffectedUnitArrowIf(GridUnit& unit) {

	if (displayShadedUnitArrows) {

		if (unit.getTotalShade() / maximumShade >= displayPercentageThreshhold) {

			if (unit.getArrowTransformNode().isNull()) 
				makeUnitArrowMesh(unit);

			unit.setArrowShadePlug(maximumShade);
			unit.updateArrowMesh();
			unit.setArrowVisibility(true);
		}
		else {
			unit.setArrowVisibility(false);
		}
	}
	else {

		unit.setArrowVisibility(false);
	}
}

MVector BlockPointGrid::getObjectTranslation(MObject node, MStatus& status) {

	MFnDagNode dagNode(node, &status);
	MDagPath dagPath;
	dagNode.getPath(dagPath);
	MFnTransform transform(dagPath, &status);
	return transform.getTranslation(MSpace::kWorld, &status);
}