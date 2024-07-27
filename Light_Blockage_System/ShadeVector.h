#pragma once

#include <queue>
#include <unordered_set>

#include <maya/MVector.h>

#include "Point_Int.h"

/*
	ShadeVector objects serve as nodes in the tree data structure rooted at the shadeRoot member variable of a BlockPointGrid.
	Each represents a vector emitted from an obstructed point in space, which when applied to its destination unit, reduces the amount and alters the direction of light.
	See BlockPointGrid::createShadeIndexTree() for more details.
*/
struct ShadeVector {

	double proximity = 0.;

	// The maximum amount of shade this ShadeIndex can apply to a unit
	double shadeStrength = 0.;

	// A vector pointing from the shadeRoot to the unit this ShadeVector shades. The length of this vector is shadeStrength divided by the number of paths available 
	// through the tree to reach the shaded unit
	MVector shadeVector = MVector(0., 0., 0.);

	// The shadeVector multiplied by the number of times this ShadeVector's or any parent ShadeVector's paths have converged
	// This is required to make propagation more effiecient. Note that its value is unique to each propagation.
	MVector combinedShadeVector = MVector(0., 0., 0.);

	// A 3D integer vector to the unit shaded by this ShadeVector.  That is, this vector can be added to any 3D index in BlockPointGrid::grid, and will result
	// in the index of the grid unit that this ShadeVector would be applied to.
	Point_Int toUnit;

	// A list of pointers to child ShadeVectors
	std::vector<std::shared_ptr<ShadeVector>> blockedShadeVectors;

	// The number of converged paths that are propagating with this ShadeVector.
	int convergedPaths = 1;

	ShadeVector(const Point_Int& toUnit) : toUnit(toUnit) {}

	ShadeVector(const Point_Int& toUnit, double proximity) : toUnit(toUnit), proximity(proximity) {}

	void setShadeVectors(const MVector& v) {

		shadeVector = v;
		combinedShadeVector = v;
	}

	void eraseBlockee(std::shared_ptr<ShadeVector> shadeIndex) {

		blockedShadeVectors.erase(std::remove(blockedShadeVectors.begin(), blockedShadeVectors.end(), shadeIndex), blockedShadeVectors.end());
	}

	// Add child ShadeVector nodes to the queue.  The 'encountered' list will have been reset before each propagation.  If a child has already been 
	// encountered, it means we have reached it via a different path, so add to its converged paths.
	void getBlocked(std::queue<std::shared_ptr<ShadeVector>>& vectorsToUnits, std::unordered_set<std::shared_ptr<ShadeVector>>& encountered, int pathsOfParent) {

		for (auto& v : blockedShadeVectors) {

			if (encountered.find(v) == encountered.end()) {

				v->convergedPaths = pathsOfParent;
				vectorsToUnits.push(v);
				encountered.insert(v);
			}
			else {

				v->convergedPaths += pathsOfParent;
			}

			v->combinedShadeVector = v->shadeVector * v->convergedPaths;
		}
	}
};

