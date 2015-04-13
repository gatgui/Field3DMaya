// Copyright (c) 2011 Prime Focus Film.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the
// distribution. Neither the name of Prime Focus Film nor the
// names of its contributors may be used to endorse or promote
// products derived from this software without specific prior written
// permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.


#include "field3D_Format.h"
#include "maya_Tools.h"
#include "tinyLogger.h"

#include <maya/MArgList.h>
#include <maya/MStatus.h>
#include <maya/MDagPath.h>
#include <maya/MItDag.h>
#include <maya/MFnFluid.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MMatrix.h>
#include <maya/MGlobal.h>

#include <iostream>
#include <sstream>

static std::string extractFluidName(const MString &name)
{
  std::string nameStr = name.asChar();
  size_t pos = nameStr.rfind('_');
  return nameStr.substr(0, pos);
}

static std::string partitionName(const std::string &fluidName)
{
  size_t p;
  
  std::string partition = fluidName;
  
  // strip parent names
  p = partition.rfind('|');
  if (p != std::string::npos)
  {
    partition = partition.substr(p + 1);
  }
  
  // stip namespaces
  p = partition.rfind(':');
  if (p != std::string::npos)
  {
    partition = partition.substr(p + 1);
  }
  
  return partition;
}

static std::string extractChannelName(const MString &name)
{
  std::string nameStr = name.asChar();
  size_t pos = nameStr.rfind('_');
  return nameStr.substr(pos+1);
}

template <typename T>
static std::string display3(T tab[3])
{
  std::stringstream sres;
  sres << tab[0] << " " << tab[1] << " " << tab[2];
  return sres.str();
}

template <typename T>
static std::string display4x4(T mat[4][4])
{
  std::ostringstream oss;
  
  oss << "{{" << mat[0][0] << ", " << mat[1][0] << ", " << mat[2][0] << ", " << mat[3][0] << "}, ";
  oss <<  "{" << mat[0][1] << ", " << mat[1][1] << ", " << mat[2][1] << ", " << mat[3][1] << "}, ";
  oss <<  "{" << mat[0][2] << ", " << mat[1][2] << ", " << mat[2][2] << ", " << mat[3][2] << "}, ";
  oss <<  "{" << mat[0][3] << ", " << mat[1][3] << ", " << mat[2][3] << ", " << mat[3][3] << "}}";
  
  return oss.str();
}

// ------------------------------------------- CONSTRUCTOR - DESTRUCTOR

Field3dCacheFormat::Field3dCacheFormat(Field3DTools::FieldTypeEnum type,
                                       Field3DTools::FieldDataTypeEnum data_type)
  : MPxCacheFormat()
  , m_fieldType(type)
  , m_dataType(data_type)
  , m_mode((FileAccessMode)-1)
  , m_inFile(0)
  , m_outFile(0)
{
  Field3D::initIO();

  /*
  m_inFile = 0;
  m_outFile = 0;
  m_isFileOpened = false;
  m_readNameStack = true;
  m_offset[0] = 0.0;
  m_offset[1] = 0.0;
  m_offset[2] = 0.0;
  */
  
}

Field3dCacheFormat::~Field3dCacheFormat()
{
  m_cacheFiles.clear();
  m_remapChannels.clear();
  
  if (m_inFile)
  {
    delete m_inFile;
  }
  
  if (m_outFile)
  {
    delete m_outFile;
  }
}

bool Field3dCacheFormat::identifyPath(const MString &path, MString &dirname, MString &basename, MString &frame, MTime &t, MString &ext)
{
  int idx, idx0, idx1;
  
  #ifdef _WIN32
  idx0 = path.rindexW('\\');
  idx1 = path.rindexW('/');
  idx = (idx0 > idx1 ? idx0 : idx1);
  #else
  idx = path.rindexW('/');
  #endif
    
  if (idx == -1)
  {
      dirname = "";
      basename = path;
  }
  else
  {
      dirname = path.substringW(0, idx-1);
      basename = path.substringW(idx+1, path.length());
  }
  
  idx = basename.rindexW('.');
  if (idx == -1)
  {
      ext = "";
  }
  else
  {
      ext = basename.substringW(idx+1, basename.length());
      basename = basename.substringW(0, idx-1);
  }
  
  if (ext != "f3d")
  {
    return false;
  }
  
  bool timeFound = false;
  int fval = 0;
  double tval = 0.0;
  
  // search for .frame.subfrane pattern
  
  idx0 = basename.rindexW(".");
  
  if (idx0 != -1)
  {
    MString tmp0 = basename.substringW(idx0+1, basename.length());
    
    if (sscanf(tmp0.asChar(), "%d", &fval) == 1)
    {
      basename = basename.substringW(0, idx0-1);
      
      idx1 = basename.rindexW(".");
      
      if (idx1 != -1)
      {
        MString tmp1 = basename.substringW(idx1+1, basename.length());
        
        if (sscanf(tmp1.asChar(), "%d", &fval) == 1)
        {
          // basename.<frame>.<subframe>.f3d
          basename = basename.substringW(0, idx1-1);
          
          tval = 0.0;
          if (sscanf((tmp1 + "." + tmp0).asChar(), "%lf", &tval) == 1)
          {
            t.setValue(tval);
            frame = tmp1 + "." + tmp0;
            timeFound = true;
          }
        }
        else
        {
          t.setValue(double(fval));
          frame = tmp0;
          timeFound = true;
        }
      }
      else
      {
        t.setValue(double(fval));
        frame = tmp0;
        timeFound = true;
      }
    }
  }
  
  if (!timeFound)
  {
    // search for FrameXXTickYY pattern
    idx0 = basename.indexW("Frame");
    
    if (idx0 != -1)
    {
      MString tmp = basename.substringW(idx0, basename.length());
      
      idx1 = tmp.indexW("Tick");
      
      if (idx1 != -1)
      {
        int ticks = 0;
        
        if (sscanf(tmp.asChar(), "Frame%dTick%d", &fval, &ticks) == 2)
        {
          basename = basename.substringW(0, idx0-1);
          t.setValue(double(fval) + ticks * MTime(1.0, MTime::k6000FPS).asUnits(MTime::uiUnit()));
          frame = tmp;
          timeFound = true;
        }
      }
      else
      {
        if (sscanf(tmp.asChar(), "Frame%d", &fval) == 1)
        {
          basename = basename.substringW(0, idx0-1);
          t.setValue(double(fval));
          frame = tmp;
          timeFound = true;
        }
      }
    }
  }
  
  if (timeFound)
  {
    MGlobal::displayInfo("Field3dCacheFormat::identifyPath: \"" + path + "\" -> dirname=" + dirname + ", basename=" + basename + ", frame=" + frame + ", ext=" + ext + ", time=" + t.value());
  }
  
  return timeFound;
}

unsigned long Field3dCacheFormat::fillCacheFiles(const MString &path)
{
  MString dirname, basename, frame, ext;
  MTime t;
  
  identifyPath(path, dirname, basename, frame, t, ext);
  return fillCacheFiles(dirname, basename, ext);
}

unsigned long Field3dCacheFormat::fillCacheFiles(const MString &dirname, const MString &basename, const MString &ext)
{
  MStringArray tmp;

  std::string dn = dirname.asChar();
  if (dn.length() > 0)
  {
    char lc = dn[dn.length()-1];
    
    #ifdef _WIN32
    if (lc != '\\' && lc != '/')
    #else
    if (lc != '/')
    #endif
    {
      dn.push_back('/');
    }
  }
  else
  {
    dn = "./";
  }
  
  MString mdn = dn.c_str();
  
  MGlobal::executeCommand("getFileList -folder \"" + mdn + "\" -filespec \"" + basename + "*." + ext + "\"", tmp);
  
  m_cacheFiles.clear();
  
  MString _dn, _bn, _fn, _e;
  MTime t;
  
  for (unsigned long i=0; i<tmp.length(); ++i)
  {
    if (identifyPath(tmp[i], _dn, _bn, _fn, t, _e))
    {
        m_cacheFiles[t].path = mdn + tmp[i];
    }
  }
      
  return (unsigned long) m_cacheFiles.size();
}

MStatus Field3dCacheFormat::open(const MString &fileName, FileAccessMode mode)
{
  MGlobal::displayInfo("f3dCache::open \"" + fileName + "\"");
  
  FileAccessMode curMode = m_mode;
  
  m_mode = mode;
  
  if (mode == kRead || mode == kReadWrite)
  {
    MGlobal::displayInfo("  For reading");
    
    MString dn, bn, frm, ext;
    MTime t;
    
    if (!identifyPath(fileName, dn, bn, frm, t, ext))
    {
      return MS::kFailure;
    }
    
    std::string inFilename = std::string(dn.asChar()) + "/" + bn.asChar();
    
    if (curMode == (FileAccessMode)-1 || inFilename != m_inFilename)
    {
      fillCacheFiles(fileName);
      m_curCacheFile = m_cacheFiles.end();
      // ??? Want to read that from .xml <extra> tags
      m_remapChannels.clear();
      
      m_inFilename = inFilename;
      
      m_inPartition = "";
      m_inChannel = "";
    }
    
    std::map<MTime, CacheEntry>::iterator it = m_cacheFiles.find(t);
    
    if (it == m_cacheFiles.end())
    {
      m_curCacheFile = it;
      
      return MS::kFailure;
    }
    else if (it != m_curCacheFile || !m_inFile)
    {
      if (m_inFile)
      {
        delete m_inFile;
        
        it->second.fields.clear();
        
        m_inPartition = "";
        m_inChannel = "";
      }
      
      m_inFile = new Field3DInputFile();
    
      // open the file
      if (!m_inFile->open(it->second.path.asChar()))
      {
        ERROR(std::string("Opening of") +  fileName.asChar() + "failed : Unknown reason");
        delete m_inFile;
        m_inFile = 0;
        
        return MS::kFailure;
      }
      
      m_curCacheFile = it;
      
      m_curField = it->second.fields.end();
      
      // const Field3D::V3f er(-999.999,-999.999,-999.999);
      // Field3D::V3f off = m_inFile->metadata().vecFloatMetadata("Offset", er);
    }
  }
  
  if (mode == kWrite || mode == kReadWrite)
  {
    MGlobal::displayInfo("  For writing");
    
    if (m_outFile)
    {
      delete m_outFile;
    }
    
    m_outFile = new Field3DOutputFile();
     
    // create the file
    if (!m_outFile->create(fileName.asChar(), Field3DOutputFile::OverwriteMode))
    {
      ERROR(std::string("Creation of ") + fileName.asChar() + "failed : Unknown reason");
      delete m_outFile;
      m_outFile = 0;
      
      return MS::kFailure;
    }
    
    m_outFilename = fileName.asChar();
  }
  
  m_mode = mode;
  
  return MS::kSuccess;
}

void Field3dCacheFormat::close()
{
  MGlobal::displayInfo("f3dCache::close");
  // don't close file: this gets called several times for the same frame
  
  if (m_outFile)
  {
    delete m_outFile;
    m_outFile = 0;
  }
}

MStatus Field3dCacheFormat::isValid()
{
  MGlobal::displayInfo("f3dCache::isValid");
  
  if (m_mode == kRead || m_mode == kReadWrite)
  {
    //if (m_cacheFiles.size() > 0 && m_curCacheFile == m_cacheFiles.end())
    if (!m_inFile)
    {
      return MS::kFailure;
    }
  }
  
  if (m_mode == kWrite || m_mode == kReadWrite)
  {
    if (!m_outFile)
    {
      return MS::kFailure;
    }
  }
  
  return MS::kSuccess;
}

MStatus Field3dCacheFormat::rewind()
{
  MGlobal::displayInfo("f3dCache::rewind");
  return MS::kSuccess;
}

MString Field3dCacheFormat::extension()
{
  return "f3d";
}



MStatus Field3dCacheFormat::writeHeader(const MString &, MTime &, MTime &)
{
  // Write any global metadata here
  return MS::kSuccess;
}

void Field3dCacheFormat::beginWriteChunk()
{
  // Noop
}

MStatus Field3dCacheFormat::writeTime(MTime &time)
{
  // Note: doesn't seem to get called at all
  std::ostringstream oss;
  oss << time.as(MTime::uiUnit());
  
  MGlobal::displayInfo(MString("f3dCache::writeTime ") + oss.str().c_str());
  return MS::kFailure;
}

MStatus Field3dCacheFormat::writeChannelName(const MString &name)
{
  if (m_outFile)
  {
    std::string fluidName = extractFluidName(name);
    
    m_outChannel = extractChannelName(name);
    m_outPartition = partitionName(fluidName);
    
    if (m_outPartition.empty() || m_outChannel.empty())
    {
      return MS::kFailure;
    }
    
    CHECK_MSTATUS_AND_RETURN_IT(MayaTools::getFluidNode(fluidName, m_outFluid));
    
    // Get dynamic offset == {0.0, 0.0, 0.0} if auto-resize is off
    m_outOffset[0] = 0.0f;
    m_outOffset[1] = 0.0f;
    m_outOffset[2] = 0.0f;
    
    MayaTools::getNodeValue(m_outFluid, "dynamicOffsetX", m_outOffset[0]);
    MayaTools::getNodeValue(m_outFluid, "dynamicOffsetY", m_outOffset[1]);
    MayaTools::getNodeValue(m_outFluid, "dynamicOffsetZ", m_outOffset[2]);
    
    return MS::kSuccess;
  }
  else
  {
    return MS::kFailure;
  }
}

MStatus Field3dCacheFormat::writeDoubleArray(const MDoubleArray &array)
{
  return writeArray(array);
}

MStatus Field3dCacheFormat::writeFloatArray(const MFloatArray &array)
{
  return writeArray(array);
}

MStatus Field3dCacheFormat::writeIntArray(const MIntArray &)
{
  MGlobal::displayInfo("f3dCache::writeIntArray");
  return MS::kFailure;
}

MStatus Field3dCacheFormat::writeDoubleVectorArray(const MVectorArray &)
{
  MGlobal::displayInfo("f3dCache::writeDoubleVectorArray");
  return MS::kFailure;
}

MStatus Field3dCacheFormat::writeFloatVectorArray(const MFloatVectorArray &)
{
  MGlobal::displayInfo("f3dCache::writeFloatVectorArray");
  return MS::kFailure;
}

MStatus Field3dCacheFormat::writeInt32(int value)
{
  std::ostringstream oss;
  oss << value;
  
  MGlobal::displayInfo(MString("f3dCache::writeInt32 ") + oss.str().c_str());
  return MS::kFailure;
}

struct OffsetAndDimension
{
  Field3D::V3f off;
  Field3D::V3f dim;
};

static void WriteOffsetAndDimension(Field3D::FieldRes::Ptr field, void *userData)
{
  if (field && userData)
  {
    OffsetAndDimension *offAndDim = (OffsetAndDimension*) userData;
    
    field->metadata().setVecFloatMetadata("Offset", offAndDim->off); 
    field->metadata().setVecFloatMetadata("Dimension", offAndDim->dim);
  }
}

template <class T>
MStatus Field3dCacheFormat::writeArray(T &array)
{
  // _ Resolution is implicitly present in field3d via FieldRes::dataResolution()
  //   so we don't need to store it in a specific extra location.
  // _ Offset is stored in a global metadata while invoking writeHeader()
  //   see this function for more explanations
  
  if (m_outChannel == "resolution" || m_outChannel == "offset" )
  {
    return MS::kSuccess;
  }
  
  double transform[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f},
                            {1.0f, 1.0f, 0.0f, 0.0f},
                            {1.0f, 0.0f, 1.0f, 0.0f},
                            {1.0f, 0.0f, 0.0f, 1.0f}};
  unsigned int resolution[3] = {0, 0, 0};
  double dimension[3] = {0.0, 0.0, 0.0};
  
  // Get transform matrix
  MMatrix parentTransf = m_outFluid.dagPath().inclusiveMatrix();
  
  // Get resolution
  m_outFluid.getResolution(resolution[0], resolution[1], resolution[2]);

  // Get dimension
  m_outFluid.getDimensions(dimension[0], dimension[1], dimension[2]);

  // Move the center to [0, 1]
  double mapTo01[4][4] = {
      { 1.0  , 0.0  , 0.0  , 0.0 } ,
      { 0.0  , 1.0  , 0.0  , 0.0 } ,
      { 0.0  , 0.0  , 1.0  , 0.0 } ,
      { -0.5 , -0.5 , -0.5 , 1.0 }
  };
  MMatrix mapTo01Transf = MMatrix(mapTo01);

  // Auto-resize's offset
  double autoResize[4][4]= {
      { dimension[0]  , 0.0           , 0.0           , 0.0 } ,
      { 0.0           , dimension[1]  , 0.0           , 0.0 } ,
      { 0.0           , 0.0           , dimension[2]  , 0.0 } ,
      { m_outOffset[0], m_outOffset[1], m_outOffset[2], 1.0 }
  };
  MMatrix autoResizeTransf = MMatrix(autoResize);

  // Final transformation matrix
  MMatrix resTransf = mapTo01Transf * autoResizeTransf * parentTransf;
  resTransf.get(transform);
  
  bool (*writeField)(Field3D::Field3DOutputFile *out,
                     const std::string &fluidName,
                     const std::string &fieldName,
                     unsigned int res[3],
                     double transform[4][4],
                     const T &data,
                     Field3DTools::writeMetadataFunc writeMetadata,
                     void *writeMetadataUser);
  
  // Test the type of array
  bool isVectorField = (m_outChannel == "velocity" ||
                        m_outChannel == "color" ||
                        m_outChannel == "texture");
  
  if (!isVectorField)
  {
    // select the propers function
    if (m_dataType == Field3DTools::HALF)
    {
      if (m_fieldType == Field3DTools::SPARSE)
      {
        writeField = Field3DTools::writeSparseScalarField<Field3D::half, T> ;
      }
      else
      {
        writeField = Field3DTools::writeDenseScalarField<Field3D::half, T> ;
      }
    }
    else if (m_dataType == Field3DTools::FLOAT)
    {
      if (m_fieldType == Field3DTools::SPARSE)
      {
        writeField = Field3DTools::writeSparseScalarField<float, T>;
      }
      else
      {
        writeField = Field3DTools::writeDenseScalarField<float, T>;
      }
    }
    else if (m_dataType == Field3DTools::DOUBLE)
    {
      if (m_fieldType == Field3DTools::SPARSE)
      {
        writeField = Field3DTools::writeSparseScalarField<double, T>;
      }
      else
      {
        writeField = Field3DTools::writeDenseScalarField<double, T>;
      }
    }
    else
    {
      ERROR( "Writing of " + m_outChannel + " file failed : Unknown Types");
      return MS::kFailure;
    }
  }
  else
  {
    bool isVel = (m_outChannel == "velocity");
    
    if (m_dataType == Field3DTools::HALF)
    {
      if (isVel)
      {
        writeField = Field3DTools::writeMACVectorField<Field3D::half, T> ;
      }
      else if (m_fieldType == Field3DTools::SPARSE)
      {
        writeField = Field3DTools::writeSparseVectorField<Field3D::half, T> ;
      }
      else
      {
        writeField = Field3DTools::writeDenseVectorField<Field3D::half, T> ;
      }
    }
    else if (m_dataType == Field3DTools::FLOAT)
    {
      if (isVel)
      {
        writeField = Field3DTools::writeMACVectorField<float, T> ;
      }
      else if (m_fieldType == Field3DTools::SPARSE)
      {
        writeField = Field3DTools::writeSparseVectorField<float, T> ;
      }
      else
      {
        writeField = Field3DTools::writeDenseVectorField<float, T> ;
      }
    }
    else if (m_dataType == Field3DTools::DOUBLE)
    {
      if (isVel)
      {
        writeField = Field3DTools::writeMACVectorField<double, T> ;
      }
      else if (m_fieldType == Field3DTools::SPARSE)
      {
        writeField = Field3DTools::writeSparseVectorField<double, T> ;
      }
      else
      {
        writeField = Field3DTools::writeDenseVectorField<double, T> ;
      }
    }
    else
    {
      ERROR( "Writing of " + m_outChannel + " file failed : Unknown Types");
      return MS::kFailure;
    }
  }
  
  // field metadata
  OffsetAndDimension offAndDim;
  
  offAndDim.off = Field3D::V3f(m_outOffset[0], m_outOffset[1], m_outOffset[2]);
  offAndDim.dim = Field3D::V3f(dimension[0], dimension[1], dimension[2]);
  
  // write this field
  bool res = writeField(m_outFile,
                        m_outPartition,
                        m_outChannel,
                        resolution,
                        transform,
                        array,
                        WriteOffsetAndDimension,
                        &offAndDim);
  
  if (!res)
  {
    ERROR( "Writing of " + m_outChannel + " file failed : Unknown reason ( see above for an explanation ? )");
    return MS::kFailure;
  }
  
  return MS::kSuccess;
}

void Field3dCacheFormat::endWriteChunk()
{
  // Noop
}



MStatus Field3dCacheFormat::readHeader()
{
  MGlobal::displayInfo("f3dCache::readHeader");
  return MS::kFailure;
}

MStatus Field3dCacheFormat::beginReadChunk()
{
  MGlobal::displayInfo("f3dCache::beginReadChunk");
  // Check for valid pointer?
  // Read current file fields
  
  return MS::kSuccess;
}


MStatus Field3dCacheFormat::findChannelName(const MString &name)
{
  MGlobal::displayInfo("f3dCache::findChannelName \"" + name + "\"");
  
  if (m_curCacheFile == m_cacheFiles.end())
  {
    return MS::kFailure;
  }
  
  // split into partition name
  // call in turn for each fields
  
  std::string fluidName = extractFluidName(name);
  
  std::string channel = extractChannelName(name);
  std::string partition = partitionName(fluidName);
  
  CacheEntry &cacheFile = m_curCacheFile->second;
  
  // check if partition has changed
  if (m_inPartition != partition)
  {
    // re-read partition fields
    cacheFile.fields.clear();
    
    cacheFile.resolution[0] = 0.0f;
    cacheFile.resolution[1] = 0.0f;
    cacheFile.resolution[2] = 0.0f;
    
    cacheFile.offset[0] = 0.0f;
    cacheFile.offset[1] = 0.0f;
    cacheFile.offset[2] = 0.0f;
    
    // read all fields
    std::vector<std::string> fields;
    
    Field3DTools::getFieldNames(m_inFile, partition, fields);
    for (size_t i=0; i<fields.size(); ++i)
    {
      Field3DTools::Fld field;
      
      if (Field3DTools::getFieldValueType(m_inFile, partition, fields[i], field))
      {
        cacheFile.fields[fields[i]] = field;
        
        // supposes all field in a given partition have the same resolution, offset and dimension
        
        Field3D::V3i res = field.baseField->dataResolution();
        
        if (res.x > cacheFile.resolution.x)
        {
          cacheFile.resolution.x = res.x;
        }
        if (res.y > cacheFile.resolution.y)
        {
          cacheFile.resolution.y = res.y;
        }
        if (res.z > cacheFile.resolution.z)
        {
          cacheFile.resolution.z = res.z;
        }
        
        cacheFile.offset += field.baseField->metadata().vecFloatMetadata("Offset", Field3D::V3f(0.0f, 0.0f, 0.0f));
        
        cacheFile.dimension += field.baseField->metadata().vecFloatMetadata("Dimension", Field3D::V3f(1.0f, 1.0f, 1.0f));
      }
    }
    
    if (cacheFile.fields.size() > 0)
    {
      float scl = 1.0f / float(cacheFile.fields.size());
      
      cacheFile.offset *= scl;
      cacheFile.dimension *= scl;
    }
    
    m_curField = cacheFile.fields.end();
  }
  
  if (cacheFile.fields.size() == 0)
  {
    return MS::kFailure;
  }
  
  // remap channel name
  std::map<std::string, std::string>::iterator rit = m_remapChannels.find(channel);
  if (rit != m_remapChannels.end())
  {
    channel = rit->second;
  }
  
  m_inPartition = partition;
  m_inChannel = channel;
  
  if (channel == "resolution" || channel == "offset")
  {
    return MS::kSuccess;
  }
  
  m_curField = cacheFile.fields.find(channel);
  
  return (m_curField != cacheFile.fields.end() ? MS::kSuccess : MS::kFailure);
}

MStatus Field3dCacheFormat::readChannelName(MString &name)
{
  MGlobal::displayInfo("f3dCache::readChannelName");
  return MS::kFailure;
}

void Field3dCacheFormat::endReadChunk()
{
  MGlobal::displayInfo("f3dCache::endReadChunk");
}

MStatus Field3dCacheFormat::readTime(MTime &time)
{
  MGlobal::displayInfo("f3dCache::readTime");
  return MS::kFailure;
}

MStatus Field3dCacheFormat::findTime(MTime &time, MTime &foundTime)
{
  std::ostringstream oss;
  oss << time.as(MTime::uiUnit());
  
  MGlobal::displayInfo(MString("f3dCache::findTime ") + oss.str().c_str());
  return MS::kFailure;
}

MStatus Field3dCacheFormat::readNextTime(MTime &foundTime)
{
  MGlobal::displayInfo("f3dCache::readNextTime");
  return MS::kFailure;
}

unsigned Field3dCacheFormat::readArraySize()
{
  MGlobal::displayInfo("f3dCache::readArraySize");
  
  if (m_curCacheFile != m_cacheFiles.end())
  {
    if (m_inChannel == "resolution" || m_inChannel == "offset")
    {
      return 3;
    }
    else if (m_curField != m_curCacheFile->second.fields.end())
    {
      // return field resolution
      Field3DTools::Fld &fld = m_curField->second;
      
      Field3D::V3i res = fld.baseField->dataResolution();
      
      switch (fld.fieldType)
      {
      case Field3DTools::DenseScalarField_Half:
      case Field3DTools::DenseScalarField_Float:
      case Field3DTools::DenseScalarField_Double:
      case Field3DTools::SparseScalarField_Half:
      case Field3DTools::SparseScalarField_Float:
      case Field3DTools::SparseScalarField_Double:
        return (res.x * res.y * res.z);
      case Field3DTools::DenseVectorField_Half:
      case Field3DTools::DenseVectorField_Float:
      case Field3DTools::DenseVectorField_Double:
      case Field3DTools::SparseVectorField_Half:
      case Field3DTools::SparseVectorField_Float:
      case Field3DTools::SparseVectorField_Double:
        return 3 * (res.x * res.y * res.z);
      case Field3DTools::MACField_Half:
      case Field3DTools::MACField_Float:
        return ((res.x + 1) * res.y * res.z) +
               (res.x * (res.y + 1) * res.z) +
               (res.y * res.y * (res.z + 1));
      default:
        return 0;
      }
    }
  }
  
  return 0;
}

MStatus Field3dCacheFormat::readDoubleArray(MDoubleArray &array, unsigned size)
{
  MGlobal::displayInfo("f3dCache::readDoubleArray");
  return readArray(array, size);
}

MStatus Field3dCacheFormat::readFloatArray(MFloatArray &array, unsigned size)
{
  MGlobal::displayInfo("f3dCache::readFloatArray");
  return readArray(array, size);
}

MStatus Field3dCacheFormat::readIntArray(MIntArray &, unsigned size)
{
  MGlobal::displayInfo("f3dCache::readIntArray");
  return MS::kFailure;
}

MStatus Field3dCacheFormat::readDoubleVectorArray(MVectorArray &, unsigned arraySize)
{
  MGlobal::displayInfo("f3dCache::readDoubleVectorArray");
  return MS::kFailure;
}

MStatus Field3dCacheFormat::readFloatVectorArray(MFloatVectorArray &array, unsigned arraySize)
{
  MGlobal::displayInfo("f3dCache::readFloatVectorArray");
  return MS::kFailure;
}

int Field3dCacheFormat::readInt32()
{
  MGlobal::displayInfo("f3dCache::readInt32");
  return 0;
}

template <class T>
MStatus Field3dCacheFormat::readArray(T &array, unsigned long arraySize)
{
  if (m_curCacheFile == m_cacheFiles.end())
  {
    return MS::kFailure;
  }
  
  CacheEntry &cacheFile = m_curCacheFile->second;
  
  array.setLength(arraySize);
  
  if (m_inChannel == "resolution")
  {
    array[0] = (unsigned int) cacheFile.resolution.x;
    array[1] = (unsigned int) cacheFile.resolution.y;
    array[2] = (unsigned int) cacheFile.resolution.z;
    
    return MS::kSuccess;
  }
  else if (m_inChannel == "offset")
  {
    array[0] = cacheFile.offset.x;
    array[1] = cacheFile.offset.y;
    array[2] = cacheFile.offset.z;
    
    return MS::kSuccess;
  }
  else if (m_curField == cacheFile.fields.end())
  {
    return MS::kFailure;
  }
  
  Field3DTools::Fld &field = m_curField->second;
  
  
  // pointer to the read function we'll call based on the dynamic type
  bool success = false;
  
  // select the proper function to call
  switch (field.fieldType)
  {
  case Field3DTools::DenseScalarField_Half:
    success = Field3DTools::readScalarField<Field3D::half, T>(field.dhScalarField, array);
    break;
  case Field3DTools::DenseScalarField_Float:
    success = Field3DTools::readScalarField<float, T>(field.dfScalarField, array);
    break;
  case Field3DTools::DenseScalarField_Double:
    success = Field3DTools::readScalarField<double, T>(field.ddScalarField, array);
    break;
  case Field3DTools::SparseScalarField_Half:
    success = Field3DTools::readScalarField<Field3D::half, T>(field.shScalarField, array);
    break;
  case Field3DTools::SparseScalarField_Float:
    success = Field3DTools::readScalarField<float, T>(field.sfScalarField, array);
    break;
  case Field3DTools::SparseScalarField_Double:
    success = Field3DTools::readScalarField<double, T>(field.sdScalarField, array);
    break;
  case Field3DTools::DenseVectorField_Half:
    success = Field3DTools::readVectorField<Field3D::half, T>(field.dhVectorField, array);
    break;
  case Field3DTools::DenseVectorField_Float:
    success = Field3DTools::readVectorField<float, T>(field.dfVectorField, array);
    break;
  case Field3DTools::DenseVectorField_Double:
    success = Field3DTools::readVectorField<double, T>(field.ddVectorField, array);
    break;
  case Field3DTools::SparseVectorField_Half:
    success = Field3DTools::readVectorField<Field3D::half, T>(field.shVectorField, array);
    break;
  case Field3DTools::SparseVectorField_Float:
    success = Field3DTools::readVectorField<float, T>(field.sfVectorField, array);
    break;
  case Field3DTools::SparseVectorField_Double:
    success = Field3DTools::readVectorField<double, T>(field.sdVectorField, array);
    break;
  case Field3DTools::MACField_Half:
    success = Field3DTools::readMACField<Field3D::half, T>(field.mhField, array);
    break;
  case Field3DTools::MACField_Float:
    success = Field3DTools::readMACField<float, T>(field.mfField, array);
    break;
  case Field3DTools::MACField_Double:
    success = Field3DTools::readMACField<double, T>(field.mdField, array);
    break;
  default:
    ERROR("Type unknown or unsupported");
    return MS::kFailure;
  }
  
  return (success ? MS::kSuccess : MS::kFailure);
}


bool Field3dCacheFormat::handlesDescription()
{
  MGlobal::displayInfo(MString("f3dCache::handlesDescription: ") + (m_mode == kWrite ? "false" : "true"));
  
  if (m_mode == kWrite)
  {
    // leave it to maya
    return false;
  }
  else
  {
    return true;
  }
}

MStatus Field3dCacheFormat::readDescription(MCacheFormatDescription &description, const MString &descriptionFileLocation, const MString &baseFileName)
{
  MGlobal::displayInfo("f3dCache::readDescription \"" + descriptionFileLocation + "\", \"" + baseFileName + "\"");
  
  return MPxCacheFormat::readDescription(description, descriptionFileLocation, baseFileName);
}

MStatus Field3dCacheFormat::writeDescription(const MCacheFormatDescription &description, const MString &descriptionFileLocation, const MString &baseFileName)
{
  return MPxCacheFormat::writeDescription(description, descriptionFileLocation, baseFileName);
}

/*
MStatus Field3dCacheFormat::open(const MString& fileName, FileAccessMode mode)
{
  if (fileName == m_filename.c_str())
  {
    if (mode == kRead && m_inFile)
    {
      // Nothing to do
      return MS::kSuccess;
    }
    
    if (mode == kWrite && m_outFile)
    {
      // Nothing to do
      return MS::kSuccess;
    }
    
    if (mode == kReadWrite && m_inFile && m_outFile)
    {
      // Nothing to do
      return MS::kSuccess;
    }
  }
  else
  {
    // delete previous Field3dFile :
    // Field3DInput/Output/File ::clear() and close()
    // doesn't seem to work properly
    
    if (m_inFile)
    {
      delete m_inFile;
      m_inFile = 0;
    }
    
    if (m_outFile)
    {
      delete m_outFile;
      m_outFile = 0;
    }
  }
  
  // Even if file is already open, reset the name stack
  m_readNameStack = true;

  if ((mode == kReadWrite || mode == kRead) && !m_inFile)
  {
    m_inFile = new Field3DInputFile();
    
    // open the file
    if (!m_inFile->open(fileName.asChar()))
    {
      ERROR(std::string("Opening of") +  fileName.asChar() + "failed : Unknown reason");
      delete m_inFile;
      m_inFile = 0;
      m_isFileOpened = false;
      return MS::kFailure;
    }
    
    // retreive the offset from the global meta-data
    const Field3D::V3f er(-999.999,-999.999,-999.999);
    Field3D::V3f off = m_inFile->metadata().vecFloatMetadata("Offset", er);
    
    if (off == er)
    {
      m_offset[0] = 0.0;
      m_offset[1] = 0.0;
      m_offset[2] = 0.0;
    }
    else
    {
      m_offset[0] = off[0];
      m_offset[1] = off[1];
      m_offset[2] = off[2];
    }
  }

  if ((mode == kReadWrite || mode == kWrite) && !m_outFile)
  {
    m_outFile = new Field3DOutputFile();
     
    // create the file
    if (!m_outFile->create(fileName.asChar(), Field3DOutputFile::OverwriteMode))
    {
      ERROR(std::string("Creation of ") + fileName.asChar() + "failed : Unknown reason");
      delete m_outFile;
      m_outFile = 0;
      m_isFileOpened = false;
      return MS::kFailure;
    }
  }

  if (mode != kReadWrite && mode != kWrite && mode != kRead)
  {
    ERROR(std::string("Opening of ") + fileName.asChar() + "failed : Access mode is not defined");
    m_isFileOpened = false;
    return MS::kFailure ;
  }
  
  // everything is ok from there
  m_filename = fileName.asChar();
  m_isFileOpened = true;

  return MS::kSuccess;
}

MStatus Field3dCacheFormat::isValid()
{
  return (m_isFileOpened ? MS::kSuccess : MS::kFailure);
}

MStatus Field3dCacheFormat::rewind()
{
  return open(MString(m_filename.c_str()), kRead);
}

void Field3dCacheFormat::close()
{
  // Note: do nothing, maya is constantly calling this one
  //       just keep our files around until another file is opened or the cache destroyed
}

//--------------------------------------- WRITE ---------------------------

MStatus Field3dCacheFormat::writeHeader(const MString &version, MTime &startTime, MTime &endTime)
{
  // Offset only needs to be kept separately for Maya.
  // We can't write it as a HDF5 partition's attribute nor
  // metadata since Field3D doesn't provide access to the
  // low-level HDF5 file. So we have 2 options : write it per
  // field, but it's a bit redundant. Instead we choose to
  // write it as a global metadata since this plugin
  // is supposed to export individual fluid from Maya
  // sharing the same mapping
  if (!m_outFile)
  {
    return MS::kFailure;
  }
  
  std::string fluidName = extractFluidName(m_currentName);  // "fluidName_channelName" => fluidName

  if (fluidName.empty())
  {
    // TODO : find out why this fluidName can be empty
    return MS::kSuccess;
  }

  // get fluid node
  MFnFluid fluid;
  
  CHECK_MSTATUS_AND_RETURN_IT(MayaTools::getFluidNode(fluidName, fluid)) ;

  // get dynamic offset == {0.0, 0.0, 0.0} if auto-resize is off
  MayaTools::getNodeValue(fluid, "dynamicOffsetX", m_offset[0]);
  MayaTools::getNodeValue(fluid, "dynamicOffsetY", m_offset[1]);
  MayaTools::getNodeValue(fluid, "dynamicOffsetZ", m_offset[2]);

  // write global metadata attached to the file
  const Field3D::V3f off(m_offset[0], m_offset[1], m_offset[2]);
  
  // m_outFile->metadata().setStrMetadata("Info", "File generated by Maya");
  m_outFile->metadata().setVecFloatMetadata("Offset", off);
  m_outFile->writeGlobalMetadata();

  //  hid_t m_file;
  //  File::Partition::Ptr newPart(new File::Partition);
  //  H5ScopedGcreate partGroup(m_file, newPart->name.c_str());
  //  if (!writeAttribute(partGroup.id(), "is_field3d_partition", "1")) {
  //  }

  return MS::kSuccess;
}

MStatus Field3dCacheFormat::writeChannelName(const MString& name)
{
  m_currentName = name;
  return MS::kSuccess;
}

MStatus Field3dCacheFormat::writeDoubleArray(const MDoubleArray& array)
{
  return writeArray(array);
}

MStatus Field3dCacheFormat::writeFloatArray(const MFloatArray& array)
{
  return writeArray(array);
}
*/

//-------------------------------------------------- READ -------------------------------------------------------

/*
MStatus Field3dCacheFormat::readHeader()
{
  // this function seems to be never invoked ...
  return m_isFileOpened ? MS::kSuccess : MS::kFailure ;
}

MStatus Field3dCacheFormat::findChannelName(const MString& name)
{
  if (!m_inFile)
  {
    return MS::kFailure;
  }
  
  std::string channelName = extractChannelName(name);
  std::string fluidName = extractFluidName(name);
  std::string partition = partitionName(fluidName);

  // resolution and offset are implicitely present
  if (channelName == "resolution" || channelName == "offset")
  {
    m_currentName = name;
    return MS::kSuccess;
  }
  
  // parse channel names present in the field3d file
  std::vector<std::string> channels;
  
  Field3DTools::getFieldNames(m_inFile, partition, channels);

  // iterate through channels and look for the needed one
  if (std::find(channels.begin(), channels.end(), channelName) == channels.end())
  {
    ERROR("Failed to find " + channelName + " Unknown reason.");
    return MS::kFailure;
  }

  // name was found , record it
  m_currentName = name;
  
  return MS::kSuccess;
}

MStatus Field3dCacheFormat::readChannelName(MString& name)
//
//  Given that the right time has already been found, find the name
//  of the channel we're trying to read.
//
//  If no more channels exist, return false. Some callers rely on this false return
//  value to terminate scanning for channels, thus it's not an error condition.
//
{
  if (!m_inFile)
  {
    return MS::kFailure;
  }
  
  std::string fluidName = extractFluidName(m_currentName);

  // re-read the name stack if needed, add extra name
  // resolution and offset since they don't exists as
  // separate fields in the file
  if (m_readNameStack)
  {
    m_nameStack.clear();
    
    std::vector<std::string> tmp;
    
    Field3DTools::getFieldNames(m_inFile, partitionName(fluidName), tmp);
    
    m_nameStack.push_back(fluidName + std::string("_resolution"));
    m_nameStack.push_back(fluidName + std::string("_offset"));
    
    for (std::vector<std::string>::iterator i=tmp.begin(); i!=tmp.end(); ++i)
    {
      m_nameStack.push_back(fluidName + "_" + *i);
    }
    
    m_readNameStack = false;
  }

  // if there are some remaining names in the stack
  // return MS::kSuccess
  if (!m_nameStack.empty())
  {
    name = m_nameStack.back().c_str();
    
    m_nameStack.pop_back();
    
    m_currentName = name;
    
    return MS::kSuccess;
  }
  
  return MS::kFailure;
}

unsigned Field3dCacheFormat::readArraySize()
{
  if (!m_inFile)
  {
    return 0;
  }
  
  std::string channelName = extractChannelName(m_currentName);
  std::string fluidName = extractFluidName(m_currentName);
  std::string partition = partitionName(fluidName);
  
  if (channelName == "resolution" || channelName == "offset")
  {
    return 3;
  }
  
  unsigned size = 0;
  
  // get resolution of the first field found
  Field3DTools::SupportedFieldTypeEnum fieldType = Field3DTools::TypeUnsupported;
  
  // this will read field data, but they will be cached
  if (Field3DTools::getFieldValueType(m_inFile, partition, channelName, fieldType))
  {
    unsigned int resolution[3] = {0, 0, 0};
    
    Field3DTools::getFieldsResolution(m_inFile, partition, channelName, resolution);
    
    switch (fieldType)
    {
    case Field3DTools::DenseScalarField_Half:
    case Field3DTools::DenseScalarField_Float:
    case Field3DTools::DenseScalarField_Double:
    case Field3DTools::SparseScalarField_Half:
    case Field3DTools::SparseScalarField_Float:
    case Field3DTools::SparseScalarField_Double:
      size = resolution[0] * resolution[1] * resolution[2];
      break;
    
    case Field3DTools::DenseVectorField_Half:
    case Field3DTools::DenseVectorField_Float:
    case Field3DTools::DenseVectorField_Double:
    case Field3DTools::SparseVectorField_Half:
    case Field3DTools::SparseVectorField_Float:
    case Field3DTools::SparseVectorField_Double:
      size = resolution[0] * resolution[1] * resolution[2] * 3;
      break;
    
    case Field3DTools::MACField_Half:
    case Field3DTools::MACField_Float:
    case Field3DTools::MACField_Double:
      size  = (resolution[0] + 1) * resolution[1] * resolution[2];
      size += resolution[0] * (resolution[1] + 1) * resolution[2];
      size += resolution[0] * resolution[1] * (resolution[2] + 1);
      break;
    
    default:
      ERROR("Failed to get channel resolution " + channelName + " : Type not recognized ");
      return 0;
    }
  }
  else
  {
    ERROR("Failed to get channel resolution " + channelName + " : Type not recognized ");
    return 0;
  }

  return size;

}

template <class T> // T is MFloatArray or MDoubleArray
MStatus Field3dCacheFormat::readArray(T &array, unsigned int arraySize)
{
  if (!m_inFile)
  {
    return MS::kFailure;
  }
  
  std::string channelName = extractChannelName(m_currentName) ;
  std::string fluidName = extractFluidName(m_currentName)   ;
  std::string partition = partitionName(fluidName);

  // assuming the resolution of all fields are at the same
  // which could be obviously not true for any generic Field3d file
  unsigned int resolution[3] = {1, 1, 1};
  
  std::stringstream size ;
  size << arraySize   ;
  
  // allocate memory
  array.setLength(arraySize);

  if (channelName == "resolution")
  {
    Field3DTools::getFieldsResolution(m_inFile, partition, "", resolution);
    
    array[0] = resolution[0];
    array[1] = resolution[1];
    array[2] = resolution[2];
    
    return MS::kSuccess;
  }
  else if (channelName == "offset")
  {
    array[0] = m_offset[0];
    array[1] = m_offset[1];
    array[2] = m_offset[2];
    
    return MS::kSuccess;
  }
  
  Field3DTools::getFieldsResolution(m_inFile, partition, channelName, resolution);
  
  // check dynamically the type of the field
  Field3DTools::SupportedFieldTypeEnum fieldType = Field3DTools::TypeUnsupported ;
  
  bool res = Field3DTools::getFieldValueType(m_inFile, partition, channelName, fieldType);
  
  if (!res)
  {
    ERROR("Failed to read " + channelName + " : Data type unsupported");
    return MS::kFailure;
  }

  // pointer to the read function we'll call based on the dynamic type
  bool (*readFuncPtr)(Field3D::Field3DInputFile*, const std::string&, const std::string&, T&) = NULL;
  std::string typeName = "";

  // select the proper function to call
  switch (fieldType)
  {
  case Field3DTools::DenseScalarField_Half:
    typeName = "Dense Scalar Field Half";
    readFuncPtr =  & (Field3DTools::readScalarField<Field3D::DenseField<Field3D::half> >) ;
    break;
  case Field3DTools::DenseScalarField_Float:
    typeName = "Dense Scalar Field Float";
    readFuncPtr =  & (Field3DTools::readScalarField<Field3D::DenseField<float> >) ;
    break;
  case Field3DTools::DenseScalarField_Double:
    typeName = "Dense Scalar Field Double";
    readFuncPtr =  & (Field3DTools::readScalarField<Field3D::DenseField<double> >) ;
    break;
  case Field3DTools::SparseScalarField_Half:
    typeName = "Sparse Scalar Field Half";
    readFuncPtr =  & (Field3DTools::readScalarField<Field3D::SparseField<Field3D::half> >) ;
    break;
  case Field3DTools::SparseScalarField_Float:
    typeName = "Sparse Scalar Field Float";
    readFuncPtr =  & (Field3DTools::readScalarField<Field3D::SparseField<float> >) ;
    break;
  case Field3DTools::SparseScalarField_Double:
    typeName = "Sparse Scalar Field Double";
    readFuncPtr =  & (Field3DTools::readScalarField<Field3D::SparseField<double> >) ;
    break;
  case Field3DTools::DenseVectorField_Half:
    typeName = "Dense Vector Field Half";
    readFuncPtr =  & (Field3DTools::readVectorField<Field3D::DenseField<Field3D::V3h> >) ;
    break;
  case Field3DTools::DenseVectorField_Float:
    typeName = "Dense Vector Field Float";
    readFuncPtr =  & (Field3DTools::readVectorField<Field3D::DenseField<Field3D::V3f> >) ;
    break;
  case Field3DTools::DenseVectorField_Double:
    typeName = "Dense Vector Field Double";
    readFuncPtr =  & (Field3DTools::readVectorField<Field3D::DenseField<Field3D::V3d> >) ;
    break;
  case Field3DTools::SparseVectorField_Half:
    typeName = "Sparse Vector Field Half";
    readFuncPtr =  & (Field3DTools::readVectorField<Field3D::SparseField<Field3D::V3h> >) ;
    break;
  case Field3DTools::SparseVectorField_Float:
    typeName = "Sparse Vector Field Float";
    readFuncPtr =  & (Field3DTools::readVectorField<Field3D::SparseField<Field3D::V3f> >) ;
    break;
  case Field3DTools::SparseVectorField_Double:
    typeName = "Sparse Vector Field Double";
    readFuncPtr =  & (Field3DTools::readVectorField<Field3D::SparseField<Field3D::V3d> >) ;
    break;
  case Field3DTools::MACField_Half:
    typeName = "MAC Field Half";
    readFuncPtr =  & (Field3DTools::readMACField<Field3D::half>) ;
    break;
  case Field3DTools::MACField_Float:
    typeName = "MAC Field Float";
    readFuncPtr =  & (Field3DTools::readMACField<float>) ;
    break;
  case Field3DTools::MACField_Double:
    typeName = "MAC Field Double";
    readFuncPtr =  & (Field3DTools::readMACField<double>) ;
    break;
  default:
    ERROR("Type unknown or unsupported");
    return MS::kFailure;
  }
  
  // call the function
  bool read_ok = (*readFuncPtr)(m_inFile, partition, channelName, array);

  // check if the field was successfully read
  if (!read_ok)
  {
    ERROR( "Failed to read " + channelName );
    return MS::kFailure;
  }

  return  MS::kSuccess;
}

MStatus Field3dCacheFormat::readFloatArray(MFloatArray& array, unsigned arraySize)
{
  return readArray(array, arraySize);
}

MStatus Field3dCacheFormat::readDoubleArray(MDoubleArray& array, unsigned arraySize)
{
  return readArray(array, arraySize);
}

// -------------------------------------------------- TIME ---------------------------

MStatus Field3dCacheFormat::readTime(MTime &time)
{
  //cout<<red<<"readTime "<<normal<<endl;

  // exract the time from the name of the
  // cache file to keep things simple
  size_t framePos = m_filename.rfind("Frame");
  size_t pointPos = m_filename.rfind(".");
  
  std::string frameNumberStr = m_filename.substr(framePos, pointPos);

  int frameNumber = atoi(frameNumberStr.c_str());

  return (frameNumber > 0 ? MS::kSuccess : MS::kFailure);
}

MStatus Field3dCacheFormat::writeTime(MTime &time)
{
  return MS::kSuccess;
}

MStatus Field3dCacheFormat::readNextTime(MTime& foundTime)
//
// Read the next time based on the current read position.
//
{
  MTime readAwTime(0.0, MTime::k6000FPS);
  bool ret = readTime(readAwTime);
  foundTime = readAwTime;

  return (ret ? MS::kSuccess : MS::kFailure);
}

MStatus Field3dCacheFormat::findTime(MTime& time, MTime& foundTime)
//
// Find the biggest cached time, which is smaller or equal to
// seekTime and return foundTime
//
{

  MTime timeTolerance(0.0, MTime::k6000FPS);
  MTime seekTime(time);
  MTime preTime( seekTime - timeTolerance );
  MTime postTime( seekTime + timeTolerance );

  bool fileRewound = false;
  
  while (1)
  {
    bool timeTagFound = beginReadChunk();
    
    if (!timeTagFound && !fileRewound )
    {
      if(!rewind())
      {
        return MS::kFailure;
      }
      fileRewound = true;
      timeTagFound = beginReadChunk();
    }
    
    if (timeTagFound)
    {
      MTime rTime(0.0, MTime::k6000FPS);
      readTime(rTime);

      if (rTime >= preTime && rTime <= postTime)
      {
        foundTime = rTime;
        return MS::kSuccess;
      }
      
      if (rTime > postTime)
      {
        if (!fileRewound)
        {
          if(!rewind())
          {
            return MS::kFailure;
          }
          fileRewound = true;
        }
        else
        {
          // Time could not be found
          //
          return MS::kFailure;
        }
      }
      else
      {
        fileRewound = true;
      }
      endReadChunk();
    }
    else
    {
      // Not a valid disk cache file.
      break;
    }
  }

  return MS::kFailure;
}
*/
