#ifndef FIELD3D_MAYA_QUERY_H
#define FIELD3D_MAYA_QUERY_H

#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgList.h>

class QueryF3d : public MPxCommand
{
public:
  
  QueryF3d();
  virtual ~QueryF3d();
    
  MStatus doIt(const MArgList&);

  static void* creator();
  static MSyntax newSyntax();
};

#endif
