#include "field3D_ForceLoad.h"

MTypeId Field3DForceLoad::id(0x5E5FF);

Field3DForceLoad::Field3DForceLoad()
   : MPxNode()
{
}

Field3DForceLoad::~Field3DForceLoad()
{
}

MStatus Field3DForceLoad::compute(const MPlug &plug, MDataBlock &data)
{
   return MStatus::kUnknownParameter;
}

void* Field3DForceLoad::creator()
{
   return new Field3DForceLoad();
}

MStatus Field3DForceLoad::initialize()
{
   return MStatus::kSuccess;
}
