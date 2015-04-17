#ifndef FIELD3D_MAYA_EXPORT
#define FIELD3D_MAYA_EXPORT

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgList.h>
#include <maya/MFnFluid.h>
#include <maya/MDagPath.h>
#include <maya/MSelectionList.h>
#include <map>
#include <string>

class exportF3d : public MPxCommand
{
public:
  
  exportF3d();
  virtual ~exportF3d();
    
  MStatus doIt(const MArgList&);

  static void* creator();
  static MSyntax newSyntax();

private:
  
  template <typename FField, typename VField, typename MField>
  void setF3dField(MFnFluid &fluidFn, const char *outputPath, const MDagPath &dagPath);
  
  MStatus parseArgs(const MArgList& args);
  
  const std::string& remapChannel(const std::string &name) const;
  
private:

  MString        m_outputDir;
  MString        m_outputName;
  bool           m_verbose;
  MSelectionList m_slist;
  int            m_start; //<-start of simulation
  int            m_end; //<-end of simulation
  bool           m_hasDensity; //<- export densiyt as well
  bool           m_hasTemperature; //<- export temprature as well
  bool           m_hasFuel; //<- export fuel as well
  bool           m_hasColor; //<- export color as well
  bool           m_hasVelocity; //<- export velocity as well
  bool           m_hasPressure; //<- export presurre as well
  bool           m_hasTexture; //<- export texture as well
  bool           m_hasFalloff; //<- export falloff as well
  int            m_numOversample; //<- oversamples the fluids but only writes out on whole frames
  bool           m_forNCache; //<- export with .xml for use with nCache node
  bool           m_ignoreDensity;
  bool           m_ignoreTemperature;
  bool           m_ignoreFuel;
  bool           m_ignoreColor;
  bool           m_ignoreVelocity;
  bool           m_ignorePressure;
  bool           m_ignoreTexture;
  bool           m_ignoreFalloff;
  bool           m_sparse;
  bool           m_half;
  double         m_sparseThreshold;
  int            m_sparseBlockOrder;
  double         m_sparseScalarDefault;
  double         m_sparseVectorDefault[3];
  std::map<std::string, std::string> m_remapChannels;
};


#endif
