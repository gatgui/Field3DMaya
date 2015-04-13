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

bool getFieldsResolution(Field3D::Field3DInputFile *inFile, const std::string &partition, const std::string &name, unsigned int (&resolution)[3])
{
  bool found = false;
  
  resolution[0] = resolution[1] = resolution[2] = 0;
  
  // take the highest resolution of all layers
  
  // Note: don't need to check for half/float/double, readProxyLayer actually ignores the base type
  
  // loop through scalars fields
  Field3D::EmptyField<Field3D::half>::Vec scalarfields = readProxyScalarLayers<Field3D::half>(inFile, partition, name);
  Field3D::EmptyField<Field3D::half>::Vec::const_iterator its = scalarfields.begin();
  
  for (; its != scalarfields.end(); ++its)
  {
    resolution[0] = ( (unsigned int) (*its)->dataResolution().x > resolution[0]) ? (*its)->dataResolution().x : resolution[0];
    resolution[1] = ( (unsigned int) (*its)->dataResolution().y > resolution[1]) ? (*its)->dataResolution().y : resolution[1];
    resolution[2] = ( (unsigned int) (*its)->dataResolution().z > resolution[2]) ? (*its)->dataResolution().z : resolution[2];
    found = true || found;
  }

  // loop through vector fields
  Field3D::EmptyField<FIELD3D_VEC3_T<Field3D::half> >::Vec vectorfields = readProxyVectorLayers<Field3D::half>(inFile, partition, name);
  Field3D::EmptyField<FIELD3D_VEC3_T<Field3D::half> >::Vec::const_iterator itv = vectorfields.begin();
  
  for (; itv != vectorfields.end(); ++itv)
  {
    resolution[0] = ( (unsigned int) (*itv)->dataResolution().x > resolution[0]) ? (*itv)->dataResolution().x : resolution[0];
    resolution[1] = ( (unsigned int) (*itv)->dataResolution().y > resolution[1]) ? (*itv)->dataResolution().y : resolution[1];
    resolution[2] = ( (unsigned int) (*itv)->dataResolution().z > resolution[2]) ? (*itv)->dataResolution().z : resolution[2];
    found = true || found;
  }

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

/*
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
*/

bool getFieldValueType( Field3DInputFile *inFile , const std::string &partition, const std::string &name, Fld &fld)
{
  typedef Field3D::half half;
  
  Field<half>::Vec hsres = readScalarLayers<half>(inFile, partition, name);
  if (!hsres.empty())
  {
    Field3D::DenseField<half>::Ptr fieldD = Field3D::field_dynamic_cast<Field3D::DenseField<half> >(hsres[0]);
    if (fieldD)
    {
      fld.fieldType = DenseScalarField_Half;
      fld.baseField = fieldD;
      fld.dhScalarField = fieldD;
      return true;
    }
    
    Field3D::SparseField<half>::Ptr fieldS = Field3D::field_dynamic_cast<Field3D::SparseField<half> >(hsres[0]);
    if (fieldS)
    {
      fld.fieldType = SparseScalarField_Half;
      fld.baseField = fieldS;
      fld.shScalarField = fieldS;
      return true;
    }
    
    // error
    ERROR( "Type of half scalar field " + name + " unknown : not a dense field nor a sparse field nor a MAC field");
    
    fld.fieldType = TypeUnsupported;
    
    return false;
  }
  
  Field<float>::Vec fsres = readScalarLayers<float>(inFile, partition, name);
  if (!fsres.empty())
  {
    Field3D::DenseField<float>::Ptr fieldD = Field3D::field_dynamic_cast<Field3D::DenseField<float> >(fsres[0]);
    if (fieldD)
    {
      fld.fieldType = DenseScalarField_Float;
      fld.baseField = fieldD;
      fld.dfScalarField = fieldD;
      return true;
    }
    
    Field3D::SparseField<float>::Ptr fieldS = Field3D::field_dynamic_cast<Field3D::SparseField<float> >(fsres[0]);
    if (fieldS)
    {
      fld.fieldType = SparseScalarField_Float;
      fld.baseField = fieldS;
      fld.sfScalarField = fieldS;
      return true;
    }
    
    // error
    ERROR( "Type of float scalar field " + name + " unknown : not a dense field nor a sparse field nor a MAC field");
    
    fld.fieldType = TypeUnsupported;
    
    return false;
  }
  
  Field<double>::Vec dsres = readScalarLayers<double>(inFile, partition, name);
  if (!dsres.empty())
  {
    Field3D::DenseField<double>::Ptr fieldD = Field3D::field_dynamic_cast<Field3D::DenseField<double> >(dsres[0]);
    if (fieldD)
    {
      fld.fieldType = DenseScalarField_Double;
      fld.baseField = fieldD;
      fld.ddScalarField = fieldD;
      return true;
    }
    
    Field3D::SparseField<double>::Ptr fieldS = Field3D::field_dynamic_cast<Field3D::SparseField<double> >(dsres[0]);
    if (fieldS)
    {
      fld.fieldType = SparseScalarField_Double;
      fld.baseField = fieldS;
      fld.sdScalarField = fieldS;
      return true;
    }
    
    // error
    ERROR( "Type of float scalar field " + name + " unknown : not a dense field nor a sparse field nor a MAC field");
    
    fld.fieldType = TypeUnsupported;
    
    return false;
  }
  
  Field<FIELD3D_VEC3_T<half> >::Vec hvres = readVectorLayers<half>(inFile, partition, name);
  if (!hvres.empty())
  {
    Field3D::DenseField<Field3D::V3h>::Ptr fieldD = Field3D::field_dynamic_cast<Field3D::DenseField<Field3D::V3h> >(hvres[0]);
    if (fieldD)
    {
      fld.fieldType = DenseVectorField_Half;
      fld.baseField = fieldD;
      fld.dhVectorField = fieldD;
      return true;
    }
    
    Field3D::SparseField<Field3D::V3h>::Ptr fieldS = Field3D::field_dynamic_cast<Field3D::SparseField<Field3D::V3h> >(hvres[0]);
    if (fieldS)
    {
      fld.fieldType = SparseVectorField_Half;
      fld.baseField = fieldS;
      fld.shVectorField = fieldS;
      return true;
    }
    
    Field3D::MACField<Field3D::V3h>::Ptr fieldM = Field3D::field_dynamic_cast<Field3D::MACField<Field3D::V3h> >(hvres[0]);
    if (fieldM)
    {
      fld.fieldType = MACField_Half;
      fld.baseField = fieldM;
      fld.mhField = fieldM;
      return true;
    }
    
    // error
    ERROR( "Type of half vector field " + name + " unknown : not a dense field nor a sparse field nor a MAC Field");
    
    fld.fieldType = TypeUnsupported;
    
    return false;
  }
  
  Field<FIELD3D_VEC3_T<float> >::Vec fvres = readVectorLayers<float>(inFile, partition, name);
  if (!fvres.empty())
  {
    Field3D::DenseField<Field3D::V3f>::Ptr fieldD = Field3D::field_dynamic_cast<Field3D::DenseField<Field3D::V3f> >(fvres[0]);
    if (fieldD)
    {
      fld.fieldType = DenseVectorField_Float;
      fld.baseField = fieldD;
      fld.dfVectorField = fieldD;
      return true;
    }
    
    Field3D::SparseField<Field3D::V3f>::Ptr fieldS = Field3D::field_dynamic_cast<Field3D::SparseField<Field3D::V3f> >(fvres[0]);
    if (fieldS)
    {
      fld.fieldType = SparseVectorField_Float;
      fld.baseField = fieldS;
      fld.sfVectorField = fieldS;
      return true;
    }
    
    Field3D::MACField<Field3D::V3f>::Ptr fieldM = Field3D::field_dynamic_cast<Field3D::MACField<Field3D::V3f> >(fvres[0]);
    if (fieldM)
    {
      fld.fieldType = MACField_Float;
      fld.baseField = fieldM;
      fld.mfField = fieldM;
      return true;
    }
    
    // error
    ERROR( "Type of float vector field " + name + " unknown : not a dense field nor a sparse field nor a MAC Field");
    
    fld.fieldType = TypeUnsupported;
    
    return false;
  }
  
  Field<FIELD3D_VEC3_T<double> >::Vec dvres = readVectorLayers<double>(inFile, partition, name);
  if (!dvres.empty())
  {
    Field3D::DenseField<Field3D::V3d>::Ptr fieldD = Field3D::field_dynamic_cast< Field3D::DenseField<Field3D::V3d> >(dvres[0]);
    if (fieldD)
    {
      fld.fieldType = DenseVectorField_Double;
      fld.baseField = fieldD;
      fld.ddVectorField = fieldD;
      return true;
    }
    
    Field3D::SparseField<Field3D::V3d>::Ptr fieldS = Field3D::field_dynamic_cast< Field3D::SparseField<Field3D::V3d> >(dvres[0]);
    if (fieldS)
    {
      fld.fieldType = SparseVectorField_Double;
      fld.baseField = fieldS;
      fld.sdVectorField = fieldS;
      return true;
    }
    
    Field3D::MACField<Field3D::V3d>::Ptr fieldM = Field3D::field_dynamic_cast< Field3D::MACField<Field3D::V3d> >(dvres[0]);
    if (fieldM)
    {
      fld.fieldType = MACField_Double;
      fld.baseField = fieldM;
      fld.mdField = fieldM;
      return true;
    }
    
    // error
    ERROR( "Type of float vector field " + name + " unknown : not a dense field nor a sparse field nor a MAC Field");
    
    fld.fieldType = TypeUnsupported;
    
    return false;
  }
  
  // error
  ERROR( std::string("Type of data in field ") + name + " unknown : not a float field nor a half float field");
  
  fld.fieldType = TypeUnsupported;
  
  return false;

}

bool getFieldValueType( Field3DInputFile *inFile , const std::string &name , Fld &fld )
{
  return getFieldValueType( inFile, "", name, fld );
}




}
