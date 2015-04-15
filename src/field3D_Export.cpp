//----------------------------------------------------------------------------//

/*
 * Copyright (c) 2009 Sony Pictures Imageworks Inc
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.  Neither the name of Sony Pictures Imageworks nor the
 * names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
//----------------------------------------------------------------------------//

/*! \file exportF3d.cpp
  \brief Simple exporter for Maya Fluid to f3d format.  density, temperature, 
  fuel are exported automatically if they are valid, all other fields require flags.
*/

#include "field3D_Export.h"

#include <maya/MPlug.h>
#include <maya/MArgDatabase.h>
#include <maya/MItSelectionList.h>
#include <maya/MFnCamera.h>
#include <maya/MAnimControl.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagPath.h>
#include <maya/MMatrix.h>
#include <maya/MTransformationMatrix.h>

#include <maya/MIOStream.h>
#include <maya/MGlobal.h>
#include <maya/MFnTransform.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MFileIO.h>
#include <maya/MCommonSystemUtils.h>
#include <maya/MTime.h>

#include <sys/wait.h>
#include <ctype.h>
#include <string>
#include <list>
#include <vector>
#include <iostream>
#include <fstream>

#include <Field3D/DenseField.h>
#include <Field3D/SparseField.h>
#include <Field3D/MACField.h>
#include <Field3D/Field3DFile.h>
#include <Field3D/InitIO.h>

#include <maya/MComputation.h>

#define ERRCHKR   \
  if ( MS::kSuccess != stat ) { \
    std::cerr << stat.errorString().asChar(); \
    return stat;  \
  }

#define ERRCHK    \
  if ( MS::kSuccess != stat ) { \
    std::cerr << stat.errorString().asChar(); \
  }


using namespace Field3D;

//----------------------------------------------------------------------------//

void StripWS(std::string &s)
{
  size_t p = s.find_first_not_of(" \t\n");
  
  if (p == std::string::npos)
  {
    s = "";
  }
  else
  {
    if (p > 0)
    {
      s = s.substr(p);
    }
    
    p = s.find_last_not_of(" \t\n");
    
    if (p == std::string::npos)
    {
      return;
    }
    else if (p + 1 < s.length())
    {
      s = s.substr(0, p + 1);
    }
  }
}

size_t Split(const std::string &s, char sep, std::vector<std::string> &splits, bool strip=true)
{
  std::string split;
  
  splits.clear();
  
  size_t p0 = 0;
  size_t p1 = s.find(sep, p0);
  
  while (p1 != std::string::npos)
  {
    split = s.substr(p0, p1 - p0);
    
    if (strip)
    {
      StripWS(split);
    }
    
    if (split.length() > 0)
    {
      splits.push_back(split);
    }
    
    p0 = p1 + 1;
    p1 = s.find(sep, p0);
  }
  
  split = s.substr(p0);
  
  if (strip)
  {
    StripWS(split);
  }
  
  if (split.length() > 0)
  {
    splits.push_back(split);
  }
  
  return splits.size();
}

//----------------------------------------------------------------------------//

void* exportF3d::creator()
{
  return new exportF3d();
}

//----------------------------------------------------------------------------//

exportF3d::exportF3d()
{
  m_start = 1;
  m_end = 1;
  m_verbose = false;
  
  m_hasDensity = false;
  m_hasTemperature = false;
  m_hasFuel = false;    
  m_hasColor = false;
  m_hasVelocity = false;
  m_hasPressure = false;
  m_hasTexture = false;
  m_hasFalloff = false;
  
  m_ignoreDensity = false;
  m_ignoreTemperature = false;
  m_ignoreFuel = false;    
  m_ignoreColor = false;
  m_ignoreVelocity = false;
  m_ignorePressure = true;
  m_ignoreTexture = false;
  m_ignoreFalloff = false;
  
  m_numOversample = 0;
  m_forNCache = false;
  
  m_sparse = false;
  m_half = false;
}

//----------------------------------------------------------------------------//

exportF3d::~exportF3d()
{
}

//----------------------------------------------------------------------------//

MSyntax exportF3d::newSyntax()
{
  MSyntax syntax;
  MStatus stat;
  stat = syntax.addFlag("-st",  "-startTime", MSyntax::kLong); ERRCHK;    
  stat = syntax.addFlag("-et",  "-endTime", MSyntax::kLong); ERRCHK;  
  stat = syntax.addFlag("-od",  "-outputDir", MSyntax::kString); ERRCHK; 
  stat = syntax.addFlag("-on",  "-outputName", MSyntax::kString); ERRCHK; 
  stat = syntax.addFlag("-id",  "-ignoreDensity", MSyntax::kNoArg); ERRCHK; 
  stat = syntax.addFlag("-itm", "-ignoreTemperature", MSyntax::kNoArg); ERRCHK; 
  stat = syntax.addFlag("-ifu", "-ignoreFuel", MSyntax::kNoArg); ERRCHK; 
  stat = syntax.addFlag("-iv",  "-ignoreVelocity", MSyntax::kNoArg); ERRCHK; 
  stat = syntax.addFlag("-ic",  "-ignoreColor", MSyntax::kNoArg); ERRCHK; 
  stat = syntax.addFlag("-ap",  "-addPressure", MSyntax::kNoArg); ERRCHK; 
  stat = syntax.addFlag("-itx", "-ignoreTexture", MSyntax::kNoArg); ERRCHK; 
  stat = syntax.addFlag("-ifa", "-ignoreFalloff", MSyntax::kNoArg); ERRCHK; 
  stat = syntax.addFlag("-ns",  "-numOversample", MSyntax::kLong); ERRCHK; 
  stat = syntax.addFlag("-fnc", "-forNCache", MSyntax::kNoArg); ERRCHK;
  stat = syntax.addFlag("-sp",  "-sparse", MSyntax::kBoolean); ERRCHK;
  stat = syntax.addFlag("-hf",  "-half", MSyntax::kBoolean); ERRCHK;
  stat = syntax.addFlag("-rc",  "-remapChannels", MSyntax::kString); ERRCHK;
  
  stat = syntax.addFlag("-d", "-debug");ERRCHK; 
  syntax.addFlag("-h", "-help");
  
  syntax.useSelectionAsDefault(true);
  stat = syntax.setObjectType(MSyntax::kSelectionList, 1);

  syntax.enableQuery(false);
  syntax.enableEdit(false);

  return syntax;
}

MStatus exportF3d::parseArgs( const MArgList &args )
{
  MStatus status;

  MArgDatabase argData(syntax(), args);
  
  if (argData.isFlagSet("-help"))
  {
    MString help = (
      "\nexportF3d is used to export 3D fluid data to either f3d\n"
      "Synopsis: exportF3d [flags] [fluidObject... ]\n"
      "Flags:\n"
      "    -od    -outputDir    string  Output directory\n"
      "    -on    -outputName   string  Output base name (can contain printf like frame pattern)\n"
      "    -st    -startTime    int     Start of simulation\n"
      "    -et    -endTime      int     End of simulation\n"
      "    -id    -ignoreDensity        Don't output 'density' field\n"
      "    -itm   -ignoreTemperature    Don't output 'temperature' field\n"
      "    -ifu   -ignoreFuel           Don't output 'fuel' field\n"
      "    -iv    -ignoreVelocity       Don't output 'velocity' field\n"
      "    -ic    -ignoreColor          Don't output 'color' field\n"
      "    -itx   -ignoreTexture        Don't output 'texture' field\n"
      "    -ifa   -ignoreFalloff        Don't output 'falloff' field\n"
      "    -ap    -addPressure          Output 'pressure' field\n"
      "    -ns    -numOversample int    Oversamples the solver with N sub frame(s) but\n"
      "                                   only writes out whole frame sim data\n"
      "    -sp    -sparse       bool    Outut sparse field or not (false by default)\n"    
      "    -hf    -half         bool    Outut half float fields or not (false by default)\n"
      "    -rc    -remapChannels string Remap fluid channels (',' separated list of oldName=newName)\n"
      "                                   (default is 'velocity=v_mac,color=Cd,texture=coord'\n"
      "    -fnc   -forNCache            Format file and field names to be nCache compatible\n"
      "                                   Overrides output filename to <outputDir>/<outputName>Frame<frame>.f3d\n"
      "                                   Note: default channel name remapping is ignored\n"
      "    -d     -debug\n"
      "    -h     -help\n"
      "Example:\n"
      "1- exportF3d -st 1 -et 100 -od \"/tmp/\" fluidObject \n\n"
      );
    
    MGlobal::displayInfo(help);
  }
  
  if (argData.isFlagSet("-debug"))
  {
    m_verbose = true;
  }
  
  if (!argData.isFlagSet("-outputDir") )
  {
    MGlobal::displayInfo("outputDir is required");
    return MS::kFailure;            
  }

  if (argData.isFlagSet("-startTime"))
  {
    status = argData.getFlagArgument("-startTime", 0, m_start);
  }
  
  if (argData.isFlagSet("-endTime"))
  {
    status = argData.getFlagArgument("-endTime", 0, m_end);
  }
  else 
  {
    m_end = m_start;
  }
    
  m_ignoreDensity = argData.isFlagSet("-ignoreDensity");
  m_ignoreTemperature = argData.isFlagSet("-ignoreTemperature");
  m_ignoreFuel = argData.isFlagSet("-ignoreFuel");
  m_ignoreVelocity = argData.isFlagSet("-ignoreVelocity");
  m_ignoreColor = argData.isFlagSet("-ignoreColor");
  m_ignorePressure = !argData.isFlagSet("-addPressure");
  m_ignoreTexture = argData.isFlagSet("-ignoreTexture");
  m_ignoreFalloff = argData.isFlagSet("-ignoreFalloff");
  m_forNCache = argData.isFlagSet("-forNCache");
  m_remapChannels.clear();
    
  if (argData.isFlagSet("-outputDir"))
  {
    status = argData.getFlagArgument("-outputDir", 0, m_outputDir);
  }
  
  if (argData.isFlagSet("-outputName"))
  {
    status = argData.getFlagArgument("-outputName", 0, m_outputName);
  }

  if (argData.isFlagSet("-numOversample"))
  {
    status = argData.getFlagArgument("-numOversample", 0, m_numOversample);
    if (m_numOversample < 0)
    { 
      m_numOversample = 0;
      MGlobal::displayWarning("numOversample can't be less than zero, setting it to 1");
    }
  }
  
  if (argData.isFlagSet("-sparse"))
  {
    argData.getFlagArgument("-sparse", 0, m_sparse);
  }
  
  if (argData.isFlagSet("-half"))
  {
    argData.getFlagArgument("-half", 0, m_half);
  }
  
  if (argData.isFlagSet("-remapChannels"))
  {
    MString sarg;
    
    argData.getFlagArgument("-remapChannels", 0, sarg);
    
    std::string rc = sarg.asChar();
    std::vector<std::string> items;
    
    Split(rc, ',', items, true);
    
    for (size_t i=0; i<items.size(); ++i)
    {
      size_t p = items[i].find('=');
      
      if (p != std::string::npos)
      {
        std::string from = items[i].substr(0, p);
        std::string to = items[i].substr(p + 1);
        
        StripWS(from);
        StripWS(to);
        
        if (from.length() > 0 && to.length() > 0)
        {
          m_remapChannels[from] = to;
        }
      }
    }
  }
  else
  {
    if (!m_forNCache)
    {
      m_remapChannels["velocity"] = "v_mac";
      m_remapChannels["color"] = "Cd";
      m_remapChannels["texture"] = "coord";
    }
  }
  
  status = argData.getObjects(m_slist);
  if (!status)
  {
    status.perror("no fluid object was selected");
    return status;
  }
  
  // Get the selected Fluid Systems
  if (m_slist.length() > 1)
  {
    MGlobal::displayWarning("[exportF3d]: only first fluid object is used to export");
  }
  
  return MS::kSuccess;
}

const std::string& exportF3d::remapChannel(const std::string &name) const
{
  std::map<std::string, std::string>::const_iterator rit = m_remapChannels.find(name);
  
  return (rit == m_remapChannels.end() ? name : rit->second);
}

MStatus exportF3d::doIt(const MArgList& args)
{
  MStatus status;
  MString result;
    
  status = parseArgs(args);
  
  if (!status)
  {
    status.perror("Parsing error");
    return status;
  }
  
  float currentFrame = MAnimControl::currentTime().value();

  MItSelectionList selListIter(m_slist, MFn::kFluid, &status);  
  
  for (; !selListIter.isDone(); selListIter.next())
  {
    MDagPath  dagPath;
    MObject   selectedObject;
    
    status = selListIter.getDependNode(selectedObject);
    status = selListIter.getDagPath(dagPath);
    
    // Create function set for the fluid
    MFnFluid fluidFn(dagPath, &status);
    if (status != MS::kSuccess)
    {
      continue;
    }
  
    if (m_verbose)
    {        
      std::cout << "------------------------------------------------------" << std::endl;
      std::cout << " Selected object: " << fluidFn.name().asChar() << std::endl;
      std::cout << " Selected object type: " << selectedObject.apiTypeStr() << std::endl;
      std::cout << " Remap channel names: " << std::endl;
      for (std::map<std::string, std::string>::const_iterator it = m_remapChannels.begin(); it != m_remapChannels.end(); ++it)
      {
        std::cout <<"   \"" << it->first << "\" => \"" << it->second << "\"" << std::endl;
      }
      std::cout << std::endl << std::endl;
    }      
       
    MFnFluid::FluidMethod method;
    MFnFluid::FluidGradient gradient;
    
    if (!m_ignoreDensity)
    {
      status = fluidFn.getDensityMode(method, gradient);
      m_ignoreDensity = (status.error() || (method != MFnFluid::kStaticGrid && method != MFnFluid::kDynamicGrid));
    }
    
    if (!m_ignoreTemperature)
    {
      status = fluidFn.getTemperatureMode(method, gradient);
      m_ignoreTemperature = (status.error() || (method != MFnFluid::kStaticGrid && method != MFnFluid::kDynamicGrid));
    }
    
    if (!m_ignoreFuel)
    {
      status = fluidFn.getFuelMode(method, gradient);
      m_ignoreFuel = (status.error() || (method != MFnFluid::kStaticGrid && method != MFnFluid::kDynamicGrid));
    }
    
    if (!m_ignoreVelocity)
    {
      status = fluidFn.getVelocityMode(method, gradient);
      m_ignoreVelocity = (status.error() || (method != MFnFluid::kStaticGrid && method != MFnFluid::kDynamicGrid));
    }

    if (!m_ignoreColor)
    {
      MFnFluid::ColorMethod colorMethod;
      status = fluidFn.getColorMode(colorMethod);
      m_ignoreColor = (status.error() || colorMethod == MFnFluid::kUseShadingColor);
    }

    if (!m_ignorePressure && m_ignoreVelocity)
    {
      // The pressure data only exists if the velocity method is kStaticGrid or kDynamicGrid
      m_ignorePressure = true;
    }
    
    if (!m_ignoreTexture)
    {
      MFnFluid::CoordinateMethod coordMode;
      status = fluidFn.getCoordinateMode(coordMode);
      m_ignoreTexture = (status.error() || coordMode != MFnFluid::kGrid);
    }

    if (!m_ignoreFalloff)
    {
      MFnFluid::FalloffMethod falloffMethod;
      status = fluidFn.getFalloffMode(falloffMethod);
      m_ignoreFalloff = (status.error() || falloffMethod != MFnFluid::kNoFalloffGrid);
    }
    
    if (m_verbose)
    {
      MGlobal::displayInfo("Ignore");
      MGlobal::displayInfo(MString("  density: ") + (m_ignoreDensity ? "true" : "false"));
      MGlobal::displayInfo(MString("  temperature: ") + (m_ignoreTemperature ? "true" : "false"));
      MGlobal::displayInfo(MString("  fuel: ") + (m_ignoreFuel ? "true" : "false"));
      MGlobal::displayInfo(MString("  velocity: ") + (m_ignoreVelocity ? "true" : "false"));
      MGlobal::displayInfo(MString("  color: ") + (m_ignoreColor ? "true" : "false"));
      MGlobal::displayInfo(MString("  pressure: ") + (m_ignorePressure ? "true" : "false"));
      MGlobal::displayInfo(MString("  texture: ") + (m_ignoreTexture ? "true" : "false"));
      MGlobal::displayInfo(MString("  falloff: ") + (m_ignoreFalloff ? "true" : "false"));
    }

    // Pre-process file base name
    MString fluidPath;
    std::string fluidName;
    bool hasFramePattern = false;
    char tmp[4096];
    size_t p = 0;
    
    // setup output base name
    if (m_outputName.length() > 0)
    {
      fluidName = m_outputName.asChar();
    }
    else
    {
      fluidName = fluidFn.name().asChar();
      
      p = fluidName.rfind(':');
      if (p != std::string::npos)
      {
        fluidName = fluidName.substr(p + 1);
      }
    }
    
    // remove .f3d extension if any
    p = fluidName.rfind('.');
    if (p != std::string::npos && fluidName.substr(p) == ".f3d")
    {
      fluidName = fluidName.substr(0, p);
    }
    
    // check for %d or %0xd pattern
    p = fluidName.rfind('%');
    if (p != std::string::npos)
    {
      size_t n = fluidName.length();
      size_t p1 = p + 1;
      
      while (p1 < n && fluidName[p1] != 'd')
      {
        if (fluidName[p1] < '0'  || fluidName[p1] > '9')
        {
          break;
        }
        ++p1;
      }
      
      hasFramePattern = (p1 < n && fluidName[p1] == 'd');
    }
    
    // remove frame pattern when making nCache compatible sequence
    if (m_forNCache && hasFramePattern)
    {
      // p still contains the position of the '%'
      size_t p1 = fluidName.find('d', p);
      
      fluidName = fluidName.substr(0, p) + fluidName.substr(p1 + 1);
      
      hasFramePattern = false;
    }
    
    // remove trailing '.' if any
    size_t n = fluidName.length() - 1;
    if (fluidName[n] == '.')
    {
      fluidName = fluidName.substr(0, n);
    }
    
    // setup file pattern
    std::string filePattern = fluidName;
    
    if (m_forNCache)
    {
      sprintf(tmp, "%sFrame%%d.f3d", fluidName.c_str());
      filePattern = tmp;
    }
    else if (!hasFramePattern)
    {
      sprintf(tmp, "%s.%%04d.f3d", fluidName.c_str());
      filePattern = tmp;
    }
    else
    {
      filePattern += ".f3d";
    }
    
    // Go through the selected frame range
    MComputation computation;
    computation.beginComputation();
    
    double step = 1.0 / (1.0 + m_numOversample);
    
    int numOversample = 0;
    double dt = 1.0;
    
    MTime t(double(m_start) - 1.0, MTime::uiUnit());
    
    for (int frame=m_start; frame<=m_end; ++frame)
    {
      if (computation.isInterruptRequested())
      {
        break;
      }
       
      for (int s=0; s<numOversample; ++s)
      {
        t.setValue(t.value() + dt);
        MAnimControl::setCurrentTime(t);
        // Do we need to force grid evaluation?
      }
      
      t.setValue(t.value() + dt);
      status = MAnimControl::setCurrentTime(t);
      // Do we need to force grid evaluation?
      
      fluidPath = m_outputDir + "/";
      
      sprintf(tmp, filePattern.c_str(), frame);
      
      fluidPath += tmp;
      
      MGlobal::displayInfo(MString("Writting: ") + fluidPath);
      
      if (m_sparse)
      {
        if (m_half)
        {
          setF3dField<SparseFieldh, SparseField3h, MACField3h>(fluidFn, fluidPath.asChar(), dagPath);
        }
        else
        {
          setF3dField<SparseFieldf, SparseField3f, MACField3f>(fluidFn, fluidPath.asChar(), dagPath);
        }
      }
      else
      {
        if (m_half)
        {
          setF3dField<DenseFieldh, DenseField3h, MACField3h>(fluidFn, fluidPath.asChar(), dagPath);
        }
        else
        {
          setF3dField<DenseFieldf, DenseField3f, MACField3f>(fluidFn, fluidPath.asChar(), dagPath);
        }
      }
      
      // past first frame, numOversample and dt match m_numOversample and step
      numOversample = m_numOversample;
      dt = step;
    }

    computation.endComputation(); 
    
    // generate .xml for nCache compatible output
    if (m_forNCache)
    {
      if (m_verbose)
      {
        MGlobal::displayInfo("Generating nCache XML");
        MGlobal::displayInfo(MString("  density: ") + (m_hasDensity ? "true" : "false"));
        MGlobal::displayInfo(MString("  temperature: ") + (m_hasTemperature ? "true" : "false"));
        MGlobal::displayInfo(MString("  fuel: ") + (m_hasFuel ? "true" : "false"));
        MGlobal::displayInfo(MString("  velocity: ") + (m_hasVelocity ? "true" : "false"));
        MGlobal::displayInfo(MString("  color: ") + (m_hasColor ? "true" : "false"));
        MGlobal::displayInfo(MString("  pressure: ") + (m_hasPressure ? "true" : "false"));
        MGlobal::displayInfo(MString("  texture: ") + (m_hasTexture ? "true" : "false"));
        MGlobal::displayInfo(MString("  falloff: ") + (m_hasFalloff ? "true" : "false"));
      }
      
      std::string xmlPath = m_outputDir.asChar();
      
      xmlPath += "/";
      xmlPath += fluidName;
      xmlPath += ".xml";
      
      FILE *f = fopen(xmlPath.c_str(), "w");
      
      MTime t(1, MTime::uiUnit());
      
      int TimePerFrame = int(floor(t.asUnits(MTime::k6000FPS)));
      
      t.setValue(m_start);  
      int StartTime = int(floor(t.asUnits(MTime::k6000FPS)));
      
      t.setValue(m_end);
      int EndTime = int(floor(t.asUnits(MTime::k6000FPS)));
      
      fprintf(f, "<?xml version=\"1.0\"?>\n");
      fprintf(f, "<Autodesk_Cache_File>\n");
      fprintf(f, "  <cacheType Type=\"OneFilePerFrame\" Format=\"f3d_%s_%s\"/>\n", (m_sparse ? "sparse" : "dense"), (m_half ? "half" : "float"));
      fprintf(f, "  <time Range=\"%d-%d\"/>\n", StartTime, EndTime);
      fprintf(f, "  <cacheTimePerFrame TimePerFrame=\"%d\"/>\n", TimePerFrame);
      fprintf(f, "  <cacheVersion Version=\"2.0\"/>\n");
      fprintf(f, "  <extra>f3d.file=%s</extra>\n", filePattern.c_str());
      fprintf(f, "  <Channels>\n");
      
      int d = 0;
      
      if (m_hasDensity)
      {
        fprintf(f, "    <channel%d ChannelName=\"%s_%s\" ChannelType=\"FloatArray\" ChannelInterpretation=\"density\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d++, dagPath.partialPathName().asChar(), remapChannel("density").c_str(), TimePerFrame, StartTime, EndTime);
      }
      if (m_hasVelocity)
      {
        fprintf(f, "    <channel%d ChannelName=\"%s_%s\" ChannelType=\"FloatArray\" ChannelInterpretation=\"velocity\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d++, dagPath.partialPathName().asChar(), remapChannel("velocity").c_str(), TimePerFrame, StartTime, EndTime);
      }
      if (m_hasTemperature)
      {
        fprintf(f, "    <channel%d ChannelName=\"%s_%s\" ChannelType=\"FloatArray\" ChannelInterpretation=\"temperature\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d++, dagPath.partialPathName().asChar(), remapChannel("temperature").c_str(), TimePerFrame, StartTime, EndTime);
      }
      if (m_hasFuel)
      {
        fprintf(f, "    <channel%d ChannelName=\"%s_%s\" ChannelType=\"FloatArray\" ChannelInterpretation=\"fuel\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d++, dagPath.partialPathName().asChar(), remapChannel("fuel").c_str(), TimePerFrame, StartTime, EndTime);
      }
      if (m_hasPressure)
      {
        fprintf(f, "    <channel%d ChannelName=\"%s_%s\" ChannelType=\"FloatArray\" ChannelInterpretation=\"pressure\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d++, dagPath.partialPathName().asChar(), remapChannel("pressure").c_str(), TimePerFrame, StartTime, EndTime);
      }
      if (m_hasFalloff)
      {
        fprintf(f, "    <channel%d ChannelName=\"%s_%s\" ChannelType=\"FloatArray\" ChannelInterpretation=\"falloff\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d++, dagPath.partialPathName().asChar(), remapChannel("falloff").c_str(), TimePerFrame, StartTime, EndTime);
      }
      if (m_hasColor)
      {
        fprintf(f, "    <channel%d ChannelName=\"%s_%s\" ChannelType=\"FloatArray\" ChannelInterpretation=\"color\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d++, dagPath.partialPathName().asChar(), remapChannel("color").c_str(), TimePerFrame, StartTime, EndTime);
      }
      if (m_hasTexture)
      {
        fprintf(f, "    <channel%d ChannelName=\"%s_%s\" ChannelType=\"FloatArray\" ChannelInterpretation=\"texture\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d++, dagPath.partialPathName().asChar(), remapChannel("texture").c_str(), TimePerFrame, StartTime, EndTime);
      }
      
      // Dummy fields (implicit in Field3D format)
      fprintf(f, "    <channel%d ChannelName=\"%s_resolution\" ChannelType=\"FloatArray\" ChannelInterpretation=\"resolution\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
              d++, dagPath.partialPathName().asChar(), TimePerFrame, StartTime, EndTime);
      
      fprintf(f, "    <channel%d ChannelName=\"%s_offset\" ChannelType=\"FloatArray\" ChannelInterpretation=\"offset\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
              d++, dagPath.partialPathName().asChar(), TimePerFrame, StartTime, EndTime);
      
      fprintf(f, "  </Channels>\n");
      fprintf(f, "</Autodesk_Cache_File>\n");
      fprintf(f, "\n");
      
      fclose(f);
    }
    
    break;
  }

  setResult(result);
  MAnimControl::setCurrentTime(currentFrame);
    
  return MS::kSuccess;
}



/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

template <typename FField, typename VField, typename MField>
void exportF3d::setF3dField(MFnFluid &fluidFn, const char *outputPath, 
                            const MDagPath &dagPath)
{
  try
  { 
    MStatus stat;

    unsigned int i, xres = 0, yres = 0, zres = 0;
    double xdim, ydim, zdim;
    
    // Get the resolution of the fluid container      
    stat = fluidFn.getResolution(xres, yres, zres);
    stat = fluidFn.getDimensions(xdim, ydim, zdim);
    
    V3d size(xdim, ydim, zdim);
    const V3i res(xres, yres, zres);
    
    //int psizeTot = fluidFn.gridSize();

    /// get the transform and rotation
    MObject parentObj = fluidFn.parent(0, &stat);
    if (stat != MS::kSuccess)
    {
      MGlobal::displayError("Can't find fluid's parent node");
      return;
    }
    
    MDagPath parentPath = dagPath;
    parentPath.pop();
    MTransformationMatrix tmatFn(dagPath.inclusiveMatrix());
    if (stat != MS::kSuccess)
    {
      MGlobal::displayError("Failed to get transformation matrix of fluid's parent node");
      return;
    }
    
    MFnTransform fnXform(parentPath, &stat);
    if (stat != MS::kSuccess)
    {
      MGlobal::displayError("Can't create a MFnTransform from fluid's parent node");
      return;
    }
    
    m_hasDensity = false;
    m_hasTemperature = false;
    m_hasFuel = false;
    m_hasColor = false;
    m_hasVelocity = false;
    m_hasPressure = false;
    m_hasTexture = false;
    m_hasFalloff = false;
    
    bool hasAnyField = false;

    float *density = 0;
    float *temp = 0;
    float *fuel = 0;
    float *pressure = 0;
    float *falloff = 0;
    float *r = 0;
    float *g = 0;
    float *b = 0;
    float *u = 0;
    float *v = 0;
    float *w = 0;
    float *Xvel = 0;
    float *Yvel = 0;
    float *Zvel = 0;
    
    if (!m_ignoreDensity)
    {
      density = fluidFn.density(&stat);
      m_hasDensity = !stat.error();
      hasAnyField = hasAnyField || m_hasDensity;
    }
    
    if (!m_ignoreTemperature)
    {
      temp = fluidFn.temperature(&stat);
      m_hasTemperature = !stat.error();
      hasAnyField = hasAnyField || m_hasTemperature;
    }
    
    if (!m_ignoreFuel)
    {
      fuel = fluidFn.fuel(&stat);
      m_hasFuel = !stat.error();
      hasAnyField = hasAnyField || m_hasFuel;
    }
    
    if (!m_ignorePressure)
    { 
      pressure = fluidFn.pressure(&stat);
      m_hasPressure = !stat.error();
      hasAnyField = hasAnyField || m_hasPressure;
    }
    
    if (!m_ignoreFalloff)
    {
      falloff = fluidFn.falloff(&stat);
      m_hasFalloff = !stat.error();
      hasAnyField = hasAnyField || m_hasFalloff;
    }

    if (!m_ignoreColor)
    {
      stat = fluidFn.getColors(r, b, g);
      m_hasColor = !stat.error();
      hasAnyField = hasAnyField || m_hasColor;
    }
      
    if (!m_ignoreTexture)
    {
      stat = fluidFn.getCoordinates(u, v, w);
      m_hasTexture = !stat.error();
      hasAnyField = hasAnyField || m_hasTexture;
    }

    /// velocity info
    if (!m_ignoreVelocity)
    {
      stat = fluidFn.getVelocity(Xvel, Yvel, Zvel);
      m_hasVelocity = !stat.error();
      hasAnyField = hasAnyField || m_hasVelocity;
    }
    

    if (!hasAnyField)
    {
      MGlobal::displayError("No fluid attributes found for writing, please check fluids settings");
      return;
    }
            
    /// Fields 
    typename FField::Ptr densityFld, tempFld, fuelFld, pressureFld, falloffFld;
    typename VField::Ptr CdFld, uvwFld;
    typename MField::Ptr vMac;
    
    typedef typename FField::value_type ScalarType;
    typedef typename VField::value_type VectorType;
    typedef typename VectorType::BaseType ComponentType;
    
    
    MPlug autoResizePlug = fluidFn.findPlug("autoResize", &stat); 
    bool autoResize;
    autoResizePlug.getValue(autoResize);

    // maya's fluid transformation
    V3d dynamicOffset(0);
    M44d localToWorld;
    MatrixFieldMapping::Ptr mapping(new MatrixFieldMapping());

    M44d fluid_mat(tmatFn.asMatrix().matrix);

    if (autoResize)
    {      
      fluidFn.findPlug("dofx").getValue(dynamicOffset[0]);
      fluidFn.findPlug("dofy").getValue(dynamicOffset[1]);
      fluidFn.findPlug("dofz").getValue(dynamicOffset[2]);
    }

    Box3i extents;
    extents.max = res - V3i(1);
    extents.min = V3i(0);
    mapping->setExtents(extents);
  
    localToWorld.setScale(size);
    localToWorld *= M44d().setTranslation( -0.5 * size );
    localToWorld *= M44d().setTranslation( dynamicOffset );
    localToWorld *= fluid_mat;
    
    Field3D::V3f Offset(Field3D::V3f(dynamicOffset.x, dynamicOffset.y, dynamicOffset.z));
    Field3D::V3f Dimension(xdim, ydim, zdim);
    
    mapping->setLocalToWorld(localToWorld);  
      
    if (m_hasDensity)
    {
      densityFld = new FField;
      densityFld->setSize(res);
      densityFld->setMapping(mapping);
      densityFld->metadata().setVecFloatMetadata("Offset", Offset);
      densityFld->metadata().setVecFloatMetadata("Dimension", Dimension);
    }
    
    if (m_hasFuel)
    {
      fuelFld = new FField;
      fuelFld->setSize(res); 
      fuelFld->setMapping(mapping);
      fuelFld->metadata().setVecFloatMetadata("Offset", Offset);
      fuelFld->metadata().setVecFloatMetadata("Dimension", Dimension);
    }
    
    if (m_hasTemperature)
    {
      tempFld = new FField;
      tempFld->setSize(res);
      tempFld->setMapping(mapping);
      tempFld->metadata().setVecFloatMetadata("Offset", Offset);
      tempFld->metadata().setVecFloatMetadata("Dimension", Dimension);
    }
    
    if (m_hasPressure)
    {
      pressureFld = new FField;
      pressureFld->setSize(res);
      pressureFld->setMapping(mapping);
      pressureFld->metadata().setVecFloatMetadata("Offset", Offset);
      pressureFld->metadata().setVecFloatMetadata("Dimension", Dimension);
    }
    
    if (m_hasFalloff)
    {
      falloffFld = new FField;
      falloffFld->setSize(res);
      falloffFld->setMapping(mapping);
      falloffFld->metadata().setVecFloatMetadata("Offset", Offset);
      falloffFld->metadata().setVecFloatMetadata("Dimension", Dimension);
    }
    
    if (m_hasVelocity)
    {
      vMac = new MField;
      vMac->setSize(res);
      vMac->setMapping(mapping);
      vMac->metadata().setVecFloatMetadata("Offset", Offset);
      vMac->metadata().setVecFloatMetadata("Dimension", Dimension);
    }
    
    if (m_hasColor)
    {
      CdFld = new VField;
      CdFld->setSize(res);
      CdFld->setMapping(mapping);
      CdFld->metadata().setVecFloatMetadata("Offset", Offset);
      CdFld->metadata().setVecFloatMetadata("Dimension", Dimension);
    } 
    
    if (m_hasTexture)
    {
      uvwFld = new VField;
      uvwFld->setSize(res);
      uvwFld->setMapping(mapping);
      uvwFld->metadata().setVecFloatMetadata("Offset", Offset);
      uvwFld->metadata().setVecFloatMetadata("Dimension", Dimension);
    } 
        
    size_t iX, iY, iZ;      
    for ( iZ = 0; iZ < zres; iZ++ ) 
    {
      for ( iX = 0; iX < xres; iX++ )
      {
        for ( iY = 0; iY < yres ; iY++ ) 
        {
          /// data is in x major but we are writting in z major order
          i = fluidFn.index(iX, iY, iZ);
            
          if (m_hasDensity) 
          {
            densityFld->fastLValue(iX, iY, iZ) = (ScalarType) density[i];
          }
          
          if (m_hasTemperature)
          {
            tempFld->fastLValue(iX, iY, iZ) = (ScalarType) temp[i];
          }
          
          if (m_hasFuel)
          {
            fuelFld->fastLValue(iX, iY, iZ) = (ScalarType) fuel[i];
          }
          
          if (m_hasPressure)
          {
            pressureFld->fastLValue(iX, iY, iZ) = (ScalarType) pressure[i];
          }
          
          if (m_hasFalloff)
          {
            falloffFld->fastLValue(iX, iY, iZ) = (ScalarType) falloff[i];
          }
          
          if (m_hasColor)
          {
            CdFld->fastLValue(iX, iY, iZ) = VectorType((ComponentType)r[i], (ComponentType)g[i], (ComponentType)b[i]);
          }
          
          if (m_hasTexture)
          {
            // can be a 2D fluid
            uvwFld->fastLValue(iX, iY, iZ) = VectorType((ComponentType)u[i], (ComponentType)v[i], (ComponentType)(w ? w[i] : 0.0f));
          }
        }
      }      
    }
    
    if (m_hasVelocity)
    {
      unsigned x, y, z;
      
      for (z=0; z<zres; ++z)
      {
        for (y=0; y<yres; ++y)
        {
          for (x=0; x<xres+1; ++x)
          {
            vMac->u(x, y, z) = (ComponentType) Xvel[fluidFn.index(x, y, z, xres+1, yres, zres)];
          }
        }
      }
      
      for (z=0; z<zres; ++z)
      {
        for (y=0; y<yres+1; ++y)
        {
          for (x=0; x<xres; ++x)
          {
            vMac->v(x, y, z) = (ComponentType) Yvel[fluidFn.index(x, y, z, xres, yres+1, zres)];
          }
        }
      }
      
      for (z=0; z<zres+1; ++z)
      {
        for (y=0; y<yres; ++y)
        {
          for (x=0; x<xres; ++x) 
          {
            vMac->w(x, y, z) = (ComponentType) (Zvel ? Zvel[fluidFn.index(x, y, z, xres, yres, zres+1)] : 0.0f);
          }
        }
      }
    } 
     
    Field3DOutputFile out;
    
    if (!out.create(outputPath))
    {
      MGlobal::displayError("Couldn't create file: "+ MString(outputPath));
      return;
    }
    
    std::string partition(fluidFn.name().asChar());
    
    // strip namespace from shape name to use as partition name
    size_t p = partition.rfind(':');
    if (p != std::string::npos)
    {
      partition = partition.substr(p + 1);
    }
    
    if (m_hasDensity)
    {
      out.writeScalarLayer<typename FField::value_type>(partition, remapChannel("density"), densityFld);
    }
    
    if (m_hasFuel)
    { 
      out.writeScalarLayer<typename FField::value_type>(partition, remapChannel("fuel"), fuelFld);
    }
    
    if (m_hasTemperature)
    {
      out.writeScalarLayer<typename FField::value_type>(partition, remapChannel("temperature"), tempFld);
    }
    
    if (m_hasColor)
    {
      out.writeVectorLayer<typename VField::value_type::BaseType>(partition, remapChannel("color"), CdFld);
    }
    
    if (m_hasVelocity)
    {
      out.writeVectorLayer<typename MField::real_t>(partition, remapChannel("velocity"), vMac);      
    }
    
    if (m_hasTexture)
    {
      out.writeVectorLayer<typename VField::value_type::BaseType>(partition, remapChannel("texture"), uvwFld);
    }
    
    if (m_hasFalloff)
    {
      out.writeScalarLayer<typename FField::value_type>(partition, remapChannel("falloff"), falloffFld);
    }
    
    if (m_hasPressure)
    {
      out.writeScalarLayer<typename FField::value_type>(partition, remapChannel("pressure"), pressureFld);
    }

    out.close(); 

  }
  catch (const std::exception &e)
  {

    MGlobal::displayError(MString(e.what()));
    return;
  }


}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
