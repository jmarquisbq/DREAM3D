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

#include "ui_StatsGenMDFWidget.h"

#include <QtWidgets/QWidget>

#include "SIMPLib/SIMPLib.h"
#include "SIMPLib/Common/Constants.h"
#include "SIMPLib/StatsData/PrecipitateStatsData.h"
#include "SIMPLib/StatsData/PrimaryStatsData.h"
#include "SIMPLib/StatsData/StatsData.h"
#include "SIMPLib/StatsData/TransformationStatsData.h"

#include "EbsdLib/Core/EbsdLibConstants.h"

#include "SyntheticBuilding/Gui/Widgets/TableModels/SGODFTableModel.h"

class SGMDFTableModel;
class QwtPlot;
class QwtPlotCurve;
class QwtPlotPicker;
class QwtPickerMachine;

/**
 * @class StatsGenMDFWidget StatsGenMDFWidget.h StatsGenerator/StatsGenMDFWidget.h
 * @brief This class gives GUIs an interface into the MDF settings
 *
 * @date Apr 20, 2011
 * @version 1.0
 */
class StatsGenMDFWidget : public QWidget, private Ui::StatsGenMDFWidget
{
  Q_OBJECT

public:
  StatsGenMDFWidget(QWidget* parent = nullptr);
  virtual ~StatsGenMDFWidget();

  void setupGui();

  /**
   * @brief initQwtPlot
   * @param xAxisName
   * @param yAxisName
   * @param plot
   */
  void initQwtPlot(const QString& xAxisName, const QString& yAxisName, QwtPlot* plot);

  /**
   * @brief Setter property for PhaseIndex
   */
  void setPhaseIndex(int value);
  /**
   * @brief Getter property for PhaseIndex
   * @return Value of PhaseIndex
   */
  int getPhaseIndex() const;

  /**
   * @brief Setter property for CrystalStructure
   */
  void setCrystalStructure(unsigned int value);
  /**
   * @brief Getter property for CrystalStructure
   * @return Value of CrystalStructure
   */
  unsigned int getCrystalStructure() const;

  /**
   * @brief Setter property for ODFTableModel
   */
  void setODFTableModel(SGODFTableModel* value);
  /**
   * @brief Getter property for ODFTableModel
   * @return Value of ODFTableModel
   */
  SGODFTableModel* getODFTableModel() const;

  int getMisorientationData(StatsData* statsData, PhaseType::Type phaseType, bool preflight = false);

  /**
   * @brief extractStatsData
   * @param index
   * @param statsData
   * @param phaseType
   */
  void extractStatsData(int index, StatsData* statsData, PhaseType::Type phaseType);

  /**
   * @brief tableModel
   * @return
   */
  SGMDFTableModel* tableModel();

public Q_SLOTS:
  void updatePlots();

protected Q_SLOTS:
  void on_addMDFRowBtn_clicked();
  void on_deleteMDFRowBtn_clicked();
  void on_m_MDFUpdateBtn_clicked();
  void on_loadMDFBtn_clicked();

  /**
   * @brief tableDataChanged
   * @param topLeft
   * @param bottomRight
   */
  void tableDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

Q_SIGNALS:

  void dataChanged();

protected:
  void updateMDFPlot(std::vector<float>& odf);

private:
  SGODFTableModel* m_ODFTableModel = nullptr;

  int m_PhaseIndex = {-1};
  unsigned int m_CrystalStructure = {EbsdLib::CrystalStructure::Cubic_High};

  SGMDFTableModel* m_MDFTableModel = nullptr;
  QwtPlotCurve* m_PlotCurve = nullptr;

  QwtPlotPicker* m_PlotPicker = nullptr;
  QwtPickerMachine* m_PlotPickerMachine = nullptr;

  QString m_OpenDialogLastFilePath; // Must be last in the list

public:
  StatsGenMDFWidget(const StatsGenMDFWidget&) = delete;            // Copy Constructor Not Implemented
  StatsGenMDFWidget(StatsGenMDFWidget&&) = delete;                 // Move Constructor Not Implemented
  StatsGenMDFWidget& operator=(const StatsGenMDFWidget&) = delete; // Copy Assignment Not Implemented
  StatsGenMDFWidget& operator=(StatsGenMDFWidget&&) = delete;      // Move Assignment Not Implemented
};
