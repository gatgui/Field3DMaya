#include "field3D_Import.h"
#include <maya/MDagModifier.h>
#include <maya/MNamespace.h>
#include <maya/MGlobal.h>
#include <maya/MTime.h>
#include <maya/MSyntax.h>
#include <maya/MArgParser.h>
#include <maya/MSelectionList.h>
#include <maya/MFnTransform.h>
#include <maya/MMatrix.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>
#include <maya/MPoint.h>
#include <maya/MPlug.h>
#include <Field3D/Field3DFile.h>
#include <Field3D/EmptyField.h>
#include <Field3D/FieldMapping.h>
#include <vector>
#include <set>
#include <cstdio>
#include <cstdlib>
#include <cstring>

//----------------------------------------------------------------------------//

extern void StripWS(std::string &s);
extern size_t Split(const std::string &s, char sep, std::vector<std::string> &splits, bool strip=true);

extern void ToPrintfPattern(std::string &name);
extern size_t GetFileList(const std::string &filePattern, std::vector<std::string> &files, int *start=0, int *end=0);

static std::string NormalizePath(const std::string &p, bool doCase=false)
{
  std::string out = p;
  
  #ifdef _WIN32
  for (size_t i=0; i<out.length(); ++i)
  {
    if (out[i] == '\\')
    {
      out[i] = '/';
    }
    else if (doCase && (out[i] >= 'A' && out[i] <= 'Z'))
    {
      out[i] = 'a' + (out[i] - 'A');
    }
  }
  #endif
  
  return out;
}

static bool SamePath(const std::string &p0, const std::string &p1)
{
  return (NormalizePath(p0, true) == NormalizePath(p1, true));
}

//----------------------------------------------------------------------------//

struct FieldInfo
{
  Field3D::FieldRes::Ptr field;
  std::string name;
};

//----------------------------------------------------------------------------//

void* importF3d::creator()
{
  return new importF3d();
}

MSyntax importF3d::newSyntax()
{
  MSyntax syntax;
  
  syntax.addFlag("-f", "-file", MSyntax::kString);
  syntax.addFlag("-ns", "-namespace", MSyntax::kString);
  syntax.addFlag("-xf", "-xmlFile", MSyntax::kString);
  syntax.addFlag("-xo", "-xmlOnly", MSyntax::kNoArg);
  syntax.addFlag("-rc", "-remapChannels", MSyntax::kString);
  syntax.addFlag("-v", "-verbose", MSyntax::kNoArg);
  syntax.addFlag("-rs", "-recacheSparse", MSyntax::kBoolean);
  syntax.addFlag("-rf", "-recacheFormat", MSyntax::kString);
  
  syntax.setMinObjects(0);
  syntax.setMaxObjects(0);
  
  return syntax;
}

importF3d::importF3d()
{
}

importF3d::~importF3d()
{
}
  
MStatus importF3d::doIt(const MArgList &argList)
{
  MStatus stat;
  MSyntax syntax = newSyntax();
  MArgParser args(syntax, argList);
  
  if (!args.isFlagSet("-file"))
  {
    MGlobal::displayError("importF3d: missing -f/-file flag");
    return MS::kFailure;
  }
  
  bool verbose = args.isFlagSet("-verbose");
  
  MString sarg;
  
  stat = args.getFlagArgument("-file", 0, sarg);
  if (stat != MS::kSuccess)
  {
    stat.perror("importF3d");
    return stat;
  }
  
  std::string pat = sarg.asChar();
  
  ToPrintfPattern(pat);
  
  if (verbose)
  {
    MGlobal::displayInfo("importF3d: File pattern \"" + sarg + "\" -> \"" + MString(pat.c_str()) + "\"");
  }
  
  std::vector<std::string> files;
  int startFrame = -1;
  int endFrame = -1;
  
  if (GetFileList(pat, files, &startFrame, &endFrame) == 0)
  {
    MGlobal::displayError("importF3d: No file matching " + sarg);
    return MS::kFailure;
  }
  
  if (verbose)
  {
    MGlobal::displayInfo("importF3d: Found file(s)");
    for (size_t i=0; i<files.size(); ++i)
    {
      MGlobal::displayInfo("  " + MString(files[i].c_str()));
    }
  }
  
  if (args.isFlagSet("-remapChannels"))
  {
    MString sarg;
    
    args.getFlagArgument("-remapChannels", 0, sarg);
    
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
          if (verbose)
          {
            MGlobal::displayInfo(MString("importF3d: Remap \"") + from.c_str() + "\" to \"" + to.c_str() + "\"");
          }
          m_remapChannels[from] = to;
        }
      }
    }
  }
  
  std::string ns = "";
  
  if (args.isFlagSet("-namespace"))
  {
    MString tmp;
    
    stat = args.getFlagArgument("-namespace", 0, tmp);
    if (stat != MS::kSuccess)
    {
      stat.perror("importF3d");
      return stat;
    }
    
    ns = tmp.asChar();
    if (ns.length() > 0 && ns[ns.length()-1] == ':')
    {
      ns = ns.substr(0, ns.length()-1);
    }
  }
  
  std::string cacheDir = ".";
  size_t ls = pat.find_last_of("\\/");
  if (ls != std::string::npos)
  {
    cacheDir = pat.substr(0, ls);
  }
  
  bool xmlOnly = args.isFlagSet("-xmlOnly");
  std::string xmlDir = cacheDir;
  std::string xmlBase = "";
  std::string xmlExt = ".xml";
  
  if (args.isFlagSet("-xmlFile"))
  {
    MString xmlFile;
    
    stat = args.getFlagArgument("-xmlFile", 0, xmlFile);
    
    if (stat != MS::kSuccess)
    {
      stat.perror("importF3d");
      return stat;
    }
    
    xmlDir = xmlFile.asChar();
    
    size_t p0 = xmlDir.find_last_of("\\/");
    
    if (p0 == std::string::npos)
    {
      xmlBase = xmlDir;
      xmlDir = ".";
    }
    else
    {
      xmlBase = xmlDir.substr(p0 + 1);
      xmlDir = xmlDir.substr(0, p0);
    }
    
    p0 = xmlBase.find('.');
    
    if (p0 != std::string::npos)
    {
      size_t p1 = xmlBase.rfind('.');
      if (p1 > p0)
      {
        xmlExt = xmlBase.substr(p0, p1 - p0) + ".xml";
        xmlBase = xmlBase.substr(0, p0);
      }
      else
      {
        xmlBase = xmlBase.substr(0, p0);
      }
    }
    
    if (xmlBase.length() > 0)
    {
      xmlBase += "_";
    }
    
    if (verbose)
    {
      MGlobal::displayInfo(MString("importF3d: xmlDir = ") + xmlDir.c_str());
      MGlobal::displayInfo(MString("importF3d: xmlBase = ") + xmlBase.c_str());
      MGlobal::displayInfo(MString("importF3d: xmlExt = ") + xmlExt.c_str());
    }
  }
  
  Field3D::Field3DInputFile f3d;
  
  if (f3d.open(files[0]))
  {
    std::set<std::string> scalarChannelNames;
    std::set<std::string> vectorChannelNames;
    
    scalarChannelNames.insert("density");
    scalarChannelNames.insert("temperature");
    scalarChannelNames.insert("fuel");
    scalarChannelNames.insert("falloff");
    scalarChannelNames.insert("pressure");
    
    vectorChannelNames.insert("velocity");
    vectorChannelNames.insert("texture");
    vectorChannelNames.insert("color");
    
    // read fields from first file in sequence only?
    std::vector<std::string> partitions;
    
    f3d.getPartitionNames(partitions);
    
    for (size_t i=0; i<partitions.size(); ++i)
    {
      bool dontWrite = false;
      
      std::string &partition = partitions[i];
      
      MSelectionList sl;
      MFnFluid fluid;
      MFnTransform fluidTr;
      
      std::string xmlFile = xmlDir + "/" + xmlBase + partition + xmlExt;
      
      FILE *f = fopen(xmlFile.c_str(), "r");
      if (f)
      {
        fclose(f);
        dontWrite = true;
      }
      else
      {
        f = fopen(xmlFile.c_str(), "w");
        if (!f)
        {
          MGlobal::displayWarning(MString("importF3d: Cannot open file ") + xmlFile.c_str() + " for writing");
          continue;
        }
      }
      
      MGlobal::displayInfo(MString("importF3d: Create XML \"") + xmlFile.c_str() + "\"");
      
      MString shapeName = (ns + partition).c_str();
      
      if (!xmlOnly)
      {
        MString trName;
        
        if (!MNamespace::namespaceExists(ns.c_str()))
        {
          MNamespace::addNamespace(ns.substr(0, ns.length()-1).c_str());
        }
        
        if (sl.add(shapeName) == MS::kSuccess)
        {
          MDagPath path;
          
          sl.getDagPath(0, path);
          
          if (fluid.setObject(path) != MS::kSuccess)
          {
            MGlobal::displayWarning("importF3d: Target object is not a fluid container");
            continue;
          }
          else
          {
            path.pop();
            fluidTr.setObject(path);
          }
        }
        else
        {
          MDagModifier dagmod;
          
          MObject trObj = dagmod.createNode("fluidShape");
          
          if (dagmod.doIt() != MS::kSuccess)
          {
            MGlobal::displayWarning("importF3d: Could not create fluid container");
            continue;
          }
          
          fluidTr.setObject(trObj);
          
          MObject shObj = fluidTr.child(0);
          fluid.setObject(shObj);
          
          size_t p = partition.find("Shape");
          if (p != std::string::npos)
          {
            trName = (ns + partition.substr(0, p)).c_str();
          }
          else
          {
            trName = (ns + partition + "Xform").c_str();
          }
          
          dagmod.renameNode(trObj, trName);
          dagmod.renameNode(shObj, shapeName);
          dagmod.doIt();
        }
      }
      
      // -remapChannels "coord=texture,v_mac=velocity"
        
      std::vector<std::string> layerNames;
      std::map<std::string, FieldInfo> channels;
      std::map<std::string, FieldInfo>::iterator channelIt;
      
      f3d.getScalarLayerNames(layerNames, partition);
      for (size_t i=0; i<layerNames.size(); ++i)
      {
        FieldInfo info;
        
        info.name = layerNames[i];
        
        std::string channel = remapChannel(info.name);
        
        if (scalarChannelNames.find(channel) == scalarChannelNames.end())
        {
          MGlobal::displayWarning(MString("importF3d: Ignore layer \"") + layerNames[i].c_str() + "\" (not a valid maya fluid scalar channel, use -remapChannels if necessary)");
          continue;
        }
        
        channelIt = channels.find(channel);
        
        if (channelIt == channels.end())
        {
          // type doesn't really matter
          Field3D::EmptyField<Field3D::half>::Vec fields = f3d.readProxyLayer<Field3D::half>(partition, info.name, false);
          if (fields.empty())
          {
            continue;
          }
          
          info.field = fields[0];
          
          channels[channel] = info;
        }
        else if (channelIt->second.name != info.name)
        {
          MGlobal::displayWarning(MString("importF3d: Conflicting layer for channel \"") + channel.c_str() + "\"");
          continue;
        }
      }
      
      layerNames.clear();
      f3d.getVectorLayerNames(layerNames, partition);
      for (size_t i=0; i<layerNames.size(); ++i)
      {
        FieldInfo info;
        
        info.name = layerNames[i];
        
        std::string channel = remapChannel(info.name);
        
        if (vectorChannelNames.find(channel) == vectorChannelNames.end())
        {
          MGlobal::displayWarning(MString("importF3d: Ignore layer \"") + layerNames[i].c_str() + "\" (not a valid maya fluid vector channel, use -remapChannels if necessary)");
          continue;
        }
        
        channelIt = channels.find(channel);
        
        if (channelIt == channels.end())
        {
          // type doesn't matter for EmptyField, use any
          Field3D::EmptyField<Field3D::V3h>::Vec fields = f3d.readProxyLayer<Field3D::V3h>(partition, info.name, true);
          if (fields.empty())
          {
            continue;
          }
          
          info.field = fields[0];
          
          channels[channel] = info;
        }
        else if (channelIt->second.name != info.name)
        {
          MGlobal::displayWarning(MString("importF3d: Conflicting layer for channel \"") + channel.c_str() + "\"");
          continue;
        }
      }
        
      if (!dontWrite)
      {
        bool sparse = false;
        MString format = "half";
        
        if (args.isFlagSet("-recacheSparse"))
        {
          args.getFlagArgument("-recacheSparse", 0, sparse);
        }
        
        if (args.isFlagSet("-recacheFormat"))
        {
          args.getFlagArgument("-recacheFormat", 0, format);
        }
        
        MTime t(1, MTime::uiUnit());
        
        int timePerFrame = int(floor(t.asUnits(MTime::k6000FPS)));
        
        t.setValue(startFrame);  
        int startTime = int(floor(t.asUnits(MTime::k6000FPS)));
        
        t.setValue(endFrame);
        int endTime = int(floor(t.asUnits(MTime::k6000FPS)));
        
        fprintf(f, "<?xml version=\"1.0\"?>\n");
        fprintf(f, "<Autodesk_Cache_File>\n");
        fprintf(f, "  <cacheType Type=\"OneFilePerFrame\" Format=\"f3d_%s_%s\"/>\n", (sparse ? "sparse" : "dense"), format.asChar());
        fprintf(f, "  <time Range=\"%d-%d\"/>\n", startTime, endTime);
        fprintf(f, "  <cacheTimePerFrame TimePerFrame=\"%d\"/>\n", timePerFrame);
        fprintf(f, "  <cacheVersion Version=\"2.0\"/>\n");
        if (SamePath(xmlDir, cacheDir))
        {
          size_t p = pat.find_last_of("\\/");
          if (p != std::string::npos)
          {
            fprintf(f, "  <extra>f3d.file=%s</extra>\n", pat.substr(p+1).c_str());
          }
          else
          {
            fprintf(f, "  <extra>f3d.file=%s</extra>\n", pat.c_str());
          }
        }
        else
        {
          fprintf(f, "  <extra>f3d.file=%s</extra>\n", pat.c_str());
        }
        for (std::map<std::string, std::string>::iterator mit = m_remapChannels.begin(); mit != m_remapChannels.end(); ++mit)
        {
          fprintf(f, "  <extra>f3d.remap=%s:%s</extra>\n", mit->second.c_str(), mit->first.c_str());
        }
        fprintf(f, "  <Channels>\n");
        
        int d = 0;
        
        for (std::set<std::string>::iterator nit = scalarChannelNames.begin(); nit != scalarChannelNames.end(); ++nit)
        {
          channelIt = channels.find(*nit);
          if (channelIt != channels.end())
          {
            fprintf(f, "    <channel%d ChannelName=\"%s_%s\" ChannelType=\"FloatArray\" ChannelInterpretation=\"%s\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                    d++, shapeName.asChar(), nit->c_str(), nit->c_str(), timePerFrame, startTime, endTime);
          }
        }
        
        for (std::set<std::string>::iterator nit = vectorChannelNames.begin(); nit != vectorChannelNames.end(); ++nit)
        {
          channelIt = channels.find(*nit);
          if (channelIt != channels.end())
          {
            fprintf(f, "    <channel%d ChannelName=\"%s_%s\" ChannelType=\"FloatArray\" ChannelInterpretation=\"%s\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                    d++, shapeName.asChar(), nit->c_str(), nit->c_str(), timePerFrame, startTime, endTime);
          }
        }
        
        fprintf(f, "    <channel%d ChannelName=\"%s_resolution\" ChannelType=\"FloatArray\" ChannelInterpretation=\"resolution\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d++, shapeName.asChar(), timePerFrame, startTime, endTime);
        
        fprintf(f, "    <channel%d ChannelName=\"%s_offset\" ChannelType=\"FloatArray\" ChannelInterpretation=\"offset\" SamplingType=\"Regular\" SamplingRate=\"%d\" StartTime=\"%d\" EndTime=\"%d\"/>\n",
                d++, shapeName.asChar(), timePerFrame, startTime, endTime);
        
        fprintf(f, "  </Channels>\n");
        fprintf(f, "</Autodesk_Cache_File>\n");
        fprintf(f, "\n");
        
        fclose(f);
      }
      
      if (!xmlOnly)
      {
        MDGModifier dgmod;
        MSelectionList sl;
        
        // Get time node
        MObject oTime;
        sl.add("time1");
        sl.getDependNode(0, oTime);
        MFnDependencyNode nTime(oTime);
        
        // Setup Field3DInfo node
        MObject oInfo = dgmod.createNode("Field3DInfo");
        dgmod.doIt();
        
        MFnDependencyNode nInfo(oInfo);
        
        nInfo.findPlug("filename").setString(pat.c_str());
        nInfo.findPlug("transformMode").setShort(1);
        
        dgmod.connect(nTime.findPlug("outTime"), nInfo.findPlug("time"));
        dgmod.connect(nInfo.findPlug("outTranslate"), fluidTr.findPlug("translate"));
        dgmod.connect(nInfo.findPlug("outRotate"), fluidTr.findPlug("rotate"));
        dgmod.connect(nInfo.findPlug("outRotateOrder"), fluidTr.findPlug("rotateOrder"));
        dgmod.connect(nInfo.findPlug("outRotateAxis"), fluidTr.findPlug("rotateAxis"));
        dgmod.connect(nInfo.findPlug("outRotatePivot"), fluidTr.findPlug("rotatePivot"));
        dgmod.connect(nInfo.findPlug("outRotatePivotTranslate"), fluidTr.findPlug("rotatePivotTranslate"));
        dgmod.connect(nInfo.findPlug("outScale"), fluidTr.findPlug("scale"));
        dgmod.connect(nInfo.findPlug("outScalePivot"), fluidTr.findPlug("scalePivot"));
        dgmod.connect(nInfo.findPlug("outScalePivotTranslate"), fluidTr.findPlug("scalePivotTranslate"));
        dgmod.connect(nInfo.findPlug("outShear"), fluidTr.findPlug("shear"));
        dgmod.connect(nInfo.findPlug("outDimensionX"), fluid.findPlug("dimensionsW"));
        dgmod.connect(nInfo.findPlug("outDimensionY"), fluid.findPlug("dimensionsH"));
        dgmod.connect(nInfo.findPlug("outDimensionZ"), fluid.findPlug("dimensionsD"));
        dgmod.doIt();
        
        // Setup rendering quality
        fluid.findPlug("quality").setFloat(2.0f);
        fluid.findPlug("renderInterpolator").setInt(3);
        
        // Setup grids
        fluid.findPlug("densityMethod").setInt(channels.find("density") != channels.end() ? 2 : 0);
        fluid.findPlug("velocityMethod").setInt(channels.find("velocity") != channels.end() ? 2 : 0);
        fluid.findPlug("temperatureMethod").setInt(channels.find("temperature") != channels.end() ? 2 : 0);
        fluid.findPlug("fuelMethod").setInt(channels.find("fuel") != channels.end() ? 2 : 0);
        fluid.findPlug("colorMethod").setInt(channels.find("color") != channels.end() ? 2 : 0);
        fluid.findPlug("falloffMethod").setInt(channels.find("falloff") != channels.end() ? 1 : 0);
        fluid.findPlug("coordinateMethod").setInt(channels.find("texture") != channels.end() ? 1 : 0);
        
        // Assign cache
        MString cmd = "{ ";
        cmd += "string $objects[] = {\"" + shapeName + "\"}; ";
        cmd += "doImportFluidCacheFile(\"";
        cmd += xmlFile.c_str();
        cmd += "\", \"xmlcache\", $objects, {}); }";
        
        MGlobal::executeCommand(cmd);
      }
    }
    
    return MS::kSuccess;
  }
  else
  {
    return MS::kFailure;
  }
}

const std::string& importF3d::remapChannel(const std::string &name) const
{
  std::map<std::string, std::string>::const_iterator it = m_remapChannels.find(name);
  return (it != m_remapChannels.end() ? it->second : name);
}
