#include "field3D_Query.h"
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MArgParser.h>
#include <Field3D/Field3DFile.h>
#include <Field3D/DenseField.h>
#include <Field3D/SparseField.h>
#include <Field3D/EmptyField.h>
#include <string>
#include <vector>
#include <map>
#include <set>

// ---

void ToPrintfPattern(std::string &name)
{
  char *buf = new char[name.length() + 32];
  
  sprintf(buf, name.c_str(), -999999);
      
  if (name == buf)
  {
    std::string tmp = name;
    
    size_t p0 = tmp.rfind('#');
    
    if (p0 != std::string::npos)
    {
      // # style frame pattern
      size_t p1 = tmp.find_last_not_of("#", p0);
      
      size_t n = p0 - p1;
      
      char pat[16];
      
      if (n > 1)
      {
        sprintf(pat, "%%0%dd", int(n));
      }
      else
      {
        sprintf(pat, "%%d");
      }
      
      name = tmp.substr(0, p1 + 1) + pat + tmp.substr(p0 + 1);
    }
  }
  
  delete[] buf;
}

size_t GetFileList(const std::string &filePattern, std::vector<std::string> &files, int *start=0, int *end=0)
{
  size_t p0 = filePattern.find_last_of("\\/");
  
  std::string dirname = (p0 == std::string::npos ? "." : filePattern.substr(0, p0));
  std::string basename = (p0 == std::string::npos ? filePattern : filePattern.substr(p0 + 1));
  
  files.clear();
  
  p0 = basename.find('%');
  
  size_t p1 = std::string::npos;
  
  if (p0 != std::string::npos)
  {
    p1 = basename.find('d', p0);
    
    if (p1 != std::string::npos)
    {
      for (size_t p=p0+1; p<p1; ++p)
      {
        char c = basename[p];
        
        if (c < '0' || c > '9')
        {
          p1 = std::string::npos;
          break;
        }
      }
    }
  }
  
  if (p1 == std::string::npos)
  {
    // no frame pattern
    
    files.push_back(filePattern);
    
    if (start)
    {
      *start = -1;
    }
    if (end)
    {
      *end = -1;
    }
  }
  else
  {
    // Build maya filespec string for using with getFileList
    MString mayaSpec;
    
    mayaSpec += basename.substr(0, p0).c_str();
    mayaSpec += "*";
    mayaSpec += basename.substr(p1 + 1).c_str();
    
    p0 = dirname.find('\\');
    while (p0 != std::string::npos)
    {
      dirname[p0] = '/';
      p0 = dirname.find('\\', p0+1);
    }
    
    if (dirname[dirname.length()-1] != '/')
    {
      dirname += "/";
    }
    
    MStringArray allFiles;
    std::map<int, std::string> frameFiles;
    
    MGlobal::executeCommand("getFileList -filespec \"" + mayaSpec + "\" -folder \"" + MString(dirname.c_str()) + "\"", allFiles);
    
    int frame = -1;
    int sframe = 999999;
    int eframe = -999999;
    
    for (unsigned int i=0; i<allFiles.length(); ++i)
    {
      if (sscanf(allFiles[i].asChar(), basename.c_str(), &frame) != 1)
      {
        MGlobal::displayWarning("queryF3d: File \"" + allFiles[i] + "\" doesn't match frame pattern");
      }
      else
      {
        if (frame < sframe)
        {
          sframe = frame;
        }
        else if (frame > eframe)
        {
          eframe = frame;
        }
        frameFiles[frame] = dirname + allFiles[i].asChar();
      }
    }
    
    if (frameFiles.size() > 0)
    {
      files.resize(frameFiles.size());
      
      size_t i = 0;
      
      for (std::map<int, std::string>::iterator it = frameFiles.begin(); it != frameFiles.end(); ++it, ++i)
      {
        files[i] = it->second;
      }
    }
    else
    {
      sframe = -1;
      eframe = -1;
    }
    
    if (start)
    {
      *start = sframe;
    }
    
    if (end)
    {
      *end = eframe;
    }
  }
  
  return files.size();
}


// ---

void* QueryF3d::creator()
{
  return new QueryF3d();
}

MSyntax QueryF3d::newSyntax()
{
  MSyntax syntax;
  
  syntax.addFlag("-f", "-file", MSyntax::kString);
  syntax.addFlag("-r", "-range", MSyntax::kNoArg);
  syntax.addFlag("-v", "-verbose", MSyntax::kNoArg);
  syntax.addFlag("-pl", "-partitions", MSyntax::kNoArg);
  syntax.addFlag("-p", "-partition", MSyntax::kString);
  syntax.addFlag("-ll", "-layers", MSyntax::kNoArg);
  syntax.addFlag("-l", "-layer", MSyntax::kString);
  syntax.addFlag("-sc", "-scalar", MSyntax::kNoArg);
  syntax.addFlag("-vc", "-vector", MSyntax::kNoArg);
  syntax.addFlag("-res", "-resolution", MSyntax::kNoArg);
  
  syntax.setMinObjects(0);
  syntax.setMaxObjects(0);
  
  return syntax;
}

QueryF3d::QueryF3d()
{
}

QueryF3d::~QueryF3d()
{
}
  
MStatus QueryF3d::doIt(const MArgList &argList)
{
  MStatus stat;
  MSyntax syntax = newSyntax();
  MArgParser args(syntax, argList);
  char msg[4096];
  
  if (!args.isFlagSet("-file"))
  {
    MGlobal::displayError("queryF3d: missing -f/-file flag");
    return MS::kFailure;
  }
  
  bool verbose = args.isFlagSet("-verbose");
  
  MString sarg;
  
  stat = args.getFlagArgument("-file", 0, sarg);
  if (stat != MS::kSuccess)
  {
    stat.perror("queryF3d");
    return stat;
  }
  
  std::string pat = sarg.asChar();
  
  ToPrintfPattern(pat);
  
  if (verbose)
  {
    MGlobal::displayInfo("queryF3d: File pattern \"" + sarg + "\" -> \"" + MString(pat.c_str()) + "\"");
  }
  
  std::vector<std::string> files;
  int startFrame = -1;
  int endFrame = -1;
  
  if (GetFileList(pat, files, &startFrame, &endFrame) == 0)
  {
    MGlobal::displayError("queryF3d: No file matching " + sarg);
    return MS::kFailure;
  }
  
  if (verbose)
  {
    MGlobal::displayInfo("queryF3d: Found file(s)");
    for (size_t i=0; i<files.size(); ++i)
    {
      MGlobal::displayInfo("  " + MString(files[i].c_str()));
    }
  }
  
  if (args.isFlagSet("-range"))
  {
    MIntArray res;
    
    res.append(startFrame);
    res.append(endFrame);
    
    setResult(res);
    
    return MS::kSuccess;
  }
  else
  {
    Field3D::Field3DInputFile f3d;
    
    if (f3d.open(files[0]))
    {
      if (args.isFlagSet("-partitions"))
      {
        MStringArray rv;
        std::vector<std::string> names;
        
        f3d.getPartitionNames(names);
        
        if (verbose)
        {
          sprintf(msg, "queryF3d: Found %lu partition(s)", names.size());
          MGlobal::displayInfo(msg);
        }
        
        for (size_t i=0; i<names.size(); ++i)
        {
          rv.append(names[i].c_str());
        }
        
        setResult(rv);
        
        return MS::kSuccess;
      }
      else if (args.isFlagSet("-layers"))
      {
        if (!args.isFlagSet("-partition"))
        {
          MGlobal::displayError("queryF3d: Please specify the partition with -p/-partition flag");
          return MS::kFailure;
        }
        
        stat = args.getFlagArgument("-partition", 0, sarg);
        if (stat != MS::kSuccess)
        {
          stat.perror("queryF3d");
          return stat;
        }
        
        MStringArray rv;
        
        std::string partition = sarg.asChar();
        std::vector<std::string> names;
        
        bool scalar = args.isFlagSet("-scalar");
        bool vector = args.isFlagSet("-vector");
        
        if (!scalar && !vector)
        {
          // neither -scalar nor -vector flag were set, output both
          scalar = true;
          vector = true;
        }
        
        if (scalar)
        {
          f3d.getScalarLayerNames(names, partition);
          for (size_t i=0; i<names.size(); ++i)
          {
            rv.append(names[i].c_str());
          }
        }
        
        if (vector)
        {
          names.clear();
          
          f3d.getVectorLayerNames(names, partition);
          for (size_t i=0; i<names.size(); ++i)
          {
            rv.append(names[i].c_str());
          }
        }
        
        setResult(rv);
        
        return MS::kSuccess;
      }
      else if (args.isFlagSet("-resolution"))
      {
        if (!args.isFlagSet("-partition"))
        {
          MGlobal::displayError("queryF3d: Please specify the partition with -p/-partition flag");
          return MS::kFailure;
        }
        
        if (!args.isFlagSet("-layer"))
        {
          MGlobal::displayError("queryF3d: Please specify the layer with -l/-layer flag");
          return MS::kFailure;
        }
        
        stat = args.getFlagArgument("-partition", 0, sarg);
        if (stat != MS::kSuccess)
        {
          stat.perror("queryF3d");
          return stat;
        }
        
        std::string partition = sarg.asChar();
        
        stat = args.getFlagArgument("-layer", 0, sarg);
        if (stat != MS::kSuccess)
        {
          stat.perror("queryF3d");
          return stat;
        }
        
        std::string layer = sarg.asChar();
        
        if (verbose)
        {
          sprintf(msg, "queryF3d: Read resolution for %s.%s", partition.c_str(), layer.c_str());
          MGlobal::displayInfo(msg);
        }
        
        MIntArray rv;
        
        Field3D::EmptyField<Field3D::half>::Vec hfld = f3d.readProxyLayer<Field3D::half>(partition, layer, false);
        
        if (hfld.size() == 0)
        {
          hfld = f3d.readProxyLayer<Field3D::half>(partition, layer, true);
          if (verbose && hfld.size() > 0)
          {
            MGlobal::displayInfo("(vector-half field)");
          }
        }
        else if (verbose)
        {
          MGlobal::displayInfo("(scalar-half field)");
        }
        
        if (hfld.size() > 0)
        {
          Field3D::V3i res = hfld[0]->dataResolution();
          
          rv.append(res.x);
          rv.append(res.y);
          rv.append(res.z);
          
          setResult(rv);
          
          return MS::kSuccess;
        }
        
        Field3D::EmptyField<float>::Vec ffld = f3d.readProxyLayer<float>(partition, layer, false);
        
        if (ffld.size() == 0)
        {
          ffld = f3d.readProxyLayer<float>(partition, layer, true);
          if (verbose && ffld.size() > 0)
          {
            MGlobal::displayInfo("(vector-float field)");
          }
        }
        else if (verbose)
        {
          MGlobal::displayInfo("(scalar-float field)");
        }
        
        if (ffld.size() > 0)
        {
          Field3D::V3i res = ffld[0]->dataResolution();
          
          rv.append(res.x);
          rv.append(res.y);
          rv.append(res.z);
          
          setResult(rv);
          
          return MS::kSuccess;
        }
        
        Field3D::EmptyField<double>::Vec dfld = f3d.readProxyLayer<double>(partition, layer, false);
        
        if (dfld.size() == 0)
        {
          dfld = f3d.readProxyLayer<double>(partition, layer, true);
          if (verbose && dfld.size() > 0)
          {
            MGlobal::displayInfo("(vector-double field)");
          }
        }
        else if (verbose)
        {
          MGlobal::displayInfo("(scalar-double field)");
        }
        
        if (dfld.size() > 0)
        {
          Field3D::V3i res = dfld[0]->dataResolution();
          
          rv.append(res.x);
          rv.append(res.y);
          rv.append(res.z);
          
          setResult(rv);
          
          return MS::kSuccess;
        }
        
        MGlobal::displayWarning("queryF3d: Unsupported field type");
        
        return MS::kFailure;
      }
      else
      {
        MGlobal::displayInfo("queryF3d: Nothing to query");
        return MS::kFailure;
      }
    }
    else
    {
      MGlobal::displayError("queryF3d: Could not open file \"" + MString(files[0].c_str()) + "\"");
      return MS::kFailure;
    }
  }
}
