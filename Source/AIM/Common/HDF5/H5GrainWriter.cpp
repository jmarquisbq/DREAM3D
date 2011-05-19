/* ============================================================================
 * Copyright (c) 2010, Michael A. Jackson (BlueQuartz Software)
 * Copyright (c) 2010, Michael A. Groeber (US Air Force Research Laboratory)
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
 * Neither the name of Michael A. Jackson nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
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
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#include "H5GrainWriter.h"

#include "AIM/Common/HDF5/VTKH5Constants.h"
#include "AIM/Common/HDF5/AIM_H5VtkDataWriter.h"
#include "AIM/Common/OIMColoring.hpp"
#include "AIM/Common/ReconstructionFunc.h"
#include "AIM/Common/GrainGeneratorFunc.h"
#include "AIM/Common/VTKWriterMacros.h"


#define H5GW_IPF_COLOR()\
if (r->crystruct[phase] == AIM::Reconstruction::Cubic) {\
  OIMColoring::GenerateIPFColor(r->m_Grains[r->voxels[i].grain_index]->euler1, r->m_Grains[r->voxels[i].grain_index]->euler2, r->m_Grains[r->voxels[i].grain_index]->euler3, RefDirection[0], RefDirection[1], RefDirection[2], rgb, hkl);\
} else if (r->crystruct[phase] == AIM::Reconstruction::Hexagonal)\
{\
  q1[1] = r->m_Grains[r->voxels[i].grain_index]->avg_quat[1];\
  q1[2] = r->m_Grains[r->voxels[i].grain_index]->avg_quat[2];\
  q1[3] = r->m_Grains[r->voxels[i].grain_index]->avg_quat[3];\
  q1[4] = r->m_Grains[r->voxels[i].grain_index]->avg_quat[4];\
  OIMColoring::CalculateHexIPFColor(q1, RefDirection[0], RefDirection[1], RefDirection[2], rgb);\
}\
ipfColor[j * 3] = rgb[0];\
ipfColor[j * 3 + 1] = rgb[1];\
ipfColor[j * 3 + 2] = rgb[2];\


#define H5GW_DECLS() \
    int err = -1;\
  AIM_H5VtkDataWriter::Pointer h5writer = AIM_H5VtkDataWriter::New();\
  h5writer->setFileName(hdfFile);\
  err = h5writer->openFile(false);\
  std::stringstream ss;\
  std::string hdfPath;\
  std::vector<std::string > hdfPaths;\
  int numgrains = r->m_Grains.size();\
  int phase;\
  int pcount = 0;\
  float q1[5];\
  unsigned char rgb[3] =  { 0, 0, 0 };\
  unsigned char hkl[3] = { 0, 0, 0 };\
  VTK_IPF_COLOR_REFDIRECTION(RefDirection);\
  int ocol, orow, oplane;\
  int col, row, plane;\
  int pid;\


#define H5GW_GRAIN_LOOP_1() \
    vector<int >* vlist = r->m_Grains[i]->voxellist;\
    int vid = vlist->at(0);\
    ss.str("");\
    ss << "/" << i;\
    hdfPath = ss.str();\
    hdfPaths.push_back(hdfPath);\
    std::vector<float > points;\
    std::vector<int32_t > cells(vlist->size() * 9);\
    std::vector<int32_t > cell_types(vlist->size(), VTK_CELLTYPE_VOXEL);\
    std::vector<int32_t > grainName(vlist->size());\
    std::vector<unsigned char > ipfColor(vlist->size() * 3);\
    std::vector<int32_t>  phaseValues(vlist->size());\
    std::map<int, int> pointMap;\
    err = 0;\
    pcount = 0;\
    size_t cIdx = 0;

#define H5GW_VLIST_LOOP_1()\
vid = vlist->at(j);\
ocol = vid % r->xpoints;\
orow = (vid / r->xpoints) % r->ypoints;\
oplane = vid / (r->xpoints * r->ypoints);\
cells[cIdx] = 8;\
++cIdx;\
for (int k = 0; k < 8; k++) {\
  if (k == 0) col = ocol, row = orow, plane = oplane;\
  if (k == 1) col = ocol + 1, row = orow, plane = oplane;\
  if (k == 2) col = ocol, row = orow + 1, plane = oplane;\
  if (k == 3) col = ocol + 1, row = orow + 1, plane = oplane;\
  if (k == 4) col = ocol, row = orow, plane = oplane + 1;\
  if (k == 5) col = ocol + 1, row = orow, plane = oplane + 1;\
  if (k == 6) col = ocol, row = orow + 1, plane = oplane + 1;\
  if (k == 7) col = ocol + 1, row = orow + 1, plane = oplane + 1;\
  pid = (plane * (r->xpoints + 1) * (r->ypoints + 1)) + (row * (r->xpoints + 1)) + col;\
  if (pointMap.find(pid) == pointMap.end())  {\
    pointMap[pid] = pcount;\
    pcount++;\
    points.push_back((col * r->resx));\
    points.push_back((row * r->resy));\
    points.push_back((plane * r->resz));\
  }\
  cells[cIdx] = pointMap[pid];\
  ++cIdx;\
}


#define H5GW_GRAIN_LOOP_2() \
err = h5writer->writeUnstructuredGrid(hdfPath, points, cells, cell_types);\
err = h5writer->writeFieldData<int> (hdfPath, grainName, AIM::Representation::Grain_ID.c_str(), 1);\
size_t size = r->m_Grains[i]->neighborlist->size();\
if (size > 0) {\
err = h5writer->writeFieldData<int> (hdfPath, *(r->m_Grains[i]->neighborlist), AIM::Representation::Neighbor_Grain_ID_List.c_str(), 1);\
}\
err = h5writer->writeCellData<int> (hdfPath, grainName, AIM::Representation::Grain_ID.c_str(), 1);\
err = h5writer->writeCellData<unsigned char> (hdfPath, ipfColor, AIM::Representation::IPFColor.c_str(), 3);\
err = h5writer->writeCellData<int32_t> (hdfPath, phaseValues, AIM::Representation::Phase.c_str(), 1);



// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
H5GrainWriter::H5GrainWriter()
{
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
H5GrainWriter::~H5GrainWriter()
{
}


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int H5GrainWriter::writeHDF5GrainsFile(ReconstructionFunc* r, const std::string &hdfFile)
{
  H5GW_DECLS()

  err = 0;

  for (int i = 1; i < numgrains; i++)
  {
    H5GW_GRAIN_LOOP_1()

    // These are Reconstruction specific arrays that we want to write
    std::vector<float > kernelAvgDisorientation(vlist->size());
    std::vector<float > grainAvgDisorientation(vlist->size());
    std::vector<float > imageQuality(vlist->size());
    std::vector<float > schmidFactor(vlist->size());

    for (std::vector<int >::size_type j = 0; j < vlist->size(); j++)
    {
      H5GW_VLIST_LOOP_1()

      phase = r->voxels[vid].phase;
      phaseValues[j] = phase;
      if (r->crystruct[phase] == AIM::Reconstruction::Cubic)
      {
        OIMColoring::GenerateIPFColor(r->voxels[vid].euler1,
                                      r->voxels[vid].euler2,
                                      r->voxels[vid].euler3,
                                      RefDirection[0], RefDirection[1], RefDirection[2],
                                      rgb, hkl);
      }
      if (r->crystruct[phase] == AIM::Reconstruction::Hexagonal)
      {
        q1[0] = r->voxels[i].quat[1];
        q1[1] = r->voxels[i].quat[2];
        q1[2] = r->voxels[i].quat[3];
        q1[3] = r->voxels[i].quat[4];
        OIMColoring::CalculateHexIPFColor(q1, RefDirection[0], RefDirection[1], RefDirection[2], rgb);
      }
      ipfColor[j * 3] = rgb[0];
      ipfColor[j * 3 + 1] = rgb[1];
      ipfColor[j * 3 + 2] = rgb[2];

      // Reconstruction Specific Assignments
      kernelAvgDisorientation[j] = r->voxels[vid].kernelmisorientation;
      grainAvgDisorientation[j] = r->voxels[vid].grainmisorientation;
      imageQuality[j] = r->voxels[vid].imagequality;
      grainName[j] = r->voxels[vid].grain_index;
      schmidFactor[j] = r->m_Grains[r->voxels[vid].grain_index]->schmidfactor;
    }
    H5GW_GRAIN_LOOP_2()

    err = h5writer->writeCellData<float> (hdfPath, kernelAvgDisorientation, AIM::Representation::KernelAvgDisorientation.c_str(), 1);
    err = h5writer->writeCellData<float> (hdfPath, grainAvgDisorientation, AIM::Representation::GrainAvgDisorientation.c_str(), 1);
    err = h5writer->writeCellData<float> (hdfPath, imageQuality, AIM::Representation::ImageQuality.c_str(), 1);
    schmidFactor[0] = r->m_Grains[i]->schmidfactor;
    err = h5writer->writeFieldData<float> (hdfPath, schmidFactor, AIM::Representation::SchmidFactor.c_str(), 1);
  }

  err = h5writer->writeObjectIndex(hdfPaths);
  err = h5writer->closeFile();
  return err;
}




// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
int H5GrainWriter::writeHDF5GrainsFile(GrainGeneratorFunc* r, const std::string &hdfFile)
{
  H5GW_DECLS()

  err = 0;

  for (int i = 1; i < numgrains; i++)
  {
    H5GW_GRAIN_LOOP_1()

    for (std::vector<int >::size_type j = 0; j < vlist->size(); j++)
    {
      H5GW_VLIST_LOOP_1()

      phase = r->voxels[vid].phase;
      phaseValues[j] = phase;
      H5GW_IPF_COLOR()

    }
    H5GW_GRAIN_LOOP_2()
  }

  err = h5writer->writeObjectIndex(hdfPaths);
  err = h5writer->closeFile();
  return err;
}

