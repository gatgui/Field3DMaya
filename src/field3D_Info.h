#ifndef FIELD3D_MAYA_VRAYMATRIX
#define FIELD3D_MAYA_VRAYMATRIX

#include <maya/MPxNode.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MTime.h>
#include <maya/MVector.h>
#include <maya/MString.h>

class Field3DVRayMatrix : public MPxNode
{
public:
   
   static MTypeId id;
   
   static MObject aFilename;
   static MObject aTime;
   static MObject aPartition;
   static MObject aField;
   static MObject aOverrideOffset;
   static MObject aOffset;
   static MObject aOverrideDimension;
   static MObject aDimension;
   static MObject aTransformMode;
   
   static MObject aOutPartitions;
   static MObject aOutFields;
   static MObject aOutResolution;
   static MObject aOutHasDimension;
   static MObject aOutDimension;
   static MObject aOutHasOffset;
   static MObject aOutOffset;
   static MObject aOutMatrix;
   // same as matrix but decomposed
   static MObject aOutScale;
   static MObject aOutScaleX;
   static MObject aOutScaleY;
   static MObject aOutScaleZ;
   static MObject aOutScalePivot;
   static MObject aOutScalePivotX;
   static MObject aOutScalePivotY;
   static MObject aOutScalePivotZ;
   static MObject aOutScalePivotTranslate;
   static MObject aOutScalePivotTranslateX;
   static MObject aOutScalePivotTranslateY;
   static MObject aOutScalePivotTranslateZ;
   static MObject aOutRotate;
   static MObject aOutRotateX;
   static MObject aOutRotateY;
   static MObject aOutRotateZ;
   static MObject aOutRotatePivot;
   static MObject aOutRotatePivotX;
   static MObject aOutRotatePivotY;
   static MObject aOutRotatePivotZ;
   static MObject aOutRotatePivotTranslate;
   static MObject aOutRotatePivotTranslateX;
   static MObject aOutRotatePivotTranslateY;
   static MObject aOutRotatePivotTranslateZ;
   static MObject aOutRotateOrder;
   static MObject aOutRotateAxis;
   static MObject aOutRotateAxisX;
   static MObject aOutRotateAxisY;
   static MObject aOutRotateAxisZ;
   static MObject aOutShear;
   static MObject aOutShearXY;
   static MObject aOutShearXZ;
   static MObject aOutShearYZ;
   static MObject aOutTranslate;
   static MObject aOutTranslateX;
   static MObject aOutTranslateY;
   static MObject aOutTranslateZ;

public:
   
   Field3DVRayMatrix();
   virtual ~Field3DVRayMatrix();
   
   virtual MStatus compute(const MPlug &plug, MDataBlock &data);
   
   static void* creator();
   static MStatus initialize();
   
private:
   
   void reset();
   void update(const MString &filename, MTime t, double eps=0.0001);
   
   char *mBuffer;
   size_t mBufferLength;
   MString mLastFilename;
   std::string mFramePattern;
   MTime mLastTime;
   
   MVector mTranslate;
   double mRotate[3];
   MTransformationMatrix::RotationOrder mRotateOrder;
   double mScale[3];
   double mShear[3];
};

#endif