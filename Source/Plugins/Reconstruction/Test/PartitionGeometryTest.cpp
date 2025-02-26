/* ============================================================================
 * Copyright (c) 2022-2022 BlueQuartz Software, LLC
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
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#pragma once

#include <QtCore/QFile>

#include "SIMPLib/SIMPLib.h"
#include "SIMPLib/CoreFilters/DataContainerReader.h"
#include "SIMPLib/CoreFilters/DataContainerWriter.h"
#include "SIMPLib/DataContainers/DataContainerArray.h"
#include "SIMPLib/Geometry/ImageGeom.h"

#include "Reconstruction/ReconstructionFilters/PartitionGeometry.h"
#include "Reconstruction/Test/ReconstructionTestFileLocations.h"
#include "Reconstruction/Test/UnitTestSupport.hpp"

class PartitionGeometryTest
{

public:
  PartitionGeometryTest() = default;
  ~PartitionGeometryTest() = default;
  PartitionGeometryTest(const PartitionGeometryTest&) = delete;            // Copy Constructor
  PartitionGeometryTest(PartitionGeometryTest&&) = delete;                 // Move Constructor
  PartitionGeometryTest& operator=(const PartitionGeometryTest&) = delete; // Copy Assignment
  PartitionGeometryTest& operator=(PartitionGeometryTest&&) = delete;      // Move Assignment

  // -----------------------------------------------------------------------------
  void TestGeometry(PartitionGeometry::Pointer filter, const QString& inputFile, const DataArrayPath& arrayPath, const QString& exemplaryArrayName)
  {
    DataContainerArray::Pointer dca = DataContainerArray::New();
    {
      DataContainerReader::Pointer filter = DataContainerReader::New();
      DataContainerArrayProxy dcaProxy = filter->readDataContainerArrayStructure(inputFile);
      filter->setInputFileDataContainerArrayProxy(dcaProxy);
      filter->setInputFile(inputFile);
      filter->setDataContainerArray(dca);
      filter->execute();
      int err = filter->getErrorCode();
      DREAM3D_REQUIRE(err >= 0)
    }

    filter->setDataContainerArray(dca);
    filter->execute();
    int err = filter->getErrorCode();
    DREAM3D_REQUIRE(err >= 0)

    AttributeMatrix::Pointer am = dca->getAttributeMatrix(arrayPath);
    DREAM3D_REQUIRE(am != AttributeMatrix::NullPointer())

    Int32ArrayType::Pointer partitionIds = am->getAttributeArrayAs<Int32ArrayType>(arrayPath.getDataArrayName());
    DREAM3D_REQUIRE(partitionIds != Int32ArrayType::NullPointer())

    Int32ArrayType::Pointer exemplaryPartitionIds = am->getAttributeArrayAs<Int32ArrayType>(exemplaryArrayName);
    DREAM3D_REQUIRE(exemplaryPartitionIds != Int32ArrayType::NullPointer())

    DREAM3D_REQUIRE_EQUAL(partitionIds->getSize(), exemplaryPartitionIds->getSize())

    int32_t* partitionIdsPtr = partitionIds->getPointer(0);
    int32_t* exemplaryPartitionIdsPtr = exemplaryPartitionIds->getPointer(0);
    for(size_t i = 0; i < partitionIds->getSize(); i++)
    {
      const int32_t partitionId = partitionIdsPtr[i];
      const int32_t exemplaryId = exemplaryPartitionIdsPtr[i];
      DREAM3D_REQUIRE_EQUAL(partitionId, exemplaryId)
    }
  }

  // -----------------------------------------------------------------------------
  void TestGeometryError(PartitionGeometry::Pointer filter, const QString& inputFile, const DataArrayPath& arrayPath, int expectedErrorCode)
  {
    DataContainerArray::Pointer dca = DataContainerArray::New();
    {
      DataContainerReader::Pointer filter = DataContainerReader::New();
      DataContainerArrayProxy dcaProxy = filter->readDataContainerArrayStructure(inputFile);
      filter->setInputFileDataContainerArrayProxy(dcaProxy);
      filter->setInputFile(inputFile);
      filter->setDataContainerArray(dca);
      filter->execute();
      int err = filter->getErrorCode();
      DREAM3D_REQUIRE(err >= 0)
    }

    filter->setDataContainerArray(dca);
    filter->execute();
    int err = filter->getErrorCode();
    DREAM3D_REQUIRE(err == expectedErrorCode)
  }

  // -----------------------------------------------------------------------------
  PartitionGeometry::Pointer CreateBasicPartitionGeometryFilter(const QString& inputFile, const DataArrayPath& arrayPath, const IntVec3Type& numOfPartitionsPerAxis,
                                                                PartitionGeometry::PartitioningMode partitioningMode, const std::optional<DataArrayPath>& maskArrayPath)
  {
    PartitionGeometry::Pointer filter = PartitionGeometry::New();
    filter->setPartitioningMode(static_cast<int>(partitioningMode));
    filter->setNumberOfPartitionsPerAxis(numOfPartitionsPerAxis);
    filter->setAttributeMatrixPath(arrayPath);
    filter->setPartitionIdsArrayName(arrayPath.getDataArrayName());

    if(maskArrayPath.has_value())
    {
      filter->setUseVertexMask(true);
      filter->setVertexMaskPath(*maskArrayPath);
    }

    return filter;
  }

  // -----------------------------------------------------------------------------
  void TestBasicGeometry(const QString& inputFile, const DataArrayPath& arrayPath, const IntVec3Type& numOfPartitionsPerAxis, const QString& exemplaryArrayName,
                         const std::optional<DataArrayPath>& maskArrayPath = {})
  {
    PartitionGeometry::Pointer filter = CreateBasicPartitionGeometryFilter(inputFile, arrayPath, numOfPartitionsPerAxis, PartitionGeometry::PartitioningMode::Basic, maskArrayPath);
    TestGeometry(filter, inputFile, arrayPath, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestBasicGeometryError(const QString& inputFile, const DataArrayPath& arrayPath, const IntVec3Type& numOfPartitionsPerAxis, int expectedErrorCode,
                              const std::optional<DataArrayPath>& maskArrayPath = {})
  {
    PartitionGeometry::Pointer filter = CreateBasicPartitionGeometryFilter(inputFile, arrayPath, numOfPartitionsPerAxis, PartitionGeometry::PartitioningMode::Basic, maskArrayPath);
    TestGeometryError(filter, inputFile, arrayPath, expectedErrorCode);
  }

  // -----------------------------------------------------------------------------
  void TestAdvancedGeometry(const QString& inputFile, const DataArrayPath& arrayPath, const IntVec3Type& numOfPartitionsPerAxis, const FloatVec3Type& partitioningSchemeOrigin,
                            const FloatVec3Type& lengthPerPartition, const QString& exemplaryArrayName)
  {
    PartitionGeometry::Pointer filter = PartitionGeometry::New();
    filter->setPartitioningMode(static_cast<int>(PartitionGeometry::PartitioningMode::Advanced));
    filter->setNumberOfPartitionsPerAxis(numOfPartitionsPerAxis);
    filter->setPartitioningSchemeOrigin(partitioningSchemeOrigin);
    filter->setLengthPerPartition(lengthPerPartition);
    filter->setAttributeMatrixPath(arrayPath);
    filter->setPartitionIdsArrayName(arrayPath.getDataArrayName());

    TestGeometry(filter, inputFile, arrayPath, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestBoundingBoxGeometry(const QString& inputFile, const DataArrayPath& arrayPath, const IntVec3Type& numOfPartitionsPerAxis, const FloatVec3Type& lowerLeftCoord,
                               const FloatVec3Type& upperRightCoord, const QString& exemplaryArrayName)
  {
    PartitionGeometry::Pointer filter = PartitionGeometry::New();
    filter->setPartitioningMode(static_cast<int>(PartitionGeometry::PartitioningMode::BoundingBox));
    filter->setNumberOfPartitionsPerAxis(numOfPartitionsPerAxis);
    filter->setLowerLeftCoord(lowerLeftCoord);
    filter->setUpperRightCoord(upperRightCoord);
    filter->setAttributeMatrixPath(arrayPath);
    filter->setPartitionIdsArrayName(arrayPath.getDataArrayName());

    TestGeometry(filter, inputFile, arrayPath, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestExistingPartitioningSchemeGeometry(const QString& inputFile, const DataArrayPath& arrayPath, const QString& exemplaryArrayName, const DataArrayPath& partitioningSchemeDCPath)
  {
    PartitionGeometry::Pointer filter = PartitionGeometry::New();
    filter->setPartitioningMode(static_cast<int>(PartitionGeometry::PartitioningMode::ExistingPartitioningScheme));
    filter->setPartitioningSchemeDataContainerName(partitioningSchemeDCPath);
    filter->setAttributeMatrixPath(arrayPath);
    filter->setPartitionIdsArrayName(arrayPath.getDataArrayName());

    TestGeometry(filter, inputFile, arrayPath, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestBasicImageGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryImageGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {5, 5, 5};
    DataArrayPath arrayPath = {"DataContainer", "CellData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBasicGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestAdvancedImageGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryImageGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {5, 5, 5};
    FloatVec3Type partitioningSchemeOrigin = {-10, 5, 2};
    FloatVec3Type lengthPerPartition = {5, 5, 5};
    DataArrayPath arrayPath = {"DataContainer", "CellData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestAdvancedGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, partitioningSchemeOrigin, lengthPerPartition, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestBoundingBoxImageGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryImageGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {5, 5, 5};
    FloatVec3Type lowerLeftCoord = {-10, 5, 2};
    FloatVec3Type upperRightCoord = {15, 30, 27};
    DataArrayPath arrayPath = {"DataContainer", "CellData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBoundingBoxGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, lowerLeftCoord, upperRightCoord, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestExistingPartitioningSchemeImageGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryImageGeomIdsPath;
    DataArrayPath arrayPath = {"DataContainer", "CellData", "PartitioningSchemeIds"};
    DataArrayPath partitioningSchemeDCPath = {"PartitioningSchemeDataContainer", "", ""};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestExistingPartitioningSchemeGeometry(inputFile, arrayPath, exemplaryArrayName, partitioningSchemeDCPath);
  }

  // -----------------------------------------------------------------------------
  void TestBasicRectGridGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryRectGridGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {5, 5, 5};
    DataArrayPath arrayPath = {"DataContainer", "CellData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBasicGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestAdvancedRectGridGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryRectGridGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {5, 5, 5};
    FloatVec3Type partitioningSchemeOrigin = {0, 0, 0};
    FloatVec3Type lengthPerPartition = {6, 6, 6};
    DataArrayPath arrayPath = {"DataContainer", "CellData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestAdvancedGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, partitioningSchemeOrigin, lengthPerPartition, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestBoundingBoxRectGridGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryRectGridGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {5, 5, 5};
    FloatVec3Type lowerLeftCoord = {0, 0, 0};
    FloatVec3Type upperRightCoord = {30, 30, 30};
    DataArrayPath arrayPath = {"DataContainer", "CellData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBoundingBoxGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, lowerLeftCoord, upperRightCoord, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestExistingPartitioningSchemeRectGridGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryRectGridGeomIdsPath;
    DataArrayPath arrayPath = {"DataContainer", "CellData", "PartitioningSchemeIds"};
    DataArrayPath partitioningSchemeDCPath = {"PartitioningSchemeDataContainer", "", ""};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestExistingPartitioningSchemeGeometry(inputFile, arrayPath, exemplaryArrayName, partitioningSchemeDCPath);
  }

  // -----------------------------------------------------------------------------
  void TestBasicTriangleGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryTriangleGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {5, 4, 4};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBasicGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestAdvancedTriangleGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryTriangleGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {5, 4, 4};
    FloatVec3Type partitioningSchemeOrigin = {-0.997462, -0.997462, -0.00001};
    FloatVec3Type lengthPerPartition = {0.398984, 0.49873, 0.247939};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestAdvancedGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, partitioningSchemeOrigin, lengthPerPartition, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestBoundingBoxTriangleGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryTriangleGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {5, 4, 4};
    FloatVec3Type lowerLeftCoord = {-0.997462, -0.997462, -0.00001};
    FloatVec3Type upperRightCoord = {0.997463, 0.997462, 0.991746};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBoundingBoxGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, lowerLeftCoord, upperRightCoord, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestExistingPartitioningSchemeTriangleGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryTriangleGeomIdsPath;
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    DataArrayPath partitioningSchemeDCPath = {"PartitioningSchemeDataContainer", "", ""};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestExistingPartitioningSchemeGeometry(inputFile, arrayPath, exemplaryArrayName, partitioningSchemeDCPath);
  }

  // -----------------------------------------------------------------------------
  void TestMaskedTriangleGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryTriangleGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {5, 4, 4};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    DataArrayPath maskPath = {"DataContainer", "VertexData", "Mask"};
    QString exemplaryArrayName = "MaskedExemplaryPartitioningSchemeIds";

    TestBasicGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, exemplaryArrayName, maskPath);
  }

  // -----------------------------------------------------------------------------
  void TestBasicEdgeGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryEdgeGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {4, 4, 4};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBasicGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestAdvancedEdgeGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryEdgeGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {4, 4, 4};
    FloatVec3Type partitioningSchemeOrigin = {-0.997462, -0.997462, -0.00001};
    FloatVec3Type lengthPerPartition = {0.49873, 0.49873, 0.247939};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestAdvancedGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, partitioningSchemeOrigin, lengthPerPartition, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestBoundingBoxEdgeGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryEdgeGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {4, 4, 4};
    FloatVec3Type lowerLeftCoord = {-0.997462, -0.997462, -0.00001};
    FloatVec3Type upperRightCoord = {0.997462, 0.997462, 0.991746};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBoundingBoxGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, lowerLeftCoord, upperRightCoord, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestExistingPartitioningSchemeEdgeGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryEdgeGeomIdsPath;
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    DataArrayPath partitioningSchemeDCPath = {"PartitioningSchemeDataContainer", "", ""};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestExistingPartitioningSchemeGeometry(inputFile, arrayPath, exemplaryArrayName, partitioningSchemeDCPath);
  }

  // -----------------------------------------------------------------------------
  void TestMaskedEdgeGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryEdgeGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {4, 4, 4};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    DataArrayPath maskPath = {"DataContainer", "VertexData", "Mask"};
    QString exemplaryArrayName = "MaskedExemplaryPartitioningSchemeIds";

    TestBasicGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, exemplaryArrayName, maskPath);
  }

  // -----------------------------------------------------------------------------
  void TestBasicVertexGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryVertexGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {20, 10, 5};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBasicGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestAdvancedVertexGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryVertexGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {20, 10, 5};
    FloatVec3Type partitioningSchemeOrigin = {-0.997462, -0.997462, -0.00001};
    FloatVec3Type lengthPerPartition = {0.099746, 0.199492, 0.198351};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestAdvancedGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, partitioningSchemeOrigin, lengthPerPartition, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestBoundingBoxVertexGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryVertexGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {20, 10, 5};
    FloatVec3Type lowerLeftCoord = {-0.997462, -0.997462, -0.00001};
    FloatVec3Type upperRightCoord = {0.997462, 0.997458, 0.991745};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBoundingBoxGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, lowerLeftCoord, upperRightCoord, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestExistingPartitioningSchemeVertexGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryVertexGeomIdsPath;
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    DataArrayPath partitioningSchemeDCPath = {"PartitioningSchemeDataContainer", "", ""};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestExistingPartitioningSchemeGeometry(inputFile, arrayPath, exemplaryArrayName, partitioningSchemeDCPath);
  }

  // -----------------------------------------------------------------------------
  void TestMaskedVertexGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryVertexGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {20, 10, 5};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    DataArrayPath maskPath = {"DataContainer", "VertexData", "Mask"};
    QString exemplaryArrayName = "MaskedExemplaryPartitioningSchemeIds";

    TestBasicGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, exemplaryArrayName, maskPath);
  }

  // -----------------------------------------------------------------------------
  void TestBasicQuadGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryQuadGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {10, 5, 3};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBasicGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestAdvancedQuadGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryQuadGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {10, 5, 3};
    FloatVec3Type partitioningSchemeOrigin = {-0.997462, -0.997462, -0.00001};
    FloatVec3Type lengthPerPartition = {0.199492, 0.398984, 0.330585333333333};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestAdvancedGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, partitioningSchemeOrigin, lengthPerPartition, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestBoundingBoxQuadGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryQuadGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {10, 5, 3};
    FloatVec3Type lowerLeftCoord = {-0.997462, -0.997462, -0.00001};
    FloatVec3Type upperRightCoord = {0.997462, 0.997462, 0.991746};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBoundingBoxGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, lowerLeftCoord, upperRightCoord, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestExistingPartitioningSchemeQuadGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryQuadGeomIdsPath;
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    DataArrayPath partitioningSchemeDCPath = {"PartitioningSchemeDataContainer", "", ""};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestExistingPartitioningSchemeGeometry(inputFile, arrayPath, exemplaryArrayName, partitioningSchemeDCPath);
  }

  // -----------------------------------------------------------------------------
  void TestMaskedQuadGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryQuadGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {10, 5, 3};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    DataArrayPath maskPath = {"DataContainer", "VertexData", "Mask"};
    QString exemplaryArrayName = "MaskedExemplaryPartitioningSchemeIds";

    TestBasicGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, exemplaryArrayName, maskPath);
  }

  // -----------------------------------------------------------------------------
  void TestBasicTetrahedralGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryTetrahedralGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {100, 45, 8};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBasicGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestAdvancedTetrahedralGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryTetrahedralGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {100, 45, 8};
    FloatVec3Type partitioningSchemeOrigin = {-0.997462, -0.997462, -0.00001};
    FloatVec3Type lengthPerPartition = {0.0199492, 0.044331555555556, 0.12397};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestAdvancedGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, partitioningSchemeOrigin, lengthPerPartition, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestBoundingBoxTetrahedralGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryTetrahedralGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {100, 45, 8};
    FloatVec3Type lowerLeftCoord = {-0.997462, -0.997462, -0.00001};
    FloatVec3Type upperRightCoord = {0.997458, 0.99746, 0.99175};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBoundingBoxGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, lowerLeftCoord, upperRightCoord, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestExistingPartitioningSchemeTetrahedralGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryTetrahedralGeomIdsPath;
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    DataArrayPath partitioningSchemeDCPath = {"PartitioningSchemeDataContainer", "", ""};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestExistingPartitioningSchemeGeometry(inputFile, arrayPath, exemplaryArrayName, partitioningSchemeDCPath);
  }

  // -----------------------------------------------------------------------------
  void TestMaskedTetrahedralGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryTetrahedralGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {100, 45, 8};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    DataArrayPath maskPath = {"DataContainer", "VertexData", "Mask"};
    QString exemplaryArrayName = "MaskedExemplaryPartitioningSchemeIds";

    TestBasicGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, exemplaryArrayName, maskPath);
  }

  // -----------------------------------------------------------------------------
  void TestBasicHexahedralGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryHexahedralGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {6, 7, 8};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBasicGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestAdvancedHexahedralGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryHexahedralGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {6, 7, 8};
    FloatVec3Type partitioningSchemeOrigin = {0.9999989867210388, 0.9999989867210388, 1.5499989986419678};
    FloatVec3Type lengthPerPartition = {1.105000376701355, 0.2857145667076111, 0.2500002384185791};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestAdvancedGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, partitioningSchemeOrigin, lengthPerPartition, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestBoundingBoxHexahedralGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryHexahedralGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {6, 7, 8};
    FloatVec3Type lowerLeftCoord = {0.9999989867210388, 0.9999989867210388, 1.5499989986419678};
    FloatVec3Type upperRightCoord = {7.630001068115234, 3.0000009536743164, 3.5500009059906006};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBoundingBoxGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, lowerLeftCoord, upperRightCoord, exemplaryArrayName);
  }

  // -----------------------------------------------------------------------------
  void TestExistingPartitioningSchemeHexahedralGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryHexahedralGeomIdsPath;
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    DataArrayPath partitioningSchemeDCPath = {"PartitioningSchemeDataContainer", "", ""};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestExistingPartitioningSchemeGeometry(inputFile, arrayPath, exemplaryArrayName, partitioningSchemeDCPath);
  }

  // -----------------------------------------------------------------------------
  void TestMaskedHexahedralGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryHexahedralGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {6, 7, 8};
    DataArrayPath arrayPath = {"DataContainer", "VertexData", "PartitioningSchemeIds"};
    DataArrayPath maskPath = {"DataContainer", "VertexData", "Mask"};
    QString exemplaryArrayName = "MaskedExemplaryPartitioningSchemeIds";

    TestBasicGeometry(inputFile, arrayPath, numOfPartitionsPerAxis, exemplaryArrayName, maskPath);
  }

  // -----------------------------------------------------------------------------
  void TestPlanalXYNodeGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryPlanalXYNodeGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {3, 3, 3};
    DataArrayPath arrayPath = {"VertexDataContainer", "AttributeMatrix", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBasicGeometryError(inputFile, arrayPath, numOfPartitionsPerAxis, -3042);
  }

  // -----------------------------------------------------------------------------
  void TestPlanalXZNodeGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryPlanalXZNodeGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {3, 3, 3};
    DataArrayPath arrayPath = {"VertexDataContainer", "AttributeMatrix", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBasicGeometryError(inputFile, arrayPath, numOfPartitionsPerAxis, -3041);
  }

  // -----------------------------------------------------------------------------
  void TestPlanalYZNodeGeometry()
  {
    QString inputFile = UnitTest::PartitionGeometryTest::ExemplaryPlanalYZNodeGeomIdsPath;
    IntVec3Type numOfPartitionsPerAxis = {3, 3, 3};
    DataArrayPath arrayPath = {"VertexDataContainer", "AttributeMatrix", "PartitioningSchemeIds"};
    QString exemplaryArrayName = "ExemplaryPartitioningSchemeIds";

    TestBasicGeometryError(inputFile, arrayPath, numOfPartitionsPerAxis, -3040);
  }

  // -----------------------------------------------------------------------------
  void operator()()
  {
    int err = EXIT_SUCCESS;
    Q_UNUSED(err)

    DREAM3D_REGISTER_TEST(TestBasicImageGeometry())
    DREAM3D_REGISTER_TEST(TestAdvancedImageGeometry())
    DREAM3D_REGISTER_TEST(TestBoundingBoxImageGeometry())
    DREAM3D_REGISTER_TEST(TestExistingPartitioningSchemeImageGeometry())

    DREAM3D_REGISTER_TEST(TestBasicRectGridGeometry())
    DREAM3D_REGISTER_TEST(TestAdvancedRectGridGeometry())
    DREAM3D_REGISTER_TEST(TestBoundingBoxRectGridGeometry())
    DREAM3D_REGISTER_TEST(TestExistingPartitioningSchemeRectGridGeometry())

    DREAM3D_REGISTER_TEST(TestBasicTriangleGeometry())
    DREAM3D_REGISTER_TEST(TestAdvancedTriangleGeometry())
    DREAM3D_REGISTER_TEST(TestBoundingBoxTriangleGeometry())
    DREAM3D_REGISTER_TEST(TestExistingPartitioningSchemeTriangleGeometry())
    DREAM3D_REGISTER_TEST(TestMaskedTriangleGeometry())

    DREAM3D_REGISTER_TEST(TestBasicEdgeGeometry())
    DREAM3D_REGISTER_TEST(TestAdvancedEdgeGeometry())
    DREAM3D_REGISTER_TEST(TestBoundingBoxEdgeGeometry())
    DREAM3D_REGISTER_TEST(TestExistingPartitioningSchemeEdgeGeometry())
    DREAM3D_REGISTER_TEST(TestMaskedEdgeGeometry())

    DREAM3D_REGISTER_TEST(TestBasicVertexGeometry())
    DREAM3D_REGISTER_TEST(TestAdvancedVertexGeometry())
    DREAM3D_REGISTER_TEST(TestBoundingBoxVertexGeometry())
    DREAM3D_REGISTER_TEST(TestExistingPartitioningSchemeVertexGeometry())
    DREAM3D_REGISTER_TEST(TestMaskedVertexGeometry())

    DREAM3D_REGISTER_TEST(TestBasicQuadGeometry())
    DREAM3D_REGISTER_TEST(TestAdvancedQuadGeometry())
    DREAM3D_REGISTER_TEST(TestBoundingBoxQuadGeometry())
    DREAM3D_REGISTER_TEST(TestExistingPartitioningSchemeQuadGeometry())
    DREAM3D_REGISTER_TEST(TestMaskedQuadGeometry())

    DREAM3D_REGISTER_TEST(TestBasicTetrahedralGeometry())
    DREAM3D_REGISTER_TEST(TestAdvancedTetrahedralGeometry())
    DREAM3D_REGISTER_TEST(TestBoundingBoxTetrahedralGeometry())
    DREAM3D_REGISTER_TEST(TestExistingPartitioningSchemeTetrahedralGeometry())
    DREAM3D_REGISTER_TEST(TestMaskedTetrahedralGeometry())

    DREAM3D_REGISTER_TEST(TestBasicHexahedralGeometry())
    DREAM3D_REGISTER_TEST(TestAdvancedHexahedralGeometry())
    DREAM3D_REGISTER_TEST(TestBoundingBoxHexahedralGeometry())
    DREAM3D_REGISTER_TEST(TestExistingPartitioningSchemeHexahedralGeometry())
    DREAM3D_REGISTER_TEST(TestMaskedHexahedralGeometry())

    DREAM3D_REGISTER_TEST(TestPlanalXYNodeGeometry())
    DREAM3D_REGISTER_TEST(TestPlanalXZNodeGeometry())
    DREAM3D_REGISTER_TEST(TestPlanalYZNodeGeometry())
  }
};
