/*
 * Your License or Copyright can go here
 */
#include "GeneratePrimaryStatsData.h"

#include <QtCore/QTextStream>

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/Texture/StatsGen.hpp"
#include "SIMPLib/Common/Constants.h"
#include "SIMPLib/DataArrays/DataArray.hpp"
#include "SIMPLib/DataArrays/StatsDataArray.h"
#include "SIMPLib/DataArrays/StringDataArray.h"
#include "SIMPLib/DataContainers/DataContainer.h"
#include "SIMPLib/DataContainers/DataContainerArray.h"
#include "SIMPLib/FilterParameters/AttributeMatrixCreationFilterParameter.h"
#include "SIMPLib/FilterParameters/AttributeMatrixSelectionFilterParameter.h"
#include "SIMPLib/FilterParameters/BooleanFilterParameter.h"
#include "SIMPLib/FilterParameters/ChoiceFilterParameter.h"
#include "SIMPLib/FilterParameters/DataContainerCreationFilterParameter.h"
#include "SIMPLib/FilterParameters/DoubleFilterParameter.h"
#include "SIMPLib/FilterParameters/DynamicTableFilterParameter.h"
#include "SIMPLib/FilterParameters/IntFilterParameter.h"
#include "SIMPLib/FilterParameters/LinkedBooleanFilterParameter.h"
#include "SIMPLib/FilterParameters/LinkedPathCreationFilterParameter.h"
#include "SIMPLib/FilterParameters/PreflightUpdatedValueFilterParameter.h"
#include "SIMPLib/FilterParameters/StringFilterParameter.h"
#include "SIMPLib/Math/MatrixMath.h"
#include "SIMPLib/Math/SIMPLibMath.h"
#include "SIMPLib/Math/SIMPLibRandom.h"
#include "SIMPLib/StatsData/PrimaryStatsData.h"
#include "SIMPLib/Utilities/ColorUtilities.h"

#include "SyntheticBuilding/SyntheticBuildingConstants.h"
#include "SyntheticBuilding/SyntheticBuildingFilters/Presets/AbstractMicrostructurePreset.h"
#include "SyntheticBuilding/SyntheticBuildingFilters/Presets/PrimaryEquiaxedPreset.h"
#include "SyntheticBuilding/SyntheticBuildingFilters/Presets/PrimaryRecrystallizedPreset.h"
#include "SyntheticBuilding/SyntheticBuildingFilters/Presets/PrimaryRolledPreset.h"
#include "SyntheticBuilding/SyntheticBuildingFilters/StatsGeneratorUtilities.h"
#include "SyntheticBuilding/SyntheticBuildingVersion.h"

/* Create Enumerations to allow the created Attribute Arrays to take part in renaming */
enum createdPathID : RenameDataPath::DataID_t
{
  AttributeMatrixID20 = 20,
  AttributeMatrixID21 = 21,
};

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
GeneratePrimaryStatsData::GeneratePrimaryStatsData()
{
  initialize();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
GeneratePrimaryStatsData::~GeneratePrimaryStatsData() = default;

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::initialize()
{
  clearErrorCode();
  clearWarningCode();
  setCancel(false);
  m_StatsDataArray = nullptr;
  m_PrimaryStatsData = nullptr;
  m_CrystalStructures = nullptr;
  m_PhaseTypes = nullptr;
  m_PhaseNames = nullptr;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setupFilterParameters()
{
  FilterParameterVectorType parameters;
  parameters.push_back(SIMPL_NEW_STRING_FP("Phase Name", PhaseName, FilterParameter::Category::Parameter, GeneratePrimaryStatsData));
  {
    ChoiceFilterParameter::Pointer parameter = ChoiceFilterParameter::New();
    parameter->setHumanLabel("Crystal Symmetry");
    parameter->setPropertyName("CrystalSymmetry");

    std::vector<QString> choices; // Please add choices to the choices QVector to finish this widget
    choices.push_back("Hexagonal (High)");
    choices.push_back("Cubic (High)");
    choices.push_back("Hexagonal (Low)");
    choices.push_back("Cubic (Low)");
    choices.push_back("Triclinic");
    choices.push_back("Monoclinic");
    choices.push_back("OrthoRhombic");
    choices.push_back("Tetragonal (Low)");
    choices.push_back("Tetragonal (High)");
    choices.push_back("Trigonal (Low)");
    choices.push_back("Trigonal (High)");

    parameter->setChoices(choices);
    parameter->setCategory(FilterParameter::Category::Parameter);
    parameter->setSetterCallback(SIMPL_BIND_SETTER(GeneratePrimaryStatsData, this, CrystalSymmetry));
    parameter->setGetterCallback(SIMPL_BIND_GETTER(GeneratePrimaryStatsData, this, CrystalSymmetry));
    parameters.push_back(parameter);
  }

  {
    ChoiceFilterParameter::Pointer parameter = ChoiceFilterParameter::New();
    parameter->setHumanLabel("Microstructure Preset Model");
    parameter->setPropertyName("MicroPresetModel");

    std::vector<QString> choices; // Please add choices to the choices QVector to finish this widget
    choices.push_back("Primary Equiaxed");
    choices.push_back("Primary Rolled");
    choices.push_back("Primary Recrystallized");

    parameter->setChoices(choices);
    parameter->setCategory(FilterParameter::Category::Parameter);
    parameter->setSetterCallback(SIMPL_BIND_SETTER(GeneratePrimaryStatsData, this, MicroPresetModel));
    parameter->setGetterCallback(SIMPL_BIND_GETTER(GeneratePrimaryStatsData, this, MicroPresetModel));
    parameters.push_back(parameter);
  }

  parameters.push_back(SIMPL_NEW_DOUBLE_FP("Phase Fraction", PhaseFraction, FilterParameter::Category::Parameter, GeneratePrimaryStatsData));
  parameters.push_back(SIMPL_NEW_DOUBLE_FP("Mu", Mu, FilterParameter::Category::Parameter, GeneratePrimaryStatsData));

  parameters.push_back(SIMPL_NEW_DOUBLE_FP("Sigma", Sigma, FilterParameter::Category::Parameter, GeneratePrimaryStatsData));
  parameters.push_back(SIMPL_NEW_DOUBLE_FP("Min.Cut Off", MinCutOff, FilterParameter::Category::Parameter, GeneratePrimaryStatsData));
  parameters.push_back(SIMPL_NEW_DOUBLE_FP("Max Cut Off", MaxCutOff, FilterParameter::Category::Parameter, GeneratePrimaryStatsData));
  parameters.push_back(SIMPL_NEW_DOUBLE_FP("Bin Step Size", BinStepSize, FilterParameter::Category::Parameter, GeneratePrimaryStatsData));

  //---------------------------
  PreflightUpdatedValueFilterParameter::Pointer param = SIMPL_NEW_PREFLIGHTUPDATEDVALUE_FP("Bins Created:", NumberOfBins, FilterParameter::Category::Parameter, GeneratePrimaryStatsData);
  param->setReadOnly(true);
  parameters.push_back(param);

  PreflightUpdatedValueFilterParameter::Pointer param2 = SIMPL_NEW_PREFLIGHTUPDATEDVALUE_FP("Feature ESD:", FeatureESD, FilterParameter::Category::Parameter, GeneratePrimaryStatsData);
  param2->setReadOnly(true);
  parameters.push_back(param2);
  //---------------------------

  // Table 3 - Dynamic rows and fixed columns
  {
    QStringList cHeaders;
    cHeaders << "Euler 1"
             << "Euler 2"
             << "Euler 3"
             << "Weight"
             << "Sigma";
    std::vector<std::vector<double>> defaultTable(1, std::vector<double>(5, 0.0));
    m_OdfData.setColHeaders(cHeaders);
    m_OdfData.setTableData(defaultTable);
    m_OdfData.setDynamicRows(true);
    parameters.push_back(SIMPL_NEW_DYN_TABLE_FP("ODF", OdfData, FilterParameter::Category::Parameter, GeneratePrimaryStatsData, {false}));
  }
  {
    QStringList cHeaders;
    cHeaders << "Angle(w)"
             << "Axis (h)"
             << "Axis (k)"
             << "Axis (l)"
             << "Weight (MRD)";
    std::vector<std::vector<double>> defaultTable(1, std::vector<double>(5, 0.0));
    m_MdfData.setColHeaders(cHeaders);
    m_MdfData.setTableData(defaultTable);
    m_MdfData.setDynamicRows(true);
    parameters.push_back(SIMPL_NEW_DYN_TABLE_FP("MDF", MdfData, FilterParameter::Category::Parameter, GeneratePrimaryStatsData, {false}));
  }
  {
    QStringList cHeaders;
    cHeaders << "Euler 1"
             << "Euler 2"
             << "Euler 3"
             << "Weight"
             << "Sigma";
    std::vector<std::vector<double>> defaultTable(1, std::vector<double>(5, 0.0));
    m_AxisOdfData.setColHeaders(cHeaders);
    m_AxisOdfData.setTableData(defaultTable);
    m_AxisOdfData.setDynamicRows(true);
    parameters.push_back(SIMPL_NEW_DYN_TABLE_FP("Axis ODF", AxisOdfData, FilterParameter::Category::Parameter, GeneratePrimaryStatsData, {false}));
  }

  std::vector<QString> linkedProps = {"DataContainerName"};
  linkedProps.push_back("CellEnsembleAttributeMatrixName");
  parameters.push_back(
      SIMPL_NEW_LINKED_BOOL_FP("Create Data Container & Ensemble AttributeMatrix", CreateEnsembleAttributeMatrix, FilterParameter::Category::Parameter, GeneratePrimaryStatsData, linkedProps));

  parameters.push_back(SIMPL_NEW_DC_CREATION_FP("Data Container", DataContainerName, FilterParameter::Category::CreatedArray, GeneratePrimaryStatsData));
  parameters.push_back(
      SIMPL_NEW_AM_WITH_LINKED_DC_FP("Cell Ensemble Attribute Matrix", CellEnsembleAttributeMatrixName, DataContainerName, FilterParameter::Category::CreatedArray, GeneratePrimaryStatsData));

  linkedProps.clear();
  linkedProps.push_back("SelectedEnsembleAttributeMatrix");
  parameters.push_back(SIMPL_NEW_LINKED_BOOL_FP("Append To Existing AttributeMatrix", AppendToExistingAttributeMatrix, FilterParameter::Category::Parameter, GeneratePrimaryStatsData, linkedProps));
  AttributeMatrixSelectionFilterParameter::RequirementType amReq2;
  parameters.push_back(SIMPL_NEW_AM_SELECTION_FP("Selected Ensemble AttributeMatrix", SelectedEnsembleAttributeMatrix, FilterParameter::Category::Parameter, GeneratePrimaryStatsData, amReq2));
  setFilterParameters(parameters);
}

#define FLOAT_RANGE_CHECK(var, min, max, error)                                                                                                                                                        \
  if(m_##var < min || m_##var > max)                                                                                                                                                                   \
  {                                                                                                                                                                                                    \
    setErrorCondition(error, "Valid range for " #var " is " #min "~" #max);                                                                                                                            \
  }

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::dataCheck()
{
  clearErrorCode();
  clearWarningCode();

  initialize();

  FLOAT_RANGE_CHECK(Mu, -10.0, 10.0, -95000);
  FLOAT_RANGE_CHECK(Sigma, 0.0, 10.0, -95001);
  FLOAT_RANGE_CHECK(MinCutOff, 0, 1000000, -95002)
  FLOAT_RANGE_CHECK(MaxCutOff, 0, 1000000, -95003)

  if((m_CreateEnsembleAttributeMatrix && m_AppendToExistingAttributeMatrix) || (!m_CreateEnsembleAttributeMatrix && !m_AppendToExistingAttributeMatrix))
  {
    setErrorCondition(-95010, "CreateEnsembleAttributeMatrix & AppendToExistingAttributeMatrix can NOT both be true or false. One must be true and one must be false.");
    return;
  }

  // Use has asked to create a whole new EnsembleAttributeMatrix
  if(m_CreateEnsembleAttributeMatrix && !m_AppendToExistingAttributeMatrix)
  {
    DataContainerArray::Pointer dca = getDataContainerArray();
    DataContainer::Pointer dc = dca->createNonPrereqDataContainer(this, getDataContainerName());
    if(getErrorCode() < 0)
    {
      return;
    }

    std::vector<size_t> tDims(1, 2); // we need 2 slots in the array. ZERO=Junk, 1 = our new primary stats data
    AttributeMatrix::Pointer cellEnsembleAttrMat = dc->createNonPrereqAttributeMatrix(this, getCellEnsembleAttributeMatrixName(), tDims, AttributeMatrix::Type::CellEnsemble, AttributeMatrixID21);
    if(getErrorCode() < 0)
    {
      return;
    }
    StatsDataArray::Pointer statsDataArray = StatsDataArray::New();
    statsDataArray->resizeTuples(tDims[0]);
    cellEnsembleAttrMat->insertOrAssign(statsDataArray);
    m_StatsDataArray = statsDataArray.get();

    PrimaryStatsData::Pointer primaryStatsData = PrimaryStatsData::New();
    statsDataArray->setStatsData(1, primaryStatsData);
    m_PrimaryStatsData = primaryStatsData.get();

    std::vector<size_t> cDims(1, 1);
    UInt32ArrayType::Pointer crystalStructures = UInt32ArrayType::CreateArray(tDims, cDims, SIMPL::EnsembleData::CrystalStructures, true);
    crystalStructures->setValue(0, EbsdLib::CrystalStructure::UnknownCrystalStructure);
    cellEnsembleAttrMat->insertOrAssign(crystalStructures);
    m_CrystalStructures = crystalStructures.get();

    UInt32ArrayType::Pointer phaseTypes = UInt32ArrayType::CreateArray(tDims, cDims, SIMPL::EnsembleData::PhaseTypes, true);
    phaseTypes->setValue(0, static_cast<PhaseType::EnumType>(PhaseType::Type::Unknown));
    cellEnsembleAttrMat->insertOrAssign(phaseTypes);
    m_PhaseTypes = phaseTypes.get();

    StringDataArray::Pointer phaseNames = StringDataArray::CreateArray(tDims[0], SIMPL::EnsembleData::PhaseName, true);
    phaseNames->setValue(0, PhaseType::getPhaseTypeString(PhaseType::Type::Unknown));
    cellEnsembleAttrMat->insertOrAssign(phaseNames);
    m_PhaseNames = phaseNames.get();

    setPhaseIndex(1); // If we are creating the StatsDataArray then we are the first phase
    if(!getInPreflight())
    {
      phaseNames->setValue(m_PhaseIndex, m_PhaseName);
    }
  }

  // User wants to Append to existing AttributeMatrix
  if(!m_CreateEnsembleAttributeMatrix && m_AppendToExistingAttributeMatrix)
  {
    DataContainerArray::Pointer dca = getDataContainerArray();
    AttributeMatrix::Pointer cellEnsembleAttrMat = dca->getAttributeMatrix(m_SelectedEnsembleAttributeMatrix);
    if(nullptr == cellEnsembleAttrMat.get())
    {
      setErrorCondition(-95020, QString("AttributeMatrix does not exist at path %1").arg(m_SelectedEnsembleAttributeMatrix.serialize("/")));
      return;
    }

    // Resize the AttributeMatrix, which should resize all the AttributeArrays
    std::vector<size_t> tDims(1, cellEnsembleAttrMat->getNumberOfTuples() + 1);
    cellEnsembleAttrMat->resizeAttributeArrays(tDims);

    StatsDataArray::Pointer statsDataArray = cellEnsembleAttrMat->getAttributeArrayAs<StatsDataArray>(SIMPL::EnsembleData::Statistics);
    if(nullptr == statsDataArray.get())
    {
      statsDataArray = StatsDataArray::New();
      statsDataArray->resizeTuples(tDims[0]);
      cellEnsembleAttrMat->insertOrAssign(statsDataArray);
    }
    m_StatsDataArray = statsDataArray.get();

    PrimaryStatsData::Pointer primaryStatsData = PrimaryStatsData::New();
    statsDataArray->setStatsData(tDims[0] - 1, primaryStatsData);
    m_PrimaryStatsData = primaryStatsData.get();

    std::vector<size_t> cDims(1, 1);

    UInt32ArrayType::Pointer crystalStructures = cellEnsembleAttrMat->getAttributeArrayAs<UInt32ArrayType>(SIMPL::EnsembleData::CrystalStructures);
    if(nullptr == crystalStructures.get())
    {
      crystalStructures = UInt32ArrayType::CreateArray(tDims, cDims, SIMPL::EnsembleData::CrystalStructures, true);
      crystalStructures->setValue(0, EbsdLib::CrystalStructure::UnknownCrystalStructure);
      cellEnsembleAttrMat->insertOrAssign(crystalStructures);
    }
    m_CrystalStructures = crystalStructures.get();

    UInt32ArrayType::Pointer phaseTypes = cellEnsembleAttrMat->getAttributeArrayAs<UInt32ArrayType>(SIMPL::EnsembleData::PhaseTypes);
    if(nullptr == phaseTypes.get())
    {
      phaseTypes = UInt32ArrayType::CreateArray(tDims, cDims, SIMPL::EnsembleData::PhaseTypes, true);
      phaseTypes->setValue(0, static_cast<PhaseType::EnumType>(PhaseType::Type::Unknown));
      cellEnsembleAttrMat->insertOrAssign(phaseTypes);
    }
    m_PhaseTypes = phaseTypes.get();

    StringDataArray::Pointer phaseNames = cellEnsembleAttrMat->getAttributeArrayAs<StringDataArray>(SIMPL::EnsembleData::PhaseName);
    if(nullptr == phaseNames.get())
    {
      phaseNames = StringDataArray::CreateArray(tDims[0], SIMPL::EnsembleData::PhaseName, true);
      phaseNames->setValue(0, PhaseType::getPhaseTypeString(PhaseType::Type::Unknown));
      cellEnsembleAttrMat->insertOrAssign(phaseNames);
    }
    m_PhaseNames = phaseNames.get();

    setPhaseIndex(tDims[0] - 1); // If we are creating the StatsDataArray then we are the first phase
    if(!getInPreflight())
    {
      phaseNames->setValue(m_PhaseIndex, m_PhaseName);
    }
  }

  // If all goes well, then calculate the number of bins to display back on the user interface.
  float max, min; // Only needed for the method. Not used otherwise.
  int numBins = StatsGen::ComputeNumberOfBins(m_Mu, m_Sigma, m_MinCutOff, m_MaxCutOff, m_BinStepSize, max, min);
  m_NumberOfBins = QString::number(numBins, 10);
  // Calculate a more understandable "Grain Size" for the user
  float esd = std::exp(m_Mu + (m_Sigma * m_Sigma) / 2.0);
  QLocale loc = QLocale::system();
  m_FeatureESD = loc.toString(esd);
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::execute()
{
  initialize();
  dataCheck();
  if(getErrorCode() < 0)
  {
    return;
  }

  if(getCancel())
  {
    return;
  }

  typedef QVector<float> FloatVectorType;
  int err = 0;
  int size = 250;
  FloatVectorType x;
  FloatVectorType y;
  err = StatsGen::GenLogNormalPlotData<FloatVectorType>(m_Mu, m_Sigma, x, y, size, m_MinCutOff, m_MaxCutOff);
  if(err == 1)
  {
    setErrorCondition(-95011, "Error generating the LogNormal Data");
    return;
  }
  float yMax = 0.0f;
  // float xMax = x[size-1];
  for(int i = 0; i < size; ++i)
  {
    if(y[i] > yMax)
    {
      yMax = y[i];
    }
  }

  FloatVectorType xCo;
  FloatVectorType yCo;
  int numsizebins = 1;
  FloatVectorType binSizes;
  err = StatsGen::GenCutOff<float, FloatVectorType>(m_Mu, m_Sigma, m_MinCutOff, m_MaxCutOff, m_BinStepSize, xCo, yCo, yMax, numsizebins, binSizes);
  if(err == 1)
  {
    setErrorCondition(-95012, "Error generating the Min or Max Cut Off values");
    return;
  }

  QMap<QString, QVector<float>> dataMap;
  dataMap[AbstractMicrostructurePreset::kBinNumbers] = binSizes;

  AbstractMicrostructurePreset::Pointer absPresetPtr;
  if(m_MicroPresetModel == 0)
  {
    absPresetPtr = PrimaryEquiaxedPreset::New();
  }
  else if(m_MicroPresetModel == 1)
  {
    absPresetPtr = PrimaryRolledPreset::New();
  }
  else if(m_MicroPresetModel == 2)
  {
    absPresetPtr = PrimaryRecrystallizedPreset::New();
  }

  m_CrystalStructures->setComponent(m_PhaseIndex, 0, m_CrystalSymmetry);
  m_PhaseNames->setValue(m_PhaseIndex, m_PhaseName);
  m_PhaseTypes->setValue(m_PhaseIndex, static_cast<PhaseType::EnumType>(PhaseType::Type::Primary));
  m_PrimaryStatsData->setName(m_PhaseName);
  m_PrimaryStatsData->setPhaseFraction(m_PhaseFraction);

  normalizePhaseFractions(m_StatsDataArray);

  // Feature Diameter Info
  m_PrimaryStatsData->setBinStepSize(m_BinStepSize);
  m_PrimaryStatsData->setMaxFeatureDiameter(xCo[1]);
  m_PrimaryStatsData->setMinFeatureDiameter(xCo[0]);
  // Feature Size Distribution
  {
    VectorOfFloatArray data;
    FloatArrayType::Pointer d1 = FloatArrayType::CreateArray(1, SIMPL::StringConstants::Average, true);
    FloatArrayType::Pointer d2 = FloatArrayType::CreateArray(1, SIMPL::StringConstants::StandardDeviation, true);
    data.push_back(d1);
    data.push_back(d2);
    d1->setValue(0, m_Mu);
    d2->setValue(0, m_Sigma);
    m_PrimaryStatsData->setFeatureSizeDistribution(data);
    m_PrimaryStatsData->setFeatureSize_DistType(SIMPL::DistributionType::LogNormal);
    m_PrimaryStatsData->generateBinNumbers();
  }

  {
    absPresetPtr->initializeOmega3TableModel(dataMap); // Beta
    VectorOfFloatArray data;
    FloatArrayType::Pointer d1 = FloatArrayType::FromQVector(dataMap[AbstractMicrostructurePreset::kAlpha], SIMPL::StringConstants::Alpha);
    FloatArrayType::Pointer d2 = FloatArrayType::FromQVector(dataMap[AbstractMicrostructurePreset::kBeta], SIMPL::StringConstants::Beta);
    data.push_back(d1);
    data.push_back(d2);
    m_PrimaryStatsData->setFeatureSize_Omegas(data);
    m_PrimaryStatsData->setOmegas_DistType(absPresetPtr->getDistributionType(AbstractMicrostructurePreset::kOmega3Distribution));
  }

  {
    absPresetPtr->initializeBOverATableModel(dataMap); // Beta
    VectorOfFloatArray data;
    FloatArrayType::Pointer d1 = FloatArrayType::FromQVector(dataMap[AbstractMicrostructurePreset::kAlpha], SIMPL::StringConstants::Alpha);
    FloatArrayType::Pointer d2 = FloatArrayType::FromQVector(dataMap[AbstractMicrostructurePreset::kBeta], SIMPL::StringConstants::Beta);
    data.push_back(d1);
    data.push_back(d2);
    m_PrimaryStatsData->setFeatureSize_BOverA(data);
    m_PrimaryStatsData->setBOverA_DistType(absPresetPtr->getDistributionType(AbstractMicrostructurePreset::kBOverADistribution));
  }

  {
    absPresetPtr->initializeCOverATableModel(dataMap); // Beta
    VectorOfFloatArray data;
    FloatArrayType::Pointer d1 = FloatArrayType::FromQVector(dataMap[AbstractMicrostructurePreset::kAlpha], SIMPL::StringConstants::Alpha);
    FloatArrayType::Pointer d2 = FloatArrayType::FromQVector(dataMap[AbstractMicrostructurePreset::kBeta], SIMPL::StringConstants::Beta);
    data.push_back(d1);
    data.push_back(d2);
    m_PrimaryStatsData->setFeatureSize_COverA(data);
    m_PrimaryStatsData->setBOverA_DistType(absPresetPtr->getDistributionType(AbstractMicrostructurePreset::kCOverADistribution));
  }

  {
    absPresetPtr->initializeNeighborTableModel(dataMap); // LogNormal

    VectorOfFloatArray data;
    FloatArrayType::Pointer d1 = FloatArrayType::FromQVector(dataMap[AbstractMicrostructurePreset::kMu], SIMPL::StringConstants::Average);
    FloatArrayType::Pointer d2 = FloatArrayType::FromQVector(dataMap[AbstractMicrostructurePreset::kSigma], SIMPL::StringConstants::StandardDeviation);
    data.push_back(d1);
    data.push_back(d2);
    m_PrimaryStatsData->setFeatureSize_Neighbors(data);
    m_PrimaryStatsData->setBOverA_DistType(absPresetPtr->getDistributionType(AbstractMicrostructurePreset::kNeighborDistribution));
  }

  {
    absPresetPtr->initializeODFTableModel(dataMap);

    std::vector<float> e1s;
    std::vector<float> e2s;
    std::vector<float> e3s;
    std::vector<float> weights;
    std::vector<float> sigmas;

    std::vector<std::vector<double>> odfData = m_OdfData.getTableData();
    for(size_t i = 0; i < odfData.size(); i++)
    {
      e1s.push_back(odfData[i][0] * SIMPLib::Constants::k_PiOver180D);
      e2s.push_back(odfData[i][1] * SIMPLib::Constants::k_PiOver180D);
      e3s.push_back(odfData[i][2] * SIMPLib::Constants::k_PiOver180D);
      weights.push_back(odfData[i][3]);
      sigmas.push_back(odfData[i][4]);
    }
    // Convert angles to Radians when this is implemented
    StatsGeneratorUtilities::GenerateODFBinData(m_PrimaryStatsData, PhaseType::Type::Primary, m_CrystalSymmetry, e1s, e2s, e3s, weights, sigmas, true);
  }

  {
    absPresetPtr->initializeMDFTableModel(dataMap);
    std::vector<float> e1s;
    std::vector<float> e2s;
    std::vector<float> e3s;
    std::vector<float> odf_weights;
    std::vector<float> sigmas;
    std::vector<std::vector<double>> odfData = m_OdfData.getTableData();
    for(size_t i = 0; i < odfData.size(); i++)
    {
      e1s.push_back(odfData[i][0] * static_cast<float>(SIMPLib::Constants::k_PiOver180D));
      e2s.push_back(odfData[i][1] * static_cast<float>(SIMPLib::Constants::k_PiOver180D));
      e3s.push_back(odfData[i][2] * static_cast<float>(SIMPLib::Constants::k_PiOver180D));
      odf_weights.push_back(odfData[i][3]);
      sigmas.push_back(odfData[i][4]);
    }

    std::vector<float> odf = StatsGeneratorUtilities::GenerateODFData(m_CrystalSymmetry, e1s, e2s, e3s, odf_weights, sigmas, true);

    std::vector<float> angles;
    std::vector<float> axes;
    std::vector<float> weights;
    std::vector<std::vector<double>> mdfData = m_MdfData.getTableData();
    for(size_t i = 0; i < mdfData.size(); i++)
    {
      angles.push_back(mdfData[i][0]);
      axes.push_back(mdfData[i][1]);
      axes.push_back(mdfData[i][2]);
      axes.push_back(mdfData[i][3]);
      weights.push_back(mdfData[i][4]);
    }

    StatsGeneratorUtilities::GenerateMisorientationBinData(m_PrimaryStatsData, PhaseType::Type::Primary, m_CrystalSymmetry, odf, angles, axes, weights, true);
  }

  {
    absPresetPtr->initializeAxisODFTableModel(dataMap);
    std::vector<float> e1s;
    std::vector<float> e2s;
    std::vector<float> e3s;
    std::vector<float> weights;
    std::vector<float> sigmas;
    std::vector<std::vector<double>> axisOdfData = m_AxisOdfData.getTableData();
    for(size_t i = 0; i < axisOdfData.size(); i++)
    {
      e1s.push_back(axisOdfData[i][0] * SIMPLib::Constants::k_PiOver180D);
      e2s.push_back(axisOdfData[i][1] * SIMPLib::Constants::k_PiOver180D);
      e3s.push_back(axisOdfData[i][2] * SIMPLib::Constants::k_PiOver180D);
      weights.push_back(axisOdfData[i][3]);
      sigmas.push_back(axisOdfData[i][4]);
    }
    StatsGeneratorUtilities::GenerateAxisODFBinData(m_PrimaryStatsData, PhaseType::Type::Primary, e1s, e2s, e3s, weights, sigmas, true);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString GeneratePrimaryStatsData::getNumberOfBins()
{
  return m_NumberOfBins;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString GeneratePrimaryStatsData::getFeatureESD()
{
  return m_FeatureESD;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::normalizePhaseFractions(StatsDataArray* statsDataArray)
{
  size_t count = statsDataArray->getNumberOfTuples();
  // Start at index 1 since the first one is junk
  float totalPhaseFraction = 0.0f;
  for(size_t i = 1; i < count; i++)
  {
    StatsData::Pointer statsData = statsDataArray->getStatsData(i);
    totalPhaseFraction += totalPhaseFraction + statsData->getPhaseFraction();
  }
  // Now loop again and set the correct phase fractions
  for(size_t i = 1; i < count; i++)
  {
    StatsData::Pointer statsData = statsDataArray->getStatsData(i);
    statsData->setPhaseFraction(statsData->getPhaseFraction() / totalPhaseFraction);
  }
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
AbstractFilter::Pointer GeneratePrimaryStatsData::newFilterInstance(bool copyFilterParameters) const
{
  GeneratePrimaryStatsData::Pointer filter = GeneratePrimaryStatsData::New();
  if(copyFilterParameters)
  {
    copyFilterParameterInstanceVariables(filter.get());
  }
  return filter;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString GeneratePrimaryStatsData::getCompiledLibraryName() const
{
  return SyntheticBuildingConstants::SyntheticBuildingBaseName;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString GeneratePrimaryStatsData::getBrandingString() const
{
  return "StatsGenerator";
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString GeneratePrimaryStatsData::getFilterVersion() const
{
  QString version;
  QTextStream vStream(&version);
  vStream << SyntheticBuilding::Version::Major() << "." << SyntheticBuilding::Version::Minor() << "." << SyntheticBuilding::Version::Patch();
  return version;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString GeneratePrimaryStatsData::getGroupName() const
{
  return SIMPL::FilterGroups::Unsupported;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QUuid GeneratePrimaryStatsData::getUuid() const
{
  return QUuid("{383f0e2a-c82e-5f7e-a904-156828b42314}");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString GeneratePrimaryStatsData::getSubGroupName() const
{
  return "StatsGenerator";
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
QString GeneratePrimaryStatsData::getHumanLabel() const
{
  return "Generate Primary StatsData";
}

// -----------------------------------------------------------------------------
GeneratePrimaryStatsData::Pointer GeneratePrimaryStatsData::NullPointer()
{
  return Pointer(static_cast<Self*>(nullptr));
}

// -----------------------------------------------------------------------------
std::shared_ptr<GeneratePrimaryStatsData> GeneratePrimaryStatsData::New()
{
  struct make_shared_enabler : public GeneratePrimaryStatsData
  {
  };
  std::shared_ptr<make_shared_enabler> val = std::make_shared<make_shared_enabler>();
  val->setupFilterParameters();
  return val;
}

// -----------------------------------------------------------------------------
QString GeneratePrimaryStatsData::getNameOfClass() const
{
  return QString("GeneratePrimaryStatsData");
}

// -----------------------------------------------------------------------------
QString GeneratePrimaryStatsData::ClassName()
{
  return QString("GeneratePrimaryStatsData");
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setPhaseName(const QString& value)
{
  m_PhaseName = value;
}

// -----------------------------------------------------------------------------
QString GeneratePrimaryStatsData::getPhaseName() const
{
  return m_PhaseName;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setPhaseIndex(int value)
{
  m_PhaseIndex = value;
}

// -----------------------------------------------------------------------------
int GeneratePrimaryStatsData::getPhaseIndex() const
{
  return m_PhaseIndex;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setCrystalSymmetry(int value)
{
  m_CrystalSymmetry = value;
}

// -----------------------------------------------------------------------------
int GeneratePrimaryStatsData::getCrystalSymmetry() const
{
  return m_CrystalSymmetry;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setMicroPresetModel(int value)
{
  m_MicroPresetModel = value;
}

// -----------------------------------------------------------------------------
int GeneratePrimaryStatsData::getMicroPresetModel() const
{
  return m_MicroPresetModel;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setPhaseFraction(double value)
{
  m_PhaseFraction = value;
}

// -----------------------------------------------------------------------------
double GeneratePrimaryStatsData::getPhaseFraction() const
{
  return m_PhaseFraction;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setMu(double value)
{
  m_Mu = value;
}

// -----------------------------------------------------------------------------
double GeneratePrimaryStatsData::getMu() const
{
  return m_Mu;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setSigma(double value)
{
  m_Sigma = value;
}

// -----------------------------------------------------------------------------
double GeneratePrimaryStatsData::getSigma() const
{
  return m_Sigma;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setMinCutOff(double value)
{
  m_MinCutOff = value;
}

// -----------------------------------------------------------------------------
double GeneratePrimaryStatsData::getMinCutOff() const
{
  return m_MinCutOff;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setMaxCutOff(double value)
{
  m_MaxCutOff = value;
}

// -----------------------------------------------------------------------------
double GeneratePrimaryStatsData::getMaxCutOff() const
{
  return m_MaxCutOff;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setBinStepSize(double value)
{
  m_BinStepSize = value;
}

// -----------------------------------------------------------------------------
double GeneratePrimaryStatsData::getBinStepSize() const
{
  return m_BinStepSize;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setCreateEnsembleAttributeMatrix(bool value)
{
  m_CreateEnsembleAttributeMatrix = value;
}

// -----------------------------------------------------------------------------
bool GeneratePrimaryStatsData::getCreateEnsembleAttributeMatrix() const
{
  return m_CreateEnsembleAttributeMatrix;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setDataContainerName(const DataArrayPath& value)
{
  m_DataContainerName = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GeneratePrimaryStatsData::getDataContainerName() const
{
  return m_DataContainerName;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setCellEnsembleAttributeMatrixName(const QString& value)
{
  m_CellEnsembleAttributeMatrixName = value;
}

// -----------------------------------------------------------------------------
QString GeneratePrimaryStatsData::getCellEnsembleAttributeMatrixName() const
{
  return m_CellEnsembleAttributeMatrixName;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setAppendToExistingAttributeMatrix(bool value)
{
  m_AppendToExistingAttributeMatrix = value;
}

// -----------------------------------------------------------------------------
bool GeneratePrimaryStatsData::getAppendToExistingAttributeMatrix() const
{
  return m_AppendToExistingAttributeMatrix;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setSelectedEnsembleAttributeMatrix(const DataArrayPath& value)
{
  m_SelectedEnsembleAttributeMatrix = value;
}

// -----------------------------------------------------------------------------
DataArrayPath GeneratePrimaryStatsData::getSelectedEnsembleAttributeMatrix() const
{
  return m_SelectedEnsembleAttributeMatrix;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setOdfData(const DynamicTableData& value)
{
  m_OdfData = value;
}

// -----------------------------------------------------------------------------
DynamicTableData GeneratePrimaryStatsData::getOdfData() const
{
  return m_OdfData;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setMdfData(const DynamicTableData& value)
{
  m_MdfData = value;
}

// -----------------------------------------------------------------------------
DynamicTableData GeneratePrimaryStatsData::getMdfData() const
{
  return m_MdfData;
}

// -----------------------------------------------------------------------------
void GeneratePrimaryStatsData::setAxisOdfData(const DynamicTableData& value)
{
  m_AxisOdfData = value;
}

// -----------------------------------------------------------------------------
DynamicTableData GeneratePrimaryStatsData::getAxisOdfData() const
{
  return m_AxisOdfData;
}
