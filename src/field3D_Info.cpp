#include "field3D_VRayMatrix.h"
#include "field3D_Tools.h"
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MAngle.h>
#include <cmath>

using namespace Field3D;

MTypeId Field3DInfo::id(0x5E5FE);

MObject Field3DInfo::aFilename;
MObject Field3DInfo::aTime;
MObject Field3DInfo::aPartition;
MObject Field3DInfo::aField;
MObject Field3DInfo::aOverrideOffset;
MObject Field3DInfo::aOffset;
MObject Field3DInfo::aOverrideDimension;
MObject Field3DInfo::aDimension;
MObject Field3DInfo::aTransformMode;

MObject Field3DInfo::aOutPartitions;
MObject Field3DInfo::aOutFields;
MObject Field3DInfo::aOutResolution;
MObject Field3DInfo::aOutHasDimension;
MObject Field3DInfo::aOutDimension;
MObject Field3DInfo::aOutHasOffset;
MObject Field3DInfo::aOutOffset;
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

#define ADD_XYZ_ATTR(attr, longName, shortName)\
  attr = cattr.create(longName, shortName);\
  attr ## X = nattr.create(longName "X", shortName "x", MFnNumericData::kDouble, 0.0);\
  nattr.setStorable(false);\
  nattr.setReadable(true);\
  nattr.setWritable(false);\
  attr ## Y = nattr.create(longName "Y", shortName "y", MFnNumericData::kDouble, 0.0);\
  nattr.setStorable(false);\
  nattr.setReadable(true);\
  nattr.setWritable(false);\
  attr ## Z = nattr.create(longName "Z", shortName "z", MFnNumericData::kDouble, 0.0);\
  nattr.setStorable(false);\
  nattr.setReadable(true);\
  nattr.setWritable(false);\
  cattr.addChild(attr ## X);\
  cattr.addChild(attr ## Y);\
  cattr.addChild(attr ## Z);\
  addAttribute(attr)

#define AFFECTS_OUTPUTS(attr)\
  attributeAffects(attr, aOutScale);\
  attributeAffects(attr, aOutScalePivot);\
  attributeAffects(attr, aOutScalePivotTranslate);\
  attributeAffects(attr, aOutRotate);\
  attributeAffects(attr, aOutRotatePivot);\
  attributeAffects(attr, aOutRotatePivotTranslate);\
  attributeAffects(attr, aOutRotateOrder);\
  attributeAffects(attr, aOutRotateAxis);\
  attributeAffects(attr, aOutShear);\
  attributeAffects(attr, aOutTranslate)

void* Field3DInfo::creator()
{
  return new Field3DInfo();
}

MStatus Field3DInfo::initialize()
{
  MFnCompoundAttribute cattr;
  MFnNumericAttribute nattr;
  MFnUnitAttribute uattr;
  MFnTypedAttribute tattr;
  MFnEnumAttribute eattr;
  
  aFilename = tattr.create("filename", "fn", MFnData::kString);
  tattr.setStorable(true);
  tattr.setReadable(true);
  tattr.setWritable(true);
  tattr.setKeyable(false);
  addAttribute(aFilename);
  
  aTime = uattr.create("time", "tm", MFnUnitAttribute::kTime);
  uattr.setStorable(true);
  uattr.setWritable(true);
  uattr.setReadable(true);
  addAttribute(aTime);
  
  aPartition = tattr.create("partition", "prt", MFnData::kString);
  tattr.setStorable(true);
  tattr.setReadable(true);
  tattr.setWritable(true);
  tattr.setKeyable(false);
  addAttribute(aPartition);
  
  aField = tattr.create("field", "fld", MFnData::kString);
  tattr.setStorable(true);
  tattr.setReadable(true);
  tattr.setWritable(true);
  tattr.setKeyable(false);
  addAttribute(aField);
  
  aOverrideOffset = nattr.create("overrideOffset", "oof", MFnNumericData::kBoolean, 0);
  nattr.setStorable(true);
  nattr.setReadable(true);
  nattr.setWritable(true);
  nattr.setKeyable(false);
  addAttribute(aOverrideDimension);
  
  aOffset = nattr.createPoint("offset", "off");
  nattr.setDefault(0.0, 0.0, 0.0);
  nattr.setStorable(true);
  nattr.setReadable(true);
  nattr.setWritable(true);
  nattr.setKeyable(true);
  addAttribute(aOffset);
  
  aOverrideDimension = nattr.create("overrideDimension", "odi", MFnNumericData::kBoolean, 0);
  nattr.setStorable(true);
  nattr.setReadable(true);
  nattr.setWritable(true);
  nattr.setKeyable(false);
  addAttribute(aOverrideDimension);
  
  aDimension = nattr.createPoint("dimension", "dim");
  nattr.setDefault(1.0, 1.0, 1.0);
  nattr.setStorable(true);
  nattr.setReadable(true);
  nattr.setWritable(true);
  nattr.setKeyable(true);
  addAttribute(aDimension);
  
  aMode = eattr.create("transformMode", "trm");
  eattr.setStorable(true);
  eattr.setReadable(true);
  eattr.setWritable(true);
  eattr.setKeyable(false);
  eattr.addField("full", 0);
  eattr.addField("without_dimension", 1);
  eattr.addField("without_offset", 2);
  eattr.addField("without_dimension_and_offset", 3);
  addAttribute(aMode);
  
  
  aOutPartitions = tattr.create("outPartitions", "opts", MFnData::kString);
  tattr.setArray(true);
  tattr.setStorable(false);
  tattr.setReadable(true);
  tattr.setWritable(false);
  addAttribute(aOutPartitions);
  
  aOutFields = tattr.create("outFields", "ofls", MFnData::kString);
  tattr.setArray(true);
  tattr.setStorable(false);
  tattr.setReadable(true);
  tattr.setWritable(false);
  addAttribute(aOutFields);
  
  aOutResolution = nattr.createPoint("outResolution", "ores");
  nattr.setStorable(false);
  nattr.setReadable(true);
  nattr.setWritable(false);
  addAttribute(aOutResolution);
  
  aOutHasDimension = nattr.create("outHasDimension", "ohd", MFnNumericData::kBoolean, 0);
  nattr.setStorable(false);
  nattr.setReadable(true);
  nattr.setWritable(false);
  addAttribute(aOutHasDimension);
  
  aOutDimension =  nattr.createPoint("outDimension", "odim");
  nattr.setStorable(false);
  nattr.setReadable(true);
  nattr.setWritable(false);
  addAttribute(aOutDimension);
  
  aOutHasOffset = nattr.create("outHasOffset", "oho", MFnNumericData::kBoolean, 0);
  nattr.setStorable(false);
  nattr.setReadable(true);
  nattr.setWritable(false);
  addAttribute(aOutHasOffset);
  
  aOutOffset = nattr.createPoint("outOffset", "ooff");
  nattr.setStorable(false);
  nattr.setReadable(true);
  nattr.setWritable(false);
  addAttribute(aOutOffset);
  
  // aOutMatrix = ;
  
  aOutRotateOrder = eattr.create("outRotateOrder", "oro");
  eattr.addField("xyz", 0);
  eattr.addField("yzx", 1);
  eattr.addField("zxy", 2);
  eattr.addField("xzy", 3);
  eattr.addField("yxz", 4);
  eattr.addField("zyx", 5);
  eattr.setStorable(false);
  eattr.setReadable(true);
  eattr.setWritable(false);
  addAttribute(aOutRotateOrder);
  
  aOutShear = cattr.create("outShear", "osh");
  aOutShearXY = nattr.create("outShearXY", "oshx", MFnNumericData::kDouble, 0.0);
  nattr.setStorable(false);
  nattr.setReadable(true);
  nattr.setWritable(false);
  aOutShearXZ = nattr.create("outShearXZ", "oshy", MFnNumericData::kDouble, 0.0);
  nattr.setStorable(false);
  nattr.setReadable(true);
  nattr.setWritable(false);
  aOutShearYZ = nattr.create("outShearYZ", "oshz", MFnNumericData::kDouble, 0.0);
  nattr.setStorable(false);
  nattr.setReadable(true);
  nattr.setWritable(false);
  cattr.addChild(aOutShearXY);
  cattr.addChild(aOutShearXZ);
  cattr.addChild(aOutShearYZ);
  addAttribute(aOutShear);
  
  ADD_XYZ_ATTR(aOutScale, "outScale", "os");
  ADD_XYZ_ATTR(aOutScalePivot, "outScalePivot", "osp");
  ADD_XYZ_ATTR(aOutScalePivotTranslate, "outScalePivotTranslate", "ospt");
  ADD_XYZ_ATTR(aOutRotate, "outRotate", "or");
  ADD_XYZ_ATTR(aOutRotatePivot, "outRotatePivot", "orp");
  ADD_XYZ_ATTR(aOutRotatePivotTranslate, "outRotatePivotTranslate", "orpt");
  ADD_XYZ_ATTR(aOutRotateAxis, "outRotateAxis", "ora");
  ADD_XYZ_ATTR(aOutTranslate, "outTranslate", "ot");
  
  AFFECTS_OUTPUTS(aFilename);
  AFFECTS_OUTPUTS(aTime);
  
  return MStatus::kSuccess;
}

Field3DInfo::Field3DInfo()
   : MPxNode()
   , mBuffer(0)
   , mBufferLength(0)
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
}

void Field3DInfo::reset()
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
}

void Field3DInfo::update(const MString &filename, MTime t, double eps)
{
  if (filename != mLastFilename ||
      fabs(mLastTime.as(MTime::uiUnit()) - t.as(MTime::uiUnit())) > eps)
  {
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
      
      MGlobal::displayInfo(MString("Use frame pattern: ") + mFramePattern.c_str());
    }
    
    if (mBuffer)
    {
      sprintf(mBuffer, mFramePattern.c_str(), fullframe);
      
      Field3D::Field3DInputFile *f3dIn = new Field3D::Field3DInputFile();
      
      if (!f3dIn->open(mBuffer))
      {
        //MGlobal::displayWarning(MString("Invalid f3d file \"") + mBuffer + "\"");
        reset();
      }
      else
      {
        MTransformationMatrix zupM, resM, finalM;
        
        // Yup -> Zup convert matrix
        double rX[3] = {MAngle(-90.0, MAngle::kDegrees).asRadians(), 0.0, 0.0};
        zupM.setRotation(rX, MTransformationMatrix::kXYZ);  
        
        // Resolution offset matrix
        unsigned int res[3] = {0, 0, 0};
        Field3DTools::getFieldsResolution(f3dIn, res);
        MVector tR(0.5 * res[0], 0.5 * res[1], 0.5 * res[2]);
        resM.setTranslation(tR, MSpace::kTransform);
        
        finalM = (resM.asMatrix() * zupM.asMatrix()).inverse();
        
        mTranslate = finalM.getTranslation(MSpace::kTransform);
        finalM.getRotation(mRotate, mRotateOrder);
        finalM.getScale(mScale, MSpace::kTransform);
        finalM.getShear(mShear, MSpace::kTransform);
      }
      
      delete f3dIn;
    }
    else
    {
      reset();
    }
    
    mLastFilename = filename;
    mLastTime = t;
  }
}

MStatus Field3DInfo::compute(const MPlug &plug, MDataBlock &data)
{
  MDataHandle hFilename = data.inputValue(aFilename);
  MDataHandle hTime = data.inputValue(aTime);
  
  const MString &filename = hFilename.asString();
  MTime t = hTime.asTime();
  
  update(filename, t);
  
  MObject oAttr = plug.attribute();
  
  // Translation
  if (oAttr == aOutTranslate)
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

