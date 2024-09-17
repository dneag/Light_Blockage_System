
#include "SimpleShapes.h"

MObject SimpleShapes::makeCube(const MPoint& location, double size, MString name, std::map<std::string, ChannelGroup>& channels) {

	MObject cube = makeCube(location, size, name);
	int channelNum = 0;
	for (auto& [name, cGrp] : channels)
		cGrp.handle = createChannel(cube, name.c_str(), cGrp.initialValue, ++channelNum);

	MFnDagNode nodeFn;
	nodeFn.setObject(cube);

	return cube;
}

MObject SimpleShapes::makeCube(const MPoint& location, double size, MString name) {

	return makeBox(location, size, size, size, name);
}

MObject SimpleShapes::makeBox(const MPoint& center, double xSize, double ySize, double zSize, MString name) {

	double halfX = xSize / 2.;
	double halfY = ySize / 2.;
	double halfZ = zSize / 2.;

	// Create all vertex points and add them to the list
	std::vector<MPoint> vertLocs;
	vertLocs.push_back({ center.x - halfX, center.y - halfY, center.z - halfZ });
	vertLocs.push_back({ center.x - halfX, center.y - halfY, center.z + halfZ });
	vertLocs.push_back({ center.x + halfX, center.y - halfY, center.z + halfZ });
	vertLocs.push_back({ center.x + halfX, center.y - halfY, center.z - halfZ });
	vertLocs.push_back({ center.x - halfX, center.y + halfY, center.z - halfZ });
	vertLocs.push_back({ center.x - halfX, center.y + halfY, center.z + halfZ });
	vertLocs.push_back({ center.x + halfX, center.y + halfY, center.z + halfZ });
	vertLocs.push_back({ center.x + halfX, center.y + halfY, center.z - halfZ });

	// Add the vertices to the Maya array
	MFloatPointArray cubeVertLocs;
	for (int i = 0; i < 8; i++)
		cubeVertLocs.append(MFloatPoint(static_cast<float>(vertLocs[i].x), static_cast<float>(vertLocs[i].y), static_cast<float>(vertLocs[i].z)));

	// A cube always has 6 faces and each face is always a quad
	MIntArray iaCubeFaceCounts;
	for (int i = 0; i < 6; i++)
		iaCubeFaceCounts.append(4);

	MIntArray iaCubeFaceConnects;

	// Add faceConnects for the bottom face
	iaCubeFaceConnects.append(0);
	iaCubeFaceConnects.append(3);
	iaCubeFaceConnects.append(2);
	iaCubeFaceConnects.append(1);

	// Add faceConnects for 3 of the middle faces
	for (int i = 0; i < 3; i++) {

		iaCubeFaceConnects.append(i);
		iaCubeFaceConnects.append(i + 1);
		iaCubeFaceConnects.append(i + 4 + 1);
		iaCubeFaceConnects.append(i + 4);
	}

	// Add faceConnects for the last middle face
	iaCubeFaceConnects.append(3);
	iaCubeFaceConnects.append(0);
	iaCubeFaceConnects.append(4);
	iaCubeFaceConnects.append(7);

	// Add faceConnects for the top face
	iaCubeFaceConnects.append(4);
	iaCubeFaceConnects.append(5);
	iaCubeFaceConnects.append(6);
	iaCubeFaceConnects.append(7);

	// For us, vs, and uvConnects, the following will create a unitized set of UVs.  That is, each face will have its own UV shell with the 4 coordinates at the corners of 0-1 space
	MFloatArray us, vs;
	for (int i = 0; i < 6; i++) {
		us.append(0.); us.append(1.); us.append(1.); us.append(0.);
		vs.append(0.); vs.append(0.); vs.append(1.); vs.append(1.);
	}

	MIntArray uvConnects;
	for (int i = 0; i < us.length(); i++) {
		uvConnects.append(i);
	}

	MFnMesh fnCube;
	MObject cubeShape = fnCube.create(8, 6, cubeVertLocs, iaCubeFaceCounts, iaCubeFaceConnects, us, vs);
	fnCube.assignUVs(iaCubeFaceCounts, uvConnects);
	MFnDagNode nodeFn;
	nodeFn.setObject(cubeShape);
	nodeFn.setName(name);

	return nodeFn.object();
}

MObject SimpleShapes::makeSmallArrow(const MPoint& location, const MVector& vect, MString name, double radius) {

	MStatus status;

	const int numVerts = 4;
	const int numFaces = 3;
	MPoint vertArray[4];
	int vertIndex = 0;
	MPoint arrowBase = MPoint(0., 0., 0.) - (vect * .5);

	SphAngles sa = findVectorAngles(vect);
	// Create a space oriented to the vector
	Space vectorSpace(findVectorAngles(vect));

	// Create the first loop of vertices
	double pol = 0.;
	while (vertIndex < 3) {
		MVector cv = vectorSpace.makeVector(pol, (MH::PI * .5), radius);
		vertArray[vertIndex] = arrowBase + vectorSpace.makeVector(pol, (MH::PI * .5), radius);
		pol -= (MH::PI * 2.) / 3.;
		vertIndex++;
	}

	// Create the last vertex, completing the head of the arrow
	vertArray[vertIndex] = arrowBase + vect;

	MFloatPointArray meshVertLocs;
	MIntArray meshFaceConnects;
	MIntArray meshFaceCounts;

	for (int i = 0; i < 4; i++)
		meshVertLocs.append(MFloatPoint(static_cast<float>(vertArray[i].x), static_cast<float>(vertArray[i].y), static_cast<float>(vertArray[i].z)));

	// Make faceConnects and counts for tris (arrow tip)
	meshFaceConnects.append(0);
	meshFaceConnects.append(1);
	meshFaceConnects.append(3);
	meshFaceCounts.append(3);
	meshFaceConnects.append(1);
	meshFaceConnects.append(2);
	meshFaceConnects.append(3);
	meshFaceCounts.append(3);
	meshFaceConnects.append(2);
	meshFaceConnects.append(0);
	meshFaceConnects.append(3);
	meshFaceCounts.append(3);

	MFnMesh fnArrow;
	MObject arrow = fnArrow.create(numVerts, numFaces, meshVertLocs, meshFaceCounts, meshFaceConnects);

	// Give our object a name
	MFnTransform nodeFn(arrow, &status);
	nodeFn.setName(name);

	nodeFn.setTranslation(MVector(location), MSpace::kTransform);

	MString hardenEdgesCommand = "polySoftEdge -a 0 -ch 0 " + name;
	MGlobal::executeCommand(hardenEdgesCommand);

	return arrow;
}

MObject SimpleShapes::makeSphere(const MPoint& location, double radius, std::string name, std::map<std::string, double> channels) {

	MObject sphere = makeSphere(location, radius, name);

	int channelNum = 0;
	for (const auto& [key, value] : channels) {
		createChannel(sphere, key.c_str(), value, ++channelNum);
	}

	return sphere;
}

MObject SimpleShapes::makeSphere(const MPoint& location, double radius, std::string name)
{
	int axisDivisions = 8;
	int heightDivisions = 8;

	int sphereDivisions = axisDivisions - 1;
	int sphereSides = heightDivisions;
	int numSphereVerts = (sphereDivisions * sphereSides) + 2;
	int numSphereFaces = (sphereDivisions + 1) * sphereSides;

	std::vector<MPoint> sphereVertList(numSphereVerts);
	sphereVertList[0] = MPoint(0., -radius, 0.);
	int vertIndex = 1;
	double azi = MH::PI;
	double polarIncrement = (MH::PI * 2.) / sphereSides;
	double aziIncrement = MH::PI / (sphereDivisions + 1);

	for (int i = 0; i < sphereDivisions; i++) {

		double pol = 0.;
		azi -= aziIncrement;
		for (int j = 0; j < sphereSides; j++)
		{
			double lengthTimesSinAzi = radius * std::sin(azi);

			sphereVertList[vertIndex].x = (lengthTimesSinAzi * std::cos(pol));
			sphereVertList[vertIndex].y = (radius * std::cos(azi));
			sphereVertList[vertIndex].z = (lengthTimesSinAzi * std::sin(pol));
			pol -= polarIncrement;
			vertIndex++;
		}
	}

	sphereVertList[vertIndex] = MPoint(0., radius, 0.);

	int lastVertIndex = vertIndex;

	MFloatPointArray sphereVertLocs;
	for (int i = 0; i < numSphereVerts; i++)
		sphereVertLocs.append(MFloatPoint(static_cast<float>(sphereVertList[i].x), static_cast<float>(sphereVertList[i].y), static_cast<float>(sphereVertList[i].z)));

	std::vector<int> sphereFaceCounts(numSphereFaces);
	int quadCount = (sphereDivisions - 1) * sphereSides;
	int faceIndex = 0;
	while (faceIndex < sphereSides) {
		sphereFaceCounts[faceIndex] = 3;
		faceIndex++;
	}

	while (faceIndex < (sphereSides + quadCount)) {

		sphereFaceCounts[faceIndex] = 4;
		faceIndex++;
	}

	while (faceIndex < sphereSides + quadCount + sphereSides) {

		sphereFaceCounts[faceIndex] = 3;
		faceIndex++;
	}

	MIntArray iaSphereFaceCounts;
	for (int i = 0; i < numSphereFaces; i++)
		iaSphereFaceCounts.append(sphereFaceCounts[i]);

	int triConnects = sphereSides * 2 * 3, quadConnects = quadCount * 4;
	int numSphereFaceConnects = quadConnects + triConnects;
	std::vector<int> sphereFaceConnects(numSphereFaceConnects);

	int connectIndex = 0;

	for (int side = 0; side < sphereSides; side++)
	{
		sphereFaceConnects[connectIndex] = 0;

		if (side == sphereSides - 1)
			sphereFaceConnects[connectIndex + 1] = 1;
		else
			sphereFaceConnects[connectIndex + 1] = side + 2;

		sphereFaceConnects[connectIndex + 2] = side + 1;

		connectIndex += 3;
	}

	for (int i = 0; i < sphereDivisions - 1; i++)
	{
		for (int side = 1; side <= sphereSides; side++)
		{
			int vertex = side + (i * sphereSides);

			if (side == sphereSides)
			{
				sphereFaceConnects[connectIndex] = vertex;
				sphereFaceConnects[connectIndex + 1] = vertex + 1 - sphereSides;
				sphereFaceConnects[connectIndex + 2] = vertex + 1;
				sphereFaceConnects[connectIndex + 3] = vertex + sphereSides;
			}
			else
			{
				sphereFaceConnects[connectIndex] = vertex;
				sphereFaceConnects[connectIndex + 1] = vertex + 1;
				sphereFaceConnects[connectIndex + 2] = vertex + 1 + sphereSides;
				sphereFaceConnects[connectIndex + 3] = vertex + sphereSides;
			}

			connectIndex += 4;
		}
	}


	for (int side = 1; side <= sphereSides; side++) {

		int vertex = ((sphereDivisions - 1) * sphereSides) + side;

		sphereFaceConnects[connectIndex] = vertex;

		if (side == sphereSides)
			sphereFaceConnects[connectIndex + 1] = (vertex + 1) - sphereSides;
		else
			sphereFaceConnects[connectIndex + 1] = vertex + 1;

		sphereFaceConnects[connectIndex + 2] = lastVertIndex;

		connectIndex += 3;
	}

	MIntArray iaSphereFaceConnects;
	for (int i = 0; i < numSphereFaceConnects; i++)
		iaSphereFaceConnects.append(sphereFaceConnects[i]);

	MFnMesh fnSphere;
	MObject sphere = fnSphere.create(numSphereVerts, numSphereFaces, sphereVertLocs, iaSphereFaceCounts, iaSphereFaceConnects);

	MFnTransform nodeFn(sphere);
	nodeFn.setName(name.c_str());
	nodeFn.setTranslation(MVector(location), MSpace::kTransform);

	return sphere;
}

void SimpleShapes::setObjectMaterial(MObject& shapeNode, MObject& shadingGroup) {

	if (shadingGroup.isNull()) {
		MGlobal::displayError(MString() + "Shading group not found: " + MFnDagNode(shadingGroup).name());
		return;
	}

	// shadingGroup is of type kShadingEngine, which has the MFnSet function set.  I think this is because a single kShadingEngine object can be applied to multiple meshes
	MFnSet fnSet(shadingGroup);

	// The following two lines do the same thing - add the shape node to the shading group.  The wacky second one suppresses the output message to the script editor.
	//fnSet.addMember(shapeNode);
	MGlobal::executeCommand("catch(`sets -edit -forceElement " + fnSet.name() + " " + MFnDagNode(shapeNode).fullPathName() + "`);", false, false);
}

MPlug SimpleShapes::createChannel(MObject obj, MString name, double value, int channelNum) {

	MStatus status;

	if (name.length() < 2)
		MGlobal::displayInfo(MString() + __FUNCTION__ + " Warning: channel name has less than 2 chars: " + name);

	// Create channels for Unit Density and Unit Blockage and get handles to each
	MFnDependencyNode objFn(obj);
	MFnNumericAttribute attrFn;
	std::string briefName = "c_" + std::to_string(channelNum);
	MObject attr = attrFn.create(name, briefName.c_str(), MFnNumericData::kFloat, status);
	attrFn.setKeyable(true);
	attrFn.setStorable(true);
	attrFn.setWritable(true);
	attrFn.setReadable(true);
	objFn.addAttribute(attr);

	MPlug plg = objFn.findPlug(attr, true);

	plg.setLocked(false);
	plg.setValue(value);
	plg.setLocked(true);

	return plg;
}

void SimpleShapes::unlockRotates(MString objName) {

	MString command = "setAttr - lock false " + objName + ".rx";
	MGlobal::executeCommand(command);
	command = "setAttr - lock false " + objName + ".ry";
	MGlobal::executeCommand(command);
	command = "setAttr - lock false " + objName + ".rz";
	MGlobal::executeCommand(command);
}

void SimpleShapes::lockRotates(MString objName) {

	MString command = "setAttr - lock true " + objName + ".rx";
	MGlobal::executeCommand(command);
	command = "setAttr - lock true " + objName + ".ry";
	MGlobal::executeCommand(command);
	command = "setAttr - lock true " + objName + ".rz";
	MGlobal::executeCommand(command);
}

void SimpleShapes::lockTransforms(MString objName) {

	MString command = "setAttr - lock true " + objName + ".tx";
	MGlobal::executeCommand(command);
	command = "setAttr - lock true " + objName + ".ty";
	MGlobal::executeCommand(command);
	command = "setAttr - lock true " + objName + ".tz";
	MGlobal::executeCommand(command);
	command = "setAttr - lock true " + objName + ".rx";
	MGlobal::executeCommand(command);
	command = "setAttr - lock true " + objName + ".ry";
	MGlobal::executeCommand(command);
	command = "setAttr - lock true " + objName + ".rz";
	MGlobal::executeCommand(command);
	command = "setAttr - lock true " + objName + ".sx";
	MGlobal::executeCommand(command);
	command = "setAttr - lock true " + objName + ".sy";
	MGlobal::executeCommand(command);
	command = "setAttr - lock true " + objName + ".sz";
	MGlobal::executeCommand(command);
}