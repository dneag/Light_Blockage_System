//
//  pluginMain.cpp
//

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

MStatus initializePlugin(MObject obj)
{
    MStatus status;

    MFnPlugin fnPlugin(obj, "Dan Neag", "1.0", "Any");

    MGlobal::displayInfo("Plugin has been initialized");
   /* status = fnPlugin.registerCommand("displayNearSelectedPoint", DisplayNearSelectedPoint::creator, DisplayNearSelectedPoint::newSyntax);
    CHECK_MSTATUS_AND_RETURN_IT(status);*/

    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus status;

    MFnPlugin fnPlugin(obj);

    MGlobal::displayInfo("Plugin has been uninitialized");
    /*status = fnPlugin.deregisterCommand("displayNearSelectedPoint");
    CHECK_MSTATUS_AND_RETURN_IT(status);*/

    return MS::kSuccess;
}

