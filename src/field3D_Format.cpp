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
#include <maya/MCacheFormatDescription.h>

#include <iostream>
#include <sstream>
#include <fstream>

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
   
   // search for .frame.subframe pattern
   
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
   
   m_inSeq.clear();
   
   MString _dn, _bn, _fn, _e;
   MTime t;
   int f, sf;
   
   for (unsigned long i=0; i<tmp.length(); ++i)
   {
      if (m_inDesc.filePattern.length() > 0)
      {
         if (m_inDesc.useSubFrames)
         {
            if (sscanf(tmp[i].asChar(), m_inDesc.filePattern.c_str(), &f, &sf) != 2)
            {
               continue;
            }
         }
         else
         {
            if (sscanf(tmp[i].asChar(), m_inDesc.filePattern.c_str(), &f) != 1)
            {
               continue;
            }
         }
      }
      
      if (identifyPath(tmp[i], _dn, _bn, _fn, t, _e))
      {
         m_inSeq[t] = mdn + tmp[i];
      }
   }
   
   m_inCurFile = m_inSeq.end();
   
   return (unsigned long) m_inSeq.size();
}

void Field3dCacheFormat::resetInputFile()
{
   m_inFields.clear();
   m_inCurField = m_inFields.end();
   m_inNextField = m_inFields.end();
   
   m_inFluidName = "";
   m_inPartition = "";
   
   m_inResolution = Field3D::V3i(0, 0, 0);
   m_inOffset = Field3D::V3f(0.0f, 0.0f, 0.0f);
   m_inDimension = Field3D::V3f(1.0f, 1.0f, 1.0f);
   
   if (m_inFile)
   {
      delete m_inFile;
      m_inFile = 0;
   }
   
   m_inCurFile = m_inSeq.end();
}

MStatus Field3dCacheFormat::open(const MString &fileName, FileAccessMode mode)
{
   m_mode = mode;
   
   if (mode == kRead || mode == kReadWrite)
   {
      // Note: this may be called several time for the same frame
      MString dn, bn, frm, ext;
      MTime t;
      
      if (!identifyPath(fileName, dn, bn, frm, t, ext))
      {
         return MS::kFailure;
      }
      
      std::string inFilename = std::string(dn.asChar()) + "/" + bn.asChar();
      
      if (inFilename != m_inFilename)
      {
         SequenceDesc desc;
         
         readDescription(inFilename + ".xml", desc);
                  
         if (desc.dir.length() > 0)
         {
            inFilename = desc.dir + "/" + bn.asChar();
            dn = desc.dir.c_str();
         }
         
         // re-evaluate inFilename
         
         if (inFilename != m_inFilename)
         {
            m_inDesc = desc;
            
            // this is a difference file sequence
            resetInputFile();
            
            m_inFilename = inFilename;
            
            // don't use fileName as directory may have changed
            // basename should stay identical whatever the frame pattern is
            
            fillCacheFiles(dn, bn, ext);
         }
      }
      
      std::map<MTime, MString>::iterator it = m_inSeq.find(t);
      
      if (it == m_inSeq.end())
      {
         // not a valid frame file
         resetInputFile();
         return MS::kFailure;
      }
      else
      {
         if (it != m_inCurFile && m_inFile)
         {
            // different frame than currently loaded one
            resetInputFile();
         }
         
         if (!m_inFile)
         {
            // frame not yet read
            m_inFile = new Field3DInputFile();
            
            if (!m_inFile->open(it->second.asChar()))
            {
               ERROR(std::string("Opening of") +  fileName.asChar() + "failed : Unknown reason");
               resetInputFile();
               
               return MS::kFailure;
            }
         }
         
         // at this point, if open was called for same frame as currently loaded one
         // nothing should have changed (it == m_inCurFile)
         
         m_inCurFile = it;
         m_inCurField = m_inFields.end();
         m_inNextField = m_inFields.begin();
      }
   }
   
   if (mode == kWrite || mode == kReadWrite)
   {
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
   
   return MS::kSuccess;
}

void Field3dCacheFormat::close()
{
   // don't close m_inFile as this method may be called several times for the same frame
   
   if (m_outFile)
   {
      delete m_outFile;
      m_outFile = 0;
   }
}

MStatus Field3dCacheFormat::isValid()
{
   if (m_mode == kRead || m_mode == kReadWrite)
   {
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
   // Never called
   return MS::kSuccess;
}

MString Field3dCacheFormat::extension()
{
   return "f3d";
}



MStatus Field3dCacheFormat::writeHeader(const MString &, MTime &, MTime &)
{
   return MS::kSuccess;
}

void Field3dCacheFormat::beginWriteChunk()
{
}

MStatus Field3dCacheFormat::writeTime(MTime &time)
{
   // Never called
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
   // Never called
   return MS::kFailure;
}

MStatus Field3dCacheFormat::writeDoubleVectorArray(const MVectorArray &)
{
   // Never called
   return MS::kFailure;
}

MStatus Field3dCacheFormat::writeFloatVectorArray(const MFloatVectorArray &)
{
   // Never called
   return MS::kFailure;
}

MStatus Field3dCacheFormat::writeInt32(int value)
{
   // Never called
   return MS::kFailure;
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
   
   double transform[4][4] = {{ 1.0, 0.0, 0.0, 0.0 },
                             { 1.0, 1.0, 0.0, 0.0 },
                             { 1.0, 0.0, 1.0, 0.0 },
                             { 1.0, 0.0, 0.0, 1.0 }};
   unsigned int resolution[3] = {0, 0, 0};
   double dimension[3] = {0.0, 0.0, 0.0};
   
   // Get transform matrix
   MMatrix parentTransf = m_outFluid.dagPath().inclusiveMatrix();
   
   // Get resolution
   m_outFluid.getResolution(resolution[0], resolution[1], resolution[2]);
   
   // Get dimension
   m_outFluid.getDimensions(dimension[0], dimension[1], dimension[2]);
   
   // Move the center to [0, 1]
   double mapTo01[4][4] = {{  1.0 ,  0.0 ,  0.0 , 0.0 } ,
                           {  0.0 ,  1.0 ,  0.0 , 0.0 } ,
                           {  0.0 ,  0.0 ,  1.0 , 0.0 } ,
                           { -0.5 , -0.5 , -0.5 , 1.0 }};
   MMatrix mapTo01Transf = MMatrix(mapTo01);
   
   // Auto-resize's offset
   double autoResize[4][4]= {{ dimension[0]  , 0.0           , 0.0           , 0.0 } ,
                             { 0.0           , dimension[1]  , 0.0           , 0.0 } ,
                             { 0.0           , 0.0           , dimension[2]  , 0.0 } ,
                             { m_outOffset[0], m_outOffset[1], m_outOffset[2], 1.0 }};
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
   return MS::kSuccess;
}

MStatus Field3dCacheFormat::beginReadChunk()
{
   return MS::kSuccess;
}

void Field3dCacheFormat::initFields(const std::string &partition)
{
   // re-read partition fields
   m_inFields.clear();
   
   m_inResolution = Field3D::V3i(0, 0, 0);
   m_inOffset = Field3D::V3f(0.0f, 0.0f, 0.0f);
   m_inDimension = Field3D::V3f(1.0f, 1.0f, 1.0f);
   
   // read all fields
   std::vector<std::string> fields;
   
   Field3DTools::getFieldNames(m_inFile, partition, fields);
   
   for (size_t i=0; i<fields.size(); ++i)
   {
      Field3DTools::Fld field;
      
      if (Field3DTools::getFieldValueType(m_inFile, partition, fields[i], field))
      {
         m_inFields[fields[i]] = field;
         
         // supposes all field in a given partition have the same resolution, offset and dimension
         
         Field3D::V3i res = field.baseField->dataResolution();
         
         if (res.x > m_inResolution.x)
         {
            m_inResolution.x = res.x;
         }
         if (res.y > m_inResolution.y)
         {
            m_inResolution.y = res.y;
         }
         if (res.z > m_inResolution.z)
         {
            m_inResolution.z = res.z;
         }
         
         m_inOffset += field.baseField->metadata().vecFloatMetadata("Offset", Field3D::V3f(0.0f, 0.0f, 0.0f));
         
         m_inDimension += field.baseField->metadata().vecFloatMetadata("Dimension", Field3D::V3f(1.0f, 1.0f, 1.0f));
      }
   }
   
   if (m_inFields.size() > 0)
   {
      float scl = 1.0f / float(m_inFields.size());
      
      m_inOffset *= scl;
      m_inDimension *= scl;
   }
   
   // add dummy fields for 'resolution' and 'offset'
   Field3DTools::Fld dummyField;
   dummyField.fieldType = Field3DTools::TypeUnsupported;
   
   m_inFields["resolution"] = dummyField;
   m_inFields["offset"] = dummyField;
   
   m_inCurField = m_inFields.end();
   m_inNextField = m_inFields.begin();
}

MStatus Field3dCacheFormat::findChannelName(const MString &name)
{
   if (!m_inFile)
   {
      return MS::kFailure;
   }
   
   std::string fluidName = extractFluidName(name);
   std::string channel = extractChannelName(name);
   std::string partition = partitionName(fluidName);
   
   if (m_inPartition != partition)
   {
      // partition has changed, reload fields
      initFields(partition);
      
      m_inFluidName = fluidName;
      m_inPartition = partition;
   }
   
   if (m_inFields.size() == 0)
   {
      return MS::kFailure;
   }
   
   // map maya name to field3d name
   std::map<std::string, std::string>::iterator rit = m_inDesc.mapChannels.find(channel);
   if (rit != m_inDesc.mapChannels.end())
   {
      channel = rit->second;
   }
   
   m_inCurField = m_inFields.find(channel);
   return (m_inCurField != m_inFields.end() ? MS::kSuccess : MS::kFailure);
}

MStatus Field3dCacheFormat::readChannelName(MString &name)
{
   if (!m_inFile)
   {
      return MS::kFailure;
   }
   
   if (m_inFluidName == "")
   {
      return MS::kFailure;
   }
   
   if (m_inNextField != m_inFields.end())
   {
      m_inCurField = m_inNextField++;
      
      std::string channel = m_inCurField->first;
      
      // map field3d name to maya name
      std::map<std::string, std::string>::iterator rit = m_inDesc.unmapChannels.find(channel);
      if (rit != m_inDesc.unmapChannels.end())
      {
         channel = rit->second;
      }
      
      name = (m_inFluidName + "_" + channel).c_str();
      
      return MS::kSuccess;
   }
   else
   {
      return MS::kFailure;
   }
}

void Field3dCacheFormat::endReadChunk()
{
}

MStatus Field3dCacheFormat::readTime(MTime &time)
{
   // Never called
   return MS::kFailure;
}

MStatus Field3dCacheFormat::findTime(MTime &time, MTime &foundTime)
{
   // Never called
   return MS::kFailure;
}

MStatus Field3dCacheFormat::readNextTime(MTime &foundTime)
{
   // Never called
   return MS::kFailure;
}

unsigned Field3dCacheFormat::readArraySize()
{
   unsigned rv = 0;
   
   if (m_inCurField != m_inFields.end())
   {
      if (m_inCurField->first == "resolution" || m_inCurField->first == "offset")
      {
         rv = 3;
      }
      else if (m_inCurField->second.baseField)
      {
         Field3DTools::Fld &fld = m_inCurField->second;
         
         Field3D::V3i res = fld.baseField->dataResolution();
         
         switch (fld.fieldType)
         {
         case Field3DTools::DenseScalarField_Half:
         case Field3DTools::DenseScalarField_Float:
         case Field3DTools::DenseScalarField_Double:
         case Field3DTools::SparseScalarField_Half:
         case Field3DTools::SparseScalarField_Float:
         case Field3DTools::SparseScalarField_Double:
            rv = (res.x * res.y * res.z);
            break;
         case Field3DTools::DenseVectorField_Half:
         case Field3DTools::DenseVectorField_Float:
         case Field3DTools::DenseVectorField_Double:
         case Field3DTools::SparseVectorField_Half:
         case Field3DTools::SparseVectorField_Float:
         case Field3DTools::SparseVectorField_Double:
            rv = 3 * (res.x * res.y * res.z);
            break;
         case Field3DTools::MACField_Half:
         case Field3DTools::MACField_Float:
            rv = ((res.x + 1) * res.y * res.z) +
                 (res.x * (res.y + 1) * res.z) +
                 (res.y * res.y * (res.z + 1));
            break;
         default:
            break;
         }
      }
   }
   
   return rv;
}

MStatus Field3dCacheFormat::readDoubleArray(MDoubleArray &array, unsigned size)
{
   return readArray(array, size);
}

MStatus Field3dCacheFormat::readFloatArray(MFloatArray &array, unsigned size)
{
   return readArray(array, size);
}

MStatus Field3dCacheFormat::readIntArray(MIntArray &, unsigned size)
{
   // Never called
   return MS::kFailure;
}

MStatus Field3dCacheFormat::readDoubleVectorArray(MVectorArray &, unsigned arraySize)
{
   // Never called
   return MS::kFailure;
}

MStatus Field3dCacheFormat::readFloatVectorArray(MFloatVectorArray &array, unsigned arraySize)
{
   // Never called
   return MS::kFailure;
}

int Field3dCacheFormat::readInt32()
{
   // Never called
   return 0;
}

template <class T>
MStatus Field3dCacheFormat::readArray(T &array, unsigned long arraySize)
{
   if (!m_inFile)
   {
      return MS::kFailure;
   }
   
   array.setLength(arraySize);
   
   if (m_inCurField == m_inFields.end())
   {
      return MS::kFailure;
   }
   else if (m_inCurField->first == "resolution")
   {
      array[0] = (unsigned int) m_inResolution.x;
      array[1] = (unsigned int) m_inResolution.y;
      array[2] = (unsigned int) m_inResolution.z;
      
      return MS::kSuccess;
   }
   else if (m_inCurField->first == "offset")
   {
      array[0] = m_inOffset.x;
      array[1] = m_inOffset.y;
      array[2] = m_inOffset.z;
      
      return MS::kSuccess;
   }
   
   Field3DTools::Fld &field = m_inCurField->second;
   
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

bool Field3dCacheFormat::readDescription(const std::string &xmlPath, Field3dCacheFormat::SequenceDesc &desc)
{
   std::ifstream is(xmlPath.c_str());
   
   if (!is.is_open())
   {
      return false;
   }
   else
   {
      MGlobal::displayInfo(MString("Reading cache description \"") + xmlPath.c_str() + "\"");
      bool inExtra = false;
      std::string line;
      std::string extra;
      
      // supposes we don't have twice the <extra> tag on the same line
      
      while (is.good())
      {
         std::getline(is, line);
         
         if (inExtra)
         {
            size_t p0 = line.find("</extra>");
            
            if (p0 != std::string::npos)
            {
               extra += line.substr(0, p0);
               inExtra = false;
            }
            else
            {
               extra += line + "\n";
            }
         }
         else
         {
            size_t p0 = line.find("<extra>");
            
            if (p0 != std::string::npos)
            {
               p0 += strlen("<extra>");
               
               size_t p1 = line.find("</extra>", p0);
               
               if (p1 == std::string::npos)
               {
                  extra = line.substr(p0) + "\n";
                  inExtra = true;
               }
               else
               {
                  extra = line.substr(p0, p1 - p0);
               }
            }
         }
         
         if (inExtra == false && extra.length() > 0)
         {
            size_t p = extra.find("f3d.file=");
         
            if (p != std::string::npos)
            {
               desc.filePattern = extra.substr(p + strlen("f3d.file="));
               
               size_t n = std::count(desc.filePattern.begin(), desc.filePattern.end(), '%');
               
               if (n == 0 || n > 2)
               {
                  MGlobal::displayWarning(MString("  Ignoring file pattern: ") + desc.filePattern.c_str());
                  desc.filePattern = "";
               }
               else
               {
                  desc.useSubFrames = (n == 2);
                  
                  size_t p = desc.filePattern.find_last_of("\\/");
                  if (p != std::string::npos)
                  {
                     desc.dir = desc.filePattern.substr(0, p);
                     desc.filePattern = desc.filePattern.substr(p + 1);
                  }
                  else
                  {
                     desc.dir = "";
                  }
                  
                  MGlobal::displayInfo(MString("  File pattern: ") + desc.filePattern.c_str() + " (in directory: \"" + desc.dir.c_str() + "\")");
               }
            }
            else
            {
               // f3d.remap=texture:coord
               // f3d.remap=velocity:v_mac
               p = extra.find("f3d.remap=");
               
               if (p != std::string::npos)
               {
                  std::string remap = extra.substr(p + strlen("f3d.remap="));
                  p = remap.find(':');
                  
                  if (p != std::string::npos)
                  {
                     std::string mayaName = remap.substr(0, p);
                     
                     if (mayaName != "resolution" && mayaName != "offset")
                     {
                        std::string f3dName = remap.substr(p+1);
                        
                        MGlobal::displayInfo(MString("  Remap channel \"") + mayaName.c_str() + "\" -> \"" + f3dName.c_str() + "\"");
                        
                        desc.mapChannels[mayaName] = f3dName;
                        desc.unmapChannels[f3dName] = mayaName;
                     }
                  }
               }
            }
            
            extra = "";
         }
      }
      
      return true;
   }
}


bool Field3dCacheFormat::handlesDescription()
{
   // Never called on read
   return false;
}

MStatus Field3dCacheFormat::readDescription(MCacheFormatDescription &description, const MString &descriptionFileLocation, const MString &baseFileName)
{
   return MPxCacheFormat::readDescription(description, descriptionFileLocation, baseFileName);
}

MStatus Field3dCacheFormat::writeDescription(const MCacheFormatDescription &description, const MString &descriptionFileLocation, const MString &baseFileName)
{
   return MPxCacheFormat::writeDescription(description, descriptionFileLocation, baseFileName);
}
