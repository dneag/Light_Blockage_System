#pragma once

#include <string>
#include <map>
#include <vector>

#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFnMesh.h>
#include <maya/MFnTransform.h>
#include <maya/MFloatArray.h>
#include <maya/MFnSet.h>

#include "MathHelper.h"

class SimpleShapes {

public:

	// Creates a cube and returns the transform object associated with the cube's shape node
	static MObject makeCube(const MPoint& location, double size, MString name);

	static MObject makeCube(const MPoint& location, double size, MString name, std::map<std::string, double> channels);

	static MObject makeBox(const MPoint& center, double xSize, double ySize, double zSize, MString name);

	static MObject makeSmallArrow(const MPoint& location, const MVector& vect, MString name, double radius);

	static MObject makeSphere(const MPoint& location, double radius, std::string name);

	static MObject makeSphere(const MPoint& location, double radius, std::string name, std::map<std::string, double> channels);

	static void createChannel(MObject obj, MString name, double value);

	static void setObjectMaterial(MObject& shapeNode, MObject& shadingGroup);

	static void unlockRotates(MString objName);

	static void lockRotates(MString objName);

	static void lockTransforms(MString objName);

};