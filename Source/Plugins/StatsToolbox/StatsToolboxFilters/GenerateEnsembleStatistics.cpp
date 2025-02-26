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

#include "GenerateEnsembleStatistics.h"

#include <QtCore/QDebug>
#include <QtCore/QTextStream>

#include "SIMPLib/Common/Constants.h"
#include "SIMPLib/Common/PhaseType.h"
#include "SIMPLib/DataContainers/DataContainer.h"
#include "SIMPLib/DataContainers/DataContainerArray.h"
#include "SIMPLib/FilterParameters/AbstractFilterParametersReader.h"
#include "SIMPLib/FilterParameters/ChoiceFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/FloatFilterParameter.h"
#include "SIMPLib/FilterParameters/LinkedBooleanFilterParameter.h"
#include "SIMPLib/FilterParameters/LinkedPathCreationFilterParameter.h"
#include "SIMPLib/FilterParameters/PhaseTypeSelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/SeparatorFilterParameter.h"
#include "SIMPLib/FilterParameters/StringFilterParameter.h"
#include "SIMPLib/Math/MatrixMath.h"
#include "SIMPLib/Math/SIMPLibMath.h"
#include "SIMPLib/StatsData/BoundaryStatsData.h"
#include "SIMPLib/StatsData/MatrixStatsData.h"
#include "SIMPLib/StatsData/PrecipitateStatsData.h"
#include "SIMPLib/StatsData/PrimaryStatsData.h"
#include "SIMPLib/StatsData/TransformationStatsData.h"

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/OrientationTransformation.hpp"
#include "EbsdLib/Core/Quaternion.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

#include "StatsToolbox/DistributionAnalysisOps/BetaOps.h"
#include "StatsToolbox/DistributionAnalysisOps/LogNormalOps.h"
#include "StatsToolbox/DistributionAnalysisOps/PowerLawOps.h"
#include "StatsToolbox/StatsToolboxConstants.h"
#include "StatsToolbox/StatsToolboxVersion.h"

/* Create Enumerations to allow the created Attribute Arrays to take part in renaming */
enum createdPathID : RenameDataPath::DataID_t
{
  DataArrayID30 = 30,
  DataArrayID31 = 31,
};

// FIXME: #1 Need to update this to link the phase selectionwidget to the rest of the GUI, so that it preflights after it's updated.
// FIXME: #2 Need to fix phase selectionWidget to not show phase 0
// FIXME: #3 Need to link phase selectionWidget to option to include Radial Distribution Function instead of an extra linkedProps boolean.

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
GenerateEnsembleStatistics::GenerateEnsembleStatistics()
{
  m_DistributionAnalysis.push_back(BetaOps::New());
  m_DistributionAnalysis.push_back(LogNormalOps::New());
  m_DistributionAnalysis.push_back(PowerLawOps::New());

  m_NeighborList = NeighborList<int32_t>::NullPointer();
  m_SharedSurfaceAreaList = NeighborList<float>::NullPointer();

  m_StatsDataArray = StatsDataArray::NullPointer();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
GenerateEnsembleStatistics::~GenerateEnsembleStatistics() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setupFilterParameters()
{
  std::vector<QString> choices;
  choices.push_back("Beta");
  choices.push_back("Lognormal");
  choices.push_back("Power");
  FilterParameterVectorType parameters;
  std::vector<QString> phaseTypeStrings;
  PhaseType::getPhaseTypeStrings(phaseTypeStrings);
  PhaseTypeSelectionFilterParameter::Pointer phaseType_parameter = PhaseTypeSelectionFilterParameter::Create(
      "Phase Types", "PhaseTypeData", getCellEnsembleAttributeMatrixPath(), FilterParameter::Category::Parameter, SIMPL_BIND_SETTER(GenerateEnsembleStatistics, this, PhaseTypeData),
      SIMPL_BIND_GETTER(GenerateEnsembleStatistics, this, PhaseTypeData), "PhaseTypeArray", "PhaseCount", "CellEnsembleAttributeMatrixPath", phaseTypeStrings);
  parameters.push_back(phaseType_parameter);
  parameters.push_back(SIMPL_NEW_FLOAT_FP("Size Correlation Spacing", SizeCorrelationResolution, FilterParameter::Category::Parameter, GenerateEnsembleStatistics));
  parameters.push_back(SeparatorFilterParameter::Create("Cell Feature Data", FilterParameter::Category::RequiredArray));
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::Int32, 1, AttributeMatrix::Category::Feature);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Feature Phases", FeaturePhasesArrayPath, FilterParameter::Category::RequiredArray, GenerateEnsembleStatistics, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::NeighborList, 1, AttributeMatrix::Category::Feature);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Neighbor List", NeighborListArrayPath, FilterParameter::Category::RequiredArray, GenerateEnsembleStatistics, req));
  }

  std::vector<QString> linkedProps;

  linkedProps.clear();
  linkedProps.push_back("SizeDistributionFitType");
  linkedProps.push_back("BiasedFeaturesArrayPath");
  linkedProps.push_back("EquivalentDiametersArrayPath");
  linkedProps.push_back("AspectRatioDistributionFitType");
  linkedProps.push_back("AspectRatiosArrayPath");
  linkedProps.push_back("Omega3DistributionFitType");
  linkedProps.push_back("Omega3sArrayPath");
  linkedProps.push_back("NeighborhoodDistributionFitType");
  linkedProps.push_back("NeighborhoodsArrayPath");
  linkedProps.push_back("AxisEulerAnglesArrayPath");
  parameters.push_back(SIMPL_NEW_LINKED_BOOL_FP("Calculate Morphological Statistics", CalculateMorphologicalStats, FilterParameter::Category::Parameter, GenerateEnsembleStatistics, linkedProps));
  parameters.push_back(SIMPL_NEW_CHOICE_FP("Size Distribution Fit Type", SizeDistributionFitType, FilterParameter::Category::Parameter, GenerateEnsembleStatistics, choices, false));
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::Bool, 1, AttributeMatrix::Category::Feature);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Biased Features", BiasedFeaturesArrayPath, FilterParameter::Category::RequiredArray, GenerateEnsembleStatistics, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::Float, 1, AttributeMatrix::Category::Feature);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Equivalent Diameters", EquivalentDiametersArrayPath, FilterParameter::Category::RequiredArray, GenerateEnsembleStatistics, req));
  }
  parameters.push_back(SIMPL_NEW_CHOICE_FP("Aspect Ratio Distribution Fit Type", AspectRatioDistributionFitType, FilterParameter::Category::Parameter, GenerateEnsembleStatistics, choices, false));
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::Float, 2, AttributeMatrix::Category::Feature);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Aspect Ratios", AspectRatiosArrayPath, FilterParameter::Category::RequiredArray, GenerateEnsembleStatistics, req));
  }
  parameters.push_back(SIMPL_NEW_CHOICE_FP("Omega3 Distribution Fit Type", Omega3DistributionFitType, FilterParameter::Category::Parameter, GenerateEnsembleStatistics, choices, false));
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::Float, 1, AttributeMatrix::Category::Feature);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Omega3s", Omega3sArrayPath, FilterParameter::Category::RequiredArray, GenerateEnsembleStatistics, req));
  }
  parameters.push_back(SIMPL_NEW_CHOICE_FP("Neighborhood Distribution Fit Type", NeighborhoodDistributionFitType, FilterParameter::Category::Parameter, GenerateEnsembleStatistics, choices, false));
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::Int32, 1, AttributeMatrix::Category::Feature);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Neighborhoods", NeighborhoodsArrayPath, FilterParameter::Category::RequiredArray, GenerateEnsembleStatistics, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::Float, 3, AttributeMatrix::Category::Feature);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Axis Euler Angles", AxisEulerAnglesArrayPath, FilterParameter::Category::RequiredArray, GenerateEnsembleStatistics, req));
  }

  linkedProps.clear();
  linkedProps.push_back("CrystalStructuresArrayPath");
  linkedProps.push_back("SurfaceFeaturesArrayPath");
  linkedProps.push_back("VolumesArrayPath");
  linkedProps.push_back("FeatureEulerAnglesArrayPath");
  linkedProps.push_back("AvgQuatsArrayPath");
  linkedProps.push_back("SharedSurfaceAreaListArrayPath");
  linkedProps.push_back("CrystalStructuresArrayPath");
  parameters.push_back(
      SIMPL_NEW_LINKED_BOOL_FP("Calculate Crystallographic Statistics", CalculateCrystallographicStats, FilterParameter::Category::Parameter, GenerateEnsembleStatistics, linkedProps));
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::Bool, 1, AttributeMatrix::Category::Feature);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Surface Features", SurfaceFeaturesArrayPath, FilterParameter::Category::RequiredArray, GenerateEnsembleStatistics, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::Float, 1, AttributeMatrix::Category::Feature);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Volumes", VolumesArrayPath, FilterParameter::Category::RequiredArray, GenerateEnsembleStatistics, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::Float, 3, AttributeMatrix::Category::Feature);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Average Euler Angles", FeatureEulerAnglesArrayPath, FilterParameter::Category::RequiredArray, GenerateEnsembleStatistics, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::Float, 4, AttributeMatrix::Category::Feature);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Average Quaternions", AvgQuatsArrayPath, FilterParameter::Category::RequiredArray, GenerateEnsembleStatistics, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::NeighborList, 1, AttributeMatrix::Category::Feature);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Shared Surface Area List", SharedSurfaceAreaListArrayPath, FilterParameter::Category::RequiredArray, GenerateEnsembleStatistics, req));
  }
  parameters.push_back(SeparatorFilterParameter::Create("Cell Ensemble Data", FilterParameter::Category::RequiredArray));
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::UInt32, 1, AttributeMatrix::Category::Ensemble);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Crystal Structures", CrystalStructuresArrayPath, FilterParameter::Category::RequiredArray, GenerateEnsembleStatistics, req));
  }
  parameters.push_back(SeparatorFilterParameter::Create("Cell Ensemble Data", FilterParameter::Category::CreatedArray));
  // The user types in an array name for the Phase Types
  parameters.push_back(SIMPL_NEW_DA_WITH_LINKED_AM_FP("Phase Types", PhaseTypesArrayName, CellEnsembleAttributeMatrixPath, CellEnsembleAttributeMatrixPath, FilterParameter::Category::CreatedArray,
                                                      GenerateEnsembleStatistics));
  // The user types in an array name for Statistics
  parameters.push_back(SIMPL_NEW_DA_WITH_LINKED_AM_FP("Statistics", StatisticsArrayName, CellEnsembleAttributeMatrixPath, CellEnsembleAttributeMatrixPath, FilterParameter::Category::CreatedArray,
                                                      GenerateEnsembleStatistics));

  linkedProps.clear();
  linkedProps.push_back("RDFArrayPath");
  linkedProps.push_back("MaxMinRDFArrayPath");
  parameters.push_back(SIMPL_NEW_LINKED_BOOL_FP("Include Radial Distribution Function", IncludeRadialDistFunc, FilterParameter::Category::Parameter, GenerateEnsembleStatistics, linkedProps));
  {
    DataArraySelectionFilterParameter::RequirementType req =
        DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::Float, SIMPL::Defaults::AnyComponentSize, AttributeMatrix::Category::Ensemble);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Radial Distribution Function", RDFArrayPath, FilterParameter::Category::RequiredArray, GenerateEnsembleStatistics, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateCategoryRequirement(SIMPL::TypeNames::Float, 2, AttributeMatrix::Category::Ensemble);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Max and Min Separation Distances", MaxMinRDFArrayPath, FilterParameter::Category::RequiredArray, GenerateEnsembleStatistics, req));
  }

  setFilterParameters(parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::readFilterParameters(AbstractFilterParametersReader* reader, int index)
{
  reader->openFilterGroup(this, index);
  setCellEnsembleAttributeMatrixPath(reader->readDataArrayPath("CellEnsembleAttributeMatrixPath", getCellEnsembleAttributeMatrixPath()));
  setCalculateMorphologicalStats(reader->readValue("CalculateMorphologicalStats", getCalculateMorphologicalStats()));
  setIncludeRadialDistFunc(reader->readValue("IncludeRadialDistFunc", getIncludeRadialDistFunc()));
  setCellEnsembleAttributeMatrixPath(reader->readDataArrayPath("CellEnsembleAttributeMatrixPath", getCellEnsembleAttributeMatrixPath()));
  setPhaseTypesArrayName(reader->readString("PhaseTypesArrayName", getPhaseTypesArrayName()));
  setNeighborListArrayPath(reader->readDataArrayPath("NeighborListArrayPath", getNeighborListArrayPath()));
  setStatisticsArrayName(reader->readString("StatisticsArrayName", getStatisticsArrayName()));
  setAvgQuatsArrayPath(reader->readDataArrayPath("AvgQuatsArrayPath", getAvgQuatsArrayPath()));
  setFeatureEulerAnglesArrayPath(reader->readDataArrayPath("FeatureEulerAnglesArrayPath", getFeatureEulerAnglesArrayPath()));
  setVolumesArrayPath(reader->readDataArrayPath("VolumesArrayPath", getVolumesArrayPath()));
  setSurfaceFeaturesArrayPath(reader->readDataArrayPath("SurfaceFeaturesArrayPath", getSurfaceFeaturesArrayPath()));
  setCrystalStructuresArrayPath(reader->readDataArrayPath("CrystalStructuresArrayPath", getCrystalStructuresArrayPath()));
  setAxisEulerAnglesArrayPath(reader->readDataArrayPath("AxisEulerAnglesArrayPath", getAxisEulerAnglesArrayPath()));
  setOmega3sArrayPath(reader->readDataArrayPath("Omega3sArrayPath", getOmega3sArrayPath()));
  setRDFArrayPath(reader->readDataArrayPath("RDFArrayPath", getRDFArrayPath()));
  setMaxMinRDFArrayPath(reader->readDataArrayPath("MaxMinRDFArrayPath", getMaxMinRDFArrayPath()));
  setAspectRatiosArrayPath(reader->readDataArrayPath("AspectRatiosArrayPath", getAspectRatiosArrayPath()));
  setNeighborhoodsArrayPath(reader->readDataArrayPath("NeighborhoodsArrayPath", getNeighborhoodsArrayPath()));
  setSharedSurfaceAreaListArrayPath(reader->readDataArrayPath("SharedSurfaceAreaListArrayPath", getSharedSurfaceAreaListArrayPath()));
  setEquivalentDiametersArrayPath(reader->readDataArrayPath("EquivalentDiametersArrayPath", getEquivalentDiametersArrayPath()));
  setBiasedFeaturesArrayPath(reader->readDataArrayPath("BiasedFeaturesArrayPath", getBiasedFeaturesArrayPath()));
  setFeaturePhasesArrayPath(reader->readDataArrayPath("FeaturePhasesArrayPath", getFeaturePhasesArrayPath()));
  setComputeSizeDistribution(reader->readValue("ComputeSizeDistribution", getComputeSizeDistribution()));
  setSizeDistributionFitType(reader->readValue("SizeDistributionFitType", getSizeDistributionFitType()));
  setComputeAspectRatioDistribution(reader->readValue("ComputeAspectRatioDistribution", getComputeAspectRatioDistribution()));
  setAspectRatioDistributionFitType(reader->readValue("AspectRatioDistributionFitType", getAspectRatioDistributionFitType()));
  setComputeOmega3Distribution(reader->readValue("ComputeOmega3Distribution", getComputeOmega3Distribution()));
  setOmega3DistributionFitType(reader->readValue("Omega3DistributionFitType", getOmega3DistributionFitType()));
  setComputeNeighborhoodDistribution(reader->readValue("ComputeNeighborhoodDistribution", getComputeNeighborhoodDistribution()));
  setNeighborhoodDistributionFitType(reader->readValue("NeighborhoodDistributionFitType", getNeighborhoodDistributionFitType()));
  setCalculateCrystallographicStats(reader->readValue("CalculateCrystallographicStats", getCalculateCrystallographicStats()));
  setCalculateODF(reader->readValue("CalculateODF", getCalculateODF()));
  setCalculateMDF(reader->readValue("CalculateMDF", getCalculateMDF()));
  setCalculateAxisODF(reader->readValue("CalculateAxisODF", getCalculateAxisODF()));
  setSizeCorrelationResolution(reader->readValue("SizeCorrelationResolution", getSizeCorrelationResolution()));

  QVector<PhaseType::EnumType> data; // = getPhaseTypeData();
  data = reader->readArray("PhaseTypeArray", data);
  PhaseType::Types vec = PhaseType::FromQVector(data);

  setPhaseTypeData(vec);

  reader->closeFilterGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::readFilterParameters(QJsonObject& obj)
{
  AbstractFilter::readFilterParameters(obj);

  setCalculateAxisODF(static_cast<bool>(obj["CalculateAxisODF"].toInt()));
  setCalculateMDF(static_cast<bool>(obj["CalculateMDF"].toInt()));
  setCalculateODF(static_cast<bool>(obj["CalculateODF"].toInt()));
  setComputeAspectRatioDistribution(static_cast<bool>(obj["ComputeAspectRatioDistribution"].toInt()));
  setComputeNeighborhoodDistribution(static_cast<bool>(obj["ComputeNeighborhoodDistribution"].toInt()));
  setComputeOmega3Distribution(static_cast<bool>(obj["ComputeOmega3Distribution"].toInt()));
  setComputeSizeDistribution(static_cast<bool>(obj["ComputeSizeDistribution"].toInt()));

  QJsonObject dapObj = obj["CellEnsembleAttributeMatrixPath"].toObject();
  DataArrayPath dap;
  dap.readJson(dapObj);
  setCellEnsembleAttributeMatrixPath(dap);

  QJsonArray jsonArray = obj["PhaseTypeArray"].toArray();
  PhaseType::Types vec(jsonArray.size(), PhaseType::Type::Unknown);
  for(int i = 0; i < jsonArray.size(); i++)
  {
    vec[i] = static_cast<PhaseType::Type>(jsonArray[i].toInt());
  }
  setPhaseTypeData(vec);
}

// FP: Check why these values are not connected to a filter parameter!

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::writeFilterParameters(QJsonObject& obj) const
{
  AbstractFilter::writeFilterParameters(obj);

  obj["CalculateAxisODF"] = static_cast<int>(getCalculateAxisODF());
  obj["CalculateMDF"] = static_cast<int>(getCalculateMDF());
  obj["CalculateODF"] = static_cast<int>(getCalculateODF());
  obj["ComputeAspectRatioDistribution"] = static_cast<int>(getComputeAspectRatioDistribution());
  obj["ComputeNeighborhoodDistribution"] = static_cast<int>(getComputeNeighborhoodDistribution());
  obj["ComputeOmega3Distribution"] = static_cast<int>(getComputeOmega3Distribution());
  obj["ComputeSizeDistribution"] = static_cast<int>(getComputeSizeDistribution());

  DataArrayPath dap = getCellEnsembleAttributeMatrixPath();
  QJsonObject dapObj;

  dapObj["Data Container Name"] = dap.getDataContainerName();
  dapObj["Attribute Matrix Name"] = dap.getAttributeMatrixName();
  dapObj["Data Array Name"] = dap.getDataArrayName();
  obj["CellEnsembleAttributeMatrixPath"] = dapObj;

  PhaseType::Types data = getPhaseTypeData();
  QJsonArray jsonArray;
  for(const auto& d : data)
  {
    jsonArray.push_back(static_cast<int>(d));
  }
  obj["PhaseTypeArray"] = jsonArray;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::initialize()
{
  m_NeighborList = NeighborList<int32_t>::NullPointer();
  m_SharedSurfaceAreaList = NeighborList<float>::NullPointer();

  m_StatsDataArray = StatsDataArray::NullPointer();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::dataCheck()
{
  clearErrorCode();
  clearWarningCode();
  initialize();

  DataArrayPath tempPath;

  std::vector<size_t> cDims(1, 1);

  setComputeSizeDistribution(getCalculateMorphologicalStats());
  setComputeAspectRatioDistribution(getCalculateMorphologicalStats());
  setComputeOmega3Distribution(getCalculateMorphologicalStats());
  setComputeNeighborhoodDistribution(getCalculateMorphologicalStats());
  setCalculateAxisODF(getCalculateMorphologicalStats());
  setCalculateODF(getCalculateCrystallographicStats());
  setCalculateMDF(getCalculateCrystallographicStats());

  QVector<DataArrayPath> dataArrayPaths;

  m_FeaturePhasesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<int32_t>>(this, getFeaturePhasesArrayPath(), cDims);
  if(nullptr != m_FeaturePhasesPtr.lock())
  {
    m_FeaturePhases = m_FeaturePhasesPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */
  if(getErrorCode() >= 0)
  {
    dataArrayPaths.push_back(getFeaturePhasesArrayPath());
  }

  if(m_ComputeSizeDistribution || m_ComputeOmega3Distribution || m_ComputeAspectRatioDistribution || m_ComputeNeighborhoodDistribution || m_CalculateAxisODF)
  {
    m_BiasedFeaturesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<bool>>(this, getBiasedFeaturesArrayPath(), cDims);
    if(nullptr != m_BiasedFeaturesPtr.lock())
    {
      m_BiasedFeatures = m_BiasedFeaturesPtr.lock()->getPointer(0);
    } /* Now assign the raw pointer to data from the DataArray<T> object */
    if(getErrorCode() >= 0)
    {
      dataArrayPaths.push_back(getBiasedFeaturesArrayPath());
    }

    m_EquivalentDiametersPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<float>>(this, getEquivalentDiametersArrayPath(), cDims);
    if(nullptr != m_EquivalentDiametersPtr.lock())
    {
      m_EquivalentDiameters = m_EquivalentDiametersPtr.lock()->getPointer(0);
    } /* Now assign the raw pointer to data from the DataArray<T> object */
    if(getErrorCode() >= 0)
    {
      dataArrayPaths.push_back(getEquivalentDiametersArrayPath());
    }
  }

  if(m_ComputeNeighborhoodDistribution)
  {
    m_NeighborhoodsPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<int32_t>>(this, getNeighborhoodsArrayPath(), cDims);
    if(nullptr != m_NeighborhoodsPtr.lock())
    {
      m_Neighborhoods = m_NeighborhoodsPtr.lock()->getPointer(0);
    } /* Now assign the raw pointer to data from the DataArray<T> object */
    if(getErrorCode() >= 0)
    {
      dataArrayPaths.push_back(getNeighborhoodsArrayPath());
    }
  }

  if(m_ComputeAspectRatioDistribution)
  {
    cDims[0] = 2;
    m_AspectRatiosPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<float>>(this, getAspectRatiosArrayPath(), cDims);
    if(nullptr != m_AspectRatiosPtr.lock())
    {
      m_AspectRatios = m_AspectRatiosPtr.lock()->getPointer(0);
    } /* Now assign the raw pointer to data from the DataArray<T> object */
    if(getErrorCode() >= 0)
    {
      dataArrayPaths.push_back(getAspectRatiosArrayPath());
    }
  }

  if(m_ComputeOmega3Distribution)
  {
    cDims[0] = 1;
    m_Omega3sPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<float>>(this, getOmega3sArrayPath(), cDims);
    if(nullptr != m_Omega3sPtr.lock())
    {
      m_Omega3s = m_Omega3sPtr.lock()->getPointer(0);
    } /* Now assign the raw pointer to data from the DataArray<T> object */
    if(getErrorCode() >= 0)
    {
      dataArrayPaths.push_back(getOmega3sArrayPath());
    }
  }

  if(m_CalculateAxisODF)
  {
    cDims[0] = 3;
    m_AxisEulerAnglesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<float>>(this, getAxisEulerAnglesArrayPath(), cDims);
    if(nullptr != m_AxisEulerAnglesPtr.lock())
    {
      m_AxisEulerAngles = m_AxisEulerAnglesPtr.lock()->getPointer(0);
    } /* Now assign the raw pointer to data from the DataArray<T> object */
    if(getErrorCode() >= 0)
    {
      dataArrayPaths.push_back(getAxisEulerAnglesArrayPath());
    }
  }

  if(m_IncludeRadialDistFunc)
  {
    DataArray<float>::Pointer tempPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<float>>(this, getRDFArrayPath(), cDims);
    if(nullptr != tempPtr.get())
    {
      m_RadialDistFuncPtr = tempPtr;
      m_RadialDistFunc = tempPtr->getPointer(0);
    }

    cDims[0] = 2;
    m_MaxMinRadialDistFuncPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<float>>(this, getMaxMinRDFArrayPath(), cDims);
    if(nullptr != m_MaxMinRadialDistFuncPtr.lock())
    {
      m_MaxMinRadialDistFunc = m_MaxMinRadialDistFuncPtr.lock()->getPointer(0);
    } /* Now assign the raw pointer to data from the DataArray<T> object */
  }

  if(m_CalculateODF || m_CalculateMDF)
  {
    cDims[0] = 1;
    m_CrystalStructuresPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<unsigned int>>(this, getCrystalStructuresArrayPath(), cDims);
    if(nullptr != m_CrystalStructuresPtr.lock())
    {
      m_CrystalStructures = m_CrystalStructuresPtr.lock()->getPointer(0);
    } /* Now assign the raw pointer to data from the DataArray<T> object */

    m_SurfaceFeaturesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<bool>>(this, getSurfaceFeaturesArrayPath(), cDims);
    if(nullptr != m_SurfaceFeaturesPtr.lock())
    {
      m_SurfaceFeatures = m_SurfaceFeaturesPtr.lock()->getPointer(0);
    } /* Now assign the raw pointer to data from the DataArray<T> object */
    if(getErrorCode() >= 0)
    {
      dataArrayPaths.push_back(getSurfaceFeaturesArrayPath());
    }
  }

  if(m_CalculateODF)
  {
    cDims[0] = 1;
    m_VolumesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<float>>(this, getVolumesArrayPath(), cDims);
    if(nullptr != m_VolumesPtr.lock())
    {
      m_Volumes = m_VolumesPtr.lock()->getPointer(0);
    } /* Now assign the raw pointer to data from the DataArray<T> object */
    if(getErrorCode() >= 0)
    {
      dataArrayPaths.push_back(getVolumesArrayPath());
    }

    cDims[0] = 3;
    m_FeatureEulerAnglesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<float>>(this, getFeatureEulerAnglesArrayPath(), cDims);
    if(nullptr != m_FeatureEulerAnglesPtr.lock())
    {
      m_FeatureEulerAngles = m_FeatureEulerAnglesPtr.lock()->getPointer(0);
    } /* Now assign the raw pointer to data from the DataArray<T> object */
    if(getErrorCode() >= 0)
    {
      dataArrayPaths.push_back(getFeatureEulerAnglesArrayPath());
    }
  }

  if(m_CalculateMDF)
  {
    cDims[0] = 4;
    m_AvgQuatsPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<float>>(this, getAvgQuatsArrayPath(), cDims);
    if(nullptr != m_AvgQuatsPtr.lock())
    {
      m_AvgQuats = m_AvgQuatsPtr.lock()->getPointer(0);
    } /* Now assign the raw pointer to data from the DataArray<T> object */
    if(getErrorCode() >= 0)
    {
      dataArrayPaths.push_back(getAvgQuatsArrayPath());
    }

    // Now we are going to get a "Pointer" to the NeighborList object out of the DataContainer
    cDims[0] = 1;
    m_SharedSurfaceAreaList = getDataContainerArray()->getPrereqArrayFromPath<NeighborList<float>>(this, getSharedSurfaceAreaListArrayPath(), cDims);
    if(getErrorCode() >= 0)
    {
      dataArrayPaths.push_back(getSharedSurfaceAreaListArrayPath());
    }
  }

  cDims[0] = 1;
  m_NeighborList = getDataContainerArray()->getPrereqArrayFromPath<NeighborList<int>>(this, getNeighborListArrayPath(), cDims);
  if(getErrorCode() >= 0)
  {
    dataArrayPaths.push_back(getNeighborListArrayPath());
  }

  if(m_PhaseTypeData.empty())
  {
    setErrorCondition(-1000, "The phase type array must contain at least one member. An Ensemble Attribute Matrix must be selected");
    return;
  }
  if(m_PhaseTypeData.size() == 1 && m_PhaseTypeData[0] == PhaseType::Type::Unknown)
  {
    setErrorCondition(-1001, "The phase type array must contain at least one member. An Ensemble Attribute Matrix must be selected");
    return;
  }

  cDims[0] = 1;
  tempPath.update(getCellEnsembleAttributeMatrixPath().getDataContainerName(), getCellEnsembleAttributeMatrixPath().getAttributeMatrixName(), getPhaseTypesArrayName());
  m_PhaseTypesPtr = getDataContainerArray()->createNonPrereqArrayFromPath<DataArray<uint32_t>>(this, tempPath, static_cast<PhaseType::EnumType>(PhaseType::Type::Unknown), cDims, "", DataArrayID31);
  if(nullptr != m_PhaseTypesPtr.lock())
  {
    m_PhaseTypes = m_PhaseTypesPtr.lock()->getPointer(0);
    m_PhaseTypeData.resize(m_PhaseTypesPtr.lock()->getNumberOfTuples());
  } /* Now assign the raw pointer to data from the DataArray<T> object */

  // now create and add the stats array itself
  DataContainer::Pointer m = getDataContainerArray()->getPrereqDataContainer(this, getCellEnsembleAttributeMatrixPath().getDataContainerName());
  if(getErrorCode() < 0 || m == nullptr)
  {
    return;
  }
  AttributeMatrix::Pointer attrMat = m->getPrereqAttributeMatrix(this, getCellEnsembleAttributeMatrixPath().getAttributeMatrixName(), -301);
  if(getErrorCode() < 0 || attrMat == nullptr)
  {
    return;
  }

  m_StatsDataArray = StatsDataArray::CreateArray(m_PhaseTypesPtr.lock()->getNumberOfTuples(), getStatisticsArrayName(), true);
  m_StatsDataArray->fillArrayWithNewStatsData(m_PhaseTypesPtr.lock()->getNumberOfTuples(), m_PhaseTypes);
  attrMat->insertOrAssign(m_StatsDataArray);

  if(m_SizeDistributionFitType != SIMPL::DistributionType::LogNormal)
  {
    setWarningCondition(-1000, "The size distribution needs to be a lognormal distribution otherwise unpredictable results may occur");
  }
  if(m_AspectRatioDistributionFitType != SIMPL::DistributionType::Beta)
  {
    setWarningCondition(-1000, "The aspect ratio distribution needs to be a beta distribution otherwise unpredictable results may occur");
  }
  if(m_Omega3DistributionFitType != SIMPL::DistributionType::Beta)
  {
    setWarningCondition(-1000, "The Omega3 distribution needs to be a beta distribution otherwise unpredictable results may occur");
  }
  if(m_NeighborhoodDistributionFitType != SIMPL::DistributionType::LogNormal)
  {
    setWarningCondition(-1000, "The neighborhood distribution type needs to be a lognormal distribution otherwise unpredictable results may occur");
  }

  getDataContainerArray()->validateNumberOfTuples(this, dataArrayPaths);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::gatherSizeStats()
{
  StatsDataArray& statsDataArray = *(m_StatsDataArray);

  float maxdiam = 0.0f;
  float mindiam = 0.0f;
  float totalUnbiasedVolume = 0.0f;
  QVector<VectorOfFloatArray> sizedist;
  std::vector<std::vector<std::vector<float>>> values;

  FloatArrayType::Pointer binnumbers;
  size_t numfeatures = m_EquivalentDiametersPtr.lock()->getNumberOfTuples();
  size_t numensembles = m_PhaseTypesPtr.lock()->getNumberOfTuples();

  std::vector<float> fractions(numensembles, 0.0f);
  sizedist.resize(numensembles);
  values.resize(numensembles);

  for(size_t i = 1; i < numensembles; i++)
  {
    sizedist[i] = statsDataArray[i]->CreateCorrelatedDistributionArrays(m_SizeDistributionFitType, 1);
    values[i].resize(1);
  }

  float vol = 0.0f;
  for(size_t i = 1; i < numfeatures; i++)
  {
    if(!m_BiasedFeatures[i])
    {
      values[m_FeaturePhases[i]][0].push_back(m_EquivalentDiameters[i]);
    }
    vol = (1.0f / 6.0f) * SIMPLib::Constants::k_PiD * m_EquivalentDiameters[i] * m_EquivalentDiameters[i] * m_EquivalentDiameters[i];
    fractions[m_FeaturePhases[i]] = fractions[m_FeaturePhases[i]] + vol;
    totalUnbiasedVolume = totalUnbiasedVolume + vol;
  }
  for(size_t i = 1; i < numensembles; i++)
  {
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Matrix))
    {
      MatrixStatsData::Pointer pp = std::dynamic_pointer_cast<MatrixStatsData>(statsDataArray[i]);
      pp->setPhaseFraction((fractions[i] / totalUnbiasedVolume));
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Primary))
    {
      PrimaryStatsData::Pointer pp = std::dynamic_pointer_cast<PrimaryStatsData>(statsDataArray[i]);
      pp->setPhaseFraction((fractions[i] / totalUnbiasedVolume));
      m_DistributionAnalysis[m_SizeDistributionFitType]->calculateCorrelatedParameters(values[i], sizedist[i]);
      pp->setFeatureSizeDistribution(sizedist[i]);
      DistributionAnalysisOps::determineMaxAndMinValues(values[i][0], maxdiam, mindiam);
      int32_t numbins = int32_t(maxdiam / m_SizeCorrelationResolution) + 1;
      pp->setFeatureDiameterInfo(m_SizeCorrelationResolution, maxdiam, mindiam);
      binnumbers = FloatArrayType::CreateArray(numbins, SIMPL::StringConstants::BinNumber, true);
      DistributionAnalysisOps::determineBinNumbers(maxdiam, mindiam, m_SizeCorrelationResolution, binnumbers);
      pp->setBinNumbers(binnumbers);
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Precipitate))
    {
      PrecipitateStatsData::Pointer pp = std::dynamic_pointer_cast<PrecipitateStatsData>(statsDataArray[i]);
      pp->setPhaseFraction((fractions[i] / totalUnbiasedVolume));
      m_DistributionAnalysis[m_SizeDistributionFitType]->calculateCorrelatedParameters(values[i], sizedist[i]);
      pp->setFeatureSizeDistribution(sizedist[i]);
      DistributionAnalysisOps::determineMaxAndMinValues(values[i][0], maxdiam, mindiam);
      int32_t numbins = int32_t(maxdiam / m_SizeCorrelationResolution) + 1;
      pp->setFeatureDiameterInfo(m_SizeCorrelationResolution, maxdiam, mindiam);
      binnumbers = FloatArrayType::CreateArray(numbins, SIMPL::StringConstants::BinNumber, true);
      DistributionAnalysisOps::determineBinNumbers(maxdiam, mindiam, m_SizeCorrelationResolution, binnumbers);
      pp->setBinNumbers(binnumbers);
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Transformation))
    {
      TransformationStatsData::Pointer tp = std::dynamic_pointer_cast<TransformationStatsData>(statsDataArray[i]);
      tp->setPhaseFraction((fractions[i] / totalUnbiasedVolume));
      m_DistributionAnalysis[m_SizeDistributionFitType]->calculateCorrelatedParameters(values[i], sizedist[i]);
      tp->setFeatureSizeDistribution(sizedist[i]);
      DistributionAnalysisOps::determineMaxAndMinValues(values[i][0], maxdiam, mindiam);
      int numbins = int(maxdiam / m_SizeCorrelationResolution) + 1;
      tp->setFeatureDiameterInfo(m_SizeCorrelationResolution, maxdiam, mindiam);
      binnumbers = FloatArrayType::CreateArray(numbins, SIMPL::StringConstants::BinNumber, true);
      DistributionAnalysisOps::determineBinNumbers(maxdiam, mindiam, m_SizeCorrelationResolution, binnumbers);
      tp->setBinNumbers(binnumbers);
    }
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::gatherAspectRatioStats()
{
  StatsDataArray& statsDataArray = *(m_StatsDataArray);

  size_t bin = 0;
  QVector<VectorOfFloatArray> boveras;
  QVector<VectorOfFloatArray> coveras;
  std::vector<std::vector<std::vector<float>>> bvalues;
  std::vector<std::vector<std::vector<float>>> cvalues;
  std::vector<float> mindiams;
  std::vector<float> binsteps;
  size_t numfeatures = m_AspectRatiosPtr.lock()->getNumberOfTuples();
  size_t numensembles = m_PhaseTypesPtr.lock()->getNumberOfTuples();

  boveras.resize(numensembles);
  coveras.resize(numensembles);
  bvalues.resize(numensembles);
  cvalues.resize(numensembles);
  mindiams.resize(numensembles);
  binsteps.resize(numensembles);
  for(size_t i = 1; i < numensembles; i++)
  {
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Primary))
    {
      PrimaryStatsData::Pointer pp = std::dynamic_pointer_cast<PrimaryStatsData>(statsDataArray[i]);
      boveras[i] = pp->CreateCorrelatedDistributionArrays(m_AspectRatioDistributionFitType, pp->getBinNumbers()->getSize());
      coveras[i] = pp->CreateCorrelatedDistributionArrays(m_AspectRatioDistributionFitType, pp->getBinNumbers()->getSize());
      bvalues[i].resize(pp->getBinNumbers()->getSize());
      cvalues[i].resize(pp->getBinNumbers()->getSize());
      mindiams[i] = pp->getMinFeatureDiameter();
      binsteps[i] = pp->getBinStepSize();
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Precipitate))
    {
      PrecipitateStatsData::Pointer pp = std::dynamic_pointer_cast<PrecipitateStatsData>(statsDataArray[i]);
      boveras[i] = pp->CreateCorrelatedDistributionArrays(m_AspectRatioDistributionFitType, pp->getBinNumbers()->getSize());
      coveras[i] = pp->CreateCorrelatedDistributionArrays(m_AspectRatioDistributionFitType, pp->getBinNumbers()->getSize());
      bvalues[i].resize(pp->getBinNumbers()->getSize());
      cvalues[i].resize(pp->getBinNumbers()->getSize());
      mindiams[i] = pp->getMinFeatureDiameter();
      binsteps[i] = pp->getBinStepSize();
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Transformation))
    {
      TransformationStatsData::Pointer tp = std::dynamic_pointer_cast<TransformationStatsData>(statsDataArray[i]);
      boveras[i] = tp->CreateCorrelatedDistributionArrays(m_AspectRatioDistributionFitType, tp->getBinNumbers()->getSize());
      coveras[i] = tp->CreateCorrelatedDistributionArrays(m_AspectRatioDistributionFitType, tp->getBinNumbers()->getSize());
      bvalues[i].resize(tp->getBinNumbers()->getSize());
      cvalues[i].resize(tp->getBinNumbers()->getSize());
      mindiams[i] = tp->getMinFeatureDiameter();
      binsteps[i] = tp->getBinStepSize();
    }
  }
  for(size_t i = 1; i < numfeatures; i++)
  {
    if(m_PhaseTypes[m_FeaturePhases[i]] == static_cast<PhaseType::EnumType>(PhaseType::Type::Primary) ||
       m_PhaseTypes[m_FeaturePhases[i]] == static_cast<PhaseType::EnumType>(PhaseType::Type::Precipitate) ||
       m_PhaseTypes[m_FeaturePhases[i]] == static_cast<PhaseType::EnumType>(PhaseType::Type::Transformation))
    {
      if(!m_BiasedFeatures[i])
      {
        bin = size_t((m_EquivalentDiameters[i] - mindiams[m_FeaturePhases[i]]) / binsteps[m_FeaturePhases[i]]);
        bvalues[m_FeaturePhases[i]][bin].push_back(m_AspectRatios[2 * i]);
        cvalues[m_FeaturePhases[i]][bin].push_back(m_AspectRatios[2 * i + 1]);
      }
    }
  }
  for(size_t i = 1; i < numensembles; i++)
  {
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Primary))
    {
      PrimaryStatsData::Pointer pp = std::dynamic_pointer_cast<PrimaryStatsData>(statsDataArray[i]);
      m_DistributionAnalysis[m_AspectRatioDistributionFitType]->calculateCorrelatedParameters(bvalues[i], boveras[i]);
      m_DistributionAnalysis[m_AspectRatioDistributionFitType]->calculateCorrelatedParameters(cvalues[i], coveras[i]);
      pp->setFeatureSize_BOverA(boveras[i]);
      pp->setFeatureSize_COverA(coveras[i]);
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Precipitate))
    {
      PrecipitateStatsData::Pointer pp = std::dynamic_pointer_cast<PrecipitateStatsData>(statsDataArray[i]);
      m_DistributionAnalysis[m_AspectRatioDistributionFitType]->calculateCorrelatedParameters(bvalues[i], boveras[i]);
      m_DistributionAnalysis[m_AspectRatioDistributionFitType]->calculateCorrelatedParameters(cvalues[i], coveras[i]);
      pp->setFeatureSize_BOverA(boveras[i]);
      pp->setFeatureSize_COverA(coveras[i]);
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Transformation))
    {
      TransformationStatsData::Pointer tp = std::dynamic_pointer_cast<TransformationStatsData>(statsDataArray[i]);
      m_DistributionAnalysis[m_AspectRatioDistributionFitType]->calculateCorrelatedParameters(bvalues[i], boveras[i]);
      m_DistributionAnalysis[m_AspectRatioDistributionFitType]->calculateCorrelatedParameters(cvalues[i], coveras[i]);
      tp->setFeatureSize_BOverA(boveras[i]);
      tp->setFeatureSize_COverA(coveras[i]);
    }
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::gatherOmega3Stats()
{
  StatsDataArray& statsDataArray = *(m_StatsDataArray);

  size_t bin = 0;
  QVector<VectorOfFloatArray> omega3s;
  std::vector<std::vector<std::vector<float>>> values;
  QVector<float> mindiams;
  QVector<float> binsteps;
  size_t numfeatures = m_Omega3sPtr.lock()->getNumberOfTuples();
  size_t numensembles = m_PhaseTypesPtr.lock()->getNumberOfTuples();

  omega3s.resize(numensembles);
  values.resize(numensembles);
  mindiams.resize(numensembles);
  binsteps.resize(numensembles);
  for(size_t i = 1; i < numensembles; i++)
  {
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Primary))
    {
      PrimaryStatsData::Pointer pp = std::dynamic_pointer_cast<PrimaryStatsData>(statsDataArray[i]);
      omega3s[i] = pp->CreateCorrelatedDistributionArrays(m_Omega3DistributionFitType, pp->getBinNumbers()->getSize());
      values[i].resize(pp->getBinNumbers()->getSize());
      mindiams[i] = pp->getMinFeatureDiameter();
      binsteps[i] = pp->getBinStepSize();
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Precipitate))
    {
      PrecipitateStatsData::Pointer pp = std::dynamic_pointer_cast<PrecipitateStatsData>(statsDataArray[i]);
      omega3s[i] = pp->CreateCorrelatedDistributionArrays(m_Omega3DistributionFitType, pp->getBinNumbers()->getSize());
      values[i].resize(pp->getBinNumbers()->getSize());
      mindiams[i] = pp->getMinFeatureDiameter();
      binsteps[i] = pp->getBinStepSize();
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Transformation))
    {
      TransformationStatsData::Pointer tp = std::dynamic_pointer_cast<TransformationStatsData>(statsDataArray[i]);
      omega3s[i] = tp->CreateCorrelatedDistributionArrays(m_Omega3DistributionFitType, tp->getBinNumbers()->getSize());
      values[i].resize(tp->getBinNumbers()->getSize());
      mindiams[i] = tp->getMinFeatureDiameter();
      binsteps[i] = tp->getBinStepSize();
    }
  }
  for(size_t i = 1; i < numfeatures; i++)
  {
    if(m_PhaseTypes[m_FeaturePhases[i]] == static_cast<PhaseType::EnumType>(PhaseType::Type::Primary) ||
       m_PhaseTypes[m_FeaturePhases[i]] == static_cast<PhaseType::EnumType>(PhaseType::Type::Precipitate) ||
       m_PhaseTypes[m_FeaturePhases[i]] == static_cast<PhaseType::EnumType>(PhaseType::Type::Transformation))
    {
      if(!m_BiasedFeatures[i])
      {
        bin = size_t((m_EquivalentDiameters[i] - mindiams[m_FeaturePhases[i]]) / binsteps[m_FeaturePhases[i]]);
        values[m_FeaturePhases[i]][bin].push_back(m_Omega3s[i]);
      }
    }
  }
  for(size_t i = 1; i < numensembles; i++)
  {
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Primary))
    {
      PrimaryStatsData::Pointer pp = std::dynamic_pointer_cast<PrimaryStatsData>(statsDataArray[i]);
      m_DistributionAnalysis[m_Omega3DistributionFitType]->calculateCorrelatedParameters(values[i], omega3s[i]);
      pp->setFeatureSize_Omegas(omega3s[i]);
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Precipitate))
    {
      PrecipitateStatsData::Pointer pp = std::dynamic_pointer_cast<PrecipitateStatsData>(statsDataArray[i]);
      m_DistributionAnalysis[m_Omega3DistributionFitType]->calculateCorrelatedParameters(values[i], omega3s[i]);
      pp->setFeatureSize_Omegas(omega3s[i]);
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Transformation))
    {
      TransformationStatsData::Pointer tp = std::dynamic_pointer_cast<TransformationStatsData>(statsDataArray[i]);
      m_DistributionAnalysis[m_Omega3DistributionFitType]->calculateCorrelatedParameters(values[i], omega3s[i]);
      tp->setFeatureSize_Omegas(omega3s[i]);
    }
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::gatherNeighborhoodStats()
{
  StatsDataArray& statsDataArray = *(m_StatsDataArray);

  size_t bin = 0;
  QVector<VectorOfFloatArray> neighborhoods;
  std::vector<std::vector<std::vector<float>>> values;
  std::vector<float> mindiams;
  std::vector<float> binsteps;
  size_t numfeatures = m_NeighborhoodsPtr.lock()->getNumberOfTuples();
  size_t numensembles = m_PhaseTypesPtr.lock()->getNumberOfTuples();

  neighborhoods.resize(numensembles);
  values.resize(numensembles);
  mindiams.resize(numensembles);
  binsteps.resize(numensembles);
  for(size_t i = 1; i < numensembles; i++)
  {
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Primary))
    {
      PrimaryStatsData::Pointer pp = std::dynamic_pointer_cast<PrimaryStatsData>(statsDataArray[i]);
      neighborhoods[i] = pp->CreateCorrelatedDistributionArrays(m_NeighborhoodDistributionFitType, pp->getBinNumbers()->getSize());
      values[i].resize(pp->getBinNumbers()->getSize());
      mindiams[i] = pp->getMinFeatureDiameter();
      binsteps[i] = pp->getBinStepSize();
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Precipitate))
    {
      PrecipitateStatsData::Pointer pp = std::dynamic_pointer_cast<PrecipitateStatsData>(statsDataArray[i]);
      neighborhoods[i] = pp->CreateCorrelatedDistributionArrays(m_NeighborhoodDistributionFitType, pp->getBinNumbers()->getSize());
      values[i].resize(pp->getBinNumbers()->getSize());
      mindiams[i] = pp->getMinFeatureDiameter();
      binsteps[i] = pp->getBinStepSize();
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Transformation))
    {
      TransformationStatsData::Pointer tp = std::dynamic_pointer_cast<TransformationStatsData>(statsDataArray[i]);
      neighborhoods[i] = tp->CreateCorrelatedDistributionArrays(m_NeighborhoodDistributionFitType, tp->getBinNumbers()->getSize());
      values[i].resize(tp->getBinNumbers()->getSize());
      mindiams[i] = tp->getMinFeatureDiameter();
      binsteps[i] = tp->getBinStepSize();
    }
  }

  for(size_t i = 1; i < numfeatures; i++)
  {
    if(m_PhaseTypes[m_FeaturePhases[i]] == static_cast<PhaseType::EnumType>(PhaseType::Type::Primary) ||
       m_PhaseTypes[m_FeaturePhases[i]] == static_cast<PhaseType::EnumType>(PhaseType::Type::Precipitate) ||
       m_PhaseTypes[m_FeaturePhases[i]] == static_cast<PhaseType::EnumType>(PhaseType::Type::Transformation))
    {
      if(!m_BiasedFeatures[i])
      {
        bin = size_t((m_EquivalentDiameters[i] - mindiams[m_FeaturePhases[i]]) / binsteps[m_FeaturePhases[i]]);
        values[m_FeaturePhases[i]][bin].push_back(static_cast<float>(m_Neighborhoods[i]));
      }
    }
  }
  for(size_t i = 1; i < numensembles; i++)
  {
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Primary))
    {
      PrimaryStatsData::Pointer pp = std::dynamic_pointer_cast<PrimaryStatsData>(statsDataArray[i]);
      m_DistributionAnalysis[m_NeighborhoodDistributionFitType]->calculateCorrelatedParameters(values[i], neighborhoods[i]);
      pp->setFeatureSize_Neighbors(neighborhoods[i]);
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Precipitate))
    {
      PrecipitateStatsData::Pointer pp = std::dynamic_pointer_cast<PrecipitateStatsData>(statsDataArray[i]);
      m_DistributionAnalysis[m_NeighborhoodDistributionFitType]->calculateCorrelatedParameters(values[i], neighborhoods[i]);
      pp->setFeatureSize_Clustering(neighborhoods[i]);
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Transformation))
    {
      TransformationStatsData::Pointer tp = std::dynamic_pointer_cast<TransformationStatsData>(statsDataArray[i]);
      m_DistributionAnalysis[m_NeighborhoodDistributionFitType]->calculateCorrelatedParameters(values[i], neighborhoods[i]);
      tp->setFeatureSize_Neighbors(neighborhoods[i]);
    }
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::gatherODFStats()
{
  StatsDataArray& statsDataArray = *(m_StatsDataArray);
  std::vector<LaueOps::Pointer> m_OrientationOps = LaueOps::GetAllOrientationOps();
  size_t bin = 0;
  size_t numfeatures = m_FeatureEulerAnglesPtr.lock()->getNumberOfTuples();
  size_t numensembles = m_PhaseTypesPtr.lock()->getNumberOfTuples();
  int32_t phase = 0;
  std::vector<float> totalvol;
  std::vector<FloatArrayType::Pointer> eulerodf;

  totalvol.resize(numensembles);
  eulerodf.resize(numensembles);
  uint64_t dims = 0;
  for(size_t i = 1; i < numensembles; i++)
  {
    totalvol[i] = 0;
    uint32_t laueClass = m_CrystalStructures[i];
    if(laueClass == EbsdLib::CrystalStructure::Hexagonal_High)
    {
      dims = 36 * 36 * 12;
      eulerodf[i] = FloatArrayType::CreateArray(dims, SIMPL::StringConstants::ODF, true);
      for(uint64_t j = 0; j < dims; j++)
      {
        eulerodf[i]->setValue(j, 0.0);
      }
    }
    else if(laueClass == EbsdLib::CrystalStructure::Cubic_High)
    {
      dims = 18 * 18 * 18;
      eulerodf[i] = FloatArrayType::CreateArray(dims, SIMPL::StringConstants::ODF, true);
      for(uint64_t j = 0; j < dims; j++)
      {
        eulerodf[i]->setValue(j, 0.0);
      }
    }
    else
    {
      QString errorMessage;
      QTextStream out(&errorMessage);
      out << "The option 'Calculate Crystallographic Statistics' only works with Laue classes [Cubic m3m] and [Hexagonal 6/mmm]. ";
      out << "The offending phase was " << i << " with a value of " << QString::fromStdString(m_OrientationOps[laueClass]->getSymmetryName());
      out << ".\nThe following Laue classes were also found [Phase #] Laue Class:\n";
      for(size_t e = 1; e < numensembles; e++)
      {
        uint32_t lc = m_CrystalStructures[e];
        out << "  [" << QString::number(e) << "] " << QString::fromStdString(m_OrientationOps[lc]->getSymmetryName());
        if(e < numensembles - 1)
        {
          out << "\n";
        }
      }
      setErrorCondition(-3015, errorMessage);
      return;
    }
  }
  for(size_t i = 1; i < numfeatures; i++)
  {
    if(!m_SurfaceFeatures[i])
    {
      totalvol[m_FeaturePhases[i]] = totalvol[m_FeaturePhases[i]] + m_Volumes[i];
    }
  }
  for(size_t i = 1; i < numfeatures; i++)
  {
    if(!m_SurfaceFeatures[i])
    {
      phase = m_CrystalStructures[m_FeaturePhases[i]];
      Orientation<float> eu(m_AxisEulerAngles[3 * i], m_FeatureEulerAngles[3 * i + 1], m_FeatureEulerAngles[3 * i + 2]);

      Orientation<double> rod = OrientationTransformation::eu2ro<Orientation<float>, Orientation<double>>(eu);
      bin = m_OrientationOps[phase]->getOdfBin(rod);
      eulerodf[m_FeaturePhases[i]]->setValue(bin, (eulerodf[m_FeaturePhases[i]]->getValue(bin) + (m_Volumes[i] / totalvol[m_FeaturePhases[i]])));
    }
  }
  for(size_t i = 1; i < numensembles; i++)
  {
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Primary))
    {
      PrimaryStatsData::Pointer pp = std::dynamic_pointer_cast<PrimaryStatsData>(statsDataArray[i]);
      pp->setODF(eulerodf[i]);
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Precipitate))
    {
      PrecipitateStatsData::Pointer pp = std::dynamic_pointer_cast<PrecipitateStatsData>(statsDataArray[i]);
      pp->setODF(eulerodf[i]);
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Transformation))
    {
      TransformationStatsData::Pointer tp = std::dynamic_pointer_cast<TransformationStatsData>(statsDataArray[i]);
      tp->setODF(eulerodf[i]);
    }
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::gatherMDFStats()
{
  StatsDataArray& statsDataArray = *(m_StatsDataArray);
  std::vector<LaueOps::Pointer> m_OrientationOps = LaueOps::GetAllOrientationOps();
  // But since a pointer is difficult to use operators with we will now create a
  // reference variable to the pointer with the correct variable name that allows
  // us to use the same syntax as the "vector of vectors"
  NeighborList<int32_t>& neighborlist = *(m_NeighborList.lock());
  // And we do the same for the SharedSurfaceArea list
  NeighborList<float>& neighborsurfacearealist = *(m_SharedSurfaceAreaList.lock());

  float n1 = 0.0f, n2 = 0.0f, n3 = 0.0f;
  int32_t mbin = 0;
  float w = 0.0f;

  float* currentAvgQuatPtr = nullptr;

  size_t numfeatures = m_FeaturePhasesPtr.lock()->getNumberOfTuples();
  size_t numensembles = m_PhaseTypesPtr.lock()->getNumberOfTuples();
  uint32_t phase1 = 0, phase2 = 0;
  QVector<float> totalSurfaceArea;
  QVector<FloatArrayType::Pointer> misobin;
  int32_t numbins = 0;

  misobin.resize(numensembles);
  totalSurfaceArea.resize(numensembles);
  for(size_t i = 1; i < numensembles; ++i)
  {
    totalSurfaceArea[i] = 0;
    if(EbsdLib::CrystalStructure::Hexagonal_High == m_CrystalStructures[i])
    {
      numbins = 36 * 36 * 12;
      misobin[i] = FloatArrayType::CreateArray(numbins, SIMPL::StringConstants::MisorientationBins, true);
    }
    else if(EbsdLib::CrystalStructure::Cubic_High == m_CrystalStructures[i])
    {
      numbins = 18 * 18 * 18;
      misobin[i] = FloatArrayType::CreateArray(numbins, SIMPL::StringConstants::MisorientationBins, true);
    }
    // Now initialize all bins to 0.0
    for(int32_t j = 0; j < numbins; j++)
    {
      misobin[i]->setValue(j, 0.0);
    }
  }
  int32_t nname = 0;
  float nsa = 0.0f;
  for(size_t i = 1; i < numfeatures; i++)
  {
    currentAvgQuatPtr = m_AvgQuatsPtr.lock()->getTuplePointer(i);

    QuatF q1(currentAvgQuatPtr[0], currentAvgQuatPtr[1], currentAvgQuatPtr[2], currentAvgQuatPtr[3]);
    phase1 = m_CrystalStructures[m_FeaturePhases[i]];
    for(size_t j = 0; j < neighborlist[i].size(); j++)
    {
      w = 10000.0f;
      nname = neighborlist[i][j];
      currentAvgQuatPtr = m_AvgQuatsPtr.lock()->getTuplePointer(nname);

      QuatF q2(currentAvgQuatPtr[0], currentAvgQuatPtr[1], currentAvgQuatPtr[2], currentAvgQuatPtr[3]);

      phase2 = m_CrystalStructures[m_FeaturePhases[nname]];
      if(phase1 == phase2)
      {
        OrientationD axisAngle = m_OrientationOps[phase1]->calculateMisorientation(q1, q2);
        w = axisAngle[3];
      }
      if(phase1 == phase2)
      {
        Orientation<double> rod = OrientationTransformation::ax2ro<OrientationF, OrientationD>(OrientationF(n1, n2, n3, w));

        if((nname > i || m_SurfaceFeatures[nname]))
        {
          mbin = m_OrientationOps[phase1]->getMisoBin(rod);
          nsa = neighborsurfacearealist[i][j];
          misobin[m_FeaturePhases[i]]->setValue(mbin, (misobin[m_FeaturePhases[i]]->getValue(mbin) + nsa));
          totalSurfaceArea[m_FeaturePhases[i]] = totalSurfaceArea[m_FeaturePhases[i]] + nsa;
        }
      }
    }
  }

  for(size_t i = 1; i < numensembles; i++)
  {
    for(size_t j = 0; j < misobin[i]->getSize(); j++)
    {
      misobin[i]->setValue(j, (misobin[i]->getValue(j) / totalSurfaceArea[i]));
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Primary))
    {
      PrimaryStatsData::Pointer pp = std::dynamic_pointer_cast<PrimaryStatsData>(statsDataArray[i]);
      pp->setMisorientationBins(misobin[i]);
      pp->setBoundaryArea(totalSurfaceArea[i]);
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Precipitate))
    {
      PrecipitateStatsData::Pointer pp = std::dynamic_pointer_cast<PrecipitateStatsData>(statsDataArray[i]);
      pp->setMisorientationBins(misobin[i]);
      pp->setBoundaryArea(totalSurfaceArea[i]);
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Transformation))
    {
      TransformationStatsData::Pointer tp = std::dynamic_pointer_cast<TransformationStatsData>(statsDataArray[i]);
      tp->setMisorientationBins(misobin[i]);
      tp->setBoundaryArea(totalSurfaceArea[i]);
    }
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::gatherAxisODFStats()
{
  StatsDataArray& statsDataArray = *(m_StatsDataArray);
  std::vector<LaueOps::Pointer> m_OrientationOps = LaueOps::GetAllOrientationOps();
  int32_t bin = 0;
  QVector<FloatArrayType::Pointer> axisodf;
  QVector<float> totalaxes;
  size_t numfeatures = m_AxisEulerAnglesPtr.lock()->getNumberOfTuples();
  size_t numXTals = m_PhaseTypesPtr.lock()->getNumberOfTuples();
  axisodf.resize(numXTals);
  totalaxes.resize(numXTals);
  for(size_t i = 1; i < numXTals; i++)
  {
    totalaxes[i] = 0.0;
    axisodf[i] = FloatArrayType::CreateArray((36 * 36 * 36), SIMPL::StringConstants::AxisOrientation, true);
    for(int32_t j = 0; j < (36 * 36 * 36); j++)
    {
      axisodf[i]->setValue(j, 0.0);
    }
  }
  for(size_t i = 1; i < numfeatures; i++)
  {
    if(!m_BiasedFeatures[i])
    {
      totalaxes[m_FeaturePhases[i]]++;
    }
  }
  for(size_t i = 1; i < numfeatures; i++)
  {
    if(!m_BiasedFeatures[i])
    {

      Orientation<float> eu(m_AxisEulerAngles + 3 * i, 3); // Wrap the pointer
      Orientation<double> rod = OrientationTransformation::eu2ro<Orientation<float>, Orientation<double>>(eu);

      m_OrientationOps[EbsdLib::CrystalStructure::OrthoRhombic]->getODFFZRod(rod);
      bin = m_OrientationOps[EbsdLib::CrystalStructure::OrthoRhombic]->getOdfBin(rod);
      axisodf[m_FeaturePhases[i]]->setValue(bin, (axisodf[m_FeaturePhases[i]]->getValue(bin) + static_cast<float>((1.0 / totalaxes[m_FeaturePhases[i]]))));
    }
  }

  for(size_t i = 1; i < numXTals; i++)
  {
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Primary))
    {
      PrimaryStatsData::Pointer pp = std::dynamic_pointer_cast<PrimaryStatsData>(statsDataArray[i]);
      pp->setAxisOrientation(axisodf[i]);
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Precipitate))
    {
      PrecipitateStatsData::Pointer pp = std::dynamic_pointer_cast<PrecipitateStatsData>(statsDataArray[i]);
      pp->setAxisOrientation(axisodf[i]);
    }
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Transformation))
    {
      TransformationStatsData::Pointer tp = std::dynamic_pointer_cast<TransformationStatsData>(statsDataArray[i]);
      tp->setAxisOrientation(axisodf[i]);
    }
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::gatherRadialDistFunc()
{
  StatsDataArray& statsDataArray = *(m_StatsDataArray);

  size_t numBins = m_RadialDistFuncPtr.lock()->getNumberOfComponents();
  size_t numensembles = m_PhaseTypesPtr.lock()->getNumberOfTuples();

  for(size_t i = 1; i < numensembles; i++)
  {
    if(m_PhaseTypes[i] == static_cast<PhaseType::EnumType>(PhaseType::Type::Precipitate))
    {
      RdfData::Pointer rdfData = RdfData::New();
      std::vector<float> freqs(numBins);

      for(size_t j = 0; j < numBins; j++)
      {
        freqs[j] = m_RadialDistFunc[i * numBins + j];
      }
      rdfData->setFrequencies(freqs);

      // std::cout << "index" << i << std::endl;
      // std::cout << "Rad Dist" << m_MaxMinRadialDistFunc[i * 2] << std::endl;
      // std::cout << "Rad Dist" << m_MaxMinRadialDistFunc[i * 2 + 1] << std::endl;

      rdfData->setMaxDistance(m_MaxMinRadialDistFunc[i * 2]);
      rdfData->setMinDistance(m_MaxMinRadialDistFunc[i * 2 + 1]);

      PrecipitateStatsData::Pointer pp = std::dynamic_pointer_cast<PrecipitateStatsData>(statsDataArray[i]);
      if(nullptr == pp)
      {
        Q_ASSERT_X(nullptr != pp, "StatsDataArray could not be down-cast to a PrecipitatesStatsDataArray", "");
      }
      pp->setRadialDistFunction(rdfData);
    }
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::calculatePPTBoundaryFrac()
{
  StatsDataArray& statsDataArray = *(m_StatsDataArray);

  NeighborList<int32_t>& neighborlist = *(m_NeighborList.lock());
  size_t numensembles = m_PhaseTypesPtr.lock()->getNumberOfTuples();
  size_t numfeatures = m_FeaturePhasesPtr.lock()->getNumberOfTuples();
  std::vector<int32_t> boundaryPPT(numensembles, 0);
  std::vector<int32_t> totalNumPPT(numensembles, 0);
  std::vector<float> PPTBoundaryFrac(numensembles, 0);
  int32_t count = 0;

  for(size_t k = 1; k < numensembles; k++)
  {
    if(m_PhaseTypes[k] == static_cast<PhaseType::EnumType>(PhaseType::Type::Precipitate))
    {
      for(size_t i = 1; i < numfeatures; i++)
      {
        if(m_FeaturePhases[i] == k)
        {
          totalNumPPT[k]++;

          for(size_t j = 0; j < neighborlist[i].size(); j++)
          {
            if(m_FeaturePhases[i] != m_FeaturePhases[neighborlist[i][j]] &&
               m_PhaseTypes[m_FeaturePhases[neighborlist[i][j]]] !=
                   static_cast<PhaseType::EnumType>(PhaseType::Type::Matrix)) // Currently counts something as on the boundary if it has at least two neighbors of a different
                                                                              // non-matrix phase. Might want to specify which phase in the future.
            {
              count++;
            }
          }
          if(count >= 2)
          {
            boundaryPPT[k]++;
          }
          count = 0;
        }
      }
      PPTBoundaryFrac[k] = (float)boundaryPPT[k] / (float)totalNumPPT[k];
      PrecipitateStatsData::Pointer pp = std::dynamic_pointer_cast<PrecipitateStatsData>(statsDataArray[k]);
      pp->setPrecipBoundaryFraction(PPTBoundaryFrac[k]);
    }
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int GenerateEnsembleStatistics::getPhaseCount()
{
  DataContainerArray::Pointer dca = getDataContainerArray();
  if(nullptr == dca.get())
  {
    //  qDebug() << getNameOfClass() <<  "::getPhaseCount()  dca was nullptr";
    return -1;
  }

  AttributeMatrix::Pointer inputAttrMat = dca->getAttributeMatrix(getCellEnsembleAttributeMatrixPath());
  if(nullptr == inputAttrMat.get())
  {
    //  qDebug() << getNameOfClass() << "::getPhaseCount()  CellEnsembleAttributeMatrix was nullptr";
    //  qDebug() << "     " << getCellEnsembleAttributeMatrixPath().serialize("/");
    return -2;
  }

  if(inputAttrMat->getType() < AttributeMatrix::Type::VertexEnsemble || inputAttrMat->getType() > AttributeMatrix::Type::CellEnsemble)
  {
    //  qDebug() << getNameOfClass() << "::getPhaseCount() CellEnsembleAttributeMatrix was not correct Type";
    //  qDebug() << "     " << getCellEnsembleAttributeMatrixPath().serialize("/");
    //  qDebug() << "     " << (int)(inputAttrMat->getType());
    return -3;
  }

  // qDebug() << getNameOfClass() << "::getPhaseCount() data->getNumberOfTuples(): " << inputAttrMat->getTupleDimensions();
  // qDebug() << "Name" << inputAttrMat->getName();
  std::vector<size_t> tupleDims = inputAttrMat->getTupleDimensions();

  size_t phaseCount = 1;
  for(const auto& tupleDim : tupleDims)
  {
    phaseCount = phaseCount * tupleDim;
  }
  return phaseCount;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::execute()
{
  dataCheck();
  if(getErrorCode() < 0)
  {
    return;
  }

  size_t totalEnsembles = m_PhaseTypesPtr.lock()->getNumberOfTuples();

  // Check to see if the user has over ridden the phase types for this filter
  if(!m_PhaseTypeData.empty())
  {
    if(static_cast<int32_t>(m_PhaseTypeData.size()) < totalEnsembles)
    {
      setErrorCondition(-3013, "The number of phase types entered is less than the number of Ensembles");
      return;
    }
    if(static_cast<int32_t>(m_PhaseTypeData.size()) > totalEnsembles)
    {
      QString ss = QObject::tr("The number of phase types entered is more than the number of Ensembles. Only the first %1 will be used").arg(totalEnsembles - 1);
      setErrorCondition(-3014, ss);
    }
    for(int32_t r = 0; r < totalEnsembles; ++r)
    {
      m_PhaseTypes[r] = static_cast<PhaseType::EnumType>(m_PhaseTypeData[r]);
    }
    m_StatsDataArray->fillArrayWithNewStatsData(m_PhaseTypesPtr.lock()->getNumberOfTuples(), m_PhaseTypes);
  }

  if(m_ComputeSizeDistribution)
  {
    gatherSizeStats();
  }
  if(m_ComputeAspectRatioDistribution)
  {
    gatherAspectRatioStats();
  }
  if(m_ComputeOmega3Distribution)
  {
    gatherOmega3Stats();
  }
  if(m_ComputeNeighborhoodDistribution)
  {
    gatherNeighborhoodStats();
  }
  if(m_CalculateODF)
  {
    gatherODFStats();
  }
  if(getErrorCode() < 0)
  {
    return;
  }
  if(m_CalculateMDF)
  {
    gatherMDFStats();
  }
  if(m_CalculateAxisODF)
  {
    gatherAxisODFStats();
  }
  if(m_IncludeRadialDistFunc)
  {
    gatherRadialDistFunc();
  }

  calculatePPTBoundaryFrac();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AbstractFilter::Pointer GenerateEnsembleStatistics::newFilterInstance(bool copyFilterParameters) const
{
  GenerateEnsembleStatistics::Pointer filter = GenerateEnsembleStatistics::New();
  if(copyFilterParameters)
  {
    copyFilterParameterInstanceVariables(filter.get());
    // Here we need to set all sorts of stuff that is going to get missed. This
    // is predominantly for FilterParameters that take multiple SIMPL_FILTER_PARAMETERS
    // Those will get missed.
    filter->setPhaseTypeData(getPhaseTypeData());
    filter->setCellEnsembleAttributeMatrixPath(getCellEnsembleAttributeMatrixPath());
  }
  return filter;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString GenerateEnsembleStatistics::getCompiledLibraryName() const
{
  return StatsToolboxConstants::StatsToolboxBaseName;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString GenerateEnsembleStatistics::getBrandingString() const
{
  return "Statistics";
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString GenerateEnsembleStatistics::getFilterVersion() const
{
  QString version;
  QTextStream vStream(&version);
  vStream << StatsToolbox::Version::Major() << "." << StatsToolbox::Version::Minor() << "." << StatsToolbox::Version::Patch();
  return version;
}
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString GenerateEnsembleStatistics::getGroupName() const
{
  return SIMPL::FilterGroups::StatisticsFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QUuid GenerateEnsembleStatistics::getUuid() const
{
  return QUuid("{19a1cb76-6b46-528d-b629-1af5f1d6344c}");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString GenerateEnsembleStatistics::getSubGroupName() const
{
  return SIMPL::FilterSubGroups::EnsembleStatsFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString GenerateEnsembleStatistics::getHumanLabel() const
{
  return "Generate Ensemble Statistics";
}

// -----------------------------------------------------------------------------
GenerateEnsembleStatistics::Pointer GenerateEnsembleStatistics::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
std::shared_ptr<GenerateEnsembleStatistics> GenerateEnsembleStatistics::New()
{
  struct make_shared_enabler : public GenerateEnsembleStatistics
  {
  };
  std::shared_ptr<make_shared_enabler> val = std::make_shared<make_shared_enabler>();
  val->setupFilterParameters();
  return val;
}

// -----------------------------------------------------------------------------
QString GenerateEnsembleStatistics::getNameOfClass() const
{
  return QString("GenerateEnsembleStatistics");
}

// -----------------------------------------------------------------------------
QString GenerateEnsembleStatistics::ClassName()
{
  return QString("GenerateEnsembleStatistics");
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setCellEnsembleAttributeMatrixPath(const DataArrayPath& value)
{
  m_CellEnsembleAttributeMatrixPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getCellEnsembleAttributeMatrixPath() const
{
  return m_CellEnsembleAttributeMatrixPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setPhaseTypesArrayName(const QString& value)
{
  m_PhaseTypesArrayName = value;
}

// -----------------------------------------------------------------------------
QString GenerateEnsembleStatistics::getPhaseTypesArrayName() const
{
  return m_PhaseTypesArrayName;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setPhaseTypeArray(const PhaseType::Types& value)
{
  m_PhaseTypeArray = value;
}

// -----------------------------------------------------------------------------
PhaseType::Types GenerateEnsembleStatistics::getPhaseTypeArray() const
{
  return m_PhaseTypeArray;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setPhaseTypeData(const PhaseType::Types& value)
{
  m_PhaseTypeData = value;
}

// -----------------------------------------------------------------------------
PhaseType::Types GenerateEnsembleStatistics::getPhaseTypeData() const
{
  return m_PhaseTypeData;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setNeighborListArrayPath(const DataArrayPath& value)
{
  m_NeighborListArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getNeighborListArrayPath() const
{
  return m_NeighborListArrayPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setSharedSurfaceAreaListArrayPath(const DataArrayPath& value)
{
  m_SharedSurfaceAreaListArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getSharedSurfaceAreaListArrayPath() const
{
  return m_SharedSurfaceAreaListArrayPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setFeaturePhasesArrayPath(const DataArrayPath& value)
{
  m_FeaturePhasesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getFeaturePhasesArrayPath() const
{
  return m_FeaturePhasesArrayPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setBiasedFeaturesArrayPath(const DataArrayPath& value)
{
  m_BiasedFeaturesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getBiasedFeaturesArrayPath() const
{
  return m_BiasedFeaturesArrayPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setEquivalentDiametersArrayPath(const DataArrayPath& value)
{
  m_EquivalentDiametersArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getEquivalentDiametersArrayPath() const
{
  return m_EquivalentDiametersArrayPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setNeighborhoodsArrayPath(const DataArrayPath& value)
{
  m_NeighborhoodsArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getNeighborhoodsArrayPath() const
{
  return m_NeighborhoodsArrayPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setAspectRatiosArrayPath(const DataArrayPath& value)
{
  m_AspectRatiosArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getAspectRatiosArrayPath() const
{
  return m_AspectRatiosArrayPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setOmega3sArrayPath(const DataArrayPath& value)
{
  m_Omega3sArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getOmega3sArrayPath() const
{
  return m_Omega3sArrayPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setAxisEulerAnglesArrayPath(const DataArrayPath& value)
{
  m_AxisEulerAnglesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getAxisEulerAnglesArrayPath() const
{
  return m_AxisEulerAnglesArrayPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setCrystalStructuresArrayPath(const DataArrayPath& value)
{
  m_CrystalStructuresArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getCrystalStructuresArrayPath() const
{
  return m_CrystalStructuresArrayPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setSurfaceFeaturesArrayPath(const DataArrayPath& value)
{
  m_SurfaceFeaturesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getSurfaceFeaturesArrayPath() const
{
  return m_SurfaceFeaturesArrayPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setVolumesArrayPath(const DataArrayPath& value)
{
  m_VolumesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getVolumesArrayPath() const
{
  return m_VolumesArrayPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setRDFArrayPath(const DataArrayPath& value)
{
  m_RDFArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getRDFArrayPath() const
{
  return m_RDFArrayPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setMaxMinRDFArrayPath(const DataArrayPath& value)
{
  m_MaxMinRDFArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getMaxMinRDFArrayPath() const
{
  return m_MaxMinRDFArrayPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setFeatureEulerAnglesArrayPath(const DataArrayPath& value)
{
  m_FeatureEulerAnglesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getFeatureEulerAnglesArrayPath() const
{
  return m_FeatureEulerAnglesArrayPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setAvgQuatsArrayPath(const DataArrayPath& value)
{
  m_AvgQuatsArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GenerateEnsembleStatistics::getAvgQuatsArrayPath() const
{
  return m_AvgQuatsArrayPath;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setStatisticsArrayName(const QString& value)
{
  m_StatisticsArrayName = value;
}

// -----------------------------------------------------------------------------
QString GenerateEnsembleStatistics::getStatisticsArrayName() const
{
  return m_StatisticsArrayName;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setIncludeRadialDistFunc(bool value)
{
  m_IncludeRadialDistFunc = value;
}

// -----------------------------------------------------------------------------
bool GenerateEnsembleStatistics::getIncludeRadialDistFunc() const
{
  return m_IncludeRadialDistFunc;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setCalculateMorphologicalStats(bool value)
{
  m_CalculateMorphologicalStats = value;
}

// -----------------------------------------------------------------------------
bool GenerateEnsembleStatistics::getCalculateMorphologicalStats() const
{
  return m_CalculateMorphologicalStats;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setComputeSizeDistribution(bool value)
{
  m_ComputeSizeDistribution = value;
}

// -----------------------------------------------------------------------------
bool GenerateEnsembleStatistics::getComputeSizeDistribution() const
{
  return m_ComputeSizeDistribution;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setSizeDistributionFitType(int value)
{
  m_SizeDistributionFitType = value;
}

// -----------------------------------------------------------------------------
int GenerateEnsembleStatistics::getSizeDistributionFitType() const
{
  return m_SizeDistributionFitType;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setComputeAspectRatioDistribution(bool value)
{
  m_ComputeAspectRatioDistribution = value;
}

// -----------------------------------------------------------------------------
bool GenerateEnsembleStatistics::getComputeAspectRatioDistribution() const
{
  return m_ComputeAspectRatioDistribution;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setAspectRatioDistributionFitType(int value)
{
  m_AspectRatioDistributionFitType = value;
}

// -----------------------------------------------------------------------------
int GenerateEnsembleStatistics::getAspectRatioDistributionFitType() const
{
  return m_AspectRatioDistributionFitType;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setComputeOmega3Distribution(bool value)
{
  m_ComputeOmega3Distribution = value;
}

// -----------------------------------------------------------------------------
bool GenerateEnsembleStatistics::getComputeOmega3Distribution() const
{
  return m_ComputeOmega3Distribution;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setOmega3DistributionFitType(int value)
{
  m_Omega3DistributionFitType = value;
}

// -----------------------------------------------------------------------------
int GenerateEnsembleStatistics::getOmega3DistributionFitType() const
{
  return m_Omega3DistributionFitType;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setComputeNeighborhoodDistribution(bool value)
{
  m_ComputeNeighborhoodDistribution = value;
}

// -----------------------------------------------------------------------------
bool GenerateEnsembleStatistics::getComputeNeighborhoodDistribution() const
{
  return m_ComputeNeighborhoodDistribution;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setNeighborhoodDistributionFitType(int value)
{
  m_NeighborhoodDistributionFitType = value;
}

// -----------------------------------------------------------------------------
int GenerateEnsembleStatistics::getNeighborhoodDistributionFitType() const
{
  return m_NeighborhoodDistributionFitType;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setCalculateCrystallographicStats(bool value)
{
  m_CalculateCrystallographicStats = value;
}

// -----------------------------------------------------------------------------
bool GenerateEnsembleStatistics::getCalculateCrystallographicStats() const
{
  return m_CalculateCrystallographicStats;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setCalculateODF(bool value)
{
  m_CalculateODF = value;
}

// -----------------------------------------------------------------------------
bool GenerateEnsembleStatistics::getCalculateODF() const
{
  return m_CalculateODF;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setCalculateMDF(bool value)
{
  m_CalculateMDF = value;
}

// -----------------------------------------------------------------------------
bool GenerateEnsembleStatistics::getCalculateMDF() const
{
  return m_CalculateMDF;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setCalculateAxisODF(bool value)
{
  m_CalculateAxisODF = value;
}

// -----------------------------------------------------------------------------
bool GenerateEnsembleStatistics::getCalculateAxisODF() const
{
  return m_CalculateAxisODF;
}

// -----------------------------------------------------------------------------
void GenerateEnsembleStatistics::setSizeCorrelationResolution(float value)
{
  m_SizeCorrelationResolution = value;
}

// -----------------------------------------------------------------------------
float GenerateEnsembleStatistics::getSizeCorrelationResolution() const
{
  return m_SizeCorrelationResolution;
}
