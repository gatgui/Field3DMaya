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


#ifndef FIELD3DTOOLS_H
#define FIELD3DTOOLS_H

#include <vector>
#include <string>

#include <Field3D/Field3DFile.h>
#include <Field3D/DenseField.h>
#include <Field3D/SparseField.h>
#include <Field3D/MACField.h>
#include <Field3D/Types.h>
#include <Field3D/FieldMetadata.h>
#include <Field3D/InitIO.h>

#include "tinyLogger.h"

namespace Field3DTools
{

const float SPARSE_THRESHOLD = 0.0000001 ;

enum SupportedFieldTypeEnum
{
   DenseScalarField_Half,
   DenseScalarField_Float,
   SparseScalarField_Half,
   SparseScalarField_Float,
   DenseVectorField_Half,
   DenseVectorField_Float,
   SparseVectorField_Half,
   SparseVectorField_Float,
   MACField_Half,
   MACField_Float,
   DenseScalarField_Double,
   DenseVectorField_Double,
   SparseScalarField_Double,
   SparseVectorField_Double,
   MACField_Double,
   TypeUnsupported
};

struct Fld
{
   SupportedFieldTypeEnum fieldType;
      
   Field3D::FieldRes::Ptr baseField;
   
   Field3D::SparseField<Field3D::half>::Ptr shScalarField;
   Field3D::SparseField<float>::Ptr sfScalarField;
   Field3D::SparseField<double>::Ptr sdScalarField;
   Field3D::DenseField<Field3D::half>::Ptr dhScalarField;
   Field3D::DenseField<float>::Ptr dfScalarField;
   Field3D::DenseField<double>::Ptr ddScalarField;
   
   Field3D::SparseField<Field3D::V3h>::Ptr shVectorField;
   Field3D::SparseField<Field3D::V3f>::Ptr sfVectorField;
   Field3D::SparseField<Field3D::V3d>::Ptr sdVectorField;
   Field3D::DenseField<Field3D::V3h>::Ptr dhVectorField;
   Field3D::DenseField<Field3D::V3f>::Ptr dfVectorField;
   Field3D::DenseField<Field3D::V3d>::Ptr ddVectorField;
   
   Field3D::MACField<Field3D::V3h>::Ptr mhField;
   Field3D::MACField<Field3D::V3f>::Ptr mfField;
   Field3D::MACField<Field3D::V3d>::Ptr mdField;
};

enum FieldTypeEnum
{
   DENSE,
   SPARSE
};

enum FieldDataTypeEnum
{
   FLOAT,
   HALF,
   DOUBLE
};

// ---------------------  Infos

void getFieldNames(Field3D::Field3DInputFile *file, std::vector<std::string> &names);
void getFieldNames(Field3D::Field3DInputFile *file, const std::string &partition, std::vector<std::string> &names);

bool getFieldsResolution(Field3D::Field3DInputFile *inFile, unsigned int (&res)[3]);
bool getFieldsResolution(Field3D::Field3DInputFile *inFile, const std::string &name, unsigned int (&res)[3]);
bool getFieldsResolution(Field3D::Field3DInputFile *inFile, const std::string &partition, const std::string &name, unsigned int (&res)[3]);

bool getFieldValueType(Field3D::Field3DInputFile *inFile, const std::string &name, Fld &fld);
bool getFieldValueType(Field3D::Field3DInputFile *inFile, const std::string &partition, const std::string &name, Fld &fld);

template <typename Data_T>
void setFieldProperties(Field3D::ResizableField<Data_T> &field,
                        const std::string &name,
                        const std::string &attribute,
                        double transform[4][4])
{
   // name, attribute
   field.name = name;
   field.attribute = attribute;

   // mapping
   Field3D::MatrixFieldMapping::Ptr mapping(new Field3D::MatrixFieldMapping);
   Field3D::M44d transf(transform);

   // just store the local transform
   mapping->setLocalToWorld(transf);
   field.setMapping(mapping);
}

// Note:
//   Field3DInputFile::readScalar|VectorLayers<T>(partitionName, layerName) doesn't handling empty names
//   implement our own

template <class Data_T>
typename Field3D::EmptyField<Data_T>::Vec readProxyScalarLayers(Field3D::Field3DInputFile *in,
                                                                const std::string &partition,
                                                                const std::string &name)
{
   typedef typename Field3D::EmptyField<Data_T>::Vec FieldList;
   
   if (partition.length() > 0)
   {
      if (name.length() > 0)
      {
         return in->readProxyLayer<Data_T>(partition, name, false);
      }
      else
      {
         std::vector<std::string> names;
         FieldList ret;
         
         getFieldNames(in, partition, names);
         
         for (size_t i=0; i<names.size(); ++i)
         {
            FieldList tmp = in->readProxyLayer<Data_T>(partition, names[i], false);
            
            for (size_t j=0; j<tmp.size(); ++j)
            {
               ret.push_back(tmp[j]);
            }
         }
         
         return ret;
      }
   }
   else
   {
      return in->readProxyScalarLayers<Data_T>(name);
   }
}

template <class Data_T>
typename Field3D::EmptyField<FIELD3D_VEC3_T<Data_T> >::Vec readProxyVectorLayers(Field3D::Field3DInputFile *in,
                                                                                 const std::string &partition,
                                                                                 const std::string &name)
{
   typedef typename Field3D::EmptyField<FIELD3D_VEC3_T<Data_T> >::Vec FieldList;
   
   if (partition.length() > 0)
   {
      if (name.length() > 0)
      {
         return in->readProxyLayer<FIELD3D_VEC3_T<Data_T> >(partition, name, true);
      }
      else
      {
         std::vector<std::string> names;
         FieldList ret;
         
         getFieldNames(in, partition, names);
         
         for (size_t i=0; i<names.size(); ++i)
         {
            FieldList tmp = in->readProxyLayer<FIELD3D_VEC3_T<Data_T> >(partition, names[i], true);
            
            for (size_t j=0; j<tmp.size(); ++j)
            {
               ret.push_back(tmp[j]);
            }
         }
         
         return ret;
      }
   }
   else
   {
      return in->readProxyVectorLayers<FIELD3D_VEC3_T<Data_T> >(name);
   }
}

template <class Data_T>
typename Field3D::Field<Data_T>::Vec readScalarLayers(Field3D::Field3DInputFile *in,
                                                      const std::string &partition,
                                                      const std::string &name)
{
   typedef typename Field3D::Field<Data_T>::Vec FieldList;
   
   if (partition.length() > 0)
   {
      if (name.length() > 0)
      {
         return in->readScalarLayers<Data_T>(partition, name);
      }
      else
      {
         std::vector<std::string> names;
         FieldList ret;
         
         getFieldNames(in, partition, names);
         
         for (size_t i=0; i<names.size(); ++i)
         {
            FieldList tmp = in->readScalarLayers<Data_T>(partition, names[i]);
            
            for (size_t j=0; j<tmp.size(); ++j)
            {
               ret.push_back(tmp[j]);
            }
         }
         
         return ret;
      }
   }
   else
   {
      return in->readScalarLayers<Data_T>(name);
   }
}

template <class Data_T>
typename Field3D::Field<FIELD3D_VEC3_T<Data_T> >::Vec readVectorLayers(Field3D::Field3DInputFile *in,
                                                                       const std::string &partition,
                                                                       const std::string &name)
{
   typedef typename Field3D::Field<FIELD3D_VEC3_T<Data_T> >::Vec FieldList;
   
   if (partition.length() > 0)
   {
      if (name.length() > 0)
      {
         return in->readVectorLayers<Data_T>(partition, name);
      }
      else
      {
         std::vector<std::string> names;
         FieldList ret;
         
         getFieldNames(in, partition, names);
         
         for (size_t i=0; i<names.size(); ++i)
         {
            FieldList tmp = in->readVectorLayers<Data_T>(partition, names[i]);
            
            for (size_t j=0; j<tmp.size(); ++j)
            {
               ret.push_back(tmp[j]);
            }
         }
         
         return ret;
      }
   }
   else
   {
      return in->readVectorLayers<Data_T>(name);
   }
}

// ---------------------  Read Field3d field into raw arrays

template <typename ImportType, typename MayaArray>
bool readScalarField(typename Field3D::Field<ImportType>::Ptr field,
                     MayaArray &data)
{
   typedef typename Field3D::Field<ImportType> FieldType;
   
   if (!field)
   {
      return false;
   }

   // non-safe cast to unsigned int : but resolution
   // should'nt be stored as ints in field3D anyway
   unsigned int resolution[3];
   
   Field3D::V3i reso = field->dataResolution();
   
   resolution[0] = (unsigned int) reso.x;
   resolution[1] = (unsigned int) reso.y;
   resolution[2] = (unsigned int) reso.z;
   
   unsigned int nvoxels = resolution[0] * resolution[1] * resolution[2];
   
   if (data.length() < nvoxels)
   {
      return false;
   }
   
   typename FieldType::const_iterator it = field->cbegin();
   typename FieldType::const_iterator itend = field->cend();
   
   for (; it != itend; ++it)
   {
      data[it.x + resolution[0] * (it.y + resolution[1] * it.z)] = *it;
   }

   return true;
}


template <typename ImportType, typename MayaArray>
bool readVectorField(typename Field3D::Field<FIELD3D_VEC3_T<ImportType> >::Ptr field,
                     MayaArray &data)
{
   typedef typename Field3D::Field<FIELD3D_VEC3_T<ImportType> > FieldType;
   
   if (!field)
   {
      return false;
   }

   // non-safe cast to unsigned int : but resolution
   // should'nt be stored as ints in field3D anyway
   unsigned int resolution[3];
   
   Field3D::V3i reso = field->dataResolution();
   
   resolution[0] = (unsigned int) reso.x;
   resolution[1] = (unsigned int) reso.y;
   resolution[2] = (unsigned int) reso.z;
   
   typename FieldType::const_iterator it = field->cbegin();
   typename FieldType::const_iterator itend = field->cend();
   
   unsigned int nvoxels = resolution[0] * resolution[1] * resolution[2];
   unsigned int xoff = 0;
   unsigned int yoff = nvoxels;
   unsigned int zoff = 2 * nvoxels;
   
   if (data.length() < 2 * nvoxels)
   {
      return false;
   }
   
   bool is2D = (data.length() < 3 * nvoxels);
   
   if (is2D)
   {
      for (; it != itend; ++it)
      {
         size_t off = it.x + resolution[0] * (it.y + resolution[1] * it.z);
         
         data[xoff + off] = (*it).x;
         data[yoff + off] = (*it).y;
      }
   }
   else
   {
      for (; it != itend; ++it)
      {
         size_t off = it.x + resolution[0] * (it.y + resolution[1] * it.z);
         
         data[xoff + off] = (*it).x;
         data[yoff + off] = (*it).y;
         data[zoff + off] = (*it).z;
      }
   }
   
   return true;
}

template <typename ImportType, typename MayaArray>
bool readMACField(typename Field3D::MACField<FIELD3D_VEC3_T<ImportType> >::Ptr field,
                  MayaArray &data)
{
   typedef Field3D::MACField<FIELD3D_VEC3_T<ImportType> > FieldType;
   
   if (!field)
   {
      return false;
   }

   // non-safe cast to unsigned int : but resolution
   // should'nt be stored as ints in field3D anyway
   unsigned int resolution[3];
   
   Field3D::V3i reso = field->dataResolution();
   
   resolution[0] = (unsigned int) reso.x;
   resolution[1] = (unsigned int) reso.y;
   resolution[2] = (unsigned int) reso.z;
   
   // copy data into MAC field
   Field3D::MACComponent compo[3] = {Field3D::MACCompU, Field3D::MACCompV, Field3D::MACCompW};
   unsigned int off = 0;
   unsigned int r[3] = {0, 0, 0};
   
   for (unsigned int cp=0; cp<3; ++cp)
   {
      if (cp == 0)
      {
         off = 0;
         r[0] = resolution[0]+1;
         r[1] = resolution[1];
         r[2] = resolution[2];
      }
      else if (cp == 1)
      {
         off += r[0] * r[1] * r[2];
         r[0] = resolution[0];
         r[1] = resolution[1]+1;
         r[2] = resolution[2];
      }
      else if (cp == 2)
      {
         off += r[0] * r[1] * r[2];
         r[0] = resolution[0];
         r[1] = resolution[1];
         r[2] = resolution[2]+1;
      }
      
      if (off + r[0] * r[1] * r[2] > data.length())
      {
         break;
      }

      typename FieldType::mac_comp_iterator it = field->begin_comp(compo[cp]);
      typename FieldType::mac_comp_iterator itend = field->end_comp(compo[cp]);
      
      for ( ; it != itend; ++it)
      {
         data[off + it.x + r[0] * (it.y + r[1] * it.z)] = *it;
      }

   }

   return true;
}

template <typename ImportType, typename MayaArray>
bool readScalarFieldFromFile(Field3D::Field3DInputFile *in,
                             const std::string &fluidName,
                             const std::string &fieldName,
                             MayaArray &data)
{
   typename Field3D::Field<ImportType>::Vec sl = readScalarLayers<ImportType>(in, fluidName, fieldName);
   
   return readScalarField(sl[0], data);
}


template <typename ImportType, typename MayaArray>
bool readVectorFieldFromFile(Field3D::Field3DInputFile *in,
                             const std::string &fluidName,
                             const std::string &fieldName,
                             MayaArray &data)
{
   typename Field3D::Field<FIELD3D_VEC3_T<ImportType> >::Vec sl = readVectorLayers<ImportType>(in, fluidName, fieldName)       ;
   
   return readVectorField(sl[0], data);
}

template <typename ImportType, typename MayaArray>
bool readMACFieldFromFile(Field3D::Field3DInputFile *in,
                             const std::string &fluidName,
                             const std::string &fieldName,
                             MayaArray &data)
{
   typedef typename Field3D::Field<FIELD3D_VEC3_T<ImportType> > FieldType;
   typedef typename Field3D::MACField<FIELD3D_VEC3_T<ImportType> > MACFieldType;
   
   typename FieldType::Vec sl = readVectorLayers<ImportType>(in, fluidName, fieldName);
   
   return readMACField(Field3D::field_dynamic_cast<MACFieldType>(sl[0]), data);
}


// ---------------------  Write raw arrays into Field3D files

typedef void writeMetadataFunc(Field3D::FieldRes::Ptr field, void*);

template <typename ExportType, typename MayaArray>
bool writeDenseScalarField(Field3D::Field3DOutputFile *out,
                           const std::string &fluidName,
                           const std::string &fieldName,
                           unsigned int res[3],
                           double transform[4][4],
                           const MayaArray &data,
                           writeMetadataFunc writeMetadata=0,
                           void *writeMetadataUser=0)
{
   // field declaration
   typename Field3D::DenseField<ExportType>::Ptr field = new Field3D::DenseField<ExportType>();

   // properties
   Field3DTools::setFieldProperties(*field.get(), fluidName, fieldName, transform);

   // copy channel into the scalar field
   field->setSize(Field3D::V3i(res[0], res[1], res[2]));
   
   for (unsigned int k=0; k<res[2]; ++k)
   {
      for (unsigned int j=0; j<res[1]; ++j)
      {
         for (unsigned int i=0; i<res[0]; ++i)
         {
            // TODO : check conversion
            ExportType val = (ExportType) data[i + res[0] * (j + res[1] * k)];
            
            field->fastLValue(i, j, k) = val;
         }
      }
   }
   
   if (writeMetadata)
   {
      writeMetadata(field, writeMetadataUser);
   }

   // write it onto disk
   if (!out->writeScalarLayer<ExportType>(field))
   {
      ERROR( std::string("Problem while writing dense scalar field ") + fieldName + " : Unknown Reason ");
      return false;
   }

   return true;
}

template <typename ExportType, typename MayaArray>
bool writeSparseScalarField(Field3D::Field3DOutputFile *out,
                            const std::string &fluidName,
                            const std::string &fieldName,
                            unsigned int res[3],
                            double transform[4][4],
                            const MayaArray &data,
                            writeMetadataFunc writeMetadata=0,
                            void *writeMetadataUser=0)
{
   // field declaration
   typename Field3D::SparseField<ExportType>::Ptr field = new Field3D::SparseField<ExportType>();

   // properties
   Field3DTools::setFieldProperties(*field.get(), fluidName, fieldName, transform);

   // copy channel into the scalar field
   field->setSize(Field3D::V3i(res[0], res[1], res[2]));
   
   for (unsigned int k=0; k<res[2]; ++k)
   {
      for (unsigned int j=0; j<res[1]; ++j)
      {
         for (unsigned int i=0; i<res[0]; ++i)
         {
            ExportType val = (ExportType) data[i + res[0] * (j + res[1] * k)];
            
            field->fastLValue(i, j, k) = val;
         }
      }
   }
   
   if (writeMetadata)
   {
      writeMetadata(field, writeMetadataUser);
   }
   
   // write it onto disk
   if (!out->writeScalarLayer<ExportType>(field))
   {
      ERROR( std::string("Problem while writing sparse scalar field ") + fieldName + " : Unknown Reason ");
      return false;
   }

   return true;
}

template <typename ExportType, typename MayaArray>
bool writeDenseVectorField(Field3D::Field3DOutputFile *out,
                           const std::string &fluidName,
                           const std::string &fieldName,
                           unsigned int res[3],
                           double transform[4][4],
                           const MayaArray &data,
                           writeMetadataFunc writeMetadata=0,
                           void *writeMetadataUser=0)
{
   // field declaration
   typename Field3D::DenseField<FIELD3D_VEC3_T<ExportType> >::Ptr field = new Field3D::DenseField<FIELD3D_VEC3_T<ExportType> >();
   
   unsigned int nvoxels = res[0] * res[1] * res[2];
   
   if (data.length() < 2 * nvoxels)
   {
      return false;
   }
   
   bool is3D = (data.length() >= 3 * nvoxels ? true : false);
   
   unsigned int xbase = 0;
   unsigned int ybase = nvoxels;
   unsigned int zbase = (is3D ? 2 * nvoxels : 0);
   
   // properties
   Field3DTools::setFieldProperties(*field.get(), fluidName, fieldName, transform);
   
   // copy channel into the vector field
   field->setSize(Field3D::V3i(res[0], res[1], res[2]));
   
   for (unsigned int k=0; k<res[2]; ++k)
   {
      for (unsigned int j=0; j<res[1]; ++j)
      {
         for (unsigned int i=0; i<res[0]; ++i)
         {
            size_t off = i + res[0] * (j + res[1] * k);
            
            // TODO : check conversion
            ExportType a = (ExportType) data[xbase + off];
            ExportType b = (ExportType) data[ybase + off];
            ExportType c = (ExportType) (is3D ? data[zbase + off] : 0.0f);
            
            field->fastLValue(i, j, k) = Imath::Vec3<ExportType>(a, b, c);
         }
      }
   }
   
   if (writeMetadata)
   {
      writeMetadata(field, writeMetadataUser);
   }

   // write it onto disk
   if (!out->writeScalarLayer<FIELD3D_VEC3_T<ExportType> >(field))
   {
      ERROR( std::string("Problem while writing dense vector field ") + fieldName + " : Unknown Reason ");
      return false;
   }

   return true;
}

template <typename ExportType, typename MayaArray>
bool writeSparseVectorField(Field3D::Field3DOutputFile *out,
                            const std::string &fluidName,
                            const std::string &fieldName,
                            unsigned int res[3],
                            double transform[4][4],
                            const MayaArray &data,
                            writeMetadataFunc writeMetadata=0,
                            void *writeMetadataUser=0)
{
   // field declaration
   typename Field3D::SparseField<FIELD3D_VEC3_T<ExportType> >::Ptr field = new Field3D::SparseField<FIELD3D_VEC3_T<ExportType> >();
   
   // setup X, Y and Z field base offsets
   unsigned int nvoxels = res[0] * res[1] * res[2];
   
   if (data.length() < 2 * nvoxels)
   {
      return false;
   }
   
   bool is3D = (data.length() >= 3 * nvoxels ? true : false);
   
   unsigned int xbase = 0;
   unsigned int ybase = nvoxels;
   unsigned int zbase = (is3D ? 2 * nvoxels : 0);
   
   // properties
   Field3DTools::setFieldProperties(*field, fluidName, fieldName, transform);

   // copy channel into the vector field
   field->setSize(Field3D::V3i(res[0], res[1], res[2]));
   
   for (unsigned int k=0; k<res[2]; ++k)
   {
      for (unsigned int j=0; j<res[1]; ++j)
      {
         for (unsigned int i=0; i<res[0]; ++i)
         {
            size_t off = i + res[0] * (j + res[1] * k);
            
            // TODO : check conversion
            ExportType a = (ExportType) data[xbase + off];
            ExportType b = (ExportType) data[ybase + off];
            ExportType c = (ExportType) (is3D ? data[zbase + off] : 0.0f);

            field->fastLValue(i, j, k) = Imath::Vec3<ExportType>(a, b, c);
         }
      }
   }
   
   if (writeMetadata)
   {
      writeMetadata(field, writeMetadataUser);
   }
   
   // write it onto disk
   if (!out->writeScalarLayer<FIELD3D_VEC3_T<ExportType> >(field))
   {
      ERROR( std::string("Problem while writing sparse vector field ") + fieldName + " : Unknown Reason ");
      return false;
   }
   
   return true;
}

template <typename ExportType, typename MayaArray>
bool writeMACVectorField(Field3D::Field3DOutputFile *out,
                         const std::string &fluidName,
                         const std::string &fieldName,
                         unsigned int res[3],
                         double transform[4][4],
                         const MayaArray &v,
                         writeMetadataFunc writeMetadata=0,
                         void *writeMetadataUser=0)
{
   // field declaration
   typename Field3D::MACField<FIELD3D_VEC3_T<ExportType> >::Ptr field = new Field3D::MACField<FIELD3D_VEC3_T<ExportType> >();
   
   // setup X, Y and Z field base offsets
   unsigned int nvoxelsx = (res[0] + 1) * res[1] * res[2];
   unsigned int nvoxelsy = res[0] * (res[1] + 1) * res[2];
   unsigned int nvoxelsz = res[0] * res[1] * (res[2] + 1);
   
   if (v.length() < (nvoxelsx + nvoxelsy))
   {
      return false;
   }
   
   bool is3D = (v.length() >= (nvoxelsx + nvoxelsy + nvoxelsz) ? true : false);
   
   unsigned int xbase = 0;
   unsigned int ybase = nvoxelsx;
   unsigned int zbase = (is3D ? nvoxelsx + nvoxelsy: 0);
   
   // properties
   Field3DTools::setFieldProperties(*field, fluidName, fieldName, transform);

   // copy channel into the vector field
   field->setSize(Field3D::V3i(res[0], res[1], res[2]));
   
   unsigned int x, y, z;
   
   // do the common job for all components (instead of doing it per component)
   for (x = 0; x < res[0]; ++x)
   {
      for (y = 0; y < res[1]; ++y)
      {
         for (z = 0; z < res[2]; ++z)
         {
            // TODO : check conversion
            field->u(x, y, z) = (ExportType) v[xbase + x + (res[0] + 1) * (y + res[1] * z)];
            field->v(x, y, z) = (ExportType) v[ybase + x + res[0] * (y + (res[1] + 1) * z)];
            field->w(x, y, z) = (ExportType) (is3D ? v[zbase + x + res[0] * (y + res[1] * z)] : 0.0f);
         }
      }
   }

   // and fill the remaining component :u
   x = res[0];
   for (y = 0; y < res[1]; ++y)
   {
      for (z = 0; z < res[2]; ++z)
      {
         field->u(res[0], y, z) = (ExportType) v[xbase + x + (res[0] + 1) * (y + res[1] * z)];
      }
   }

   // and fill the remaining component : v
   y = res[1];
   for (x = 0; x < res[0]; ++x)
   {
      for (z = 0; z < res[2]; ++z)
      {
         field->v(x, res[1], z) = (ExportType) v[ybase + x + res[0] * (y + (res[1] + 1) * z)];
      }
   }
   
   // and fill the remaining component : w
   z = res[2];
   for (x = 0; x < res[0]; ++x)
   {
      for (y = 0; y < res[1]; ++y)
      {
         field->w(x, y, res[2]) = (ExportType) (is3D ? v[zbase + x + res[0] * (y + res[1] * z)] : 0.0f);
      }
   }
   
   if (writeMetadata)
   {
      writeMetadata(field, writeMetadataUser);
   }

   // write it onto disk
   if (!out->writeScalarLayer<FIELD3D_VEC3_T<ExportType> >(field))
   {
      ERROR( std::string("Problem while writing MAC vector field ") + fieldName + " : Unknown Reason ");
      return false;
   }

   return true;
}

}

#endif
