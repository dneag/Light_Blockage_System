/*
BlockPointGrid.cpp
*/

#include <math.h>
#include <map>
#include "BlockPointGrid.h"

void BlockPointGrid::createShadeVectorTree() {

	MGlobal::displayInfo(MString() + "*** Entered createShadeVectorTree ***");
	MStatus status;

	// Precalculate subdivision size and volume. There will be 8^timesToSubDivide subdivisions for each unit. 
	int timesToSubDivide = 3;
	double subdivisionSize = unitSize * std::pow(.5, timesToSubDivide);
	double subdivisionVolume = std::pow(subdivisionSize, 3);

	// Key:  ShadeVector
	// Value:  The subdivisions within the ShadeVector's unit
	std::unordered_map< ShadeVector*, std::vector<MVector>> subdivisionsByUnit;

	// Key:  ShadeVector
	// Value:  Locations of all subdivisions for the ShadeVector
	std::unordered_map<ShadeVector*, std::vector<MVector>> totalOccludedVolumesByShadeVectors;

	MGlobal::displayInfo(MString() + "*** Finding ShadeVectors and their subdivisions ***");

	// Find all shade vectors in shade range and populate the two maps
	findAllShadeVectorSubdivisions(subdivisionsByUnit, totalOccludedVolumesByShadeVectors, subdivisionVolume, timesToSubDivide);

	std::unordered_set<ShadeVector*> done;
	MGlobal::displayInfo(MString() + "*** Finding volume blocked ***");

	// Compute the total volume occluded by each ShadeVector as well as that shared by neighbors. 
	findAllShadedVolume(shadeRoot.get(), subdivisionsByUnit, totalOccludedVolumesByShadeVectors, done, subdivisionVolume, subdivisionSize);

	// Adjust the value of the volume that ShadeVectors share with their neighbors so it is more accurate
	finalizeSharedVolumeBlocked();

	// Uncomment the following line to display the range of the shade vector tree.  Each unit represents a ShadeVector and will have
	// channels indicating the amount of shared volume for each of its child ShadeVectors
	//displayShadeVectorUnitsByLevel(subdivisionSize, totalOccludedVolumesByShadeVectors);
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

	setShadingGroups();
	createShadeVectorTree();
	initiateGrid();

	MGlobal::displayInfo(MString() + "Grid created with " + (xElements * yElements * zElements) + " units.");
	MGlobal::displayInfo(MString() + "	xElements: " + xElements + ", yElements: " + yElements + ", zElements: " + zElements);
	MGlobal::displayInfo(MString() + "	unitSize: " + unitSize);
	MGlobal::displayInfo(MString() + "	base: (" + base.x + ", " + base.y + ", " + base.z + ")");
	MGlobal::displayInfo(MString() + "	xIndexOffset: " + xIndexOffset + ", yIndexOffset: " + yIndexOffset + ", zIndexOffset: " + zIndexOffset);
	MGlobal::displayInfo(MString() + "	shadeRange: " + shadeRange);
	MGlobal::displayInfo(MString() + "	halfConeAngle: " + halfConeAngle);
	MGlobal::displayInfo(MString() + "	intensity: " + intensity);
	MGlobal::displayInfo(MString() + "	maxVolumeBlocked: " + maxVolumeBlocked);

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

		u->setArrowDensityPlug();

		bool add = densityChange > 0;
		Point_Int dirtyUnitIndex = u->getGridIndex();

		// A change in density made to this unit affects the shade travelling through it, which is represented by appliedShadeIndices.
		// So, before applying the shade resulting from the density change in this unit, adjust the existing shade accordingly.  If this unit has
		// become dense, then shade that had been travelling through it is removed. If it has lost density, then shade that it was blocking is put back.
		for (const auto& [sv, percentage] : u->getAppliedShadeVectors()) {
			//if (u->xIndex == 16 && u->zIndex == 16 && (u->yIndex < 5 && u->yIndex > 0)) {
			//MString action = add ? "Removing" : "Putting back";
			//MGlobal::displayInfo(MString() + action + " below occupant: " + occupant.first->toUnit.toMString());

			status = propagateFrom(sv, dirtyUnitIndex - sv->toUnit, percentage, !add);
			CHECK_MSTATUS_AND_RETURN_IT(status);
		}

		status = propagateFrom(shadeRoot.get(), dirtyUnitIndex, 1., add);
		CHECK_MSTATUS_AND_RETURN_IT(status);

		u->setBlocked(add);
	}

	dirtyDensityUnits.clear();
	updateAllUnitsLightConditions();

	return MS::kSuccess;
}

MStatus BlockPointGrid::propagateFrom(ShadeVector* startShadeVector, Point_Int blockerIndex, double startingPercentage, bool add) {

	MStatus status;

	std::vector<SvRelay> thisLevel;
	for (auto& n : startShadeVector->neighborShadeVectors)
		thisLevel.push_back({ n.neighbor.get(), n.percentShared * startingPercentage });

	// encountered keeps track of which ShadeVectors have been added to thisLevel so that we can keep them unique.  It also records the position
	// of each in thisLevel so that we can quickly access and modify them when needed.
	std::unordered_map<std::shared_ptr<ShadeVector>, std::size_t> encountered;

	while (!thisLevel.empty()) {

		std::vector<SvRelay> nextLevel;

		for (auto& relay : thisLevel) {

			ShadeVector& next = *relay.sv;

			int X = blockerIndex.x + next.toUnit.x;
			int Y = blockerIndex.y + next.toUnit.y;
			int Z = blockerIndex.z + next.toUnit.z;

			if (indicesAreOnGrid(X, Y, Z)) {

				GridUnit& unit = grid[X][Y][Z];

				if (add) {

					unit.applyShadeVector(&relay);
				}
				else {

					unit.unapplyShadeVector(&relay);
				}

				if (!unit.isBlocked()) {
					next.getNeighbors(nextLevel, encountered, relay.cumulativePercentage);
				}

				dirtyUnits.insert(&unit);
			}
		}

		thisLevel = std::move(nextLevel);
		encountered.clear();
	}

	return MS::kSuccess;
}

void BlockPointGrid::updateAllUnitsLightConditions() {

	for (auto& unit : dirtyUnits) {

		unit->updateLightConditions(intensity, maxVolumeBlocked, unblockedLightDirection);

		displayAffectedUnitArrowIf(*unit);

		if (!displayShadedUnitArrows && unit->arrowMeshIsVisible()) {

			unit->updateArrowMesh();
			unit->setArrowShadePlug();
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

void BlockPointGrid::findAllShadeVectorSubdivisions(std::unordered_map< ShadeVector*, std::vector<MVector>>& subdivisionsByUnit,
	std::unordered_map<ShadeVector*, std::vector<MVector>>& totalOccludedVolumesByShadeVectors, double subdivisionVolume, double timesToSubDivide) {

	int subdivisionCount = 0;

	std::vector<MVector> rootSubDivisionsInRange = getSubDivisionsInShadeRange({ 0.,0.,0. }, timesToSubDivide);
	shadeRoot->volumeInRange = rootSubDivisionsInRange.size() * subdivisionVolume;
	MGlobal::displayInfo(MString() + "shadeRoot volumeInRange: " + shadeRoot->volumeInRange);

	subdivisionsByUnit[shadeRoot.get()] = rootSubDivisionsInRange;
	totalOccludedVolumesByShadeVectors[shadeRoot.get()] = rootSubDivisionsInRange;

	std::queue<std::shared_ptr<ShadeVector>> shadeVectors;
	shadeVectors.push(shadeRoot);
	std::unordered_map<Point_Int, std::shared_ptr<ShadeVector>, Point_Int::HashFunction> encounteredShadeVectors;
	encounteredShadeVectors[Point_Int(0, 0, 0)] = shadeRoot;
	double unitVolume = std::pow(unitSize, 3);

	while (!shadeVectors.empty()) {

		std::shared_ptr<ShadeVector> next = shadeVectors.front();
		shadeVectors.pop();

		for (const auto& toNeighbor : VECTORS_TO_NEIGHBORS) {

			Point_Int neighborIndex = next->toUnit + toNeighbor;

			if (encounteredShadeVectors.find(neighborIndex) == encounteredShadeVectors.end()) {

				encounteredShadeVectors[neighborIndex] = nullptr; // Mark encountered even the indices out of range so we don't have to redundantly check them
				MVector fullVectorToNeighbor = neighborIndex.toMVector() * unitSize;
				std::vector<MVector> subDivisionsInRange = getSubDivisionsInShadeRange(fullVectorToNeighbor, timesToSubDivide);

				if (subDivisionsInRange.size() * subdivisionVolume > unitVolume * .001) {

					std::shared_ptr<ShadeVector> newShadeVector = std::make_shared<ShadeVector>(neighborIndex);

					subdivisionsByUnit[newShadeVector.get()] = subDivisionsInRange;
					totalOccludedVolumesByShadeVectors[newShadeVector.get()] = subDivisionsInRange;
					shadeVectors.push(newShadeVector);
					encounteredShadeVectors[neighborIndex] = newShadeVector;
					newShadeVector->volumeInRange = subDivisionsInRange.size() * subdivisionVolume;
					/*MGlobal::displayInfo(MString() + "Added new ShadeVector at " + neighborIndex.toMString() + ". subDivisionsInRange: " + subDivisionsInRange.size()
						+ ", volumeInRange: " + newShadeVector->volumeInRange);*/
				}
			}

			// Here we check if next is between the neighbor and the shade root.  If it is, then the neighbor is added
			// since some portion of it will be in next's shade
			if (encounteredShadeVectors[neighborIndex]) { // If the neighbor is in shade range

				// If all three of the neighbor's dimensions are >= the current's, then next is between some portion of it
				if (std::abs(neighborIndex.x) >= std::abs(next->toUnit.x)
					&& std::abs(neighborIndex.y) >= std::abs(next->toUnit.y)
					&& std::abs(neighborIndex.z) >= std::abs(next->toUnit.z)) {

					next->neighborShadeVectors.push_back({ encounteredShadeVectors[neighborIndex], 0., 0. });
				}
			}
		}
	}
}

void BlockPointGrid::findAllShadedVolume(ShadeVector* shadeVector,
	std::unordered_map< ShadeVector*, std::vector<MVector>>& subdivisionsByUnit,
	std::unordered_map<ShadeVector*, std::vector<MVector>>& totalOccludedVolumesByShadeVectors,
	std::unordered_set<ShadeVector*>& done, double subdivisionVolume, double subdivisionSize) {

	if (done.find(shadeVector) != done.end())
		return;

	//MGlobal::displayInfo(MString() + "In findAllShadedVolume3 for " + shadeVector->toUnit.toMString());
	for (auto& neighbor : shadeVector->neighborShadeVectors) {
		//MGlobal::displayInfo(MString() + "checking neighbor: " + neighbor.neighbor->toUnit.toMString());
		findAllShadedVolume(neighbor.neighbor.get(), subdivisionsByUnit, totalOccludedVolumesByShadeVectors, done,
			subdivisionVolume, subdivisionSize);
	}

	//MGlobal::displayInfo(MString() + "Computing volume blocked for " + shadeVector->toUnit.toMString());

	shadeVector->volumeBlocked += shadeVector->volumeInRange;

	std::queue<ShadeVector*> extendedNeighbors;
	std::unordered_set<ShadeVector*> neighborsEncountered;
	std::vector<std::pair<MVector, MVector>> unitSidesFacingOrigin = getUnitSidesFacingShadeOrigin(*shadeVector);

	// Find the portions of the ShadeVector's adjacent units that lie in the frustrum beyond its unit
	for (auto& shared : shadeVector->neighborShadeVectors) {

		shadeVector->volumeBlocked += computeShadedVolume(unitSidesFacingOrigin, subdivisionsByUnit[shared.neighbor.get()],
			totalOccludedVolumesByShadeVectors[shadeVector], subdivisionSize, subdivisionVolume);

		extendedNeighbors.push(shared.neighbor.get());
		neighborsEncountered.insert(shared.neighbor.get());
	}

	// Loop through all neighbors' neighbors, adding their intersecting volumes to the sv, until we reach the end of shade range
	while (!extendedNeighbors.empty()) {

		ShadeVector* nextNeighbor = extendedNeighbors.front();
		extendedNeighbors.pop();

		for (const auto& neighbor : nextNeighbor->neighborShadeVectors) { // loop through all neighboring ShadeVectors

			if (neighborsEncountered.find(neighbor.neighbor.get()) == neighborsEncountered.end()) {

				shadeVector->volumeBlocked += computeShadedVolume(unitSidesFacingOrigin, subdivisionsByUnit[neighbor.neighbor.get()],
					totalOccludedVolumesByShadeVectors[shadeVector], subdivisionSize, subdivisionVolume);

				extendedNeighbors.push(neighbor.neighbor.get());
				neighborsEncountered.insert(neighbor.neighbor.get());
			}
		}
	}

	// The length of the actual shadeVector is equal to the volumeBlocked of the ShadeVector node.  Once that has been calculated we can set the vector
	shadeVector->shadeVector = shadeVector->toUnit.toMVector().normal() * shadeVector->volumeBlocked;

	for (auto& neighbor : shadeVector->neighborShadeVectors) {

		neighbor.sharedBlockage = findVolumeSharedWithNeighbor(unitSidesFacingOrigin, totalOccludedVolumesByShadeVectors[neighbor.neighbor.get()],
			subdivisionSize, subdivisionVolume);

		neighbor.percentShared = neighbor.sharedBlockage / neighbor.neighbor->volumeBlocked;
	}

	if (shadeVector->toUnit == Point_Int(0, 0, 0))
		maxVolumeBlocked = shadeVector->volumeBlocked;

	done.insert(shadeVector);
}

double BlockPointGrid::findVolumeSharedWithNeighbor(const std::vector<std::pair<MVector, MVector>>& blockerUnitSidesFacingOrigin,
	const std::vector<MVector>& shadedSubdivisions, const double subdivisionSize, const double subdivisionVolume) {

	double volume = 0.;

	for (const auto& subdivision : shadedSubdivisions) { // Loop through all subdivisions of the neighbor 

		MVector dir = subdivision.normal(); // Direction to the subdivision

		// Check for intersection with each side
		for (const auto& [sideNormal, sideCenter] : blockerUnitSidesFacingOrigin) {

			double dotProduct = (dir * sideNormal);

			if (dotProduct < -1e-6) {

				double t = (sideCenter * sideNormal) / dotProduct;
				MPoint pointOfIntersection = dir * t;

				// The line intersects with the plane the side lies on, but we still need to check if that point of intersection
				// actually lies within the area of the unit's side.  
				if (pointOfIntersectionIsOnSide(pointOfIntersection, sideNormal, sideCenter, dir)) {

					volume += subdivisionVolume;
					break;
				}
			}
		}
	}

	return volume;
}

std::vector<std::pair<MVector, MVector>> BlockPointGrid::getUnitSidesFacingShadeOrigin(const ShadeVector& sv) const {

	std::vector<std::pair<MVector, MVector>> unitSidesFacingOrigin;
	if (sv.toUnit != shadeRoot->toUnit) {

		if (sv.toUnit.x != 0) {
			double opposite = sv.toUnit.x > 0 ? -1. : 1.;
			MVector normal = { opposite, 0., 0. };
			MVector location = (sv.toUnit.toMVector() * unitSize) + (normal * (unitSize * .5));
			unitSidesFacingOrigin.push_back({ normal, location });
		}

		if (sv.toUnit.y != 0) {
			double opposite = sv.toUnit.y > 0 ? -1. : 1.;
			MVector normal = { 0., opposite, 0. };
			MVector location = (sv.toUnit.toMVector() * unitSize) + (normal * (unitSize * .5));
			unitSidesFacingOrigin.push_back({ normal, location });
		}

		if (sv.toUnit.z != 0) {
			double opposite = sv.toUnit.z > 0 ? -1. : 1.;
			MVector normal = { 0., 0., opposite };
			MVector location = (sv.toUnit.toMVector() * unitSize) + (normal * (unitSize * .5));
			unitSidesFacingOrigin.push_back({ normal, location });
		}
	}
	else {

		// If this is the shadeRoot, then we have a special case.  Any of its sides that intersect with shade range should be tested
		// for intersection.  Assuming the shade range angle is never greater than 180 degrees, this will always be all sides except for the top.
		// Also, note that every subdivision tested is gauranteed to intersect with these, so we could definitely leverage that to improve 
		// performance.  But doing it this way keeps it consistent with the way other ShadeVectors are calculated and may make the code less error prone.
		unitSidesFacingOrigin.push_back({ {0., 1., 0.}, {0.,-unitSize * .5, 0.} });
		unitSidesFacingOrigin.push_back({ {-1., 0., 0.}, { unitSize * .5, 0., 0.} });
		unitSidesFacingOrigin.push_back({ {0., 0., -1.}, {0.,0., unitSize * .5 } });
		unitSidesFacingOrigin.push_back({ {1., 0., 0.}, {-unitSize * .5,0., 0.} });
		unitSidesFacingOrigin.push_back({ {0., 0., 1.}, {0.,0., -unitSize * .5} });

	}

	return unitSidesFacingOrigin;
}

double BlockPointGrid::computeShadedVolume(const std::vector<std::pair<MVector, MVector>>& blockerUnitSidesFacingOrigin,
	const std::vector<MVector>& neighborSubdivisions, std::vector<MVector>& subdivisionsInVolume,
	const double subdivisionSize, const double subdivisionVolume) {

	double volume = 0.;

	for (const auto& subdivision : neighborSubdivisions) { // Loop through all subdivisions of the neighbor 

		MVector dir = subdivision.normal(); // Direction to the subdivision

		// Check for intersection with each side
		for (const auto& [sideNormal, sideCenter] : blockerUnitSidesFacingOrigin) {

			double dotProduct = (dir * sideNormal);

			/*
				If the dot product of the side normal and direction to the subd is 0 or very close to it, then the two vectors are about perpendicular,
				meaning our line is about parallel to the plane and cannot intersect.  This shouldn't be possible since we established that these
				planes face the origin from which the line to the unit is drawn, but just to be safe...  Also, note we are checking that the dot product
				is negative, indicating the direction to the subd faces the normal, i.e. the angle between them is greater than 90 degrees.
			*/
			if (dotProduct < -1e-6) {

				double t = (sideCenter * sideNormal) / dotProduct;
				MPoint pointOfIntersection = dir * t;


				// The line intersects with the plane the side lies on, but we still need to check if that point of intersection
				// actually lies within the area of the unit's side.  
				if (pointOfIntersectionIsOnSide(pointOfIntersection, sideNormal, sideCenter, dir)) {

					volume += subdivisionVolume;
					subdivisionsInVolume.push_back(subdivision);
					break;
				}
			}
		}
	}

	return volume;
}

bool BlockPointGrid::pointOfIntersectionIsOnSide(const MPoint& pointOfIntersection, const MVector& sideNormal, const MVector& sideCenter, const MVector& ray) const {

	/*
		A point of intersection with the plane the side lies on has been found, but we still need to check if that point
		actually lies within the area of the unit's side.  For this, we can get the vector from the side's center
		to that POI and check whether its dimensions perpendicular to the side's normal are greater than half the unit size.
		If any of them are, then the poi is not on the side. Note that points on the very edge of sides are considered on the
		side, which results in overlapping volumes.
	*/

	MVector centerToPOI = pointOfIntersection - sideCenter;
	double toEdgeApprox = unitSize * .5 + 1e-6;

	// This commented section will eliminate overlaps (I think...still needs more testing).  Not sure if it's necessary.
	/*if (almostEqual(sideNormal.y, 0.)) {

		if (std::abs(centerToPOI.y) > toEdgeApprox) {
			return false;
		}
		else if (almostEqual(centerToPOI.y, unitSize * .5)) {
			return false;
		}
	}
	else {

		if (ray.x > 0 && centerToPOI.x > 0 && almostEqual(centerToPOI.x, unitSize * .5)) {
			return false;
		}

		if (ray.x < 0 && centerToPOI.x < 0 && almostEqual(centerToPOI.x, -unitSize * .5)) {
			return false;
		}
	}

	if (ray.z > 0 && centerToPOI.z > 0 && almostEqual(centerToPOI.z, unitSize * .5)) {
		return false;
	}

	if (ray.z < 0 && centerToPOI.z < 0 && almostEqual(centerToPOI.z, -unitSize * .5)) {
		return false;
	}*/

	if ((almostEqual(sideNormal.x, 0.) && std::abs(centerToPOI.x) > toEdgeApprox)
		|| (almostEqual(sideNormal.y, 0.) && std::abs(centerToPOI.y) > toEdgeApprox)
		|| (almostEqual(sideNormal.z, 0.) && std::abs(centerToPOI.z) > toEdgeApprox))
	{
		// The POI is not on the side
		return false;
	}
	else {
		return true;
	}
}

void BlockPointGrid::finalizeSharedVolumeBlocked() const {

	// Gather blockedNeighbors
	std::unordered_map<ShadeVector*, std::vector<std::pair<ShadeVector*, double>>> blockedNeighbors;
	std::unordered_set<ShadeVector*> encountered;
	std::queue<ShadeVector*> shadeVectors;
	shadeVectors.push(shadeRoot.get());

	while (!shadeVectors.empty()) {

		ShadeVector* next = shadeVectors.front();
		for (auto& shared : next->neighborShadeVectors) {

			blockedNeighbors[shared.neighbor.get()].push_back({ next, shared.sharedBlockage });

			if (encountered.find(shared.neighbor.get()) == encountered.end()) {
				shadeVectors.push(shared.neighbor.get());
				encountered.insert(shared.neighbor.get());
			}
		}

		shadeVectors.pop();
	}

	// blockedNeighbors has all ShadeVectors with a corresponding list of ShadeVectors that share some occluded volume
	for (auto& [blockedNeighbor, blockers] : blockedNeighbors) {

		// Sum up the current volumes
		double totalSharedBeforeAdjustment = 0.;
		for (auto& [blocker, blocked] : blockers) {

			totalSharedBeforeAdjustment += blocked;
		}

		// Take the difference between this total and what it should be
		double volumeError = totalSharedBeforeAdjustment - blockedNeighbor->volumeBlocked;

		// For each blocker of this blockedNeighbor, subtract an amount of blocked volume proportional to its original amount in
		// totalBlockedBeforeAdjustment, then set the resulting value for the blocking ShadeVector's corresponding neighbor in neighborShadeVectors
		for (auto& [blocker, blocked] : blockers) {

			double adjustment = (blocked / totalSharedBeforeAdjustment) * volumeError;
			blocked -= adjustment;
			for (auto& neighborShare : blocker->neighborShadeVectors) {

				if (neighborShare.neighbor.get() == blockedNeighbor) {

					neighborShare.sharedBlockage = blocked;
					neighborShare.percentShared = blocked / blockedNeighbor->volumeBlocked;
				}
			}
		}

		// Test that the final sum is about equal to the neighbor's volume blocked 
		double finalTotalVolumeBlocked = 0.;
		double finalTotalPercentageBlocked = 0.;
		for (auto& [blocker, blocked] : blockers) {
			for (auto& neighborShared : blocker->neighborShadeVectors) {

				if (neighborShared.neighbor.get() == blockedNeighbor) {

					finalTotalVolumeBlocked += neighborShared.sharedBlockage;
					finalTotalPercentageBlocked += neighborShared.percentShared;
				}
			}
		}

		if (!almostEqual(finalTotalVolumeBlocked, blockedNeighbor->volumeBlocked)) {
			MGlobal::displayError(MString() + "Total volume blocked by neighbors of " + blockedNeighbor->toUnit.toMString() + " is " + finalTotalVolumeBlocked
				+ ".  Should be " + blockedNeighbor->volumeBlocked);
		}

		if (!almostEqual(finalTotalPercentageBlocked, 1.)) {
			MGlobal::displayError(MString() + "Total percent blocked by neighbors of " + blockedNeighbor->toUnit.toMString() + " is " + finalTotalPercentageBlocked
				+ ".  Should be 100%");
		}
	}
}

std::vector<MVector> BlockPointGrid::getSubDivisionsInShadeRange(const MVector& vectorToUnit, int timesToSubDivide) {

	double intersectionVolume = 0.;
	std::vector<MVector> subdivisions;
	std::vector<MVector> cubesToDivide = { vectorToUnit };
	double subDivisionSize = unitSize;

	// This loop will yield a list of centers of cubic subdivisions of the unit.  The number of subdivisions is 8^timesToSubDivide
	for (int i = 0; i < timesToSubDivide; ++i) {

		subdivisions.clear();

		for (const auto& c : cubesToDivide) {

			divideCubeToEighths(c, subDivisionSize, subdivisions);
		}

		subDivisionSize *= .5;
		cubesToDivide = subdivisions;
	}

	std::vector<MVector> subdivisionsInRange;
	MVector down = { 0.,-1.,0. };
	for (const auto& s : subdivisions) {

		if (s.length() < shadeRange && s.angle(down) <= halfConeAngle) {

			subdivisionsInRange.push_back(s);
		}
	}

	return subdivisionsInRange;
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

void BlockPointGrid::displayShadeVectorUnitsByLevel(double subdivisionSize,
	std::unordered_map<ShadeVector*, std::vector<MVector>>& totalOccludedVolumesByShadeVectors) {

	MStatus status;
	MFnDagNode svByLevelDagNodeFn;
	assignTransformForDagFn("ShadeVector_units_by_level", svByLevelDagNodeFn, status);
	std::unordered_map<ShadeVector*, std::map<std::string, ChannelGroup>> shadeVectorChannels;

	int level = 0;
	std::unordered_set<ShadeVector*> thisLevel;
	thisLevel.insert(shadeRoot.get());
	int subdCounter = 0;

	while (!thisLevel.empty()) {

		MFnDagNode thisLevelDagNodeFn;
		assignTransformForDagFn("level " + std::to_string(level), thisLevelDagNodeFn, status);
		MObject thisLevelHandle = thisLevelDagNodeFn.object();
		svByLevelDagNodeFn.addChild(thisLevelHandle);

		std::unordered_set<ShadeVector*> nextLevel;

		double totalBlockedThisLevel = 0.;
		for (const auto& sv : thisLevel) {

			MGlobal::displayInfo(MString() + "Level " + level + ": " + sv->toUnit.toMString() + " - " + sv->volumeBlocked);

			totalBlockedThisLevel += sv->volumeBlocked;

			MFnDagNode svGroupDagNodeFn;
			assignTransformForDagFn("_" + sv->toUnit.toString() + "_group", svGroupDagNodeFn, status);
			MObject svGroupHandle = svGroupDagNodeFn.object();
			thisLevelDagNodeFn.addChild(svGroupHandle);

			MObject svUnitCube;
			createShadeVectorUnitTransform(svUnitCube, sv, svGroupDagNodeFn, shadeVectorChannels);

			bool createSubdivisionMeshes = false;

			if (!svUnitCube.isNull() && createSubdivisionMeshes
				&& sv->toUnit == Point_Int(0, 0, 0)
				) {

				MFnDagNode totalSubdsDagNodeFn;
				assignTransformForDagFn("_" + sv->toUnit.toString() + "_total_subdivisions", totalSubdsDagNodeFn, status);
				MObject totalSubdsHandle = totalSubdsDagNodeFn.object();
				svGroupDagNodeFn.addChild(totalSubdsHandle);

				for (const auto& subdivision : totalOccludedVolumesByShadeVectors[sv]) {

					makeSubdMesh(subdivision, subdivisionSize, ++subdCounter, totalSubdsDagNodeFn);
				}
			}

			for (const auto& neighbor : sv->neighborShadeVectors) {

				if (nextLevel.find(neighbor.neighbor.get()) == nextLevel.end()) {

					nextLevel.insert(neighbor.neighbor.get());
				}
			}
		}

		MGlobal::displayInfo(MString() + "*** Level " + level + " total: " + totalBlockedThisLevel + " ***");

		thisLevel = nextLevel;
		level++;
	}
}

void BlockPointGrid::createShadeVectorUnitTransform(MObject& handle, ShadeVector* sv, MFnDagNode& debugGroupDagNodeFn,
	std::unordered_map<ShadeVector*, std::map<std::string, ChannelGroup>>& shadeVectorChannels) {

	std::map<std::string, ChannelGroup> channels;
	for (auto& shared : sv->neighborShadeVectors) {

		channels[shared.neighbor->toUnit.toString() + "_prcnt"] = { shared.percentShared };
		channels[shared.neighbor->toUnit.toString() + "_total"] = { shared.sharedBlockage };
	}
	channels["totalVolumeBlocked"] = sv->volumeBlocked;
	handle = SimpleShapes::makeCube(sv->toUnit.toMVector() * unitSize, unitSize, "debug_unit_" + sv->toUnit.toMString(), channels);
	SimpleShapes::setObjectMaterial(handle, defaultShadingGroup);
	debugGroupDagNodeFn.addChild(handle);

	shadeVectorChannels[sv] = channels;
}

void BlockPointGrid::makeSubdMesh(const MVector& subdLoc, double subdSize, int subdCounter, MFnDagNode& parent) {

	std::string subdName = "subd_" + std::to_string(subdLoc.x) + "_" + std::to_string(subdLoc.y) + "_" + std::to_string(subdLoc.z) + "_" + std::to_string(subdCounter);
	MObject sdTransform = SimpleShapes::makeCube(subdLoc, subdSize, subdName.c_str());
	SimpleShapes::setObjectMaterial(sdTransform, defaultShadingGroup);
	parent.addChild(sdTransform);
	MFnDagNode sdGrpDagNodeFn(sdTransform);
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

		if (unit.getShadePercentage() >= displayPercentageThreshhold) {

			if (unit.getCubeTransformNode().isNull()) 
				makeUnitCubeMesh(unit);

			unit.setCubeShadePlug();
			unit.setUVsToTile(transparencyTileMapTileSize, maxVolumeBlocked, uvOffSet);
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

		if (unit.getShadePercentage() >= displayPercentageThreshhold) {

			if (unit.getArrowTransformNode().isNull()) 
				makeUnitArrowMesh(unit);

			unit.setArrowShadePlug();
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

void BlockPointGrid::setShadingGroups() {

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
			MGlobal::displayWarning("Shading group not found for material: " + materialName);
	}
}

MVector BlockPointGrid::getObjectTranslation(MObject node, MStatus& status) {

	MFnDagNode dagNode(node, &status);
	MDagPath dagPath;
	dagNode.getPath(dagPath);
	MFnTransform transform(dagPath, &status);
	return transform.getTranslation(MSpace::kWorld, &status);
}