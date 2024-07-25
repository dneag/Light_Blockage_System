#include "BlockPoint.h"

MObject BlockPoint::createBPMesh(MFnDagNode& bpMeshGroupDagNodeFn, MObject& shadingGroup) {

	std::map<std::string, double> channels;
	channels["Density"] = density;
	bpTransformNode = SimpleShapes::makeSphere(loc, radius, "bp", channels);
	bpMeshGroupDagNodeFn.addChild(bpTransformNode);

	MStatus status;
	MFnDagNode nodeFn;
	nodeFn.setObject(bpTransformNode);
	MObject bpShapeNode;

	for (unsigned int i = 0; i < nodeFn.childCount(); ++i) {
		MObject child = nodeFn.child(i, &status);
		if (status == MStatus::kSuccess && child.hasFn(MFn::kMesh)) {
			bpShapeNode = child;
			break;
		}
	}

	if (bpShapeNode.isNull()) {
		MGlobal::displayError("Could not find shape node for unit " + name + " cube mesh");
	}

	MFnMesh fnShape(bpShapeNode, &status);
	if (status != MStatus::kSuccess) {
		MGlobal::displayError("Failed to set shape node to MFnMesh function set: " + status.errorString());
	}

	// shadingGroup is of type kShadingEngine, which has the MFnSet function set.
	MFnSet fnSet(shadingGroup);

	// The following two lines do the same thing - add the shape node to the shading group.  The second one suppresses the output message to the script editor
	//fnSet.addMember(shapeNode);
	MGlobal::executeCommand("catch(`sets -edit -forceElement " + fnSet.name() + " " + MFnDagNode(bpShapeNode).fullPathName() + "`);", false, false);

	return bpTransformNode;
}