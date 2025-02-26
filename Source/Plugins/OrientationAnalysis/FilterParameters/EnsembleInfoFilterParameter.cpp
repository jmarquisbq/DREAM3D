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

#include "EnsembleInfoFilterParameter.h"

#include <QtCore/QJsonArray>

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
EnsembleInfoFilterParameter::EnsembleInfoFilterParameter() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
EnsembleInfoFilterParameter::~EnsembleInfoFilterParameter() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
EnsembleInfoFilterParameter::Pointer EnsembleInfoFilterParameter::Create(const QString& humanLabel, const QString& propertyName, EnsembleInfo defaultValue, Category category,
                                                                         const SetterCallbackType& setterCallback, const GetterCallbackType& getterCallback, QVector<QString> choices,
                                                                         bool showOperators, const std::vector<int>& groupIndices)
{
  EnsembleInfoFilterParameter::Pointer ptr = EnsembleInfoFilterParameter::New();
  ptr->setHumanLabel(humanLabel);
  ptr->setPropertyName(propertyName);
  QVariant var;
  var.setValue(defaultValue);
  ptr->setDefaultValue(var);
  ptr->setCategory(category);
  ptr->setChoices(choices);
  ptr->setShowOperators(showOperators);
  ptr->setGroupIndices(groupIndices);
  ptr->setSetterCallback(setterCallback);
  ptr->setGetterCallback(getterCallback);

  return ptr;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString EnsembleInfoFilterParameter::getWidgetType() const
{
  return QString("EnsembleInfoCreationWidget");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void EnsembleInfoFilterParameter::readJson(const QJsonObject& json)
{
  QJsonValue jsonValue = json[getPropertyName()];
  if(jsonValue.isUndefined())
  {
    jsonValue = json[getLegacyPropertyName()];
  }
  if(!jsonValue.isUndefined() && m_SetterCallback)
  {
    QJsonArray jsonArray = jsonValue.toArray();

    EnsembleInfo inputs;
    for(int i = 0; i < jsonArray.size(); i++)
    {
      QJsonObject ensembleObj = jsonArray[i].toObject();

      if(ensembleObj["CrystalStructure"].isDouble() && ensembleObj["PhaseType"].isDouble() && ensembleObj["PhaseName"].isString())
      {
        EnsembleInfo::CrystalStructure crystalStructure = static_cast<EnsembleInfo::CrystalStructure>(ensembleObj["CrystalStructure"].toInt());
        PhaseType::Type phaseType = static_cast<PhaseType::Type>(ensembleObj["PhaseType"].toInt());
        QString phaseName = ensembleObj["PhaseName"].toString();

        inputs.addValues(static_cast<EnsembleInfo::CrystalStructure>(crystalStructure), static_cast<PhaseType::Type>(phaseType), phaseName);
      }
    }

    m_SetterCallback(inputs);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void EnsembleInfoFilterParameter::writeJson(QJsonObject& json) const
{
  if(m_GetterCallback)
  {
    QJsonArray inputsArray;

    EnsembleInfo inputs = m_GetterCallback();
    for(int i = 0; i < inputs.size(); i++)
    {
      EnsembleInfo::CrystalStructure crystalStructure;
      PhaseType::Type phaseType;
      QString phaseName;

      inputs.getValues(i, crystalStructure, phaseType, phaseName);

      QJsonObject obj;
      obj["CrystalStructure"] = static_cast<int>(crystalStructure);
      obj["PhaseType"] = static_cast<int>(phaseType);
      obj["PhaseName"] = phaseName;

      inputsArray.push_back(obj);
    }

    json[getPropertyName()] = inputsArray;
  }
}

// -----------------------------------------------------------------------------
EnsembleInfoFilterParameter::Pointer EnsembleInfoFilterParameter::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
EnsembleInfoFilterParameter::Pointer EnsembleInfoFilterParameter::New()
{
  Pointer sharedPtr(new(EnsembleInfoFilterParameter));
  return sharedPtr;
}

// -----------------------------------------------------------------------------
QString EnsembleInfoFilterParameter::getNameOfClass() const
{
  return QString("EnsembleInfoFilterParameter");
}

// -----------------------------------------------------------------------------
QString EnsembleInfoFilterParameter::ClassName()
{
  return QString("EnsembleInfoFilterParameter");
}

// -----------------------------------------------------------------------------
void EnsembleInfoFilterParameter::setChoices(const QVector<QString>& value)
{
  m_Choices = value;
}

// -----------------------------------------------------------------------------
QVector<QString> EnsembleInfoFilterParameter::getChoices() const
{
  return m_Choices;
}

// -----------------------------------------------------------------------------
void EnsembleInfoFilterParameter::setShowOperators(bool value)
{
  m_ShowOperators = value;
}

// -----------------------------------------------------------------------------
bool EnsembleInfoFilterParameter::getShowOperators() const
{
  return m_ShowOperators;
}

// -----------------------------------------------------------------------------
void EnsembleInfoFilterParameter::setDefaultGeometryTypes(const IGeometry::Types& value)
{
  m_DefaultGeometryTypes = value;
}

// -----------------------------------------------------------------------------
IGeometry::Types EnsembleInfoFilterParameter::getDefaultGeometryTypes() const
{
  return m_DefaultGeometryTypes;
}

// -----------------------------------------------------------------------------
void EnsembleInfoFilterParameter::setDefaultAttributeMatrixTypes(const AttributeMatrix::Types& value)
{
  m_DefaultAttributeMatrixTypes = value;
}

// -----------------------------------------------------------------------------
AttributeMatrix::Types EnsembleInfoFilterParameter::getDefaultAttributeMatrixTypes() const
{
  return m_DefaultAttributeMatrixTypes;
}

// -----------------------------------------------------------------------------
void EnsembleInfoFilterParameter::setSetterCallback(const EnsembleInfoFilterParameter::SetterCallbackType& value)
{
  m_SetterCallback = value;
}

// -----------------------------------------------------------------------------
EnsembleInfoFilterParameter::SetterCallbackType EnsembleInfoFilterParameter::getSetterCallback() const
{
  return m_SetterCallback;
}

// -----------------------------------------------------------------------------
void EnsembleInfoFilterParameter::setGetterCallback(const EnsembleInfoFilterParameter::GetterCallbackType& value)
{
  m_GetterCallback = value;
}

// -----------------------------------------------------------------------------
EnsembleInfoFilterParameter::GetterCallbackType EnsembleInfoFilterParameter::getGetterCallback() const
{
  return m_GetterCallback;
}
