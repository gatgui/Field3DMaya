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


#include "field3D_Tools.h"


using namespace Field3D ;
using namespace std     ;

namespace Field3DTools {

void getFieldNames( Field3DInputFile *file, const std::string &partition, vector< string > &names)
{
  // get all partition names ( should be only one present,
  // since maya fluids are stored individualy) but we parse
  // everyone here for the sake of genericity
  names.clear();
  
  std::vector<std::string> partitionNames;
  
  if (partition.length() == 0)
  {
    file->getPartitionNames(partitionNames);
  }
  else
  {
    partitionNames.push_back(partition);
  }

  // loop through partition and harvest attributes names
  for (std::vector<std::string>::iterator it = partitionNames.begin() ; it!= partitionNames.end() ; ++it)
  {
    std::vector<std::string> scalarNames;
    file->getScalarLayerNames(scalarNames, *it);

    std::vector<std::string> vectorNames;
    file->getVectorLayerNames(vectorNames, *it);

    // add it to the main names
    std::copy(scalarNames.begin(), scalarNames.end(), std::back_inserter(names));
    std::copy(vectorNames.begin(), vectorNames.end(), std::back_inserter(names));
  }
}

void getFieldNames( Field3DInputFile *file, vector< string > &names)
{
  getFieldNames( file, "", names );
}

template <typename T>
bool getHighestResolution(Field3D::Field3DInputFile *inFile, const std::string &partition, const std::string &name, unsigned int (&resMax)[3])
{

  // take the highest resolution of all layers
  bool found = false;
  
  // loop through scalars fields
  typename Field3D::Field<T>::Vec scalarfields = readScalarLayers<T>(inFile, partition, name);
  typename Field3D::Field<T>::Vec::const_iterator its = scalarfields.begin();
  
  for (; its != scalarfields.end(); ++its)
  {
    resMax[0] = ( (unsigned int) (*its)->dataResolution().x > resMax[0]) ? (*its)->dataResolution().x : resMax[0];
    resMax[1] = ( (unsigned int) (*its)->dataResolution().y > resMax[1]) ? (*its)->dataResolution().y : resMax[1];
    resMax[2] = ( (unsigned int) (*its)->dataResolution().z > resMax[2]) ? (*its)->dataResolution().z : resMax[2];
    found = true || found;
  }

  // loop through vector fields
  typename Field3D::Field< FIELD3D_VEC3_T<T> >::Vec vectorfields = readVectorLayers<T>(inFile, partition, name);
  typename Field3D::Field< FIELD3D_VEC3_T<T> >::Vec::const_iterator itv = vectorfields.begin();
  
  for (; itv != vectorfields.end(); ++itv)
  {
    resMax[0] = ( (unsigned int) (*itv)->dataResolution().x > resMax[0]) ? (*itv)->dataResolution().x : resMax[0];
    resMax[1] = ( (unsigned int) (*itv)->dataResolution().y > resMax[1]) ? (*itv)->dataResolution().y : resMax[1];
    resMax[2] = ( (unsigned int) (*itv)->dataResolution().z > resMax[2]) ? (*itv)->dataResolution().z : resMax[2];
    found = true || found;
  }

  return found;
}

bool getFieldsResolution(Field3D::Field3DInputFile *inFile, const std::string &partition, const std::string &name, unsigned int (&resolution)[3])
{
  bool found = false;
  
  resolution[0] = resolution[1] = resolution[2] = 0;
  
  found = getHighestResolution<Field3D::half>(inFile, partition, name, resolution) || found;
  found = getHighestResolution<float>(inFile, partition, name, resolution) || found;
  found = getHighestResolution<double>(inFile, partition, name, resolution) || found;
  
  return found;
}

bool getFieldsResolution(Field3D::Field3DInputFile *inFile, const std::string &name, unsigned int (&resolution)[3])
{
  return getFieldsResolution(inFile, "", name, resolution);
}

bool getFieldsResolution(Field3D::Field3DInputFile *inFile, unsigned int (&resolution)[3])
{
  return getFieldsResolution(inFile, "", "", resolution);
}

template<typename Data_Type>
bool testScalarDataType(Field3DInputFile *inFile, const std::string &partition, const std::string &name)
{
  typename Field<Data_Type>::Vec res = readScalarLayers<Data_Type>(inFile, partition, name);
  return (!res.empty());
}


template<typename Data_Type>
bool testVectorDataType(Field3DInputFile *inFile, const std::string &partition, const std::string &name)
{
  typename Field<FIELD3D_VEC3_T<Data_Type> >::Vec res = readVectorLayers<Data_Type>(inFile, partition, name);
  return (!res.empty());
}

bool getFieldValueType( Field3DInputFile *inFile , const std::string &partition, const std::string &name , SupportedFieldTypeEnum &type )
{
  typedef Field3D::half half;
  
  // TODO : template for below
  if ( Field3DTools::testScalarDataType<half>(inFile, partition, name) )
  {
    Field<half>::Vec res = readScalarLayers<half>(inFile, partition, name);
    
    if ( !res.empty() )
    {
      Field3D::DenseField<half>::Ptr fieldD = Field3D::field_dynamic_cast<Field3D::DenseField<half> >(res[0]);
      if ( fieldD ) { type = DenseScalarField_Half; return true; }
      
      Field3D::SparseField<half>::Ptr fieldS = Field3D::field_dynamic_cast<Field3D::SparseField<half> >(res[0]);
      if ( fieldS ) { type = SparseScalarField_Half; return true; }
      
      // error
      ERROR( "Type of half scalar field " + name + " unknown : not a dense field nor a sparse field nor a MAC field");
      type = TypeUnsupported;
      return false;
    }
  }
  else if ( Field3DTools::testScalarDataType<float>(inFile, partition, name) )
  {
    Field<float>::Vec res = readScalarLayers<float>(inFile, partition, name);
    
    if ( !res.empty() )
    {
      Field3D::DenseField<float>::Ptr fieldD = Field3D::field_dynamic_cast<Field3D::DenseField<float> >(res[0]);
      if ( fieldD ) { type = DenseScalarField_Float; return true  ;}
      
      Field3D::SparseField<float>::Ptr fieldS = Field3D::field_dynamic_cast<Field3D::SparseField<float> >(res[0]);
      if ( fieldS ) { type = SparseScalarField_Float; return true; }
      
      // error
      ERROR( "Type of float scalar field " + name + " unknown : not a dense field nor a sparse field nor a MAC field");
      type = TypeUnsupported;
      return false;
    }
  }
  else if ( Field3DTools::testScalarDataType<double>(inFile, partition, name) )
  {
    Field<double>::Vec res = readScalarLayers<double>(inFile, partition, name);
    
    if ( !res.empty() )
    {
      Field3D::DenseField<double>::Ptr fieldD = Field3D::field_dynamic_cast<Field3D::DenseField<double> >(res[0]);
      if ( fieldD ) { type = DenseScalarField_Double; return true  ;}
      
      Field3D::SparseField<double>::Ptr fieldS = Field3D::field_dynamic_cast<Field3D::SparseField<double> >(res[0]);
      if ( fieldS ) { type = SparseScalarField_Double; return true; }
      
      // error
      ERROR( "Type of float scalar field " + name + " unknown : not a dense field nor a sparse field nor a MAC field");
      type = TypeUnsupported;
      return false;
    }
  }
  else if ( Field3DTools::testVectorDataType<half>(inFile, partition, name) )
  {
    Field<FIELD3D_VEC3_T<half> >::Vec res = readVectorLayers<half>(inFile, partition, name);
    
    if ( !res.empty() )
    {
      Field3D::DenseField<Field3D::V3h>::Ptr fieldD = Field3D::field_dynamic_cast<Field3D::DenseField<Field3D::V3h> >(res[0]);
      if ( fieldD ) { type = DenseVectorField_Half; return true; }
      
      Field3D::SparseField<Field3D::V3h>::Ptr fieldS = Field3D::field_dynamic_cast<Field3D::SparseField<Field3D::V3h> >(res[0]);
      if ( fieldS ) { type = SparseVectorField_Half; return true; }
      
      Field3D::MACField<Field3D::V3h>::Ptr fieldM = Field3D::field_dynamic_cast<Field3D::MACField<Field3D::V3h> >(res[0]);
      if ( fieldM ) { type = MACField_Half; return true;}
      
      // error
      ERROR( "Type of half vector field " + name + " unknown : not a dense field nor a sparse field nor a MAC Field");
      type = TypeUnsupported;
      return false;
    }
  }
  else if( Field3DTools::testVectorDataType<float>(inFile, partition, name) )
  {
      Field<FIELD3D_VEC3_T<float> >::Vec res = readVectorLayers<float>(inFile, partition, name);
      
      if ( !res.empty() )
      {
        Field3D::DenseField<Field3D::V3f>::Ptr fieldD = Field3D::field_dynamic_cast<Field3D::DenseField<Field3D::V3f> >(res[0]);
        if ( fieldD ) { type = DenseVectorField_Float; return true; }
        
        Field3D::SparseField<Field3D::V3f>::Ptr fieldS = Field3D::field_dynamic_cast<Field3D::SparseField<Field3D::V3f> >(res[0]);
        if ( fieldS ) { type = SparseVectorField_Float; return true; }
        
        Field3D::MACField<Field3D::V3f>::Ptr fieldM = Field3D::field_dynamic_cast<Field3D::MACField<Field3D::V3f> >(res[0]);
        if ( fieldM ) { type = MACField_Float; return true; }
        
        // error
        ERROR( "Type of float vector field " + name + " unknown : not a dense field nor a sparse field nor a MAC Field");
        type = TypeUnsupported;
        return false;
      }
  }
  else if( Field3DTools::testVectorDataType<double>(inFile, partition, name) )
  {
      Field<FIELD3D_VEC3_T<double> >::Vec res = readVectorLayers<double>(inFile, partition, name);
      
      if ( !res.empty() )
      {
        Field3D::DenseField<Field3D::V3d>::Ptr fieldD = Field3D::field_dynamic_cast< Field3D::DenseField<Field3D::V3d> >(res[0]);
        if ( fieldD ) { type = DenseVectorField_Double; return true; }
        
        Field3D::SparseField<Field3D::V3d>::Ptr fieldS = Field3D::field_dynamic_cast< Field3D::SparseField<Field3D::V3d> >(res[0]);
        if ( fieldS ) { type = SparseVectorField_Double; return true; }
        
        Field3D::MACField<Field3D::V3d>::Ptr fieldM = Field3D::field_dynamic_cast< Field3D::MACField<Field3D::V3d> >(res[0]);
        if ( fieldM ) { type = MACField_Double; return true; }
        
        // error
        ERROR( "Type of float vector field " + name + " unknown : not a dense field nor a sparse field nor a MAC Field");
        type = TypeUnsupported;
        return false;
      }
  }
  
  // error
  ERROR( std::string("Type of data in field ") + name + " unknown : not a float field nor a half float field");
  type = TypeUnsupported;
  return false;

}

bool getFieldValueType( Field3DInputFile *inFile , const std::string &name , SupportedFieldTypeEnum &type )
{
  return getFieldValueType( inFile, "", name, type );
}




}
