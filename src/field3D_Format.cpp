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

std::string extractFluidName(const MString &name)
{
  std::string nameStr = name.asChar();
  size_t pos = nameStr.rfind('_');
  return nameStr.substr(0, pos);
}

std::string partitionName(const std::string &fluidName)
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

std::string extractChannelName(const MString &name)
{
  std::string nameStr = name.asChar();
  size_t pos = nameStr.rfind('_');
  return nameStr.substr(pos+1);
}

template <typename T>
std::string display3(T tab[3])
{
  std::stringstream sres;
  sres << tab[0] << " " << tab[1] << " " << tab[2];
  return sres.str();
}

// ------------------------------------------- CONSTRUCTOR - DESTRUCTOR

Field3dCacheFormat::Field3dCacheFormat(Field3DTools::FieldTypeEnum type,
                                       Field3DTools::FieldDataTypeEnum data_type)
{
  Field3D::initIO();

  m_inFile = 0;
  m_outFile = 0;
  m_isFileOpened = false;
  m_readNameStack = true;
  m_offset[0] = 0.0;
  m_offset[1] = 0.0;
  m_offset[2] = 0.0;

  m_fieldType = type;
  m_dataType = data_type;
}

Field3dCacheFormat::~Field3dCacheFormat()
{
  if (m_inFile)
  {
    delete m_inFile;
  }
  
  if (m_outFile)
  {
    delete m_outFile;
  }
}

MStatus Field3dCacheFormat::open(const MString &fileName, FileAccessMode mode)
{
  MGlobal::displayInfo("f3dCache::open \"" + fileName + "\"");
  if (mode == kRead || mode == kReadWrite)
  {
    MGlobal::displayInfo("  For reading");
  }
  if (mode == kWrite || mode == kReadWrite)
  {
    MGlobal::displayInfo("  For writing");
  }
  
  return MS::kFailure;
}

void Field3dCacheFormat::close()
{
  MGlobal::displayInfo("f3dCache::close");
  // don't close file: this gets called several times for the same frame
}

MStatus Field3dCacheFormat::isValid()
{
  MGlobal::displayInfo("f3dCache::isValid");
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

MStatus Field3dCacheFormat::readHeader()
{
  MGlobal::displayInfo("f3dCache::readHeader");
  return MS::kFailure;
}

MStatus Field3dCacheFormat::beginReadChunk()
{
  MGlobal::displayInfo("f3dCache::beginReadChunk");
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
  return 0;
}

MStatus Field3dCacheFormat::readDoubleArray(MDoubleArray &, unsigned size)
{
  MGlobal::displayInfo("f3dCache::readDoubleArray");
  return MS::kFailure;
}

MStatus Field3dCacheFormat::readFloatArray(MFloatArray &, unsigned size)
{
  MGlobal::displayInfo("f3dCache::beginReadChunk");
  return MS::kFailure;
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

MStatus Field3dCacheFormat::findChannelName(const MString &name)
{
  MGlobal::displayInfo("f3dCache::findChannelName \"" + name + "\"");
  return MS::kFailure;
}

MStatus Field3dCacheFormat::readChannelName(MString &name)
{
  MGlobal::displayInfo("f3dCache::readChannelName");
  return MS::kFailure;
}

MStatus Field3dCacheFormat::writeHeader(const MString &version, MTime &startTime, MTime &endTime)
{
  std::ostringstream oss;
  oss << startTime.as(MTime::uiUnit()) << " - " << endTime.as(MTime::uiUnit());
  
  MGlobal::displayInfo("f3dCache::writeHeader " + version + ", " + oss.str().c_str());
  return MS::kFailure;
}

void Field3dCacheFormat::beginWriteChunk()
{
  MGlobal::displayInfo("f3dCache::beginWriteChunk");
}

void Field3dCacheFormat::endWriteChunk()
{
  MGlobal::displayInfo("f3dCache::endWriteChunk");
}

MStatus Field3dCacheFormat::writeTime(MTime &time)
{
  std::ostringstream oss;
  oss << time.as(MTime::uiUnit());
  
  MGlobal::displayInfo(MString("f3dCache::writeTime ") + oss.str().c_str());
  return MS::kFailure;
}

MStatus Field3dCacheFormat::writeDoubleArray(const MDoubleArray &)
{
  MGlobal::displayInfo("f3dCache::writeDoubleArray");
  return MS::kFailure;
}

MStatus Field3dCacheFormat::writeFloatArray(const MFloatArray &)
{
  MGlobal::displayInfo("f3dCache::writeFloatArray");
  return MS::kFailure;
}

MStatus Field3dCacheFormat::writeIntArray(const MIntArray &)
{
  MGlobal::displayInfo("f3dCache::writeIntArray");
  return MS::kFailure;
}

MStatus Field3dCacheFormat::writeDoubleVectorArray(const MVectorArray &array)
{
  MGlobal::displayInfo("f3dCache::writeDoubleVectorArray");
  return MS::kFailure;
}

MStatus Field3dCacheFormat::writeFloatVectorArray(const MFloatVectorArray &array)
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

MStatus Field3dCacheFormat::writeChannelName(const MString &name)
{
  MGlobal::displayInfo("f3dCache::writeChannelName \"" + name + "\"");
  return MS::kFailure;
}

bool Field3dCacheFormat::handlesDescription()
{
  MGlobal::displayInfo("f3dCache::handlesDescription");
  return true;
}

MStatus Field3dCacheFormat::readDescription(MCacheFormatDescription &description, const MString &descriptionFileLocation, const MString &baseFileName)
{
  MGlobal::displayInfo("f3dCache::readDescription \"" + descriptionFileLocation + "\", \"" + baseFileName + "\"");
  return MPxCacheFormat::readDescription(description, descriptionFileLocation, baseFileName);
}

MStatus Field3dCacheFormat::writeDescription(const MCacheFormatDescription &description, const MString &descriptionFileLocation, const MString &baseFileName)
{
  MGlobal::displayInfo("f3dCache::writeDescription \"" + descriptionFileLocation + "\", \"" + baseFileName + "\"");
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
*/

template <class T> // T is MFloatArray or MDoubleArray
MStatus Field3dCacheFormat::writeArray(T &array)
{
  if (!m_outFile)
  {
    return MS::kFailure;
  }
  
  // "fluidName_channelName" => channelName
  // "fluidName_channelName" => fluidName
  std::string channelName = extractChannelName( m_currentName );
  std::string fluidName = extractFluidName( m_currentName );
  std::string partition = partitionName( fluidName );
  
  LOG("Writing channel " + channelName ) ;

  if (partition.empty())
  {
    // TODO : find out why this fluidName can be empty
    return MS::kSuccess;
  }
  
  // _ Resolution is implicitly present in field3d via FieldRes::dataResolution()
  //   so we don't need to store it in a specific extra location.
  // _ Offset is stored in a global metadata while invoking writeHeader()
  //   see this function for more explanations
  if (channelName == "resolution" || channelName == "offset" )
  {
    return MS::kSuccess;
  }

  // maya node
  MFnFluid fluid;
  CHECK_MSTATUS_AND_RETURN_IT( MayaTools::getFluidNode(fluidName, fluid) );

  // transform
  double transform[4][4];
  CHECK_MSTATUS_AND_RETURN_IT( MayaTools::getTransform(fluidName, transform) );

  // resolution
  unsigned int resolution[3] = {1, 1, 1};
  fluid.getResolution(resolution[0], resolution[1], resolution[2]);

  // dimension != {1,1,1} if auto-resize is enabled
  double dimension[3] = {0.0, 0.0, 0.0};
  fluid.getDimensions(dimension[0], dimension[1], dimension[2]);

  // move the center to [0,1]
  double mapTo01[4][4] = {
      { 1.0  , 0.0  , 0.0  , 0.0 } ,
      { 0.0  , 1.0  , 0.0  , 0.0 } ,
      { 0.0  , 0.0  , 1.0  , 0.0 } ,
      { -0.5 , -0.5 , -0.5 , 1.0 }
  };
  MMatrix mapTo01Transf = MMatrix(mapTo01);

  // auto-resize's offset
  double autoResize[4][4]= {
      { dimension[0] , 0.0          , 0.0          , 0.0 } ,
      { 0.0          , dimension[1] , 0.0          , 0.0 } ,
      { 0.0          , 0.0          , dimension[2] , 0.0 } ,
      { m_offset[0]  , m_offset[1]  , m_offset[2]  , 1.0 }
  };
  MMatrix autoResizeTransf = MMatrix(autoResize);

  // apply transformation
  MMatrix parentTransf = MMatrix(transform);
  MMatrix resTransf = mapTo01Transf * autoResizeTransf * parentTransf;
  
  resTransf.get(transform);

  // test the type of array
  bool density     = ( channelName == "density"     );
  bool pressure    = ( channelName == "pressure"    );
  bool fuel        = ( channelName == "fuel"        );
  bool temperature = ( channelName == "temperature" );
  bool falloff     = ( channelName == "falloff"     );
  bool color       = ( channelName == "color"       );
  bool coord       = ( channelName == "texture"     );
  bool velocity    = ( channelName == "velocity"    );

  if (density || pressure || fuel || temperature  || falloff)
  {
    // pointer to the read function we'll call based on the dynamic type
    Field3DTools::WriteScalarFieldFunc writeScalarFuncPtr = NULL;

    // fetch the raw data
    float *          data = NULL                ;
    if (density)     data = fluid.density()     ;
    if (pressure)    data = fluid.pressure()    ;
    if (fuel)        data = fluid.fuel()        ;
    if (temperature) data = fluid.temperature() ;
    if (falloff)     data = fluid.falloff()     ;

    // select the propers function
    if ( m_dataType == Field3DTools::HALF)
    {
      if ( m_fieldType == Field3DTools::SPARSE )
      {
        writeScalarFuncPtr = &Field3DTools::writeSparseScalarField<Field3D::half> ;
      }
      else
      {
        writeScalarFuncPtr = &Field3DTools::writeDenseScalarField<Field3D::half> ;
      }
    }
    else if ( m_dataType == Field3DTools::FLOAT)
    {
      if ( m_fieldType == Field3DTools::SPARSE )
      {
        writeScalarFuncPtr = &Field3DTools::writeSparseScalarField<float>;
      }
      else
      {
        writeScalarFuncPtr = &Field3DTools::writeDenseScalarField<float>;
      }
    }
    else
    {
      ERROR( "Writing of " + channelName + " file failed : Unknown Types");
      return MS::kFailure;
    }

    // write this field
    bool res = (*writeScalarFuncPtr)(m_outFile, partition.c_str(), channelName.c_str(), resolution, transform, data);
    if (!res)
    {
      ERROR( "Writing of " + channelName + " file failed : Unknown reason ( see above for an explanation ? )");
      return MS::kFailure;
    }
  }
  else if (color || coord || velocity)
  {
    // fetch the raw data
    float         *a = NULL, *b = NULL, *c = NULL ;
    if (color)    fluid.getColors(a, b, c)        ;
    if (coord)    fluid.getCoordinates(a, b, c)   ;
    if (velocity) fluid.getVelocity(a, b, c)      ;

    // pointer to the read function we'll call based on the dynamic type
    Field3DTools::WriteVectorFieldFunc writeVectorFuncPtr = NULL;

    // color and coord must not be stored as sparse fields
    // as we don't know how the threshold can affect them
    if (velocity)
    {
      if ( m_dataType == Field3DTools::HALF )
      {
        writeVectorFuncPtr = &Field3DTools::writeMACVectorField<Field3D::half> ;
      }
      else if ( m_dataType == Field3DTools::FLOAT )
      {
        writeVectorFuncPtr = &Field3DTools::writeMACVectorField<float> ;
      }
      else
      {
        ERROR( "Writing of " + channelName + " file failed : Unknown Types");
        return MS::kFailure;
      }
    }
    else
    {
      if ( m_dataType == Field3DTools::HALF )
      {
        if ( m_fieldType == Field3DTools::SPARSE )
        {
          writeVectorFuncPtr = &Field3DTools::writeSparseVectorField<Field3D::half> ;
        }
        else
        {
          writeVectorFuncPtr = &Field3DTools::writeDenseVectorField<Field3D::half> ;
        }
      }
      else if ( m_dataType == Field3DTools::FLOAT )
      {
        if ( m_fieldType == Field3DTools::SPARSE )
        {
          writeVectorFuncPtr = &Field3DTools::writeSparseVectorField<float> ;
        }
        else
        {
          writeVectorFuncPtr = &Field3DTools::writeDenseVectorField<float> ;
        }
      }
      else
      {
        ERROR( "Writing of " + channelName + " file failed : Unknown Types");
        return MS::kFailure;
      }
    }
    
    // write this field
    bool res = (*writeVectorFuncPtr)(m_outFile, partition.c_str(), channelName.c_str(), resolution, transform, a, b, c);

    if (!res)
    {
      ERROR( "Writing of " + channelName + " file failed : Unknown reason ( see above for an explanation ? )");
      return MS::kFailure;
    }
  }

  return MS::kSuccess;

}

/*
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
*/

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

/*
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
