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
#pragma once

#include "SurfaceMeshing/SurfaceMeshingDLLExport.h"

#include "SIMPLib/SIMPLib.h"
#include "SIMPLib/DataArrays/DataArray.hpp"
#include "SIMPLib/Filtering/AbstractFilter.h"
#include "SIMPLib/Geometry/IGeometry.h"
#include "SIMPLib/Geometry/IGeometryGrid.h"

#include <memory>

/**
 * @brief The QuickSurfaceMesh class. See [Filter documentation](@ref quicksurfacemesh) for details.
 */
class SurfaceMeshing_EXPORT QuickSurfaceMesh : public AbstractFilter
{
  Q_OBJECT

  // Start Python bindings declarations
  PYB11_BEGIN_BINDINGS(QuickSurfaceMesh SUPERCLASS AbstractFilter)
  PYB11_FILTER()
  PYB11_SHARED_POINTERS(QuickSurfaceMesh)
  PYB11_FILTER_NEW_MACRO(QuickSurfaceMesh)
  PYB11_PROPERTY(std::vector<DataArrayPath> SelectedDataArrayPaths READ getSelectedDataArrayPaths WRITE setSelectedDataArrayPaths)
  PYB11_PROPERTY(DataArrayPath SurfaceDataContainerName READ getSurfaceDataContainerName WRITE setSurfaceDataContainerName)
  PYB11_PROPERTY(QString VertexAttributeMatrixName READ getVertexAttributeMatrixName WRITE setVertexAttributeMatrixName)
  PYB11_PROPERTY(QString FaceAttributeMatrixName READ getFaceAttributeMatrixName WRITE setFaceAttributeMatrixName)
  PYB11_PROPERTY(DataArrayPath FeatureIdsArrayPath READ getFeatureIdsArrayPath WRITE setFeatureIdsArrayPath)
  PYB11_PROPERTY(QString FaceLabelsArrayName READ getFaceLabelsArrayName WRITE setFaceLabelsArrayName)
  PYB11_PROPERTY(QString NodeTypesArrayName READ getNodeTypesArrayName WRITE setNodeTypesArrayName)
  PYB11_PROPERTY(QString FeatureAttributeMatrixName READ getFeatureAttributeMatrixName WRITE setFeatureAttributeMatrixName)
  PYB11_PROPERTY(bool FixProblemVoxels READ getFixProblemVoxels WRITE setFixProblemVoxels)
  PYB11_END_BINDINGS()
  // End Python bindings declarations

public:
  using Self = QuickSurfaceMesh;
  using Pointer = std::shared_ptr<Self>;
  using ConstPointer = std::shared_ptr<const Self>;
  using WeakPointer = std::weak_ptr<Self>;
  using ConstWeakPointer = std::weak_ptr<const Self>;

  /**
   * @brief Returns a NullPointer wrapped by a shared_ptr<>
   * @return
   */
  static Pointer NullPointer();

  /**
   * @brief Creates a new object wrapped in a shared_ptr<>
   * @return
   */
  static Pointer New();

  /**
   * @brief Returns the name of the class for QuickSurfaceMesh
   */
  QString getNameOfClass() const override;
  /**
   * @brief Returns the name of the class for QuickSurfaceMesh
   */
  static QString ClassName();

  ~QuickSurfaceMesh() override;

  /**
   * @brief Setter property for SelectedDataArrayPaths
   */
  void setSelectedDataArrayPaths(const std::vector<DataArrayPath>& value);
  /**
   * @brief Getter property for SelectedDataArrayPaths
   * @return Value of SelectedDataArrayPaths
   */
  std::vector<DataArrayPath> getSelectedDataArrayPaths() const;
  Q_PROPERTY(DataArrayPathVec SelectedDataArrayPaths READ getSelectedDataArrayPaths WRITE setSelectedDataArrayPaths)

  /**
   * @brief Setter property for SurfaceDataContainerName
   */
  void setSurfaceDataContainerName(const DataArrayPath& value);
  /**
   * @brief Getter property for SurfaceDataContainerName
   * @return Value of SurfaceDataContainerName
   */
  DataArrayPath getSurfaceDataContainerName() const;
  Q_PROPERTY(DataArrayPath SurfaceDataContainerName READ getSurfaceDataContainerName WRITE setSurfaceDataContainerName)

  /**
   * @brief Setter property for TripleLineDataContainerName
   */
  void setTripleLineDataContainerName(const DataArrayPath& value);
  /**
   * @brief Getter property for TripleLineDataContainerName
   * @return Value of TripleLineDataContainerName
   */
  DataArrayPath getTripleLineDataContainerName() const;
  Q_PROPERTY(DataArrayPath TripleLineDataContainerName READ getTripleLineDataContainerName WRITE setTripleLineDataContainerName)

  /**
   * @brief Setter property for VertexAttributeMatrixName
   */
  void setVertexAttributeMatrixName(const QString& value);
  /**
   * @brief Getter property for VertexAttributeMatrixName
   * @return Value of VertexAttributeMatrixName
   */
  QString getVertexAttributeMatrixName() const;
  Q_PROPERTY(QString VertexAttributeMatrixName READ getVertexAttributeMatrixName WRITE setVertexAttributeMatrixName)

  /**
   * @brief Setter property for FaceAttributeMatrixName
   */
  void setFaceAttributeMatrixName(const QString& value);
  /**
   * @brief Getter property for FaceAttributeMatrixName
   * @return Value of FaceAttributeMatrixName
   */
  QString getFaceAttributeMatrixName() const;
  Q_PROPERTY(QString FaceAttributeMatrixName READ getFaceAttributeMatrixName WRITE setFaceAttributeMatrixName)

  /**
   * @brief Setter property for FeatureIdsArrayPath
   */
  void setFeatureIdsArrayPath(const DataArrayPath& value);
  /**
   * @brief Getter property for FeatureIdsArrayPath
   * @return Value of FeatureIdsArrayPath
   */
  DataArrayPath getFeatureIdsArrayPath() const;
  Q_PROPERTY(DataArrayPath FeatureIdsArrayPath READ getFeatureIdsArrayPath WRITE setFeatureIdsArrayPath)

  /**
   * @brief Setter property for FaceLabelsArrayName
   */
  void setFaceLabelsArrayName(const QString& value);
  /**
   * @brief Getter property for FaceLabelsArrayName
   * @return Value of FaceLabelsArrayName
   */
  QString getFaceLabelsArrayName() const;
  Q_PROPERTY(QString FaceLabelsArrayName READ getFaceLabelsArrayName WRITE setFaceLabelsArrayName)

  /**
   * @brief Setter property for NodeTypesArrayName
   */
  void setNodeTypesArrayName(const QString& value);
  /**
   * @brief Getter property for NodeTypesArrayName
   * @return Value of NodeTypesArrayName
   */
  QString getNodeTypesArrayName() const;
  Q_PROPERTY(QString NodeTypesArrayName READ getNodeTypesArrayName WRITE setNodeTypesArrayName)

  /**
   * @brief Setter property for FeatureAttributeMatrixName
   */
  void setFeatureAttributeMatrixName(const QString& value);
  /**
   * @brief Getter property for FeatureAttributeMatrixName
   * @return Value of FeatureAttributeMatrixName
   */
  QString getFeatureAttributeMatrixName() const;
  Q_PROPERTY(QString FeatureAttributeMatrixName READ getFeatureAttributeMatrixName WRITE setFeatureAttributeMatrixName)

  /**
   * @brief Setter property for FeatureAttributeMatrixName
   */
  void setFixProblemVoxels(bool value);
  /**
   * @brief Getter property for FeatureAttributeMatrixName
   * @return Value of FeatureAttributeMatrixName
   */
  bool getFixProblemVoxels() const;
  Q_PROPERTY(bool FixProblemVoxels READ getFixProblemVoxels WRITE setFixProblemVoxels)

  /**
   * @brief getCompiledLibraryName Reimplemented from @see AbstractFilter class
   */
  QString getCompiledLibraryName() const override;

  /**
   * @brief getBrandingString Returns the branding string for the filter, which is a tag
   * used to denote the filter's association with specific plugins
   * @return Branding string
   */
  QString getBrandingString() const override;

  /**
   * @brief getFilterVersion Returns a version string for this filter. Default
   * value is an empty string.
   * @return
   */
  QString getFilterVersion() const override;

  /**
   * @brief newFilterInstance Reimplemented from @see AbstractFilter class
   */
  AbstractFilter::Pointer newFilterInstance(bool copyFilterParameters) const override;

  /**
   * @brief getGroupName Reimplemented from @see AbstractFilter class
   */
  QString getGroupName() const override;

  /**
   * @brief getSubGroupName Reimplemented from @see AbstractFilter class
   */
  QString getSubGroupName() const override;

  /**
   * @brief getUuid Return the unique identifier for this filter.
   * @return A QUuid object.
   */
  QUuid getUuid() const override;

  /**
   * @brief getHumanLabel Reimplemented from @see AbstractFilter class
   */
  QString getHumanLabel() const override;

  /**
   * @brief setupFilterParameters Reimplemented from @see AbstractFilter class
   */
  void setupFilterParameters() override;

  /**
   * @brief execute Reimplemented from @see AbstractFilter class
   */
  void execute() override;

protected:
  QuickSurfaceMesh();
  /**
   * @brief dataCheck Checks for the appropriate parameter values and availability of arrays
   */
  void dataCheck() override;

  /**
   * @brief Initializes all the private instance variables.
   */
  void initialize();

private:
  std::weak_ptr<DataArray<int32_t>> m_FeatureIdsPtr;
  int32_t* m_FeatureIds = nullptr;
  std::weak_ptr<DataArray<int32_t>> m_FaceLabelsPtr;
  int32_t* m_FaceLabels = nullptr;
  std::weak_ptr<DataArray<int8_t>> m_NodeTypesPtr;
  int8_t* m_NodeTypes = nullptr;

  std::vector<DataArrayPath> m_SelectedDataArrayPaths = {};
  DataArrayPath m_SurfaceDataContainerName = {SIMPL::Defaults::TriangleDataContainerName, "", ""};
  DataArrayPath m_TripleLineDataContainerName = {"TripleLines", "", ""};
  QString m_VertexAttributeMatrixName = {SIMPL::Defaults::VertexAttributeMatrixName};
  QString m_FaceAttributeMatrixName = {SIMPL::Defaults::FaceAttributeMatrixName};
  DataArrayPath m_FeatureIdsArrayPath = {SIMPL::Defaults::ImageDataContainerName, SIMPL::Defaults::CellAttributeMatrixName, SIMPL::CellData::FeatureIds};
  QString m_FaceLabelsArrayName = {SIMPL::FaceData::SurfaceMeshFaceLabels};
  QString m_NodeTypesArrayName = {SIMPL::VertexData::SurfaceMeshNodeType};
  QString m_FeatureAttributeMatrixName = {SIMPL::Defaults::FaceFeatureAttributeMatrixName};
  bool m_FixProblemVoxels = true;
  bool m_GenerateTripleLines = false;

  std::vector<IDataArray::WeakPointer> m_SelectedWeakPtrVector;
  std::vector<IDataArray::WeakPointer> m_CreatedWeakPtrVector;

  /**
   * @brief getGridCoordinates
   * @param grid
   * @param x
   * @param y
   * @param z
   * @param coords
   */
  void getGridCoordinates(const IGeometryGrid::Pointer& grid, size_t x, size_t y, size_t z, float* coords);

  /**
   * @brief flipProblemVoxelCase1
   * @param v1
   * @param v2
   * @param v3
   * @param v4
   * @param v5
   * @param v6
   */
  void flipProblemVoxelCase1(MeshIndexType v1, MeshIndexType v2, MeshIndexType v3, MeshIndexType v4, MeshIndexType v5, MeshIndexType v6);

  /**
   * @brief flipProblemVoxelCase2
   * @param v1
   * @param v2
   * @param v3
   * @param v4
   */
  void flipProblemVoxelCase2(MeshIndexType v1, MeshIndexType v2, MeshIndexType v3, MeshIndexType v4);

  /**
   * @brief flipProblemVoxelCase3
   * @param v1
   * @param v2
   * @param v3
   */
  void flipProblemVoxelCase3(MeshIndexType v1, MeshIndexType v2, MeshIndexType v3);

  void correctProblemVoxels();

  void determineActiveNodes(std::vector<MeshIndexType>& m_NodeIds, MeshIndexType& nodeCount, MeshIndexType& triangleCount);

  void createNodesAndTriangles(std::vector<MeshIndexType> m_NodeIds, MeshIndexType nodeCount, MeshIndexType triangleCount);

  /**
   * @brief updateFaceInstancePointers Updates raw Face pointers
   */
  void updateFaceInstancePointers();

  /**
   * @brief updateVertexInstancePointers Updates raw Vertex pointers
   */
  void updateVertexInstancePointers();

  /**
   * @brief generateTripleLines
   */
  void generateTripleLines();

public:
  QuickSurfaceMesh(const QuickSurfaceMesh&) = delete;            // Copy Constructor Not Implemented
  QuickSurfaceMesh(QuickSurfaceMesh&&) = delete;                 // Move Constructor Not Implemented
  QuickSurfaceMesh& operator=(const QuickSurfaceMesh&) = delete; // Copy Assignment Not Implemented
  QuickSurfaceMesh& operator=(QuickSurfaceMesh&&) = delete;      // Move assignment Not Implemented
};
