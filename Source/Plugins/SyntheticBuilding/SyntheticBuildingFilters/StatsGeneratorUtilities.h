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
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <vector>

#include "SIMPLib/Common/PhaseType.h"

#include "SyntheticBuilding/SyntheticBuildingDLLExport.h"

class StatsData;

class SyntheticBuilding_EXPORT StatsGeneratorUtilities
{
public:
  virtual ~StatsGeneratorUtilities();

  /**
   * @brief GenerateODFBinData
   * @param statsData
   * @param phaseType
   * @param e1s
   * @param e2s
   * @param e3s
   * @param weights
   * @param sigmas
   */
  static void GenerateODFBinData(StatsData* statsData, PhaseType::Type phaseType, unsigned int crystalStructure, std::vector<float>& e1s, std::vector<float>& e2s, std::vector<float>& e3s,
                                 std::vector<float>& weights, std::vector<float>& sigmas, bool computeODF = true);

  static void GenerateAxisODFBinData(StatsData* statsData, PhaseType::Type phaseType, std::vector<float>& e1s, std::vector<float>& e2s, std::vector<float>& e3s, std::vector<float>& weights,
                                     std::vector<float>& sigmas, bool computeAxisODF = true);

  static std::vector<float> GenerateODFData(unsigned int crystalStructure, std::vector<float>& e1s, std::vector<float>& e2s, std::vector<float>& e3s, std::vector<float>& weights,
                                            std::vector<float>& sigmas, bool computeODF = true);

  static void GenerateMisorientationBinData(StatsData* statsData, PhaseType::Type phaseType, unsigned int crystalStruct, std::vector<float>& odf, std::vector<float>& angles, std::vector<float>& axes,
                                            std::vector<float>& weights, bool computeMDF = true);

protected:
  StatsGeneratorUtilities();

public:
  StatsGeneratorUtilities(const StatsGeneratorUtilities&) = delete;            // Copy Constructor Not Implemented
  StatsGeneratorUtilities(StatsGeneratorUtilities&&) = delete;                 // Move Constructor Not Implemented
  StatsGeneratorUtilities& operator=(const StatsGeneratorUtilities&) = delete; // Copy Assignment Not Implemented
  StatsGeneratorUtilities& operator=(StatsGeneratorUtilities&&) = delete;      // Move Assignment Not Implemented
};
