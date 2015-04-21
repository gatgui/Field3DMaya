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


#include "maya_Tools.h"


#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MAnimControl.h>
#include <maya/MArgList.h>
#include <maya/MDagPath.h>
#include <maya/MItDag.h>
#include <maya/MFnDagNode.h>
#include <iostream>

using namespace std;


namespace MayaTools {

bool dirmap( std::string &path ) {
  MString rv = "";
  
  if (MGlobal::executeCommand(MString("dirmap -cd \"") + path.c_str() + "\"", rv) == MS::kSuccess)
  {
    path = rv.asChar();
    return true;
  }
  else
  {
    return false;
  }
}

MStatus getDagPath( string nodeName , MDagPath &dagPath , MFn::Type type = MFn::kInvalid) {

  MSelectionList sl;
  
  if (sl.add(nodeName.c_str()) == MS::kSuccess &&
      sl.getDagPath(0, dagPath) == MS::kSuccess &&
      (type == MFn::kInvalid || dagPath.hasFn(type)))
  {
    return MS::kSuccess;
  }
  else
  {
    return MS::kNotFound;
  }
}


MStatus getTransform( string nodeName , double (&transform)[4][4] ) {

  MDagPath dagPath;
  CHECK_MSTATUS_AND_RETURN_IT( getDagPath(nodeName, dagPath) ) ;

  MStatus status     = MS::kSuccess;
  MMatrix transformM = dagPath.inclusiveMatrix(&status);

  CHECK_MSTATUS_AND_RETURN_IT( status );
  CHECK_MSTATUS_AND_RETURN_IT( transformM.get(transform) );

  return MS::kSuccess;
}



MStatus getFluidNode(string fluidName, MFnFluid &fluid) {

  // get the corresponding DAG Path
  MDagPath dagPath;

  CHECK_MSTATUS_AND_RETURN_IT( getDagPath( fluidName, dagPath, MFn::kFluid) );
  
  return fluid.setObject(dagPath);
}


MStatus getNodeValue( MFnDependencyNode &node , const char *valueName, float &result )
{
  MStatus status;
  MPlug plug = node.findPlug (valueName, &status);
  CHECK_MSTATUS_AND_RETURN_IT( status );
  CHECK_MSTATUS_AND_RETURN_IT( plug.getValue (result) );
  return  MS::kSuccess;
}




}
