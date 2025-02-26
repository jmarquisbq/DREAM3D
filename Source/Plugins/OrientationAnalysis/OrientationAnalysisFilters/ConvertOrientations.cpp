/* ============================================================================
 * Copyright (c) 2009-2016 BlueQuartz Software, LLC
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of BlueQuartz Software, the US Air Force, nor the names of its
 * contributors may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The code contained herein was partially funded by the following contracts:
 *    United States Air Force Prime Contract FA8650-07-D-5800
 *    United States Air Force Prime Contract FA8650-10-D-5210
 *    United States Prime Contract Navy N00173-07-C-2068
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "ConvertOrientations.h"

#include <QtCore/QTextStream>

#include "SIMPLib/DataContainers/DataContainerArray.h"
#include "SIMPLib/FilterParameters/AbstractFilterParametersReader.h"
#include "SIMPLib/FilterParameters/ChoiceFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/LinkedPathCreationFilterParameter.h"
#include "SIMPLib/FilterParameters/SeparatorFilterParameter.h"
#include "SIMPLib/FilterParameters/StringFilterParameter.h"

#include "EbsdLib/LaueOps/LaueOps.h"
#include "EbsdLib/OrientationMath/OrientationConverter.hpp"

#include "OrientationAnalysis/OrientationAnalysisConstants.h"
#include "OrientationAnalysis/OrientationAnalysisVersion.h"

/* Create Enumerations to allow the created Attribute Arrays to take part in renaming */
enum createdPathID : RenameDataPath::DataID_t
{
  DataArrayID30 = 30,
  DataArrayID31 = 31,
  DataArrayID32 = 32,
};

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ConvertOrientations::ConvertOrientations() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ConvertOrientations::~ConvertOrientations() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ConvertOrientations::setupFilterParameters()
{
  FilterParameterVectorType parameters;

  {
    ChoiceFilterParameter::Pointer parameter = ChoiceFilterParameter::New();
    parameter->setHumanLabel("Input Orientation Type");
    parameter->setPropertyName("InputType");
    parameter->setSetterCallback(SIMPL_BIND_SETTER(ConvertOrientations, this, InputType));
    parameter->setGetterCallback(SIMPL_BIND_GETTER(ConvertOrientations, this, InputType));

    parameter->setChoices(OrientationConverter<FloatArrayType, float>::GetOrientationTypeStrings<std::vector<QString>>());
    parameter->setCategory(FilterParameter::Category::Parameter);
    parameters.push_back(parameter);
  }

  {
    ChoiceFilterParameter::Pointer parameter = ChoiceFilterParameter::New();
    parameter->setHumanLabel("Output Orientation Type");
    parameter->setPropertyName("OutputType");
    parameter->setSetterCallback(SIMPL_BIND_SETTER(ConvertOrientations, this, OutputType));
    parameter->setGetterCallback(SIMPL_BIND_GETTER(ConvertOrientations, this, OutputType));

    parameter->setChoices(OrientationConverter<FloatArrayType, float>::GetOrientationTypeStrings<std::vector<QString>>());
    parameter->setCategory(FilterParameter::Category::Parameter);
    parameters.push_back(parameter);
  }

  {
    DataArraySelectionFilterParameter::RequirementType req;
    req.daTypes = std::vector<QString>(2, SIMPL::TypeNames::Double);
    req.daTypes[1] = SIMPL::TypeNames::Float;
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Input Orientations", InputOrientationArrayPath, FilterParameter::Category::RequiredArray, ConvertOrientations, req, {0}));
  }

  parameters.push_back(SIMPL_NEW_DA_WITH_LINKED_AM_FP("Output Orientations", OutputOrientationArrayName, InputOrientationArrayPath, InputOrientationArrayPath, FilterParameter::Category::CreatedArray,
                                                      ConvertOrientations, {0}));

  setFilterParameters(parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ConvertOrientations::readFilterParameters(AbstractFilterParametersReader* reader, int index)
{
  reader->openFilterGroup(this, index);
  setInputType(reader->readValue("InputType", getInputType()));
  setOutputType(reader->readValue("OutputType", getOutputType()));
  setInputOrientationArrayPath(reader->readDataArrayPath("InputOrientationArrayPath", getInputOrientationArrayPath()));
  setOutputOrientationArrayName(reader->readString("OutputOrientationArrayName", getOutputOrientationArrayName()));
  reader->closeFilterGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ConvertOrientations::initialize()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ConvertOrientations::dataCheck()
{
  clearErrorCode();
  clearWarningCode();

  if(getInputType() == getOutputType())
  {
    QString ss = QObject::tr("Input and output orientation representation types must be different");
    setErrorCondition(-1000, ss);
  }

  int32_t minIndex = OrientationConverter<FloatArrayType, float>::GetMinIndex();
  int32_t maxIndex = OrientationConverter<FloatArrayType, float>::GetMaxIndex();

  if(getInputType() < minIndex || getInputType() > maxIndex)
  {
    QString ss = QObject::tr("There was an error with the selection of the input orientation type. The valid values range from 0 to %1").arg(maxIndex);
    setErrorCondition(-1001, ss);
  }

  if(getOutputType() < minIndex || getOutputType() > maxIndex)
  {
    QString ss = QObject::tr("There was an error with the selection of the output orientation type. The valid values range from 0 to %1").arg(maxIndex);
    setErrorCondition(-1002, ss);
  }

  // We need to return NOW because the next lines assume we have and index that is within
  // the valid bounds
  if(getErrorCode() < 0)
  {
    return;
  }

  // Figure out what kind of Array the user selected
  // Get the input data and create the output Data appropriately
  IDataArray::Pointer iDataArrayPtr = getDataContainerArray()->getPrereqIDataArrayFromPath(this, getInputOrientationArrayPath());
  if(getErrorCode() < 0)
  {
    return;
  }
  int numComps = iDataArrayPtr->getNumberOfComponents();
  std::vector<int32_t> componentCounts = OrientationConverter<FloatArrayType, float>::GetComponentCounts<std::vector<int32_t>>();
  if(numComps != componentCounts[getInputType()])
  {
    QString sizeNameMappingString;
    QTextStream strm(&sizeNameMappingString);
    std::vector<QString> names = OrientationConverter<FloatArrayType, float>::GetOrientationTypeStrings<std::vector<QString>>();
    for(int i = 0; i < OrientationConverter<FloatArrayType, float>::GetMaxIndex() + 1; i++)
    {
      strm << "[" << names[i] << "=" << componentCounts[i] << "] ";
    }

    QString ss = QObject::tr("The number of components (%1) of the input array does not match the required number of components for the input type (%2). These are the required Component counts. %3")
                     .arg(numComps)
                     .arg(componentCounts[getInputType()])
                     .arg(sizeNameMappingString);
    setErrorCondition(-1006, ss);
  }

  DataArrayPath outputArrayPath = getInputOrientationArrayPath();
  outputArrayPath.setDataArrayName(getOutputOrientationArrayName());

  FloatArrayType::Pointer fArray = std::dynamic_pointer_cast<FloatArrayType>(iDataArrayPtr);
  if(nullptr != fArray.get())
  {
    std::vector<size_t> outputCDims(1, componentCounts[getOutputType()]);
    getDataContainerArray()->createNonPrereqArrayFromPath<DataArray<float>>(this, outputArrayPath, 0, outputCDims, "", DataArrayID31);
  }

  DoubleArrayType::Pointer dArray = std::dynamic_pointer_cast<DoubleArrayType>(iDataArrayPtr);
  if(nullptr != dArray.get())
  {
    std::vector<size_t> outputCDims(1, componentCounts[getOutputType()]);
    getDataContainerArray()->createNonPrereqArrayFromPath<DataArray<double>>(this, outputArrayPath, 0, outputCDims, "", DataArrayID32);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
template <typename T>
void generateRepresentation(ConvertOrientations* filter, typename DataArray<T>::Pointer inputOrientations, typename DataArray<T>::Pointer outputOrientations)
{
  using ArrayType = DataArray<T>;
  using ArrayPointerType = typename ArrayType::Pointer;
  using OCType = OrientationConverter<ArrayType, T>;
  std::vector<typename OCType::Pointer> converters(8);

  converters[0] = EulerConverter<ArrayType, T>::New();
  converters[1] = OrientationMatrixConverter<ArrayType, T>::New();
  converters[2] = QuaternionConverter<ArrayType, T>::New();
  converters[3] = AxisAngleConverter<ArrayType, T>::New();
  converters[4] = RodriguesConverter<ArrayType, T>::New();
  converters[5] = HomochoricConverter<ArrayType, T>::New();
  converters[6] = CubochoricConverter<ArrayType, T>::New();
  converters[7] = StereographicConverter<ArrayType, T>::New();

  std::vector<OrientationRepresentation::Type> ocTypes = OCType::GetOrientationTypes();

  converters[filter->getInputType()]->setInputData(inputOrientations);
  converters[filter->getInputType()]->convertRepresentationTo(ocTypes[filter->getOutputType()]);

  ArrayPointerType output = converters[filter->getInputType()]->getOutputData();
  if(nullptr == output.get())
  {
    QString ss = QObject::tr("There was an error converting the input data using convertor %1").arg(QString::fromStdString(converters[filter->getInputType()]->getNameOfClass()));
    filter->setErrorCondition(-1004, ss);
    return;
  }

  if(!output->copyIntoArray(outputOrientations))
  {
    QString ss = QObject::tr("There was an error copying the final results into the output array.");
    filter->setErrorCondition(-1003, ss);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ConvertOrientations::execute()
{
  dataCheck();
  if(getErrorCode() < 0)
  {
    return;
  }

  IDataArray::Pointer iDataArrayPtr = getDataContainerArray()->getPrereqIDataArrayFromPath(this, getInputOrientationArrayPath());

  DataArrayPath outputArrayPath = getInputOrientationArrayPath();
  outputArrayPath.setDataArrayName(getOutputOrientationArrayName());

  FloatArrayType::Pointer fArray = std::dynamic_pointer_cast<FloatArrayType>(iDataArrayPtr);
  if(nullptr != fArray.get())
  {
    std::vector<int32_t> componentCounts = OrientationConverter<FloatArrayType, float>::GetComponentCounts<std::vector<int32_t>>();
    std::vector<size_t> outputCDims(1, componentCounts[getOutputType()]);
    FloatArrayType::Pointer outData = getDataContainerArray()->getPrereqArrayFromPath<FloatArrayType>(this, outputArrayPath, outputCDims);
    generateRepresentation<float>(this, fArray, outData);
  }

  DoubleArrayType::Pointer dArray = std::dynamic_pointer_cast<DoubleArrayType>(iDataArrayPtr);
  if(nullptr != dArray.get())
  {
    std::vector<int32_t> componentCounts = OrientationConverter<DoubleArrayType, double>::GetComponentCounts<std::vector<int32_t>>();
    std::vector<size_t> outputCDims(1, componentCounts[getOutputType()]);
    DoubleArrayType::Pointer outData = getDataContainerArray()->getPrereqArrayFromPath<DoubleArrayType>(this, outputArrayPath, outputCDims);
    generateRepresentation<double>(this, dArray, outData);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AbstractFilter::Pointer ConvertOrientations::newFilterInstance(bool copyFilterParameters) const
{
  ConvertOrientations::Pointer filter = ConvertOrientations::New();
  if(copyFilterParameters)
  {
    copyFilterParameterInstanceVariables(filter.get());
  }
  return filter;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ConvertOrientations::getCompiledLibraryName() const
{
  return OrientationAnalysisConstants::OrientationAnalysisBaseName;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ConvertOrientations::getBrandingString() const
{
  return "OrientationAnalysis";
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ConvertOrientations::getFilterVersion() const
{
  QString version;
  QTextStream vStream(&version);
  vStream << OrientationAnalysis::Version::Major() << "." << OrientationAnalysis::Version::Minor() << "." << OrientationAnalysis::Version::Patch();
  return version;
}
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ConvertOrientations::getGroupName() const
{
  return SIMPL::FilterGroups::ProcessingFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QUuid ConvertOrientations::getUuid() const
{
  return QUuid("{e5629880-98c4-5656-82b8-c9fe2b9744de}");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ConvertOrientations::getSubGroupName() const
{
  return SIMPL::FilterSubGroups::ConversionFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ConvertOrientations::getHumanLabel() const
{
  return "Convert Orientation Representation";
}

// -----------------------------------------------------------------------------
ConvertOrientations::Pointer ConvertOrientations::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
std::shared_ptr<ConvertOrientations> ConvertOrientations::New()
{
  struct make_shared_enabler : public ConvertOrientations
  {
  };
  std::shared_ptr<make_shared_enabler> val = std::make_shared<make_shared_enabler>();
  val->setupFilterParameters();
  return val;
}

// -----------------------------------------------------------------------------
QString ConvertOrientations::getNameOfClass() const
{
  return QString("ConvertOrientations");
}

// -----------------------------------------------------------------------------
QString ConvertOrientations::ClassName()
{
  return QString("ConvertOrientations");
}

// -----------------------------------------------------------------------------
void ConvertOrientations::setInputType(int value)
{
  m_InputType = value;
}

// -----------------------------------------------------------------------------
int ConvertOrientations::getInputType() const
{
  return m_InputType;
}

// -----------------------------------------------------------------------------
void ConvertOrientations::setOutputType(int value)
{
  m_OutputType = value;
}

// -----------------------------------------------------------------------------
int ConvertOrientations::getOutputType() const
{
  return m_OutputType;
}

// -----------------------------------------------------------------------------
void ConvertOrientations::setInputOrientationArrayPath(const DataArrayPath& value)
{
  m_InputOrientationArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath ConvertOrientations::getInputOrientationArrayPath() const
{
  return m_InputOrientationArrayPath;
}

// -----------------------------------------------------------------------------
void ConvertOrientations::setOutputOrientationArrayName(const QString& value)
{
  m_OutputOrientationArrayName = value;
}

// -----------------------------------------------------------------------------
QString ConvertOrientations::getOutputOrientationArrayName() const
{
  return m_OutputOrientationArrayName;
}
