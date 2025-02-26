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

#include "NeighborOrientationCorrelation.h"

#include <vector>

#include <QtCore/QTextStream>

#include "SIMPLib/Common/Constants.h"
#include "SIMPLib/DataContainers/DataContainer.h"
#include "SIMPLib/DataContainers/DataContainerArray.h"
#include "SIMPLib/FilterParameters/AbstractFilterParametersReader.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/FloatFilterParameter.h"
#include "SIMPLib/FilterParameters/IntFilterParameter.h"
#include "SIMPLib/FilterParameters/MultiDataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/SeparatorFilterParameter.h"
#include "SIMPLib/Geometry/ImageGeom.h"
#include "SIMPLib/Math/SIMPLibMath.h"

#include "EbsdLib/LaueOps/LaueOps.h"

#include "OrientationAnalysis/OrientationAnalysisConstants.h"
#include "OrientationAnalysis/OrientationAnalysisVersion.h"

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/task_group.h>
#include <tbb/tick_count.h>
#endif

class NeighborOrientationCorrelationTransferDataImpl
{
public:
  NeighborOrientationCorrelationTransferDataImpl() = delete;
  NeighborOrientationCorrelationTransferDataImpl(const NeighborOrientationCorrelationTransferDataImpl&) = default;

  NeighborOrientationCorrelationTransferDataImpl(NeighborOrientationCorrelation* filter, size_t totalPoints, const std::vector<int64_t>& bestNeighbor, IDataArray::Pointer dataArrayPtr)
  : m_Filter(filter)
  , m_TotalPoints(totalPoints)
  , m_BestNeighbor(bestNeighbor)
  , m_DataArrayPtr(dataArrayPtr)
  {
  }
  NeighborOrientationCorrelationTransferDataImpl(NeighborOrientationCorrelationTransferDataImpl&&) = default;                // Move Constructor Not Implemented
  NeighborOrientationCorrelationTransferDataImpl& operator=(const NeighborOrientationCorrelationTransferDataImpl&) = delete; // Copy Assignment Not Implemented
  NeighborOrientationCorrelationTransferDataImpl& operator=(NeighborOrientationCorrelationTransferDataImpl&&) = delete;      // Move Assignment Not Implemented

  ~NeighborOrientationCorrelationTransferDataImpl() = default;

  void operator()() const
  {
    size_t progIncrement = static_cast<size_t>(m_TotalPoints / 50);
    size_t prog = 1;
    for(size_t i = 0; i < m_TotalPoints; i++)
    {
      if(i > prog)
      {
        prog = prog + progIncrement;
        m_Filter->updateProgress(progIncrement);
      }
      int64_t neighbor = m_BestNeighbor[i];
      if(neighbor != -1)
      {
        // IDataArray::Pointer p = m_AttrMat->getAttributeArray(m_VoxelArrayNames[m_VoxelArrayIndex]);
        m_DataArrayPtr->copyTuple(neighbor, i);
      }
    }
  }

private:
  NeighborOrientationCorrelation* m_Filter = nullptr;
  size_t m_TotalPoints = 0;
  std::vector<int64_t> m_BestNeighbor;
  IDataArray::Pointer m_DataArrayPtr;
};

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
NeighborOrientationCorrelation::NeighborOrientationCorrelation()
{
  m_OrientationOps = LaueOps::GetAllOrientationOps();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
NeighborOrientationCorrelation::~NeighborOrientationCorrelation() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void NeighborOrientationCorrelation::setupFilterParameters()
{
  FilterParameterVectorType parameters;
  parameters.push_back(SIMPL_NEW_FLOAT_FP("Minimum Confidence Index", MinConfidence, FilterParameter::Category::Parameter, NeighborOrientationCorrelation));
  parameters.push_back(SIMPL_NEW_FLOAT_FP("Misorientation Tolerance (Degrees)", MisorientationTolerance, FilterParameter::Category::Parameter, NeighborOrientationCorrelation));
  parameters.push_back(SIMPL_NEW_INTEGER_FP("Cleanup Level", Level, FilterParameter::Category::Parameter, NeighborOrientationCorrelation));
  parameters.push_back(SeparatorFilterParameter::Create("Cell Data", FilterParameter::Category::RequiredArray));

  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Float, 1, AttributeMatrix::Type::Cell, IGeometry::Type::Image);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Confidence Index", ConfidenceIndexArrayPath, FilterParameter::Category::RequiredArray, NeighborOrientationCorrelation, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Int32, 1, AttributeMatrix::Type::Cell, IGeometry::Type::Image);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Phases", CellPhasesArrayPath, FilterParameter::Category::RequiredArray, NeighborOrientationCorrelation, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Float, 4, AttributeMatrix::Type::Cell, IGeometry::Type::Image);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Quaternions", QuatsArrayPath, FilterParameter::Category::RequiredArray, NeighborOrientationCorrelation, req));
  }
  parameters.push_back(SeparatorFilterParameter::Create("Cell Ensemble Data", FilterParameter::Category::RequiredArray));
  {
    DataArraySelectionFilterParameter::RequirementType req =
        DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::UInt32, 1, AttributeMatrix::Type::CellEnsemble, IGeometry::Type::Image);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Crystal Structures", CrystalStructuresArrayPath, FilterParameter::Category::RequiredArray, NeighborOrientationCorrelation, req));
  }
  {
    MultiDataArraySelectionFilterParameter::RequirementType req;
    parameters.push_back(SIMPL_NEW_MDA_SELECTION_FP("Attribute Arrays to Ignore", IgnoredDataArrayPaths, FilterParameter::Category::Parameter, NeighborOrientationCorrelation, req));
  }
  setFilterParameters(parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void NeighborOrientationCorrelation::initialize()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void NeighborOrientationCorrelation::dataCheck()
{
  clearErrorCode();
  clearWarningCode();

  getDataContainerArray()->getPrereqGeometryFromDataContainer<ImageGeom>(this, getConfidenceIndexArrayPath().getDataContainerName());

  QVector<DataArrayPath> dataArrayPaths;

  std::vector<size_t> cDims(1, 1);
  m_ConfidenceIndexPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<float>>(this, getConfidenceIndexArrayPath(), cDims);
  if(nullptr != m_ConfidenceIndexPtr.lock())
  {
    m_ConfidenceIndex = m_ConfidenceIndexPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */
  if(getErrorCode() >= 0)
  {
    dataArrayPaths.push_back(getConfidenceIndexArrayPath());
  }

  m_CellPhasesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<int32_t>>(this, getCellPhasesArrayPath(), cDims);
  if(nullptr != m_CellPhasesPtr.lock())
  {
    m_CellPhases = m_CellPhasesPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */
  if(getErrorCode() >= 0)
  {
    dataArrayPaths.push_back(getCellPhasesArrayPath());
  }

  m_CrystalStructuresPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<uint32_t>>(this, getCrystalStructuresArrayPath(), cDims);
  if(nullptr != m_CrystalStructuresPtr.lock())
  {
    m_CrystalStructures = m_CrystalStructuresPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */

  cDims[0] = 4;
  m_QuatsPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<float>>(this, getQuatsArrayPath(), cDims);
  if(nullptr != m_QuatsPtr.lock())
  {
    m_Quats = m_QuatsPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */
  if(getErrorCode() >= 0)
  {
    dataArrayPaths.push_back(getQuatsArrayPath());
  }

  getDataContainerArray()->validateNumberOfTuples(this, dataArrayPaths);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void NeighborOrientationCorrelation::execute()
{
  dataCheck();
  if(getErrorCode() < 0)
  {
    return;
  }

  m_Progress = 0;
  m_TotalProgress = 0;

  DataContainer::Pointer m = getDataContainerArray()->getDataContainer(m_ConfidenceIndexArrayPath.getDataContainerName());
  size_t totalPoints = m_ConfidenceIndexPtr.lock()->getNumberOfTuples();

  float misorientationToleranceR = m_MisorientationTolerance * SIMPLib::Constants::k_PiOver180F;

  SizeVec3Type udims = m->getGeometryAs<ImageGeom>()->getDimensions();

  int64_t dims[3] = {
      static_cast<int64_t>(udims[0]),
      static_cast<int64_t>(udims[1]),
      static_cast<int64_t>(udims[2]),
  };

  size_t count = 1;
  int32_t best = 0;
  bool good = true;
  bool good2 = true;
  int64_t neighbor = 0;
  int64_t neighbor2 = 0;
  int64_t column = 0, row = 0, plane = 0;

  int64_t neighpoints[6] = {0, 0, 0, 0, 0, 0};
  neighpoints[0] = static_cast<int64_t>(-dims[0] * dims[1]);
  neighpoints[1] = static_cast<int64_t>(-dims[0]);
  neighpoints[2] = static_cast<int64_t>(-1);
  neighpoints[3] = static_cast<int64_t>(1);
  neighpoints[4] = static_cast<int64_t>(dims[0]);
  neighpoints[5] = static_cast<int64_t>(dims[0] * dims[1]);

  uint32_t phase1 = 0, phase2 = 0;

  std::vector<int32_t> neighborDiffCount(totalPoints, 0);
  std::vector<int32_t> neighborSimCount(6, 0);
  std::vector<int64_t> bestNeighbor(totalPoints, -1);
  const int32_t startLevel = 6;
  float* currentQuatPtr = nullptr;

  for(int32_t currentLevel = startLevel; currentLevel > m_Level; currentLevel--)
  {
    if(getCancel())
    {
      break;
    }

    int64_t progIncrement = static_cast<int64_t>(totalPoints / 100);
    int64_t prog = 1;
    int64_t progressInt = 0;
    for(size_t i = 0; i < totalPoints; i++)
    {
      if(int64_t(i) > prog)
      {
        progressInt = static_cast<int64_t>((static_cast<float>(i) / totalPoints) * 100.0f);
        QString ss = QObject::tr("Level %1 of %2 || Processing Data %3%").arg((startLevel - currentLevel) + 1).arg(startLevel - m_Level).arg(progressInt);
        notifyStatusMessage(ss);
        prog = prog + progIncrement;
      }

      if(m_ConfidenceIndex[i] < m_MinConfidence)
      {
        count = 0;
        column = static_cast<int64_t>(i % dims[0]);
        row = (i / dims[0]) % dims[1];
        plane = i / (dims[0] * dims[1]);
        for(size_t j = 0; j < 6; j++)
        {
          good = true;
          neighbor = int64_t(i) + neighpoints[j];
          if(j == 0 && plane == 0)
          {
            good = false;
          }
          if(j == 5 && plane == (dims[2] - 1))
          {
            good = false;
          }
          if(j == 1 && row == 0)
          {
            good = false;
          }
          if(j == 4 && row == (dims[1] - 1))
          {
            good = false;
          }
          if(j == 2 && column == 0)
          {
            good = false;
          }
          if(j == 3 && column == (dims[0] - 1))
          {
            good = false;
          }
          if(good)
          {
            phase1 = m_CrystalStructures[m_CellPhases[i]];
            currentQuatPtr = m_Quats + i * 4;
            QuatF q1(currentQuatPtr[0], currentQuatPtr[1], currentQuatPtr[2], currentQuatPtr[3]);

            phase2 = m_CrystalStructures[m_CellPhases[neighbor]];
            currentQuatPtr = m_Quats + neighbor * 4;
            QuatF q2(currentQuatPtr[0], currentQuatPtr[1], currentQuatPtr[2], currentQuatPtr[3]);
            OrientationD axisAngle(0.0, 0.0, 0.0, std::numeric_limits<double>::max());
            if(m_CellPhases[i] == m_CellPhases[neighbor] && m_CellPhases[i] > 0)
            {
              axisAngle = m_OrientationOps[phase1]->calculateMisorientation(q1, q2);
            }
            if(axisAngle[3] > misorientationToleranceR)
            {
              neighborDiffCount[i]++;
            }
            for(size_t k = j + 1; k < 6; k++)
            {
              good2 = true;
              neighbor2 = int64_t(i) + neighpoints[k];
              if(k == 0 && plane == 0)
              {
                good2 = false;
              }
              if(k == 5 && plane == (dims[2] - 1))
              {
                good2 = false;
              }
              if(k == 1 && row == 0)
              {
                good2 = false;
              }
              if(k == 4 && row == (dims[1] - 1))
              {
                good2 = false;
              }
              if(k == 2 && column == 0)
              {
                good2 = false;
              }
              if(k == 3 && column == (dims[0] - 1))
              {
                good2 = false;
              }
              if(good2)
              {
                phase1 = m_CrystalStructures[m_CellPhases[neighbor2]];
                currentQuatPtr = m_Quats + neighbor2 * 4;
                q1 = QuatF(currentQuatPtr[0], currentQuatPtr[1], currentQuatPtr[2], currentQuatPtr[3]);

                phase2 = m_CrystalStructures[m_CellPhases[neighbor]];
                currentQuatPtr = m_Quats + neighbor * 4;
                q2 = QuatF(currentQuatPtr[0], currentQuatPtr[1], currentQuatPtr[2], currentQuatPtr[3]);
                OrientationD axisAngle(0.0, 0.0, 0.0, std::numeric_limits<double>::max());
                if(m_CellPhases[neighbor2] == m_CellPhases[neighbor] && m_CellPhases[neighbor2] > 0)
                {
                  axisAngle = m_OrientationOps[phase1]->calculateMisorientation(q1, q2);
                }
                if(axisAngle[3] < misorientationToleranceR)
                {
                  neighborSimCount[j]++;
                  neighborSimCount[k]++;
                }
              }
            }
          }
        }
        for(size_t j = 0; j < 6; j++)
        {
          best = 0;
          good = true;
          neighbor = int64_t(i) + neighpoints[j];
          if(j == 0 && plane == 0)
          {
            good = false;
          }
          if(j == 5 && plane == (dims[2] - 1))
          {
            good = false;
          }
          if(j == 1 && row == 0)
          {
            good = false;
          }
          if(j == 4 && row == (dims[1] - 1))
          {
            good = false;
          }
          if(j == 2 && column == 0)
          {
            good = false;
          }
          if(j == 3 && column == (dims[0] - 1))
          {
            good = false;
          }
          if(good)
          {
            if(neighborSimCount[j] > best)
            {
              best = neighborSimCount[j];
              bestNeighbor[i] = neighbor;
            }
            neighborSimCount[j] = 0;
          }
        }
      }
    }
    QString attrMatName = m_ConfidenceIndexArrayPath.getAttributeMatrixName();

    if(getCancel())
    {
      return;
    }
#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
    bool doParallel = true;
#endif

    QList<QString> voxelArrayNames = m->getAttributeMatrix(attrMatName)->getAttributeArrayNames();
    for(const auto& dataArrayPath : m_IgnoredDataArrayPaths)
    {
      voxelArrayNames.removeAll(dataArrayPath.getDataArrayName());
    }

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
    // The idea for this parallel section is to parallelize over each Data Array that
    // will need it's data adjusted. This should go faster than before by about 2x.
    // Better speed up could be achieved if we had better data locality.
    m_Progress = 0;
    m_TotalProgress = 0;
    if(doParallel)
    {
      std::shared_ptr<tbb::task_group> taskGroup(new tbb::task_group);
      AttributeMatrix* attrMat = m->getAttributeMatrix(attrMatName).get();
      m_TotalProgress = voxelArrayNames.size() * totalPoints; // Total number of points to update
      // Create and run all the tasks
      for(const auto& arrayName : voxelArrayNames)
      {
        IDataArray::Pointer dataArrayPtr = attrMat->getAttributeArray(arrayName);
        taskGroup->run(NeighborOrientationCorrelationTransferDataImpl(this, totalPoints, bestNeighbor, dataArrayPtr));
      }
      // Wait for them to complete.
      taskGroup->wait();
    }
    else
#endif
    {
      progIncrement = static_cast<int64_t>(totalPoints / 100);
      prog = 1;
      progressInt = 0;
      for(size_t i = 0; i < totalPoints; i++)
      {
        if(int64_t(i) > prog)
        {
          progressInt = static_cast<int64_t>(((float)i / totalPoints) * 100.0f);
          QString ss = QObject::tr("Level %1 of %2 || Copying Data %3%").arg((startLevel - currentLevel) + 2).arg(startLevel - m_Level).arg(progressInt);
          notifyStatusMessage(ss);
          prog = prog + progIncrement;
        }
        neighbor = bestNeighbor[i];
        if(neighbor != -1)
        {
          for(const auto& iter : voxelArrayNames)
          {
            IDataArray::Pointer p = m->getAttributeMatrix(attrMatName)->getAttributeArray(iter);
            p->copyTuple(neighbor, i);
          }
        }
      }
    }

    currentLevel = currentLevel - 1;
    m_CurrentLevel = currentLevel;
  }

  if(getCancel())
  {
    return;
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void NeighborOrientationCorrelation::updateProgress(size_t p)
{
  m_Progress += p;
  int32_t progressInt = static_cast<int>((static_cast<float>(m_Progress) / static_cast<float>(m_TotalProgress)) * 100.0f);
  QString ss = QObject::tr("Level %1 of %2 || Copying Data %3%").arg((6 - m_CurrentLevel) + 2).arg(6 - m_Level).arg(progressInt);
  notifyStatusMessage(ss);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AbstractFilter::Pointer NeighborOrientationCorrelation::newFilterInstance(bool copyFilterParameters) const
{
  NeighborOrientationCorrelation::Pointer filter = NeighborOrientationCorrelation::New();
  if(copyFilterParameters)
  {
    copyFilterParameterInstanceVariables(filter.get());
  }
  return filter;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString NeighborOrientationCorrelation::getCompiledLibraryName() const
{
  return OrientationAnalysisConstants::OrientationAnalysisBaseName;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString NeighborOrientationCorrelation::getBrandingString() const
{
  return "OrientationAnalysis";
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString NeighborOrientationCorrelation::getFilterVersion() const
{
  QString version;
  QTextStream vStream(&version);
  vStream << OrientationAnalysis::Version::Major() << "." << OrientationAnalysis::Version::Minor() << "." << OrientationAnalysis::Version::Patch();
  return version;
}
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString NeighborOrientationCorrelation::getGroupName() const
{
  return SIMPL::FilterGroups::ProcessingFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QUuid NeighborOrientationCorrelation::getUuid() const
{
  return QUuid("{6427cd5e-0ad2-5a24-8847-29f8e0720f4f}");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString NeighborOrientationCorrelation::getSubGroupName() const
{
  return SIMPL::FilterSubGroups::CleanupFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString NeighborOrientationCorrelation::getHumanLabel() const
{
  return "Neighbor Orientation Correlation";
}

// -----------------------------------------------------------------------------
NeighborOrientationCorrelation::Pointer NeighborOrientationCorrelation::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
std::shared_ptr<NeighborOrientationCorrelation> NeighborOrientationCorrelation::New()
{
  struct make_shared_enabler : public NeighborOrientationCorrelation
  {
  };
  std::shared_ptr<make_shared_enabler> val = std::make_shared<make_shared_enabler>();
  val->setupFilterParameters();
  return val;
}

// -----------------------------------------------------------------------------
QString NeighborOrientationCorrelation::getNameOfClass() const
{
  return QString("NeighborOrientationCorrelation");
}

// -----------------------------------------------------------------------------
QString NeighborOrientationCorrelation::ClassName()
{
  return QString("NeighborOrientationCorrelation");
}

// -----------------------------------------------------------------------------
void NeighborOrientationCorrelation::setMisorientationTolerance(float value)
{
  m_MisorientationTolerance = value;
}

// -----------------------------------------------------------------------------
float NeighborOrientationCorrelation::getMisorientationTolerance() const
{
  return m_MisorientationTolerance;
}

// -----------------------------------------------------------------------------
void NeighborOrientationCorrelation::setMinConfidence(float value)
{
  m_MinConfidence = value;
}

// -----------------------------------------------------------------------------
float NeighborOrientationCorrelation::getMinConfidence() const
{
  return m_MinConfidence;
}

// -----------------------------------------------------------------------------
void NeighborOrientationCorrelation::setLevel(int value)
{
  m_Level = value;
}

// -----------------------------------------------------------------------------
int NeighborOrientationCorrelation::getLevel() const
{
  return m_Level;
}

// -----------------------------------------------------------------------------
void NeighborOrientationCorrelation::setConfidenceIndexArrayPath(const DataArrayPath& value)
{
  m_ConfidenceIndexArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath NeighborOrientationCorrelation::getConfidenceIndexArrayPath() const
{
  return m_ConfidenceIndexArrayPath;
}

// -----------------------------------------------------------------------------
void NeighborOrientationCorrelation::setCellPhasesArrayPath(const DataArrayPath& value)
{
  m_CellPhasesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath NeighborOrientationCorrelation::getCellPhasesArrayPath() const
{
  return m_CellPhasesArrayPath;
}

// -----------------------------------------------------------------------------
void NeighborOrientationCorrelation::setCrystalStructuresArrayPath(const DataArrayPath& value)
{
  m_CrystalStructuresArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath NeighborOrientationCorrelation::getCrystalStructuresArrayPath() const
{
  return m_CrystalStructuresArrayPath;
}

// -----------------------------------------------------------------------------
void NeighborOrientationCorrelation::setQuatsArrayPath(const DataArrayPath& value)
{
  m_QuatsArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath NeighborOrientationCorrelation::getQuatsArrayPath() const
{
  return m_QuatsArrayPath;
}

// -----------------------------------------------------------------------------
void NeighborOrientationCorrelation::setIgnoredDataArrayPaths(const std::vector<DataArrayPath>& value)
{
  m_IgnoredDataArrayPaths = value;
}

// -----------------------------------------------------------------------------
std::vector<DataArrayPath> NeighborOrientationCorrelation::getIgnoredDataArrayPaths() const
{
  return m_IgnoredDataArrayPaths;
}
