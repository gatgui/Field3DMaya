#ifndef FIELD3D_MAYA_FORCELOAD_H
#define FIELD3D_MAYA_FORCELOAD_H

#include <maya/MPxNode.h>

class Field3DForceLoad : public MPxNode
{
public:
   static MTypeId id;

public:
   
   Field3DForceLoad();
   virtual ~Field3DForceLoad();
   
   virtual MStatus compute(const MPlug &plug, MDataBlock &data);
   
   static void* creator();
   static MStatus initialize();
};

#endif
