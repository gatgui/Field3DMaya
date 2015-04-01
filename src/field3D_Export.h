#ifndef FIELD3D_MAYA_EXPORT
#define FIELD3D_MAYA_EXPORT

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgList.h>
#include <maya/MFnFluid.h>
#include <maya/MDagPath.h>
#include <maya/MSelectionList.h>

class exportF3d : public MPxCommand
{
public:
  
  exportF3d();
  virtual ~exportF3d();
    
  MStatus doIt(const MArgList&);

  static void* creator();
  static MSyntax newSyntax();

private:
  
  void setF3dField(MFnFluid &fluidFn, const char *outputPath, const MDagPath &dagPath);
  MStatus parseArgs(const MArgList& args);
  
private:

  MString        m_outputDir;
  MString        m_outputName;
  bool           m_verbose;
  MSelectionList m_slist;
  int            m_start; //<-start of simulation
  int            m_end; //<-end of simulation
  bool           m_density; //<- export densiyt as well
  bool           m_temperature; //<- export temprature as well
  bool           m_fuel; //<- export fuel as well
  bool           m_color; //<- export color as well
  bool           m_vel; //<- export velocity as well
  bool           m_pressure; //<- export presurre as well
  bool           m_texture; //<- export texture as well
  bool           m_falloff; //<- export falloff as well
  int            m_numOversample; //<- oversamples the fluids but only writes out on whole frames
  bool           m_forNCache; //<- export with .xml for use with nCache node
};


#endif
