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
* The code contained herein was partially funded by the followig contracts:
*    United States Air Force Prime Contract FA8650-07-D-5800
*    United States Air Force Prime Contract FA8650-10-D-5210
*    United States Prime Contract Navy N00173-07-C-2068
*
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifndef _generatefaceschuhmisorientationcoloring_h_
#define _generatefaceschuhmisorientationcoloring_h_

#include <QtCore/QString>

#include "SIMPLib/SIMPLib.h"
#include "SIMPLib/Common/SIMPLibSetGetMacros.h"
#include "SIMPLib/DataArrays/IDataArray.h"
#include "SurfaceMeshing/SurfaceMeshingFilters/SurfaceMeshFilter.h"

#include "SurfaceMeshing/SurfaceMeshingConstants.h"
#include "SurfaceMeshing/SurfaceMeshingVersion.h"

/**
 * @class GenerateFaceMisorientationColors GenerateFaceMisorientationColors.h DREAM3DLib/ProcessingFilters/GenerateFaceMisorientationColors.h
 * @brief This filter calculates the normal of each triangle in the surface mesh. The calculated normals have been
 * normalized themselves. This filter is parallelized using the Threading Building Blocks library and will attempt to
 * use the optimal number of processors to do the computation.
 * @author Will Lenthe
 * @date March 12, 2014
 * @version 1.0
 */
class GenerateFaceSchuhMisorientationColoring : public SurfaceMeshFilter
{
    Q_OBJECT
    PYB11_CREATE_BINDINGS(GenerateFaceSchuhMisorientationColoring SUPERCLASS SurfaceMeshFilter)
    PYB11_PROPERTY(DataArrayPath SurfaceMeshFaceLabelsArrayPath READ getSurfaceMeshFaceLabelsArrayPath WRITE setSurfaceMeshFaceLabelsArrayPath)
    PYB11_PROPERTY(DataArrayPath AvgQuatsArrayPath READ getAvgQuatsArrayPath WRITE setAvgQuatsArrayPath)
    PYB11_PROPERTY(DataArrayPath FeaturePhasesArrayPath READ getFeaturePhasesArrayPath WRITE setFeaturePhasesArrayPath)
    PYB11_PROPERTY(DataArrayPath CrystalStructuresArrayPath READ getCrystalStructuresArrayPath WRITE setCrystalStructuresArrayPath)
    PYB11_PROPERTY(QString SurfaceMeshFaceSchuhMisorientationColorsArrayName READ getSurfaceMeshFaceSchuhMisorientationColorsArrayName WRITE setSurfaceMeshFaceSchuhMisorientationColorsArrayName)
  public:
    SIMPL_SHARED_POINTERS(GenerateFaceSchuhMisorientationColoring)
    SIMPL_FILTER_NEW_MACRO(GenerateFaceSchuhMisorientationColoring)
     SIMPL_TYPE_MACRO_SUPER_OVERRIDE(GenerateFaceSchuhMisorientationColoring, SurfaceMeshFilter)

     ~GenerateFaceSchuhMisorientationColoring() override;

     /**
      * @brief This returns the group that the filter belonds to. You can select
      * a different group if you want. The string returned here will be displayed
      * in the GUI for the filter
      */
     SIMPL_FILTER_PARAMETER(DataArrayPath, SurfaceMeshFaceLabelsArrayPath)
     Q_PROPERTY(DataArrayPath SurfaceMeshFaceLabelsArrayPath READ getSurfaceMeshFaceLabelsArrayPath WRITE setSurfaceMeshFaceLabelsArrayPath)

     SIMPL_FILTER_PARAMETER(DataArrayPath, AvgQuatsArrayPath)
     Q_PROPERTY(DataArrayPath AvgQuatsArrayPath READ getAvgQuatsArrayPath WRITE setAvgQuatsArrayPath)

     SIMPL_FILTER_PARAMETER(DataArrayPath, FeaturePhasesArrayPath)
     Q_PROPERTY(DataArrayPath FeaturePhasesArrayPath READ getFeaturePhasesArrayPath WRITE setFeaturePhasesArrayPath)

     SIMPL_FILTER_PARAMETER(DataArrayPath, CrystalStructuresArrayPath)
     Q_PROPERTY(DataArrayPath CrystalStructuresArrayPath READ getCrystalStructuresArrayPath WRITE setCrystalStructuresArrayPath)

     SIMPL_FILTER_PARAMETER(QString, SurfaceMeshFaceSchuhMisorientationColorsArrayName)
     Q_PROPERTY(QString SurfaceMeshFaceSchuhMisorientationColorsArrayName READ getSurfaceMeshFaceSchuhMisorientationColorsArrayName WRITE setSurfaceMeshFaceSchuhMisorientationColorsArrayName)

     /**
      * @brief getCompiledLibraryName Reimplemented from @see AbstractFilter class
      */
     const QString getCompiledLibraryName() const override;

     /**
      * @brief getBrandingString Returns the branding string for the filter, which is a tag
      * used to denote the filter's association with specific plugins
      * @return Branding string
      */
     const QString getBrandingString() const override;

     /**
      * @brief getFilterVersion Returns a version string for this filter. Default
      * value is an empty string.
      * @return
      */
     const QString getFilterVersion() const override;

     /**
      * @brief newFilterInstance Reimplemented from @see AbstractFilter class
      */
     AbstractFilter::Pointer newFilterInstance(bool copyFilterParameters) const override;

     /**
      * @brief getGroupName Reimplemented from @see AbstractFilter class
      */
     const QString getGroupName() const override;

     /**
      * @brief getSubGroupName Reimplemented from @see AbstractFilter class
      */
     const QString getSubGroupName() const override;

     /**
      * @brief getUuid Return the unique identifier for this filter.
      * @return A QUuid object.
      */
     const QUuid getUuid() override;

     /**
      * @brief getHumanLabel Reimplemented from @see AbstractFilter class
      */
     const QString getHumanLabel() const override;

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

     /**
      * @brief preflight Reimplemented from @see AbstractFilter class
      */
     void preflight() override;

   protected:
     GenerateFaceSchuhMisorientationColoring();

     /**
      * @brief dataCheckSurfaceMesh Checks for the appropriate parameter values and availability of arrays
      */
     void dataCheckSurfaceMesh();

     /**
      * @brief dataCheckVoxel Checks for the appropriate parameter values and availability of arrays
      */
     void dataCheckVoxel();

   private:
     DEFINE_DATAARRAY_VARIABLE(int32_t, SurfaceMeshFaceLabels)
     DEFINE_DATAARRAY_VARIABLE(uint8_t, SurfaceMeshFaceSchuhMisorientationColors)
     DEFINE_DATAARRAY_VARIABLE(float, AvgQuats)
     DEFINE_DATAARRAY_VARIABLE(int32_t, FeaturePhases)
     DEFINE_DATAARRAY_VARIABLE(unsigned int, CrystalStructures)

   public:
     GenerateFaceSchuhMisorientationColoring(const GenerateFaceSchuhMisorientationColoring&) = delete;            // Copy Constructor Not Implemented
     GenerateFaceSchuhMisorientationColoring(GenerateFaceSchuhMisorientationColoring&&) = delete;                 // Move Constructor
     GenerateFaceSchuhMisorientationColoring& operator=(const GenerateFaceSchuhMisorientationColoring&) = delete; // Copy Assignment Not Implemented
     GenerateFaceSchuhMisorientationColoring& operator=(GenerateFaceSchuhMisorientationColoring&&) = delete;      // Move Assignment
};

#endif /* _GenerateFaceSchuhMisorientationColoring_H_ */



