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

MTypeId Field3DVRayMatrix::id(0x5E5FE);

MObject Field3DVRayMatrix::aFilename;
MObject Field3DVRayMatrix::aTime;
MObject Field3DVRayMatrix::aOutScale;
MObject Field3DVRayMatrix::aOutScaleX;
MObject Field3DVRayMatrix::aOutScaleY;
MObject Field3DVRayMatrix::aOutScaleZ;
MObject Field3DVRayMatrix::aOutScalePivot;
MObject Field3DVRayMatrix::aOutScalePivotX;
MObject Field3DVRayMatrix::aOutScalePivotY;
MObject Field3DVRayMatrix::aOutScalePivotZ;
MObject Field3DVRayMatrix::aOutScalePivotTranslate;
MObject Field3DVRayMatrix::aOutScalePivotTranslateX;
MObject Field3DVRayMatrix::aOutScalePivotTranslateY;
MObject Field3DVRayMatrix::aOutScalePivotTranslateZ;
MObject Field3DVRayMatrix::aOutRotate;
MObject Field3DVRayMatrix::aOutRotateX;
MObject Field3DVRayMatrix::aOutRotateY;
MObject Field3DVRayMatrix::aOutRotateZ;
MObject Field3DVRayMatrix::aOutRotatePivot;
MObject Field3DVRayMatrix::aOutRotatePivotX;
MObject Field3DVRayMatrix::aOutRotatePivotY;
MObject Field3DVRayMatrix::aOutRotatePivotZ;
MObject Field3DVRayMatrix::aOutRotatePivotTranslate;
MObject Field3DVRayMatrix::aOutRotatePivotTranslateX;
MObject Field3DVRayMatrix::aOutRotatePivotTranslateY;
MObject Field3DVRayMatrix::aOutRotatePivotTranslateZ;
MObject Field3DVRayMatrix::aOutRotateOrder;
MObject Field3DVRayMatrix::aOutRotateAxis;
MObject Field3DVRayMatrix::aOutRotateAxisX;
MObject Field3DVRayMatrix::aOutRotateAxisY;
MObject Field3DVRayMatrix::aOutRotateAxisZ;
MObject Field3DVRayMatrix::aOutShear;
MObject Field3DVRayMatrix::aOutShearXY;
MObject Field3DVRayMatrix::aOutShearXZ;
MObject Field3DVRayMatrix::aOutShearYZ;
MObject Field3DVRayMatrix::aOutTranslate;
MObject Field3DVRayMatrix::aOutTranslateX;
MObject Field3DVRayMatrix::aOutTranslateY;
MObject Field3DVRayMatrix::aOutTranslateZ;

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

void* Field3DVRayMatrix::creator()
{
  return new Field3DVRayMatrix();
}

MStatus Field3DVRayMatrix::initialize()
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

Field3DVRayMatrix::Field3DVRayMatrix()
   : MPxNode()
   , mBuffer(0)
   , mBufferLength(0)
{
  reset();
}

Field3DVRayMatrix::~Field3DVRayMatrix()
{
  if (mBuffer)
  {
    delete[] mBuffer;
    mBuffer = 0;
  }
}

void Field3DVRayMatrix::reset()
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

void Field3DVRayMatrix::update(const MString &filename, MTime t, double eps)
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

MStatus Field3DVRayMatrix::compute(const MPlug &plug, MDataBlock &data)
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

