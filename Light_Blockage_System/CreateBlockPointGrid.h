#pragma once

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>

#include "BlockPointGrid.h"
#include "GridManager.h"

class CreateBlockPointGrid : public MPxCommand
{
public:

	virtual MStatus doIt(const MArgList& argList);

	static void* creator() { return new CreateBlockPointGrid; }

	static MSyntax newSyntax();
};