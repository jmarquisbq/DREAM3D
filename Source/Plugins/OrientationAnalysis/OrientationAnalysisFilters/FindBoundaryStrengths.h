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
#include "SIMPLib/FilterParameters/FloatVec3FilterParameter.h"
#include "SIMPLib/Filtering/AbstractFilter.h"

#include "OrientationAnalysis/OrientationAnalysisDLLExport.h"

class LaueOps;
using LaueOpsShPtrType = std::shared_ptr<LaueOps>;
using LaueOpsContainer = std::vector<LaueOpsShPtrType>;

/**
 * @brief The FindBoundaryStrengths class. See [Filter documentation](@ref findboundarystrengths) for details.
 */
class OrientationAnalysis_EXPORT FindBoundaryStrengths : public AbstractFilter
{
  Q_OBJECT

  // Start Python bindings declarations
  PYB11_BEGIN_BINDINGS(FindBoundaryStrengths SUPERCLASS AbstractFilter)
  PYB11_FILTER()
  PYB11_SHARED_POINTERS(FindBoundaryStrengths)
  PYB11_FILTER_NEW_MACRO(FindBoundaryStrengths)
  PYB11_PROPERTY(FloatVec3Type Loading READ getLoading WRITE setLoading)
  PYB11_PROPERTY(DataArrayPath SurfaceMeshFaceLabelsArrayPath READ getSurfaceMeshFaceLabelsArrayPath WRITE setSurfaceMeshFaceLabelsArrayPath)
  PYB11_PROPERTY(DataArrayPath AvgQuatsArrayPath READ getAvgQuatsArrayPath WRITE setAvgQuatsArrayPath)
  PYB11_PROPERTY(DataArrayPath FeaturePhasesArrayPath READ getFeaturePhasesArrayPath WRITE setFeaturePhasesArrayPath)
  PYB11_PROPERTY(DataArrayPath CrystalStructuresArrayPath READ getCrystalStructuresArrayPath WRITE setCrystalStructuresArrayPath)
  PYB11_PROPERTY(QString SurfaceMeshF1sArrayName READ getSurfaceMeshF1sArrayName WRITE setSurfaceMeshF1sArrayName)
  PYB11_PROPERTY(QString SurfaceMeshF1sptsArrayName READ getSurfaceMeshF1sptsArrayName WRITE setSurfaceMeshF1sptsArrayName)
  PYB11_PROPERTY(QString SurfaceMeshF7sArrayName READ getSurfaceMeshF7sArrayName WRITE setSurfaceMeshF7sArrayName)
  PYB11_PROPERTY(QString SurfaceMeshmPrimesArrayName READ getSurfaceMeshmPrimesArrayName WRITE setSurfaceMeshmPrimesArrayName)
  PYB11_END_BINDINGS()
  // End Python bindings declarations

public:
  using Self = FindBoundaryStrengths;
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
   * @brief Returns the name of the class for FindBoundaryStrengths
   */
  QString getNameOfClass() const override;
  /**
   * @brief Returns the name of the class for FindBoundaryStrengths
   */
  static QString ClassName();

  ~FindBoundaryStrengths() override;

  /**
   * @brief Setter property for Loading
   */
  void setLoading(const FloatVec3Type& value);
  /**
   * @brief Getter property for Loading
   * @return Value of Loading
   */
  FloatVec3Type getLoading() const;
  Q_PROPERTY(FloatVec3Type Loading READ getLoading WRITE setLoading)

  /**
   * @brief Setter property for SurfaceMeshFaceLabelsArrayPath
   */
  void setSurfaceMeshFaceLabelsArrayPath(const DataArrayPath& value);
  /**
   * @brief Getter property for SurfaceMeshFaceLabelsArrayPath
   * @return Value of SurfaceMeshFaceLabelsArrayPath
   */
  DataArrayPath getSurfaceMeshFaceLabelsArrayPath() const;
  Q_PROPERTY(DataArrayPath SurfaceMeshFaceLabelsArrayPath READ getSurfaceMeshFaceLabelsArrayPath WRITE setSurfaceMeshFaceLabelsArrayPath)

  /**
   * @brief Setter property for AvgQuatsArrayPath
   */
  void setAvgQuatsArrayPath(const DataArrayPath& value);
  /**
   * @brief Getter property for AvgQuatsArrayPath
   * @return Value of AvgQuatsArrayPath
   */
  DataArrayPath getAvgQuatsArrayPath() const;
  Q_PROPERTY(DataArrayPath AvgQuatsArrayPath READ getAvgQuatsArrayPath WRITE setAvgQuatsArrayPath)

  /**
   * @brief Setter property for FeaturePhasesArrayPath
   */
  void setFeaturePhasesArrayPath(const DataArrayPath& value);
  /**
   * @brief Getter property for FeaturePhasesArrayPath
   * @return Value of FeaturePhasesArrayPath
   */
  DataArrayPath getFeaturePhasesArrayPath() const;
  Q_PROPERTY(DataArrayPath FeaturePhasesArrayPath READ getFeaturePhasesArrayPath WRITE setFeaturePhasesArrayPath)

  /**
   * @brief Setter property for CrystalStructuresArrayPath
   */
  void setCrystalStructuresArrayPath(const DataArrayPath& value);
  /**
   * @brief Getter property for CrystalStructuresArrayPath
   * @return Value of CrystalStructuresArrayPath
   */
  DataArrayPath getCrystalStructuresArrayPath() const;
  Q_PROPERTY(DataArrayPath CrystalStructuresArrayPath READ getCrystalStructuresArrayPath WRITE setCrystalStructuresArrayPath)

  /**
   * @brief Setter property for SurfaceMeshF1sArrayName
   */
  void setSurfaceMeshF1sArrayName(const QString& value);
  /**
   * @brief Getter property for SurfaceMeshF1sArrayName
   * @return Value of SurfaceMeshF1sArrayName
   */
  QString getSurfaceMeshF1sArrayName() const;
  Q_PROPERTY(QString SurfaceMeshF1sArrayName READ getSurfaceMeshF1sArrayName WRITE setSurfaceMeshF1sArrayName)

  /**
   * @brief Setter property for SurfaceMeshF1sptsArrayName
   */
  void setSurfaceMeshF1sptsArrayName(const QString& value);
  /**
   * @brief Getter property for SurfaceMeshF1sptsArrayName
   * @return Value of SurfaceMeshF1sptsArrayName
   */
  QString getSurfaceMeshF1sptsArrayName() const;
  Q_PROPERTY(QString SurfaceMeshF1sptsArrayName READ getSurfaceMeshF1sptsArrayName WRITE setSurfaceMeshF1sptsArrayName)

  /**
   * @brief Setter property for SurfaceMeshF7sArrayName
   */
  void setSurfaceMeshF7sArrayName(const QString& value);
  /**
   * @brief Getter property for SurfaceMeshF7sArrayName
   * @return Value of SurfaceMeshF7sArrayName
   */
  QString getSurfaceMeshF7sArrayName() const;
  Q_PROPERTY(QString SurfaceMeshF7sArrayName READ getSurfaceMeshF7sArrayName WRITE setSurfaceMeshF7sArrayName)

  /**
   * @brief Setter property for SurfaceMeshmPrimesArrayName
   */
  void setSurfaceMeshmPrimesArrayName(const QString& value);
  /**
   * @brief Getter property for SurfaceMeshmPrimesArrayName
   * @return Value of SurfaceMeshmPrimesArrayName
   */
  QString getSurfaceMeshmPrimesArrayName() const;
  Q_PROPERTY(QString SurfaceMeshmPrimesArrayName READ getSurfaceMeshmPrimesArrayName WRITE setSurfaceMeshmPrimesArrayName)

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
  FindBoundaryStrengths();

  /**
   * @brief dataCheck Checks for the appropriate parameter values and availability of arrays
   */
  void dataCheck() override;

private:
  std::weak_ptr<DataArray<int32_t>> m_FeaturePhasesPtr;
  int32_t* m_FeaturePhases = nullptr;
  std::weak_ptr<DataArray<float>> m_AvgQuatsPtr;
  float* m_AvgQuats = nullptr;
  std::weak_ptr<DataArray<unsigned int>> m_CrystalStructuresPtr;
  unsigned int* m_CrystalStructures = nullptr;
  std::weak_ptr<DataArray<int32_t>> m_SurfaceMeshFaceLabelsPtr;
  int32_t* m_SurfaceMeshFaceLabels = nullptr;
  std::weak_ptr<DataArray<float>> m_SurfaceMeshF1sPtr;
  float* m_SurfaceMeshF1s = nullptr;
  std::weak_ptr<DataArray<float>> m_SurfaceMeshF1sptsPtr;
  float* m_SurfaceMeshF1spts = nullptr;
  std::weak_ptr<DataArray<float>> m_SurfaceMeshF7sPtr;
  float* m_SurfaceMeshF7s = nullptr;
  std::weak_ptr<DataArray<float>> m_SurfaceMeshmPrimesPtr;
  float* m_SurfaceMeshmPrimes = nullptr;

  FloatVec3Type m_Loading = {};
  DataArrayPath m_SurfaceMeshFaceLabelsArrayPath = {SIMPL::Defaults::TriangleDataContainerName, SIMPL::Defaults::FaceAttributeMatrixName, SIMPL::FaceData::SurfaceMeshFaceLabels};
  DataArrayPath m_AvgQuatsArrayPath = {SIMPL::Defaults::ImageDataContainerName, SIMPL::Defaults::CellFeatureAttributeMatrixName, SIMPL::FeatureData::AvgQuats};
  DataArrayPath m_FeaturePhasesArrayPath = {SIMPL::Defaults::ImageDataContainerName, SIMPL::Defaults::CellFeatureAttributeMatrixName, SIMPL::FeatureData::Phases};
  DataArrayPath m_CrystalStructuresArrayPath = {SIMPL::Defaults::ImageDataContainerName, SIMPL::Defaults::CellEnsembleAttributeMatrixName, SIMPL::EnsembleData::CrystalStructures};
  QString m_SurfaceMeshF1sArrayName = {SIMPL::FaceData::SurfaceMeshF1s};
  QString m_SurfaceMeshF1sptsArrayName = {SIMPL::FaceData::SurfaceMeshF1spts};
  QString m_SurfaceMeshF7sArrayName = {SIMPL::FaceData::SurfaceMeshF7s};
  QString m_SurfaceMeshmPrimesArrayName = {SIMPL::FaceData::SurfaceMeshmPrimes};

  LaueOpsContainer m_OrientationOps;
  /**
   * @brief dataCheckVoxel Checks for the appropriate parameter values and availability of arrays
   */
  void dataCheckVoxel();

  /**
   * @brief dataCheckSurfaceMesh Checks for the appropriate parameter values and availability of arrays
   */
  void dataCheckSurfaceMesh();

public:
  FindBoundaryStrengths(const FindBoundaryStrengths&) = delete;            // Copy Constructor Not Implemented
  FindBoundaryStrengths(FindBoundaryStrengths&&) = delete;                 // Move Constructor Not Implemented
  FindBoundaryStrengths& operator=(const FindBoundaryStrengths&) = delete; // Copy Assignment Not Implemented
  FindBoundaryStrengths& operator=(FindBoundaryStrengths&&) = delete;      // Move Assignment Not Implemented
};
