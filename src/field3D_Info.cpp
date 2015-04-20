#include "field3D_Info.h"
#include "maya_Tools.h"
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MAngle.h>
#include <maya/MArrayDataBuilder.h>
#include <cmath>
#include <limits>

using namespace Field3D;

MTypeId Field3DInfo::id(0x5E5FE);

MObject Field3DInfo::aFilename;
MObject Field3DInfo::aTime;
MObject Field3DInfo::aPartition;
MObject Field3DInfo::aField;
MObject Field3DInfo::aForceDimension;
MObject Field3DInfo::aDimension;
MObject Field3DInfo::aDimensionX;
MObject Field3DInfo::aDimensionY;
MObject Field3DInfo::aDimensionZ;
MObject Field3DInfo::aTransformMode;

MObject Field3DInfo::aOutPartitions;
MObject Field3DInfo::aOutFields;
MObject Field3DInfo::aOutResolution;
MObject Field3DInfo::aOutResolutionX;
MObject Field3DInfo::aOutResolutionY;
MObject Field3DInfo::aOutResolutionZ;
MObject Field3DInfo::aOutHasDimension;
MObject Field3DInfo::aOutDimension;
MObject Field3DInfo::aOutDimensionX;
MObject Field3DInfo::aOutDimensionY;
MObject Field3DInfo::aOutDimensionZ;
MObject Field3DInfo::aOutHasOffset;
MObject Field3DInfo::aOutOffset;
MObject Field3DInfo::aOutOffsetX;
MObject Field3DInfo::aOutOffsetY;
MObject Field3DInfo::aOutOffsetZ;
MObject Field3DInfo::aOutMatrix;

MObject Field3DInfo::aOutScale;
MObject Field3DInfo::aOutScaleX;
MObject Field3DInfo::aOutScaleY;
MObject Field3DInfo::aOutScaleZ;
MObject Field3DInfo::aOutScalePivot;
MObject Field3DInfo::aOutScalePivotX;
MObject Field3DInfo::aOutScalePivotY;
MObject Field3DInfo::aOutScalePivotZ;
MObject Field3DInfo::aOutScalePivotTranslate;
MObject Field3DInfo::aOutScalePivotTranslateX;
MObject Field3DInfo::aOutScalePivotTranslateY;
MObject Field3DInfo::aOutScalePivotTranslateZ;
MObject Field3DInfo::aOutRotate;
MObject Field3DInfo::aOutRotateX;
MObject Field3DInfo::aOutRotateY;
MObject Field3DInfo::aOutRotateZ;
MObject Field3DInfo::aOutRotatePivot;
MObject Field3DInfo::aOutRotatePivotX;
MObject Field3DInfo::aOutRotatePivotY;
MObject Field3DInfo::aOutRotatePivotZ;
MObject Field3DInfo::aOutRotatePivotTranslate;
MObject Field3DInfo::aOutRotatePivotTranslateX;
MObject Field3DInfo::aOutRotatePivotTranslateY;
MObject Field3DInfo::aOutRotatePivotTranslateZ;
MObject Field3DInfo::aOutRotateOrder;
MObject Field3DInfo::aOutRotateAxis;
MObject Field3DInfo::aOutRotateAxisX;
MObject Field3DInfo::aOutRotateAxisY;
MObject Field3DInfo::aOutRotateAxisZ;
MObject Field3DInfo::aOutShear;
MObject Field3DInfo::aOutShearXY;
MObject Field3DInfo::aOutShearXZ;
MObject Field3DInfo::aOutShearYZ;
MObject Field3DInfo::aOutTranslate;
MObject Field3DInfo::aOutTranslateX;
MObject Field3DInfo::aOutTranslateY;
MObject Field3DInfo::aOutTranslateZ;

#define CHECK_ERROR { if (stat != MS::kSuccess) { stat.perror("Field3DInfo"); return stat; } }

#define ADD_XYZ_INPUT_ATTR(attr, longName, shortName, defVal)\
  attr = cattr.create(longName, shortName, &stat); CHECK_ERROR; \
  attr ## X = nattr.create(longName "X", shortName "x", MFnNumericData::kDouble, defVal.x, &stat); CHECK_ERROR; \
  nattr.setStorable(true);\
  nattr.setReadable(true);\
  nattr.setWritable(true);\
  nattr.setKeyable(true);\
  attr ## Y = nattr.create(longName "Y", shortName "y", MFnNumericData::kDouble, defVal.y, &stat); CHECK_ERROR; \
  nattr.setStorable(true);\
  nattr.setReadable(true);\
  nattr.setWritable(true);\
  nattr.setKeyable(true);\
  attr ## Z = nattr.create(longName "Z", shortName "z", MFnNumericData::kDouble, defVal.z, &stat); CHECK_ERROR; \
  nattr.setStorable(true);\
  nattr.setReadable(true);\
  nattr.setWritable(true);\
  nattr.setKeyable(true);\
  stat = cattr.addChild(attr ## X); CHECK_ERROR; \
  stat = cattr.addChild(attr ## Y); CHECK_ERROR; \
  stat = cattr.addChild(attr ## Z); CHECK_ERROR; \
  stat = addAttribute(attr); CHECK_ERROR

#define ADD_XYZ_OUTPUT_ATTR(attr, longName, shortName, defVal)\
  attr = cattr.create(longName, shortName, &stat); CHECK_ERROR; \
  attr ## X = nattr.create(longName "X", shortName "x", MFnNumericData::kDouble, defVal.x, &stat); CHECK_ERROR; \
  nattr.setStorable(false);\
  nattr.setReadable(true);\
  nattr.setWritable(false);\
  nattr.setHidden(true);\
  attr ## Y = nattr.create(longName "Y", shortName "y", MFnNumericData::kDouble, defVal.y, &stat); CHECK_ERROR; \
  nattr.setStorable(false);\
  nattr.setReadable(true);\
  nattr.setWritable(false);\
  nattr.setHidden(true);\
  attr ## Z = nattr.create(longName "Z", shortName "z", MFnNumericData::kDouble, defVal.z, &stat); CHECK_ERROR; \
  nattr.setStorable(false);\
  nattr.setReadable(true);\
  nattr.setWritable(false);\
  nattr.setHidden(true);\
  cattr.setHidden(true);\
  stat = cattr.addChild(attr ## X); CHECK_ERROR; \
  stat = cattr.addChild(attr ## Y); CHECK_ERROR; \
  stat = cattr.addChild(attr ## Z); CHECK_ERROR; \
  stat = addAttribute(attr); CHECK_ERROR

#define AFFECTS_TRANSFORM_OUTPUTS(attr)\
  attributeAffects(attr, aOutTranslate);\
  attributeAffects(attr, aOutRotate);\
  attributeAffects(attr, aOutRotateOrder);\
  attributeAffects(attr, aOutRotateAxis);\
  attributeAffects(attr, aOutRotatePivot);\
  attributeAffects(attr, aOutRotatePivotTranslate);\
  attributeAffects(attr, aOutScale);\
  attributeAffects(attr, aOutScalePivot);\
  attributeAffects(attr, aOutScalePivotTranslate);\
  attributeAffects(attr, aOutShear);\
  attributeAffects(attr, aOutMatrix)


void* Field3DInfo::creator()
{
  return new Field3DInfo();
}

MStatus Field3DInfo::initialize()
{
  MStatus stat;
  
  MFnCompoundAttribute cattr;
  MFnNumericAttribute nattr;
  MFnUnitAttribute uattr;
  MFnTypedAttribute tattr;
  MFnEnumAttribute eattr;
  
  MPoint zero(0.0, 0.0, 0.0);
  MPoint one(1.0, 1.0, 1.0);
  
  // input attributes
  
  aFilename = tattr.create("filename", "fn", MFnData::kString, MObject::kNullObj, &stat); CHECK_ERROR;
  tattr.setStorable(true);
  tattr.setReadable(true);
  tattr.setWritable(true);
  tattr.setKeyable(false);
  //tattr.setUsedAsFilename(true);
  stat = addAttribute(aFilename); CHECK_ERROR;
  
  aTime = uattr.create("time", "tm", MFnUnitAttribute::kTime, 0, &stat); CHECK_ERROR;
  uattr.setStorable(true);
  uattr.setWritable(true);
  uattr.setReadable(true);
  stat = addAttribute(aTime); CHECK_ERROR;
  
  aPartition = tattr.create("partition", "prt", MFnData::kString, MObject::kNullObj, &stat); CHECK_ERROR;
  tattr.setStorable(true);
  tattr.setReadable(true);
  tattr.setWritable(true);
  tattr.setKeyable(false);
  stat = addAttribute(aPartition); CHECK_ERROR;
  
  aField = tattr.create("field", "fld", MFnData::kString);
  tattr.setStorable(true);
  tattr.setReadable(true);
  tattr.setWritable(true);
  tattr.setKeyable(false);
  stat = addAttribute(aField); CHECK_ERROR;
  
  aTransformMode = eattr.create("transformMode", "trm"); CHECK_ERROR;
  eattr.setStorable(true);
  eattr.setReadable(true);
  eattr.setWritable(true);
  eattr.setKeyable(false);
  eattr.addField("standard", 0);
  eattr.addField("fluid", 1);
  stat = addAttribute(aTransformMode); CHECK_ERROR;
  
  aForceDimension = nattr.create("forceDimension", "fdi", MFnNumericData::kBoolean, 0, &stat); CHECK_ERROR;
  nattr.setStorable(true);
  nattr.setReadable(true);
  nattr.setWritable(true);
  nattr.setKeyable(false);
  stat = addAttribute(aForceDimension); CHECK_ERROR;
  
  ADD_XYZ_INPUT_ATTR(aDimension, "dimension", "dim", one);
  
  // output attributes
  
  aOutPartitions = tattr.create("outPartitions", "opts", MFnData::kString, MObject::kNullObj, &stat); CHECK_ERROR;
  tattr.setArray(true);
  tattr.setStorable(false);
  tattr.setReadable(true);
  tattr.setWritable(false);
  tattr.setHidden(true);
  tattr.setUsesArrayDataBuilder(true);
  stat = addAttribute(aOutPartitions); CHECK_ERROR;
  
  aOutFields = tattr.create("outFields", "ofls", MFnData::kString, MObject::kNullObj, &stat); CHECK_ERROR;
  tattr.setArray(true);
  tattr.setStorable(false);
  tattr.setReadable(true);
  tattr.setWritable(false);
  tattr.setHidden(true);
  tattr.setUsesArrayDataBuilder(true);
  stat = addAttribute(aOutFields); CHECK_ERROR;
  
  ADD_XYZ_OUTPUT_ATTR(aOutResolution, "outResolution", "ores", zero);
  
  aOutHasDimension = nattr.create("outHasDimension", "ohd", MFnNumericData::kBoolean, 0, &stat); CHECK_ERROR;
  nattr.setStorable(false);
  nattr.setReadable(true);
  nattr.setWritable(false);
  nattr.setHidden(true);
  stat = addAttribute(aOutHasDimension); CHECK_ERROR;
  
  ADD_XYZ_OUTPUT_ATTR(aOutDimension, "outDimension", "odim", one);
  
  aOutHasOffset = nattr.create("outHasOffset", "oho", MFnNumericData::kBoolean, 0, &stat); CHECK_ERROR;
  nattr.setStorable(false);
  nattr.setReadable(true);
  nattr.setWritable(false);
  nattr.setHidden(true);
  stat = addAttribute(aOutHasOffset); CHECK_ERROR;
  
  ADD_XYZ_OUTPUT_ATTR(aOutOffset, "outOffset", "ooff", zero);
  
  aOutMatrix = tattr.create("outMatrix", "omtx", MFnData::kMatrix, MObject::kNullObj, &stat); CHECK_ERROR;
  tattr.setStorable(false);
  tattr.setReadable(true);
  tattr.setWritable(false);
  tattr.setHidden(true);
  stat = addAttribute(aOutMatrix); CHECK_ERROR;
  
  aOutRotateOrder = eattr.create("outRotateOrder", "oro", 0, &stat); CHECK_ERROR;
  eattr.addField("xyz", 0);
  eattr.addField("yzx", 1);
  eattr.addField("zxy", 2);
  eattr.addField("xzy", 3);
  eattr.addField("yxz", 4);
  eattr.addField("zyx", 5);
  eattr.setStorable(false);
  eattr.setReadable(true);
  eattr.setWritable(false);
  eattr.setHidden(true);
  stat = addAttribute(aOutRotateOrder); CHECK_ERROR;
  
  aOutShear = cattr.create("outShear", "osh", &stat); CHECK_ERROR;
  aOutShearXY = nattr.create("outShearXY", "oshx", MFnNumericData::kDouble, 0.0, &stat); CHECK_ERROR;
  nattr.setStorable(false);
  nattr.setReadable(true);
  nattr.setWritable(false);
  nattr.setHidden(true);
  aOutShearXZ = nattr.create("outShearXZ", "oshy", MFnNumericData::kDouble, 0.0, &stat); CHECK_ERROR;
  nattr.setStorable(false);
  nattr.setReadable(true);
  nattr.setWritable(false);
  nattr.setHidden(true);
  aOutShearYZ = nattr.create("outShearYZ", "oshz", MFnNumericData::kDouble, 0.0, &stat); CHECK_ERROR;
  nattr.setStorable(false);
  nattr.setReadable(true);
  nattr.setWritable(false);
  nattr.setHidden(true);
  cattr.setHidden(true);
  stat = cattr.addChild(aOutShearXY); CHECK_ERROR;
  stat = cattr.addChild(aOutShearXZ); CHECK_ERROR;
  stat = cattr.addChild(aOutShearYZ); CHECK_ERROR;
  stat = addAttribute(aOutShear); CHECK_ERROR;
  
  ADD_XYZ_OUTPUT_ATTR(aOutScale, "outScale", "os", one);
  ADD_XYZ_OUTPUT_ATTR(aOutScalePivot, "outScalePivot", "osp", zero);
  ADD_XYZ_OUTPUT_ATTR(aOutScalePivotTranslate, "outScalePivotTranslate", "ospt", zero);
  ADD_XYZ_OUTPUT_ATTR(aOutRotate, "outRotate", "or", zero);
  ADD_XYZ_OUTPUT_ATTR(aOutRotatePivot, "outRotatePivot", "orp", zero);
  ADD_XYZ_OUTPUT_ATTR(aOutRotatePivotTranslate, "outRotatePivotTranslate", "orpt", zero);
  ADD_XYZ_OUTPUT_ATTR(aOutRotateAxis, "outRotateAxis", "ora", zero);
  ADD_XYZ_OUTPUT_ATTR(aOutTranslate, "outTranslate", "ot", zero);
  
  // ---
  
  attributeAffects(aFilename, aOutPartitions);
  attributeAffects(aTime, aOutPartitions);
  
  attributeAffects(aFilename, aOutFields);
  attributeAffects(aTime, aOutFields);
  attributeAffects(aPartition, aOutFields);
  
  attributeAffects(aFilename, aOutHasDimension);
  attributeAffects(aTime, aOutHasDimension);
  attributeAffects(aPartition, aOutHasDimension);
  attributeAffects(aField, aOutHasDimension);
  
  attributeAffects(aFilename, aOutResolution);
  attributeAffects(aTime, aOutResolution);
  attributeAffects(aPartition, aOutResolution);
  attributeAffects(aField, aOutResolution);
  
  attributeAffects(aFilename, aOutDimension);
  attributeAffects(aTime, aOutDimension);
  attributeAffects(aPartition, aOutDimension);
  attributeAffects(aField, aOutDimension);
  attributeAffects(aForceDimension, aOutDimension);
  attributeAffects(aDimension, aOutDimension);
  
  attributeAffects(aFilename, aOutHasOffset);
  attributeAffects(aTime, aOutHasOffset);
  attributeAffects(aPartition, aOutHasOffset);
  attributeAffects(aField, aOutHasOffset);
  
  attributeAffects(aFilename, aOutOffset);
  attributeAffects(aTime, aOutOffset);
  attributeAffects(aPartition, aOutOffset);
  attributeAffects(aField, aOutOffset);
  
  AFFECTS_TRANSFORM_OUTPUTS(aFilename);
  AFFECTS_TRANSFORM_OUTPUTS(aTime);
  AFFECTS_TRANSFORM_OUTPUTS(aPartition);
  AFFECTS_TRANSFORM_OUTPUTS(aField);
  AFFECTS_TRANSFORM_OUTPUTS(aForceDimension);
  AFFECTS_TRANSFORM_OUTPUTS(aDimension);
  AFFECTS_TRANSFORM_OUTPUTS(aTransformMode);
  
  return MStatus::kSuccess;
}

Field3DInfo::Field3DInfo()
   : MPxNode()
   , mBuffer(0)
   , mBufferLength(0)
   , mFirstUpdate(true)
   , mLastForceDimension(false)
   , mLastDimension(1, 1, 1)
   , mLastTransformMode(Field3DInfo::TM_standard)
   , mFile(0)
{
  reset();
}

Field3DInfo::~Field3DInfo()
{
  if (mBuffer)
  {
    delete[] mBuffer;
    mBuffer = 0;
  }
  
  reset();
}

void Field3DInfo::reset()
{
  resetTransform();
  resetOffset();
  resetDimension();
  resetResolution();
  resetFile();
}

void Field3DInfo::resetFile()
{
  mPartitions.clear();
  mFields.clear();
  
  mField = 0;
  
  if (mFile)
  {
    delete mFile;
    mFile = 0;
  }
}

void Field3DInfo::resetOffset()
{
  mOffset = MPoint(0.0, 0.0, 0.0);
  mHasOffset = false;
}

void Field3DInfo::resetDimension()
{
  mDimension = MPoint(1.0, 1.0, 1.0);
  mHasDimension = false;
}

void Field3DInfo::resetResolution()
{
  mResolution = MPoint(0.0, 0.0, 0.0);
}

void Field3DInfo::resetTransform()
{
  mTranslate.x = 0.0;
  mTranslate.y = 0.0;
  mTranslate.z = 0.0;
  
  mRotate[0] = 0.0;
  mRotate[1] = 0.0;
  mRotate[2] = 0.0;
  
  mRotateOrder = MTransformationMatrix::kXYZ;
  
  mScale[0] = 1.0;
  mScale[1] = 1.0;
  mScale[2] = 1.0;
  
  mShear[0] = 0.0;
  mShear[1] = 0.0;
  mShear[2] = 0.0;
  
  mMatrix.setToIdentity();
}

void Field3DInfo::update(const MString &filename, MTime t,
                         const MString &partition, const MString &field,
                         bool forceDimension, const MPoint &dimension,
                         TransformMode transformMode,
                         double eps)
{
  bool forceUpdate = mFirstUpdate;
  
  if (filename != mLastFilename ||
      fabs(mLastTime.as(MTime::uiUnit()) - t.as(MTime::uiUnit())) > eps)
  {
    if (mFile)
    {
      delete mFile;
      mFile = 0;
    }
    
    int fullframe = int(floor(t.as(MTime::uiUnit()) + 0.5));
    
    if (filename != mLastFilename)
    {
      // Re-allocate filename buffer for frame expansion if necessary
      size_t len = filename.length() + 32;
      
      if (mBufferLength < len)
      {
        if (mBuffer)
        {
          delete[] mBuffer;
        }
        
        mBuffer = new char[len];
        mBufferLength = len;
      }
      
      sprintf(mBuffer, filename.asChar(), fullframe);
      
      if (filename == mBuffer)
      {
        // no changes -> filename doesn't contain printf style frame pattern
        
        std::string tmp = filename.asChar();
        
        size_t p0 = tmp.find_last_of("\\/");
        
        size_t p1 = tmp.rfind('#');
        
        if (p1 != std::string::npos && (p0 == std::string::npos || p1 > p0))
        {
          // # style frame pattern
          size_t p2 = tmp.find_last_not_of("#", p1);
          
          size_t n = p1 - p2;
          
          char pat[16];
          
          if (n > 1)
          {
            sprintf(pat, "%%0%dd", int(n));
          }
          else
          {
            sprintf(pat, "%%d");
          }
          
          mFramePattern = tmp.substr(0, p2 + 1) + pat + tmp.substr(p1+1);
        }
        else
        {
          // No frame pattern
          mFramePattern = filename.asChar();
        }
      }
      else
      {
        // printf style frame pattern
        mFramePattern = filename.asChar();
      }
      
      // Do directory mapping ourselves
      MayaTools::dirmap(mFramePattern);
      
      // MGlobal::displayInfo(MString("Use frame pattern: ") + mFramePattern.c_str());
    }
    
    if (mBuffer)
    {
      sprintf(mBuffer, mFramePattern.c_str(), fullframe);
      
      Field3D::Field3DInputFile *f3dIn = new Field3D::Field3DInputFile();
      
      if (f3dIn->open(mBuffer))
      {
        //MGlobal::displayWarning(MString("Invalid f3d file \"") + mBuffer + "\"");
        mFile = f3dIn;
      }
      else
      {
        delete f3dIn;
      }
    }
    
    forceUpdate = true;
  }
  
  if (mFile)
  {
    if (forceUpdate || partition != mLastPartition)
    {
      mPartitions.clear();
      
      if (partition.length() == 0)
      {
        mFile->getPartitionNames(mPartitions);
      }
      else
      {
        mPartitions.push_back(partition.asChar());
      }
      
      forceUpdate = true;
    }
    
    if (forceUpdate || field != mLastField)
    {
      mFields.clear();
      
      if (field.length() == 0)
      {
        if (!mPartitions.empty())
        {
          Field3DTools::getFieldNames(mFile, mPartitions[0], mFields);
        }
      }
      else
      {
        mFields.push_back(field.asChar());
      }
      
      forceUpdate = true;
    }
    
    // Get pointer to target field
    
    if (forceUpdate)
    {
      mField = 0;
      
      if (!mPartitions.empty() && !mFields.empty())
      {
        Field3D::EmptyField<float>::Vec sl = mFile->readProxyLayer<float>(mPartitions[0], mFields[0], false);
        
        if (sl.empty())
        {
          sl = mFile->readProxyLayer<float>(mPartitions[0], mFields[0], true);
          
          if (!sl.empty())
          {
            mField = sl[0];
          }
        }
        else
        {
          mField = sl[0];
        }
      }
    }
    
    if (mField)
    {
      static Field3D::V3f dv(std::numeric_limits<float>::max(),
                             std::numeric_limits<float>::max(),
                             std::numeric_limits<float>::max());
      
      if (forceUpdate)
      {
        // refresh field resolution and offset
        
        Field3D::V3i res = mField->dataResolution();
        mResolution = MPoint(res.x, res.y, res.z);
        
        Field3D::V3f o = mField->metadata().vecFloatMetadata("Offset", dv);
        mHasOffset = (o != dv);
        mOffset = (mHasOffset ? MPoint(o.x, o.y, o.z) : MPoint(0, 0, 0));
      }
      
      if (forceUpdate ||
          transformMode != mLastTransformMode ||
          forceDimension != mLastForceDimension ||
          (forceDimension && !dimension.isEquivalent(mLastDimension)))
      {
        // refresh dimensions
        
        Field3D::V3f d = mField->metadata().vecFloatMetadata("Dimension", dv);
        mHasDimension = (d != dv);
        mDimension = (mHasDimension ? MPoint(d.x, d.y, d.z) : MPoint(1, 1, 1));
        
        // refresh matrix and decompose
        
        Field3D::MatrixFieldMapping::Ptr mapping = field_dynamic_cast<Field3D::MatrixFieldMapping>(mField->mapping());
        
        if (mapping)
        {
          mMatrix = MMatrix(mapping->localToWorld().x);
        }
        else
        {
          mMatrix.setToIdentity();
        }
        
        if (transformMode == TM_fluid)
        {
          static double sMapTo01[4][4] = {{  1.0 ,  0.0 ,  0.0 , 0.0 } ,
                                          {  0.0 ,  1.0 ,  0.0 , 0.0 } ,
                                          {  0.0 ,  0.0 ,  1.0 , 0.0 } ,
                                          { -0.5 , -0.5 , -0.5 , 1.0 }};
          
          MMatrix mapTo01(sMapTo01);
          MMatrix Mdim;
          MMatrix Moff;
          
          Moff(3, 0) = mOffset.x;
          Moff(3, 1) = mOffset.y;
          Moff(3, 2) = mOffset.z;
          
          Mdim(0, 0) = mDimension.x;
          Mdim(1, 1) = mDimension.y;
          Mdim(2, 2) = mDimension.z;
          
          // mMatrix = mapTo01 * Mdim * Moff * xform
          // xform = Moffset^1 * Mdim^1 * mapTo01^-1 * mMatrix 
          
          mMatrix = Moff.inverse() * Mdim.inverse() * mapTo01.inverse() * mMatrix;
          
          if (forceDimension)
          {
            // dimension will be the value connected to fluid node 'dimensions'
            
            // apply new scale + offset so that fluid position is globally invariant
            
            Mdim(0, 0) = (dimension.x > 0.000001 ? mDimension.x / dimension.x : 1);
            Mdim(1, 1) = (dimension.y > 0.000001 ? mDimension.y / dimension.y : 1);
            Mdim(2, 2) = (dimension.z > 0.000001 ? mDimension.z / dimension.z : 1);
            
            Moff(3, 0) = mOffset.x * (1.0 - Mdim(0, 0));
            Moff(3, 1) = mOffset.y * (1.0 - Mdim(1, 1));
            Moff(3, 2) = mOffset.z * (1.0 - Mdim(2, 2));
            
            mMatrix = Mdim * Moff * mMatrix;
          }
        }
      
        MTransformationMatrix xform = mMatrix;
        
        mTranslate = xform.getTranslation(MSpace::kTransform);
        xform.getRotation(mRotate, mRotateOrder);
        xform.getScale(mScale, MSpace::kTransform);
        xform.getShear(mShear, MSpace::kTransform);
      }
      
      if (forceDimension)
      {
        mHasDimension = true;
        mDimension = dimension;
      }
    }
    else
    {
      // only reset output that depends on mField
      resetOffset();
      resetDimension();
      resetResolution();
      resetTransform();
    }
  }
  else
  {
    // reset everything
    reset();
  }
  
  // Maya doesn't like fluid shapes with zero dimension (crash)
  if (mDimension.x <= 0.0) mDimension.x = 1.0;
  if (mDimension.y <= 0.0) mDimension.y = 1.0;
  if (mDimension.z <= 0.0) mDimension.z = 1.0;
  
  mFirstUpdate = false;
  
  mLastFilename = filename;
  mLastTime = t;
  mLastPartition = partition;
  mLastField = field;
  mLastForceDimension = forceDimension;
  mLastDimension = dimension;
  mLastTransformMode = transformMode;
}

MStatus Field3DInfo::compute(const MPlug &plug, MDataBlock &data)
{
  MDataHandle hFilename = data.inputValue(aFilename);
  MDataHandle hTime = data.inputValue(aTime);
  MDataHandle hPartition = data.inputValue(aPartition);
  MDataHandle hField = data.inputValue(aField);
  MDataHandle hForceDimension = data.inputValue(aForceDimension);
  MDataHandle hDimension = data.inputValue(aDimension);
  MDataHandle hTransformMode = data.inputValue(aTransformMode);
  
  const MString &filename = hFilename.asString();
  MTime t = hTime.asTime();
  const MString &partition = hPartition.asString();
  const MString &field = hField.asString();
  bool forceDimension = hForceDimension.asBool();
  MPoint dimension = hDimension.asVector();
  TransformMode transformMode = (TransformMode) hTransformMode.asShort();
  
  update(filename, t, partition, field, forceDimension, dimension, transformMode);
  
  MObject oAttr = plug.attribute();
  
  if (oAttr == aOutPartitions)
  {
    MArrayDataHandle hOut = data.outputArrayValue(aOutPartitions);
    MString val;
    
    if (plug.isElement())
    {
      size_t idx = (size_t) plug.logicalIndex();
      
      hOut.jumpToElement(idx);
      
      val = (idx < mPartitions.size() ? mPartitions[idx].c_str() : "");
      
      hOut.outputValue().set(val);
    }
    else
    {
      MArrayDataBuilder bld = hOut.builder();
      
      for (size_t i=0; i<mPartitions.size(); ++i)
      {
        MDataHandle hElem = bld.addElement(i);
        val = mPartitions[i].c_str();
        hElem.set(val);
      }
      
      hOut.set(bld);
    }
  }
  else if (oAttr == aOutFields)
  {
    MArrayDataHandle hOut = data.outputArrayValue(aOutFields);
    MString val;
    
    if (plug.isElement())
    {
      size_t idx = (size_t) plug.logicalIndex();
      
      hOut.jumpToElement(idx);
      
      val = (idx < mFields.size() ? mFields[idx].c_str() : "");
      
      hOut.outputValue().set(val);
    }
    else
    {
      MArrayDataBuilder bld = hOut.builder();
      
      for (size_t i=0; i<mFields.size(); ++i)
      {
        MDataHandle hElem = bld.addElement(i);
        val = mFields[i].c_str();
        hElem.set(val);
      }
      
      hOut.set(bld);
    }
  }
  else if (oAttr == aOutResolution)
  {
    MDataHandle hOut = data.outputValue(aOutResolution);
    hOut.set(mResolution.x, mResolution.y, mResolution.z);
  }
  else if (oAttr == aOutResolutionX)
  {
    MDataHandle hOut = data.outputValue(aOutResolutionX);
    hOut.set(mResolution.x);
  }
  else if (oAttr == aOutResolutionY)
  {
    MDataHandle hOut = data.outputValue(aOutResolutionY);
    hOut.set(mResolution.y);
  }
  else if (oAttr == aOutResolutionZ)
  {
    MDataHandle hOut = data.outputValue(aOutResolutionZ);
    hOut.set(mResolution.z);
  }
  else if (oAttr == aOutHasOffset)
  {
    MDataHandle hOut = data.outputValue(aOutHasOffset);
    hOut.set(mHasOffset);
  }
  else if (oAttr == aOutOffset)
  {
    MDataHandle hOut = data.outputValue(aOutOffset);
    hOut.set(mOffset.x, mOffset.y, mOffset.z);
  }
  else if (oAttr == aOutOffsetX)
  {
    MDataHandle hOut = data.outputValue(aOutOffsetX);
    hOut.set(mOffset.x);
  }
  else if (oAttr == aOutOffsetY)
  {
    MDataHandle hOut = data.outputValue(aOutOffsetY);
    hOut.set(mOffset.y);
  }
  else if (oAttr == aOutOffsetZ)
  {
    MDataHandle hOut = data.outputValue(aOutOffsetZ);
    hOut.set(mOffset.z);
  }
  else if (oAttr == aOutHasDimension)
  {
    MDataHandle hOut = data.outputValue(aOutHasDimension);
    hOut.set(mHasDimension);
  }
  else if (oAttr == aOutDimension)
  {
    MDataHandle hOut = data.outputValue(aOutDimension);
    hOut.set(mDimension.x, mDimension.y, mDimension.z);
  }
  else if (oAttr == aOutDimensionX)
  {
    MDataHandle hOut = data.outputValue(aOutDimensionX);
    hOut.set(mDimension.x);
  }
  else if (oAttr == aOutDimensionY)
  {
    MDataHandle hOut = data.outputValue(aOutDimensionY);
    hOut.set(mDimension.y);
  }
  else if (oAttr == aOutDimensionZ)
  {
    MDataHandle hOut = data.outputValue(aOutDimensionZ);
    hOut.set(mDimension.z);
  }
  else if (oAttr == aOutMatrix)
  {
    MDataHandle hOut = data.outputValue(aOutMatrix);
    hOut.set(mMatrix);
  }
  // Translation
  else if (oAttr == aOutTranslate)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(mTranslate.x, mTranslate.y, mTranslate.z);
  }
  else if (oAttr == aOutTranslateX)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(mTranslate.x);
  }
  else if (oAttr == aOutTranslateY)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(mTranslate.y);
  }
  else if (oAttr == aOutTranslateZ)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(mTranslate.z);
  }
  // Rotation
  else if (oAttr == aOutRotate)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(mRotate[0], mRotate[1], mRotate[2]);
  }
  else if (oAttr == aOutRotateX)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(mRotate[0]);
  }
  else if (oAttr == aOutRotateY)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(mRotate[1]);
  }
  else if (oAttr == aOutRotateZ)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(mRotate[2]);
  }
  else if (oAttr == aOutRotateOrder)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.setShort(short(mRotateOrder));
  }
  // Scale
  else if (oAttr == aOutScale)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(mScale[0], mScale[1], mScale[2]);
  }
  else if (oAttr == aOutScaleX)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(mScale[0]);
  }
  else if (oAttr == aOutScaleY)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(mScale[1]);
  }
  else if (oAttr == aOutScaleZ)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(mScale[2]);
  }
  // Shear
  else if (oAttr == aOutShear)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(mShear[0], mShear[1], mShear[2]);
  }
  else if (oAttr == aOutShearXY)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(mShear[0]);
  }
  else if (oAttr == aOutShearXZ)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(mShear[1]);
  }
  else if (oAttr == aOutShearYZ)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(mShear[2]);
  }
  // Other
  else if (oAttr == aOutRotateAxis ||
           oAttr == aOutRotatePivot ||
           oAttr == aOutRotatePivotTranslate ||
           oAttr == aOutScalePivot ||
           oAttr == aOutScalePivotTranslate)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(0.0, 0.0, 0.0);
  }
  else if (oAttr == aOutRotateAxisX ||
           oAttr == aOutRotateAxisY ||
           oAttr == aOutRotateAxisZ ||
           oAttr == aOutRotatePivotX ||
           oAttr == aOutRotatePivotY ||
           oAttr == aOutRotatePivotZ ||
           oAttr == aOutRotatePivotTranslateX ||
           oAttr == aOutRotatePivotTranslateY ||
           oAttr == aOutRotatePivotTranslateZ ||
           oAttr == aOutScalePivotX ||
           oAttr == aOutScalePivotY ||
           oAttr == aOutScalePivotZ ||
           oAttr == aOutScalePivotTranslateX ||
           oAttr == aOutScalePivotTranslateY ||
           oAttr == aOutScalePivotTranslateZ)
  {
    MDataHandle hOut = data.outputValue(oAttr);
    hOut.set(0.0);
  }
  else
  {
    return MS::kUnknownParameter;
  }
  
  data.setClean(plug);
  return MS::kSuccess;
}

