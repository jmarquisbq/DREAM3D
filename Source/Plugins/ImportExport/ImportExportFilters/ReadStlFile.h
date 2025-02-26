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

#include <memory>

#include "SIMPLib/SIMPLib.h"
#include "SIMPLib/DataArrays/DataArray.hpp"
#include "SIMPLib/Filtering/AbstractFilter.h"

#include "ImportExport/ImportExportDLLExport.h"

/**
 * @brief The ReadStlFile class. See [Filter documentation](@ref readstlfile) for details.
 */
class ImportExport_EXPORT ReadStlFile : public AbstractFilter
{
  Q_OBJECT

  // Start Python bindings declarations
  PYB11_BEGIN_BINDINGS(ReadStlFile SUPERCLASS AbstractFilter)
  PYB11_FILTER()
  PYB11_SHARED_POINTERS(ReadStlFile)
  PYB11_FILTER_NEW_MACRO(ReadStlFile)
  PYB11_PROPERTY(DataArrayPath SurfaceMeshDataContainerName READ getSurfaceMeshDataContainerName WRITE setSurfaceMeshDataContainerName)
  PYB11_PROPERTY(QString FaceAttributeMatrixName READ getFaceAttributeMatrixName WRITE setFaceAttributeMatrixName)
  PYB11_PROPERTY(QString StlFilePath READ getStlFilePath WRITE setStlFilePath)
  PYB11_PROPERTY(QString FaceNormalsArrayName READ getFaceNormalsArrayName WRITE setFaceNormalsArrayName)
  PYB11_PROPERTY(bool ScaleOutput READ getScaleOutput WRITE setScaleOutput)
  PYB11_PROPERTY(float ScaleFactor READ getScaleFactor WRITE setScaleFactor)
  PYB11_PROPERTY(QString VertexAttributeMatrixName READ getVertexAttributeMatrixName WRITE setVertexAttributeMatrixName)

  PYB11_END_BINDINGS()
  // End Python bindings declarations

public:
  using Self = ReadStlFile;
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
   * @brief Returns the name of the class for ReadStlFile
   */
  QString getNameOfClass() const override;
  /**
   * @brief Returns the name of the class for ReadStlFile
   */
  static QString ClassName();

  ~ReadStlFile() override;

  /**
   * @brief Setter property for SurfaceMeshDataContainerName
   */
  void setSurfaceMeshDataContainerName(const DataArrayPath& value);
  /**
   * @brief Getter property for SurfaceMeshDataContainerName
   * @return Value of SurfaceMeshDataContainerName
   */
  DataArrayPath getSurfaceMeshDataContainerName() const;
  Q_PROPERTY(DataArrayPath SurfaceMeshDataContainerName READ getSurfaceMeshDataContainerName WRITE setSurfaceMeshDataContainerName)

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
   * @brief Setter property for StlFilePath
   */
  void setStlFilePath(const QString& value);
  /**
   * @brief Getter property for StlFilePath
   * @return Value of StlFilePath
   */
  QString getStlFilePath() const;
  Q_PROPERTY(QString StlFilePath READ getStlFilePath WRITE setStlFilePath)

  /**
   * @brief Setter property for FaceNormalsArrayName
   */
  void setFaceNormalsArrayName(const QString& value);
  /**
   * @brief Getter property for FaceNormalsArrayName
   * @return Value of FaceNormalsArrayName
   */
  QString getFaceNormalsArrayName() const;
  Q_PROPERTY(QString FaceNormalsArrayName READ getFaceNormalsArrayName WRITE setFaceNormalsArrayName)

  /**
   * @brief Setter property for ScaleOutput
   */
  void setScaleOutput(bool value);
  /**
   * @brief Getter property for ScaleOutput
   * @return Value of ScaleOutput
   */
  bool getScaleOutput() const;
  Q_PROPERTY(bool ScaleOutput READ getScaleOutput WRITE setScaleOutput)

  /**
   * @brief Setter property for ScaleOutput
   */
  void setScaleFactor(float value);
  /**
   * @brief Getter property for ScaleOutput
   * @return Value of ScaleOutput
   */
  float getScaleFactor() const;
  Q_PROPERTY(float ScaleFactor READ getScaleFactor WRITE setScaleFactor)


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
   * @brief readFilterParameters Reimplemented from @see AbstractFilter class
   */
  void readFilterParameters(AbstractFilterParametersReader* reader, int index) override;

  /**
   * @brief execute Reimplemented from @see AbstractFilter class
   */
  void execute() override;

protected:
  ReadStlFile();
  /**
   * @brief dataCheck Checks for the appropriate parameter values and availability of arrays
   */
  void dataCheck() override;

  /**
   * @brief Initializes all the private instance variables.
   */
  void initialize();

private:
  std::weak_ptr<DataArray<double>> m_FaceNormalsPtr;
  double* m_FaceNormals = nullptr;

  DataArrayPath m_SurfaceMeshDataContainerName = {SIMPL::Defaults::TriangleDataContainerName, "", ""};
  QString m_FaceAttributeMatrixName = {SIMPL::Defaults::FaceAttributeMatrixName};
  QString m_VertexAttributeMatrixName = {SIMPL::Defaults::VertexAttributeMatrixName};
  QString m_StlFilePath = {""};
  QString m_FaceNormalsArrayName = {SIMPL::FaceData::SurfaceMeshFaceNormals};

  float m_minXcoord = {std::numeric_limits<float>::max()};
  float m_maxXcoord = {-std::numeric_limits<float>::max()};
  float m_minYcoord = {std::numeric_limits<float>::max()};
  float m_maxYcoord = {-std::numeric_limits<float>::max()};
  float m_minZcoord = {std::numeric_limits<float>::max()};
  float m_maxZcoord = {-std::numeric_limits<float>::max()};

  bool m_ScaleOutput = false;
  float m_ScaleFactor = 1.0F;

  /**
   * @brief updateFaceInstancePointers Updates raw Face pointers
   */
  void updateFaceInstancePointers();

  /**
   * @brief readFile Reads the .stl file
   */
  void readFile();

  /**
   * @brief eliminate_duplicate_nodes Removes duplicate nodes to ensure the
   * created vertex list is shared
   */
  void eliminate_duplicate_nodes();

public:
  ReadStlFile(const ReadStlFile&) = delete;            // Copy Constructor Not Implemented
  ReadStlFile(ReadStlFile&&) = delete;                 // Move Constructor Not Implemented
  ReadStlFile& operator=(const ReadStlFile&) = delete; // Copy Assignment Not Implemented
  ReadStlFile& operator=(ReadStlFile&&) = delete;      // Move Assignment Not Implemented
};
