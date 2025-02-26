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
#include "ChangeAngleRepresentation.h"

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/partitioner.h>
#endif

#include <QtCore/QDebug>
#include <QtCore/QTextStream>

#include "SIMPLib/Common/Constants.h"
#include "SIMPLib/DataContainers/DataContainerArray.h"
#include "SIMPLib/FilterParameters/AbstractFilterParametersReader.h"
#include "SIMPLib/FilterParameters/ChoiceFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/SeparatorFilterParameter.h"
#include "SIMPLib/Math/SIMPLibMath.h"

#include "OrientationAnalysis/OrientationAnalysisConstants.h"
#include "OrientationAnalysis/OrientationAnalysisVersion.h"

/**
 * @brief The ChangeAngleRepresentationImpl class implements a threaded algorithm to convert an array of
 * Euler angles from raidans to degrees or degrees to radians, depending upon the incoming
 * conversion factor.
 */
class ChangeAngleRepresentationImpl
{

public:
  ChangeAngleRepresentationImpl(float* data, float factor)
  : m_CellEulerAngles(data)
  , convFactor(factor)
  {
  }
  virtual ~ChangeAngleRepresentationImpl() = default;

  void convert(size_t start, size_t end) const
  {
    for(size_t i = start; i < end; i++)
    {
      m_CellEulerAngles[i] = m_CellEulerAngles[i] * convFactor;
    }
  }

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
  void operator()(const tbb::blocked_range<size_t>& r) const
  {
    convert(r.begin(), r.end());
  }
#endif
private:
  float* m_CellEulerAngles;
  float convFactor;
};

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ChangeAngleRepresentation::ChangeAngleRepresentation() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ChangeAngleRepresentation::~ChangeAngleRepresentation() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ChangeAngleRepresentation::setupFilterParameters()
{
  FilterParameterVectorType parameters;

  {
    ChoiceFilterParameter::Pointer parameter = ChoiceFilterParameter::New();
    parameter->setHumanLabel("Conversion Type");
    parameter->setPropertyName("ConversionType");
    parameter->setSetterCallback(SIMPL_BIND_SETTER(ChangeAngleRepresentation, this, ConversionType));
    parameter->setGetterCallback(SIMPL_BIND_GETTER(ChangeAngleRepresentation, this, ConversionType));

    std::vector<QString> choices;
    choices.push_back("Degrees to Radians");
    choices.push_back("Radians to Degrees");
    parameter->setChoices(choices);
    parameter->setCategory(FilterParameter::Category::Parameter);
    parameters.push_back(parameter);
  }
  {
    DataArraySelectionFilterParameter::RequirementType req =
        DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::Float, SIMPL::Defaults::AnyComponentSize, AttributeMatrix::Category::Any);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Angles", CellEulerAnglesArrayPath, FilterParameter::Category::RequiredArray, ChangeAngleRepresentation, req));
  }

  setFilterParameters(parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ChangeAngleRepresentation::readFilterParameters(AbstractFilterParametersReader* reader, int index)
{
  reader->openFilterGroup(this, index);
  setCellEulerAnglesArrayPath(reader->readDataArrayPath("CellEulerAnglesArrayPath", getCellEulerAnglesArrayPath()));
  setConversionType(reader->readValue("ConversionType", getConversionType()));
  reader->closeFilterGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ChangeAngleRepresentation::initialize()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ChangeAngleRepresentation::dataCheck()
{
  clearErrorCode();
  clearWarningCode();

  std::vector<size_t> cDims(1, 3);
  m_CellEulerAnglesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<float>>(this, getCellEulerAnglesArrayPath(), cDims);
  if(nullptr != m_CellEulerAnglesPtr.lock())
  {
    m_CellEulerAngles = m_CellEulerAnglesPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ChangeAngleRepresentation::execute()
{
  dataCheck();
  if(getErrorCode() < 0)
  {
    return;
  }

  int64_t totalPoints = static_cast<int64_t>(m_CellEulerAnglesPtr.lock()->getNumberOfTuples());

  float conversionFactor = 1.0f;
  if(m_ConversionType == SIMPL::EulerAngleConversionType::DegreesToRadians)
  {
    conversionFactor = static_cast<float>(M_PI / 180.0f);
  }
  else if(conversionFactor == SIMPL::EulerAngleConversionType::RadiansToDegrees)
  {
    conversionFactor = static_cast<float>(180.0f / M_PI);
  }

  totalPoints *= 3;
//  qDebug() << "ChangeAngleRepresentation: " << m_ConversionFactor << "\n";
#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
  if(true)
  {
    tbb::parallel_for(tbb::blocked_range<size_t>(0, totalPoints), ChangeAngleRepresentationImpl(m_CellEulerAngles, conversionFactor), tbb::auto_partitioner());
  }
  else
#endif
  {
    ChangeAngleRepresentationImpl serial(m_CellEulerAngles, conversionFactor);
    serial.convert(0, totalPoints);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AbstractFilter::Pointer ChangeAngleRepresentation::newFilterInstance(bool copyFilterParameters) const
{
  ChangeAngleRepresentation::Pointer filter = ChangeAngleRepresentation::New();
  if(copyFilterParameters)
  {
    copyFilterParameterInstanceVariables(filter.get());
  }
  return filter;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ChangeAngleRepresentation::getCompiledLibraryName() const
{
  return OrientationAnalysisConstants::OrientationAnalysisBaseName;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ChangeAngleRepresentation::getBrandingString() const
{
  return "OrientationAnalysis";
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ChangeAngleRepresentation::getFilterVersion() const
{
  QString version;
  QTextStream vStream(&version);
  vStream << OrientationAnalysis::Version::Major() << "." << OrientationAnalysis::Version::Minor() << "." << OrientationAnalysis::Version::Patch();
  return version;
}
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ChangeAngleRepresentation::getGroupName() const
{
  return SIMPL::FilterGroups::ProcessingFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QUuid ChangeAngleRepresentation::getUuid() const
{
  return QUuid("{f7bc0e1e-0f50-5fe0-a9e7-510b6ed83792}");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ChangeAngleRepresentation::getSubGroupName() const
{
  return SIMPL::FilterSubGroups::ConversionFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ChangeAngleRepresentation::getHumanLabel() const
{
  return "Convert Angles to Degrees or Radians";
}

// -----------------------------------------------------------------------------
ChangeAngleRepresentation::Pointer ChangeAngleRepresentation::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
std::shared_ptr<ChangeAngleRepresentation> ChangeAngleRepresentation::New()
{
  struct make_shared_enabler : public ChangeAngleRepresentation
  {
  };
  std::shared_ptr<make_shared_enabler> val = std::make_shared<make_shared_enabler>();
  val->setupFilterParameters();
  return val;
}

// -----------------------------------------------------------------------------
QString ChangeAngleRepresentation::getNameOfClass() const
{
  return QString("ChangeAngleRepresentation");
}

// -----------------------------------------------------------------------------
QString ChangeAngleRepresentation::ClassName()
{
  return QString("ChangeAngleRepresentation");
}

// -----------------------------------------------------------------------------
void ChangeAngleRepresentation::setConversionType(int value)
{
  m_ConversionType = value;
}

// -----------------------------------------------------------------------------
int ChangeAngleRepresentation::getConversionType() const
{
  return m_ConversionType;
}

// -----------------------------------------------------------------------------
void ChangeAngleRepresentation::setCellEulerAnglesArrayPath(const DataArrayPath& value)
{
  m_CellEulerAnglesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath ChangeAngleRepresentation::getCellEulerAnglesArrayPath() const
{
  return m_CellEulerAnglesArrayPath;
}
