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
#include "ScalarSegmentFeatures.h"

#include <chrono>

#include <QtCore/QTextStream>

#include "SIMPLib/Common/Constants.h"
#include "SIMPLib/DataContainers/DataContainer.h"
#include "SIMPLib/DataContainers/DataContainerArray.h"
#include "SIMPLib/FilterParameters/AbstractFilterParametersReader.h"
#include "SIMPLib/FilterParameters/BooleanFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/FloatFilterParameter.h"
#include "SIMPLib/FilterParameters/LinkedBooleanFilterParameter.h"
#include "SIMPLib/FilterParameters/LinkedPathCreationFilterParameter.h"
#include "SIMPLib/FilterParameters/SeparatorFilterParameter.h"
#include "SIMPLib/FilterParameters/StringFilterParameter.h"
#include "SIMPLib/Geometry/ImageGeom.h"

#include "Reconstruction/ReconstructionConstants.h"
#include "Reconstruction/ReconstructionVersion.h"

/* Create Enumerations to allow the created Attribute Arrays to take part in renaming */
enum createdPathID : RenameDataPath::DataID_t
{
  AttributeMatrixID21 = 21,

  DataArrayID30 = 30,
  DataArrayID31 = 31,
};

/* from http://www.newty.de/fpt/functor.html */
/**
 * @brief The CompareFunctor class serves as a functor superclass for specific implementations
 * of performing scalar comparisons
 */
class CompareFunctor
{
public:
  virtual ~CompareFunctor() = default;

  virtual bool operator()(int64_t index, int64_t neighIndex, int32_t gnum) // call using () operator
  {
    return false;
  }
};

/**
 * @brief The TSpecificCompareFunctorBool class extends @see CompareFunctor to compare boolean data
 */
class TSpecificCompareFunctorBool : public CompareFunctor
{
public:
  TSpecificCompareFunctorBool(void* data, int64_t length, bool tolerance, int32_t* featureIds)
  : m_Length(length)
  , m_FeatureIds(featureIds)
  {
    m_Data = reinterpret_cast<bool*>(data);
  }
  virtual ~TSpecificCompareFunctorBool() = default;

  bool operator()(int64_t referencepoint, int64_t neighborpoint, int32_t gnum) override
  {
    // Sanity check the indices that are being passed in.
    if(referencepoint >= m_Length || neighborpoint >= m_Length)
    {
      return false;
    }

    if(m_Data[neighborpoint] == m_Data[referencepoint])
    {
      m_FeatureIds[neighborpoint] = gnum;
      return true;
    }
    return false;
  }

protected:
  TSpecificCompareFunctorBool() = default;

private:
  bool* m_Data = nullptr;          // The data that is being compared
  int64_t m_Length = 0;            // Length of the Data Array
  int32_t* m_FeatureIds = nullptr; // The Feature Ids
};

/**
 * @brief The TSpecificCompareFunctor class extens @see CompareFunctor to compare templated data
 */
template <class T>
class TSpecificCompareFunctor : public CompareFunctor
{
public:
  TSpecificCompareFunctor(void* data, int64_t length, T tolerance, int32_t* featureIds)
  : m_Length(length)
  , m_Tolerance(tolerance)
  , m_FeatureIds(featureIds)
  {
    m_Data = reinterpret_cast<T*>(data);
  }
  virtual ~TSpecificCompareFunctor() = default;

  bool operator()(int64_t referencepoint, int64_t neighborpoint, int32_t gnum) override
  {
    // Sanity check the indices that are being passed in.
    if(referencepoint >= m_Length || neighborpoint >= m_Length)
    {
      return false;
    }

    if(m_Data[referencepoint] >= m_Data[neighborpoint])
    {
      if((m_Data[referencepoint] - m_Data[neighborpoint]) <= m_Tolerance)
      {
        m_FeatureIds[neighborpoint] = gnum;
        return true;
      }
    }
    else
    {
      if((m_Data[neighborpoint] - m_Data[referencepoint]) <= m_Tolerance)
      {
        m_FeatureIds[neighborpoint] = gnum;
        return true;
      }
    }
    return false;
  }

protected:
  TSpecificCompareFunctor() = default;

private:
  T* m_Data = nullptr;               // The data that is being compared
  int64_t m_Length = 0;              // Length of the Data Array
  T m_Tolerance = static_cast<T>(0); // The tolerance of the comparison
  int32_t* m_FeatureIds = nullptr;   // The Feature Ids
};

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ScalarSegmentFeatures::ScalarSegmentFeatures() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ScalarSegmentFeatures::~ScalarSegmentFeatures() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::setupFilterParameters()
{
  SegmentFeatures::setupFilterParameters();
  FilterParameterVectorType parameters;
  std::vector<QString> linkedProps = {"GoodVoxelsArrayPath"};
  parameters.push_back(SIMPL_NEW_FLOAT_FP("Scalar Tolerance", ScalarTolerance, FilterParameter::Category::Parameter, ScalarSegmentFeatures));
  parameters.push_back(SIMPL_NEW_LINKED_BOOL_FP("Use Mask Array", UseGoodVoxels, FilterParameter::Category::Parameter, ScalarSegmentFeatures, linkedProps));
  parameters.push_back(SIMPL_NEW_BOOL_FP("Randomize Feature Ids", RandomizeFeatureIds, FilterParameter::Category::Parameter, ScalarSegmentFeatures));
  parameters.push_back(SeparatorFilterParameter::Create("Cell Data", FilterParameter::Category::RequiredArray));
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(SIMPL::Defaults::AnyPrimitive, 1, AttributeMatrix::Type::Cell, IGeometry::Type::Any);
    std::vector<IGeometry::Type> geomTypes = {IGeometry::Type::Image, IGeometry::Type::RectGrid};
    req.dcGeometryTypes = geomTypes;
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Scalar Array to Segment", ScalarArrayPath, FilterParameter::Category::RequiredArray, ScalarSegmentFeatures, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Bool, 1, AttributeMatrix::Type::Cell, IGeometry::Type::Image);
    std::vector<IGeometry::Type> geomTypes = {IGeometry::Type::Image, IGeometry::Type::RectGrid};
    req.dcGeometryTypes = geomTypes;
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Mask", GoodVoxelsArrayPath, FilterParameter::Category::RequiredArray, ScalarSegmentFeatures, req));
  }
  parameters.push_back(SeparatorFilterParameter::Create("Cell Data", FilterParameter::Category::CreatedArray));
  parameters.push_back(SIMPL_NEW_DA_WITH_LINKED_AM_FP("Feature Ids", FeatureIdsArrayName, ScalarArrayPath, ScalarArrayPath, FilterParameter::Category::CreatedArray, ScalarSegmentFeatures));
  parameters.push_back(SeparatorFilterParameter::Create("Cell Feature Data", FilterParameter::Category::CreatedArray));
  parameters.push_back(
      SIMPL_NEW_AM_WITH_LINKED_DC_FP("Cell Feature Attribute Matrix", CellFeatureAttributeMatrixName, ScalarArrayPath, FilterParameter::Category::CreatedArray, ScalarSegmentFeatures));
  parameters.push_back(SIMPL_NEW_DA_WITH_LINKED_AM_FP("Active", ActiveArrayName, ScalarArrayPath, CellFeatureAttributeMatrixName, FilterParameter::Category::CreatedArray, ScalarSegmentFeatures));
  setFilterParameters(parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::readFilterParameters(AbstractFilterParametersReader* reader, int index)
{
  reader->openFilterGroup(this, index);
  setActiveArrayName(reader->readString("ActiveArrayName", getActiveArrayName()));
  setCellFeatureAttributeMatrixName(reader->readString("CellFeatureAttributeMatrixName", getCellFeatureAttributeMatrixName()));
  setFeatureIdsArrayName(reader->readString("FeatureIdsArrayName", getFeatureIdsArrayName()));
  setGoodVoxelsArrayPath(reader->readDataArrayPath("GoodVoxelsArrayPath", getGoodVoxelsArrayPath()));
  setUseGoodVoxels(reader->readValue("UseGoodVoxels", getUseGoodVoxels()));
  setScalarArrayPath(reader->readDataArrayPath("ScalarArrayPath", getScalarArrayPath()));
  setScalarTolerance(reader->readValue("ScalarTolerance", getScalarTolerance()));
  reader->closeFilterGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::updateFeatureInstancePointers()
{
  clearErrorCode();
  clearWarningCode();

  if(nullptr != m_ActivePtr.lock())
  {
    m_Active = m_ActivePtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::initialize()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::dataCheck()
{
  clearErrorCode();
  clearWarningCode();
  DataArrayPath tempPath;

  // Set the DataContainerName for the Parent Class (SegmentFeatures) to Use
  setDataContainerName(m_ScalarArrayPath.getDataContainerName());

  SegmentFeatures::dataCheck();
  if(getErrorCode() < 0)
  {
    return;
  }

  DataContainer::Pointer m = getDataContainerArray()->getPrereqDataContainer(this, getDataContainerName(), false);
  if(getErrorCode() < 0 || nullptr == m.get())
  {
    return;
  }

  std::vector<size_t> tDims(1, 0);
  m->createNonPrereqAttributeMatrix(this, getCellFeatureAttributeMatrixName(), tDims, AttributeMatrix::Type::CellFeature, AttributeMatrixID21);

  QVector<DataArrayPath> dataArrayPaths;

  std::vector<size_t> cDims(1, 1);
  tempPath.update(getDataContainerName(), m_ScalarArrayPath.getAttributeMatrixName(), getFeatureIdsArrayName());
  m_FeatureIdsPtr = getDataContainerArray()->createNonPrereqArrayFromPath<DataArray<int32_t>>(this, tempPath, 0, cDims);
  if(nullptr != m_FeatureIdsPtr.lock())
  {
    m_FeatureIds = m_FeatureIdsPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */

  m_InputDataPtr = getDataContainerArray()->getPrereqIDataArrayFromPath(this, getScalarArrayPath());
  if(nullptr != m_InputDataPtr.lock())
  {
    m_InputData = m_InputDataPtr.lock()->getVoidPointer(0);
    if(m_InputDataPtr.lock()->getNumberOfComponents() != 1)
    {
      QString ss = QObject::tr("The selected array is not a scalar array. The number of components is %1").arg(m_InputDataPtr.lock()->getNumberOfComponents());
      setErrorCondition(-3011, ss);
    }
  }
  if(getErrorCode() >= 0)
  {
    dataArrayPaths.push_back(getScalarArrayPath());
  }

  if(m_UseGoodVoxels)
  {
    m_GoodVoxelsPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<bool>>(this, getGoodVoxelsArrayPath(), cDims);
    if(nullptr != m_GoodVoxelsPtr.lock())
    {
      m_GoodVoxels = m_GoodVoxelsPtr.lock()->getPointer(0);
    } /* Now assign the raw pointer to data from the DataArray<T> object */
    if(getErrorCode() >= 0)
    {
      dataArrayPaths.push_back(getGoodVoxelsArrayPath());
    }
  }

  tempPath.update(getDataContainerName(), getCellFeatureAttributeMatrixName(), getActiveArrayName());
  m_ActivePtr = getDataContainerArray()->createNonPrereqArrayFromPath<DataArray<bool>>(this, tempPath, true, cDims, "", DataArrayID31);
  if(nullptr != m_ActivePtr.lock())
  {
    m_Active = m_ActivePtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */

  getDataContainerArray()->validateNumberOfTuples(this, dataArrayPaths);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::randomizeFeatureIds(int64_t totalPoints, int64_t totalFeatures)
{
  notifyStatusMessage("Randomizing Feature Ids");
  // Generate an even distribution of numbers between the min and max range
  const int64_t rangeMin = 1;
  const int64_t rangeMax = totalFeatures - 1;
  initializeVoxelSeedGenerator(rangeMin, rangeMax);

  DataArray<int64_t>::Pointer rndNumbers = DataArray<int64_t>::CreateArray(totalFeatures, std::string("_INTERNAL_USE_ONLY_NewFeatureIds"), true);

  int64_t* gid = rndNumbers->getPointer(0);
  gid[0] = 0;

  for(int64_t i = 1; i < totalFeatures; ++i)
  {
    gid[i] = i;
  }

  int64_t r = 0;
  int64_t temp = 0;

  //--- Shuffle elements by randomly exchanging each with one other.
  for(int64_t i = 1; i < totalFeatures; i++)
  {
    r = m_Distribution(m_Generator); // Random remaining position.
    if(r >= totalFeatures)
    {
      continue;
    }
    temp = gid[i];
    gid[i] = gid[r];
    gid[r] = temp;
  }

  // Now adjust all the Grain Id values for each Voxel
  for(int64_t i = 0; i < totalPoints; ++i)
  {
    m_FeatureIds[i] = gid[m_FeatureIds[i]];
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int64_t ScalarSegmentFeatures::getSeed(int32_t gnum, int64_t nextSeed)
{
  clearErrorCode();
  clearWarningCode();
  DataContainer::Pointer m = getDataContainerArray()->getDataContainer(getDataContainerName());

  size_t totalPoints = m_FeatureIdsPtr.lock()->getNumberOfTuples();
  int64_t seed = -1;
  // start with the next voxel after the last seed
  size_t randpoint = static_cast<size_t>(nextSeed);
  while(seed == -1 && randpoint < totalPoints)
  {
    if(m_FeatureIds[randpoint] == 0) // If the GrainId of the voxel is ZERO then we can use this as a seed point
    {
      if(!m_UseGoodVoxels || m_GoodVoxels[randpoint])
      {
        seed = randpoint;
      }
      else
      {
        randpoint += 1;
      }
    }
    else
    {
      randpoint += 1;
    }
  }
  if(seed >= 0)
  {
    m_FeatureIds[seed] = gnum;
    std::vector<size_t> tDims(1, gnum + 1);
    m->getAttributeMatrix(getCellFeatureAttributeMatrixName())->resizeAttributeArrays(tDims);
    updateFeatureInstancePointers();
  }
  return seed;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
bool ScalarSegmentFeatures::determineGrouping(int64_t referencepoint, int64_t neighborpoint, int32_t gnum)
{
  if(m_FeatureIds[neighborpoint] == 0 && (!m_UseGoodVoxels || m_GoodVoxels[neighborpoint]))
  {
    CompareFunctor* func = m_Compare.get();
    return (*func)((size_t)(referencepoint), (size_t)(neighborpoint), gnum);
    //     | Functor  ||calling the operator() method of the CompareFunctor Class |
  }

  return false;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::initializeVoxelSeedGenerator(const int64_t rangeMin, const int64_t rangeMax)
{

  std::mt19937_64::result_type seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  m_Generator.seed(seed);
  m_Distribution = std::uniform_int_distribution<int64_t>(rangeMin, rangeMax);
  m_Distribution = std::uniform_int_distribution<int64_t>(rangeMin, rangeMax);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::execute()
{
  dataCheck();
  if(getErrorCode() < 0)
  {
    return;
  }

  DataContainer::Pointer m = getDataContainerArray()->getDataContainer(getDataContainerName());

  std::vector<size_t> tDims(1, 1);
  m->getAttributeMatrix(getCellFeatureAttributeMatrixName())->resizeAttributeArrays(tDims);
  updateFeatureInstancePointers();

  int64_t totalPoints = static_cast<int64_t>(m_FeatureIdsPtr.lock()->getNumberOfTuples());
  int64_t inDataPoints = static_cast<int64_t>(m_InputDataPtr.lock()->getNumberOfTuples());

  QString dType = m_InputDataPtr.lock()->getTypeAsString();
  if(m_InputDataPtr.lock()->getNumberOfComponents() != 1)
  {
    m_Compare = std::shared_ptr<CompareFunctor>(new CompareFunctor()); // The default CompareFunctor which ALWAYS returns false for the comparison
  }
  else if(dType.compare("int8_t") == 0)
  {
    m_Compare = std::shared_ptr<TSpecificCompareFunctor<int8_t>>(new TSpecificCompareFunctor<int8_t>(m_InputData, inDataPoints, static_cast<int8_t>(m_ScalarTolerance), m_FeatureIds));
  }
  else if(dType.compare("uint8_t") == 0)
  {
    m_Compare = std::shared_ptr<TSpecificCompareFunctor<uint8_t>>(new TSpecificCompareFunctor<uint8_t>(m_InputData, inDataPoints, static_cast<uint8_t>(m_ScalarTolerance), m_FeatureIds));
  }
  else if(dType.compare("bool") == 0)
  {
    m_Compare = std::shared_ptr<TSpecificCompareFunctorBool>(new TSpecificCompareFunctorBool(m_InputData, inDataPoints, static_cast<bool>(m_ScalarTolerance), m_FeatureIds));
  }
  else if(dType.compare("int16_t") == 0)
  {
    m_Compare = std::shared_ptr<TSpecificCompareFunctor<int16_t>>(new TSpecificCompareFunctor<int16_t>(m_InputData, inDataPoints, static_cast<int16_t>(m_ScalarTolerance), m_FeatureIds));
  }
  else if(dType.compare("uint16_t") == 0)
  {
    m_Compare = std::shared_ptr<TSpecificCompareFunctor<uint16_t>>(new TSpecificCompareFunctor<uint16_t>(m_InputData, inDataPoints, static_cast<uint16_t>(m_ScalarTolerance), m_FeatureIds));
  }
  else if(dType.compare("int32_t") == 0)
  {
    m_Compare = std::shared_ptr<TSpecificCompareFunctor<int32_t>>(new TSpecificCompareFunctor<int32_t>(m_InputData, inDataPoints, static_cast<int32_t>(m_ScalarTolerance), m_FeatureIds));
  }
  else if(dType.compare("uint32_t") == 0)
  {
    m_Compare = std::shared_ptr<TSpecificCompareFunctor<uint32_t>>(new TSpecificCompareFunctor<uint32_t>(m_InputData, inDataPoints, static_cast<uint32_t>(m_ScalarTolerance), m_FeatureIds));
  }
  else if(dType.compare("int64_t") == 0)
  {
    m_Compare = std::shared_ptr<TSpecificCompareFunctor<int64_t>>(new TSpecificCompareFunctor<int64_t>(m_InputData, inDataPoints, static_cast<int64_t>(m_ScalarTolerance), m_FeatureIds));
  }
  else if(dType.compare("uint64_t") == 0)
  {
    m_Compare = std::shared_ptr<TSpecificCompareFunctor<uint64_t>>(new TSpecificCompareFunctor<uint64_t>(m_InputData, inDataPoints, static_cast<uint64_t>(m_ScalarTolerance), m_FeatureIds));
  }
  else if(dType.compare("float") == 0)
  {
    m_Compare = std::shared_ptr<TSpecificCompareFunctor<float>>(new TSpecificCompareFunctor<float>(m_InputData, inDataPoints, m_ScalarTolerance, m_FeatureIds));
  }
  else if(dType.compare("double") == 0)
  {
    m_Compare = std::shared_ptr<TSpecificCompareFunctor<double>>(new TSpecificCompareFunctor<double>(m_InputData, inDataPoints, static_cast<double>(m_ScalarTolerance), m_FeatureIds));
  }

  // Generate the random voxel indices that will be used for the seed points to start a new grain growth/agglomeration
  const int64_t rangeMin = 0;
  const int64_t rangeMax = totalPoints - 1;
  initializeVoxelSeedGenerator(rangeMin, rangeMax);

  SegmentFeatures::execute();

  int64_t totalFeatures = static_cast<int64_t>(m_ActivePtr.lock()->getNumberOfTuples());
  if(totalFeatures < 2)
  {
    setErrorCondition(-87000, "The number of Features was 0 or 1 which means no Features were detected. A threshold value may be set too high");
    return;
  }

  // By default we randomize grains
  if(m_RandomizeFeatureIds)
  {
    totalPoints = static_cast<int64_t>(m->getGeometry()->getNumberOfElements());
    randomizeFeatureIds(totalPoints, totalFeatures);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AbstractFilter::Pointer ScalarSegmentFeatures::newFilterInstance(bool copyFilterParameters) const
{
  ScalarSegmentFeatures::Pointer filter = ScalarSegmentFeatures::New();
  if(copyFilterParameters)
  {
    copyFilterParameterInstanceVariables(filter.get());
  }
  return filter;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ScalarSegmentFeatures::getCompiledLibraryName() const
{
  return ReconstructionConstants::ReconstructionBaseName;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ScalarSegmentFeatures::getBrandingString() const
{
  return "Reconstruction";
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ScalarSegmentFeatures::getFilterVersion() const
{
  QString version;
  QTextStream vStream(&version);
  vStream << Reconstruction::Version::Major() << "." << Reconstruction::Version::Minor() << "." << Reconstruction::Version::Patch();
  return version;
}
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ScalarSegmentFeatures::getGroupName() const
{
  return SIMPL::FilterGroups::ReconstructionFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QUuid ScalarSegmentFeatures::getUuid() const
{
  return QUuid("{2c5edebf-95d8-511f-b787-90ee2adf485c}");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ScalarSegmentFeatures::getSubGroupName() const
{
  return SIMPL::FilterSubGroups::SegmentationFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString ScalarSegmentFeatures::getHumanLabel() const
{
  return "Segment Features (Scalar)";
}

// -----------------------------------------------------------------------------
ScalarSegmentFeatures::Pointer ScalarSegmentFeatures::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
std::shared_ptr<ScalarSegmentFeatures> ScalarSegmentFeatures::New()
{
  struct make_shared_enabler : public ScalarSegmentFeatures
  {
  };
  std::shared_ptr<make_shared_enabler> val = std::make_shared<make_shared_enabler>();
  val->setupFilterParameters();
  return val;
}

// -----------------------------------------------------------------------------
QString ScalarSegmentFeatures::getNameOfClass() const
{
  return QString("ScalarSegmentFeatures");
}

// -----------------------------------------------------------------------------
QString ScalarSegmentFeatures::ClassName()
{
  return QString("ScalarSegmentFeatures");
}

// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::setCellFeatureAttributeMatrixName(const QString& value)
{
  m_CellFeatureAttributeMatrixName = value;
}

// -----------------------------------------------------------------------------
QString ScalarSegmentFeatures::getCellFeatureAttributeMatrixName() const
{
  return m_CellFeatureAttributeMatrixName;
}

// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::setScalarArrayPath(const DataArrayPath& value)
{
  m_ScalarArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath ScalarSegmentFeatures::getScalarArrayPath() const
{
  return m_ScalarArrayPath;
}

// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::setScalarTolerance(float value)
{
  m_ScalarTolerance = value;
}

// -----------------------------------------------------------------------------
float ScalarSegmentFeatures::getScalarTolerance() const
{
  return m_ScalarTolerance;
}

// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::setRandomizeFeatureIds(bool value)
{
  m_RandomizeFeatureIds = value;
}

// -----------------------------------------------------------------------------
bool ScalarSegmentFeatures::getRandomizeFeatureIds() const
{
  return m_RandomizeFeatureIds;
}

// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::setUseGoodVoxels(bool value)
{
  m_UseGoodVoxels = value;
}

// -----------------------------------------------------------------------------
bool ScalarSegmentFeatures::getUseGoodVoxels() const
{
  return m_UseGoodVoxels;
}

// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::setGoodVoxelsArrayPath(const DataArrayPath& value)
{
  m_GoodVoxelsArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath ScalarSegmentFeatures::getGoodVoxelsArrayPath() const
{
  return m_GoodVoxelsArrayPath;
}

// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::setFeatureIdsArrayName(const QString& value)
{
  m_FeatureIdsArrayName = value;
}

// -----------------------------------------------------------------------------
QString ScalarSegmentFeatures::getFeatureIdsArrayName() const
{
  return m_FeatureIdsArrayName;
}

// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::setActiveArrayName(const QString& value)
{
  m_ActiveArrayName = value;
}

// -----------------------------------------------------------------------------
QString ScalarSegmentFeatures::getActiveArrayName() const
{
  return m_ActiveArrayName;
}
