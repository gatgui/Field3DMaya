#ifndef FIELD3D_MAYA_INFO
#define FIELD3D_MAYA_INFO

#include <maya/MPxNode.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MTime.h>
#include <maya/MVector.h>
#include <maya/MPoint.h>
#include <maya/MMatrix.h>
#include <maya/MString.h>
#include <vector>
#include <string>
#include "field3D_Tools.h"

class Field3DInfo : public MPxNode
{
public:
   
   static MTypeId id;
   
   static MObject aFilename;
   static MObject aTime;
   static MObject aPartition;
   static MObject aField;
   static MObject aForceDimension;
   static MObject aDimension;
   static MObject aDimensionX;
   static MObject aDimensionY;
   static MObject aDimensionZ;
   static MObject aTransformMode;
   
   static MObject aOutPartitions;
   static MObject aOutFields;
   static MObject aOutResolution;
   static MObject aOutResolutionX;
   static MObject aOutResolutionY;
   static MObject aOutResolutionZ;
   static MObject aOutHasDimension;
   static MObject aOutDimension;
   static MObject aOutDimensionX;
   static MObject aOutDimensionY;
   static MObject aOutDimensionZ;
   static MObject aOutHasOffset;
   static MObject aOutOffset;
   static MObject aOutOffsetX;
   static MObject aOutOffsetY;
   static MObject aOutOffsetZ;
   static MObject aOutMatrix;
   static MObject aOutMatrixInverse;
   static MObject aOutRawMatrix;
   static MObject aOutRawMatrixInverse;
   
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
   
   Field3DInfo();
   virtual ~Field3DInfo();
   
   virtual MStatus compute(const MPlug &plug, MDataBlock &data);
   
   static void* creator();
   static MStatus initialize();
   
private:
   
   enum TransformMode
   {
      TM_raw = 0,
      TM_fluid,
      TM_raw_inverse,
      TM_fluid_inverse
   };
   
   void reset();
   void resetOffset();
   void resetDimension();
   void resetResolution();
   void resetTransform();
   void resetFile();
   void update(const MString &filename, MTime t,
               const MString &partition, const MString &field,
               bool forceDimension, const MPoint &dimension,
               TransformMode transformMode,
               double eps=0.0001);
   
   char *mBuffer;
   size_t mBufferLength;
   bool mFirstUpdate;
   MString mLastFilename;
   std::string mFramePattern;
   MTime mLastTime;
   MString mLastPartition;
   MString mLastField;
   bool mLastForceDimension;
   MPoint mLastDimension;
   TransformMode mLastTransformMode;
   Field3D::Field3DInputFile *mFile;
   Field3D::EmptyField<float>::Ptr mField;
   
   std::vector<std::string> mPartitions;
   std::vector<std::string> mFields;
   MPoint mResolution;
   bool mHasOffset;
   MPoint mOffset;
   bool mHasDimension;
   MPoint mDimension;
   MVector mTranslate;
   double mRotate[3];
   MTransformationMatrix::RotationOrder mRotateOrder;
   double mScale[3];
   double mShear[3];
   MMatrix mMatrix;
   MMatrix mMatrixInverse;
   MMatrix mRawMatrix;
   MMatrix mRawMatrixInverse;
};

#endif