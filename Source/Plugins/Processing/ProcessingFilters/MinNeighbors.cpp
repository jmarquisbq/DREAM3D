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
#include "MinNeighbors.h"

#include <QtCore/QTextStream>

#include "SIMPLib/Common/Constants.h"
#include "SIMPLib/DataContainers/DataContainer.h"
#include "SIMPLib/DataContainers/DataContainerArray.h"
#include "SIMPLib/FilterParameters/AbstractFilterParametersReader.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/IntFilterParameter.h"
#include "SIMPLib/FilterParameters/LinkedBooleanFilterParameter.h"
#include "SIMPLib/FilterParameters/MultiDataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/SeparatorFilterParameter.h"
#include "SIMPLib/Geometry/ImageGeom.h"

#include "Processing/ProcessingConstants.h"
#include "Processing/ProcessingVersion.h"

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
MinNeighbors::MinNeighbors() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
MinNeighbors::~MinNeighbors() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void MinNeighbors::setupFilterParameters()
{
  FilterParameterVectorType parameters;
  parameters.push_back(SIMPL_NEW_INTEGER_FP("Minimum Number Neighbors", MinNumNeighbors, FilterParameter::Category::Parameter, MinNeighbors));
  std::vector<QString> linkedProps;
  linkedProps.push_back("PhaseNumber");
  linkedProps.push_back("FeaturePhasesArrayPath");
  parameters.push_back(SIMPL_NEW_LINKED_BOOL_FP("Apply to Single Phase Only", ApplyToSinglePhase, FilterParameter::Category::Parameter, MinNeighbors, linkedProps));
  parameters.push_back(SIMPL_NEW_INTEGER_FP("Phase Index", PhaseNumber, FilterParameter::Category::Parameter, MinNeighbors));
  parameters.push_back(SeparatorFilterParameter::Create("Cell Data", FilterParameter::Category::RequiredArray));
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Int32, 1, AttributeMatrix::Type::Cell, IGeometry::Type::Image);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Feature Ids", FeatureIdsArrayPath, FilterParameter::Category::RequiredArray, MinNeighbors, req));
  }
  parameters.push_back(SeparatorFilterParameter::Create("Cell Feature Data", FilterParameter::Category::RequiredArray));
  {
    DataArraySelectionFilterParameter::RequirementType req =
        DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Int32, 1, AttributeMatrix::Type::CellFeature, IGeometry::Type::Image);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Feature Phases", FeaturePhasesArrayPath, FilterParameter::Category::RequiredArray, MinNeighbors, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req =
        DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Int32, 1, AttributeMatrix::Type::CellFeature, IGeometry::Type::Image);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Number of Neighbors", NumNeighborsArrayPath, FilterParameter::Category::RequiredArray, MinNeighbors, req));
  }
  {
    MultiDataArraySelectionFilterParameter::RequirementType req;
    parameters.push_back(SIMPL_NEW_MDA_SELECTION_FP("Attribute Arrays to Ignore", IgnoredDataArrayPaths, FilterParameter::Category::Parameter, MinNeighbors, req));
  }
  setFilterParameters(parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void MinNeighbors::initialize()
{
  m_Neighbors = nullptr;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void MinNeighbors::dataCheck()
{
  clearErrorCode();
  clearWarningCode();
  initialize();

  QVector<DataArrayPath> dataArrayPaths;

  if(getMinNumNeighbors() < 0)
  {
    QString ss = QObject::tr("The minimum number of neighbors (%1) must be 0 or positive").arg(getMinNumNeighbors());
    setErrorCondition(-5555, ss);
  }

  getDataContainerArray()->getPrereqGeometryFromDataContainer<ImageGeom>(this, getFeatureIdsArrayPath().getDataContainerName());

  std::vector<size_t> cDims(1, 1);
  m_FeatureIdsPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<int32_t>>(this, getFeatureIdsArrayPath(), cDims);
  if(nullptr != m_FeatureIdsPtr.lock())
  {
    m_FeatureIds = m_FeatureIdsPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */

  m_NumNeighborsPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<int32_t>>(this, getNumNeighborsArrayPath(), cDims);
  if(nullptr != m_NumNeighborsPtr.lock())
  {
    m_NumNeighbors = m_NumNeighborsPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */
  if(getErrorCode() >= 0)
  {
    dataArrayPaths.push_back(getNumNeighborsArrayPath());
  }

  if(getApplyToSinglePhase())
  {
    m_FeaturePhasesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<int32_t>>(this, getFeaturePhasesArrayPath(), cDims);
    if(nullptr != m_FeaturePhasesPtr.lock())
    {
      m_FeaturePhases = m_FeaturePhasesPtr.lock()->getPointer(0);
    } /* Now assign the raw pointer to data from the DataArray<T> object */
    if(getErrorCode() >= 0)
    {
      dataArrayPaths.push_back(getFeaturePhasesArrayPath());
    }
  }

  getDataContainerArray()->validateNumberOfTuples(this, dataArrayPaths);

  // Throw a warning to inform the user that the neighbor list arrays could be deleted by this filter
  QString featureIdsPath = getFeatureIdsArrayPath().getDataContainerName() + "/" + getFeatureIdsArrayPath().getAttributeMatrixName() + "/" + getFeatureIdsArrayPath().getDataArrayName();
  int err = 0;
  AttributeMatrix::Pointer featureAM = getDataContainerArray()->getPrereqAttributeMatrixFromPath(this, getNumNeighborsArrayPath(), err);
  if(nullptr == featureAM.get())
  {
    return;
  }

  QString ss = QObject::tr("If this filter modifies the Cell Level Array '%1', all arrays of type NeighborList will be deleted.  These arrays are:\n").arg(featureIdsPath);
  QList<QString> featureArrayNames = featureAM->getAttributeArrayNames();
  for(const auto& featureArrayName : featureArrayNames)
  {
    IDataArray::Pointer arr = featureAM->getAttributeArray(featureArrayName);
    QString type = arr->getTypeAsString();
    if(type.compare("NeighborList<T>") == 0)
    {
      ss.append("\n" + getNumNeighborsArrayPath().getDataContainerName() + "/" + getNumNeighborsArrayPath().getAttributeMatrixName() + "/" + arr->getName());
    }
  }

  setWarningCondition(-5556, ss);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void MinNeighbors::execute()
{
  dataCheck();
  if(getErrorCode() < 0)
  {
    return;
  }

  // If running on a single phase, validate that the user has not entered a phase number
  // that is not in the system ; the filter would not crash otherwise, but the user should
  // be notified of unanticipated behavior ; this cannot be done in the dataCheck since
  // we don't have acces to the data yet
  if(m_ApplyToSinglePhase)
  {
    AttributeMatrix::Pointer featAttrMat =
        getDataContainerArray()->getDataContainer(getFeaturePhasesArrayPath().getDataContainerName())->getAttributeMatrix(getFeaturePhasesArrayPath().getAttributeMatrixName());
    size_t numFeatures = featAttrMat->getNumberOfTuples();
    bool unavailablePhase = true;
    for(size_t i = 0; i < numFeatures; i++)
    {
      if(m_FeaturePhases[i] == m_PhaseNumber)
      {
        unavailablePhase = false;
        break;
      }
    }

    if(unavailablePhase)
    {
      QString ss = QObject::tr("The phase number (%1) is not available in the supplied Feature phases array with path (%2)").arg(m_PhaseNumber).arg(m_FeaturePhasesArrayPath.serialize());
      setErrorCondition(-5555, ss);
      return;
    }
  }

  QVector<bool> activeObjects = merge_containedfeatures();
  if(getErrorCode() < 0)
  {
    return;
  }
  assign_badpoints();

  DataContainer::Pointer m = getDataContainerArray()->getDataContainer(m_NumNeighborsArrayPath.getDataContainerName());
  AttributeMatrix::Pointer cellFeatureAttrMat = m->getAttributeMatrix(m_NumNeighborsArrayPath.getAttributeMatrixName());
  cellFeatureAttrMat->removeInactiveObjects(activeObjects, m_FeatureIdsPtr.lock().get());
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void MinNeighbors::assign_badpoints()
{
  DataContainer::Pointer m = getDataContainerArray()->getDataContainer(m_NumNeighborsArrayPath.getDataContainerName());

  size_t totalPoints = m_FeatureIdsPtr.lock()->getNumberOfTuples();
  SizeVec3Type udims = m->getGeometryAs<ImageGeom>()->getDimensions();

  int64_t dims[3] = {
      static_cast<int64_t>(udims[0]),
      static_cast<int64_t>(udims[1]),
      static_cast<int64_t>(udims[2]),
  };

  Int32ArrayType::Pointer neighborsPtr = Int32ArrayType::CreateArray(totalPoints, std::string("_INTERNAL_USE_ONLY_Neighbors"), true);
  m_Neighbors = neighborsPtr->getPointer(0);
  neighborsPtr->initializeWithValue(-1);

  int32_t good = 1;
  int32_t current = 0;
  int32_t most = 0;
  int64_t neighpoint = 0;
  size_t numfeatures = m_NumNeighborsPtr.lock()->getNumberOfTuples();

  int64_t neighpoints[6] = {0, 0, 0, 0, 0, 0};
  neighpoints[0] = -dims[0] * dims[1];
  neighpoints[1] = -dims[0];
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = dims[0];
  neighpoints[5] = dims[0] * dims[1];

  size_t counter = 1;
  int64_t count = 0;
  int64_t kstride = 0, jstride = 0;
  int32_t featurename = 0, feature = 0;
  int32_t neighbor = 0;
  QVector<int32_t> n(numfeatures + 1, 0);
  while(counter != 0)
  {
    counter = 0;
    for(int64_t k = 0; k < dims[2]; k++)
    {
      kstride = dims[0] * dims[1] * k;
      for(int64_t j = 0; j < dims[1]; j++)
      {
        jstride = dims[0] * j;
        for(int64_t i = 0; i < dims[0]; i++)
        {
          count = kstride + jstride + i;
          featurename = m_FeatureIds[count];
          if(featurename < 0)
          {
            counter++;
            current = 0;
            most = 0;
            for(int32_t l = 0; l < 6; l++)
            {
              good = 1;
              neighpoint = count + neighpoints[l];
              if(l == 0 && k == 0)
              {
                good = 0;
              }
              if(l == 5 && k == (dims[2] - 1))
              {
                good = 0;
              }
              if(l == 1 && j == 0)
              {
                good = 0;
              }
              if(l == 4 && j == (dims[1] - 1))
              {
                good = 0;
              }
              if(l == 2 && i == 0)
              {
                good = 0;
              }
              if(l == 3 && i == (dims[0] - 1))
              {
                good = 0;
              }
              if(good == 1)
              {
                feature = m_FeatureIds[neighpoint];
                if(feature >= 0)
                {
                  n[feature]++;
                  current = n[feature];
                  if(current > most)
                  {
                    most = current;
                    m_Neighbors[count] = neighpoint;
                  }
                }
              }
            }
            for(int32_t l = 0; l < 6; l++)
            {
              good = 1;
              neighpoint = count + neighpoints[l];
              if(l == 0 && k == 0)
              {
                good = 0;
              }
              if(l == 5 && k == (dims[2] - 1))
              {
                good = 0;
              }
              if(l == 1 && j == 0)
              {
                good = 0;
              }
              if(l == 4 && j == (dims[1] - 1))
              {
                good = 0;
              }
              if(l == 2 && i == 0)
              {
                good = 0;
              }
              if(l == 3 && i == (dims[0] - 1))
              {
                good = 0;
              }
              if(good == 1)
              {
                feature = m_FeatureIds[neighpoint];
                if(feature >= 0)
                {
                  n[feature] = 0;
                }
              }
            }
          }
        }
      }
    }
    QString attrMatName = m_FeatureIdsArrayPath.getAttributeMatrixName();
    QList<QString> voxelArrayNames = m->getAttributeMatrix(attrMatName)->getAttributeArrayNames();
    for(const auto& dataArrayPath : m_IgnoredDataArrayPaths)
    {
      voxelArrayNames.removeAll(dataArrayPath.getDataArrayName());
    }
    for(size_t j = 0; j < totalPoints; j++)
    {
      featurename = m_FeatureIds[j];
      neighbor = m_Neighbors[j];
      if(featurename < 0 && neighbor >= 0 && m_FeatureIds[neighbor] >= 0)
      {
        for(const auto& arrayName : voxelArrayNames)
        {
          IDataArray::Pointer p = m->getAttributeMatrix(attrMatName)->getAttributeArray(arrayName);
          p->copyTuple(neighbor, j);
        }
      }
    }
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QVector<bool> MinNeighbors::merge_containedfeatures()
{
  // Since this method is called from the 'execute' and the DataContainer validity
  // was checked there we are just going to get the Shared Pointer to the DataContainer
  DataContainer::Pointer m = getDataContainerArray()->getDataContainer(m_NumNeighborsArrayPath.getDataContainerName());

  bool good = false;

  size_t totalPoints = m->getGeometryAs<ImageGeom>()->getNumberOfElements();
  size_t totalFeatures = m_NumNeighborsPtr.lock()->getNumberOfTuples();

  QVector<bool> activeObjects(totalFeatures, true);

  for(size_t i = 1; i < totalFeatures; i++)
  {
    if(!m_ApplyToSinglePhase)
    {
      if(m_NumNeighbors[i] >= m_MinNumNeighbors)
      {
        good = true;
      }
      else
      {
        activeObjects[i] = false;
      }
    }
    else
    {
      if(m_NumNeighbors[i] >= m_MinNumNeighbors || m_FeaturePhases[i] != m_PhaseNumber)
      {
        good = true;
      }
      else
      {
        activeObjects[i] = false;
      }
    }
  }
  if(!good)
  {
    setErrorCondition(-1, "The minimum number of neighbors is larger than the Feature with the most neighbors.  All Features would be removed");
    return activeObjects;
  }
  for(size_t i = 0; i < totalPoints; i++)
  {
    int32_t featurename = m_FeatureIds[i];
    if(!activeObjects[featurename])
    {
      m_FeatureIds[i] = -1;
    }
  }
  return activeObjects;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AbstractFilter::Pointer MinNeighbors::newFilterInstance(bool copyFilterParameters) const
{
  MinNeighbors::Pointer filter = MinNeighbors::New();
  if(copyFilterParameters)
  {
    copyFilterParameterInstanceVariables(filter.get());
  }
  return filter;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString MinNeighbors::getCompiledLibraryName() const
{
  return ProcessingConstants::ProcessingBaseName;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString MinNeighbors::getBrandingString() const
{
  return "Processing";
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString MinNeighbors::getFilterVersion() const
{
  QString version;
  QTextStream vStream(&version);
  vStream << Processing::Version::Major() << "." << Processing::Version::Minor() << "." << Processing::Version::Patch();
  return version;
}
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString MinNeighbors::getGroupName() const
{
  return SIMPL::FilterGroups::ProcessingFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QUuid MinNeighbors::getUuid() const
{
  return QUuid("{dab5de3c-5f81-5bb5-8490-73521e1183ea}");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString MinNeighbors::getSubGroupName() const
{
  return SIMPL::FilterSubGroups::CleanupFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString MinNeighbors::getHumanLabel() const
{
  return "Minimum Number of Neighbors";
}

// -----------------------------------------------------------------------------
MinNeighbors::Pointer MinNeighbors::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
std::shared_ptr<MinNeighbors> MinNeighbors::New()
{
  struct make_shared_enabler : public MinNeighbors
  {
  };
  std::shared_ptr<make_shared_enabler> val = std::make_shared<make_shared_enabler>();
  val->setupFilterParameters();
  return val;
}

// -----------------------------------------------------------------------------
QString MinNeighbors::getNameOfClass() const
{
  return QString("MinNeighbors");
}

// -----------------------------------------------------------------------------
QString MinNeighbors::ClassName()
{
  return QString("MinNeighbors");
}

// -----------------------------------------------------------------------------
void MinNeighbors::setMinNumNeighbors(int value)
{
  m_MinNumNeighbors = value;
}

// -----------------------------------------------------------------------------
int MinNeighbors::getMinNumNeighbors() const
{
  return m_MinNumNeighbors;
}

// -----------------------------------------------------------------------------
void MinNeighbors::setApplyToSinglePhase(bool value)
{
  m_ApplyToSinglePhase = value;
}

// -----------------------------------------------------------------------------
bool MinNeighbors::getApplyToSinglePhase() const
{
  return m_ApplyToSinglePhase;
}

// -----------------------------------------------------------------------------
void MinNeighbors::setPhaseNumber(int value)
{
  m_PhaseNumber = value;
}

// -----------------------------------------------------------------------------
int MinNeighbors::getPhaseNumber() const
{
  return m_PhaseNumber;
}

// -----------------------------------------------------------------------------
void MinNeighbors::setFeatureIdsArrayPath(const DataArrayPath& value)
{
  m_FeatureIdsArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath MinNeighbors::getFeatureIdsArrayPath() const
{
  return m_FeatureIdsArrayPath;
}

// -----------------------------------------------------------------------------
void MinNeighbors::setFeaturePhasesArrayPath(const DataArrayPath& value)
{
  m_FeaturePhasesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath MinNeighbors::getFeaturePhasesArrayPath() const
{
  return m_FeaturePhasesArrayPath;
}

// -----------------------------------------------------------------------------
void MinNeighbors::setNumNeighborsArrayPath(const DataArrayPath& value)
{
  m_NumNeighborsArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath MinNeighbors::getNumNeighborsArrayPath() const
{
  return m_NumNeighborsArrayPath;
}

// -----------------------------------------------------------------------------
void MinNeighbors::setIgnoredDataArrayPaths(const std::vector<DataArrayPath>& value)
{
  m_IgnoredDataArrayPaths = value;
}

// -----------------------------------------------------------------------------
std::vector<DataArrayPath> MinNeighbors::getIgnoredDataArrayPaths() const
{
  return m_IgnoredDataArrayPaths;
}
