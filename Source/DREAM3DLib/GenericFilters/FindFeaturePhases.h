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
#ifndef _FindFeaturePhases_H_
#define _FindFeaturePhases_H_

#include <vector>
#include <QtCore/QString>

#include "DREAM3DLib/DREAM3DLib.h"
#include "DREAM3DLib/Common/DREAM3DSetGetMacros.h"
#include "DREAM3DLib/DataArrays/IDataArray.h"
#include "DREAM3DLib/Common/Constants.h"

#include "DREAM3DLib/Common/AbstractFilter.h"
#include "DREAM3DLib/DataContainers/VolumeDataContainer.h"

/**
 *
 */
class DREAM3DLib_EXPORT FindFeaturePhases : public AbstractFilter
{
    Q_OBJECT /* Need this for Qt's signals and slots mechanism to work */
  public:
    DREAM3D_SHARED_POINTERS(FindFeaturePhases)
    DREAM3D_STATIC_NEW_MACRO(FindFeaturePhases)
    DREAM3D_TYPE_MACRO_SUPER(FindFeaturePhases, AbstractFilter)

    virtual ~FindFeaturePhases();
    DREAM3D_FILTER_PARAMETER(DataArrayPath, CellFeatureAttributeMatrixName)
    Q_PROPERTY(DataArrayPath CellFeatureAttributeMatrixName READ getCellFeatureAttributeMatrixName WRITE setCellFeatureAttributeMatrixName)

    DREAM3D_FILTER_PARAMETER(DataArrayPath, FeatureIdsArrayPath)
    Q_PROPERTY(DataArrayPath FeatureIdsArrayPath READ getFeatureIdsArrayPath WRITE setFeatureIdsArrayPath)

    DREAM3D_FILTER_PARAMETER(DataArrayPath, CellPhasesArrayPath)
    Q_PROPERTY(DataArrayPath CellPhasesArrayPath READ getCellPhasesArrayPath WRITE setCellPhasesArrayPath)

    DREAM3D_FILTER_PARAMETER(QString, FeaturePhasesArrayName)
    Q_PROPERTY(QString FeaturePhasesArrayName READ getFeaturePhasesArrayName WRITE setFeaturePhasesArrayName)

    virtual const QString getCompiledLibraryName() { return Generic::GenericBaseName; }
    virtual AbstractFilter::Pointer newFilterInstance(bool copyFilterParameters);
    virtual const QString getGroupName() { return DREAM3D::FilterGroups::GenericFilters; }
    virtual const QString getSubGroupName() { return DREAM3D::FilterSubGroups::MiscFilters; }
    virtual const QString getHumanLabel() { return "Find Feature Phases"; }

    /**
    * @brief This method will instantiate all the end user settable options/parameters
    * for this filter
    */
    virtual void setupFilterParameters();

    /**
    * @brief This method will write the options to a file
    * @param writer The writer that is used to write the options to a file
    */
    virtual int writeFilterParameters(AbstractFilterParametersWriter* writer, int index);

    /**
    * @brief This method will read the options from a file
    * @param reader The reader that is used to read the options from a file
    */
    virtual void readFilterParameters(AbstractFilterParametersReader* reader, int index);

    virtual void execute();
    virtual void preflight();

  signals:
    void updateFilterParameters(AbstractFilter* filter);
    void parametersChanged();
    void preflightAboutToExecute();
    void preflightExecuted();

  protected:
    FindFeaturePhases();

  private:
    DEFINE_REQUIRED_DATAARRAY_VARIABLE(int32_t, FeatureIds)
    DEFINE_REQUIRED_DATAARRAY_VARIABLE(int32_t, CellPhases)
    DEFINE_CREATED_DATAARRAY_VARIABLE(int32_t, FeaturePhases)

    void dataCheck();

    FindFeaturePhases(const FindFeaturePhases&); // Copy Constructor Not Implemented
    void operator=(const FindFeaturePhases&); // Operator '=' Not Implemented

};

#endif /* FindFeaturePhases_H_ */



