/*
 * Your License or Copyright can go here
 */

#pragma once

#include <memory>

#include "SIMPLib/SIMPLib.h"
#include "SIMPLib/DataArrays/DataArray.hpp"
#include "SIMPLib/Filtering/AbstractFilter.h"

#include "StatsToolbox/StatsToolboxDLLExport.h"

/**
 * @brief The ComputeMomentInvariants2D class. See [Filter documentation](@ref computemomentinvariants2d) for details.
 */
class StatsToolbox_EXPORT ComputeMomentInvariants2D : public AbstractFilter
{
  Q_OBJECT

  // Start Python bindings declarations
  PYB11_BEGIN_BINDINGS(ComputeMomentInvariants2D SUPERCLASS AbstractFilter)
  PYB11_FILTER()
  PYB11_SHARED_POINTERS(ComputeMomentInvariants2D)
  PYB11_FILTER_NEW_MACRO(ComputeMomentInvariants2D)
  PYB11_PROPERTY(DataArrayPath FeatureIdsArrayPath READ getFeatureIdsArrayPath WRITE setFeatureIdsArrayPath)
  PYB11_PROPERTY(DataArrayPath FeatureRectArrayPath READ getFeatureRectArrayPath WRITE setFeatureRectArrayPath)
  PYB11_PROPERTY(bool NormalizeMomentInvariants READ getNormalizeMomentInvariants WRITE setNormalizeMomentInvariants)
  PYB11_PROPERTY(DataArrayPath Omega1ArrayPath READ getOmega1ArrayPath WRITE setOmega1ArrayPath)
  PYB11_PROPERTY(DataArrayPath Omega2ArrayPath READ getOmega2ArrayPath WRITE setOmega2ArrayPath)
  PYB11_PROPERTY(bool SaveCentralMoments READ getSaveCentralMoments WRITE setSaveCentralMoments)
  PYB11_PROPERTY(DataArrayPath CentralMomentsArrayPath READ getCentralMomentsArrayPath WRITE setCentralMomentsArrayPath)
  PYB11_END_BINDINGS()
  // End Python bindings declarations

public:
  using Self = ComputeMomentInvariants2D;
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
   * @brief Returns the name of the class for ComputeMomentInvariants2D
   */
  QString getNameOfClass() const override;
  /**
   * @brief Returns the name of the class for ComputeMomentInvariants2D
   */
  static QString ClassName();

  ~ComputeMomentInvariants2D() override;

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
   * @brief Setter property for FeatureRectArrayPath
   */
  void setFeatureRectArrayPath(const DataArrayPath& value);
  /**
   * @brief Getter property for FeatureRectArrayPath
   * @return Value of FeatureRectArrayPath
   */
  DataArrayPath getFeatureRectArrayPath() const;
  Q_PROPERTY(DataArrayPath FeatureRectArrayPath READ getFeatureRectArrayPath WRITE setFeatureRectArrayPath)

  /**
   * @brief Setter property for NormalizeMomentInvariants
   */
  void setNormalizeMomentInvariants(bool value);
  /**
   * @brief Getter property for NormalizeMomentInvariants
   * @return Value of NormalizeMomentInvariants
   */
  bool getNormalizeMomentInvariants() const;
  Q_PROPERTY(bool NormalizeMomentInvariants READ getNormalizeMomentInvariants WRITE setNormalizeMomentInvariants)

  /**
   * @brief Setter property for Omega1ArrayPath
   */
  void setOmega1ArrayPath(const DataArrayPath& value);
  /**
   * @brief Getter property for Omega1ArrayPath
   * @return Value of Omega1ArrayPath
   */
  DataArrayPath getOmega1ArrayPath() const;
  Q_PROPERTY(DataArrayPath Omega1ArrayPath READ getOmega1ArrayPath WRITE setOmega1ArrayPath)

  /**
   * @brief Setter property for Omega2ArrayPath
   */
  void setOmega2ArrayPath(const DataArrayPath& value);
  /**
   * @brief Getter property for Omega2ArrayPath
   * @return Value of Omega2ArrayPath
   */
  DataArrayPath getOmega2ArrayPath() const;
  Q_PROPERTY(DataArrayPath Omega2ArrayPath READ getOmega2ArrayPath WRITE setOmega2ArrayPath)

  /**
   * @brief Setter property for SaveCentralMoments
   */
  void setSaveCentralMoments(bool value);
  /**
   * @brief Getter property for SaveCentralMoments
   * @return Value of SaveCentralMoments
   */
  bool getSaveCentralMoments() const;
  Q_PROPERTY(bool SaveCentralMoments READ getSaveCentralMoments WRITE setSaveCentralMoments)

  /**
   * @brief Setter property for CentralMomentsArrayPath
   */
  void setCentralMomentsArrayPath(const DataArrayPath& value);
  /**
   * @brief Getter property for CentralMomentsArrayPath
   * @return Value of CentralMomentsArrayPath
   */
  DataArrayPath getCentralMomentsArrayPath() const;
  Q_PROPERTY(DataArrayPath CentralMomentsArrayPath READ getCentralMomentsArrayPath WRITE setCentralMomentsArrayPath)

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
  ComputeMomentInvariants2D();

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
  std::weak_ptr<DataArray<uint32_t>> m_FeatureRectPtr;
  uint32_t* m_FeatureRect = nullptr;
  std::weak_ptr<DataArray<float>> m_Omega1Ptr;
  float* m_Omega1 = nullptr;
  std::weak_ptr<DataArray<float>> m_Omega2Ptr;
  float* m_Omega2 = nullptr;
  std::weak_ptr<DataArray<float>> m_CentralMomentsPtr;
  float* m_CentralMoments = nullptr;

  DataArrayPath m_FeatureIdsArrayPath = {SIMPL::Defaults::ImageDataContainerName, SIMPL::Defaults::CellAttributeMatrixName, SIMPL::CellData::FeatureIds};
  DataArrayPath m_FeatureRectArrayPath = {SIMPL::Defaults::ImageDataContainerName, SIMPL::Defaults::CellFeatureAttributeMatrixName, "FeatureRect"};
  bool m_NormalizeMomentInvariants = {true};
  DataArrayPath m_Omega1ArrayPath = {SIMPL::Defaults::ImageDataContainerName, SIMPL::Defaults::CellFeatureAttributeMatrixName, "Omega1"};
  DataArrayPath m_Omega2ArrayPath = {SIMPL::Defaults::ImageDataContainerName, SIMPL::Defaults::CellFeatureAttributeMatrixName, "Omega2"};
  bool m_SaveCentralMoments = {false};
  DataArrayPath m_CentralMomentsArrayPath = {SIMPL::Defaults::ImageDataContainerName, SIMPL::Defaults::CellFeatureAttributeMatrixName, "CentralMoments"};

public:
  ComputeMomentInvariants2D(const ComputeMomentInvariants2D&) = delete;            // Copy Constructor Not Implemented
  ComputeMomentInvariants2D(ComputeMomentInvariants2D&&) = delete;                 // Move Constructor Not Implemented
  ComputeMomentInvariants2D& operator=(const ComputeMomentInvariants2D&) = delete; // Copy Assignment Not Implemented
  ComputeMomentInvariants2D& operator=(ComputeMomentInvariants2D&&) = delete;      // Move Assignment Not Implemented
};
