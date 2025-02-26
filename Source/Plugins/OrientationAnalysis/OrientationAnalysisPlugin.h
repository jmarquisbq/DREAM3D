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

#include <QtCore/QObject>
#include <QtCore/QSettings>

#include "SIMPLib/Plugin/ISIMPLibPlugin.h"

#include "OrientationAnalysis/OrientationAnalysisDLLExport.h"

/**
 * @class OrientationAnalysisPlugin OrientationAnalysisPlugin.hSurfaceMeshing/OrientationAnalysisPlugin.h
 * @brief
 *
 * @date May 10, 2012
 * @version 1.0
 */
class OrientationAnalysis_EXPORT OrientationAnalysisPlugin : public QObject, public ISIMPLibPlugin

{
  Q_OBJECT
  Q_INTERFACES(ISIMPLibPlugin)
  Q_PLUGIN_METADATA(IID "net.bluequartz.dream3d.OrientationAnalysisPlugin")

public:
  OrientationAnalysisPlugin();
  ~OrientationAnalysisPlugin() override;
  /**
   * @brief Returns the name of the plugin that appears on the file system.
   *
   * Note that if the build is a debug build there will be a _Plugin postfix
   * to the filename.
   */
  QString getPluginFileName() override;

  /**
   * @brief getPluginDisplayName The name that should be used for human facing
   * labels and display strings
   * @return
   */
  QString getPluginDisplayName() override;

  /**
   * @brief getPluginBaseName The Name of the plugin.
   *
   * This typically will NOT have the Plugin suffix.
   * @return
   */
  QString getPluginBaseName() override;

  /**
   * @brief Returns the version
   */
  QString getVersion() override;

  /**
   * @brief Returns the compatibility version
   */
  QString getCompatibilityVersion() override;

  /**
   * @brief Returns the name of the vendor
   */
  QString getVendor() override;

  /**
   * @brief Returns the URL of the plugin
   */
  QString getURL() override;

  /**
   * @brief Returns the location of the plugin
   */
  QString getLocation() override;

  /**
   * @brief Returns the description of the plugin
   */
  QString getDescription() override;

  /**
   * @brief Returns the copyright of the plugin
   */
  QString getCopyright() override;

  /**
   * @brief Returns the license of the plugin
   */
  QString getLicense() override;

  /**
   * @brief Returns the filters of the plugin
   */
  QList<QString> getFilters() override;

  /**
   * @brief Returns the third party licenses of the plugin
   */
  QMap<QString, QString> getThirdPartyLicenses() override;

  /**
   * @brief Returns the load status of the plugin
   */
  bool getDidLoad() override;

  /**
   * @brief Sets the load status of the plugin
   */
  void setDidLoad(bool didLoad) override;

  /**
   * @brief Sets the location of the plugin on the file system.
   * This is required so that we can cache the file path information
   * as the plugin is loaded.
   */
  void setLocation(QString filePath) override;

  /**
   * @brief Register all the filters with the FilterWidgetFactory
   */
  void registerFilterWidgets(FilterWidgetManager* fwm) override;

  /**
   * @brief This registers the filters that this plugin implements with the Filter Manager that is passed in
   * @param fm The FilterManager to register the filters into.
   */
  void registerFilters(FilterManager* fm) override;

  /**
   * @brief Writes the settings in the input gui to the Application's preference file
   * @param prefs A valid QSettings pointer.
   */
  void writeSettings(QSettings& prefs) override;

  /**
   * @brief Reads the settings from the Application's preference file and sets
   * the input GUI widgets accordingly.
   * @param prefs
   */
  void readSettings(QSettings& prefs) override;

private:
  QString m_Version;
  QString m_CompatibilityVersion;
  QString m_Vendor;
  QString m_URL;
  QString m_Location;
  QString m_Copyright;
  QList<QString> m_Filters;
  bool m_DidLoad;

public:
  OrientationAnalysisPlugin(const OrientationAnalysisPlugin&) = delete;            // Copy Constructor Not Implemented
  OrientationAnalysisPlugin(OrientationAnalysisPlugin&&) = delete;                 // Move Constructor Not Implemented
  OrientationAnalysisPlugin& operator=(const OrientationAnalysisPlugin&) = delete; // Copy Assignment Not Implemented
  OrientationAnalysisPlugin& operator=(OrientationAnalysisPlugin&&) = delete;      // Move Assignment Not Implemented
};
