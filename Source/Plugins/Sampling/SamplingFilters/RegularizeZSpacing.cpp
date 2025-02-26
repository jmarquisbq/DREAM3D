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
#include "RegularizeZSpacing.h"

#include <fstream>

#include <QtCore/QTextStream>

#include "SIMPLib/Common/Constants.h"
#include "SIMPLib/DataContainers/DataContainer.h"
#include "SIMPLib/DataContainers/DataContainerArray.h"
#include "SIMPLib/FilterParameters/AbstractFilterParametersReader.h"
#include "SIMPLib/FilterParameters/AttributeMatrixSelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/FloatFilterParameter.h"
#include "SIMPLib/FilterParameters/InputFileFilterParameter.h"
#include "SIMPLib/FilterParameters/SeparatorFilterParameter.h"
#include "SIMPLib/Geometry/ImageGeom.h"

#include "Sampling/SamplingConstants.h"
#include "Sampling/SamplingVersion.h"

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
RegularizeZSpacing::RegularizeZSpacing() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
RegularizeZSpacing::~RegularizeZSpacing() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void RegularizeZSpacing::setupFilterParameters()
{
  FilterParameterVectorType parameters;
  parameters.push_back(SIMPL_NEW_INPUT_FILE_FP("Current Z Positions File", InputFile, FilterParameter::Category::Parameter, RegularizeZSpacing, "*.txt"));
  parameters.push_back(SIMPL_NEW_FLOAT_FP("New Z Spacing", NewZRes, FilterParameter::Category::Parameter, RegularizeZSpacing));
  parameters.push_back(SeparatorFilterParameter::Create("Cell Data", FilterParameter::Category::RequiredArray));
  {
    AttributeMatrixSelectionFilterParameter::RequirementType req = AttributeMatrixSelectionFilterParameter::CreateRequirement(AttributeMatrix::Type::Cell, IGeometry::Type::Image);
    parameters.push_back(SIMPL_NEW_AM_SELECTION_FP("Cell Attribute Matrix", CellAttributeMatrixPath, FilterParameter::Category::RequiredArray, RegularizeZSpacing, req));
  }
  setFilterParameters(parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void RegularizeZSpacing::readFilterParameters(AbstractFilterParametersReader* reader, int index)
{
  reader->openFilterGroup(this, index);
  setCellAttributeMatrixPath(reader->readDataArrayPath("CellAttributeMatrixPath", getCellAttributeMatrixPath()));
  setInputFile(reader->readString("InputFile", getInputFile()));
  setNewZRes(reader->readValue("NewZRes", getNewZRes()));
  reader->closeFilterGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void RegularizeZSpacing::initialize()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void RegularizeZSpacing::dataCheck()
{
  clearErrorCode();
  clearWarningCode();

  if(getNewZRes() <= 0)
  {
    QString ss = QObject::tr("The new Z resolution Y (%1) must be positive").arg(getNewZRes());
    setErrorCondition(-5555, ss);
  }

  std::ifstream inFile;
  inFile.open(m_InputFile.toLatin1().data());

  if(!inFile.good())
  {
    QString ss = QObject::tr("Unable to open input file with name '%1'").arg(getInputFile());
    setErrorCondition(-5556, ss);
    return;
  }

  ImageGeom::Pointer image = getDataContainerArray()->getPrereqGeometryFromDataContainer<ImageGeom>(this, getCellAttributeMatrixPath().getDataContainerName());
  AttributeMatrix::Pointer cellAttrMat = getDataContainerArray()->getPrereqAttributeMatrixFromPath(this, getCellAttributeMatrixPath(), -301);
  if(getErrorCode() < 0)
  {
    return;
  }

  float zval = 0.0f;
  for(size_t iter = 0; iter < image->getZPoints() + 1; iter++)
  {
    inFile >> zval;
  }
  size_t zP = static_cast<size_t>(zval / getNewZRes());
  if(zP == 0)
  {
    zP = 1;
  }

  if(getInPreflight())
  {
    image->setDimensions(std::make_tuple(image->getXPoints(), image->getYPoints(), zP));
    std::vector<size_t> tDims(3, 0);
    tDims[0] = image->getXPoints();
    tDims[1] = image->getYPoints();
    tDims[2] = zP;
    cellAttrMat->resizeAttributeArrays(tDims);
  }

  inFile.close();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void RegularizeZSpacing::execute()
{
  dataCheck();
  if(getErrorCode() < 0)
  {
    return;
  }

  DataContainer::Pointer m = getDataContainerArray()->getDataContainer(getCellAttributeMatrixPath().getDataContainerName());

  SizeVec3Type dims = m->getGeometryAs<ImageGeom>()->getDimensions();

  std::ifstream inFile;
  inFile.open(m_InputFile.toLatin1().data());

  float zval = 0.0f;
  std::vector<float> zboundvalues(dims[2] + 1, 0.0);
  for(size_t iter = 0; iter < dims[2] + 1; iter++)
  {
    inFile >> zval;
    zboundvalues[iter] = zval;
  }
  inFile.close();

  FloatVec3Type spacing = m->getGeometryAs<ImageGeom>()->getSpacing();

  float sizez = zboundvalues[dims[2]];
  size_t m_XP = dims[0];
  size_t m_YP = dims[1];
  size_t m_ZP = static_cast<size_t>(sizez / m_NewZRes);
  if(m_ZP == 0)
  {
    m_ZP = 1;
  }
  size_t totalPoints = m_XP * m_YP * m_ZP;

  size_t index = 0, oldindex = 0;
  size_t plane = 0;
  std::vector<size_t> newindicies(totalPoints, 0);
  for(size_t i = 0; i < m_ZP; i++)
  {
    plane = 0;
    for(size_t iter = 1; iter < dims[2]; iter++)
    {
      if((i * m_NewZRes) > zboundvalues[iter])
      {
        plane = iter;
      }
    }
    for(size_t j = 0; j < m_YP; j++)
    {
      for(size_t k = 0; k < m_XP; k++)
      {
        oldindex = (plane * dims[0] * dims[1]) + (j * dims[0]) + k;
        index = (i * dims[0] * dims[1]) + (j * dims[0]) + k;
        newindicies[index] = oldindex;
      }
    }
  }

  AttributeMatrix::Pointer cellAttrMat = m->getAttributeMatrix(getCellAttributeMatrixPath().getAttributeMatrixName());
  std::vector<size_t> tDims(3, 0);
  tDims[0] = m_XP;
  tDims[1] = m_YP;
  tDims[2] = m_ZP;
  AttributeMatrix::Pointer newCellAttrMat = AttributeMatrix::New(tDims, cellAttrMat->getName(), cellAttrMat->getType());

  QList<QString> voxelArrayNames = cellAttrMat->getAttributeArrayNames();
  for(QList<QString>::iterator iter = voxelArrayNames.begin(); iter != voxelArrayNames.end(); ++iter)
  {
    IDataArray::Pointer p = cellAttrMat->getAttributeArray(*iter);
    // Make a copy of the 'p' array that has the same name. When placed into
    // the data container this will over write the current array with
    // the same name. At least in theory
    IDataArray::Pointer data = p->createNewArray(p->getNumberOfTuples(), p->getComponentDimensions(), p->getName());
    data->resizeTuples(totalPoints);
    void* source = nullptr;
    void* destination = nullptr;
    size_t newIndicies_I = 0;
    int nComp = data->getNumberOfComponents();
    for(size_t i = 0; i < static_cast<size_t>(totalPoints); i++)
    {
      newIndicies_I = newindicies[i];

      source = p->getVoidPointer((nComp * newIndicies_I));
      destination = data->getVoidPointer((data->getNumberOfComponents() * i));
      ::memcpy(destination, source, p->getTypeSize() * data->getNumberOfComponents());
    }
    cellAttrMat->removeAttributeArray(*iter);
    newCellAttrMat->insertOrAssign(data);
  }
  m->getGeometryAs<ImageGeom>()->setSpacing(spacing[0], spacing[1], m_NewZRes);
  m->getGeometryAs<ImageGeom>()->setDimensions(m_XP, m_YP, m_ZP);
  m->removeAttributeMatrix(getCellAttributeMatrixPath().getAttributeMatrixName());
  m->addOrReplaceAttributeMatrix(newCellAttrMat);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AbstractFilter::Pointer RegularizeZSpacing::newFilterInstance(bool copyFilterParameters) const
{
  RegularizeZSpacing::Pointer filter = RegularizeZSpacing::New();
  if(copyFilterParameters)
  {
    copyFilterParameterInstanceVariables(filter.get());
  }
  return filter;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString RegularizeZSpacing::getCompiledLibraryName() const
{
  return SamplingConstants::SamplingBaseName;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString RegularizeZSpacing::getBrandingString() const
{
  return "Sampling";
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString RegularizeZSpacing::getFilterVersion() const
{
  QString version;
  QTextStream vStream(&version);
  vStream << Sampling::Version::Major() << "." << Sampling::Version::Minor() << "." << Sampling::Version::Patch();
  return version;
}
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString RegularizeZSpacing::getGroupName() const
{
  return SIMPL::FilterGroups::SamplingFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QUuid RegularizeZSpacing::getUuid() const
{
  return QUuid("{bc4952fa-34ca-50bf-a1e9-2b9f7e5d47ce}");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString RegularizeZSpacing::getSubGroupName() const
{
  return SIMPL::FilterSubGroups::ResolutionFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString RegularizeZSpacing::getHumanLabel() const
{
  return "Regularize Z Spacing";
}

// -----------------------------------------------------------------------------
RegularizeZSpacing::Pointer RegularizeZSpacing::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
std::shared_ptr<RegularizeZSpacing> RegularizeZSpacing::New()
{
  struct make_shared_enabler : public RegularizeZSpacing
  {
  };
  std::shared_ptr<make_shared_enabler> val = std::make_shared<make_shared_enabler>();
  val->setupFilterParameters();
  return val;
}

// -----------------------------------------------------------------------------
QString RegularizeZSpacing::getNameOfClass() const
{
  return QString("RegularizeZSpacing");
}

// -----------------------------------------------------------------------------
QString RegularizeZSpacing::ClassName()
{
  return QString("RegularizeZSpacing");
}

// -----------------------------------------------------------------------------
void RegularizeZSpacing::setCellAttributeMatrixPath(const DataArrayPath& value)
{
  m_CellAttributeMatrixPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath RegularizeZSpacing::getCellAttributeMatrixPath() const
{
  return m_CellAttributeMatrixPath;
}

// -----------------------------------------------------------------------------
void RegularizeZSpacing::setInputFile(const QString& value)
{
  m_InputFile = value;
}

// -----------------------------------------------------------------------------
QString RegularizeZSpacing::getInputFile() const
{
  return m_InputFile;
}

// -----------------------------------------------------------------------------
void RegularizeZSpacing::setNewZRes(float value)
{
  m_NewZRes = value;
}

// -----------------------------------------------------------------------------
float RegularizeZSpacing::getNewZRes() const
{
  return m_NewZRes;
}
