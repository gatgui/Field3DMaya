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


#ifndef FIELD3D_MAYA_CACHE_FORMAT
#define FIELD3D_MAYA_CACHE_FORMAT

// This is added to prevent multiple definitions of the MApiVersion string.
#define _MApiVersion

#include <maya/MIOStream.h>
#include <maya/MPxNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MVector.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MPxCacheFormat.h>
#include <maya/MTime.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFloatArray.h>
#include <maya/MIntArray.h>
#include <maya/MVectorArray.h>
#include <maya/MFnFluid.h>

#include <Field3D/Field3DFile.h>
#include <Field3D/MACField.h>
#include <Field3D/DenseField.h>
#include <Field3D/InitIO.h>

#include <deque>

using namespace Field3D;


#include "field3D_Tools.h"

class Field3dCacheFormat : public MPxCacheFormat
{
public:
   
   // specific creator : D = Dense , S = Sparse, F = float, H = half, D = double
   static void *DHCreator() { return new Field3dCacheFormat(Field3DTools::DENSE, Field3DTools::HALF); }
   static void *DFCreator() { return new Field3dCacheFormat(Field3DTools::DENSE, Field3DTools::FLOAT); }
   static void *DDCreator() { return new Field3dCacheFormat(Field3DTools::DENSE, Field3DTools::DOUBLE); }
   static void *SHCreator() { return new Field3dCacheFormat(Field3DTools::SPARSE, Field3DTools::HALF); }
   static void *SFCreator() { return new Field3dCacheFormat(Field3DTools::SPARSE, Field3DTools::FLOAT); }
   static void *SDCreator() { return new Field3dCacheFormat(Field3DTools::SPARSE, Field3DTools::DOUBLE); }
   
public:
   
   Field3dCacheFormat(Field3DTools::FieldTypeEnum type,
                      Field3DTools::FieldDataTypeEnum data_type);
   ~Field3dCacheFormat();
   
   virtual MStatus open(const MString &fileName, FileAccessMode mode);
   virtual void close();
   virtual MStatus isValid();
   virtual MStatus rewind();
   virtual MString extension(); // { return "f3d"; }
   
   virtual MStatus readHeader();
   virtual MStatus beginReadChunk();
   virtual void endReadChunk();
   virtual MStatus readTime(MTime &time);
   virtual MStatus findTime(MTime &time, MTime &foundTime);
   virtual MStatus readNextTime(MTime &foundTime);
   virtual unsigned readArraySize();
   virtual MStatus readDoubleArray(MDoubleArray &, unsigned size);
   virtual MStatus readFloatArray(MFloatArray &, unsigned size);
   virtual MStatus readIntArray(MIntArray &, unsigned size);
   virtual MStatus readDoubleVectorArray(MVectorArray &, unsigned arraySize);
   virtual MStatus readFloatVectorArray(MFloatVectorArray &array, unsigned arraySize);
   virtual int readInt32();
   virtual MStatus findChannelName(const MString &name);
   virtual MStatus readChannelName(MString &name);
   
   virtual MStatus writeHeader(const MString &version, MTime &startTime, MTime &endTime);
   virtual void beginWriteChunk();
   virtual void endWriteChunk();
   virtual MStatus writeTime(MTime &time);
   virtual MStatus writeDoubleArray(const MDoubleArray &);
   virtual MStatus writeFloatArray(const MFloatArray &);
   virtual MStatus writeIntArray(const MIntArray &);
   virtual MStatus writeDoubleVectorArray(const MVectorArray &array);
   virtual MStatus writeFloatVectorArray(const MFloatVectorArray &array);
   virtual MStatus writeInt32(int);
   virtual MStatus writeChannelName(const MString &name);
   
   virtual bool handlesDescription();
   virtual MStatus readDescription(MCacheFormatDescription &description, const MString &descriptionFileLocation, const MString &baseFileName);
   virtual MStatus writeDescription(const MCacheFormatDescription &description, const MString &descriptionFileLocation, const MString &baseFileName);
   
   /*
   // general functions inherited from MPxCacheFormat
   MStatus open    ( const MString& fileName, FileAccessMode mode);
   void    close   ();
   MStatus isValid ();
   
   // write functions inherited from MPxCacheFormat
   MStatus writeHeader      ( const MString& version, MTime& startTime, MTime& endTime);
   MStatus writeFloatArray  ( const MFloatArray&   );
   MStatus writeDoubleArray ( const MDoubleArray&  );
   MStatus writeChannelName ( const MString & name );
   MStatus writeTime        ( MTime& time);
   void    beginWriteChunk  () {};
   void    endWriteChunk    () {};
   
   // read functions inherited from MPxCacheFormat
   MStatus  readFloatArray  ( MFloatArray&  , unsigned size );
   MStatus  readDoubleArray ( MDoubleArray& , unsigned size );
   MStatus  findChannelName ( const MString & name);
   MStatus  readChannelName ( MString& name);
   unsigned readArraySize   ();
   MStatus  readHeader      ();
   MStatus  beginReadChunk  () {return MStatus::kSuccess;};
   void     endReadChunk    () {};
   
   // timeline
   MStatus  readTime        ( MTime& time);
   MStatus  findTime        ( MTime& time, MTime& foundTime);
   MStatus  readNextTime    ( MTime& foundTime);
   MStatus  rewind();
   
private:
   
   template <class T>  // T is MFloatArray or MDoubleArray
   MStatus writeArray(T  &array);
   
   template <class T>  // T is MFloatArray or MDoubleArray
   MStatus readArray(T &array, unsigned arraySize);
   */
   
   template <class T>
   MStatus readArray(T &array, unsigned long arraySize);
   
   template <class T>
   MStatus writeArray(T &array);
   
private:
   
   Field3DTools::FieldTypeEnum m_fieldType;
   Field3DTools::FieldDataTypeEnum m_dataType;
   
   FileAccessMode m_mode;
   
   std::string m_inFilename;
   Field3DInputFile *m_inFile;
   std::string m_inFluidName;
   std::string m_inPartition;
   std::string m_inChannel;
   
   std::string m_outFilename;
   Field3DOutputFile *m_outFile;
   std::string m_outPartition;
   std::string m_outChannel;
   MFnFluid m_outFluid;
   float m_outOffset[3];
   
   
   struct CacheEntry
   {
      MString path;
      std::map<std::string, Field3DTools::Fld> fields;
      Field3D::V3i resolution;
      Field3D::V3f offset;
      Field3D::V3f dimension;
   };
   
   std::map<std::string, std::string> m_remapChannels;
   std::map<MTime, CacheEntry> m_cacheFiles;
   std::map<MTime, CacheEntry>::iterator m_curCacheFile;
   std::map<std::string, Field3DTools::Fld>::iterator m_curField;
   std::map<std::string, Field3DTools::Fld>::iterator m_nextField;
   
   bool identifyPath(const MString &path, MString &dirname, MString &basename, MString &frame, MTime &t, MString &ext);
   unsigned long fillCacheFiles(const MString &path);
   unsigned long fillCacheFiles(const MString &dirname, const MString &basename, const MString &ext);
   
   /*
   std::string m_filename;
   bool m_isFileOpened;
   MString m_currentName;
   bool m_readNameStack;
   float m_offset[3];
   
   std::deque<std::string> m_nameStack;
   */
   
   // export Type
   
};

#endif
