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

#include "FindTwinBoundarySchmidFactors.h"

#include <fstream>

#include <QtCore/QTextStream>

#include "SIMPLib/Common/Constants.h"
#include "SIMPLib/DataContainers/DataContainerArray.h"
#include "SIMPLib/FilterParameters/AbstractFilterParametersReader.h"
#include "SIMPLib/FilterParameters/DataArraySelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/FloatVec3FilterParameter.h"
#include "SIMPLib/FilterParameters/LinkedBooleanFilterParameter.h"
#include "SIMPLib/FilterParameters/LinkedPathCreationFilterParameter.h"
#include "SIMPLib/FilterParameters/OutputFileFilterParameter.h"
#include "SIMPLib/FilterParameters/SeparatorFilterParameter.h"
#include "SIMPLib/FilterParameters/StringFilterParameter.h"
#include "SIMPLib/Geometry/ImageGeom.h"
#include "SIMPLib/Geometry/TriangleGeom.h"
#include "SIMPLib/Math/GeometryMath.h"
#include "SIMPLib/Math/MatrixMath.h"
#include "SIMPLib/Math/SIMPLibMath.h"
#include "SIMPLib/Utilities/FileSystemPathHelper.h"

#include "EbsdLib/Core/Quaternion.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

#include "OrientationAnalysis/OrientationAnalysisConstants.h"
#include "OrientationAnalysis/OrientationAnalysisVersion.h"

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/partitioner.h>
#endif

/**
 * @brief The CalculateTwinBoundarySchmidFactorsImpl class implements a threaded algorithm that computes the
 * Schmid factors across twin boundaries.
 */
class CalculateTwinBoundarySchmidFactorsImpl
{
  int32_t* m_Labels;
  double* m_Normals;
  float* m_Quats;
  bool* m_TwinBoundary;
  float* m_TwinBoundarySchmidFactors;
  float* m_LoadDir;
  LaueOpsContainer m_OrientationOps;

public:
  CalculateTwinBoundarySchmidFactorsImpl(float* LoadingDir, int32_t* Labels, double* Normals, float* Quats, bool* TwinBoundary, float* TwinBoundarySchmidFactors)
  : m_Labels(Labels)
  , m_Normals(Normals)
  , m_Quats(Quats)
  , m_TwinBoundary(TwinBoundary)
  , m_TwinBoundarySchmidFactors(TwinBoundarySchmidFactors)
  , m_LoadDir(LoadingDir)
  {
    m_OrientationOps = LaueOps::GetAllOrientationOps();
  }
  virtual ~CalculateTwinBoundarySchmidFactorsImpl() = default;

  void generate(size_t start, size_t end) const
  {
    int32_t feature1 = 0, feature2 = 0, feature = 0;
    float normal[3] = {0.0f, 0.0f, 0.0f};
    float g1[3][3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};
    float schmid1 = 0.0f, schmid2 = 0.0f, schmid3 = 0.0f;
    //    QuatF q1 = QuaternionMathF::New();
    //    QuatF* quats = reinterpret_cast<QuatF*>(m_Quats);

    float n[3] = {0.0f, 0.0f, 0.0f};
    float b[3] = {0.0f, 0.0f, 0.0f};
    float crystalLoading[3] = {0.0f, 0.0f, 0.0f};
    float cosPhi = 0.0f, cosLambda = 0.0f;
    float* currentQuatPtr = nullptr;

    for(size_t i = start; i < end; i++)
    {
      if(m_TwinBoundary[i])
      {
        feature1 = m_Labels[2 * i];
        feature2 = m_Labels[2 * i + 1];
        normal[0] = m_Normals[3 * i];
        normal[1] = m_Normals[3 * i + 1];
        normal[2] = m_Normals[3 * i + 2];
        schmid1 = 0.0f, schmid2 = 0.0f, schmid3 = 0.0f;
        if(feature1 > feature2)
        {
          feature = feature1;
        }
        else
        {
          feature = feature2;
        }
        currentQuatPtr = m_Quats + feature * 4;

        // calculate crystal direction parallel to normal
        OrientationTransformation::qu2om<QuatF, OrientationF>({currentQuatPtr[0], currentQuatPtr[1], currentQuatPtr[2], currentQuatPtr[3]}).toGMatrix(g1);
        MatrixMath::Multiply3x3with3x1(g1, normal, n);
        // calculate crystal direction parallel to loading direction
        MatrixMath::Multiply3x3with3x1(g1, m_LoadDir, crystalLoading);

        if(n[2] < 0.0f)
        {
          n[0] = -n[0], n[1] = -n[1], n[2] = -n[2];
        }
        if(n[0] > 0.0f && n[1] > 0.0f)
        {
          n[0] = 1.0f, n[1] = 1.0f, n[2] = 1.0f;
          cosPhi = fabsf(GeometryMath::CosThetaBetweenVectors(crystalLoading, n));
          b[0] = 1.0f, b[1] = -1.0f, b[2] = 0.0f;
          cosLambda = fabsf(GeometryMath::CosThetaBetweenVectors(crystalLoading, b));
          schmid1 = cosPhi * cosLambda;
          b[0] = -1.0f, b[1] = 0.0f, b[2] = 1.0f;
          cosLambda = fabsf(GeometryMath::CosThetaBetweenVectors(crystalLoading, b));
          schmid2 = cosPhi * cosLambda;
          b[0] = 0.0f, b[1] = -1.0f, b[2] = 1.0f;
          cosLambda = fabsf(GeometryMath::CosThetaBetweenVectors(crystalLoading, b));
          schmid3 = cosPhi * cosLambda;
          m_TwinBoundarySchmidFactors[3 * i] = schmid1;
          m_TwinBoundarySchmidFactors[3 * i + 1] = schmid2;
          m_TwinBoundarySchmidFactors[3 * i + 2] = schmid3;
        }
        else if(n[0] > 0.0f && n[1] < 0.0f)
        {
          n[0] = 1.0f, n[1] = -1.0f, n[2] = 1.0f;
          cosPhi = fabsf(GeometryMath::CosThetaBetweenVectors(crystalLoading, n));
          b[0] = 1.0f, b[1] = 1.0f, b[2] = 0.0f;
          cosLambda = fabsf(GeometryMath::CosThetaBetweenVectors(crystalLoading, b));
          schmid1 = cosPhi * cosLambda;
          b[0] = 0.0f, b[1] = 1.0f, b[2] = 1.0f;
          cosLambda = fabsf(GeometryMath::CosThetaBetweenVectors(crystalLoading, b));
          schmid2 = cosPhi * cosLambda;
          b[0] = -1.0f, b[1] = 0.0f, b[2] = 1.0f;
          cosLambda = fabsf(GeometryMath::CosThetaBetweenVectors(crystalLoading, b));
          schmid3 = cosPhi * cosLambda;
          m_TwinBoundarySchmidFactors[3 * i] = schmid1;
          m_TwinBoundarySchmidFactors[3 * i + 1] = schmid2;
          m_TwinBoundarySchmidFactors[3 * i + 2] = schmid3;
        }
        else if(n[0] < 0.0f && n[1] > 0.0f)
        {
          n[0] = -1.0f, n[1] = 1.0f, n[2] = 1.0f;
          cosPhi = fabsf(GeometryMath::CosThetaBetweenVectors(crystalLoading, n));
          b[0] = 1.0f, b[1] = 1.0f, b[2] = 0.0f;
          cosLambda = fabsf(GeometryMath::CosThetaBetweenVectors(crystalLoading, b));
          schmid1 = cosPhi * cosLambda;
          b[0] = 1.0f, b[1] = 0.0f, b[2] = 1.0f;
          cosLambda = fabsf(GeometryMath::CosThetaBetweenVectors(crystalLoading, b));
          schmid2 = cosPhi * cosLambda;
          b[0] = 0.0f, b[1] = -1.0f, b[2] = 1.0f;
          cosLambda = fabsf(GeometryMath::CosThetaBetweenVectors(crystalLoading, b));
          schmid3 = cosPhi * cosLambda;
          m_TwinBoundarySchmidFactors[3 * i] = schmid1;
          m_TwinBoundarySchmidFactors[3 * i + 1] = schmid2;
          m_TwinBoundarySchmidFactors[3 * i + 2] = schmid3;
        }
        else if(n[0] < 0.0f && n[1] < 0.0f)
        {
          n[0] = -1.0f, n[1] = -1.0f, n[2] = 1.0f;
          cosPhi = fabsf(GeometryMath::CosThetaBetweenVectors(crystalLoading, n));
          b[0] = 1.0f, b[1] = 0.0f, b[2] = 1.0f;
          cosLambda = fabsf(GeometryMath::CosThetaBetweenVectors(crystalLoading, b));
          schmid1 = cosPhi * cosLambda;
          b[0] = 0.0f, b[1] = 1.0f, b[2] = 1.0f;
          cosLambda = fabsf(GeometryMath::CosThetaBetweenVectors(crystalLoading, b));
          schmid2 = cosPhi * cosLambda;
          b[0] = 1.0f, b[1] = -1.0f, b[2] = 0.0f;
          cosLambda = fabsf(GeometryMath::CosThetaBetweenVectors(crystalLoading, b));
          schmid3 = cosPhi * cosLambda;
          m_TwinBoundarySchmidFactors[3 * i] = schmid1;
          m_TwinBoundarySchmidFactors[3 * i + 1] = schmid2;
          m_TwinBoundarySchmidFactors[3 * i + 2] = schmid3;
        }
      }
      else
      {
        m_TwinBoundarySchmidFactors[3 * i] = 0.0f;
        m_TwinBoundarySchmidFactors[3 * i + 1] = 0.0f;
        m_TwinBoundarySchmidFactors[3 * i + 2] = 0.0f;
      }
    }
  }

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
  void operator()(const tbb::blocked_range<size_t>& r) const
  {
    generate(r.begin(), r.end());
  }
#endif
};

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FindTwinBoundarySchmidFactors::FindTwinBoundarySchmidFactors()
{
  m_LoadingDir[0] = 1.0f;
  m_LoadingDir[1] = 1.0f;
  m_LoadingDir[2] = 1.0f;

  m_OrientationOps = LaueOps::GetAllOrientationOps();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
FindTwinBoundarySchmidFactors::~FindTwinBoundarySchmidFactors() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindTwinBoundarySchmidFactors::setupFilterParameters()
{
  FilterParameterVectorType parameters;
  parameters.push_back(SIMPL_NEW_FLOAT_VEC3_FP("Loading Direction", LoadingDir, FilterParameter::Category::Parameter, FindTwinBoundarySchmidFactors));

  std::vector<QString> linkedProps = {"TwinBoundarySchmidFactorsFile"};
  parameters.push_back(SIMPL_NEW_LINKED_BOOL_FP("Write Twin Boundary Info File", WriteFile, FilterParameter::Category::Parameter, FindTwinBoundarySchmidFactors, linkedProps));
  parameters.push_back(SIMPL_NEW_OUTPUT_FILE_FP("Twin Boundary Info File", TwinBoundarySchmidFactorsFile, FilterParameter::Category::Parameter, FindTwinBoundarySchmidFactors));
  parameters.push_back(SeparatorFilterParameter::Create("Cell Feature Data", FilterParameter::Category::RequiredArray));
  {
    DataArraySelectionFilterParameter::RequirementType req =
        DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Float, 4, AttributeMatrix::Type::CellFeature, IGeometry::Type::Image);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Average Quaternions", AvgQuatsArrayPath, FilterParameter::Category::RequiredArray, FindTwinBoundarySchmidFactors, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req =
        DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Int32, 1, AttributeMatrix::Type::CellFeature, IGeometry::Type::Image);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Phases", FeaturePhasesArrayPath, FilterParameter::Category::RequiredArray, FindTwinBoundarySchmidFactors, req));
  }
  parameters.push_back(SeparatorFilterParameter::Create("Cell Ensemble Data", FilterParameter::Category::RequiredArray));
  {
    DataArraySelectionFilterParameter::RequirementType req =
        DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::UInt32, 1, AttributeMatrix::Type::CellEnsemble, IGeometry::Type::Image);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Crystal Structures", CrystalStructuresArrayPath, FilterParameter::Category::RequiredArray, FindTwinBoundarySchmidFactors, req));
  }
  parameters.push_back(SeparatorFilterParameter::Create("Face Data", FilterParameter::Category::RequiredArray));
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Int32, 2, AttributeMatrix::Type::Face, IGeometry::Type::Triangle);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Face Labels", SurfaceMeshFaceLabelsArrayPath, FilterParameter::Category::RequiredArray, FindTwinBoundarySchmidFactors, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Double, 3, AttributeMatrix::Type::Face, IGeometry::Type::Triangle);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Face Normals", SurfaceMeshFaceNormalsArrayPath, FilterParameter::Category::RequiredArray, FindTwinBoundarySchmidFactors, req));
  }
  {
    DataArraySelectionFilterParameter::RequirementType req = DataArraySelectionFilterParameter::CreateRequirement(SIMPL::TypeNames::Bool, 1, AttributeMatrix::Type::Face, IGeometry::Type::Triangle);
    parameters.push_back(SIMPL_NEW_DA_SELECTION_FP("Twin Boundary", SurfaceMeshTwinBoundaryArrayPath, FilterParameter::Category::RequiredArray, FindTwinBoundarySchmidFactors, req));
  }
  parameters.push_back(SeparatorFilterParameter::Create("Face Data", FilterParameter::Category::CreatedArray));
  parameters.push_back(SIMPL_NEW_DA_WITH_LINKED_AM_FP("Twin Boundary Schmid Factors", SurfaceMeshTwinBoundarySchmidFactorsArrayName, SurfaceMeshFaceLabelsArrayPath, SurfaceMeshFaceLabelsArrayPath,
                                                      FilterParameter::Category::CreatedArray, FindTwinBoundarySchmidFactors));
  setFilterParameters(parameters);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindTwinBoundarySchmidFactors::readFilterParameters(AbstractFilterParametersReader* reader, int index)
{
  reader->openFilterGroup(this, index);
  setWriteFile(reader->readValue("WriteFile", getWriteFile()));
  setSurfaceMeshTwinBoundarySchmidFactorsArrayName(reader->readString("SurfaceMeshTwinBoundarySchmidFactorsArrayName", getSurfaceMeshTwinBoundarySchmidFactorsArrayName()));
  setSurfaceMeshTwinBoundaryArrayPath(reader->readDataArrayPath("SurfaceMeshTwinBoundaryArrayPath", getSurfaceMeshTwinBoundaryArrayPath()));
  setSurfaceMeshFaceNormalsArrayPath(reader->readDataArrayPath("SurfaceMeshFaceNormalsArrayPath", getSurfaceMeshFaceNormalsArrayPath()));
  setSurfaceMeshFaceLabelsArrayPath(reader->readDataArrayPath("SurfaceMeshFaceLabelsArrayPath", getSurfaceMeshFaceLabelsArrayPath()));
  setCrystalStructuresArrayPath(reader->readDataArrayPath("CrystalStructuresArrayPath", getCrystalStructuresArrayPath()));
  setFeaturePhasesArrayPath(reader->readDataArrayPath("FeaturePhasesArrayPath", getFeaturePhasesArrayPath()));
  setAvgQuatsArrayPath(reader->readDataArrayPath("AvgQuatsArrayPath", getAvgQuatsArrayPath()));
  setLoadingDir(reader->readFloatVec3("LoadingDir", getLoadingDir()));
  setTwinBoundarySchmidFactorsFile(reader->readString("TwinBoundarySchmidFactorsFile", getTwinBoundarySchmidFactorsFile()));
  reader->closeFilterGroup();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindTwinBoundarySchmidFactors::dataCheckVoxel()
{
  clearErrorCode();
  clearWarningCode();

  QVector<DataArrayPath> dataArrayPaths;

  getDataContainerArray()->getPrereqGeometryFromDataContainer<ImageGeom>(this, getAvgQuatsArrayPath().getDataContainerName());

  std::vector<size_t> cDims(1, 4);
  m_AvgQuatsPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<float>>(this, getAvgQuatsArrayPath(), cDims);
  if(nullptr != m_AvgQuatsPtr.lock())
  {
    m_AvgQuats = m_AvgQuatsPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */
  if(getErrorCode() >= 0)
  {
    dataArrayPaths.push_back(getAvgQuatsArrayPath());
  }

  cDims[0] = 1;
  m_FeaturePhasesPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<int32_t>>(this, getFeaturePhasesArrayPath(), cDims);
  if(nullptr != m_FeaturePhasesPtr.lock())
  {
    m_FeaturePhases = m_FeaturePhasesPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */
  if(getErrorCode() >= 0)
  {
    dataArrayPaths.push_back(getFeaturePhasesArrayPath());
  }

  m_CrystalStructuresPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<uint32_t>>(this, getCrystalStructuresArrayPath(), cDims);
  if(nullptr != m_CrystalStructuresPtr.lock())
  {
    m_CrystalStructures = m_CrystalStructuresPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */

  getDataContainerArray()->validateNumberOfTuples(this, dataArrayPaths);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindTwinBoundarySchmidFactors::dataCheckSurfaceMesh()
{
  clearErrorCode();
  clearWarningCode();
  DataArrayPath tempPath;

  if(m_WriteFile)
  {
    FileSystemPathHelper::CheckOutputFile(this, "Output File Path", getTwinBoundarySchmidFactorsFile(), true);
  }
  QVector<DataArrayPath> dataArrayPaths;

  getDataContainerArray()->getPrereqGeometryFromDataContainer<TriangleGeom>(this, getSurfaceMeshFaceLabelsArrayPath().getDataContainerName());

  std::vector<size_t> cDims(1, 2);
  m_SurfaceMeshFaceLabelsPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<int32_t>>(this, getSurfaceMeshFaceLabelsArrayPath(), cDims);
  if(nullptr != m_SurfaceMeshFaceLabelsPtr.lock())
  {
    m_SurfaceMeshFaceLabels = m_SurfaceMeshFaceLabelsPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */
  if(getErrorCode() >= 0)
  {
    dataArrayPaths.push_back(getSurfaceMeshFaceLabelsArrayPath());
  }

  cDims[0] = 3;
  m_SurfaceMeshFaceNormalsPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<double>>(this, getSurfaceMeshFaceNormalsArrayPath(), cDims);
  if(nullptr != m_SurfaceMeshFaceNormalsPtr.lock())
  {
    m_SurfaceMeshFaceNormals = m_SurfaceMeshFaceNormalsPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */
  if(getErrorCode() >= 0)
  {
    dataArrayPaths.push_back(getSurfaceMeshFaceNormalsArrayPath());
  }

  tempPath.update(getSurfaceMeshFaceLabelsArrayPath().getDataContainerName(), getSurfaceMeshFaceLabelsArrayPath().getAttributeMatrixName(), getSurfaceMeshTwinBoundarySchmidFactorsArrayName());
  m_SurfaceMeshTwinBoundarySchmidFactorsPtr = getDataContainerArray()->createNonPrereqArrayFromPath<DataArray<float>>(this, tempPath, 0, cDims);
  if(nullptr != m_SurfaceMeshTwinBoundarySchmidFactorsPtr.lock())
  {
    m_SurfaceMeshTwinBoundarySchmidFactors = m_SurfaceMeshTwinBoundarySchmidFactorsPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */

  cDims[0] = 1;
  m_SurfaceMeshTwinBoundaryPtr = getDataContainerArray()->getPrereqArrayFromPath<DataArray<bool>>(this, getSurfaceMeshTwinBoundaryArrayPath(), cDims);
  if(nullptr != m_SurfaceMeshTwinBoundaryPtr.lock())
  {
    m_SurfaceMeshTwinBoundary = m_SurfaceMeshTwinBoundaryPtr.lock()->getPointer(0);
  } /* Now assign the raw pointer to data from the DataArray<T> object */
  if(getErrorCode() >= 0)
  {
    dataArrayPaths.push_back(getSurfaceMeshTwinBoundaryArrayPath());
  }

  getDataContainerArray()->validateNumberOfTuples(this, dataArrayPaths);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindTwinBoundarySchmidFactors::dataCheck()
{
  clearErrorCode();
  clearWarningCode();

  dataCheckVoxel();
  dataCheckSurfaceMesh();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void FindTwinBoundarySchmidFactors::execute()
{
  clearErrorCode();
  clearWarningCode();
  dataCheckVoxel();
  if(getErrorCode() < 0)
  {
    return;
  }
  dataCheckSurfaceMesh();
  if(getErrorCode() < 0)
  {
    return;
  }

  size_t numTriangles = m_SurfaceMeshFaceLabelsPtr.lock()->getNumberOfTuples();

  float LoadingDir[3] = {0.0f, 0.0f, 0.0f};
  LoadingDir[0] = m_LoadingDir[0];
  LoadingDir[1] = m_LoadingDir[1];
  LoadingDir[2] = m_LoadingDir[2];

#ifdef SIMPL_USE_PARALLEL_ALGORITHMS
  if(true)
  {
    tbb::parallel_for(
        tbb::blocked_range<size_t>(0, numTriangles),
        CalculateTwinBoundarySchmidFactorsImpl(LoadingDir, m_SurfaceMeshFaceLabels, m_SurfaceMeshFaceNormals, m_AvgQuats, m_SurfaceMeshTwinBoundary, m_SurfaceMeshTwinBoundarySchmidFactors),
        tbb::auto_partitioner());
  }
  else
#endif
  {
    CalculateTwinBoundarySchmidFactorsImpl serial(LoadingDir, m_SurfaceMeshFaceLabels, m_SurfaceMeshFaceNormals, m_AvgQuats, m_SurfaceMeshTwinBoundary, m_SurfaceMeshTwinBoundarySchmidFactors);
    serial.generate(0, numTriangles);
  }

  if(m_WriteFile)
  {
    std::ofstream outFile;
    outFile.open(m_TwinBoundarySchmidFactorsFile.toLatin1().data(), std::ios_base::binary);

    outFile << "Feature1	Feature2	IsTwin	Plane	Schmid1	Schmid2	Schmid3"
            << "\n";
    for(size_t i = 0; i < numTriangles; i++)
    {
      outFile << m_SurfaceMeshFaceLabels[2 * i] << "  " << m_SurfaceMeshFaceLabels[2 * i + 1] << "  " << m_SurfaceMeshTwinBoundary[i] << "  " << m_SurfaceMeshTwinBoundarySchmidFactors[3 * i] << "  "
              << m_SurfaceMeshTwinBoundarySchmidFactors[3 * i + 1] << "  " << m_SurfaceMeshTwinBoundarySchmidFactors[3 * i + 2] << "\n";
    }
    outFile.close();
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AbstractFilter::Pointer FindTwinBoundarySchmidFactors::newFilterInstance(bool copyFilterParameters) const
{
  FindTwinBoundarySchmidFactors::Pointer filter = FindTwinBoundarySchmidFactors::New();
  if(copyFilterParameters)
  {
    copyFilterParameterInstanceVariables(filter.get());
  }
  return filter;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindTwinBoundarySchmidFactors::getCompiledLibraryName() const
{
  return OrientationAnalysisConstants::OrientationAnalysisBaseName;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindTwinBoundarySchmidFactors::getBrandingString() const
{
  return "OrientationAnalysis";
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindTwinBoundarySchmidFactors::getFilterVersion() const
{
  QString version;
  QTextStream vStream(&version);
  vStream << OrientationAnalysis::Version::Major() << "." << OrientationAnalysis::Version::Minor() << "." << OrientationAnalysis::Version::Patch();
  return version;
}
// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindTwinBoundarySchmidFactors::getGroupName() const
{
  return SIMPL::FilterGroups::StatisticsFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QUuid FindTwinBoundarySchmidFactors::getUuid() const
{
  return QUuid("{b0e30e6d-912d-5a7e-aeed-750134aba86b}");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindTwinBoundarySchmidFactors::getSubGroupName() const
{
  return SIMPL::FilterSubGroups::CrystallographyFilters;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString FindTwinBoundarySchmidFactors::getHumanLabel() const
{
  return "Find Twin Boundary Schmid Factors";
}

// -----------------------------------------------------------------------------
FindTwinBoundarySchmidFactors::Pointer FindTwinBoundarySchmidFactors::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
std::shared_ptr<FindTwinBoundarySchmidFactors> FindTwinBoundarySchmidFactors::New()
{
  struct make_shared_enabler : public FindTwinBoundarySchmidFactors
  {
  };
  std::shared_ptr<make_shared_enabler> val = std::make_shared<make_shared_enabler>();
  val->setupFilterParameters();
  return val;
}

// -----------------------------------------------------------------------------
QString FindTwinBoundarySchmidFactors::getNameOfClass() const
{
  return QString("FindTwinBoundarySchmidFactors");
}

// -----------------------------------------------------------------------------
QString FindTwinBoundarySchmidFactors::ClassName()
{
  return QString("FindTwinBoundarySchmidFactors");
}

// -----------------------------------------------------------------------------
void FindTwinBoundarySchmidFactors::setWriteFile(bool value)
{
  m_WriteFile = value;
}

// -----------------------------------------------------------------------------
bool FindTwinBoundarySchmidFactors::getWriteFile() const
{
  return m_WriteFile;
}

// -----------------------------------------------------------------------------
void FindTwinBoundarySchmidFactors::setTwinBoundarySchmidFactorsFile(const QString& value)
{
  m_TwinBoundarySchmidFactorsFile = value;
}

// -----------------------------------------------------------------------------
QString FindTwinBoundarySchmidFactors::getTwinBoundarySchmidFactorsFile() const
{
  return m_TwinBoundarySchmidFactorsFile;
}

// -----------------------------------------------------------------------------
void FindTwinBoundarySchmidFactors::setLoadingDir(const FloatVec3Type& value)
{
  m_LoadingDir = value;
}

// -----------------------------------------------------------------------------
FloatVec3Type FindTwinBoundarySchmidFactors::getLoadingDir() const
{
  return m_LoadingDir;
}

// -----------------------------------------------------------------------------
void FindTwinBoundarySchmidFactors::setAvgQuatsArrayPath(const DataArrayPath& value)
{
  m_AvgQuatsArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindTwinBoundarySchmidFactors::getAvgQuatsArrayPath() const
{
  return m_AvgQuatsArrayPath;
}

// -----------------------------------------------------------------------------
void FindTwinBoundarySchmidFactors::setFeaturePhasesArrayPath(const DataArrayPath& value)
{
  m_FeaturePhasesArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindTwinBoundarySchmidFactors::getFeaturePhasesArrayPath() const
{
  return m_FeaturePhasesArrayPath;
}

// -----------------------------------------------------------------------------
void FindTwinBoundarySchmidFactors::setCrystalStructuresArrayPath(const DataArrayPath& value)
{
  m_CrystalStructuresArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindTwinBoundarySchmidFactors::getCrystalStructuresArrayPath() const
{
  return m_CrystalStructuresArrayPath;
}

// -----------------------------------------------------------------------------
void FindTwinBoundarySchmidFactors::setSurfaceMeshFaceLabelsArrayPath(const DataArrayPath& value)
{
  m_SurfaceMeshFaceLabelsArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindTwinBoundarySchmidFactors::getSurfaceMeshFaceLabelsArrayPath() const
{
  return m_SurfaceMeshFaceLabelsArrayPath;
}

// -----------------------------------------------------------------------------
void FindTwinBoundarySchmidFactors::setSurfaceMeshFaceNormalsArrayPath(const DataArrayPath& value)
{
  m_SurfaceMeshFaceNormalsArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindTwinBoundarySchmidFactors::getSurfaceMeshFaceNormalsArrayPath() const
{
  return m_SurfaceMeshFaceNormalsArrayPath;
}

// -----------------------------------------------------------------------------
void FindTwinBoundarySchmidFactors::setSurfaceMeshTwinBoundaryArrayPath(const DataArrayPath& value)
{
  m_SurfaceMeshTwinBoundaryArrayPath = value;
}

// -----------------------------------------------------------------------------
DataArrayPath FindTwinBoundarySchmidFactors::getSurfaceMeshTwinBoundaryArrayPath() const
{
  return m_SurfaceMeshTwinBoundaryArrayPath;
}

// -----------------------------------------------------------------------------
void FindTwinBoundarySchmidFactors::setSurfaceMeshTwinBoundarySchmidFactorsArrayName(const QString& value)
{
  m_SurfaceMeshTwinBoundarySchmidFactorsArrayName = value;
}

// -----------------------------------------------------------------------------
QString FindTwinBoundarySchmidFactors::getSurfaceMeshTwinBoundarySchmidFactorsArrayName() const
{
  return m_SurfaceMeshTwinBoundarySchmidFactorsArrayName;
}
