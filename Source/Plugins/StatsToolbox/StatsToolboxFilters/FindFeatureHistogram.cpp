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
#include "FindFeatureHistogram.h"

#include <QtCore/QTextStream>

#include "SIMPLib/Common/Constants.h"
#include "SIMPLib/DataContainers/DataContainer.h"
#include "SIMPLib/DataContainers/DataContainerArray.h"
#include "SIMPLib/FilterParameters/AbstractFilterParametersReader.h"
#include "SIMPLib/FilterParameters/ChoiceFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArrayCreationFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/LinkedBooleanFilterParameter.h"
#include "SIMPLib/FilterParameters/SeparatorFilterParameter.h"

#include "StatsToolbox/DistributionAnalysisOps/BetaOps.h"
#include "StatsToolbox/DistributionAnalysisOps/LogNormalOps.h"
#include "StatsToolbox/DistributionAnalysisOps/PowerLawOps.h"

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FindFeatureHistogram::FindFeatureHistogram() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FindFeatureHistogram::~FindFeatureHistogram() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindFeatureHistogram::setupFilterParameters()
{
  FilterParameterVectorType parameters;

  {
    ChoiceFilterParameter::Pointer parameter = ChoiceFilterParameter::New();
    parameter->setHumanLabel("Number of Bins");
    parameter->setPropertyName("NumberOfBins");
    parameter->setSetterCallback(SIMPL_BIND_SETTER(FindFeatureHistogram, this, NumberOfBins));
    parameter->setGetterCallback(SIMPL_BIND_GETTER(FindFeatureHistogram, this, NumberOfBins));

    parameters.push_back(parameter);
  }
  std::vector<QString> linkedProps = {"BiasedFeaturesArrayPath"};
  parameters.push_back(SIMPL_NEW_LINKED_BOOL_FP("Remove Biased Features", RemoveBiasedFeatures, FilterParameter::Category::Parameter, FindFeatureHistogram, linkedProps));
  {
    DataArraySelectionFilterParameter::RequirementType req;
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Feature Array To Bin", SelectedFeatureArrayPath, FilterParameter::Category::RequiredArray, FindFeatureHistogram, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req;
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("FeaturePhases", FeaturePhasesArrayPath, FilterParameter::Category::RequiredArray, FindFeatureHistogram, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req;
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("BiasedFeatures", BiasedFeaturesArrayPath, FilterParameter::Category::RequiredArray, FindFeatureHistogram, req));
  }

  {
    DataArrayCreationFilterParameter::RequirementType req;
    parameters.push_back(SIMPL_NEW_DA_CREATION_FP("New Ensemble Array", NewEnsembleArrayArrayPath, FilterParameter::Category::CreatedArray, FindFeatureHistogram, req));
  }

  setFilterParameters(parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindFeatureHistogram::readFilterParameters(AbstractFilterParametersReader* reader, int index)
{
  reader->openFilterGroup(this, index);
  setNewEnsembleArrayArrayPath(reader->readDataArrayPath("NewEnsembleArrayArrayPath", getNewEnsembleArrayArrayPath()));
  setBiasedFeaturesArrayPath(reader->readDataArrayPath("BiasedFeaturesArrayPath", getBiasedFeaturesArrayPath()));
  setFeaturePhasesArrayPath(reader->readDataArrayPath("FeaturePhasesArrayPath", getFeaturePhasesArrayPath()));
  setSelectedFeatureArrayPath(reader->readDataArrayPath("SelectedFeatureArrayPath", getSelectedFeatureArrayPath()));
  setNumberOfBins(reader->readValue("NumberOfBins", getNumberOfBins()));
  setRemoveBiasedFeatures(reader->readValue("RemoveBiasedFeatures", getRemoveBiasedFeatures()));
  reader->closeFilterGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindFeatureHistogram::initialize()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindFeatureHistogram::dataCheck()
{
  clearErrorCode();
  clearWarningCode();

  std::vector<size_t> dims(1, 1);
  m_FeaturePhasesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<int32_t>>(this, getFeaturePhasesArrayPath(), dims);
  if(nullptr != m_FeaturePhasesPtr.lock())
  {
    m_FeaturePhases = m_FeaturePhasesPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */

  if(m_SelectedFeatureArrayPath.isEmpty())
  {
    setErrorCondition(-11000, "An array from the Volume DataContainer must be selected.");
  }

  int numComp = m_NumberOfBins;
  getNewEnsembleArrayArrayPath().setDataArrayName(m_SelectedFeatureArrayPath.getDataArrayName() + QString("Histogram"));
  dims[0] = numComp;
  m_NewEnsembleArrayPtr = getDataContainerArray()->createNonPrereqArrayFromPath<DataArray<int>>(
      this, getNewEnsembleArrayArrayPath(), 0, dims); /* Assigns the shared_ptr<>(this, tempPath, 0, dims); Assigns the shared_ptr<> to an instance variable that is a weak_ptr<> */
  if(nullptr != m_NewEnsembleArrayPtr.lock())
  {
    m_NewEnsembleArray = m_NewEnsembleArrayPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */

  if(m_RemoveBiasedFeatures)
  {
    dims[0] = 1;
    m_BiasedFeaturesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<bool>>(this, getBiasedFeaturesArrayPath(), dims);
    if(nullptr != m_BiasedFeaturesPtr.lock())
    {
      m_BiasedFeatures = m_BiasedFeaturesPtr.lock()->getPointer(0);
    } /* Now assign the raw pointer to data from the DataArray<T> object */
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
template <typename T>
void findHistogram(IDataArray::Pointer inputData, int32_t* ensembleArray, int32_t* eIds, int NumberOfBins, bool removeBiasedFeatures, bool* biasedFeatures)
{
  typename DataArray<T>::Pointer featureArray = std::dynamic_pointer_cast<DataArray<T>>(inputData);
  if(nullptr == featureArray)
  {
    return;
  }

  T* fPtr = featureArray->getPointer(0);
  size_t numfeatures = featureArray->getNumberOfTuples();

  int32_t bin;
  int32_t ensemble;
  float min = 1000000.0f;
  float max = 0.0f;
  float value;
  for(size_t i = 1; i < numfeatures; i++)
  {
    value = fPtr[i];
    if(value > max)
    {
      max = value;
    }
    if(value < min)
    {
      min = value;
    }
  }
  float stepsize = (max - min) / NumberOfBins;

  for(size_t i = 1; i < numfeatures; i++)
  {
    if(!removeBiasedFeatures || !biasedFeatures[i])
    {
      ensemble = eIds[i];
      bin = (fPtr[i] - min) / stepsize;
      if(bin >= NumberOfBins)
      {
        bin = NumberOfBins - 1;
      }
      ensembleArray[(NumberOfBins * ensemble) + bin]++;
    }
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindFeatureHistogram::execute()
{
  dataCheck();
  if(getErrorCode() < 0)
  {
    return;
  }

  DataContainer::Pointer m = getDataContainerArray()->getDataContainer(m_SelectedFeatureArrayPath.getDataContainerName());

  QString ss;

  IDataArray::Pointer inputData = m->getAttributeMatrix(m_SelectedFeatureArrayPath.getAttributeMatrixName())->getAttributeArray(m_SelectedFeatureArrayPath.getDataArrayName());
  if(nullptr == inputData.get())
  {
    ss = QObject::tr("Selected array '%1' does not exist in the Voxel Data Container. Was it spelled correctly?").arg(m_SelectedFeatureArrayPath.getDataArrayName());
    setErrorCondition(-11001, ss);
    return;
  }

  QString dType = inputData->getTypeAsString();
  IDataArray::Pointer p = IDataArray::NullPointer();
  if(dType.compare("int8_t") == 0)
  {
    findHistogram<int8_t>(inputData, m_NewEnsembleArray, m_FeaturePhases, m_NumberOfBins, m_RemoveBiasedFeatures, m_BiasedFeatures);
  }
  else if(dType.compare("uint8_t") == 0)
  {
    findHistogram<uint8_t>(inputData, m_NewEnsembleArray, m_FeaturePhases, m_NumberOfBins, m_RemoveBiasedFeatures, m_BiasedFeatures);
  }
  else if(dType.compare("int16_t") == 0)
  {
    findHistogram<int16_t>(inputData, m_NewEnsembleArray, m_FeaturePhases, m_NumberOfBins, m_RemoveBiasedFeatures, m_BiasedFeatures);
  }
  else if(dType.compare("uint16_t") == 0)
  {
    findHistogram<uint16_t>(inputData, m_NewEnsembleArray, m_FeaturePhases, m_NumberOfBins, m_RemoveBiasedFeatures, m_BiasedFeatures);
  }
  else if(dType.compare("int32_t") == 0)
  {
    findHistogram<int32_t>(inputData, m_NewEnsembleArray, m_FeaturePhases, m_NumberOfBins, m_RemoveBiasedFeatures, m_BiasedFeatures);
  }
  else if(dType.compare("uint32_t") == 0)
  {
    findHistogram<uint32_t>(inputData, m_NewEnsembleArray, m_FeaturePhases, m_NumberOfBins, m_RemoveBiasedFeatures, m_BiasedFeatures);
  }
  else if(dType.compare("int64_t") == 0)
  {
    findHistogram<int64_t>(inputData, m_NewEnsembleArray, m_FeaturePhases, m_NumberOfBins, m_RemoveBiasedFeatures, m_BiasedFeatures);
  }
  else if(dType.compare("uint64_t") == 0)
  {
    findHistogram<uint64_t>(inputData, m_NewEnsembleArray, m_FeaturePhases, m_NumberOfBins, m_RemoveBiasedFeatures, m_BiasedFeatures);
  }
  else if(dType.compare("float") == 0)
  {
    findHistogram<float>(inputData, m_NewEnsembleArray, m_FeaturePhases, m_NumberOfBins, m_RemoveBiasedFeatures, m_BiasedFeatures);
  }
  else if(dType.compare("double") == 0)
  {
    findHistogram<double>(inputData, m_NewEnsembleArray, m_FeaturePhases, m_NumberOfBins, m_RemoveBiasedFeatures, m_BiasedFeatures);
  }
  else if(dType.compare("bool") == 0)
  {
    findHistogram<bool>(inputData, m_NewEnsembleArray, m_FeaturePhases, m_NumberOfBins, m_RemoveBiasedFeatures, m_BiasedFeatures);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AbstractFilter::Pointer FindFeatureHistogram::newFilterInstance(bool copyFilterParameters) const
{
  FindFeatureHistogram::Pointer filter = FindFeatureHistogram::New();
  if(copyFilterParameters)
  {
    copyFilterParameterInstanceVariables(filter.get());
  }
  return filter;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindFeatureHistogram::getCompiledLibraryName() const
{
  return StatsToolboxConstants::StatsToolboxBaseName;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindFeatureHistogram::getBrandingString() const
{
  return "Statistics";
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindFeatureHistogram::getFilterVersion() const
{
  QString version;
  QTextStream vStream(&version);
  vStream << StatsToolbox::Version::Major() << "." << StatsToolbox::Version::Minor() << "." << StatsToolbox::Version::Patch();
  return version;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindFeatureHistogram::getGroupName() const
{
  return SIMPL::FilterGroups::StatisticsFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QUuid FindFeatureHistogram::getUuid() const
{
  return QUuid("{f1b8354c-0aa7-517e-98c2-5e75ad2b828e}");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindFeatureHistogram::getSubGroupName() const
{
  return SIMPL::FilterSubGroups::EnsembleStatsFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindFeatureHistogram::getHumanLabel() const
{
  return "Find Feature Histogram";
}

// -----------------------------------------------------------------------------
FindFeatureHistogram::Pointer FindFeatureHistogram::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
std::shared_ptr<FindFeatureHistogram> FindFeatureHistogram::New()
{
  struct make_shared_enabler : public FindFeatureHistogram
  {
  };
  std::shared_ptr<make_shared_enabler> val = std::make_shared<make_shared_enabler>();
  val->setupFilterParameters();
  return val;
}

// -----------------------------------------------------------------------------
QString FindFeatureHistogram::getNameOfClass() const
{
  return QString("FindFeatureHistogram");
}

// -----------------------------------------------------------------------------
QString FindFeatureHistogram::ClassName()
{
  return QString("FindFeatureHistogram");
}

// -----------------------------------------------------------------------------
void FindFeatureHistogram::setSelectedFeatureArrayPath(const DataArrayPath& value)
{
  m_SelectedFeatureArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindFeatureHistogram::getSelectedFeatureArrayPath() const
{
  return m_SelectedFeatureArrayPath;
}

// -----------------------------------------------------------------------------
void FindFeatureHistogram::setNumberOfBins(int value)
{
  m_NumberOfBins = value;
}

// -----------------------------------------------------------------------------
int FindFeatureHistogram::getNumberOfBins() const
{
  return m_NumberOfBins;
}

// -----------------------------------------------------------------------------
void FindFeatureHistogram::setRemoveBiasedFeatures(bool value)
{
  m_RemoveBiasedFeatures = value;
}

// -----------------------------------------------------------------------------
bool FindFeatureHistogram::getRemoveBiasedFeatures() const
{
  return m_RemoveBiasedFeatures;
}

// -----------------------------------------------------------------------------
void FindFeatureHistogram::setFeaturePhasesArrayPath(const DataArrayPath& value)
{
  m_FeaturePhasesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindFeatureHistogram::getFeaturePhasesArrayPath() const
{
  return m_FeaturePhasesArrayPath;
}

// -----------------------------------------------------------------------------
void FindFeatureHistogram::setBiasedFeaturesArrayPath(const DataArrayPath& value)
{
  m_BiasedFeaturesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindFeatureHistogram::getBiasedFeaturesArrayPath() const
{
  return m_BiasedFeaturesArrayPath;
}

// -----------------------------------------------------------------------------
void FindFeatureHistogram::setNewEnsembleArrayArrayPath(const DataArrayPath& value)
{
  m_NewEnsembleArrayArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindFeatureHistogram::getNewEnsembleArrayArrayPath() const
{
  return m_NewEnsembleArrayArrayPath;
}
