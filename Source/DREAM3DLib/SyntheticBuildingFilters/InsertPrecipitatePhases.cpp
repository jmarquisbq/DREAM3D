/* ============================================================================
 * Copyright (c) 2011 Michael A. Jackson (BlueQuartz Software)
 * Copyright (c) 2011 Dr. Michael A. Groeber (US Air Force Research Laboratories)
 * All rights reserved.
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
 * Neither the name of Michael A. Groeber, Michael A. Jackson, the US Air Force,
 * BlueQuartz Software nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior written
 * permission.
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
 *  This code was written under United States Air Force Contract number
 *                           FA8650-07-D-5800
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "InsertPrecipitatePhases.h"

#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QDir>

#include "DREAM3DLib/Common/Constants.h"
#include "DREAM3DLib/Math/MatrixMath.h"
#include "DREAM3DLib/Math/DREAM3DMath.h"
#include "DREAM3DLib/Utilities/DREAM3DRandom.h"
#include "DREAM3DLib/DataContainers/DataContainerMacros.h"

#include "DREAM3DLib/ShapeOps/CubeOctohedronOps.h"
#include "DREAM3DLib/ShapeOps/CylinderOps.h"
#include "DREAM3DLib/ShapeOps/EllipsoidOps.h"
#include "DREAM3DLib/ShapeOps/SuperEllipsoidOps.h"

#include "DREAM3DLib/GenericFilters/FindSurfaceCells.h"
#include "DREAM3DLib/StatisticsFilters/FindNeighbors.h"
#include "DREAM3DLib/GenericFilters/RenumberGrains.h"

#include "DREAM3DLib/StatsData/PrecipitateStatsData.h"



// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
InsertPrecipitatePhases::InsertPrecipitatePhases() :
  AbstractFilter(),
  m_DataContainerName(DREAM3D::HDF5::VolumeDataContainerName),
  m_GrainIdsArrayName(DREAM3D::CellData::GrainIds),
  m_CellPhasesArrayName(DREAM3D::CellData::Phases),
  m_SurfaceVoxelsArrayName(DREAM3D::CellData::SurfaceVoxels),
  m_ActiveArrayName(DREAM3D::FieldData::Active),
  m_AxisEulerAnglesArrayName(DREAM3D::FieldData::AxisEulerAngles),
  m_AxisLengthsArrayName(DREAM3D::FieldData::AxisLengths),
  m_CentroidsArrayName(DREAM3D::FieldData::Centroids),
  m_EquivalentDiametersArrayName(DREAM3D::FieldData::EquivalentDiameters),
  m_NumCellsArrayName(DREAM3D::FieldData::NumCells),
  m_Omega3sArrayName(DREAM3D::FieldData::Omega3s),
  m_FieldPhasesArrayName(DREAM3D::FieldData::Phases),
  m_VolumesArrayName(DREAM3D::FieldData::Volumes),
  m_PhaseTypesArrayName(DREAM3D::EnsembleData::PhaseTypes),
  m_ShapeTypesArrayName(DREAM3D::EnsembleData::ShapeTypes),
  m_NumFieldsArrayName(DREAM3D::EnsembleData::NumFields),
  m_CsvOutputFile(""),
  m_PeriodicBoundaries(false),
  m_WriteGoalAttributes(false),
  m_GrainIds(NULL),
  m_CellPhases(NULL),
  m_SurfaceVoxels(NULL),
  m_AxisEulerAngles(NULL),
  m_Centroids(NULL),
  m_AxisLengths(NULL),
  m_Volumes(NULL),
  m_Omega3s(NULL),
  m_EquivalentDiameters(NULL),
  m_Active(NULL),
  m_FieldPhases(NULL),
  m_NumCells(NULL),
  m_PhaseTypes(NULL),
  m_ShapeTypes(NULL),
  m_NumFields(NULL)
{
  m_EllipsoidOps = EllipsoidOps::New();
  m_ShapeOps[DREAM3D::ShapeType::EllipsoidShape] = m_EllipsoidOps.get();
  m_SuperEllipsoidOps = SuperEllipsoidOps::New();
  m_ShapeOps[DREAM3D::ShapeType::SuperEllipsoidShape] = m_SuperEllipsoidOps.get();
  m_CubicOctohedronOps = CubeOctohedronOps::New();
  m_ShapeOps[DREAM3D::ShapeType::CubeOctahedronShape] = m_CubicOctohedronOps.get();
  m_CylinderOps = CylinderOps::New();
  m_ShapeOps[DREAM3D::ShapeType::CylinderShape] = m_CylinderOps.get();
  m_UnknownShapeOps = ShapeOps::New();
  m_ShapeOps[DREAM3D::ShapeType::UnknownShapeType] = m_UnknownShapeOps.get();

  m_OrthoOps = OrthoRhombicOps::New();

  setupFilterParameters();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
InsertPrecipitatePhases::~InsertPrecipitatePhases()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------

void InsertPrecipitatePhases::setupFilterParameters()
{
  FilterParameterVector parameters;
  {
    FilterParameter::Pointer option = FilterParameter::New();
    option->setHumanLabel("Periodic Boundary");
    option->setPropertyName("PeriodicBoundaries");
    option->setWidgetType(FilterParameter::BooleanWidget);
    option->setValueType("bool");
    parameters.push_back(option);
  }
  {
    FilterParameter::Pointer option = FilterParameter::New();
    option->setHumanLabel("Write Goal Attributes");
    option->setPropertyName("WriteGoalAttributes");
    option->setWidgetType(FilterParameter::BooleanWidget);
    option->setValueType("bool");
    parameters.push_back(option);
  }
  {
    FilterParameter::Pointer option = FilterParameter::New();
    option->setHumanLabel("Goal Attribute CSV File");
    option->setPropertyName("CsvOutputFile");
    option->setWidgetType(FilterParameter::OutputFileWidget);
    option->setFileExtension("*.csv");
    option->setFileType("Comma Separated Data");
    option->setValueType("string");
    parameters.push_back(option);
  }
  setFilterParameters(parameters);
}
// -----------------------------------------------------------------------------
void InsertPrecipitatePhases::readFilterParameters(AbstractFilterParametersReader* reader, int index)
{
  reader->openFilterGroup(this, index);
  /* Code to read the values goes between these statements */
  /* FILTER_WIDGETCODEGEN_AUTO_GENERATED_CODE BEGIN*/
  setPeriodicBoundaries( reader->readValue("PeriodicBoundaries", false) );
  setWriteGoalAttributes( reader->readValue("WriteGoalAttributes", false) );
  setCsvOutputFile( reader->readString( "CsvOutputFile", getCsvOutputFile() ) );
  /* FILTER_WIDGETCODEGEN_AUTO_GENERATED_CODE END*/
  reader->closeFilterGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int InsertPrecipitatePhases::writeFilterParameters(AbstractFilterParametersWriter* writer, int index)
{
  writer->openFilterGroup(this, index);
  writer->writeValue("PeriodicBoundaries", getPeriodicBoundaries() );
  writer->closeFilterGroup();
  return ++index; // we want to return the next index that was just written to
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void InsertPrecipitatePhases::dataCheck(bool preflight, size_t voxels, size_t fields, size_t ensembles)
{
  setErrorCondition(0);

  VolumeDataContainer* m = getDataContainerArray()->getDataContainerAs<VolumeDataContainer>(getDataContainerName());

  // Cell Data
  GET_PREREQ_DATA(m, DREAM3D, CellData, GrainIds, -300, int32_t, Int32ArrayType, voxels, 1)

  GET_PREREQ_DATA(m, DREAM3D, CellData, SurfaceVoxels, -301, int8_t, Int8ArrayType, voxels, 1)


  GET_PREREQ_DATA(m, DREAM3D, CellData, CellPhases, -302, int32_t, Int32ArrayType, voxels, 1)

  // Field Data
  CREATE_NON_PREREQ_DATA(m, DREAM3D, CellFieldData, FieldPhases, int32_t, Int32ArrayType, 0, fields, 1)
  CREATE_NON_PREREQ_DATA(m, DREAM3D, CellFieldData, EquivalentDiameters, float, FloatArrayType, 0, fields, 1)
  CREATE_NON_PREREQ_DATA(m, DREAM3D, CellFieldData, Omega3s, float, FloatArrayType, 0, fields, 1)
  CREATE_NON_PREREQ_DATA(m, DREAM3D, CellFieldData, AxisEulerAngles, float, FloatArrayType, 0, fields, 3)
  CREATE_NON_PREREQ_DATA(m, DREAM3D, CellFieldData, AxisLengths, float, FloatArrayType, 0, fields, 3)
  CREATE_NON_PREREQ_DATA(m, DREAM3D, CellFieldData, Volumes, float, FloatArrayType, 0, fields, 1)
  CREATE_NON_PREREQ_DATA(m, DREAM3D, CellFieldData, Centroids, float, FloatArrayType, 0, fields, 3)
  CREATE_NON_PREREQ_DATA(m, DREAM3D, CellFieldData, Active, bool, BoolArrayType, false, fields, 1)
  CREATE_NON_PREREQ_DATA(m, DREAM3D, CellFieldData, NumCells, int32_t, Int32ArrayType, 0, fields, 1)
  //Ensemble Data
  typedef DataArray<unsigned int> PhaseTypeArrayType;
  typedef DataArray<unsigned int> ShapeTypeArrayType;
  GET_PREREQ_DATA(m, DREAM3D, CellEnsembleData, PhaseTypes, -301, unsigned int, PhaseTypeArrayType, ensembles, 1)
  GET_PREREQ_DATA(m, DREAM3D, CellEnsembleData, ShapeTypes, -304, unsigned int, ShapeTypeArrayType, ensembles, 1)
  CREATE_NON_PREREQ_DATA(m, DREAM3D, CellEnsembleData, NumFields, int32_t, Int32ArrayType, 0, ensembles, 1)
  m_StatsDataArray = StatsDataArray::SafeObjectDownCast<IDataArray*, StatsDataArray*>(m->getCellEnsembleData(DREAM3D::EnsembleData::Statistics).get());
  if(m_StatsDataArray == NULL)
  {
    QString ss = QObject::tr("Stats Array Not Initialized At Beginning Correctly");
    setErrorCondition(-308);
    addErrorMessage(getHumanLabel(), ss, -308);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void InsertPrecipitatePhases::preflight()
{
  VolumeDataContainer* m = getDataContainerArray()->getDataContainerAs<VolumeDataContainer>(getDataContainerName());
  if(NULL == m)
  {
    setErrorCondition(-999);
    notifyErrorMessage("The DataContainer Object was NULL", -999);
    return;
  }

  dataCheck(true, 1, 1, 1);

  if (m_WriteGoalAttributes == true && getCsvOutputFile().isEmpty() == true)
  {
    QString ss = QObject::tr("%1 needs the Csv Output File Set and it was not.").arg(ClassName());
    addErrorMessage(getHumanLabel(), ss, -1);
    setErrorCondition(-387);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void InsertPrecipitatePhases::execute()
{
  int err = 0;
  setErrorCondition(err);
  DREAM3D_RANDOMNG_NEW()
  VolumeDataContainer* m = getDataContainerArray()->getDataContainerAs<VolumeDataContainer>(getDataContainerName());

  if(NULL == m)
  {
    setErrorCondition(-999);
    notifyErrorMessage("The DataContainer Object was NULL", -999);
    return;
  }

  int64_t totalPoints = m->getTotalPoints();
  size_t totalFields = m->getNumCellFieldTuples();

  if(totalFields == 0) { totalFields = 1; }
  dataCheck(false, totalPoints, totalFields, m->getNumCellEnsembleTuples());
  if (getErrorCondition() < 0)
  {
    return;
  }

  sizex = m->getXPoints() * m->getXRes();
  sizey = m->getYPoints() * m->getYRes();
  sizez = m->getZPoints() * m->getZRes();

  totalvol = sizex * sizey * sizez;

  notifyStatusMessage("Packing Precipitates - Generating and Placing Precipitates");
// this initializes the arrays to hold the details of the locations of all of the grains during packing
  Int32ArrayType::Pointer grainOwnersPtr = initialize_packinggrid();
  // Get a pointer to the Grain Owners that was just initialized in the initialize_packinggrid() method
//  int32_t* grainOwners = grainOwnersPtr->getPointer(0);
//  size_t grainOwnersIdx = 0;


  place_precipitates(grainOwnersPtr);

  notifyStatusMessage("Packing Precipitates - Assigning Voxels");
  assign_voxels();

  notifyStatusMessage("Packing Precipitates - Renumbering Grains");
  RenumberGrains::Pointer renumber_grains1 = RenumberGrains::New();
  renumber_grains1->setObservers(this->getObservers());
  renumber_grains1->setDataContainerArray(getDataContainerArray());
  renumber_grains1->execute();
  err = renumber_grains1->getErrorCondition();
  if (err < 0)
  {
    setErrorCondition(renumber_grains1->getErrorCondition());
    addErrorMessages(renumber_grains1->getPipelineMessages());
    return;
  }

  dataCheck(false, m->getTotalPoints(), m->getNumCellFieldTuples(), m->getNumCellEnsembleTuples());

  notifyStatusMessage("Packing Precipitates - Filling Gaps");
  assign_gaps();

  notifyStatusMessage("Packing Precipitates - Cleaning Up Volume");
  cleanup_grains();

  notifyStatusMessage("Packing Precipitates - Renumbering Grains");
  RenumberGrains::Pointer renumber_grains2 = RenumberGrains::New();
  renumber_grains2->setObservers(this->getObservers());
  renumber_grains2->setDataContainerArray(getDataContainerArray());
  renumber_grains2->execute();
  err = renumber_grains2->getErrorCondition();
  if (err < 0)
  {
    setErrorCondition(renumber_grains2->getErrorCondition());
    addErrorMessages(renumber_grains2->getPipelineMessages());
    return;
  }

  if(m_WriteGoalAttributes == true)
  {
    write_goal_attributes();
  }

  m->removeCellFieldData(m_EquivalentDiametersArrayName);
  m->removeCellFieldData(m_Omega3sArrayName);
  m->removeCellFieldData(m_AxisEulerAnglesArrayName);
  m->removeCellFieldData(m_AxisLengthsArrayName);
  m->removeCellFieldData(m_VolumesArrayName);
  m->removeCellFieldData(m_CentroidsArrayName);
  m->removeCellFieldData(m_NumCellsArrayName);

  // If there is an error set this to something negative and also set a message
  notifyStatusMessage("InsertPrecipitatePhases Completed");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void  InsertPrecipitatePhases::place_precipitates(Int32ArrayType::Pointer grainOwnersPtr)
{
  notifyStatusMessage("Placing Precipitates");
  DREAM3D_RANDOMNG_NEW()

  VolumeDataContainer* m = getDataContainerArray()->getDataContainerAs<VolumeDataContainer>(getDataContainerName());

  StatsDataArray& statsDataArray = *m_StatsDataArray;

  size_t udims[3] =
  { 0, 0, 0 };
  m->getDimensions(udims);
#if (CMP_SIZEOF_SIZE_T == 4)
  typedef int32_t DimType;
#else
  typedef int64_t DimType;
#endif
  DimType dims[3] =
  { static_cast<DimType>(udims[0]), static_cast<DimType>(udims[1]), static_cast<DimType>(udims[2]), };

  sizex = dims[0] * m->getXRes();
  sizey = dims[1] * m->getYRes();
  sizez = dims[2] * m->getZRes();
  totalvol = sizex * sizey * sizez;

  int64_t totalPoints = m->getTotalPoints();
  size_t currentnumgrains = m->getNumCellFieldTuples();
  if(currentnumgrains == 0)
  {
    m->resizeCellFieldDataArrays(1);
    dataCheck(false, totalPoints, 1, m->getNumCellEnsembleTuples());
    currentnumgrains = 1;
  }
  firstPrecipitateField = currentnumgrains;
  int phase;
  float precipboundaryfraction = 0.0;
  float random = 0.0f;
  int random2 = 0;
  float xc, yc, zc;
  float oldxc, oldyc, oldzc;
  int randomgrain;
  int acceptedmoves = 0;
  double totalprecipitatefractions = 0.0;

  size_t numensembles = m->getNumCellEnsembleTuples();

  for (size_t i = 1; i < numensembles; ++i)
  {
    if(m_PhaseTypes[i] == DREAM3D::PhaseType::PrecipitatePhase)
    {
      PrecipitateStatsData* pp = PrecipitateStatsData::SafePointerDownCast(statsDataArray[i].get());
      m_NumFields[i] = 0;
      precipitatephases.push_back(i);
      precipitatephasefractions.push_back(pp->getPhaseFraction());
      totalprecipitatefractions = totalprecipitatefractions + pp->getPhaseFraction();
    }
  }
  for (size_t i = 0; i < precipitatephases.size(); i++)
  {
    precipitatephasefractions[i] = static_cast<float>(precipitatephasefractions[i] / totalprecipitatefractions);
  }

  // initialize the sim and goal size distributions for the precipitate phases
  grainsizedist.resize(precipitatephases.size());
  simgrainsizedist.resize(precipitatephases.size());
  grainsizediststep.resize(precipitatephases.size());
  for (size_t i = 0; i < precipitatephases.size(); i++)
  {
    phase = precipitatephases[i];
    PrecipitateStatsData* pp = PrecipitateStatsData::SafePointerDownCast(statsDataArray[phase].get());
    grainsizedist[i].resize(40);
    simgrainsizedist[i].resize(40);
    grainsizediststep[i] = static_cast<float>(((2 * pp->getMaxGrainDiameter()) - (pp->getMinGrainDiameter() / 2.0)) / grainsizedist[i].size());
    float input = 0;
    float previoustotal = 0;
    VectorOfFloatArray GSdist = pp->getGrainSizeDistribution();
    float avg = GSdist[0]->GetValue(0);
    float stdev = GSdist[1]->GetValue(0);
    float denominatorConst = sqrtf(2.0f * stdev * stdev); // Calculate it here rather than calculating the same thing multiple times below
    for (size_t j = 0; j < grainsizedist[i].size(); j++)
    {
      input = (float(j + 1) * grainsizediststep[i]) + (pp->getMinGrainDiameter() / 2.0f);
      float logInput = logf(input);
      if(logInput <= avg)
      {
        grainsizedist[i][j] = 0.5f - 0.5f * (DREAM3DMath::erf((avg - logInput) / denominatorConst)) - previoustotal;
      }
      if(logInput > avg)
      {
        grainsizedist[i][j] = 0.5f + 0.5f * (DREAM3DMath::erf((logInput - avg) / denominatorConst)) - previoustotal;
      }
      previoustotal = previoustotal + grainsizedist[i][j];
    }
  }

  Precip precip;
  QVector<float> curphasevol;
  curphasevol.resize(precipitatephases.size());
  float change = 0.0f;
  float factor = 1.0;
  size_t iter = 0;
  for (size_t j = 0; j < precipitatephases.size(); ++j)
  {
    curphasevol[j] = 0;
    float curphasetotalvol = static_cast<float>(totalvol * totalprecipitatefractions * precipitatephasefractions[j]);
    while (curphasevol[j] < (factor * curphasetotalvol))
    {
      iter++;
      Seed++;
      phase = precipitatephases[j];
      generate_precipitate(phase, static_cast<int>(Seed), &precip, m_ShapeTypes[phase], m_OrthoOps);
      currentsizedisterror = check_sizedisterror(&precip);
      change = (currentsizedisterror) - (oldsizedisterror);
      if(change > 0 || currentsizedisterror > (1.0 - (float(iter) * 0.001)) || curphasevol[j] < (0.75 * factor * curphasetotalvol))
      {
        QString ss = QObject::tr("Packing Precipitates - Generating Grain #%1").arg(currentnumgrains);
        notifyStatusMessage(ss);

        m->resizeCellFieldDataArrays(currentnumgrains + 1);
        dataCheck(false, totalPoints, currentnumgrains + 1, m->getNumCellEnsembleTuples());
        m_Active[currentnumgrains] = true;
        transfer_attributes(currentnumgrains, &precip);
        oldsizedisterror = currentsizedisterror;
        curphasevol[j] = curphasevol[j] + m_Volumes[currentnumgrains];
        //FIXME: Initialize the Grain with some sort of default data
        iter = 0;
        currentnumgrains++;
      }
    }
  }

  // initialize the sim and goal clustering distribution for the primary phases
  clusteringdist.resize(precipitatephases.size());
  simclusteringdist.resize(precipitatephases.size());
  clusteringdiststep.resize(precipitatephases.size());
  for (size_t i = 0; i < precipitatephases.size(); i++)
  {
    phase = precipitatephases[i];
    PrecipitateStatsData* pp = PrecipitateStatsData::SafePointerDownCast(statsDataArray[phase].get());
    clusteringdist[i].resize(pp->getBinNumbers()->GetSize());
    simclusteringdist[i].resize(pp->getBinNumbers()->GetSize());
    VectorOfFloatArray Neighdist = pp->getGrainSize_Clustering();
    float normalizer = 0;
    for (size_t j = 0; j < clusteringdist[i].size(); j++)
    {
      clusteringdist[i][j].resize(40);
      float input = 0;
      float previoustotal = 0;
      float avg = Neighdist[0]->GetValue(j);
      float stdev = Neighdist[1]->GetValue(j);
      clusteringdiststep[i] = 2;
      float denominatorConst = sqrtf(2.0f * stdev * stdev); // Calculate it here rather than calculating the same thing multiple times below
      for (size_t k = 0; k < clusteringdist[i][j].size(); k++)
      {
        input = (float(k + 1) * clusteringdiststep[i]);
        float logInput = logf(input);
        if(logInput <= avg)
        {
          clusteringdist[i][j][k] = 0.5f - 0.5f * (DREAM3DMath::erf((avg - logInput) / denominatorConst)) - previoustotal;
        }
        if(logInput > avg)
        {
          clusteringdist[i][j][k] = 0.5f + 0.5f * (DREAM3DMath::erf((logInput - avg) / denominatorConst)) - previoustotal;
        }
        previoustotal = previoustotal + clusteringdist[i][j][k];
      }
      normalizer = normalizer + previoustotal;
    }
    for (size_t j = 0; j < clusteringdist[i].size(); j++)
    {
      for (size_t k = 0; k < clusteringdist[i][j].size(); k++)
      {
        clusteringdist[i][j][k] = clusteringdist[i][j][k] / normalizer;
      }
    }
  }

  //  for each grain : select centroid, determine voxels in grain, monitor filling error and decide of the 10 placements which
  // is the most beneficial, then the grain is added and its clustering are determined

  size_t numgrains = m->getNumCellFieldTuples();

  columnlist.resize(numgrains);
  rowlist.resize(numgrains);
  planelist.resize(numgrains);
  packqualities.resize(numgrains);
  fillingerror = 1;
  for (size_t i = firstPrecipitateField; i < numgrains; i++)
  {

    QString ss = QObject::tr("Packing Precipitates - Placing Precipitate #%1").arg(i);
    notifyStatusMessage(ss);

    PrecipitateStatsData* pp = PrecipitateStatsData::SafePointerDownCast(statsDataArray[m_FieldPhases[i]].get());
    precipboundaryfraction = pp->getPrecipBoundaryFraction();
    random = static_cast<float>(rg.genrand_res53());
    if(random <= precipboundaryfraction)
    {
      random2 = int(rg.genrand_res53() * double(totalPoints - 1));
      while (m_SurfaceVoxels[random2] == 0 || m_GrainIds[random2] >= firstPrecipitateField)
      {
        random2++;
        if(random2 >= totalPoints) { random2 = static_cast<int>(random2 - totalPoints); }
      }
    }
    else if(random > precipboundaryfraction)
    {
      random2 = static_cast<int>(rg.genrand_res53() * (totalPoints - 1));
      while (m_SurfaceVoxels[random2] != 0 || m_GrainIds[random2] >= firstPrecipitateField)
      {
        random2++;
        if(random2 >= totalPoints) { random2 = static_cast<int>(random2 - totalPoints); }
      }
    }
    xc = find_xcoord(random2);
    yc = find_ycoord(random2);
    zc = find_zcoord(random2);
    m_Centroids[3 * i] = xc;
    m_Centroids[3 * i + 1] = yc;
    m_Centroids[3 * i + 2] = zc;
    insert_precipitate(i);
    oldclusteringerror = check_clusteringerror(i, -1000);
//    fillingerror = check_fillingerror(i, -1000, grainOwnersPtr);
//    for (int iter_fill = 0; iter_fill < 10; iter_fill++)
//    {
//      random = static_cast<float>(rg.genrand_res53());
//      if(random <= precipboundaryfraction)
//      {
//        random2 = int(rg.genrand_res53() * double(totalPoints - 1));
//        while (m_SurfaceVoxels[random2] == 0 || m_GrainIds[random2] >= firstPrecipitateField)
//        {
//          random2++;
//          if(random2 >= totalPoints) { random2 = static_cast<int>(random2 - totalPoints); }
//        }
//      }
//      else if(random > precipboundaryfraction)
//      {
//        random2 = static_cast<int>(rg.genrand_res53() * (totalPoints - 1));
//        while (m_SurfaceVoxels[random2] != 0 || m_GrainIds[random2] >= firstPrecipitateField)
//        {
//          random2++;
//          if(random2 >= totalPoints) { random2 = static_cast<int>(random2 - totalPoints); }
//        }
//      }
//      xc = find_xcoord(random2);
//      yc = find_ycoord(random2);
//      zc = find_zcoord(random2);
//      oldxc = m_Centroids[3 * i];
//      oldyc = m_Centroids[3 * i + 1];
//      oldzc = m_Centroids[3 * i + 2];
//      oldfillingerror = fillingerror;
//      fillingerror = check_fillingerror(-1000, i, grainOwnersPtr);
//      move_precipitate(i, xc, yc, zc);
//      fillingerror = check_fillingerror(i, -1000, grainOwnersPtr);
//      if(fillingerror > oldfillingerror)
//      {
//        fillingerror = check_fillingerror(-1000, i, grainOwnersPtr);
//        move_precipitate(i, oldxc, oldyc, oldzc);
//        fillingerror = check_fillingerror(i, -1000, grainOwnersPtr);
//      }
//    }
  }

  notifyStatusMessage("Packing Grains - Initial Grain Placement Complete");


  // begin swaping/moving/adding/removing grains to try to improve packing
  int totalAdjustments = static_cast<int>(10 * ((numgrains - firstPrecipitateField) - 1));
  for (int iteration = 0; iteration < totalAdjustments; ++iteration)
  {

    QString ss;
    ss = QObject::tr("Packing Grains - Swapping/Moving/Adding/Removing Grains Iteration %1/%2").arg(iteration).arg(totalAdjustments);
    if(iteration % 100 == 0) { notifyStatusMessage(ss); }

    //    change1 = 0;
    //    change2 = 0;
    int option = iteration % 2;

    // JUMP - this option moves one grain to a random spot in the volume
    if(option == 0)
    {
      randomgrain = firstPrecipitateField + int(rg.genrand_res53() * (numgrains - firstPrecipitateField));
      if(randomgrain < firstPrecipitateField) { randomgrain = firstPrecipitateField; }
      if(randomgrain >= static_cast<int>(numgrains))
      {
        randomgrain = static_cast<int>(numgrains) - 1;
      }
      Seed++;

      PrecipitateStatsData* pp = PrecipitateStatsData::SafePointerDownCast(statsDataArray[m_FieldPhases[randomgrain]].get());
      if (NULL == pp)
      {
        continue;
      }
      precipboundaryfraction = pp->getPrecipBoundaryFraction();
      random = static_cast<float>(rg.genrand_res53());
      if(random <= precipboundaryfraction)
      {
        random2 = int(rg.genrand_res53() * double(totalPoints - 1));
        while (m_SurfaceVoxels[random2] == 0 || m_GrainIds[random2] >= firstPrecipitateField)
        {
          random2++;
          if(random2 >= totalPoints) { random2 = static_cast<int>(random2 - totalPoints); }
        }
      }
      else if(random > precipboundaryfraction)
      {
        random2 = static_cast<int>(rg.genrand_res53() * (totalPoints - 1));
        while (m_SurfaceVoxels[random2] != 0 || m_GrainIds[random2] >= firstPrecipitateField)
        {
          random2++;
          if(random2 >= totalPoints) { random2 = static_cast<int>(random2 - totalPoints); }
        }
      }
      xc = find_xcoord(random2);
      yc = find_ycoord(random2);
      zc = find_zcoord(random2);
      oldxc = m_Centroids[3 * randomgrain];
      oldyc = m_Centroids[3 * randomgrain + 1];
      oldzc = m_Centroids[3 * randomgrain + 2];
//      oldfillingerror = fillingerror;
//      fillingerror = check_fillingerror(-1000, static_cast<int>(randomgrain), grainOwnersPtr);
//      move_precipitate(randomgrain, xc, yc, zc);
//      fillingerror = check_fillingerror(static_cast<int>(randomgrain), -1000, grainOwnersPtr);
      currentclusteringerror = check_clusteringerror(-1000, randomgrain);
      move_precipitate(randomgrain, xc, yc, zc);
      currentclusteringerror = check_clusteringerror(randomgrain, -1000);
      if(currentclusteringerror <= oldclusteringerror)
      {
        oldclusteringerror = currentclusteringerror;
        acceptedmoves++;
      }
      else if(currentclusteringerror > oldclusteringerror)
      {
//      fillingerror = check_fillingerror(-1000, static_cast<int>(randomgrain), grainOwnersPtr);
//      move_precipitate(randomgrain, oldxc, oldyc, oldzc);
//      fillingerror = check_fillingerror(static_cast<int>(randomgrain), -1000, grainOwnersPtr);
        currentclusteringerror = check_clusteringerror(-1000, randomgrain);
        move_precipitate(randomgrain, oldxc, oldyc, oldzc);
        currentclusteringerror = check_clusteringerror(randomgrain, -1000);
        oldclusteringerror = currentclusteringerror;
      }
    }
    // NUDGE - this option moves one grain to a spot close to its current centroid
    if(option == 1)
    {
      randomgrain = firstPrecipitateField + int(rg.genrand_res53() * (numgrains - firstPrecipitateField));
      if(randomgrain < firstPrecipitateField) { randomgrain = firstPrecipitateField; }
      if(randomgrain >= static_cast<int>(numgrains))
      {
        randomgrain = static_cast<int>(numgrains) - 1;
      }
      Seed++;
      oldxc = m_Centroids[3 * randomgrain];
      oldyc = m_Centroids[3 * randomgrain + 1];
      oldzc = m_Centroids[3 * randomgrain + 2];
      xc = static_cast<float>(oldxc + ((2.0f * (rg.genrand_res53() - 0.5f)) * (2.0f * m_PackingRes[0])));
      yc = static_cast<float>(oldyc + ((2.0f * (rg.genrand_res53() - 0.5f)) * (2.0f * m_PackingRes[1])));
      zc = static_cast<float>(oldzc + ((2.0f * (rg.genrand_res53() - 0.5f)) * (2.0f * m_PackingRes[2])));
//      oldfillingerror = fillingerror;
//      fillingerror = check_fillingerror(-1000, static_cast<int>(randomgrain), grainOwnersPtr);
//      move_precipitate(randomgrain, xc, yc, zc);
//      fillingerror = check_fillingerror(static_cast<int>(randomgrain), -1000, grainOwnersPtr);
      currentclusteringerror = check_clusteringerror(-1000, randomgrain);
      move_precipitate(randomgrain, xc, yc, zc);
      currentclusteringerror = check_clusteringerror(randomgrain, -1000);
      if(currentclusteringerror <= oldclusteringerror)
      {
        oldclusteringerror = currentclusteringerror;
        acceptedmoves++;
      }
      else if(currentclusteringerror > oldclusteringerror)
      {
//      fillingerror = check_fillingerror(-1000, static_cast<int>(randomgrain), grainOwnersPtr);
//      move_precipitate(randomgrain, oldxc, oldyc, oldzc);
//      fillingerror = check_fillingerror(static_cast<int>(randomgrain), -1000, grainOwnersPtr);
        currentclusteringerror = check_clusteringerror(-1000, randomgrain);
        move_precipitate(randomgrain, oldxc, oldyc, oldzc);
        currentclusteringerror = check_clusteringerror(randomgrain, -1000);
        oldclusteringerror = currentclusteringerror;
      }
    }
  }

}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void InsertPrecipitatePhases::generate_precipitate(int phase, int seed, Precip* precip, unsigned int shapeclass, OrientationOps::Pointer OrthoOps)
{
  DREAM3D_RANDOMNG_NEW_SEEDED(seed)

  StatsDataArray& statsDataArray = *m_StatsDataArray;

  float r1 = 1;
  float a2 = 0, a3 = 0;
  float b2 = 0, b3 = 0;
  float diam = 0;
  float vol = 0;
  int volgood = 0;
  float phi1, PHI, phi2;
  float fourThirdsPi =  static_cast<float>((4.0f / 3.0f) * (DREAM3D::Constants::k_Pi));
  PrecipitateStatsData* pp = PrecipitateStatsData::SafePointerDownCast(statsDataArray[phase].get());
  VectorOfFloatArray GSdist = pp->getGrainSizeDistribution();
  float avg = GSdist[0]->GetValue(0);
  float stdev = GSdist[1]->GetValue(0);
  while (volgood == 0)
  {
    volgood = 1;
    diam = static_cast<float>(rg.genrand_norm(avg, stdev));
    diam = exp(diam);
    if(diam >= pp->getMaxGrainDiameter()) { volgood = 0; }
    if(diam < pp->getMinGrainDiameter()) { volgood = 0; }
    vol = fourThirdsPi * ((diam / 2.0f) * (diam / 2.0f) * (diam / 2.0f));
  }
  int diameter = int((diam - pp->getMinGrainDiameter()) / pp->getBinStepSize());
  float r2 = 0.0f, r3 = 1.0f;
  VectorOfFloatArray bovera = pp->getGrainSize_BOverA();
  VectorOfFloatArray covera = pp->getGrainSize_COverA();
  while (r2 < r3)
  {
    r2 = 0.0f, r3 = 0.0f;
    a2 = bovera[0]->GetValue(diameter);
    b2 = bovera[1]->GetValue(diameter);
    if(a2 == 0)
    {
      a2 = bovera[0]->GetValue(diameter - 1);
      b2 = bovera[1]->GetValue(diameter - 1);
    }
    r2 = static_cast<float>(rg.genrand_beta(a2, b2));
    a3 = covera[0]->GetValue(diameter);
    b3 = covera[1]->GetValue(diameter);
    if(a3 == 0)
    {
      a3 = covera[0]->GetValue(diameter - 1);
      b3 = covera[1]->GetValue(diameter - 1);
    }
    r3 = static_cast<float>( rg.genrand_beta(a3, b3) );
  }
  float random = static_cast<float>( rg.genrand_res53() );
  float totaldensity = 0;
  int bin = 0;
  FloatArrayType::Pointer axisodf = pp->getAxisOrientation();
  while (random > totaldensity && bin < static_cast<int>(axisodf->GetSize()))
  {
    totaldensity = totaldensity + axisodf->GetValue(bin);
    bin++;
  }
  OrthoOps->determineEulerAngles(bin, phi1, PHI, phi2);
  VectorOfFloatArray omega3 = pp->getGrainSize_Omegas();
  float mf = omega3[0]->GetValue(diameter);
  float s = omega3[1]->GetValue(diameter);
  float omega3f = static_cast<float>(rg.genrand_beta(mf, s));
  if(shapeclass == DREAM3D::ShapeType::EllipsoidShape) { omega3f = 1; }

  precip->m_Volumes = vol;
  precip->m_EquivalentDiameters = diam;
  precip->m_AxisLengths[0] = r1;
  precip->m_AxisLengths[1] = r2;
  precip->m_AxisLengths[2] = r3;
  precip->m_AxisEulerAngles[0] = phi1;
  precip->m_AxisEulerAngles[1] = PHI;
  precip->m_AxisEulerAngles[2] = phi2;
  precip->m_Omega3s = omega3f;
  precip->m_FieldPhases = phase;
}

void InsertPrecipitatePhases::transfer_attributes(int gnum, Precip* precip)
{
  m_Volumes[gnum] = precip->m_Volumes;
  m_EquivalentDiameters[gnum] = precip->m_EquivalentDiameters;
  m_AxisLengths[3 * gnum + 0] = precip->m_AxisLengths[0];
  m_AxisLengths[3 * gnum + 1] = precip->m_AxisLengths[1];
  m_AxisLengths[3 * gnum + 2] = precip->m_AxisLengths[2];
  m_AxisEulerAngles[3 * gnum + 0] = precip->m_AxisEulerAngles[0];
  m_AxisEulerAngles[3 * gnum + 1] = precip->m_AxisEulerAngles[1];
  m_AxisEulerAngles[3 * gnum + 2] = precip->m_AxisEulerAngles[2];
  m_Omega3s[gnum] = precip->m_Omega3s;
  m_FieldPhases[gnum] = precip->m_FieldPhases;
}

void InsertPrecipitatePhases::move_precipitate(size_t gnum, float xc, float yc, float zc)
{
  int occolumn, ocrow, ocplane;
  int nccolumn, ncrow, ncplane;
  int shiftcolumn, shiftrow, shiftplane;
  float oxc = m_Centroids[3 * gnum];
  float oyc = m_Centroids[3 * gnum + 1];
  float ozc = m_Centroids[3 * gnum + 2];
  occolumn = static_cast<int>( (oxc - (m_PackingRes[0] / 2.0f)) / m_PackingRes[0] );
  ocrow = static_cast<int>( (oyc - (m_PackingRes[1] / 2.0f)) / m_PackingRes[1] );
  ocplane = static_cast<int>( (ozc - (m_PackingRes[2] / 2.0f)) / m_PackingRes[2] );
  nccolumn = static_cast<int>( (xc - (m_PackingRes[0] / 2.0f)) / m_PackingRes[0] );
  ncrow = static_cast<int>( (yc - (m_PackingRes[1] / 2.0f)) / m_PackingRes[1] );
  ncplane = static_cast<int>( (zc - (m_PackingRes[2] / 2.0f)) / m_PackingRes[2] );
  shiftcolumn = nccolumn - occolumn;
  shiftrow = ncrow - ocrow;
  shiftplane = ncplane - ocplane;
  m_Centroids[3 * gnum] = xc;
  m_Centroids[3 * gnum + 1] = yc;
  m_Centroids[3 * gnum + 2] = zc;
  size_t size = columnlist[gnum].size();

  for (size_t i = 0; i < size; i++)
  {
    int& cl = columnlist[gnum][i];
    cl += shiftcolumn;
    int& rl = rowlist[gnum][i];
    rl += shiftrow;
    int& pl = planelist[gnum][i];
    pl += shiftplane;
  }
}


void InsertPrecipitatePhases::determine_clustering(size_t gnum, int add)
{
  VolumeDataContainer* m = getDataContainerArray()->getDataContainerAs<VolumeDataContainer>(getDataContainerName());

  float x, y, z;
  float xn, yn, zn;
  float r;
  float dia, dia2;
  int iter = 0;
  int diabin, dia2bin, clusterbin;

  int phase = m_FieldPhases[gnum];
  while (phase != precipitatephases[iter]) iter++;

  StatsDataArray& statsDataArray = *m_StatsDataArray;
  typedef QVector<QVector<float> > VectOfVectFloat_t;

  PrecipitateStatsData* pp = PrecipitateStatsData::SafePointerDownCast(statsDataArray[phase].get());
  VectOfVectFloat_t& curSimClusteringDist = simclusteringdist[iter];
  size_t curSImClusteringDist_Size = curSimClusteringDist.size();
  float oneOverClusteringDistStep = 1.0f / clusteringdiststep[iter];

  float maxGrainDia = pp->getMaxGrainDiameter();
  float minGrainDia = pp->getMinGrainDiameter();
  float oneOverBinStepSize = 1.0f / pp->getBinStepSize();

  x = m_Centroids[3 * gnum];
  y = m_Centroids[3 * gnum + 1];
  z = m_Centroids[3 * gnum + 2];
  size_t numFields = m->getNumCellFieldTuples();
  for (size_t n = firstPrecipitateField; n < numFields; n++)
  {
    if (m_FieldPhases[n] == phase)
    {
      xn = m_Centroids[3 * n];
      yn = m_Centroids[3 * n + 1];
      zn = m_Centroids[3 * n + 2];
      r = sqrtf((x - xn) * (x - xn) + (y - yn) * (y - yn) + (z - zn) * (z - zn));

      dia = m_EquivalentDiameters[gnum];
      dia2 = m_EquivalentDiameters[n];
      if(dia > maxGrainDia) { dia = maxGrainDia; }
      if(dia < minGrainDia) { dia = minGrainDia; }
      if(dia2 > maxGrainDia) { dia2 = maxGrainDia; }
      if(dia2 < minGrainDia) { dia2 = minGrainDia; }
      diabin = static_cast<size_t>(((dia - minGrainDia) * oneOverBinStepSize) );
      dia2bin = static_cast<size_t>(((dia2 - minGrainDia) * oneOverBinStepSize) );
      clusterbin = static_cast<size_t>( r * oneOverClusteringDistStep );
      if(clusterbin >= 40) { clusterbin = 39; }
      curSimClusteringDist[diabin][clusterbin] += add;
      curSimClusteringDist[dia2bin][clusterbin] += add;
    }
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
float InsertPrecipitatePhases::check_clusteringerror(int gadd, int gremove)
{
  VolumeDataContainer* m = getDataContainerArray()->getDataContainerAs<VolumeDataContainer>(getDataContainerName());

  float clusteringerror;
  float bhattdist;
  float dia;
  int nnum;
  size_t diabin = 0;
  size_t nnumbin = 0;
  int index = 0;

  int counter = 0;
  int phase;
  for (size_t iter = 0; iter < simclusteringdist.size(); ++iter)
  {
    if(gadd > 0 && m_FieldPhases[gadd] == phase)
    {
      determine_clustering(gadd, 1);
    }
    if(gremove > 0 && m_FieldPhases[gremove] == phase)
    {
      determine_clustering(gremove, -1);
    }
  }
  compare_3Ddistributions(simclusteringdist, clusteringdist, bhattdist);
  clusteringerror = bhattdist;
  return clusteringerror;
}
void InsertPrecipitatePhases::compare_1Ddistributions(QVector<float> array1, QVector<float> array2, float& bhattdist)
{
  bhattdist = 0;
  for (size_t i = 0; i < array1.size(); i++)
  {
    bhattdist = bhattdist + sqrt((array1[i] * array2[i]));
  }
}
void InsertPrecipitatePhases::compare_2Ddistributions(QVector<QVector<float> > array1, QVector<QVector<float> > array2, float& bhattdist)
{
  bhattdist = 0;
  for (size_t i = 0; i < array1.size(); i++)
  {
    for (size_t j = 0; j < array1[i].size(); j++)
    {
      bhattdist = bhattdist + sqrt((array1[i][j] * array2[i][j]));
    }
  }
}

void InsertPrecipitatePhases::compare_3Ddistributions(QVector<QVector<QVector<float> > > array1, QVector<QVector<QVector<float> > > array2, float& bhattdist)
{
  bhattdist = 0;
  QVector<QVector<float> > counts1(array1.size());
  QVector<QVector<float> > counts2(array2.size());
  for (size_t i = 0; i < array1.size(); i++)
  {
    counts1[i].resize(array1[i].size());
    counts2[i].resize(array2[i].size());
    for (size_t j = 0; j < array1[i].size(); j++)
    {
      for (size_t k = 0; k < array1[i][j].size(); k++)
      {
        counts1[i][j] += array1[i][j][k];
        counts2[i][j] += array2[i][j][k];
      }
    }
  }
  for (size_t i = 0; i < array1.size(); i++)
  {
    for (size_t j = 0; j < array1[i].size(); j++)
    {
      for (size_t k = 0; k < array1[i][j].size(); k++)
      {
        bhattdist = bhattdist + sqrt(((array1[i][j][k] / counts1[i][j]) * (array2[i][j][k] / counts2[i][j])));
      }
    }
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
float InsertPrecipitatePhases::check_sizedisterror(Precip* precip)
{
  VolumeDataContainer* m = getDataContainerArray()->getDataContainerAs<VolumeDataContainer>(getDataContainerName());

  StatsDataArray& statsDataArray = *m_StatsDataArray;

  float dia;
  float sizedisterror = 0;
  float bhattdist;
  int index;
  int count = 0;
  int phase;
  size_t grainSizeDist_Size = grainsizedist.size();
  for (size_t iter = 0; iter < grainSizeDist_Size; ++iter)
  {
    phase = precipitatephases[iter];
    PrecipitateStatsData* pp = PrecipitateStatsData::SafePointerDownCast(statsDataArray[phase].get());
    count = 0;
    QVector<float>& curGrainSizeDist = grainsizedist[iter];
    QVector<float>::size_type curGrainSizeDistSize = curGrainSizeDist.size();
    QVector<float>& curSimGrainSizeDist = simgrainsizedist[iter];
    // Initialize all Values to Zero
    for (size_t i = 0; i < curGrainSizeDistSize; i++)
    {
      curSimGrainSizeDist[i] = 0.0f;
    }

    size_t nFieldTuples = m->getNumCellFieldTuples();
    float oneOverCurGrainSizeDistStep = 1.0f / grainsizediststep[iter];
    float halfMinGrainDiameter = pp->getMinGrainDiameter() * 0.5f;
    for (size_t b = firstPrecipitateField; b < nFieldTuples; b++)
    {
      index = b;
      if(m_FieldPhases[index] == phase)
      {
        dia = m_EquivalentDiameters[index];
        dia = (dia - halfMinGrainDiameter) * oneOverCurGrainSizeDistStep;
        if(dia < 0) { dia = 0; }
        if(dia > curGrainSizeDistSize - 1.0f) { dia = curGrainSizeDistSize - 1.0f; }
        curSimGrainSizeDist[int(dia)]++;
        count++;
      }
    }

    if(precip->m_FieldPhases == phase)
    {
      dia = precip->m_EquivalentDiameters;
      dia = (dia - halfMinGrainDiameter) * oneOverCurGrainSizeDistStep;
      if(dia < 0) { dia = 0; }
      if(dia > curGrainSizeDistSize - 1.0f) { dia = curGrainSizeDistSize - 1.0f; }
      curSimGrainSizeDist[int(dia)]++;
      count++;
    }
    float oneOverCount = 1.0f / count;

    if (count == 0)
    {
      for (size_t i = 0; i < curGrainSizeDistSize; i++) { curSimGrainSizeDist[i] = 0.0; }
    }
    else
    {
      for (size_t i = 0; i < curGrainSizeDistSize; i++)
      {
        curSimGrainSizeDist[i] = curSimGrainSizeDist[i] * oneOverCount;
      }
    }
  }
  compare_2Ddistributions(simgrainsizedist, grainsizedist, bhattdist);
  sizedisterror = bhattdist;
  return sizedisterror;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
float InsertPrecipitatePhases::check_fillingerror(int gadd, int gremove, Int32ArrayType::Pointer grainOwnersPtr)
{
  size_t grainOwnersIdx = 0;
  int32_t* grainOwners = grainOwnersPtr->getPointer(0);
  fillingerror = fillingerror * float(m_TotalPackingPoints);
  int col, row, plane;
  if(gadd > 0)
  {
    size_t size = columnlist[gadd].size();
    QVector<int>& cl_gadd = columnlist[gadd];
    QVector<int>& rl_gadd = rowlist[gadd];
    QVector<int>& pl_gadd = planelist[gadd];
    float packquality = 0;
    for (size_t i = 0; i < size; i++)
    {
      col = cl_gadd[i];
      row = rl_gadd[i];
      plane = pl_gadd[i];

      if(m_PeriodicBoundaries == true)
      {
        if(col < 0) { col = col + m_PackingPoints[0]; }
        if(col > m_PackingPoints[0] - 1) { col = col - m_PackingPoints[0]; }
        if(row < 0) { row = row + m_PackingPoints[1]; }
        if(row > m_PackingPoints[1] - 1) { row = row - m_PackingPoints[1]; }
        if(plane < 0) { plane = plane + m_PackingPoints[2]; }
        if(plane > m_PackingPoints[2] - 1) { plane = plane - m_PackingPoints[2]; }
        grainOwnersIdx = (m_PackingPoints[0] * m_PackingPoints[1] * plane) + (m_PackingPoints[0] * row) + col;
        int currentGrainOwner = grainOwners[grainOwnersIdx];
        fillingerror = fillingerror + (2 * currentGrainOwner - 1);
        packquality = packquality + ((currentGrainOwner) * (currentGrainOwner));
        ++currentGrainOwner;
      }
      else
      {
        if(col >= 0 && col <= m_PackingPoints[0] - 1 && row >= 0 && row <= m_PackingPoints[1] - 1 && plane >= 0 && plane <= m_PackingPoints[2] - 1)
        {
          grainOwnersIdx = (m_PackingPoints[0] * m_PackingPoints[1] * plane) + (m_PackingPoints[0] * row) + col;
          int currentGrainOwner = grainOwners[grainOwnersIdx];
          fillingerror = fillingerror + (2 * currentGrainOwner - 1);
          packquality = packquality + ((currentGrainOwner) * (currentGrainOwner));
          ++currentGrainOwner;
        }
      }
    }
    packqualities[gadd] = static_cast<int>( packquality / float(size) );
  }
  if(gremove > 0)
  {
    size_t size = columnlist[gremove].size();
    QVector<int>& cl_gremove = columnlist[gremove];
    QVector<int>& rl_gremove = rowlist[gremove];
    QVector<int>& pl_gremove = planelist[gremove];
    for (size_t i = 0; i < size; i++)
    {
      col = cl_gremove[i];
      row = rl_gremove[i];
      plane = pl_gremove[i];
      if(m_PeriodicBoundaries == true)
      {
        if(col < 0) { col = col + m_PackingPoints[0]; }
        if(col > m_PackingPoints[0] - 1) { col = col - m_PackingPoints[0]; }
        if(row < 0) { row = row + m_PackingPoints[1]; }
        if(row > m_PackingPoints[1] - 1) { row = row - m_PackingPoints[1]; }
        if(plane < 0) { plane = plane + m_PackingPoints[2]; }
        if(plane > m_PackingPoints[2] - 1) { plane = plane - m_PackingPoints[2]; }
        grainOwnersIdx = (m_PackingPoints[0] * m_PackingPoints[1] * plane) + (m_PackingPoints[0] * row) + col;
        int currentGrainOwner = grainOwners[grainOwnersIdx];
        fillingerror = fillingerror + (-2 * currentGrainOwner + 3);
        currentGrainOwner = currentGrainOwner - 1;
      }
      else
      {
        if(col >= 0 && col <= m_PackingPoints[0] - 1 && row >= 0 && row <= m_PackingPoints[1] - 1 && plane >= 0 && plane <= m_PackingPoints[2] - 1)
        {
          grainOwnersIdx = (m_PackingPoints[0] * m_PackingPoints[1] * plane) + (m_PackingPoints[0] * row) + col;
          int currentGrainOwner = grainOwners[grainOwnersIdx];
          fillingerror = fillingerror + (-2 * currentGrainOwner + 3);
          currentGrainOwner = currentGrainOwner - 1;
        }
      }
    }
  }
  fillingerror = fillingerror / float(m_TotalPackingPoints);
  return fillingerror;
}

void InsertPrecipitatePhases::insert_precipitate(size_t gnum)
{
  DREAM3D_RANDOMNG_NEW()
  //   DataContainer* m = getDataContainerArray()->getDataContainerAs<VolumeDataContainer>(getDataContainerName());
  //  float dist;
  float inside = -1;
  int column, row, plane;
  int centercolumn, centerrow, centerplane;
  int xmin, xmax, ymin, ymax, zmin, zmax;
  float xc, yc, zc;
  float coordsRotated[3];
  float coords[3];
  float volcur = m_Volumes[gnum];
  float bovera = m_AxisLengths[3 * gnum + 1];
  float covera = m_AxisLengths[3 * gnum + 2];
  float omega3 = m_Omega3s[gnum];
  float radcur1 = 1;
  unsigned int shapeclass = m_ShapeTypes[m_FieldPhases[gnum]];

  // init any values for each of the Shape Ops
  for (QMap<unsigned int, ShapeOps*>::iterator ops = m_ShapeOps.begin(); ops != m_ShapeOps.end(); ++ops)
  {
    ops.value()->init();
  }
  // Create our Argument Map
  QMap<ShapeOps::ArgName, float> shapeArgMap;
  shapeArgMap[ShapeOps::Omega3] = omega3;
  shapeArgMap[ShapeOps::VolCur] = volcur;
  shapeArgMap[ShapeOps::B_OverA] = bovera;
  shapeArgMap[ShapeOps::C_OverA] = covera;

  radcur1 = m_ShapeOps[shapeclass]->radcur1(shapeArgMap);

  float radcur2 = (radcur1 * bovera);
  float radcur3 = (radcur1 * covera);
  float phi1 = m_AxisEulerAngles[3 * gnum];
  float PHI = m_AxisEulerAngles[3 * gnum + 1];
  float phi2 = m_AxisEulerAngles[3 * gnum + 2];
  float ga[3][3];
  OrientationMath::EulertoMat(phi1, PHI, phi2, ga);
  xc = m_Centroids[3 * gnum];
  yc = m_Centroids[3 * gnum + 1];
  zc = m_Centroids[3 * gnum + 2];
  centercolumn = static_cast<int>( (xc - (m_PackingRes[0] / 2)) / m_PackingRes[0] );
  centerrow = static_cast<int>( (yc - (m_PackingRes[1] / 2)) / m_PackingRes[1] );
  centerplane = static_cast<int>( (zc - (m_PackingRes[2] / 2)) / m_PackingRes[2] );
  xmin = int(centercolumn - ((radcur1 / m_PackingRes[0]) + 1));
  xmax = int(centercolumn + ((radcur1 / m_PackingRes[0]) + 1));
  ymin = int(centerrow - ((radcur1 / m_PackingRes[1]) + 1));
  ymax = int(centerrow + ((radcur1 / m_PackingRes[1]) + 1));
  zmin = int(centerplane - ((radcur1 / m_PackingRes[2]) + 1));
  zmax = int(centerplane + ((radcur1 / m_PackingRes[2]) + 1));
  if(xmin < -m_PackingPoints[0]) { xmin = -m_PackingPoints[0]; }
  if(xmax > 2 * m_PackingPoints[0] - 1) { xmax = (2 * m_PackingPoints[0] - 1); }
  if(ymin < -m_PackingPoints[1]) { ymin = -m_PackingPoints[1]; }
  if(ymax > 2 * m_PackingPoints[1] - 1) { ymax = (2 * m_PackingPoints[1] - 1); }
  if(zmin < -m_PackingPoints[2]) { zmin = -m_PackingPoints[2]; }
  if(zmax > 2 * m_PackingPoints[2] - 1) { zmax = (2 * m_PackingPoints[2] - 1); }
  for (int iter1 = xmin; iter1 < xmax + 1; iter1++)
  {
    for (int iter2 = ymin; iter2 < ymax + 1; iter2++)
    {
      for (int iter3 = zmin; iter3 < zmax + 1; iter3++)
      {
        column = iter1;
        row = iter2;
        plane = iter3;
        coords[0] = float(column) * m_PackingRes[0];
        coords[1] = float(row) * m_PackingRes[1];
        coords[2] = float(plane) * m_PackingRes[2];
        inside = -1;
        coords[0] = coords[0] - xc;
        coords[1] = coords[1] - yc;
        coords[2] = coords[2] - zc;
        MatrixMath::Multiply3x3with3x1(ga, coords, coordsRotated);
        float axis1comp = coordsRotated[0] / radcur1;
        float axis2comp = coordsRotated[1] / radcur2;
        float axis3comp = coordsRotated[2] / radcur3;
        inside = m_ShapeOps[shapeclass]->inside(axis1comp, axis2comp, axis3comp);
        if(inside >= 0)
        {
          columnlist[gnum].push_back(column);
          rowlist[gnum].push_back(row);
          planelist[gnum].push_back(plane);
        }
      }
    }
  }
}

void InsertPrecipitatePhases::assign_voxels()
{
  notifyStatusMessage("Assigning Voxels");

  VolumeDataContainer* m = getDataContainerArray()->getDataContainerAs<VolumeDataContainer>(getDataContainerName());

  int index;
  size_t udims[3] = {0, 0, 0};
  m->getDimensions(udims);
#if (CMP_SIZEOF_SIZE_T == 4)
  typedef int32_t DimType;
#else
  typedef int64_t DimType;
#endif
  DimType dims[3] =
  {
    static_cast<DimType>(udims[0]),
    static_cast<DimType>(udims[1]),
    static_cast<DimType>(udims[2]),
  };

  DimType neighpoints[6];
  neighpoints[0] = -dims[0] * dims[1];
  neighpoints[1] = -dims[0];
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = dims[0];
  neighpoints[5] = dims[0] * dims[1];

  float totalPoints = dims[0] * dims[1] * dims[2];
  float xRes = m->getXRes();
  float yRes = m->getYRes();
  float zRes = m->getZRes();

  int oldname;
  size_t column, row, plane;
  float inside;
  float xc, yc, zc;
  float coordsRotated[3];
  //float dist;
  float coords[3];
  DimType xmin, xmax, ymin, ymax, zmin, zmax;
  // int64_t totpoints = m->totalPoints();
  gsizes.resize(m->getNumCellFieldTuples());

  for (size_t i = firstPrecipitateField; i < m->getNumCellFieldTuples(); i++)
  {
    gsizes[i] = 0;
  }
  for (size_t i = firstPrecipitateField; i < m->getNumCellFieldTuples(); i++)
  {
    float volcur = m_Volumes[i];
    float bovera = m_AxisLengths[3 * i + 1];
    float covera = m_AxisLengths[3 * i + 2];
    float omega3 = m_Omega3s[i];
    xc = m_Centroids[3 * i];
    yc = m_Centroids[3 * i + 1];
    zc = m_Centroids[3 * i + 2];
    float radcur1 = 0.0f;
    //Unbounded Check for the size of shapeTypes. We assume a 1:1 with phase
    unsigned int shapeclass = m_ShapeTypes[m_FieldPhases[i]];

    // init any values for each of the Shape Ops
    for (QMap<unsigned int, ShapeOps*>::iterator ops = m_ShapeOps.begin(); ops != m_ShapeOps.end(); ++ops )
    {
      ops.value()->init();
    }
    // Create our Argument Map
    QMap<ShapeOps::ArgName, float> shapeArgMap;
    shapeArgMap[ShapeOps::Omega3] = omega3;
    shapeArgMap[ShapeOps::VolCur] = volcur;
    shapeArgMap[ShapeOps::B_OverA] = bovera;
    shapeArgMap[ShapeOps::C_OverA] = covera;

    radcur1 = m_ShapeOps[shapeclass]->radcur1(shapeArgMap);

    float radcur2 = (radcur1 * bovera);
    float radcur3 = (radcur1 * covera);
    float phi1 = m_AxisEulerAngles[3 * i];
    float PHI = m_AxisEulerAngles[3 * i + 1];
    float phi2 = m_AxisEulerAngles[3 * i + 2];
    float ga[3][3];
    OrientationMath::EulertoMat(phi1, PHI, phi2, ga);
    column = static_cast<size_t>( (xc - (xRes / 2.0f)) / xRes );
    row = static_cast<size_t>( (yc - (yRes / 2.0f)) / yRes );
    plane = static_cast<size_t>( (zc - (zRes / 2.0f)) / zRes );
    xmin = int(column - ((radcur1 / xRes) + 1));
    xmax = int(column + ((radcur1 / xRes) + 1));
    ymin = int(row - ((radcur1 / yRes) + 1));
    ymax = int(row + ((radcur1 / yRes) + 1));
    zmin = int(plane - ((radcur1 / zRes) + 1));
    zmax = int(plane + ((radcur1 / zRes) + 1));
    if (m_PeriodicBoundaries == true)
    {
      if (xmin < -dims[0]) { xmin = -dims[0]; }
      if (xmax > 2 * dims[0] - 1) { xmax = (2 * dims[0] - 1); }
      if (ymin < -dims[1]) { ymin = -dims[1]; }
      if (ymax > 2 * dims[1] - 1) { ymax = (2 * dims[1] - 1); }
      if (zmin < -dims[2]) { zmin = -dims[2]; }
      if (zmax > 2 * dims[2] - 1) { zmax = (2 * dims[2] - 1); }
    }
    if (m_PeriodicBoundaries == false)
    {
      if (xmin < 0) { xmin = 0; }
      if (xmax > dims[0] - 1) { xmax = dims[0] - 1; }
      if (ymin < 0) { ymin = 0; }
      if (ymax > dims[1] - 1) { ymax = dims[1] - 1; }
      if (zmin < 0) { zmin = 0; }
      if (zmax > dims[2] - 1) { zmax = dims[2] - 1; }
    }
    for (DimType iter1 = xmin; iter1 < xmax + 1; iter1++)
    {
      for (DimType iter2 = ymin; iter2 < ymax + 1; iter2++)
      {
        for (DimType iter3 = zmin; iter3 < zmax + 1; iter3++)
        {
          column = iter1;
          row = iter2;
          plane = iter3;
          if (iter1 < 0) { column = iter1 + dims[0]; }
          if (iter1 > dims[0] - 1) { column = iter1 - dims[0]; }
          if (iter2 < 0) { row = iter2 + dims[1]; }
          if (iter2 > dims[1] - 1) { row = iter2 - dims[1]; }
          if (iter3 < 0) { plane = iter3 + dims[2]; }
          if (iter3 > dims[2] - 1) { plane = iter3 - dims[2]; }
          index = (plane * dims[0] * dims[1]) + (row * dims[0]) + column;
          inside = -1;
          coords[0] = float(column) * xRes;
          coords[1] = float(row) * yRes;
          coords[2] = float(plane) * zRes;
          if (iter1 < 0) { coords[0] = coords[0] - sizex; }
          if (iter1 > dims[0] - 1) { coords[0] = coords[0] + sizex; }
          if (iter2 < 0) { coords[1] = coords[1] - sizey; }
          if (iter2 > dims[1] - 1) { coords[1] = coords[1] + sizey; }
          if (iter3 < 0) { coords[2] = coords[2] - sizez; }
          if (iter3 > dims[2] - 1) { coords[2] = coords[2] + sizez; }
//          dist = ((coords[0] - xc) * (coords[0] - xc)) + ((coords[1] - yc) * (coords[1] - yc)) + ((coords[2] - zc) * (coords[2] - zc));
//          dist = sqrtf(dist);
//          if (dist < radcur1)
//          {
          coords[0] = coords[0] - xc;
          coords[1] = coords[1] - yc;
          coords[2] = coords[2] - zc;
          MatrixMath::Multiply3x3with3x1(ga, coords, coordsRotated);
          float axis1comp = coordsRotated[0] / radcur1;
          float axis2comp = coordsRotated[1] / radcur2;
          float axis3comp = coordsRotated[2] / radcur3;
          inside = m_ShapeOps[shapeclass]->inside(axis1comp, axis2comp, axis3comp);
          if (inside >= 0)
          {
            int currentpoint = index;
            if (m_GrainIds[currentpoint] > firstPrecipitateField)
            {
              oldname = m_GrainIds[currentpoint];
              m_GrainIds[currentpoint] = -2;
            }
            if (m_GrainIds[currentpoint] < firstPrecipitateField && m_GrainIds[currentpoint] != -2)
            {
              m_GrainIds[currentpoint] = static_cast<int32_t>(i);
            }
          }
//          }
        }
      }
    }
  }
  for (size_t i = firstPrecipitateField; i < m->getNumCellFieldTuples(); i++)
  {
    m_Active[i] = false;
  }
  int gnum;
  for(size_t i = 0; i < totalPoints; i++)
  {
    gnum = m_GrainIds[i];
    if(gnum >= 0) { m_Active[gnum] = true; }
  }
}

void InsertPrecipitatePhases::assign_gaps()
{
  notifyStatusMessage("Assigning Gaps");

  VolumeDataContainer* m = getDataContainerArray()->getDataContainerAs<VolumeDataContainer>(getDataContainerName());

  int64_t totpoints = m->getTotalPoints();

  size_t udims[3] = {0, 0, 0};
  m->getDimensions(udims);
#if (CMP_SIZEOF_SIZE_T == 4)
  typedef int32_t DimType;
#else
  typedef int64_t DimType;
#endif
  DimType dims[3] =
  {
    static_cast<DimType>(udims[0]),
    static_cast<DimType>(udims[1]),
    static_cast<DimType>(udims[2]),
  };


  int index;
  int timestep = 100;
  int unassignedcount = 1;
  DimType column, row, plane;
  float inside;
  float xc, yc, zc;
  float coordsRotated[3];
  float dist;
  float coords[3];

  DimType xmin, xmax, ymin, ymax, zmin, zmax;

  float xRes = m->getXRes();
  float yRes = m->getYRes();
  float zRes = m->getZRes();

  Int32ArrayType::Pointer newownersPtr = Int32ArrayType::CreateArray(totpoints, "newowners");
  int32_t* newowners = newownersPtr->getPointer(0);
  newownersPtr->initializeWithZeros();

  FloatArrayType::Pointer ellipfuncsPtr = FloatArrayType::CreateArray(totpoints, "ellipfuncs");
  float* ellipfuncs = ellipfuncsPtr->getPointer(0);
  ellipfuncsPtr->initializeWithValues(-1);

  while (unassignedcount != 0)
  {
    unassignedcount = 0;
    timestep = timestep + 50;
    for (size_t i = firstPrecipitateField; i < m->getNumCellFieldTuples(); i++)
    {
      float volcur = m_Volumes[i];
      float bovera = m_AxisLengths[3 * i + 1];
      float covera = m_AxisLengths[3 * i + 2];
      float omega3 = m_Omega3s[i];
      xc = m_Centroids[3 * i];
      yc = m_Centroids[3 * i + 1];
      zc = m_Centroids[3 * i + 2];
      float radcur1 = 0.0f;
      //Unbounded Check for the size of shapeTypes. We assume a 1:1 with phase
      unsigned int shapeclass = m_ShapeTypes[m_FieldPhases[i]];

      // init any values for each of the Shape Ops
      for (QMap<unsigned int, ShapeOps*>::iterator ops = m_ShapeOps.begin(); ops != m_ShapeOps.end(); ++ops )
      {
        ops.value()->init();
      }
      // Create our Argument Map
      QMap<ShapeOps::ArgName, float> shapeArgMap;
      shapeArgMap[ShapeOps::Omega3] = omega3;
      shapeArgMap[ShapeOps::VolCur] = volcur;
      shapeArgMap[ShapeOps::B_OverA] = bovera;
      shapeArgMap[ShapeOps::C_OverA] = covera;

      radcur1 = m_ShapeOps[shapeclass]->radcur1(shapeArgMap);

      float radcur2 = (radcur1 * bovera);
      float radcur3 = (radcur1 * covera);
      radcur1 = (float(timestep) / 100.0f) * radcur1;
      radcur2 = (float(timestep) / 100.0f) * radcur2;
      radcur3 = (float(timestep) / 100.0f) * radcur3;
      float phi1 = m_AxisEulerAngles[3 * i];
      float PHI = m_AxisEulerAngles[3 * i + 1];
      float phi2 = m_AxisEulerAngles[3 * i + 2];
      float ga[3][3];
      OrientationMath::EulertoMat(phi1, PHI, phi2, ga);
      column = static_cast<DimType>( (xc - (xRes / 2.0f)) / xRes );
      row = static_cast<DimType>( (yc - (yRes / 2.0f)) / yRes );
      plane = static_cast<DimType>( (zc - (zRes / 2.0f)) / zRes );
      xmin = int(column - ((radcur1 / xRes) + 1));
      xmax = int(column + ((radcur1 / xRes) + 1));
      ymin = int(row - ((radcur1 / yRes) + 1));
      ymax = int(row + ((radcur1 / yRes) + 1));
      zmin = int(plane - ((radcur1 / zRes) + 1));
      zmax = int(plane + ((radcur1 / zRes) + 1));
      if (m_PeriodicBoundaries == true)
      {
        if (xmin < -dims[0]) { xmin = -dims[0]; }
        if (xmax > 2 * dims[0] - 1) { xmax = (2 * dims[0] - 1); }
        if (ymin < -dims[1]) { ymin = -dims[1]; }
        if (ymax > 2 * dims[1] - 1) { ymax = (2 * dims[1] - 1); }
        if (zmin < -dims[2]) { zmin = -dims[2]; }
        if (zmax > 2 * dims[2] - 1) { zmax = (2 * dims[2] - 1); }
      }
      if (m_PeriodicBoundaries == false)
      {
        if (xmin < 0) { xmin = 0; }
        if (xmax > dims[0] - 1) { xmax = dims[0] - 1; }
        if (ymin < 0) { ymin = 0; }
        if (ymax > dims[1] - 1) { ymax = dims[1] - 1; }
        if (zmin < 0) { zmin = 0; }
        if (zmax > dims[2] - 1) { zmax = dims[2] - 1; }
      }
      for (DimType iter1 = xmin; iter1 < xmax + 1; iter1++)
      {
        for (DimType iter2 = ymin; iter2 < ymax + 1; iter2++)
        {
          for (DimType iter3 = zmin; iter3 < zmax + 1; iter3++)
          {
            column = iter1;
            row = iter2;
            plane = iter3;
            if (iter1 < 0) { column = iter1 + dims[0]; }
            if (iter1 > dims[0] - 1) { column = iter1 - dims[0]; }
            if (iter2 < 0) { row = iter2 + dims[1]; }
            if (iter2 > dims[1] - 1) { row = iter2 - dims[1]; }
            if (iter3 < 0) { plane = iter3 + dims[2]; }
            if (iter3 > dims[2] - 1) { plane = iter3 - dims[2]; }
            index = static_cast<int>( (plane * dims[0] * dims[1]) + (row * dims[0]) + column );
            if(m_GrainIds[index] <= 0)
            {
              inside = -1;
              coords[0] = float(column) * xRes;
              coords[1] = float(row) * yRes;
              coords[2] = float(plane) * zRes;
              if (iter1 < 0) { coords[0] = coords[0] - sizex; }
              if (iter1 > dims[0] - 1) { coords[0] = coords[0] + sizex; }
              if (iter2 < 0) { coords[1] = coords[1] - sizey; }
              if (iter2 > dims[1] - 1) { coords[1] = coords[1] + sizey; }
              if (iter3 < 0) { coords[2] = coords[2] - sizez; }
              if (iter3 > dims[2] - 1) { coords[2] = coords[2] + sizez; }
              dist = ((coords[0] - xc) * (coords[0] - xc)) + ((coords[1] - yc) * (coords[1] - yc)) + ((coords[2] - zc) * (coords[2] - zc));
              dist = sqrtf(dist);
              if (dist < radcur1)
              {
                coords[0] = coords[0] - xc;
                coords[1] = coords[1] - yc;
                coords[2] = coords[2] - zc;
                MatrixMath::Multiply3x3with3x1(ga, coords, coordsRotated);
                float axis1comp = coordsRotated[0] / radcur1;
                float axis2comp = coordsRotated[1] / radcur2;
                float axis3comp = coordsRotated[2] / radcur3;
                inside = m_ShapeOps[shapeclass]->inside(axis1comp, axis2comp, axis3comp);
                if (inside >= 0 && inside > ellipfuncs[index])
                {
                  newowners[index] = i;
                  ellipfuncs[index] = inside;
                }
              }
            }
          }
        }
      }
    }
    for (int i = 0; i < totpoints; i++)
    {
      if (ellipfuncs[i] >= 0) { m_GrainIds[i] = newowners[i]; }
      if (m_GrainIds[i] <= 0) { unassignedcount++; }
      newowners[i] = -1;
      ellipfuncs[i] = -1.0;
    }
  }
  for (int i = 0; i < totpoints; i++)
  {
    if(m_GrainIds[i] > 0) { m_CellPhases[i] = m_FieldPhases[m_GrainIds[i]]; }
  }
}
void InsertPrecipitatePhases::cleanup_grains()
{
  notifyStatusMessage("Cleaning Up Grains");

  VolumeDataContainer* m = getDataContainerArray()->getDataContainerAs<VolumeDataContainer>(getDataContainerName());

  StatsDataArray& statsDataArray = *m_StatsDataArray;

  int64_t totpoints = m->getTotalPoints();
  size_t udims[3] = {0, 0, 0};
  m->getDimensions(udims);
#if (CMP_SIZEOF_SIZE_T == 4)
  typedef int32_t DimType;
#else
  typedef int64_t DimType;
#endif
  DimType dims[3] =
  {
    static_cast<DimType>(udims[0]),
    static_cast<DimType>(udims[1]),
    static_cast<DimType>(udims[2]),
  };

  DimType neighpoints[6];
  DimType xp = dims[0];
  DimType yp = dims[1];
  DimType zp = dims[2];

  neighpoints[0] = -(xp * yp);
  neighpoints[1] = -xp;
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = xp;
  neighpoints[5] = (xp * yp);
  QVector<QVector<int> > vlists;
  vlists.resize(m->getNumCellFieldTuples());
  QVector<int> currentvlist;
  QVector<bool> checked(totpoints, false);
  size_t count;
  int touchessurface = 0;
  int good;
  int neighbor;
  DimType column, row, plane;
  int index;
  float minsize = 0;
  gsizes.resize(m->getNumCellFieldTuples());
  for (size_t i = 1; i < m->getNumCellFieldTuples(); i++)
  {
    gsizes[i] = 0;
    m_Active[i] = true;
  }

  float resConst = m->getXRes() * m->getYRes() * m->getZRes();
  for (int i = 0; i < totpoints; i++)
  {
    touchessurface = 0;
    if(checked[i] == false && m_GrainIds[i] > firstPrecipitateField)
    {
      PrecipitateStatsData* pp = PrecipitateStatsData::SafePointerDownCast(statsDataArray[m_CellPhases[i]].get());
      minsize = static_cast<float>( pp->getMinGrainDiameter() * pp->getMinGrainDiameter() * pp->getMinGrainDiameter() * M_PI / 6.0f );
      minsize = static_cast<float>( int(minsize / (resConst)) );
      currentvlist.push_back(i);
      count = 0;
      while (count < currentvlist.size())
      {
        index = currentvlist[count];
        column = index % xp;
        row = (index / xp) % yp;
        plane = index / (xp * yp);
        if(column == 0 || column == xp || row == 0 || row == yp || plane == 0 || plane == zp) { touchessurface = 1; }
        for (int j = 0; j < 6; j++)
        {
          good = 1;
          neighbor = static_cast<int>( index + neighpoints[j] );
          if(m_PeriodicBoundaries == false)
          {
            if(j == 0 && plane == 0) { good = 0; }
            if(j == 5 && plane == (zp - 1)) { good = 0; }
            if(j == 1 && row == 0) { good = 0; }
            if(j == 4 && row == (yp - 1)) { good = 0; }
            if(j == 2 && column == 0) { good = 0; }
            if(j == 3 && column == (xp - 1)) { good = 0; }
            if(good == 1 && m_GrainIds[neighbor] == m_GrainIds[index] && checked[neighbor] == false)
            {
              currentvlist.push_back(neighbor);
              checked[neighbor] = true;
            }
          }
          else if(m_PeriodicBoundaries == true)
          {
            if(j == 0 && plane == 0) { neighbor = static_cast<int>( neighbor + (xp * yp * zp) ); }
            if(j == 5 && plane == (zp - 1)) { neighbor = static_cast<int>( neighbor - (xp * yp * zp) ); }
            if(j == 1 && row == 0) { neighbor = static_cast<int>( neighbor + (xp * yp) ); }
            if(j == 4 && row == (yp - 1)) { neighbor = static_cast<int>( neighbor - (xp * yp) ); }
            if(j == 2 && column == 0) { neighbor = static_cast<int>( neighbor + (xp) ); }
            if(j == 3 && column == (xp - 1)) { neighbor = static_cast<int>( neighbor - (xp) ); }
            if(m_GrainIds[neighbor] == m_GrainIds[index] && checked[neighbor] == false)
            {
              currentvlist.push_back(neighbor);
              checked[neighbor] = true;
            }
          }
        }
        count++;
      }
      size_t size = vlists[m_GrainIds[i]].size();
      if(size > 0)
      {
        if(size < currentvlist.size())
        {
          for (size_t k = 0; k < vlists[m_GrainIds[i]].size(); k++)
          {
            m_GrainIds[vlists[m_GrainIds[i]][k]] = -1;
          }
          vlists[m_GrainIds[i]].resize(currentvlist.size());
          vlists[m_GrainIds[i]].swap(currentvlist);
        }
        else if(size >= currentvlist.size())
        {
          for (size_t k = 0; k < currentvlist.size(); k++)
          {
            m_GrainIds[currentvlist[k]] = -1;
          }
        }
      }
      else if(size == 0)
      {
        if(currentvlist.size() >= minsize || touchessurface == 1)
        {
          vlists[m_GrainIds[i]].resize(currentvlist.size());
          vlists[m_GrainIds[i]].swap(currentvlist);
        }
        if(currentvlist.size() < minsize && touchessurface == 0)
        {
          for (size_t k = 0; k < currentvlist.size(); k++)
          {
            m_GrainIds[currentvlist[k]] = -1;
          }
        }
      }
      currentvlist.clear();
    }
  }
  assign_gaps();
  for (int i = 0; i < totpoints; i++)
  {
    if(m_GrainIds[i] > 0) { gsizes[m_GrainIds[i]]++; }
  }
  for (size_t i = firstPrecipitateField; i < m->getNumCellFieldTuples(); i++)
  {
    if(gsizes[i] == 0) { m_Active[i] = false; }
  }
  for (int i = 0; i < totpoints; i++)
  {
    if(m_GrainIds[i] > 0) { m_CellPhases[i] = m_FieldPhases[m_GrainIds[i]]; }
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
Int32ArrayType::Pointer  InsertPrecipitatePhases::initialize_packinggrid()
{
  VolumeDataContainer* m = getDataContainerArray()->getDataContainerAs<VolumeDataContainer>(getDataContainerName());

  m_PackingRes[0] = m->getXRes() * 2.0f;
  m_PackingRes[1] = m->getYRes() * 2.0f;
  m_PackingRes[2] = m->getZRes() * 2.0f;

  m_HalfPackingRes[0] = m_PackingRes[0] * 0.5;
  m_HalfPackingRes[1] = m_PackingRes[1] * 0.5;
  m_HalfPackingRes[2] = m_PackingRes[2] * 0.5;

  m_OneOverHalfPackingRes[0] = 1.0f / m_HalfPackingRes[0];
  m_OneOverHalfPackingRes[1] = 1.0f / m_HalfPackingRes[1];
  m_OneOverHalfPackingRes[2] = 1.0f / m_HalfPackingRes[2];

  m_PackingPoints[0] = m->getXPoints() / 2;
  m_PackingPoints[1] = m->getYPoints() / 2;
  m_PackingPoints[2] = m->getZPoints() / 2;

  m_TotalPackingPoints = m_PackingPoints[0] * m_PackingPoints[1] * m_PackingPoints[2];

  Int32ArrayType::Pointer grainOwnersPtr = Int32ArrayType::CreateArray(m_TotalPackingPoints, 1, "PackPrimaryGrains::grain_owners");
  grainOwnersPtr->initializeWithZeros();

  return grainOwnersPtr;
}


float InsertPrecipitatePhases::find_xcoord(long long int index)
{
  VolumeDataContainer* m = getDataContainerArray()->getDataContainerAs<VolumeDataContainer>(getDataContainerName());

  float x = m->getXRes() * float(index % m->getXPoints());
  return x;
}
float InsertPrecipitatePhases::find_ycoord(long long int index)
{
  VolumeDataContainer* m = getDataContainerArray()->getDataContainerAs<VolumeDataContainer>(getDataContainerName());

  float y = m->getYRes() * float((index / m->getXPoints()) % m->getYPoints());
  return y;
}
float InsertPrecipitatePhases::find_zcoord(long long int index)
{
  VolumeDataContainer* m = getDataContainerArray()->getDataContainerAs<VolumeDataContainer>(getDataContainerName());

  float z = m->getZRes() * float(index / (m->getXPoints() * m->getYPoints()));
  return z;
}
void InsertPrecipitatePhases::write_goal_attributes()
{
  int err = 0;
  setErrorCondition(err);
  VolumeDataContainer* m = getDataContainerArray()->getDataContainerAs<VolumeDataContainer>(getDataContainerName());

  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path

  QFileInfo fi(m_CsvOutputFile);
  QString parentPath = fi.path();
  QDir dir;
  if(!dir.mkpath(parentPath))
  {
    QString ss = QObject::tr("Error creating parent path '%1'").arg(parentPath);
    notifyErrorMessage(ss, -1);
    setErrorCondition(-1);
    return;
  }


  QFile outFile(getCsvOutputFile());
  if (!outFile.open(QIODevice::WriteOnly))
  {
    QString msg = QObject::tr("CSV Output file could not be opened: %1").arg(getCsvOutputFile());
    setErrorCondition(-200);
    notifyErrorMessage(msg, getErrorCondition());
    return;
  }

  QTextStream dStream(&outFile);

  char space = DREAM3D::GrainData::Delimiter;
  // Write the total number of grains
  dStream << static_cast<qint32>(m->getNumCellFieldTuples() - firstPrecipitateField);
  // Get all the names of the arrays from the Data Container
  QList<QString> headers = m->getCellFieldArrayNameList();

  QVector<IDataArray::Pointer> data;

  //For checking if an array is a neighborlist
  NeighborList<float>::Pointer neighborlistPtr = NeighborList<float>::New();

  // Print the GrainIds Header before the rest of the headers
  dStream << DREAM3D::GrainData::GrainID;
  // Loop throught the list and print the rest of the headers, ignoring those we don't want
  for(QList<QString>::iterator iter = headers.begin(); iter != headers.end(); ++iter)
  {
    // Only get the array if the name does NOT match those listed
    IDataArray::Pointer p = m->getCellFieldData(*iter);
    if(p->getNameOfClass().compare(neighborlistPtr->getNameOfClass()) != 0)
    {
      if (p->GetNumberOfComponents() == 1)
      {
        dStream << space << (*iter);
      }
      else // There are more than a single component so we need to add multiple header values
      {
        for(int k = 0; k < p->GetNumberOfComponents(); ++k)
        {
          dStream << space << (*iter) << "_" << k;
        }
      }
      // Get the IDataArray from the DataContainer
      data.push_back(p);
    }
  }
  dStream << "\n";


  // Get the number of tuples in the arrays
  size_t numTuples = data[0]->getNumberOfTuples();

  float threshold = 0.0f;

  // Skip the first grain
  for(qint32 i = firstPrecipitateField; i < numTuples; ++i)
  {
    if (((float)i / numTuples) * 100.0f > threshold)
    {
      QString ss = QObject::tr("Writing Field Data - %1% Complete").arg(((float)i / numTuples) * 100);
      notifyStatusMessage(ss);
      threshold = threshold + 5.0f;
      if (threshold < ((float)i / numTuples) * 100.0f)
      {
        threshold = ((float)i / numTuples) * 100.0f;
      }
    }

    // Print the grain id
    dStream << i;
    // Print a row of data
    for(qint32 p = 0; p < data.size(); ++p)
    {
      dStream << space;
      data[p]->printTuple(dStream, i, space);
    }
    dStream << "\n";
  }
}
