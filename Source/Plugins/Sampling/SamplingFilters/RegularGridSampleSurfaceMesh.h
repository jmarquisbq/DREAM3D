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
#include "SIMPLib/Common/Constants.h"
#include "SIMPLib/DataArrays/DataArray.hpp"
#include "SIMPLib/FilterParameters/FloatVec3FilterParameter.h"
#include "SIMPLib/Filtering/AbstractFilter.h"
#include "SIMPLib/Geometry/VertexGeom.h"

#include "Sampling/SamplingDLLExport.h"
#include "Sampling/SamplingFilters/SampleSurfaceMesh.h"

/**
 * @brief The RegularGridSampleSurfaceMesh class. See [Filter documentation](@ref regulargridsamplesurfacemesh) for details.
 */
class Sampling_EXPORT RegularGridSampleSurfaceMesh : public SampleSurfaceMesh
{
  Q_OBJECT

  // Start Python bindings declarations
  PYB11_BEGIN_BINDINGS(RegularGridSampleSurfaceMesh SUPERCLASS SampleSurfaceMesh)
  PYB11_FILTER()
  PYB11_SHARED_POINTERS(RegularGridSampleSurfaceMesh)
  PYB11_FILTER_NEW_MACRO(RegularGridSampleSurfaceMesh)
  PYB11_PROPERTY(DataArrayPath DataContainerName READ getDataContainerName WRITE setDataContainerName)
  PYB11_PROPERTY(QString CellAttributeMatrixName READ getCellAttributeMatrixName WRITE setCellAttributeMatrixName)
  PYB11_PROPERTY(int32_t LengthUnit READ getLengthUnit WRITE setLengthUnit)
  PYB11_PROPERTY(IntVec3Type Dimensions READ getDimensions WRITE setDimensions)
  PYB11_PROPERTY(FloatVec3Type Spacing READ getSpacing WRITE setSpacing)
  PYB11_PROPERTY(FloatVec3Type Origin READ getOrigin WRITE setOrigin)
  PYB11_PROPERTY(QString FeatureIdsArrayName READ getFeatureIdsArrayName WRITE setFeatureIdsArrayName)
  PYB11_END_BINDINGS()
  // End Python bindings declarations

public:
  using Self = RegularGridSampleSurfaceMesh;
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
   * @brief Returns the name of the class for RegularGridSampleSurfaceMesh
   */
  QString getNameOfClass() const override;
  /**
   * @brief Returns the name of the class for RegularGridSampleSurfaceMesh
   */
  static QString ClassName();

  ~RegularGridSampleSurfaceMesh() override;

  /**
   * @brief Setter property for DataContainerName
   */
  void setDataContainerName(const DataArrayPath& value);
  /**
   * @brief Getter property for DataContainerName
   * @return Value of DataContainerName
   */
  DataArrayPath getDataContainerName() const;
  Q_PROPERTY(DataArrayPath DataContainerName READ getDataContainerName WRITE setDataContainerName)

  /**
   * @brief Setter property for CellAttributeMatrixName
   */
  void setCellAttributeMatrixName(const QString& value);
  /**
   * @brief Getter property for CellAttributeMatrixName
   * @return Value of CellAttributeMatrixName
   */
  QString getCellAttributeMatrixName() const;
  Q_PROPERTY(QString CellAttributeMatrixName READ getCellAttributeMatrixName WRITE setCellAttributeMatrixName)

  /**
   * @brief Setter property for LengthUnit
   */
  void setLengthUnit(int32_t value);
  /**
   * @brief Getter property for LengthUnit
   * @return Value of LengthUnit
   */
  int32_t getLengthUnit() const;
  Q_PROPERTY(int32_t LengthUnit READ getLengthUnit WRITE setLengthUnit)

  /**
   * @brief Setter property for Dimensions
   */
  void setDimensions(const IntVec3Type& value);
  /**
   * @brief Getter property for Dimensions
   * @return Value of Dimensions
   */
  IntVec3Type getDimensions() const;
  Q_PROPERTY(IntVec3Type Dimensions READ getDimensions WRITE setDimensions)

  /**
   * @brief Setter property for Spacing
   */
  void setSpacing(const FloatVec3Type& value);
  /**
   * @brief Getter property for Spacing
   * @return Value of Spacing
   */
  FloatVec3Type getSpacing() const;
  Q_PROPERTY(FloatVec3Type Spacing READ getSpacing WRITE setSpacing)

  /**
   * @brief Setter property for Origin
   */
  void setOrigin(const FloatVec3Type& value);
  /**
   * @brief Getter property for Origin
   * @return Value of Origin
   */
  FloatVec3Type getOrigin() const;
  Q_PROPERTY(FloatVec3Type Origin READ getOrigin WRITE setOrigin)

  /**
   * @brief Setter property for FeatureIdsArrayName
   */
  void setFeatureIdsArrayName(const QString& value);
  /**
   * @brief Getter property for FeatureIdsArrayName
   * @return Value of FeatureIdsArrayName
   */
  QString getFeatureIdsArrayName() const;
  Q_PROPERTY(QString FeatureIdsArrayName READ getFeatureIdsArrayName WRITE setFeatureIdsArrayName)

  /**
   * @brief getBoxDimensions Returns a string describing the box dimensions and size/volume
   * @return
   */
  QString getBoxDimensions();
  Q_PROPERTY(QString BoxDimensions READ getBoxDimensions)

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
  RegularGridSampleSurfaceMesh();
  /**
   * @brief dataCheck Checks for the appropriate parameter values and availability of arrays
   */
  void dataCheck() override;

  /**
   * @brief Initializes all the private instance variables.
   */
  void initialize();

  /**
   * @brief generate_points Reimplemented from @see SampleSurfaceMesh class
   * @return VertexGeom object
   */
  VertexGeom::Pointer generate_points() override;

  /**
   * @brief assign_points Reimplemented from @see SampleSurfaceMesh class
   * @param iArray Sampled Feature Ids from superclass
   */
  void assign_points(Int32ArrayType::Pointer iArray) override;

private:
  std::weak_ptr<DataArray<int32_t>> m_FeatureIdsPtr;
  int32_t* m_FeatureIds = nullptr;

  DataArrayPath m_DataContainerName = {SIMPL::Defaults::ImageDataContainerName, "", ""};
  QString m_CellAttributeMatrixName = {SIMPL::Defaults::CellAttributeMatrixName};
  int32_t m_LengthUnit = {6}; //  Micrometers default
  IntVec3Type m_Dimensions = {128, 128, 128};
  FloatVec3Type m_Spacing = {1.0f, 1.0f, 1.0f};
  FloatVec3Type m_Origin = {0.0f, 0.0f, 0.0f};
  QString m_FeatureIdsArrayName = {SIMPL::CellData::FeatureIds};

public:
  RegularGridSampleSurfaceMesh(const RegularGridSampleSurfaceMesh&) = delete;            // Copy Constructor Not Implemented
  RegularGridSampleSurfaceMesh(RegularGridSampleSurfaceMesh&&) = delete;                 // Move Constructor Not Implemented
  RegularGridSampleSurfaceMesh& operator=(const RegularGridSampleSurfaceMesh&) = delete; // Copy Assignment Not Implemented
  RegularGridSampleSurfaceMesh& operator=(RegularGridSampleSurfaceMesh&&) = delete;      // Move Assignment Not Implemented
};
