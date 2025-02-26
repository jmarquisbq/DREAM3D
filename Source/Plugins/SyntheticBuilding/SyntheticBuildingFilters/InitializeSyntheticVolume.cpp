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

#include "InitializeSyntheticVolume.h"

#include <QtCore/QTextStream>

#include "SIMPLib/DataArrays/StatsDataArray.h"
#include "SIMPLib/DataArrays/StringDataArray.h"
#include "SIMPLib/DataContainers/DataContainer.h"
#include "SIMPLib/DataContainers/DataContainerArray.h"
#include "SIMPLib/FilterParameters/AbstractFilterParametersReader.h"
#include "SIMPLib/FilterParameters/ChoiceFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/DataContainerCreationFilterParameter.h"
#include "SIMPLib/FilterParameters/DataContainerSelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/FloatVec3FilterParameter.h"
#include "SIMPLib/FilterParameters/InputFileFilterParameter.h"
#include "SIMPLib/FilterParameters/IntVec3FilterParameter.h"
#include "SIMPLib/FilterParameters/LinkedBooleanFilterParameter.h"
#include "SIMPLib/FilterParameters/LinkedChoicesFilterParameter.h"
#include "SIMPLib/FilterParameters/LinkedPathCreationFilterParameter.h"
#include "SIMPLib/FilterParameters/PreflightUpdatedValueFilterParameter.h"
#include "SIMPLib/FilterParameters/SeparatorFilterParameter.h"
#include "SIMPLib/FilterParameters/StringFilterParameter.h"
#include "SIMPLib/Math/SIMPLibMath.h"
#include "SIMPLib/Math/SIMPLibRandom.h"
#include "SIMPLib/StatsData/BoundaryStatsData.h"
#include "SIMPLib/StatsData/MatrixStatsData.h"
#include "SIMPLib/StatsData/PrecipitateStatsData.h"
#include "SIMPLib/StatsData/PrimaryStatsData.h"
#include "SIMPLib/StatsData/TransformationStatsData.h"

#include "SyntheticBuilding/SyntheticBuildingConstants.h"
#include "SyntheticBuilding/SyntheticBuildingVersion.h"

#define INIT_SYNTH_VOLUME_CHECK(var, errCond)                                                                                                                                                          \
  if(m_##var <= 0)                                                                                                                                                                                     \
  {                                                                                                                                                                                                    \
    QString ss = QObject::tr("%1 must be positive").arg(#var);                                                                                                                                         \
    setErrorCondition(errCond, ss);                                                                                                                                                                    \
  }

enum createdPathID : RenameDataPath::DataID_t
{
  AttributeMatrixID21 = 21,

  DataContainerID = 1
};

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
InitializeSyntheticVolume::InitializeSyntheticVolume() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
InitializeSyntheticVolume::~InitializeSyntheticVolume() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::setupFilterParameters()
{
  FilterParameterVectorType parameters;
  std::vector<QString> linkedProps = {"EstimatedPrimaryFeatures"};
  linkedProps.push_back("InputPhaseTypesArrayPath");
  parameters.push_back(SIMPL_NEW_LINKED_BOOL_FP("Estimate Number of Features", EstimateNumberOfFeatures, FilterParameter::Category::Parameter, InitializeSyntheticVolume, linkedProps));

  PreflightUpdatedValueFilterParameter::Pointer param =
      SIMPL_NEW_PREFLIGHTUPDATEDVALUE_FP("Estimated Primary Features", EstimatedPrimaryFeatures, FilterParameter::Category::Parameter, InitializeSyntheticVolume);
  param->setReadOnly(true);
  parameters.push_back(param);

  {
    parameters.push_back(SeparatorFilterParameter::Create("Geometry Selection", FilterParameter::Category::RequiredArray));
    parameters.back()->setGroupIndices({1});
    parameters.back()->setPropertyName("Geometry Selection");
    DataContainerSelectionFilterParameter::RequirementType req;
    req.dcGeometryTypes = {IGeometry::Type::Image};
    parameters.push_back(SIMPL_NEW_DC_SELECTION_FP("Existing Geometry", GeometryDataContainer, FilterParameter::Category::RequiredArray, InitializeSyntheticVolume, req, {1}));
  }

  parameters.push_back(SeparatorFilterParameter::Create("Cell Ensemble Data", FilterParameter::Category::RequiredArray));

  {
    DataArraySelectionFilterParameter::RequirementType req =
        DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::StatsDataArray, 1, AttributeMatrix::Type::CellEnsemble, IGeometry::Type::Any);
    IGeometry::Types geomTypes;
    geomTypes.push_back(IGeometry::Type::Image);
    geomTypes.push_back(IGeometry::Type::Unknown);
    req.dcGeometryTypes = geomTypes;
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Statistics", InputStatsArrayPath, FilterParameter::Category::RequiredArray, InitializeSyntheticVolume, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req =
        DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::UInt32, 1, AttributeMatrix::Type::CellEnsemble, IGeometry::Type::Any);
    IGeometry::Types geomTypes;
    geomTypes.push_back(IGeometry::Type::Image);
    geomTypes.push_back(IGeometry::Type::Unknown);
    req.dcGeometryTypes = geomTypes;
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Phase Types", InputPhaseTypesArrayPath, FilterParameter::Category::RequiredArray, InitializeSyntheticVolume, req));
  }

  parameters.push_back(SIMPL_NEW_DC_CREATION_FP("Synthetic Volume Data Container", DataContainerName, FilterParameter::Category::CreatedArray, InitializeSyntheticVolume, {0}));
  parameters.push_back(SeparatorFilterParameter::Create("Cell Data", FilterParameter::Category::CreatedArray));
  parameters.push_back(SIMPL_NEW_AM_WITH_LINKED_DC_FP("Cell Attribute Matrix", CellAttributeMatrixName, DataContainerName, FilterParameter::Category::CreatedArray, InitializeSyntheticVolume, {0}));

  {
    LinkedChoicesFilterParameter::Pointer parameter = LinkedChoicesFilterParameter::New();
    parameter->setHumanLabel("Source of Geometry");
    parameter->setPropertyName("GeometrySelection");
    parameter->setSetterCallback(SIMPL_BIND_SETTER(InitializeSyntheticVolume, this, GeometrySelection));
    parameter->setGetterCallback(SIMPL_BIND_GETTER(InitializeSyntheticVolume, this, GeometrySelection));

    parameter->setDefaultValue(0);

    std::vector<QString> choices;
    choices.push_back("Create Geometry");
    choices.push_back("Copy Geometry");

    parameter->setChoices(choices);
    linkedProps.clear();
    linkedProps.push_back("Dimensions");
    linkedProps.push_back("Spacing");
    linkedProps.push_back("Origin");
    linkedProps.push_back("LengthUnit");
    linkedProps.push_back("GeometryDataContainer");
    linkedProps.push_back("Geometry Selection");
    parameter->setLinkedProperties(linkedProps);
    parameter->setEditable(false);
    parameter->setCategory(FilterParameter::Category::Parameter);
    parameters.push_back(parameter);
  }

  parameters.push_back(SIMPL_NEW_INT_VEC3_FP("Dimensions (Voxels)", Dimensions, FilterParameter::Category::Parameter, InitializeSyntheticVolume, {0}));
  parameters.push_back(SIMPL_NEW_FLOAT_VEC3_FP("Spacing", Spacing, FilterParameter::Category::Parameter, InitializeSyntheticVolume, {0}));
  parameters.back()->setLegacyPropertyName("Resolution");
  parameters.push_back(SIMPL_NEW_FLOAT_VEC3_FP("Origin", Origin, FilterParameter::Category::Parameter, InitializeSyntheticVolume, {0}));

  std::vector<QString> choices = IGeometry::GetAllLengthUnitStrings();
  parameters.push_back(SIMPL_NEW_CHOICE_FP("Length Units (For Description Only)", LengthUnit, FilterParameter::Category::Parameter, InitializeSyntheticVolume, choices, false, {0}));

  param = SIMPL_NEW_PREFLIGHTUPDATEDVALUE_FP("Box Size in Length Units", BoxDimensions, FilterParameter::Category::Parameter, InitializeSyntheticVolume);
  param->setReadOnly(true);
  parameters.push_back(param);

  setFilterParameters(parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::readFilterParameters(AbstractFilterParametersReader* reader, int index)
{
  reader->openFilterGroup(this, index);
  setDataContainerName(reader->readDataArrayPath("DataContainerName", getDataContainerName()));
  setCellAttributeMatrixName(reader->readString("CellAttributeMatrixName", getCellAttributeMatrixName()));
  setDimensions(reader->readIntVec3("Dimensions", getDimensions()));
  setSpacing(reader->readFloatVec3("Spacing", getSpacing()));
  setOrigin(reader->readFloatVec3("Origin", getOrigin()));
  setInputStatsArrayPath(reader->readDataArrayPath("InputStatsArrayPath", getInputStatsArrayPath()));
  setInputPhaseTypesArrayPath(reader->readDataArrayPath("InputPhaseTypesArrayPath", getInputPhaseTypesArrayPath()));
  setEstimateNumberOfFeatures(reader->readValue("EstimateNumberOfFeatures", getEstimateNumberOfFeatures()));
  reader->closeFilterGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::dataCheck()
{
  clearErrorCode();
  clearWarningCode();

  // Create the output Data Container
  DataContainer::Pointer m = getDataContainerArray()->createNonPrereqDataContainer(this, getDataContainerName(), DataContainerID);
  if(getErrorCode() < 0)
  {
    return;
  }

  ImageGeom::Pointer image = ImageGeom::CreateGeometry(SIMPL::Geometry::ImageGeometry);
  m->setGeometry(image);
  if(m_GeometrySelection == 0)
  {
    // Sanity Check the Dimensions and Spacing
    INIT_SYNTH_VOLUME_CHECK(Dimensions[0], -5000);
    INIT_SYNTH_VOLUME_CHECK(Dimensions[1], -5001);
    INIT_SYNTH_VOLUME_CHECK(Dimensions[2], -5002);
    INIT_SYNTH_VOLUME_CHECK(Spacing[0], -5003);
    INIT_SYNTH_VOLUME_CHECK(Spacing[1], -5004);
    INIT_SYNTH_VOLUME_CHECK(Spacing[2], -5005);
  }
  else
  {
    DataContainer::Pointer sourceGeomDC = getDataContainerArray()->getPrereqDataContainer(this, m_GeometryDataContainer);
    if(getErrorCode() < 0)
    {
      m_Dimensions = {0, 0, 0};
      m_Spacing = {0.0f, 0.0f, 0.0f};
      m_Origin = {0.0f, 0.0f, 0.0f};
      return;
    }
    ImageGeom::Pointer sourceGeom = sourceGeomDC->getGeometryAs<ImageGeom>();
    m_Dimensions = sourceGeom->getDimensions().convertType<int32_t>();
    m_Spacing = sourceGeom->getSpacing();
    m_Origin = sourceGeom->getOrigin();
    m_LengthUnit = static_cast<int32_t>(sourceGeom->getUnits());
  }
  // Set the Dimensions, Spacing and Origin of the output data container
  image->setDimensions(static_cast<size_t>(m_Dimensions[0]), static_cast<size_t>(m_Dimensions[1]), static_cast<size_t>(m_Dimensions[2]));
  image->setSpacing(m_Spacing);
  image->setOrigin(m_Origin);
  image->setUnits(static_cast<IGeometry::LengthUnit>(m_LengthUnit));

  // Create our output Cell and Ensemble Attribute Matrix objects
  std::vector<size_t> tDims = {static_cast<size_t>(m_Dimensions[0]), static_cast<size_t>(m_Dimensions[1]), static_cast<size_t>(m_Dimensions[2])};
  AttributeMatrix::Pointer cellAttrMat = m->createNonPrereqAttributeMatrix(this, getCellAttributeMatrixName(), tDims, AttributeMatrix::Type::Cell, AttributeMatrixID21);
  if(getErrorCode() < 0 && cellAttrMat == nullptr)
  {
    return;
  }

  if(m_EstimateNumberOfFeatures)
  {
    m_EstimatedPrimaryFeatures = estimateNumFeatures(m_Dimensions, m_Spacing);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::execute()
{
  dataCheck();
  if(getErrorCode() < 0)
  {
    return;
  }

  DataContainer::Pointer m = getDataContainerArray()->getDataContainer(getDataContainerName());
  AttributeMatrix::Pointer cellAttrMat = m->getAttributeMatrix(getCellAttributeMatrixName());

  // Resize the Cell AttributeMatrix to have the correct Tuple Dimensions.
  std::vector<size_t> tDims(3, 0);
  tDims[0] = m->getGeometryAs<ImageGeom>()->getXPoints();
  tDims[1] = m->getGeometryAs<ImageGeom>()->getYPoints();
  tDims[2] = m->getGeometryAs<ImageGeom>()->getZPoints();
  cellAttrMat->resizeAttributeArrays(tDims);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString InitializeSyntheticVolume::estimateNumFeatures(IntVec3Type dims, FloatVec3Type res)
{
  float totalvol = 0.0f;
  int32_t phase = 0;

  totalvol = (dims[0] * res[0]) * (dims[1] * res[1]) * (dims[2] * res[2]);
  if(totalvol == 0.0f)
  {
    return "-1";
  }

  DataContainerArray::Pointer dca = getDataContainerArray();

  // Get the PhaseTypes - Remember there is a Dummy PhaseType in the first slot of the array
  std::vector<size_t> cDims(1, 1); // This states that we are looking for an array with a single component
  UInt32ArrayType::Pointer phaseType = dca->getPrereqArrayFromPath<UInt32ArrayType>(nullptr, getInputPhaseTypesArrayPath(), cDims);
  if(phaseType == nullptr)
  {
    QString ss =
        QObject::tr("Phase types array could not be downcast using std::dynamic_pointer_cast<T> when estimating the number of grains. The path is %1").arg(getInputPhaseTypesArrayPath().serialize());
    setErrorCondition(-11002, ss);
    return "0";
  }
  if(!phaseType->isAllocated())
  {
    QString ss = QObject::tr("Phase types array was not allocated and could not be accessed for values. The path is %1").arg(getInputPhaseTypesArrayPath().serialize());
    setWarningCondition(-11003, ss);
    return "0";
  }

  std::vector<size_t> statsDims(1, 1);
  StatsDataArray::Pointer statsPtr = dca->getPrereqArrayFromPath<StatsDataArray>(this, getInputStatsArrayPath(), statsDims);
  if(statsPtr == nullptr)
  {
    QString ss =
        QObject::tr("Statistics array could not be downcast using std::dynamic_pointer_cast<T> when estimating the number of grains. The path is %1").arg(getInputStatsArrayPath().serialize());
    setErrorCondition(-11001, ss);
    return "0";
  }

  // Create a Reference Variable so we can use the [] syntax
  StatsDataArray& statsDataArray = *(statsPtr.get());

  std::vector<int32_t> primaryphases;
  std::vector<double> primaryphasefractions;
  double totalprimaryfractions = 0.0;
  // find which phases are primary phases
  int32_t phaseTypeNumTuples = static_cast<int32_t>(phaseType->getNumberOfTuples());
  for(int32_t i = 1; i < phaseTypeNumTuples; ++i)
  {
    if(phaseType->getValue(i) == static_cast<PhaseType::EnumType>(PhaseType::Type::Primary))
    {
      PrimaryStatsData::Pointer pp = std::dynamic_pointer_cast<PrimaryStatsData>(statsDataArray[i]);
      primaryphases.push_back(i);
      primaryphasefractions.push_back(pp->getPhaseFraction());
      totalprimaryfractions = totalprimaryfractions + pp->getPhaseFraction();
    }
  }

  // scale the primary phase fractions to total to 1
  for(size_t i = 0, total = primaryphasefractions.size(); i < total; ++i)
  {
    primaryphasefractions[i] = primaryphasefractions[i] / totalprimaryfractions;
  }

  SIMPL_RANDOMNG_NEW()
  // generate the Features
  int32_t gid = 1;

  float currentvol = 0.0f;
  float vol = 0.0f;
  float diam = 0.0f;
  bool volgood = false;
  static const float k_FourThirdsPi = 1.3333333333f * SIMPLib::Constants::k_PiF;
  for(size_t j = 0; j < primaryphases.size(); ++j)
  {
    float curphasetotalvol = totalvol * static_cast<float>(primaryphasefractions[j]);
    while(currentvol < curphasetotalvol)
    {
      volgood = false;
      phase = primaryphases[j];
      PrimaryStatsData::Pointer pp = std::dynamic_pointer_cast<PrimaryStatsData>(statsDataArray[phase]);
      if(nullptr == pp)
      {
        QString ss = QObject::tr("Tried to cast a statsDataArray[%1].get() to a PrimaryStatsData* "
                                 "pointer but this resulted in a nullptr pointer.\n")
                         .arg(phase)
                         .arg(phase);
        setErrorCondition(-666, ss);
        return "-1";
      }
      while(!volgood)
      {
        volgood = true;
        if(pp->getFeatureSize_DistType() == SIMPL::DistributionType::LogNormal)
        {
          VectorOfFloatArray fsdist = pp->getFeatureSizeDistribution();
          float avgdiam = 1.0f;
          float sddiam = 0.1f;
          if(fsdist.size() >= 2)
          {
            avgdiam = pp->getFeatureSizeDistribution().at(0)->getValue(0);
            sddiam = pp->getFeatureSizeDistribution().at(1)->getValue(0);
          }
          else
          {
            return QString("-1");
          }
          diam = rg.genrand_norm(avgdiam, sddiam);
          diam = expf(diam);
          if(diam >= pp->getMaxFeatureDiameter())
          {
            volgood = false;
          }
          if(diam < pp->getMinFeatureDiameter())
          {
            volgood = false;
          }
          float halfDiam = diam * 0.5f;
          vol = k_FourThirdsPi * (halfDiam * halfDiam * halfDiam);
        }
      }
      currentvol = currentvol + vol;
      gid++;
    }
  }
  return QString::number(gid);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString InitializeSyntheticVolume::getEstimatedPrimaryFeatures()
{
  return m_EstimatedPrimaryFeatures;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString InitializeSyntheticVolume::getBoxDimensions()
{
  QString lengthUnit = IGeometry::LengthUnitToString(static_cast<IGeometry::LengthUnit>(m_LengthUnit));
  QString desc;
  QTextStream ss(&desc);
  if(m_GeometrySelection == 1)
  {
    ss << "Dimensions: [" << m_Dimensions[0] << ", " << m_Dimensions[1] << ", " << m_Dimensions[2] << "]"
       << "\n";
    ss << "Spacing: [" << m_Spacing[0] << ", " << m_Spacing[1] << ", " << m_Spacing[2] << "]"
       << "\n";
    ss << "Origin: [" << m_Origin[0] << ", " << m_Origin[1] << ", " << m_Origin[2] << "]"
       << "\n";
  }

  ss << "X Range: " << m_Origin[0] << " to " << (m_Origin[0] + (m_Dimensions[0] * m_Spacing[0])) << " (Delta: " << (m_Dimensions[0] * m_Spacing[0]) << ") " << lengthUnit << "\n";
  ss << "Y Range: " << m_Origin[1] << " to " << (m_Origin[1] + (m_Dimensions[1] * m_Spacing[1])) << " (Delta: " << (m_Dimensions[1] * m_Spacing[1]) << ") " << lengthUnit << "\n";
  ss << "Z Range: " << m_Origin[2] << " to " << (m_Origin[2] + (m_Dimensions[2] * m_Spacing[2])) << " (Delta: " << (m_Dimensions[2] * m_Spacing[2]) << ") " << lengthUnit << "\n";

  float vol = (m_Dimensions[0] * m_Spacing[0]) * (m_Dimensions[1] * m_Spacing[1]) * (m_Dimensions[2] * m_Spacing[2]);
  QLocale usa(QLocale::English, QLocale::UnitedStates);

  ss << "Volume: " << usa.toString(vol) << " " << lengthUnit << "s ^3"
     << "\n";
  return desc;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AbstractFilter::Pointer InitializeSyntheticVolume::newFilterInstance(bool copyFilterParameters) const
{
  InitializeSyntheticVolume::Pointer filter = InitializeSyntheticVolume::New();
  if(copyFilterParameters)
  {
    copyFilterParameterInstanceVariables(filter.get());
  }
  return filter;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString InitializeSyntheticVolume::getCompiledLibraryName() const
{
  return SyntheticBuildingConstants::SyntheticBuildingBaseName;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString InitializeSyntheticVolume::getBrandingString() const
{
  return "SyntheticBuilding";
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString InitializeSyntheticVolume::getFilterVersion() const
{
  QString version;
  QTextStream vStream(&version);
  vStream << SyntheticBuilding::Version::Major() << "." << SyntheticBuilding::Version::Minor() << "." << SyntheticBuilding::Version::Patch();
  return version;
}
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString InitializeSyntheticVolume::getGroupName() const
{
  return SIMPL::FilterGroups::SyntheticBuildingFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QUuid InitializeSyntheticVolume::getUuid() const
{
  return QUuid("{c2ae366b-251f-5dbd-9d70-d790376c0c0d}");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString InitializeSyntheticVolume::getSubGroupName() const
{
  return SIMPL::FilterSubGroups::PackingFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString InitializeSyntheticVolume::getHumanLabel() const
{
  return "Initialize Synthetic Volume";
}

// -----------------------------------------------------------------------------
InitializeSyntheticVolume::Pointer InitializeSyntheticVolume::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
std::shared_ptr<InitializeSyntheticVolume> InitializeSyntheticVolume::New()
{
  struct make_shared_enabler : public InitializeSyntheticVolume
  {
  };
  std::shared_ptr<make_shared_enabler> val = std::make_shared<make_shared_enabler>();
  val->setupFilterParameters();
  return val;
}

// -----------------------------------------------------------------------------
QString InitializeSyntheticVolume::getNameOfClass() const
{
  return QString("InitializeSyntheticVolume");
}

// -----------------------------------------------------------------------------
QString InitializeSyntheticVolume::ClassName()
{
  return QString("InitializeSyntheticVolume");
}

// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::setDataContainerName(const DataArrayPath& value)
{
  m_DataContainerName = value;
}

// -----------------------------------------------------------------------------
DataArrayPath InitializeSyntheticVolume::getDataContainerName() const
{
  return m_DataContainerName;
}

// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::setCellAttributeMatrixName(const QString& value)
{
  m_CellAttributeMatrixName = value;
}

// -----------------------------------------------------------------------------
QString InitializeSyntheticVolume::getCellAttributeMatrixName() const
{
  return m_CellAttributeMatrixName;
}

// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::setEnsembleAttributeMatrixName(const QString& value)
{
  m_EnsembleAttributeMatrixName = value;
}

// -----------------------------------------------------------------------------
QString InitializeSyntheticVolume::getEnsembleAttributeMatrixName() const
{
  return m_EnsembleAttributeMatrixName;
}

// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::setLengthUnit(int32_t value)
{
  m_LengthUnit = value;
}

// -----------------------------------------------------------------------------
int32_t InitializeSyntheticVolume::getLengthUnit() const
{
  return m_LengthUnit;
}

// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::setDimensions(const IntVec3Type& value)
{
  m_Dimensions = value;
}

// -----------------------------------------------------------------------------
IntVec3Type InitializeSyntheticVolume::getDimensions() const
{
  return m_Dimensions;
}

// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::setSpacing(const FloatVec3Type& value)
{
  m_Spacing = value;
}

// -----------------------------------------------------------------------------
FloatVec3Type InitializeSyntheticVolume::getSpacing() const
{
  return m_Spacing;
}

// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::setOrigin(const FloatVec3Type& value)
{
  m_Origin = value;
}

// -----------------------------------------------------------------------------
FloatVec3Type InitializeSyntheticVolume::getOrigin() const
{
  return m_Origin;
}

// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::setInputStatsArrayPath(const DataArrayPath& value)
{
  m_InputStatsArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath InitializeSyntheticVolume::getInputStatsArrayPath() const
{
  return m_InputStatsArrayPath;
}

// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::setInputPhaseTypesArrayPath(const DataArrayPath& value)
{
  m_InputPhaseTypesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath InitializeSyntheticVolume::getInputPhaseTypesArrayPath() const
{
  return m_InputPhaseTypesArrayPath;
}

// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::setInputPhaseNamesArrayPath(const DataArrayPath& value)
{
  m_InputPhaseNamesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath InitializeSyntheticVolume::getInputPhaseNamesArrayPath() const
{
  return m_InputPhaseNamesArrayPath;
}

// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::setEstimateNumberOfFeatures(bool value)
{
  m_EstimateNumberOfFeatures = value;
}

// -----------------------------------------------------------------------------
bool InitializeSyntheticVolume::getEstimateNumberOfFeatures() const
{
  return m_EstimateNumberOfFeatures;
}

// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::setGeometrySelection(int32_t value)
{
  m_GeometrySelection = value;
}

// -----------------------------------------------------------------------------
int32_t InitializeSyntheticVolume::getGeometrySelection() const
{
  return m_GeometrySelection;
}
// -----------------------------------------------------------------------------
void InitializeSyntheticVolume::setGeometryDataContainer(const DataArrayPath& value)
{
  m_GeometryDataContainer = value;
}

// -----------------------------------------------------------------------------
DataArrayPath InitializeSyntheticVolume::getGeometryDataContainer() const
{
  return m_GeometryDataContainer;
}
