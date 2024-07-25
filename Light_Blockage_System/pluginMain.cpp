//
//  pluginMain.cpp
//

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

#include "CreateBlockPointGrid.h"
#include "ModifyBlockPoints.h"
#include "UpdateGridDisplay.h"

MStatus initializePlugin(MObject obj)
{
    MStatus status;

    MFnPlugin fnPlugin(obj, "Dan Neag", "1.0", "Any");

    status = fnPlugin.registerCommand("createBlockPointGrid", CreateBlockPointGrid::creator, CreateBlockPointGrid::newSyntax);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.registerCommand("modifyBlockPoints", ModifyBlockPoints::creator, ModifyBlockPoints::newSyntax);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.registerCommand("updateGridDisplay", UpdateGridDisplay::creator, UpdateGridDisplay::newSyntax);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus status;

    MFnPlugin fnPlugin(obj);

    status = fnPlugin.deregisterCommand("createBlockPointGrid");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.deregisterCommand("modifyBlockPoints");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.deregisterCommand("updateGridDisplay");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

