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

#include <QtCore/QDebug>
#include <QtCore/QFile>

#include "SIMPLib/SIMPLib.h"
#include "SIMPLib/DataArrays/DataArray.hpp"
#include "SIMPLib/DataContainers/DataContainer.h"
#include "SIMPLib/DataContainers/DataContainerArray.h"
#include "SIMPLib/Filtering/FilterFactory.hpp"
#include "SIMPLib/Filtering/FilterManager.h"
#include "SIMPLib/Filtering/FilterPipeline.h"
#include "SIMPLib/Filtering/QMetaObjectUtilities.h"
#include "SIMPLib/Geometry/TriangleGeom.h"
#include "SIMPLib/Plugin/ISIMPLibPlugin.h"
#include "SIMPLib/Plugin/SIMPLibPluginLoader.h"

#include "UnitTestSupport.hpp"

#include "SurfaceMeshingTestFileLocations.h"

class FindTriangleGeomSizesTest
{

public:
  FindTriangleGeomSizesTest() = default;
  virtual ~FindTriangleGeomSizesTest() = default;

  /**
   * @brief Returns the name of the class for FindTriangleGeomSizesTest
   */
  /**
   * @brief Returns the name of the class for FindTriangleGeomSizesTest
   */
  QString getNameOfClass() const
  {
    return QString("FindTriangleGeomSizesTest");
  }

  /**
   * @brief Returns the name of the class for FindTriangleGeomSizesTest
   */
  QString ClassName()
  {
    return QString("FindTriangleGeomSizesTest");
  }

  FindTriangleGeomSizesTest(const FindTriangleGeomSizesTest&) = delete;            // Copy Constructor Not Implemented
  FindTriangleGeomSizesTest(FindTriangleGeomSizesTest&&) = delete;                 // Move Constructor Not Implemented
  FindTriangleGeomSizesTest& operator=(const FindTriangleGeomSizesTest&) = delete; // Copy Assignment Not Implemented
  FindTriangleGeomSizesTest& operator=(FindTriangleGeomSizesTest&&) = delete;      // Move Assignment Not Implemented

  // -----------------------------------------------------------------------------
  //
  // -----------------------------------------------------------------------------
  void RemoveTestFiles()
  {
#if REMOVE_TEST_FILES
    QFile::remove(UnitTest::FindTriangleGeomSizesTest::TestFile1);
    QFile::remove(UnitTest::FindTriangleGeomSizesTest::TestFile2);
#endif
  }

  // -----------------------------------------------------------------------------
  //
  // -----------------------------------------------------------------------------
  int TestFilterAvailability()
  {
    // Now instantiate the FindTriangleGeomSizesTest Filter from the FilterManager
    QString filtName = "FindTriangleGeomSizes";
    FilterManager* fm = FilterManager::Instance();
    IFilterFactory::Pointer filterFactory = fm->getFactoryFromClassName(filtName);
    if(nullptr == filterFactory.get())
    {
      std::stringstream ss;
      ss << "The SurfaceMeshing Requires the use of the " << filtName.toStdString() << " filter which is found in the SurfaceMeshing Plugin";
      DREAM3D_TEST_THROW_EXCEPTION(ss.str())
    }
    return 0;
  }

  // -----------------------------------------------------------------------------
  //
  // -----------------------------------------------------------------------------
  int TestFindTriangleGeomSizesTest()
  {
    DataContainerArray::Pointer dca = DataContainerArray::New();

    DataContainer::Pointer tdc = DataContainer::New(SIMPL::Defaults::TriangleDataContainerName);
    dca->addOrReplaceDataContainer(tdc);

    // Basic idea is to create a surface mesh of a rectangular prism with edge lengths of 3x1x1 for a
    // total volume of 3, where the entire enclosed volume represents one feature;
    // triangle windings are mixed to make sure the filter is properly flipping
    // windings where necessary to ensure consistency
    SharedVertexList::Pointer vertex = TriangleGeom::CreateSharedVertexList(8);
    TriangleGeom::Pointer triangle = TriangleGeom::CreateGeometry(12, vertex, SIMPL::Geometry::TriangleGeometry);
    tdc->setGeometry(triangle);
    float* vertices = triangle->getVertexPointer(0);
    size_t* tris = triangle->getTriPointer(0);

    vertices[3 * 0 + 0] = -1.0f;
    vertices[3 * 0 + 1] = 0.0f;
    vertices[3 * 0 + 2] = 0.0f;

    vertices[3 * 1 + 0] = -1.0f;
    vertices[3 * 1 + 1] = 0.0f;
    vertices[3 * 1 + 2] = 1.0f;

    vertices[3 * 2 + 0] = -1.0f;
    vertices[3 * 2 + 1] = 1.0f;
    vertices[3 * 2 + 2] = 1.0f;

    vertices[3 * 3 + 0] = -1.0f;
    vertices[3 * 3 + 1] = 1.0f;
    vertices[3 * 3 + 2] = 0.0f;

    vertices[3 * 4 + 0] = 2.0f;
    vertices[3 * 4 + 1] = 0.0f;
    vertices[3 * 4 + 2] = 0.0f;

    vertices[3 * 5 + 0] = 2.0f;
    vertices[3 * 5 + 1] = 0.0f;
    vertices[3 * 5 + 2] = 1.0f;

    vertices[3 * 6 + 0] = 2.0f;
    vertices[3 * 6 + 1] = 1.0f;
    vertices[3 * 6 + 2] = 1.0f;

    vertices[3 * 7 + 0] = 2.0f;
    vertices[3 * 7 + 1] = 1.0f;
    vertices[3 * 7 + 2] = 0.0f;

    tris[3 * 0 + 0] = 0;
    tris[3 * 0 + 1] = 1;
    tris[3 * 0 + 2] = 3;

    tris[3 * 1 + 0] = 1;
    tris[3 * 1 + 1] = 2;
    tris[3 * 1 + 2] = 3;

    tris[3 * 2 + 0] = 0;
    tris[3 * 2 + 1] = 1;
    tris[3 * 2 + 2] = 5;

    tris[3 * 3 + 0] = 0;
    tris[3 * 3 + 1] = 4;
    tris[3 * 3 + 2] = 5;

    tris[3 * 4 + 0] = 5;
    tris[3 * 4 + 1] = 6;
    tris[3 * 4 + 2] = 4;

    tris[3 * 5 + 0] = 4;
    tris[3 * 5 + 1] = 7;
    tris[3 * 5 + 2] = 6;

    tris[3 * 6 + 0] = 2;
    tris[3 * 6 + 1] = 3;
    tris[3 * 6 + 2] = 7;

    tris[3 * 7 + 0] = 7;
    tris[3 * 7 + 1] = 6;
    tris[3 * 7 + 2] = 2;

    tris[3 * 8 + 0] = 1;
    tris[3 * 8 + 1] = 2;
    tris[3 * 8 + 2] = 6;

    tris[3 * 9 + 0] = 6;
    tris[3 * 9 + 1] = 5;
    tris[3 * 9 + 2] = 1;

    tris[3 * 10 + 0] = 0;
    tris[3 * 10 + 1] = 4;
    tris[3 * 10 + 2] = 7;

    tris[3 * 11 + 0] = 7;
    tris[3 * 11 + 1] = 3;
    tris[3 * 11 + 2] = 0;

    std::vector<size_t> tDims(1, 12);
    AttributeMatrix::Pointer faceAttrMat = AttributeMatrix::New(tDims, SIMPL::Defaults::FaceAttributeMatrixName, AttributeMatrix::Type::Face);
    tdc->addOrReplaceAttributeMatrix(faceAttrMat);
    tDims[0] = 2;
    AttributeMatrix::Pointer featAttrMat = AttributeMatrix::New(tDims, SIMPL::Defaults::FaceFeatureAttributeMatrixName, AttributeMatrix::Type::FaceFeature);
    tdc->addOrReplaceAttributeMatrix(featAttrMat);
    std::vector<size_t> cDims(1, 2);
    Int32ArrayType::Pointer faceLabels = Int32ArrayType::CreateArray(12, cDims, SIMPL::FaceData::SurfaceMeshFaceLabels, true);
    faceAttrMat->insertOrAssign(faceLabels);
    int32_t* faceLabelsPtr = faceLabels->getPointer(0);

    faceLabelsPtr[2 * 0 + 0] = -1;
    faceLabelsPtr[2 * 0 + 1] = 1;

    faceLabelsPtr[2 * 1 + 0] = -1;
    faceLabelsPtr[2 * 1 + 1] = 1;

    faceLabelsPtr[2 * 2 + 0] = 1;
    faceLabelsPtr[2 * 2 + 1] = -1;

    faceLabelsPtr[2 * 3 + 0] = -1;
    faceLabelsPtr[2 * 3 + 1] = 1;

    faceLabelsPtr[2 * 4 + 0] = 1;
    faceLabelsPtr[2 * 4 + 1] = -1;

    faceLabelsPtr[2 * 5 + 0] = -1;
    faceLabelsPtr[2 * 5 + 1] = 1;

    faceLabelsPtr[2 * 6 + 0] = 1;
    faceLabelsPtr[2 * 6 + 1] = -1;

    faceLabelsPtr[2 * 7 + 0] = 1;
    faceLabelsPtr[2 * 7 + 1] = -1;

    faceLabelsPtr[2 * 8 + 0] = 1;
    faceLabelsPtr[2 * 8 + 1] = -1;

    faceLabelsPtr[2 * 9 + 0] = 1;
    faceLabelsPtr[2 * 9 + 1] = -1;

    faceLabelsPtr[2 * 10 + 0] = 1;
    faceLabelsPtr[2 * 10 + 1] = -1;

    faceLabelsPtr[2 * 11 + 0] = 1;
    faceLabelsPtr[2 * 11 + 1] = -1;

    QString filtName = "FindTriangleGeomSizes";
    FilterManager* fm = FilterManager::Instance();
    IFilterFactory::Pointer factory = fm->getFactoryFromClassName(filtName);
    DREAM3D_REQUIRE(factory.get() != nullptr)

    AbstractFilter::Pointer sizeFilter = factory->create();
    DREAM3D_REQUIRE(sizeFilter.get() != nullptr)

    sizeFilter->setDataContainerArray(dca);

    bool propWasSet = true;
    QVariant var;

    DataArrayPath path(SIMPL::Defaults::TriangleDataContainerName, SIMPL::Defaults::FaceAttributeMatrixName, SIMPL::FaceData::SurfaceMeshFaceLabels);
    var.setValue(path);
    propWasSet = sizeFilter->setProperty("FaceLabelsArrayPath", var);
    if(!propWasSet)
    {
      qDebug() << "Unable to set property FaceLabelsArrayPath";
    }

    path.update(SIMPL::Defaults::TriangleDataContainerName, SIMPL::Defaults::FaceFeatureAttributeMatrixName, "");
    var.setValue(path);
    propWasSet = sizeFilter->setProperty("FeatureAttributeMatrixName", var);
    if(!propWasSet)
    {
      qDebug() << "Unable to set property FeatureAttributeMatrixName";
    }

    sizeFilter->execute();
    int32_t err = sizeFilter->getErrorCode();
    DREAM3D_REQUIRE_EQUAL(err, 0);

    AttributeMatrix::Pointer faceFeatAttrMat = tdc->getAttributeMatrix(SIMPL::Defaults::FaceFeatureAttributeMatrixName);
    FloatArrayType::Pointer volumes = faceFeatAttrMat->getAttributeArrayAs<FloatArrayType>(SIMPL::FeatureData::Volumes);

    DREAM3D_REQUIRE_EQUAL(volumes->getNumberOfTuples(), 2);
    DREAM3D_REQUIRE_EQUAL(volumes->getValue(1), 3.0f);

    return EXIT_SUCCESS;
  }

  // -----------------------------------------------------------------------------
  //
  // -----------------------------------------------------------------------------
  void operator()()
  {
    int err = EXIT_SUCCESS;
    std::cout << "---- " << getNameOfClass().toStdString() << " ----" << std::endl;

    DREAM3D_REGISTER_TEST(TestFilterAvailability());

    DREAM3D_REGISTER_TEST(TestFindTriangleGeomSizesTest())

    DREAM3D_REGISTER_TEST(RemoveTestFiles())
  }

private:
};
