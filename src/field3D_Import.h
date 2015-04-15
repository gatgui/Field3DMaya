#ifndef FIELD3D_MAYA_IMPORT
#define FIELD3D_MAYA_IMPORT

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgList.h>
#include <maya/MFnFluid.h>
#include <maya/MDagPath.h>
#include <maya/MSelectionList.h>
#include <map>
#include <string>

class importF3d : public MPxCommand
{
public:
  
  importF3d();
  virtual ~importF3d();
    
  MStatus doIt(const MArgList&);
  
  static void* creator();
  static MSyntax newSyntax();

private:
  
  const std::string& remapChannel(const std::string &name) const;
  
private:

  std::map<std::string, std::string> m_remapChannels;
};


#endif
