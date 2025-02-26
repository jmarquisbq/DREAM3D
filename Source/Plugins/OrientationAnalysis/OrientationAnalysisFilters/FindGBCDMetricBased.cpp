/* ============================================================================
 * This filter has been created by Krzysztof Glowinski (kglowinski at ymail.com).
 * It includes an implementation of the algorithm described in:
 * K.Glowinski, A.Morawiec, "Analysis of experimental grain boundary distributions
 * based on boundary-space metrics", Metall. Mater. Trans. A 45, 3189-3194 (2014).
 * Besides the algorithm itself, many parts of the code come from
 * the sources of other filters, mainly "Find GBCD" and "Write GBCD Pole Figure (GMT5)".
 * Therefore, the below copyright notice applies.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

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
#include "FindGBCDMetricBased.h"

#include <QtCore/QDir>
#include <QtCore/QTextStream>

#include "SIMPLib/Common/Constants.h"
#include "SIMPLib/DataContainers/DataContainer.h"
#include "SIMPLib/DataContainers/DataContainerArray.h"
#include "SIMPLib/FilterParameters/AbstractFilterParametersReader.h"
#include "SIMPLib/FilterParameters/AxisAngleFilterParameter.h"
#include "SIMPLib/FilterParameters/BooleanFilterParameter.h"
#include "SIMPLib/FilterParameters/ChoiceFilterParameter.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/IntFilterParameter.h"
#include "SIMPLib/FilterParameters/OutputFileFilterParameter.h"
#include "SIMPLib/FilterParameters/SeparatorFilterParameter.h"
#include "SIMPLib/Geometry/TriangleGeom.h"
#include "SIMPLib/Math/SIMPLibMath.h"
#include "SIMPLib/Utilities/FileSystemPathHelper.h"

#include "EbsdLib/LaueOps/LaueOps.h"

#include "OrientationAnalysis/OrientationAnalysisConstants.h"
#include "OrientationAnalysis/OrientationAnalysisUtilities.h"
#include "OrientationAnalysis/OrientationAnalysisVersion.h"

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/partitioner.h>
#include "tbb/concurrent_vector.h"
#endif

using LaueOpsShPtrType = std::shared_ptr<LaueOps>;
using LaueOpsContainer = std::vector<LaueOpsShPtrType>;
using namespace OrientationUtilities;

const float FindGBCDMetricBased::k_ResolutionChoices[FindGBCDMetricBased::k_NumberResolutionChoices][2] = {{3.0f, 7.0f}, {5.0f, 5.0f}, {5.0f, 7.0f}, {5.0f, 8.0f},
                                                                                                           {6.0f, 7.0f}, {7.0f, 7.0f}, {8.0f, 8.0f}}; // { for misorient., for planes }

const double FindGBCDMetricBased::k_BallVolumesM3M[FindGBCDMetricBased::k_NumberResolutionChoices] = {0.0000641361, 0.000139158, 0.000287439, 0.00038019, 0.000484151, 0.000747069, 0.00145491};

namespace GBCDMetricBased
{

/**
 * @brief The TriAreaAndNormals class defines a container that stores the area of a given triangle
 * and the two normals for grains on either side of the triangle
 */
class TriAreaAndNormals
{
public:
  double area;
  float normal_grain1_x;
  float normal_grain1_y;
  float normal_grain1_z;
  float normal_grain2_x;
  float normal_grain2_y;
  float normal_grain2_z;

  TriAreaAndNormals(double __area, float n1x, float n1y, float n1z, float n2x, float n2y, float n2z)
  : area(__area)
  , normal_grain1_x(n1x)
  , normal_grain1_y(n1y)
  , normal_grain1_z(n1z)
  , normal_grain2_x(n2x)
  , normal_grain2_y(n2y)
  , normal_grain2_z(n2z)
  {
  }

  TriAreaAndNormals()
  {
    TriAreaAndNormals(0.0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
  }
};

/**
 * @brief The TrisSelector class implements a threaded algorithm that determines which triangles to
 * include in the GBCD calculation
 */
class TrisSelector
{
  bool m_ExcludeTripleLines;
  MeshIndexType* m_Triangles;
  int8_t* m_NodeTypes;

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
  tbb::concurrent_vector<TriAreaAndNormals>* selectedTris;
#else
  QVector<TriAreaAndNormals>* selectedTris;
#endif
  QVector<int8_t>* triIncluded;
  float m_misorResol;
  int32_t m_PhaseOfInterest;
  const Matrix3fR& gFixedT;

  LaueOpsContainer m_OrientationOps;
  uint32_t cryst;
  int32_t nsym;

  float* m_Eulers;
  int32_t* m_Phases;
  int32_t* m_FaceLabels;
  double* m_FaceNormals;
  double* m_FaceAreas;

public:
  TrisSelector(bool __m_ExcludeTripleLines, MeshIndexType* __m_Triangles, int8_t* __m_NodeTypes,

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
               tbb::concurrent_vector<TriAreaAndNormals>* __selectedTris,
#else
               QVector<TriAreaAndNormals>* __selectedTris,
#endif
               QVector<int8_t>* __triIncluded, float __m_misorResol, int32_t __m_PhaseOfInterest, const Matrix3fR& __gFixedT, uint32_t* __m_CrystalStructures, float* __m_Eulers, int32_t* __m_Phases,
               int32_t* __m_FaceLabels, double* __m_FaceNormals, double* __m_FaceAreas)
  : m_ExcludeTripleLines(__m_ExcludeTripleLines)
  , m_Triangles(__m_Triangles)
  , m_NodeTypes(__m_NodeTypes)
  , selectedTris(__selectedTris)
  , triIncluded(__triIncluded)
  , m_misorResol(__m_misorResol)
  , m_PhaseOfInterest(__m_PhaseOfInterest)
  , gFixedT(__gFixedT)
  , m_Eulers(__m_Eulers)
  , m_Phases(__m_Phases)
  , m_FaceLabels(__m_FaceLabels)
  , m_FaceNormals(__m_FaceNormals)
  , m_FaceAreas(__m_FaceAreas)
  {
    m_OrientationOps = LaueOps::GetAllOrientationOps();
    cryst = __m_CrystalStructures[__m_PhaseOfInterest];
    nsym = m_OrientationOps[cryst]->getNumSymOps();
  }

  virtual ~TrisSelector() = default;

  void select(size_t start, size_t end) const
  {
    Eigen::Vector3f g1ea = {0.0f, 0.0f, 0.0f};
    Eigen::Vector3f g2ea = {0.0f, 0.0f, 0.0f};

    Matrix3fR g1;
    g1.fill(0.0f);
    Matrix3fR g2;
    g2.fill(0.0f);

    Matrix3fR g1s;
    g1s.fill(0.0f);
    Matrix3fR g2s;
    g2s.fill(0.0f);

    Matrix3fR sym1;
    sym1.fill(0.0f);
    Matrix3fR sym2;
    sym2.fill(0.0f);

    Matrix3fR g2sT;
    g2sT.fill(0.0f);

    Matrix3fR dg;
    dg.fill(0.0f);
    Matrix3fR dgT;
    dgT.fill(0.0f);

    Matrix3fR diffFromFixed;
    diffFromFixed.fill(0.0f);

    Eigen::Vector3f normal_lab = {0.0f, 0.0f, 0.0f};
    Eigen::Vector3f normal_grain1 = {0.0f, 0.0f, 0.0f};
    Eigen::Vector3f normal_grain2 = {0.0f, 0.0f, 0.0f};

    for(size_t triIdx = start; triIdx < end; triIdx++)
    {
      int32_t feature1 = m_FaceLabels[2 * triIdx];
      int32_t feature2 = m_FaceLabels[2 * triIdx + 1];

      if(feature1 < 1 || feature2 < 1)
      {
        continue;
      }
      if(m_Phases[feature1] != m_Phases[feature2])
      {
        continue;
      }
      if(m_Phases[feature1] != m_PhaseOfInterest || m_Phases[feature2] != m_PhaseOfInterest)
      {
        continue;
      }

      if(m_ExcludeTripleLines)
      {
        int64_t node1 = m_Triangles[triIdx * 3];
        int64_t node2 = m_Triangles[triIdx * 3 + 1];
        int64_t node3 = m_Triangles[triIdx * 3 + 2];

        if(m_NodeTypes[node1] != 2 || m_NodeTypes[node2] != 2 || m_NodeTypes[node3] != 2)
        {
          continue;
        }
      }

      (*triIncluded)[triIdx] = 1;

      normal_lab[0] = static_cast<float>(m_FaceNormals[3 * triIdx]);
      normal_lab[1] = static_cast<float>(m_FaceNormals[3 * triIdx + 1]);
      normal_lab[2] = static_cast<float>(m_FaceNormals[3 * triIdx + 2]);

      for(int whichEa = 0; whichEa < 3; whichEa++)
      {
        g1ea[whichEa] = m_Eulers[3 * feature1 + whichEa];
        g2ea[whichEa] = m_Eulers[3 * feature2 + whichEa];
      }

        const OrientationF oMatrix1 = OrientationTransformation::eu2om<OrientationF, OrientationF>(OrientationF(g1ea[0], g1ea[1], g1ea[2]));
      g1 = OrientationMatrixToGMatrix(oMatrix1);
        const OrientationF oMatrix2 = OrientationTransformation::eu2om<OrientationF, OrientationF>(OrientationF(g2ea[0], g2ea[1], g2ea[2]));
      g2 = OrientationMatrixToGMatrix(oMatrix2);

      for(int j = 0; j < nsym; j++)
      {
        // rotate g1 by symOp
        sym1 = EbsdLibMatrixToEigenMatrix(m_OrientationOps[cryst]->getMatSymOpF(j));
        g1s = sym1 * g1;
        // get the crystal directions along the triangle normals
        normal_grain1 = g1s * normal_lab;

        for(int k = 0; k < nsym; k++)
        {
          // calculate the symmetric misorienation
          sym2 = EbsdLibMatrixToEigenMatrix(m_OrientationOps[cryst]->getMatSymOpF(k));
          // rotate g2 by symOp
          g2s = sym2 * g2;
          // transpose rotated g2
          g2sT = g2s.transpose();
          // calculate delta g
          dg = g1s * g2sT; // dg -- the mis orientation between adjacent grains
          dgT = dg.transpose();

          for(int transpose = 0; transpose <= 1; transpose++)
          {
            // check if dg is close to gFix
            if(transpose == 0)
            {
              diffFromFixed = dg * gFixedT;
            }
            else
            {
              diffFromFixed = dgT * gFixedT;
            }

            float diffAngle = acosf((diffFromFixed(0, 0) + diffFromFixed(1, 1) + diffFromFixed(2, 2) - 1.0f) * 0.5f);

            if(diffAngle < m_misorResol)
            {
              normal_grain2 = dgT * normal_grain1; // minus sign before normal_grain2 will be added later

              if(transpose == 0)
              {
                (*selectedTris).push_back(TriAreaAndNormals(m_FaceAreas[triIdx], normal_grain1[0], normal_grain1[1], normal_grain1[2], -normal_grain2[0], -normal_grain2[1], -normal_grain2[2]));
              }
              else
              {
                (*selectedTris).push_back(TriAreaAndNormals(m_FaceAreas[triIdx], -normal_grain2[0], -normal_grain2[1], -normal_grain2[2], normal_grain1[0], normal_grain1[1], normal_grain1[2]));
              }
            }
          }
        }
      }
    }
  }

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
  void operator()(const tbb::blocked_range<size_t>& r) const
  {
    select(r.begin(), r.end());
  }
#endif
};

/**
 * @brief The ProbeDistrib class implements a threaded algorithm that determines the distribution values
 * for the GBCD
 */
class ProbeDistrib
{
  std::vector<double>* distribValues = nullptr;
  std::vector<double>* errorValues = nullptr;
  std::vector<float> samplPtsX;
  std::vector<float> samplPtsY;
  std::vector<float> samplPtsZ;
#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
  tbb::concurrent_vector<TriAreaAndNormals> selectedTris;
#else
  QVector<TriAreaAndNormals> selectedTris;
#endif
  float planeResolSq;
  double totalFaceArea;
  int numDistinctGBs;
  double ballVolume;
  const Matrix3fR& gFixedT;

public:
  ProbeDistrib(std::vector<double>* __distribValues, std::vector<double>* __errorValues, std::vector<float> __samplPtsX, std::vector<float> __samplPtsY, std::vector<float> __samplPtsZ,
#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
               tbb::concurrent_vector<TriAreaAndNormals> __selectedTris,
#else
               QVector<TriAreaAndNormals> __selectedTris,
#endif
               float __planeResolSq, double __totalFaceArea, int __numDistinctGBs, double __ballVolume, const Matrix3fR& __gFixedT)
  : distribValues(__distribValues)
  , errorValues(__errorValues)
  , samplPtsX(__samplPtsX)
  , samplPtsY(__samplPtsY)
  , samplPtsZ(__samplPtsZ)
  , selectedTris(__selectedTris)
  , planeResolSq(__planeResolSq)
  , totalFaceArea(__totalFaceArea)
  , numDistinctGBs(__numDistinctGBs)
  , ballVolume(__ballVolume)
  , gFixedT(__gFixedT)
  {
  }

  virtual ~ProbeDistrib() = default;

  void probe(size_t start, size_t end) const
  {
    for(size_t ptIdx = start; ptIdx < end; ptIdx++)
    {
      Eigen::Vector3f fixedNormal1 = {samplPtsX.at(ptIdx), samplPtsY.at(ptIdx), samplPtsZ.at(ptIdx)};
      Eigen::Vector3f fixedNormal2 = {0.0f, 0.0f, 0.0f};
      fixedNormal2 = gFixedT * fixedNormal1;

      for(int triRepresIdx = 0; triRepresIdx < static_cast<int>(selectedTris.size()); triRepresIdx++)
      {
        for(int inversion = 0; inversion <= 1; inversion++)
        {
          float sign = 1.0f;
          if(inversion == 1)
          {
            sign = -1.0f;
          }

          float theta1 = acosf(sign * (selectedTris[triRepresIdx].normal_grain1_x * fixedNormal1[0] + selectedTris[triRepresIdx].normal_grain1_y * fixedNormal1[1] +
                                       selectedTris[triRepresIdx].normal_grain1_z * fixedNormal1[2]));

          float theta2 = acosf(-sign * (selectedTris[triRepresIdx].normal_grain2_x * fixedNormal2[0] + selectedTris[triRepresIdx].normal_grain2_y * fixedNormal2[1] +
                                        selectedTris[triRepresIdx].normal_grain2_z * fixedNormal2[2]));

          float distSq = 0.5f * (theta1 * theta1 + theta2 * theta2);

          if(distSq < planeResolSq)
          {
            (*distribValues)[ptIdx] += selectedTris[triRepresIdx].area;
          }
        }
      }

      (*errorValues)[ptIdx] = sqrt((*distribValues)[ptIdx] / totalFaceArea / double(numDistinctGBs)) / ballVolume;
      (*distribValues)[ptIdx] /= totalFaceArea;
      (*distribValues)[ptIdx] /= ballVolume;
    }
  }

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
  void operator()(const tbb::blocked_range<size_t>& r) const
  {
    probe(r.begin(), r.end());
  }
#endif
};

} // namespace GBCDMetricBased

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FindGBCDMetricBased::FindGBCDMetricBased()
{
  m_MisorientationRotation.angle = 17.9f;
  m_MisorientationRotation.h = 1.0f;
  m_MisorientationRotation.k = 1.0f;
  m_MisorientationRotation.l = 1.0f;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FindGBCDMetricBased::~FindGBCDMetricBased() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setupFilterParameters()
{
  FilterParameterVectorType parameters;
  parameters.push_back(SIMPL_NEW_INTEGER_FP("Phase of Interest", PhaseOfInterest, FilterParameter::Category::Parameter, FindGBCDMetricBased));
  parameters.push_back(SIMPL_NEW_AXISANGLE_FP("Fixed Misorientation", MisorientationRotation, FilterParameter::Category::Parameter, FindGBCDMetricBased));
  {
    ChoiceFilterParameter::Pointer parameter = ChoiceFilterParameter::New();
    parameter->setHumanLabel("Limiting Distances");
    parameter->setPropertyName("ChosenLimitDists");
    parameter->setSetterCallback(SIMPL_BIND_SETTER(FindGBCDMetricBased, this, ChosenLimitDists));
    parameter->setGetterCallback(SIMPL_BIND_GETTER(FindGBCDMetricBased, this, ChosenLimitDists));

    std::vector<QString> choices;

    for(int choiceIdx = 0; choiceIdx < FindGBCDMetricBased::k_NumberResolutionChoices; choiceIdx++)
    {
      QString misorResStr;
      QString planeResStr;
      QString degSymbol = QChar(0x00B0);

      misorResStr.setNum(FindGBCDMetricBased::k_ResolutionChoices[choiceIdx][0], 'f', 0);
      planeResStr.setNum(FindGBCDMetricBased::k_ResolutionChoices[choiceIdx][1], 'f', 0);

      choices.push_back(misorResStr + degSymbol + " for Misorientations; " + planeResStr + degSymbol + " for Plane Inclinations");
    }

    parameter->setChoices(choices);
    parameter->setCategory(FilterParameter::Category::Parameter);
    parameters.push_back(parameter);
  }
  parameters.push_back(SIMPL_NEW_INTEGER_FP("Number of Sampling Points (on a Hemisphere)", NumSamplPts, FilterParameter::Category::Parameter, FindGBCDMetricBased));
  parameters.push_back(SIMPL_NEW_BOOL_FP("Exclude Triangles Directly Neighboring Triple Lines", ExcludeTripleLines, FilterParameter::Category::Parameter, FindGBCDMetricBased));
  parameters.push_back(SIMPL_NEW_OUTPUT_FILE_FP("Output Distribution File", DistOutputFile, FilterParameter::Category::Parameter, FindGBCDMetricBased, "*.dat", "DAT File"));
  parameters.push_back(SIMPL_NEW_OUTPUT_FILE_FP("Output Distribution Errors File", ErrOutputFile, FilterParameter::Category::Parameter, FindGBCDMetricBased, "*.dat", "DAT File"));
  parameters.push_back(SIMPL_NEW_BOOL_FP("Save Relative Errors Instead of Their Absolute Values", SaveRelativeErr, FilterParameter::Category::Parameter, FindGBCDMetricBased));
  parameters.push_back(SeparatorFilterParameter::Create("Vertex Data", FilterParameter::Category::RequiredArray));
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Int8, 1, AttributeMatrix::Type::Vertex, IGeometry::Type::Triangle);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Node Types", NodeTypesArrayPath, FilterParameter::Category::RequiredArray, FindGBCDMetricBased, req));
  }
  parameters.push_back(SeparatorFilterParameter::Create("Face Data", FilterParameter::Category::RequiredArray));
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Int32, 2, AttributeMatrix::Type::Face, IGeometry::Type::Triangle);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Face Labels", SurfaceMeshFaceLabelsArrayPath, FilterParameter::Category::RequiredArray, FindGBCDMetricBased, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Double, 3, AttributeMatrix::Type::Face, IGeometry::Type::Triangle);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Face Normals", SurfaceMeshFaceNormalsArrayPath, FilterParameter::Category::RequiredArray, FindGBCDMetricBased, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Double, 1, AttributeMatrix::Type::Face, IGeometry::Type::Triangle);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Face Areas", SurfaceMeshFaceAreasArrayPath, FilterParameter::Category::RequiredArray, FindGBCDMetricBased, req));
  }
  parameters.push_back(SeparatorFilterParameter::Create("Face Feature Data", FilterParameter::Category::RequiredArray));
  {
    DataArraySelectionFilterParameter::RequirementType req =
        DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Int32, 2, AttributeMatrix::Type::FaceFeature, IGeometry::Type::Triangle);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Feature Face Labels", SurfaceMeshFeatureFaceLabelsArrayPath, FilterParameter::Category::RequiredArray, FindGBCDMetricBased, req));
  }
  parameters.push_back(SeparatorFilterParameter::Create("Cell Feature Data", FilterParameter::Category::RequiredArray));
  {
    DataArraySelectionFilterParameter::RequirementType req =
        DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Float, 3, AttributeMatrix::Type::CellFeature, IGeometry::Type::Image);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Average Euler Angles", FeatureEulerAnglesArrayPath, FilterParameter::Category::RequiredArray, FindGBCDMetricBased, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req =
        DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Int32, 1, AttributeMatrix::Type::CellFeature, IGeometry::Type::Image);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Phases", FeaturePhasesArrayPath, FilterParameter::Category::RequiredArray, FindGBCDMetricBased, req));
  }
  parameters.push_back(SeparatorFilterParameter::Create("Cell Ensemble Data", FilterParameter::Category::RequiredArray));
  {
    DataArraySelectionFilterParameter::RequirementType req =
        DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::UInt32, 1, AttributeMatrix::Type::CellEnsemble, IGeometry::Type::Image);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Crystal Structures", CrystalStructuresArrayPath, FilterParameter::Category::RequiredArray, FindGBCDMetricBased, req));
  }
  setFilterParameters(parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindGBCDMetricBased::readFilterParameters(AbstractFilterParametersReader* reader, int index)
{
  reader->openFilterGroup(this, index);
  setPhaseOfInterest(reader->readValue("PhaseOfInterest", getPhaseOfInterest()));
  setMisorientationRotation(reader->readAxisAngle("MisorientationRotation", getMisorientationRotation(), -1));
  setChosenLimitDists(reader->readValue("ChosenLimitDists", getChosenLimitDists()));
  setNumSamplPts(reader->readValue("NumSamplPts", getNumSamplPts()));
  setExcludeTripleLines(reader->readValue("ExcludeTripleLines", getExcludeTripleLines()));
  setDistOutputFile(reader->readString("DistOutputFile", getDistOutputFile()));
  setErrOutputFile(reader->readString("ErrOutputFile", getErrOutputFile()));
  setSaveRelativeErr(reader->readValue("SaveRelativeErr", getSaveRelativeErr()));
  setCrystalStructuresArrayPath(reader->readDataArrayPath("CrystalStructures", getCrystalStructuresArrayPath()));
  setFeatureEulerAnglesArrayPath(reader->readDataArrayPath("FeatureEulerAngles", getFeatureEulerAnglesArrayPath()));
  setFeaturePhasesArrayPath(reader->readDataArrayPath("FeaturePhases", getFeaturePhasesArrayPath()));
  setSurfaceMeshFaceLabelsArrayPath(reader->readDataArrayPath("SurfaceMeshFaceLabels", getSurfaceMeshFaceLabelsArrayPath()));
  setSurfaceMeshFaceNormalsArrayPath(reader->readDataArrayPath("SurfaceMeshFaceNormals", getSurfaceMeshFaceNormalsArrayPath()));
  setSurfaceMeshFeatureFaceLabelsArrayPath(reader->readDataArrayPath("SurfaceMeshFeatureFaceLabels", getSurfaceMeshFeatureFaceLabelsArrayPath()));
  setSurfaceMeshFaceAreasArrayPath(reader->readDataArrayPath("SurfaceMeshFaceAreas", getSurfaceMeshFaceAreasArrayPath()));
  setNodeTypesArrayPath(reader->readDataArrayPath("NodeTypes", getNodeTypesArrayPath()));
  reader->closeFilterGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindGBCDMetricBased::initialize()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindGBCDMetricBased::dataCheck()
{
  clearErrorCode();
  clearWarningCode();

  // Fixed Misorientation (filter params.)
  if(getMisorientationRotation().angle <= 0.0 || getMisorientationRotation().angle > 180.0)
  {
    QString degSymbol = QChar(0x00B0);
    QString ss = "The misorientation angle should be in the range (0, 180" + degSymbol + "]";
    setErrorCondition(-1000, ss);
  }

  if(getMisorientationRotation().h == 0.0f && getMisorientationRotation().k == 0.0f && getMisorientationRotation().l == 0.0f)
  {
    QString ss = QObject::tr("All three indices of the misorientation axis cannot be 0");
    setErrorCondition(-1001, ss);
  }

  // Number of Sampling Points (filter params.)
  if(getNumSamplPts() < 1)
  {
    QString ss = QObject::tr("The number of sampling points must be greater than zero");
    setErrorCondition(-1002, ss);
  }

  // Set some reasonable value, but allow user to use more if he/she knows what he/she does
  if(getNumSamplPts() > 5000)
  {
    QString ss = QObject::tr("The number of sampling points is greater than 5000, but it is unlikely that many are needed");
    setWarningCondition(-1003, ss);
  }

  // Output files (filter params.)
  FileSystemPathHelper::CheckOutputFile(this, "Output Distribution File", getDistOutputFile(), true);

  FileSystemPathHelper::CheckOutputFile(this, "Output Error File", getErrOutputFile(), true);

  QFileInfo distOutFileInfo(getDistOutputFile());
  QFileInfo errOutFileInfo(getErrOutputFile());
  if(distOutFileInfo.suffix().compare("") == 0)
  {
    setDistOutputFile(getDistOutputFile().append(".dat"));
  }

  if(errOutFileInfo.suffix().compare("") == 0)
  {
    setErrOutputFile(getErrOutputFile().append(".dat"));
  }

  // Make sure the file name ends with _1 so the GMT scripts work correctly
  QString distFName = distOutFileInfo.baseName();
  if(!distFName.isEmpty() && !distFName.endsWith("_1"))
  {
    distFName = distFName + "_1";
    QString absPath = distOutFileInfo.absolutePath() + "/" + distFName + ".dat";
    setDistOutputFile(absPath);
  }

  QString errFName = errOutFileInfo.baseName();
  if(!errFName.isEmpty() && !errFName.endsWith("_1"))
  {
    errFName = errFName + "_1";
    QString absPath = errOutFileInfo.absolutePath() + "/" + errFName + ".dat";
    setErrOutputFile(absPath);
  }

  if(!getDistOutputFile().isEmpty() && getDistOutputFile() == getErrOutputFile())
  {
    QString ss = QObject::tr("The output files must be different");
    setErrorCondition(-1008, ss);
  }

  // Crystal Structures
  std::vector<size_t> cDims(1, 1);
  m_CrystalStructuresPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<unsigned int>>(this, getCrystalStructuresArrayPath(), cDims);
  if(nullptr != m_CrystalStructuresPtr.lock())
  {
    m_CrystalStructures = m_CrystalStructuresPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */

  // Phase of Interest  (filter params.)
  if(nullptr != m_CrystalStructuresPtr.lock())
  {
    if(getPhaseOfInterest() >= static_cast<int>(m_CrystalStructuresPtr.lock()->getNumberOfTuples()) || getPhaseOfInterest() <= 0)
    {
      QString ss = QObject::tr("The phase index is either larger than the number of Ensembles or smaller than 1");
      setErrorCondition(-1009, ss);
    }
  }

  // Euler Angles
  cDims[0] = 3;
  m_FeatureEulerAnglesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<float>>(this, getFeatureEulerAnglesArrayPath(), cDims);
  if(nullptr != m_FeatureEulerAnglesPtr.lock())
  {
    m_FeatureEulerAngles = m_FeatureEulerAnglesPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */

  // Phases
  cDims[0] = 1;
  m_FeaturePhasesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<int32_t>>(this, getFeaturePhasesArrayPath(), cDims);
  if(nullptr != m_FeaturePhasesPtr.lock())
  {
    m_FeaturePhases = m_FeaturePhasesPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */

  // Face Labels
  cDims[0] = 2;
  m_SurfaceMeshFaceLabelsPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<int32_t>>(this, getSurfaceMeshFaceLabelsArrayPath(), cDims);
  if(nullptr != m_SurfaceMeshFaceLabelsPtr.lock())
  {
    m_SurfaceMeshFaceLabels = m_SurfaceMeshFaceLabelsPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */

  // Face Normals
  cDims[0] = 3;
  m_SurfaceMeshFaceNormalsPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<double>>(this, getSurfaceMeshFaceNormalsArrayPath(), cDims);
  if(nullptr != m_SurfaceMeshFaceNormalsPtr.lock())
  {
    m_SurfaceMeshFaceNormals = m_SurfaceMeshFaceNormalsPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */

  // Face Areas
  cDims[0] = 1;
  m_SurfaceMeshFaceAreasPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<double>>(this, getSurfaceMeshFaceAreasArrayPath(), cDims);
  if(nullptr != m_SurfaceMeshFaceAreasPtr.lock())
  {
    m_SurfaceMeshFaceAreas = m_SurfaceMeshFaceAreasPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */

  // Feature Face Labels
  cDims[0] = 2;
  m_SurfaceMeshFeatureFaceLabelsPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<int32_t>>(this, getSurfaceMeshFeatureFaceLabelsArrayPath(), cDims);
  if(nullptr != m_SurfaceMeshFeatureFaceLabelsPtr.lock())
  {
    m_SurfaceMeshFeatureFaceLabels = m_SurfaceMeshFeatureFaceLabelsPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */

  // Node Types
  cDims[0] = 1;
  m_NodeTypesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<int8_t>>(this, getNodeTypesArrayPath(), cDims);
  if(nullptr != m_NodeTypesPtr.lock())
  {
    m_NodeTypes = m_NodeTypesPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindGBCDMetricBased::execute()
{
  dataCheck();
  if(getErrorCode() < 0)
  {
    return;
  }

  // -------------------- set resolutions and 'ball volumes' based on user's selection -------------
  float m_misorResol = FindGBCDMetricBased::k_ResolutionChoices[getChosenLimitDists()][0];
  float m_planeResol = FindGBCDMetricBased::k_ResolutionChoices[getChosenLimitDists()][1];

  m_misorResol *= SIMPLib::Constants::k_PiOver180D;
  m_planeResol *= SIMPLib::Constants::k_PiOver180D;
  float m_PlaneResolSq = m_planeResol * m_planeResol;

  // We want to work with the raw pointers for speed so get those pointers.
  uint32_t* m_CrystalStructures = m_CrystalStructuresPtr.lock()->getPointer(0);
  float* m_Eulers = m_FeatureEulerAnglesPtr.lock()->getPointer(0);
  int32_t* m_Phases = m_FeaturePhasesPtr.lock()->getPointer(0);
  int32_t* m_FaceLabels = m_SurfaceMeshFaceLabelsPtr.lock()->getPointer(0);
  double* m_FaceNormals = m_SurfaceMeshFaceNormalsPtr.lock()->getPointer(0);
  double* m_FaceAreas = m_SurfaceMeshFaceAreasPtr.lock()->getPointer(0);
  int32_t* m_FeatureFaceLabels = m_SurfaceMeshFeatureFaceLabelsPtr.lock()->getPointer(0);
  int8_t* m_NodeTypes = m_NodeTypesPtr.lock()->getPointer(0);

  DataContainer::Pointer sm = getDataContainerArray()->getDataContainer(getSurfaceMeshFaceAreasArrayPath().getDataContainerName());
  TriangleGeom::Pointer triangleGeom = sm->getGeometryAs<TriangleGeom>();
  SharedTriList::Pointer m_TrianglesPtr = triangleGeom->getTriangles();
  MeshIndexType* m_Triangles = m_TrianglesPtr->getPointer(0);

  // -------------------- check if directiories are ok and if output files can be opened -----------

  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  QFileInfo distOutFileInfo(getDistOutputFile());
  QDir distOutFileDir(distOutFileInfo.path());
  if(!distOutFileDir.mkpath("."))
  {
    QString ss;
    ss = QObject::tr("Error creating parent path '%1'").arg(distOutFileDir.path());
    setErrorCondition(-1, ss);
    return;
  }

  QFile distOutFile(getDistOutputFile());
  if(!distOutFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QString ss = QObject::tr("Error opening output file '%1'").arg(getDistOutputFile());
    setErrorCondition(-100, ss);
    return;
  }

  QFileInfo errOutFileInfo(getDistOutputFile());

  QDir errOutFileDir(errOutFileInfo.path());
  if(!errOutFileDir.mkpath("."))
  {
    QString ss;
    ss = QObject::tr("Error creating parent path '%1'").arg(errOutFileDir.path());
    setErrorCondition(-1, ss);
    return;
  }

  QFile errOutFile(getDistOutputFile());
  if(!errOutFile.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QString ss = QObject::tr("Error opening output file '%1'").arg(getDistOutputFile());
    setErrorCondition(-100, ss);
    return;
  }

  // Open the output files, should be opened and checked before starting computations
  FILE* fDist = nullptr;
  fDist = fopen(m_DistOutputFile.toLatin1().data(), "wb");
  if(nullptr == fDist)
  {
    QString ss = QObject::tr("Error opening distribution output file '%1'").arg(m_DistOutputFile);
    setErrorCondition(-1, ss);
    return;
  }

  FILE* fErr = nullptr;
  fErr = fopen(m_ErrOutputFile.toLatin1().data(), "wb");
  if(nullptr == fErr)
  {
    QString ss = QObject::tr("Error opening distribution errors output file '%1'").arg(m_ErrOutputFile);
    setErrorCondition(-1, ss);
    return;
  }

  // ------------------- before computing the distribution, we must find normalization factors -----
  double ballVolume = FindGBCDMetricBased::k_BallVolumesM3M[getChosenLimitDists()];
  {
    std::vector<LaueOps::Pointer> m_OrientationOps = LaueOps::GetAllOrientationOps();
    int32_t cryst = m_CrystalStructures[m_PhaseOfInterest];
    int32_t nsym = m_OrientationOps[cryst]->getNumSymOps();

    if(cryst != 1)
    {
      double symFactor = double(nsym) / 24.0;
      symFactor *= symFactor;
      ballVolume *= symFactor;
    }
  }

  // ------------------------------ generation of sampling points ----------------------------------
  QString ss = QObject::tr("|| Generating sampling points");
  notifyStatusMessage(ss);

  // generate "Golden Section Spiral", see http://www.softimageblog.com/archives/115
  int numSamplPts_WholeSph = 2 * m_NumSamplPts; // here we generate points on the whole sphere
  std::vector<float> samplPtsX(0);
  std::vector<float> samplPtsY(0);
  std::vector<float> samplPtsZ(0);

  const float _inc = SIMPLib::Constants::k_PiD * (3.0 - std::sqrt(5.0));

  float _off = 2.0f / float(numSamplPts_WholeSph);

  for(int ptIdx_WholeSph = 0; ptIdx_WholeSph < numSamplPts_WholeSph; ptIdx_WholeSph++)
  {
    if(getCancel())
    {
      return;
    }

    float _y = (float(ptIdx_WholeSph) * _off) - 1.0f + (0.5f * _off);
    float _r = sqrtf(fmaxf(1.0f - _y * _y, 0.0f));
    float _phi = float(ptIdx_WholeSph) * _inc;

    float z = sinf(_phi) * _r;

    if(z > 0.0f)
    {
      samplPtsX.push_back(cosf(_phi) * _r);
      samplPtsY.push_back(_y);
      samplPtsZ.push_back(z);
    }
  }

  // Add points at the equator for better performance of some plotting tools
  for(double phi = 0.0; phi <= SIMPLib::Constants::k_2PiD; phi += m_planeResol)
  {
    samplPtsX.push_back(cosf(static_cast<float>(phi)));
    samplPtsY.push_back(sinf(static_cast<float>(phi)));
    samplPtsZ.push_back(0.0f);
  }

  // Convert axis angle to matrix representation of misorientation
  Matrix3fR gFixedT;
  gFixedT.fill(0.0f);
  {
    float gFixedAngle = static_cast<float>(m_MisorientationRotation.angle * SIMPLib::Constants::k_PiOver180D);
    Eigen::Vector3f gFixedAxis = {m_MisorientationRotation.h, m_MisorientationRotation.k, m_MisorientationRotation.l};
    gFixedAxis.normalize();
    auto oMatrix = OrientationTransformation::ax2om<OrientationF, OrientationF>(OrientationF(gFixedAxis[0], gFixedAxis[1], gFixedAxis[2], gFixedAngle));
    gFixedT = OrientationMatrixToGMatrixTranspose(oMatrix);
  }

  size_t numMeshTris = m_SurfaceMeshFaceAreasPtr.lock()->getNumberOfTuples();

// ---------  find triangles (and equivalent crystallographic parameters) with +- the fixed misorientation ---------
#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
  bool doParallel = true;
  tbb::concurrent_vector<GBCDMetricBased::TriAreaAndNormals> selectedTris(0);
#else
  QVector<GBCDMetricBased::TriAreaAndNormals> selectedTris(0);
#endif

  QVector<int8_t> triIncluded(numMeshTris, 0);

  size_t trisChunkSize = 50000;
  if(numMeshTris < trisChunkSize)
  {
    trisChunkSize = numMeshTris;
  }

  for(size_t i = 0; i < numMeshTris; i = i + trisChunkSize)
  {
    if(getCancel())
    {
      return;
    }
    ss = QObject::tr("|| Step 1/2: Selecting Triangles with the Specified Misorientation (%1% completed)").arg(int(100.0 * float(i) / float(numMeshTris)));
    notifyStatusMessage(ss);
    if(i + trisChunkSize >= numMeshTris)
    {
      trisChunkSize = numMeshTris - i;
    }

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
    if(doParallel)
    {
      tbb::parallel_for(tbb::blocked_range<size_t>(i, i + trisChunkSize),
                        GBCDMetricBased::TrisSelector(m_ExcludeTripleLines, m_Triangles, m_NodeTypes, &selectedTris, &triIncluded, m_misorResol, m_PhaseOfInterest, gFixedT, m_CrystalStructures,
                                                      m_Eulers, m_Phases, m_FaceLabels, m_FaceNormals, m_FaceAreas),
                        tbb::auto_partitioner());
    }
    else
#endif
    {
      GBCDMetricBased::TrisSelector serial(m_ExcludeTripleLines, m_Triangles, m_NodeTypes, &selectedTris, &triIncluded, m_misorResol, m_PhaseOfInterest, gFixedT, m_CrystalStructures, m_Eulers,
                                           m_Phases, m_FaceLabels, m_FaceNormals, m_FaceAreas);
      serial.select(i, i + trisChunkSize);
    }
  }

  // ------------------------  find the number of distinct boundaries ------------------------------
  int32_t numDistinctGBs = 0;
  int32_t numFaceFeatures = m_SurfaceMeshFeatureFaceLabelsPtr.lock()->getNumberOfTuples();

  for(int featureFaceIdx = 0; featureFaceIdx < numFaceFeatures; featureFaceIdx++)
  {
    int32_t feature1 = m_FeatureFaceLabels[2 * featureFaceIdx];
    int32_t feature2 = m_FeatureFaceLabels[2 * featureFaceIdx + 1];

    if(feature1 < 1 || feature2 < 1)
    {
      continue;
    }
    if(m_FeaturePhases[feature1] != m_FeaturePhases[feature2])
    {
      continue;
    }
    if(m_FeaturePhases[feature1] != m_PhaseOfInterest || m_FeaturePhases[feature2] != m_PhaseOfInterest)
    {
      continue;
    }

    numDistinctGBs++;
  }

  // ----------------- determining distribution values at the sampling points (and their errors) ---
  double totalFaceArea = 0.0;
  for(size_t triIdx = 0; triIdx < numMeshTris; triIdx++)
  {
    totalFaceArea += m_FaceAreas[triIdx] * double(triIncluded.at(triIdx));
  }

  std::vector<double> distribValues(samplPtsX.size(), 0.0);
  std::vector<double> errorValues(samplPtsX.size(), 0.0);

  int32_t pointsChunkSize = 100;
  if(samplPtsX.size() < pointsChunkSize)
  {
    pointsChunkSize = samplPtsX.size();
  }

  for(int32_t i = 0; i < samplPtsX.size(); i = i + pointsChunkSize)
  {
    if(getCancel())
    {
      return;
    }
    ss = QObject::tr("|| Step 2/2: Computing Distribution Values at the Section of Interest (%1% completed)").arg(int(100.0 * float(i) / float(samplPtsX.size())));
    notifyStatusMessage(ss);
    if(i + pointsChunkSize >= samplPtsX.size())
    {
      pointsChunkSize = samplPtsX.size() - i;
    }

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
    if(doParallel)
    {
      tbb::parallel_for(tbb::blocked_range<size_t>(i, i + pointsChunkSize),
                        GBCDMetricBased::ProbeDistrib(&distribValues, &errorValues, samplPtsX, samplPtsY, samplPtsZ, selectedTris, m_PlaneResolSq, totalFaceArea, numDistinctGBs, ballVolume, gFixedT),
                        tbb::auto_partitioner());
    }
    else
#endif
    {
      GBCDMetricBased::ProbeDistrib serial(&distribValues, &errorValues, samplPtsX, samplPtsY, samplPtsZ, selectedTris, m_PlaneResolSq, totalFaceArea, numDistinctGBs, ballVolume, gFixedT);
      serial.probe(i, i + pointsChunkSize);
    }
  }

  // ------------------------------------------- writing the output --------------------------------
  fprintf(fDist, "%.1f %.1f %.1f %.1f\n", m_MisorientationRotation.h, m_MisorientationRotation.k, m_MisorientationRotation.l, m_MisorientationRotation.angle);
  fprintf(fErr, "%.1f %.1f %.1f %.1f\n", m_MisorientationRotation.h, m_MisorientationRotation.k, m_MisorientationRotation.l, m_MisorientationRotation.angle);

  for(int ptIdx = 0; ptIdx < samplPtsX.size(); ptIdx++)
  {

    float zenith = acosf(samplPtsZ.at(ptIdx));
    float azimuth = atan2f(samplPtsY.at(ptIdx), samplPtsX.at(ptIdx));

    float zenithDeg = static_cast<float>(SIMPLib::Constants::k_180OverPiD * zenith);
    float azimuthDeg = static_cast<float>(SIMPLib::Constants::k_180OverPiD * azimuth);

    fprintf(fDist, "%.8E %.8E %.8E\n", azimuthDeg, 90.0f - zenithDeg, distribValues[ptIdx]);

    if(!m_SaveRelativeErr)
    {
      fprintf(fErr, "%.8E %.28f %.8E\n", azimuthDeg, 90.0f - zenithDeg, errorValues[ptIdx]);
    }
    else
    {
      double saneErr = 100.0;
      if(distribValues[ptIdx] > 1e-10)
      {
        saneErr = fmin(100.0, 100.0 * errorValues[ptIdx] / distribValues[ptIdx]);
      }
      fprintf(fErr, "%.8E %.8E %.2E\n", azimuthDeg, 90.0f - zenithDeg, saneErr);
    }
  }
  fclose(fDist);
  fclose(fErr);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AbstractFilter::Pointer FindGBCDMetricBased::newFilterInstance(bool copyFilterParameters) const
{
  FindGBCDMetricBased::Pointer filter = FindGBCDMetricBased::New();
  if(copyFilterParameters)
  {
    copyFilterParameterInstanceVariables(filter.get());
  }
  return filter;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindGBCDMetricBased::getCompiledLibraryName() const
{
  return OrientationAnalysisConstants::OrientationAnalysisBaseName;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindGBCDMetricBased::getBrandingString() const
{
  return "OrientationAnalysis";
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindGBCDMetricBased::getFilterVersion() const
{
  QString version;
  QTextStream vStream(&version);
  vStream << OrientationAnalysis::Version::Major() << "." << OrientationAnalysis::Version::Minor() << "." << OrientationAnalysis::Version::Patch();
  return version;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindGBCDMetricBased::getGroupName() const
{
  return SIMPL::FilterGroups::StatisticsFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QUuid FindGBCDMetricBased::getUuid() const
{
  return QUuid("{d67e9f28-2fe5-5188-b0f8-323a7e603de6}");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindGBCDMetricBased::getSubGroupName() const
{
  return SIMPL::FilterSubGroups::CrystallographyFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindGBCDMetricBased::getHumanLabel() const
{
  return "Find GBCD (Metric-Based Approach)";
}

// -----------------------------------------------------------------------------
FindGBCDMetricBased::Pointer FindGBCDMetricBased::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
std::shared_ptr<FindGBCDMetricBased> FindGBCDMetricBased::New()
{
  struct make_shared_enabler : public FindGBCDMetricBased
  {
  };
  std::shared_ptr<make_shared_enabler> val = std::make_shared<make_shared_enabler>();
  val->setupFilterParameters();
  return val;
}

// -----------------------------------------------------------------------------
QString FindGBCDMetricBased::getNameOfClass() const
{
  return QString("FindGBCDMetricBased");
}

// -----------------------------------------------------------------------------
QString FindGBCDMetricBased::ClassName()
{
  return QString("FindGBCDMetricBased");
}

// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setPhaseOfInterest(int value)
{
  m_PhaseOfInterest = value;
}

// -----------------------------------------------------------------------------
int FindGBCDMetricBased::getPhaseOfInterest() const
{
  return m_PhaseOfInterest;
}

// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setMisorientationRotation(const AxisAngleInput& value)
{
  m_MisorientationRotation = value;
}

// -----------------------------------------------------------------------------
AxisAngleInput FindGBCDMetricBased::getMisorientationRotation() const
{
  return m_MisorientationRotation;
}

// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setChosenLimitDists(int value)
{
  m_ChosenLimitDists = value;
}

// -----------------------------------------------------------------------------
int FindGBCDMetricBased::getChosenLimitDists() const
{
  return m_ChosenLimitDists;
}

// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setNumSamplPts(int value)
{
  m_NumSamplPts = value;
}

// -----------------------------------------------------------------------------
int FindGBCDMetricBased::getNumSamplPts() const
{
  return m_NumSamplPts;
}

// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setExcludeTripleLines(bool value)
{
  m_ExcludeTripleLines = value;
}

// -----------------------------------------------------------------------------
bool FindGBCDMetricBased::getExcludeTripleLines() const
{
  return m_ExcludeTripleLines;
}

// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setDistOutputFile(const QString& value)
{
  m_DistOutputFile = value;
}

// -----------------------------------------------------------------------------
QString FindGBCDMetricBased::getDistOutputFile() const
{
  return m_DistOutputFile;
}

// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setErrOutputFile(const QString& value)
{
  m_ErrOutputFile = value;
}

// -----------------------------------------------------------------------------
QString FindGBCDMetricBased::getErrOutputFile() const
{
  return m_ErrOutputFile;
}

// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setSaveRelativeErr(bool value)
{
  m_SaveRelativeErr = value;
}

// -----------------------------------------------------------------------------
bool FindGBCDMetricBased::getSaveRelativeErr() const
{
  return m_SaveRelativeErr;
}

// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setCrystalStructuresArrayPath(const DataArrayPath& value)
{
  m_CrystalStructuresArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindGBCDMetricBased::getCrystalStructuresArrayPath() const
{
  return m_CrystalStructuresArrayPath;
}

// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setFeatureEulerAnglesArrayPath(const DataArrayPath& value)
{
  m_FeatureEulerAnglesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindGBCDMetricBased::getFeatureEulerAnglesArrayPath() const
{
  return m_FeatureEulerAnglesArrayPath;
}

// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setFeaturePhasesArrayPath(const DataArrayPath& value)
{
  m_FeaturePhasesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindGBCDMetricBased::getFeaturePhasesArrayPath() const
{
  return m_FeaturePhasesArrayPath;
}

// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setSurfaceMeshFaceLabelsArrayPath(const DataArrayPath& value)
{
  m_SurfaceMeshFaceLabelsArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindGBCDMetricBased::getSurfaceMeshFaceLabelsArrayPath() const
{
  return m_SurfaceMeshFaceLabelsArrayPath;
}

// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setSurfaceMeshFaceNormalsArrayPath(const DataArrayPath& value)
{
  m_SurfaceMeshFaceNormalsArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindGBCDMetricBased::getSurfaceMeshFaceNormalsArrayPath() const
{
  return m_SurfaceMeshFaceNormalsArrayPath;
}

// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setSurfaceMeshFaceAreasArrayPath(const DataArrayPath& value)
{
  m_SurfaceMeshFaceAreasArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindGBCDMetricBased::getSurfaceMeshFaceAreasArrayPath() const
{
  return m_SurfaceMeshFaceAreasArrayPath;
}

// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setSurfaceMeshFeatureFaceLabelsArrayPath(const DataArrayPath& value)
{
  m_SurfaceMeshFeatureFaceLabelsArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindGBCDMetricBased::getSurfaceMeshFeatureFaceLabelsArrayPath() const
{
  return m_SurfaceMeshFeatureFaceLabelsArrayPath;
}

// -----------------------------------------------------------------------------
void FindGBCDMetricBased::setNodeTypesArrayPath(const DataArrayPath& value)
{
  m_NodeTypesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindGBCDMetricBased::getNodeTypesArrayPath() const
{
  return m_NodeTypesArrayPath;
}
