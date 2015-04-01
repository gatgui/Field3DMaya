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
#include <Field3D/MACField.h>
#include <Field3D/Field3DFile.h>
#include <Field3D/InitIO.h>

#include <maya/MComputation.h>

#define ERRCHKR		\
	if ( MS::kSuccess != stat ) {	\
    std::cerr << stat.errorString().asChar(); \
		return stat;	\
	}

#define ERRCHK		\
	if ( MS::kSuccess != stat ) {	\
    std::cerr << stat.errorString().asChar(); \
	}


using namespace Field3D;


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
  m_density = true;
  m_temperature = true;
  m_fuel = true;    
  m_color = false;
  m_vel = false;
  m_pressure = false;
  m_texture = false;
  m_falloff = false;
  m_numOversample = 1;
  m_forNCache = false;
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
    stat = syntax.addFlag("-st",  "-startTime", MSyntax::kLong);ERRCHK;    
    stat = syntax.addFlag("-et",  "-endTime", MSyntax::kLong);ERRCHK;  
    stat = syntax.addFlag("-od",  "-outputDir", MSyntax::kString);ERRCHK; 
    stat = syntax.addFlag("-on",  "-outputName", MSyntax::kString);ERRCHK; 
    stat = syntax.addFlag("-av",  "-addVelocity", MSyntax::kNoArg);ERRCHK; 
    stat = syntax.addFlag("-ac",  "-addColor",MSyntax::kNoArg);ERRCHK; 
    stat = syntax.addFlag("-ap",  "-addPressure",MSyntax::kNoArg);ERRCHK; 
    stat = syntax.addFlag("-at",  "-addTexture",MSyntax::kNoArg);ERRCHK; 
    stat = syntax.addFlag("-af",  "-addFalloff",MSyntax::kNoArg);ERRCHK;
    stat = syntax.addFlag("-ns",  "-numOversample",MSyntax::kLong);ERRCHK; 
    stat = syntax.addFlag("-fnc", "-forNCache",MSyntax::kNoArg);ERRCHK;
    
    stat = syntax.addFlag("-d", "-debug");ERRCHK; 
    syntax.addFlag("-h", "-help");
    
    // DEFINE BEHAVIOUR OF COMMAND ARGUMENT THAT SPECIFIES THE MESH NODE:
    syntax.useSelectionAsDefault(true);
    stat = syntax.setObjectType(MSyntax::kSelectionList, 1);

    // MAKE COMMAND QUERYABLE AND NON-EDITABLE:
    syntax.enableQuery(false);
    syntax.enableEdit(false);


    return syntax;
}


MStatus exportF3d::parseArgs( const MArgList &args )
{
  MStatus status;

  MArgDatabase argData(syntax(), args);

  if (argData.isFlagSet("-debug"))
    m_verbose = true;

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
      "    -av    -addVelocity          Export velocity as v_mac\n"
      "    -ac    -addColor             Export color data\n"
      "    -ap    -addPressure          Export pressure data\n"
      "    -at    -addTexture           Export texture\n"
      "    -af    -addFalloff           Export falloff\n"
      "    -ns    -numOversample        Oversamples the solver at each sum frame but\n"
      "                                   only writes out whole frame sim data\n"
      "    -d     -debug\n"
      "    -h     -help\n"
      "Example:\n"
      "1- exportF3d -st 1 -et 100  -o \"/tmp/\"  fluidObject \n\n"
      );
    MGlobal::displayInfo(help);
    return MS::kFailure;

  }

  if (argData.isFlagSet("-startTime"))
  {
    status = argData.getFlagArgument("-startTime", 0, m_start);
  }
  if (argData.isFlagSet("-endTime"))
  {
    status = argData.getFlagArgument("-endTime", 0, m_end);
  }else 
    m_end = m_start;
    
  if (argData.isFlagSet("-addColor"))
    m_color = true;
  if (argData.isFlagSet("-addVelocity"))
    m_vel = true;
  if (argData.isFlagSet("-addPressure"))
    m_pressure = true;
  if (argData.isFlagSet("-addTexture"))
    m_texture = true;
    
  if (argData.isFlagSet("-addFalloff"))
    m_falloff = true;
    
  if (argData.isFlagSet("-outputDir"))
  {
    status = argData.getFlagArgument("-outputDir", 0, m_outputDir);
  }

  if (!argData.isFlagSet("-outputDir") )
  {
    MGlobal::displayInfo("outputDir is required");
    return MS::kFailure;            
  }
  
  if (argData.isFlagSet("-outputName"))
  {
    status = argData.getFlagArgument("-outputName", 0, m_outputName);
  }

  if (argData.isFlagSet("-numOversample"))
  {
    status = argData.getFlagArgument("-numOversample", 0, m_numOversample);
    if (m_numOversample < 1) { 
      m_numOversample = 1;
      MGlobal::displayWarning("numOversample can't be less than one, setting it to 1");
    }
  }
  
  if (argData.isFlagSet("-forNCache"))
    m_forNCache = true;

  status = argData.getObjects(m_slist);
  if (!status)
  {
    status.perror("no fluid object was selected");
    return status;
  }
  // Get the selected Fluid Systems
  if (m_slist.length() > 1) {
    MGlobal::displayWarning("[exportF3d]: only first fluid object is used to export");
  }



  return MS::kSuccess;
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
    MDagPath 	dagPath;
    MObject 	selectedObject;
    
    status = selListIter.getDependNode(selectedObject);
    status = selListIter.getDagPath(dagPath);
    // Create function set for the fluid
    MFnFluid fluidFn(dagPath, &status);
    if (status != MS::kSuccess)
      continue;
  
    if (m_verbose)
    {        
      std::cout << "------------------------------------------------------" << std::endl;
      std::cout << " Selected object: " << fluidFn.name().asChar() << std::endl;
      std::cout << " Selected object type: " << selectedObject.apiTypeStr() << std::endl;
      std::cout << std::endl << std::endl;
    }      
       
    MFnFluid::FluidMethod method;
    MFnFluid::FluidGradient gradient;
      
    status  = fluidFn.getDensityMode(method, gradient);
    if(method != MFnFluid::kStaticGrid && method != MFnFluid::kDynamicGrid) {
      m_density = false;
    }
    status  = fluidFn.getTemperatureMode(method, gradient);
    if(method != MFnFluid::kStaticGrid && method != MFnFluid::kDynamicGrid) {
      m_temperature = false;
    }
    status  = fluidFn.getFuelMode(method, gradient);
    if(method != MFnFluid::kStaticGrid && method != MFnFluid::kDynamicGrid) {      
      m_fuel = false;
    }

    status  = fluidFn.getVelocityMode(method, gradient);
    if(method != MFnFluid::kStaticGrid && method != MFnFluid::kDynamicGrid) {
      m_vel = false;
    }

    if (m_color) {
      MFnFluid::ColorMethod colorMethod;
      fluidFn.getColorMode(colorMethod);
      if(colorMethod == MFnFluid::kUseShadingColor) {
        m_color = false;
      }
    }

    if (!m_vel) {
      // Note that the pressure data only exists if the velocity method
      // is kStaticGrid or kDynamicGrid
      m_pressure = false;
    }

    if (m_falloff)
    {
      MFnFluid::FalloffMethod falloffMethod;
      status  = fluidFn.getFalloffMode(falloffMethod);
      if(falloffMethod != MFnFluid::kNoFalloffGrid ) {
        m_falloff = false;
      }
    }

    // Go through the selected frame range
    MComputation computation;
    computation.beginComputation();
    // char fluidPath[1024];
    MString fluidPath;
    std::string fluidName;
    char tmp[4096];
    
    if (m_outputName.length() > 0)
    {
      fluidName = m_outputName.asChar();
    }
    else
    {
      fluidName = fluidFn.name().asChar();
    }
    
    size_t p = 0;
    
    bool done = false;
    while (!done)
    {
      p = fluidName.find_first_of(":|", p);
      if (p != std::string::npos)
      {
        fluidName[p] = '_';
      }
      else
      {
        done = true;
      }
    }
    
    // remove extension if any
    p = fluidName.rfind('.');
    if (p != std::string::npos && fluidName.substr(p) == ".f3d")
    {
      fluidName = fluidName.substr(0, p);
    }

    for(int frame=m_start ; frame <= m_end; ++frame)
    {
      int numOversample = m_numOversample;
      if (frame == m_start)
        numOversample = 1;
      
      float dt = 1.0 / double(numOversample);

      for ( int s=numOversample-1 ; s >= 0 ; --s)
      {

        if  (computation.isInterruptRequested()) break ;
        float time = frame - s*dt;
          
        status = MAnimControl::setCurrentTime(time);
        //MPlug plugGrid = fluidFn.findPlug( "outGrid",true,&status);
        MGlobal::displayInfo(MString("Setting Current frame ")+time);

      }
      
      fluidPath = m_outputDir + "/";
      
      sprintf(tmp, fluidName.c_str(), frame);
      if (tmp == fluidName)
      {
        sprintf(tmp, "%s.%04d", fluidName.c_str(), frame);
      }
      strcat(tmp, ".f3d");
      
      fluidPath += tmp;
      
      MGlobal::displayInfo(MString("Writting: ")+fluidPath);

      setF3dField(fluidFn, fluidPath.asChar(), dagPath);
   
    }

    computation.endComputation();	
    
    if (m_forNCache) {
      // generate .xml
      
      std::string xmlPath = ""; // TODO
      
      FILE *f = fopen(xmlPath.c_str(), "w");
      
      MTime t(1, MTime::uiUnit());
      
      int TimePerFrame = int(floor(t.asUnits(MTime::k6000FPS)));
      
      t.setValue(m_start);  
      int StartTime = int(floor(t.asUnits(MTime::k6000FPS)));
      
      t.setValue(m_end);
      int EndTime = int(floor(t.asUnits(MTime::k6000FPS)));
      
      fprintf(f, "<?xml version=\"1.0\"?>\n");
      fprintf(f, "<Autodesk_Cache_File>\n");
      fprintf(f, "  <cacheVersion Version=\"2.0\"/>");
      fprintf(f, "  <cacheType Type=\"OneFilePerFrame\" Format=\"f3s_dense_float\"/>\n");
      fprintf(f, "  <cacheTimePerFrame TimePerFrame=\"%d\"/>\n", TimePerFrame);
      fprintf(f, "  <time Range=\"%d-%d\"/>\n", StartTime, EndTime);
      fprintf(f, "  <Channels>\n");
      
      std::string name = fluidFn.name().asChar();
      
      p = name.rfind(':');
      if (p != std::string::npos) {
        name = name.substr(p+1);
      }
      
      int d = 0;
      
      if (m_density) {
        fprintf(f, "    <channel%d ChannelName=\"%s_density\" ChannelType=\"FloatArray\" ChannelInterpretation=\"density\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d, name.c_str(), TimePerFrame, StartTime, EndTime);
        ++d;
      }
      if (m_temperature) {
        fprintf(f, "    <channel%d ChannelName=\"%s_temperature\" ChannelType=\"FloatArray\" ChannelInterpretation=\"temperature\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d, name.c_str(), TimePerFrame, StartTime, EndTime);
        ++d;
      }
      if (m_fuel) {
        fprintf(f, "    <channel%d ChannelName=\"%s_fuel\" ChannelType=\"FloatArray\" ChannelInterpretation=\"fuel\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d, name.c_str(), TimePerFrame, StartTime, EndTime);
        ++d;
      }
      if (m_pressure) {
        fprintf(f, "    <channel%d ChannelName=\"%s_pressure\" ChannelType=\"FloatArray\" ChannelInterpretation=\"pressure\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d, name.c_str(), TimePerFrame, StartTime, EndTime);
        ++d;
      }
      if (m_falloff) {
        fprintf(f, "    <channel%d ChannelName=\"%s_falloff\" ChannelType=\"FloatArray\" ChannelInterpretation=\"falloff\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d, name.c_str(), TimePerFrame, StartTime, EndTime);
        ++d;
      }
      if (m_vel) {
        fprintf(f, "    <channel%d ChannelName=\"%s_velocity\" ChannelType=\"FloatArray\" ChannelInterpretation=\"velocity\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d, name.c_str(), TimePerFrame, StartTime, EndTime);
        ++d;
      }
      if (m_color) {
        fprintf(f, "    <channel%d ChannelName=\"%s_color\" ChannelType=\"FloatArray\" ChannelInterpretation=\"color\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d, name.c_str(), TimePerFrame, StartTime, EndTime);
        ++d;
      }
      if (m_texture) {
        fprintf(f, "    <channel%d ChannelName=\"%s_coord\" ChannelType=\"FloatArray\" ChannelInterpretation=\"coord\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d, name.c_str(), TimePerFrame, StartTime, EndTime);
        ++d;
      }
      
      // Dummy fields (implicit in Field3D format)
      fprintf(f, "    <channel%d ChannelName=\"%s_resolution\" ChannelType=\"FloatArray\" ChannelInterpretation=\"resolution\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
              d, name.c_str(), TimePerFrame, StartTime, EndTime);
      ++d;
      
      fprintf(f, "    <channel%d ChannelName=\"%s_offset\" ChannelType=\"FloatArray\" ChannelInterpretation=\"offset\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
              d, name.c_str(), TimePerFrame, StartTime, EndTime);
      ++d;
      
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

void exportF3d::setF3dField(MFnFluid &fluidFn, const char *outputPath, 
                            const MDagPath &dagPath)
{
    
  try { 
      
    MStatus stat;

    unsigned int i, xres = 0, yres = 0, zres = 0;
    double xdim,ydim,zdim;
    // Get the resolution of the fluid container      
    stat = fluidFn.getResolution(xres, yres, zres);
    stat = fluidFn.getDimensions(xdim, ydim, zdim);
    V3d size(xdim,ydim,zdim);
    const V3i res(xres, yres, zres);
    int psizeTot  = fluidFn.gridSize();

    /// get the transform and rotation
    MObject parentObj = fluidFn.parent(0, &stat);
    if (stat != MS::kSuccess) {

      MGlobal::displayError("Can't find fluid's parent node");
      return;
    }
    MDagPath parentPath = dagPath;
    parentPath.pop();
    MTransformationMatrix tmatFn(dagPath.inclusiveMatrix());
    if (stat != MS::kSuccess) {

      MGlobal::displayError("Failed to get transformation matrix of fluid's parent node");
      return;
    }


    MFnTransform fnXform(parentPath, &stat);
    if (stat != MS::kSuccess) {

      MGlobal::displayError("Can't create a MFnTransform from fluid's parent node");
      return;
    }
          

    if (m_verbose)
    {
      fprintf(stderr, "cellnum: %dx%dx%d = %d\n",  
              xres, yres, zres,psizeTot);
    }

    float *density(NULL), *temp(NULL), *fuel(NULL);
    float *pressure(NULL), *falloff(NULL);
      
    density = fluidFn.density( &stat );
    if ( stat.error() ) m_density = false;

    temp    = fluidFn.temperature( &stat );
    if ( stat.error() ) m_temperature = false;
      
    fuel    = fluidFn.fuel( &stat );
    if ( stat.error() ) m_fuel = false;    
      
    pressure= fluidFn.pressure( &stat );
    if ( stat.error() ) m_pressure = false;

    falloff = fluidFn.falloff( &stat );
    if ( stat.error() ) m_falloff = false;

    float *r,*g,*b;
    if (m_color) {
      stat = fluidFn.getColors(r,b,g);
      if ( stat.error() ) m_color = false;
    }else
      m_color = false;
      
    float *u,*v,*w;
    if (m_texture) {
      stat = fluidFn.getCoordinates(u,v,w);
      if ( stat.error() ) m_texture = false;
    }else
      m_texture = false;

    /// velocity info
    float *Xvel(NULL),*Yvel(NULL), *Zvel(NULL);  
    if (m_vel) { 
      stat = fluidFn.getVelocity( Xvel,Yvel,Zvel );
      if ( stat.error() ) m_vel = false;
    }
    

    if (m_density == false && m_temperature==false && m_fuel==false &&
        m_pressure==false && m_falloff==false && m_vel == false && 
        m_color == false && m_texture==false)
    {
      MGlobal::displayError("No fluid attributes found for writing, please check fluids settings");
      return;
    }
            
    /// Fields 
    DenseFieldf::Ptr densityFld, tempFld, fuelFld, pressureFld, falloffFld;
    DenseField3f::Ptr CdFld, uvwFld;
    MACField3f::Ptr vMac;

    MPlug autoResizePlug = fluidFn.findPlug("autoResize", &stat); 
    bool autoResize;
    autoResizePlug.getValue(autoResize);

    // maya's fluid transformation
    V3d dynamicOffset(0);
    M44d localToWorld;
    MatrixFieldMapping::Ptr mapping(new MatrixFieldMapping());

    M44d fluid_mat(tmatFn.asMatrix().matrix);

    if(autoResize) {      
      fluidFn.findPlug("dofx").getValue(dynamicOffset[0]);
      fluidFn.findPlug("dofy").getValue(dynamicOffset[1]);
      fluidFn.findPlug("dofz").getValue(dynamicOffset[2]);
    }

    Box3i extents;
    extents.max = res - V3i(1);
    extents.min = V3i(0);
    mapping->setExtents(extents);
  
    localToWorld.setScale(size);
    localToWorld *= M44d().setTranslation( -(size*0.5) );
    localToWorld *= M44d().setTranslation( dynamicOffset );
    localToWorld *= fluid_mat;
    
    mapping->setLocalToWorld(localToWorld);  
      
    if (m_density){
      densityFld = new DenseFieldf;
      densityFld->setSize(res);
      densityFld->setMapping(mapping);
    }
    if (m_fuel){
      fuelFld = new DenseFieldf;
      fuelFld->setSize(res); 
      fuelFld->setMapping(mapping);
    }
    if (m_temperature){
      tempFld = new DenseFieldf;
      tempFld->setSize(res);
      tempFld->setMapping(mapping);
    }
    if (m_pressure){
      pressureFld = new DenseFieldf;
      pressureFld->setSize(res);
      pressureFld->setMapping(mapping);
    }
    if (m_falloff){
      falloffFld = new DenseFieldf;
      falloffFld->setSize(res);
      falloffFld->setMapping(mapping);
    }
    if (m_vel){
      vMac = new MACField3f;
      vMac->setSize(res);
      vMac->setMapping(mapping);
    } 
    if (m_color){
      CdFld = new DenseField3f;
      CdFld->setSize(res);
      CdFld->setMapping(mapping);
    } 
    if (m_texture){
      uvwFld = new DenseField3f;
      uvwFld->setSize(res);
      uvwFld->setMapping(mapping);
    } 
        
    size_t iX, iY, iZ;      
    for( iZ = 0; iZ < zres; iZ++ ) 
    {
      for( iX = 0; iX < xres; iX++ )
      {
        for( iY = 0; iY < yres ; iY++ ) 
        {
    
          /// data is in x major but we are writting in z major order
          i = fluidFn.index( iX, iY,  iZ);
            
          if ( m_density ) 
            densityFld->lvalue(iX, iY, iZ) = density[i];            
          if ( m_temperature ) 
            tempFld->lvalue(iX, iY, iZ) = temp[i];
          if ( m_fuel )   
            fuelFld->lvalue(iX, iY, iZ) = fuel[i];
          if ( m_pressure )   
            pressureFld->lvalue(iX, iY, iZ) = pressure[i];
          if ( m_falloff )   
            falloffFld->lvalue(iX, iY, iZ) = falloff[i];
          if (m_color)
            CdFld->lvalue(iX, iY, iZ) = V3f(r[i], g[i], b[i]);
          if (m_texture)
            uvwFld->lvalue(iX, iY, iZ) = V3f(u[i], v[i], w[i]);
        }
      }      
    }

      
    if (m_vel) {
      unsigned x,y,z;
      for(z=0;z<zres;++z) for(y=0;y<yres;++y) for(x=0;x<xres+1;++x) {
            vMac->u(x,y,z) = *Xvel++;
          }
        
      for(z=0;z<zres;++z) for(y=0;y<yres+1;++y) for(x=0;x<xres;++x) {
            vMac->v(x,y,z) = *Yvel++;
          }
        
      for(z=0;z<zres+1;++z) for(y=0;y<yres;++y) for(x=0;x<xres;++x) {
            vMac->w(x,y,z) = *Zvel++;
          }                        
    } 
     
    Field3DOutputFile out;
    if (!out.create(outputPath)) {
      MGlobal::displayError("Couldn't create file: "+ MString(outputPath));
      return;
    }

    std::string fieldname("maya");

    if (m_density){
        out.writeScalarLayer<float>(fieldname, "density", densityFld);
    }
    if (m_fuel) { 
        out.writeScalarLayer<float>(fieldname,"fuel", fuelFld);
    }
    if (m_temperature){
        out.writeScalarLayer<float>(fieldname,"temperature", tempFld);
    }
    if (m_color) {
        out.writeVectorLayer<float>(fieldname, (m_forNCache ? "color" : "Cd"), CdFld);
    }
    if (m_vel) {
      out.writeVectorLayer<float>(fieldname, (m_forNCache ? "velocity" : "v_mac"), vMac);      
    }
    if (m_texture) {
      out.writeVectorLayer<float>(fieldname, "coord", uvwFld);
    }
    if (m_falloff) {
      out.writeScalarLayer<float>(fieldname, "falloff", falloffFld);
    }
    if (m_pressure) {
      out.writeScalarLayer<float>(fieldname, "pressure", pressureFld);
    }

    out.close(); 

  }
  catch(const std::exception &e)
  {

    MGlobal::displayError( MString(e.what()) );
    return;
  }


}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
