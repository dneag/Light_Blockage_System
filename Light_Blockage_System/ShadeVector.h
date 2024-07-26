#pragma once

#include <queue>
#include <unordered_set>

#include <maya/MVector.h>

#include "Point_Int.h"

/*
	ShadeVector objects serve as nodes in the tree data structure rooted at the shadeRoot member variable of a BlockPointGrid.
	Each represents a vector emitted from an obstructed point in space, which when applied to its destination unit, reduces the amount and alters the direction of light.
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

	// A 3D integer vector which, when added to a grid unit, results in the index in the BlockPointGrid::grid 3D array of the unit it shades
	Point_Int toUnit;

	// A list of pointers to child ShadeVectors
	std::vector<std::shared_ptr<ShadeVector>> blockedShadeVectors;

	// The number of converged paths that are propagating with this shade index.
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

