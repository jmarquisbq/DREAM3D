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

#include "IdentifySample.h"

#include <QtCore/QTextStream>

#include "SIMPLib/Common/Constants.h"
#include "SIMPLib/Common/TemplateHelpers.h"
#include "SIMPLib/DataContainers/DataContainer.h"
#include "SIMPLib/DataContainers/DataContainerArray.h"
#include "SIMPLib/FilterParameters/AbstractFilterParametersReader.h"
#include "SIMPLib/FilterParameters/BooleanFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/LinkedBooleanFilterParameter.h"
#include "SIMPLib/FilterParameters/SeparatorFilterParameter.h"
#include "SIMPLib/Geometry/ImageGeom.h"

#include "Processing/ProcessingConstants.h"
#include "Processing/ProcessingVersion.h"

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
IdentifySample::IdentifySample() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
IdentifySample::~IdentifySample() = default;
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void IdentifySample::setupFilterParameters()
{
  FilterParameterVectorType parameters;
  parameters.push_back(SIMPL_NEW_BOOL_FP("Fill Holes in Largest Feature", FillHoles, FilterParameter::Category::Parameter, IdentifySample));
  parameters.push_back(SeparatorFilterParameter::Create("Cell Data", FilterParameter::Category::RequiredArray));
  {
    DataArraySelectionFilterParameter::RequirementType req =
        DataArraySelectionFilterParameter::CreateRequirement({SIMPL::TypeNames::Bool, SIMPL::TypeNames::UInt8}, 1, AttributeMatrix::Type::Cell, IGeometry::Type::Image);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Mask", GoodVoxelsArrayPath, FilterParameter::Category::RequiredArray, IdentifySample, req));
  }
  setFilterParameters(parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void IdentifySample::readFilterParameters(AbstractFilterParametersReader* reader, int index)
{
  reader->openFilterGroup(this, index);
  setFillHoles(reader->readValue("FillHoles", getFillHoles()));
  setGoodVoxelsArrayPath(reader->readDataArrayPath("GoodVoxelsArrayPath", getGoodVoxelsArrayPath()));
  reader->closeFilterGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void IdentifySample::initialize()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void IdentifySample::dataCheck()
{
  clearErrorCode();
  clearWarningCode();

  getDataContainerArray()->getPrereqGeometryFromDataContainer<ImageGeom>(this, getGoodVoxelsArrayPath().getDataContainerName());

  m_ArrayType = 0;

  IDataArray::Pointer inputData = getDataContainerArray()->getPrereqIDataArrayFromPath(this, getGoodVoxelsArrayPath());

  if(TemplateHelpers::CanDynamicCast<DataArray<bool>>()(inputData))
  {
    m_ArrayType = 1;
  }
  else if(TemplateHelpers::CanDynamicCast<DataArray<uint8_t>>()(inputData))
  {
    m_ArrayType = 2;
  }
  else
  {
    QString ss = QObject::tr("The input data must be of type BOOL or UINT8");
    setErrorCondition(-12001, ss);
  }
}

template <typename T>
void _execute(IdentifySample* filter)
{
  using ArrayType = DataArray<T>;
  using ArrayPointerType = typename DataArray<T>::Pointer;

  DataContainerArray::Pointer dca = filter->getDataContainerArray();
  DataContainer::Pointer m = dca->getDataContainer(filter->getGoodVoxelsArrayPath().getDataContainerName());

  std::vector<size_t> cDims = {1};
  ArrayPointerType m_GoodVoxelsPtr = dca->getPrereqArrayFromPath<ArrayType>(filter, filter->getGoodVoxelsArrayPath(), cDims);
  T* m_GoodVoxels = m_GoodVoxelsPtr->getTuplePointer(0);

  int64_t totalPoints = static_cast<int64_t>(m_GoodVoxelsPtr->getNumberOfTuples());

  SizeVec3Type udims = m->getGeometryAs<ImageGeom>()->getDimensions();

  int64_t dims[3] = {
      static_cast<int64_t>(udims[0]),
      static_cast<int64_t>(udims[1]),
      static_cast<int64_t>(udims[2]),
  };

  int64_t neighpoints[6] = {0, 0, 0, 0, 0, 0};
  int64_t xp = dims[0];
  int64_t yp = dims[1];
  int64_t zp = dims[2];

  neighpoints[0] = -(xp * yp);
  neighpoints[1] = -xp;
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = xp;
  neighpoints[5] = (xp * yp);
  std::vector<int64_t> currentvlist;
  std::vector<bool> checked(totalPoints, false);
  std::vector<bool> sample(totalPoints, false);
  int64_t biggestBlock = 0;
  size_t count = 0;
  int32_t good = 0;
  int64_t neighbor = 0;
  int64_t column = 0, row = 0, plane = 0;
  int64_t index = 0;

  // In this loop over the data we are finding the biggest contiguous set of GoodVoxels and calling that the 'sample'  All GoodVoxels that do not touch the 'sample'
  // are flipped to be called 'bad' voxels or 'not sample'
  float threshold = 0.0f;
  for(int64_t i = 0; i < totalPoints; i++)
  {
    float percentIncrement = static_cast<float>(i) / static_cast<float>(totalPoints) * 100.0f;
    if(percentIncrement > threshold)
    {
      QString ss = QObject::tr("%1% Scanned").arg(static_cast<int32_t>(percentIncrement));
      filter->notifyStatusMessage(ss);
      threshold = threshold + 5.0f;
      if(threshold < percentIncrement)
      {
        threshold = percentIncrement;
      }
    }

    if(!checked[i] && m_GoodVoxels[i])
    {
      currentvlist.push_back(i);
      count = 0;
      while(count < currentvlist.size())
      {
        index = currentvlist[count];
        column = index % xp;
        row = (index / xp) % yp;
        plane = index / (xp * yp);
        for(int32_t j = 0; j < 6; j++)
        {
          good = 1;
          neighbor = index + neighpoints[j];
          if(j == 0 && plane == 0)
          {
            good = 0;
          }
          if(j == 5 && plane == (zp - 1))
          {
            good = 0;
          }
          if(j == 1 && row == 0)
          {
            good = 0;
          }
          if(j == 4 && row == (yp - 1))
          {
            good = 0;
          }
          if(j == 2 && column == 0)
          {
            good = 0;
          }
          if(j == 3 && column == (xp - 1))
          {
            good = 0;
          }
          if(good == 1 && !checked[neighbor] && m_GoodVoxels[neighbor])
          {
            currentvlist.push_back(neighbor);
            checked[neighbor] = true;
          }
        }
        count++;
      }
      if(static_cast<int64_t>(currentvlist.size()) >= biggestBlock)
      {
        biggestBlock = currentvlist.size();
        sample.assign(totalPoints, false);
        for(int64_t j = 0; j < biggestBlock; j++)
        {
          sample[currentvlist[j]] = true;
        }
      }
      currentvlist.clear();
    }
  }
  for(int64_t i = 0; i < totalPoints; i++)
  {
    if(!sample[i] && m_GoodVoxels[i])
    {
      m_GoodVoxels[i] = false;
    }
  }
  sample.clear();
  checked.assign(totalPoints, false);

  // In this loop we are going to 'close' all of the 'holes' inside of the region already identified as the 'sample' if the user chose to do so.
  // This is done by flipping all 'bad' voxel features that do not touch the outside of the sample (i.e. they are fully contained inside of the 'sample'.
  threshold = 0.0F;
  if(filter->getFillHoles())
  {
    bool touchesBoundary = false;
    for(int64_t i = 0; i < totalPoints; i++)
    {
      float percentIncrement = static_cast<float>(i) / static_cast<float>(totalPoints) * 100.0f;
      if(percentIncrement > threshold)
      {
        QString ss = QObject::tr("%1% Filling Holes").arg(static_cast<int32_t>(percentIncrement));
        filter->notifyStatusMessage(ss);
        threshold = threshold + 5.0f;
        if(threshold < percentIncrement)
        {
          threshold = percentIncrement;
        }
      }

      if(!checked[i] && !m_GoodVoxels[i])
      {
        currentvlist.push_back(i);
        count = 0;
        touchesBoundary = false;
        while(count < currentvlist.size())
        {
          index = currentvlist[count];
          column = index % xp;
          row = (index / xp) % yp;
          plane = index / (xp * yp);
          if(column == 0 || column == (xp - 1) || row == 0 || row == (yp - 1) || plane == 0 || plane == (zp - 1))
          {
            touchesBoundary = true;
          }
          for(int32_t j = 0; j < 6; j++)
          {
            good = 1;
            neighbor = index + neighpoints[j];
            if(j == 0 && plane == 0)
            {
              good = 0;
            }
            if(j == 5 && plane == (zp - 1))
            {
              good = 0;
            }
            if(j == 1 && row == 0)
            {
              good = 0;
            }
            if(j == 4 && row == (yp - 1))
            {
              good = 0;
            }
            if(j == 2 && column == 0)
            {
              good = 0;
            }
            if(j == 3 && column == (xp - 1))
            {
              good = 0;
            }
            if(good == 1 && !checked[neighbor] && !m_GoodVoxels[neighbor])
            {
              currentvlist.push_back(neighbor);
              checked[neighbor] = true;
            }
          }
          count++;
        }
        if(!touchesBoundary)
        {
          for(size_t j = 0; j < currentvlist.size(); j++)
          {
            m_GoodVoxels[currentvlist[j]] = true;
          }
        }
        currentvlist.clear();
      }
    }
  }
  checked.clear();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void IdentifySample::execute()
{
  dataCheck();
  if(getErrorCode() < 0)
  {
    return;
  }

  if(m_ArrayType == 1)
  {
    _execute<bool>(this);
  }
  if(m_ArrayType == 2)
  {
    _execute<uint8_t>(this);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AbstractFilter::Pointer IdentifySample::newFilterInstance(bool copyFilterParameters) const
{
  IdentifySample::Pointer filter = IdentifySample::New();
  if(copyFilterParameters)
  {
    copyFilterParameterInstanceVariables(filter.get());
  }
  return filter;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString IdentifySample::getCompiledLibraryName() const
{
  return ProcessingConstants::ProcessingBaseName;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString IdentifySample::getBrandingString() const
{
  return "Processing";
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString IdentifySample::getFilterVersion() const
{
  QString version;
  QTextStream vStream(&version);
  vStream << Processing::Version::Major() << "." << Processing::Version::Minor() << "." << Processing::Version::Patch();
  return version;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString IdentifySample::getGroupName() const
{
  return SIMPL::FilterGroups::ProcessingFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QUuid IdentifySample::getUuid() const
{
  return QUuid("{0e8c0818-a3fb-57d4-a5c8-7cb8ae54a40a}");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString IdentifySample::getSubGroupName() const
{
  return SIMPL::FilterSubGroups::CleanupFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString IdentifySample::getHumanLabel() const
{
  return "Isolate Largest Feature (Identify Sample)";
}

// -----------------------------------------------------------------------------
IdentifySample::Pointer IdentifySample::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
std::shared_ptr<IdentifySample> IdentifySample::New()
{
  struct make_shared_enabler : public IdentifySample
  {
  };
  std::shared_ptr<make_shared_enabler> val = std::make_shared<make_shared_enabler>();
  val->setupFilterParameters();
  return val;
}

// -----------------------------------------------------------------------------
QString IdentifySample::getNameOfClass() const
{
  return QString("IdentifySample");
}

// -----------------------------------------------------------------------------
QString IdentifySample::ClassName()
{
  return QString("IdentifySample");
}

// -----------------------------------------------------------------------------
void IdentifySample::setFillHoles(bool value)
{
  m_FillHoles = value;
}

// -----------------------------------------------------------------------------
bool IdentifySample::getFillHoles() const
{
  return m_FillHoles;
}

// -----------------------------------------------------------------------------
void IdentifySample::setGoodVoxelsArrayPath(const DataArrayPath& value)
{
  m_GoodVoxelsArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath IdentifySample::getGoodVoxelsArrayPath() const
{
  return m_GoodVoxelsArrayPath;
}
