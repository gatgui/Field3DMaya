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


#include "plugin.h"

#ifdef _WIN32
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
MStatus initializePlugin( MObject obj )
{
  MFnPlugin plugin( obj, "Prime Focus London", "1.0" );

  CHECK_MSTATUS_AND_RETURN_IT( plugin.registerCacheFormat("f3d_dense_half"  , Field3dCacheFormat::DHCreator) );
  CHECK_MSTATUS_AND_RETURN_IT( plugin.registerCacheFormat("f3d_dense_float" , Field3dCacheFormat::DFCreator) );
  CHECK_MSTATUS_AND_RETURN_IT( plugin.registerCacheFormat("f3d_dense_double" , Field3dCacheFormat::DDCreator) );
  CHECK_MSTATUS_AND_RETURN_IT( plugin.registerCacheFormat("f3d_sparse_half" , Field3dCacheFormat::SHCreator) );
  CHECK_MSTATUS_AND_RETURN_IT( plugin.registerCacheFormat("f3d_sparse_float", Field3dCacheFormat::SFCreator) );
  CHECK_MSTATUS_AND_RETURN_IT( plugin.registerCacheFormat("f3d_sparse_double", Field3dCacheFormat::SDCreator) );
  CHECK_MSTATUS_AND_RETURN_IT( plugin.registerCommand("exportF3d", exportF3d::creator, exportF3d::newSyntax) );
  
  MStatus status = plugin.registerNode( "Field3DForceLoad",
                                        Field3DForceLoad::id,
                                        &Field3DForceLoad::creator,
                                        &Field3DForceLoad::initialize,
                                        MPxNode::kDependNode );
    
  if (!status)
  {
    status.perror( "registerNode Field3DForceLoad failed" );
    return status;
  }
  
  status = plugin.registerNode( "Field3DVRayMatrix",
                                Field3DVRayMatrix::id,
                                &Field3DVRayMatrix::creator,
                                &Field3DVRayMatrix::initialize,
                                MPxNode::kDependNode );
    
  if (!status)
  {
    status.perror( "registerNode FieldVRayMatrix failed" );
    return status;
  }
  
  status = plugin.registerCommand("queryF3d", QueryF3d::creator, QueryF3d::newSyntax);
  
  if (!status)
  {
    status.perror( "registerCommand queryF3d failed" );
    return status;
  }
  
  Field3D::initIO();
  
  return MStatus::kSuccess;
}

#ifdef _WIN32
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
MStatus uninitializePlugin( MObject obj )
{
  MFnPlugin plugin( obj );
  
  MStatus status = plugin.deregisterCommand("queryF3d");
  
  if (!status)
  {
    status.perror( "deregisterCommand queryF3d failed" );
    return status;
  }
  
  status = plugin.deregisterNode( Field3DVRayMatrix::id );
  
  if (!status)
  {
    status.perror( "deregisterNode Field3DVRayMatrix failed" );
    return status;
  }
  
  
  status = plugin.deregisterNode( Field3DForceLoad::id );
  
  if (!status)
  {
    status.perror( "deregisterNode Field3DForceLoad failed" );
    return status;
  }
  
  CHECK_MSTATUS_AND_RETURN_IT( plugin.deregisterCacheFormat("f3d_dense_half"   ) );
  CHECK_MSTATUS_AND_RETURN_IT( plugin.deregisterCacheFormat("f3d_dense_float"  ) );
  CHECK_MSTATUS_AND_RETURN_IT( plugin.deregisterCacheFormat("f3d_dense_double" ) );
  CHECK_MSTATUS_AND_RETURN_IT( plugin.deregisterCacheFormat("f3d_sparse_half"  ) );
  CHECK_MSTATUS_AND_RETURN_IT( plugin.deregisterCacheFormat("f3d_sparse_float" ) );
  CHECK_MSTATUS_AND_RETURN_IT( plugin.deregisterCacheFormat("f3d_sparse_double") );
  CHECK_MSTATUS_AND_RETURN_IT( plugin.deregisterCommand("exportF3d") );
  
  return MStatus::kSuccess;
}
