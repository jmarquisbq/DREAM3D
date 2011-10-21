/* ============================================================================
 * Copyright (c) 2010, Michael A. Jackson (BlueQuartz Software)
 * Copyright (c) 2010, Dr. Michael A. Groeber (US Air Force Research Laboratories)
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

#include "ReconstructionFunc.h"

// C Includes
#include <stdio.h>

// C++ Includes
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <set>

// TSL Ang Library Includes
#include "EbsdLib/TSL/AngReader.h"

//-- MXA Includes
#include "MXA/Common/MXAEndian.h"
#include "MXA/Utilities/MXADir.h"

//-- AIM Includes
#include "DREAM3DLib/Common/DREAM3DMath.h"
#include "DREAM3DLib/Common/Constants.h"
#include "DREAM3DLib/Common/OIMColoring.hpp"
#include "DREAM3DLib/HDF5/AIM_H5VtkDataWriter.h"


#define NEW_SHARED_ARRAY(var, type, size)\
  boost::shared_array<type> var##Array(new type[size]);\
  type* var = var##Array.get();

const static float m_pi = M_PI;
//const static float m_OnePointThree = 1.33333333333;
//const static float threesixty_over_pi = 360.0 / m_pi;
//const static float m_pi_over_180 = M_PI / 180.0;
const static float sqrt_two = powf(2.0, 0.5);
const static float acos_neg_one = acos(-1.0);
const static float acos_pos_one = acos(1.0);
const static float sin_wmin_neg_1_over_2 = sinf(acos_neg_one / 2.0);
const static float sin_wmin_pos_1_over_2 = sinf(acos_pos_one / 2.0);

#define DIMS "DIMENSIONS"
#define LOOKUP "LOOKUP_TABLE"

#if 0
// -i C:\Users\GroebeMA\Desktop\NewFolder --outputDir C:\Users\GroebeMA\Desktop\NewFolder -f Slice_ --angMaxSlice 400 -s 1 -e 30 -z 0.25 -t -g 10 -c 0.1 -o 5.0 -x 2
#endif

using namespace std;

ReconstructionFunc::ReconstructionFunc() :
resx(1.0f),
resy(1.0f),
resz(1.0f),
xpoints(0),
ypoints(0),
zpoints(0)

{
  m_HexOps = HexagonalOps::New();
  m_OrientationOps.push_back(dynamic_cast<OrientationMath*>(m_HexOps.get()));

  m_CubicOps  = CubicOps::New();
  m_OrientationOps.push_back(dynamic_cast<OrientationMath*>(m_CubicOps.get()));

  m_OrthoOps  = OrthoRhombicOps::New();
  m_OrientationOps.push_back(dynamic_cast<OrientationMath*>(m_OrthoOps.get()));

  // Just to quiet the compiler
  float  f = sqrt_two;
  f = sin_wmin_neg_1_over_2;
  f = sin_wmin_pos_1_over_2;

  grain_indicies = NULL;
  phases = NULL;
  euler1s = NULL;
  euler2s = NULL;
  euler3s = NULL;
  neighbors = NULL;
  surfacevoxels = NULL;
  quats = NULL;
  alreadychecked = NULL;
  graincounts = NULL;

  goodVoxels = NULL;


  INIT_AIMARRAY(m_GrainIndicies,int);
  INIT_AIMARRAY(m_Phases,int);
  INIT_AIMARRAY(m_Euler1s,float);
  INIT_AIMARRAY(m_Euler2s,float);
  INIT_AIMARRAY(m_Euler3s,float);
  INIT_AIMARRAY(m_Neighbors,int);
  INIT_AIMARRAY(m_SurfaceVoxels,char);
  INIT_AIMARRAY(m_GoodVoxels,bool);
  INIT_AIMARRAY(m_Quats,float);
  INIT_AIMARRAY(m_AlreadyChecked,bool);
  INIT_AIMARRAY(m_GrainCounts,int);
  INIT_AIMARRAY(m_GoodVoxels,bool);

}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ReconstructionFunc::~ReconstructionFunc()
{
  // std::cout << "~ReconstructionFunc" << std::endl;
  m_Grains.clear();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ReconstructionFunc::initialize(int nX, int nY, int nZ, float xRes, float yRes, float zRes,
                                    bool mrgTwins, bool mrgColonies, int minAllowedGrSize,
                                    float dwnSmplFact, float misoTol, std::vector<Ebsd::CrystalStructure> crystalStructures,
                                    std::vector<DREAM3D::Reconstruction::PhaseType> phaseTypes,
                                    std::vector<float> precipFractions, int alignmentMethod)
{
  notify("Initializing Variables", 0, Observable::UpdateProgressValueAndMessage);

  mergetwinsoption = (mrgTwins == true) ? 1 : 0;
  mergecoloniesoption = (mrgColonies == true) ? 1 : 0;
  minallowedgrainsize = minAllowedGrSize;
  downsamplefactor = dwnSmplFact;
  misorientationtolerance = misoTol;
  crystruct = crystalStructures;
  phaseType = phaseTypes;
  pptFractions = precipFractions;
  alignmeth = alignmentMethod;

  totalpoints = -1;
  xpoints = nX;
  ypoints = nY;
  zpoints = nZ;
  resx = xRes;
  resy = yRes;
  resz = zRes;

  sizex = (xpoints - 1) * resx;
  sizey = (ypoints - 1) * resy;
  sizez = (zpoints - 1) * resz;

  totalpoints = xpoints * ypoints * zpoints;

  int numgrains = 100;
  size_t oldSize = 0;
  m_Grains.resize(numgrains + 1);
  for(size_t g = oldSize; g < m_Grains.size(); ++g)
  {
    m_Grains[g] = Grain::New();
  }

  grain_indicies = m_GrainIndicies->WritePointer(0, totalpoints);
  phases = m_Phases->WritePointer(0, totalpoints);
  euler1s = m_Euler1s->WritePointer(0, totalpoints);
  euler2s = m_Euler2s->WritePointer(0, totalpoints);
  euler3s = m_Euler3s->WritePointer(0, totalpoints);
  neighbors = m_Neighbors->WritePointer(0, totalpoints);
  surfacevoxels = m_SurfaceVoxels->WritePointer(0, totalpoints);
  quats = m_Quats->WritePointer(0, totalpoints*5);
  m_Quats->SetNumberOfComponents(5);

  alreadychecked = m_AlreadyChecked->WritePointer(0, totalpoints);

  goodVoxels = m_GoodVoxels->WritePointer(0, totalpoints);

  for(int i=0;i<totalpoints;i++)
  {
    grain_indicies[i] = -1;
    phases[i] = 1;
    euler1s[i] = -1;
    euler2s[i] = -1;
    euler3s[i] = -1;
    neighbors[i] = -1;
    surfacevoxels[i] = 0;
    alreadychecked[i] = false;
    goodVoxels[i] = false; // All Voxels are "Bad"
  }

}

void ReconstructionFunc::initializeQuats()
{
  float qr[5];
  Ebsd::CrystalStructure xtal = Ebsd::UnknownCrystalStructure;
  int phase = -1;
  for (int i = 0; i < (xpoints * ypoints * zpoints); i++)
  {
    OrientationMath::eulertoQuat(qr, euler1s[i], euler2s[i], euler3s[i]);
    phase = phases[i];
    xtal = crystruct[phase];
    if (xtal == Ebsd::UnknownCrystalStructure)
    {
      qr[1] = 0.0;
      qr[2] = 0.0;
      qr[3] = 0.0;
      qr[4] = 1.0;
    }
    else
    {
      m_OrientationOps[xtal]->getFZQuat(qr);
    }

    quats[i*5 + 0] = 1.0f;
    quats[i*5 + 1] = qr[1];
    quats[i*5 + 2] = qr[2];
    quats[i*5 + 3] = qr[3];
    quats[i*5 + 4] = qr[4];
  }
}
void ReconstructionFunc::cleanup_data()
{
  int bestneighbor;
  int x, y, z;
  int neighpoint;
  int good = 0;
  int count = 1;
  int neighpoints[6];
  neighpoints[0] = -(xpoints * ypoints);
  neighpoints[1] = -xpoints;
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = xpoints;
  neighpoints[5] = (xpoints * ypoints);
  while (count != 0)
  {
    count = 0;
    for (int i = 0; i < (xpoints * ypoints * zpoints); i++)
    {
      if (grain_indicies[i] == -1 && goodVoxels[i] == false)
      {
		bestneighbor = -1;
        x = i % xpoints;
        y = (i / xpoints) % ypoints;
        z = i / (xpoints * ypoints);
        for (int j = 0; j < 6; j++)
        {
          good = 1;
          neighpoint = i + neighpoints[j];
          if (j == 0 && z == 0) good = 0;
          if (j == 5 && z == (zpoints - 1)) good = 0;
          if (j == 1 && y == 0) good = 0;
          if (j == 4 && y == (ypoints - 1)) good = 0;
          if (j == 2 && x == 0) good = 0;
          if (j == 3 && x == (xpoints - 1)) good = 0;
          if (good == 1)
          {
            if (goodVoxels[neighpoint] == true)
            {
              count++;
              bestneighbor = neighpoint;
            }
          }
        }
        if (bestneighbor >= 0)
        {
          neighbors[i] = bestneighbor;
        }
      }
    }
    for (int j = 0; j < (xpoints * ypoints * zpoints); j++)
    {
      if (neighbors[j] >= 0 && goodVoxels[j] == false)
      {
	    bestneighbor = neighbors[j];
        euler1s[j] = euler1s[bestneighbor];
        euler2s[j] = euler2s[bestneighbor];
        euler3s[j] = euler3s[bestneighbor];
        goodVoxels[j] = goodVoxels[bestneighbor];
		phases[j] = phases[bestneighbor];
        quats[j*5 + 0] = quats[bestneighbor*5 + 0];
        quats[j*5 + 1] = quats[bestneighbor*5 + 1];
        quats[j*5 + 2] = quats[bestneighbor*5 + 2];
        quats[j*5 + 3] = quats[bestneighbor*5 + 3];
        quats[j*5 + 4] = quats[bestneighbor*5 + 4];
        neighbors[j] = -1;
      }
    }
  }
/*  float q1[5];
  float q2[5];
  float qtot[5];
  int numVoxel;
  int col, row, plane;
  int steps = 1;
  int jStride;
  int kStride;
  int neighbor, good;
  float w, n1, n2, n3;
  for (int i = 0; i < totalpoints; i++)
  {
	if (voxels[i].grainname != 0)
    {
	  for(int j=0;j<5;j++)
	  {
		qtot[j] = 0.0;
	  }
      numVoxel = 0;
      q1[0] = 1.0;
      q1[1] = voxels[i].quat[1];
      q1[2] = voxels[i].quat[2];
      q1[3] = voxels[i].quat[3];
      q1[4] = voxels[i].quat[4];
	  for(int m=0;m<5;m++)
	  {
		  qtot[m] = qtot[m] + q1[m];
	  }
	  numVoxel++;
      col = i % xpoints;
      row = (i / xpoints) % ypoints;
      plane = i / (xpoints * ypoints);
      for (int j = -steps; j < steps + 1; j++)
      {
        jStride = j * xpoints * ypoints;
        for (int k = -steps; k < steps + 1; k++)
        {
          kStride = k * xpoints;
          for (int l = -steps; l < steps + 1; l++)
          {
            good = 1;
            neighbor = i + (jStride) + (kStride) + (l);
            if (plane + j < 0) good = 0;
            if (plane + j > zpoints - 1) good = 0;
            if (row + k < 0) good = 0;
            if (row + k > ypoints - 1) good = 0;
            if (col + l < 0) good = 0;
            if (col + l > xpoints - 1) good = 0;
            if (good == 1)
            {
              q2[0] = 1.0;
              q2[1] = voxels[neighbor].quat[1];
              q2[2] = voxels[neighbor].quat[2];
              q2[3] = voxels[neighbor].quat[3];
              q2[4] = voxels[neighbor].quat[4];
              if (crystruct == Ebsd::Hexagonal) w = OrientationMath::getMisoQuatHexagonal(q1, q2, n1, n2, n3);
              if (crystruct == Ebsd::Cubic) w = OrientationMath::getMisoQuatCubic(q1, q2, n1, n2, n3);
              if(w < 5)
			  {
				  if (crystruct == Ebsd::Cubic) OrientationMath::getNearestQuatCubic(q1, q2);
				  for(int m=0;m<5;m++)
				  {
					  qtot[m] = qtot[m] + q2[m];
				  }
				  numVoxel++;
			  }
			}
		  }
		}
	  }
	  for(int m=0;m<5;m++)
	  {
		  qtot[m] = qtot[m]/numVoxel;
		  voxels[i].quat[m] = qtot[m];
	  }
	}
  }*/
}
void ReconstructionFunc::find_border()
{
  int neighpoints[6];
  neighpoints[0] = -(xpoints * ypoints);
  neighpoints[1] = -xpoints;
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = xpoints;
  neighpoints[5] = (xpoints * ypoints);
  float w, n1, n2, n3;
  float q1[5];
  float q2[5];
  int index;
  int good = 0;
  size_t count = 0;
  int currentpoint = 0;
  int neighbor = 0;
  int col, row, plane;
  int noborder = 0;
  Ebsd::CrystalStructure phase1, phase2;
  int initialVoxelsListSize = 10000;
  std::vector<int> voxelslist(initialVoxelsListSize, -1);
  size_t totalPoints = xpoints * ypoints * zpoints;

  AIMArray<bool>::Pointer checkedPtr = AIMArray<bool>::CreateArray(totalPoints);
  bool *checked = checkedPtr->GetPointer(0);

  for (int iter = 0; iter < (totalPoints); iter++)
  {
    checked[iter] = false;
  }
  index = 0;

  while (goodVoxels[index] == true)
  {
    index++;
    if(index == totalpoints) noborder = 1;
  }
  if(noborder == 0)
  {
	  voxelslist[count] = index;
	  grain_indicies[index] = 0;
	  checked[index] = true;
	  count++;
	  for (size_t i = 0; i < count; i++)
	  {
		index = voxelslist[i];
		col = index % xpoints;
		row = (index / xpoints) % ypoints;
		plane = index / (xpoints * ypoints);
		for (int j = 0; j < 6; j++)
		{
		  good = 1;
		  neighbor = index + neighpoints[j];
		  if (j == 0 && plane == 0) good = 0;
		  if (j == 5 && plane == (zpoints - 1)) good = 0;
		  if (j == 1 && row == 0) good = 0;
		  if (j == 4 && row == (ypoints - 1)) good = 0;
		  if (j == 2 && col == 0) good = 0;
		  if (j == 3 && col == (xpoints - 1)) good = 0;
		  if (good == 1 && checked[neighbor] == false)
		  {
			if (goodVoxels[neighbor] == false)
			{
			  grain_indicies[neighbor] = 0;
			  checked[neighbor] = true;
			  voxelslist[count] = neighbor;
			  count++;
			  if (count >= voxelslist.size()) voxelslist.resize(count + initialVoxelsListSize, -1);
			}
		  }
		}
	  }
	  voxelslist.clear();
	  voxelslist.resize(initialVoxelsListSize, -1);
	  count = 0;
	  for (int iter = 0; iter < (xpoints * ypoints * zpoints); iter++)
	  {
		checked[iter] = false;
		if(grain_indicies[iter] == -1)
		{
			voxelslist[count] = iter;
			checked[iter] = true;
			count++;
			if (count >= voxelslist.size()) voxelslist.resize(count + initialVoxelsListSize, -1);
 		}
	  }
	  for (size_t j = 0; j < count; j++)
	  {
		currentpoint = voxelslist[j];
		col = currentpoint % xpoints;
		row = (currentpoint / xpoints) % ypoints;
		plane = currentpoint / (xpoints * ypoints);
		q1[0] = 0;
		q1[1] = quats[currentpoint*5 + 1];
		q1[2] = quats[currentpoint*5 + 2];
		q1[3] = quats[currentpoint*5 + 3];
		q1[4] = quats[currentpoint*5 + 4];
		phase1 = crystruct[phases[currentpoint]];
		for (int i = 0; i < 6; i++)
		{
		  good = 1;
		  neighbor = currentpoint + neighbors[i];
		  if (i == 0 && plane == 0) good = 0;
		  if (i == 5 && plane == (zpoints - 1)) good = 0;
		  if (i == 1 && row == 0) good = 0;
		  if (i == 4 && row == (ypoints - 1)) good = 0;
		  if (i == 2 && col == 0) good = 0;
		  if (i == 3 && col == (xpoints - 1)) good = 0;
		  if (good == 1 && grain_indicies[neighbor] == -1 && checked[neighbor] == false)
		  {
			voxelslist[count] = neighbor;
			checked[neighbor] = true;
			count++;
			if (count >= voxelslist.size()) voxelslist.resize(count + initialVoxelsListSize, -1);
		  }
		  if (good == 1 && grain_indicies[neighbor] == 0)
		  {
			w = 10000.0;
			q2[0] = 0;
			q2[1] = quats[neighbor*5 + 1];
			q2[2] = quats[neighbor*5 + 2];
			q2[3] = quats[neighbor*5 + 3];
			q2[4] = quats[neighbor*5 + 4];
			phase2 = crystruct[phases[neighbor]];
			if (phase1 == phase2)
			{
			  w = m_OrientationOps[phase1]->getMisoQuat( q1, q2, n1, n2, n3);
			}
			if (w < misorientationtolerance)
			{
			  grain_indicies[neighbor] = -1;
			  checked[neighbor] = true;
			  voxelslist[count] = neighbor;
			  count++;
			  if (count >= voxelslist.size()) voxelslist.resize(count + initialVoxelsListSize, -1);
			}
		  }
		}
	  }
  }
  voxelslist.clear();
}

void ReconstructionFunc::align_sections()
{
  float disorientation = 0;
  float mindisorientation = 100000000;
  float **mutualinfo12;
  float *mutualinfo1;
  float *mutualinfo2;
  int graincount1, graincount2;
  int xshift = 0;
  int yshift = 0;
  int newxshift = 0;
  int newyshift = 0;
  int oldxshift = 0;
  int oldyshift = 0;
  int count = 0;
  int slice = 0;
  int xspot, yspot;
  float w;
  float n1, n2, n3;
  float q1[5];
  float q2[5];
  float refxcentroid, refycentroid;
  float curxcentroid, curycentroid;
  int refgnum, curgnum;
  int refposition = 0;
  int curposition = 0;
  int position;
  int tempposition;
  Ebsd::CrystalStructure phase1, phase2;

  int** shifts = new int *[zpoints];
  for (int a = 0; a < zpoints; a++)
  {
    shifts[a] = new int[2];
    for (int b = 0; b < 2; b++)
    {
      shifts[a][b] = 0;
    }
  }
  int** misorients = new int *[xpoints];
  for (int a = 0; a < xpoints; a++)
  {
    misorients[a] = new int[ypoints];
    for (int b = 0; b < ypoints; b++)
    {
      misorients[a][b] = 0;
    }
  }
  if (alignmeth == DREAM3D::Reconstruction::OuterBoundary)
  {
    refxcentroid = 0;
    refycentroid = 0;
    count = 0;
    slice = (zpoints - 1);
    for (int l = 0; l < ypoints; l++)
    {
      for (int m = 0; m < xpoints; m++)
      {
        refposition = ((slice) * xpoints * ypoints) + (l * xpoints) + m;
        refxcentroid = refxcentroid + (((resx * m) + (resx / 2.0)) * (float(grain_indicies[refposition])));
        refycentroid = refycentroid + (((resy * l) + (resy / 2.0)) * (float(grain_indicies[refposition])));
        count = count + grain_indicies[refposition];
      }
    }
    refxcentroid = refxcentroid / float(count);
    refycentroid = refycentroid / float(count);
  }
  for (int iter = 1; iter < zpoints; iter++)
  {
    mindisorientation = 100000000;
    slice = (zpoints - 1) - iter;
    if (alignmeth == DREAM3D::Reconstruction::MutualInformation)
    {
      graincount1 = graincounts[slice];
      graincount2 = graincounts[slice + 1];
      mutualinfo12 = new float *[graincount1];
      mutualinfo1 = new float[graincount1];
      mutualinfo2 = new float[graincount2];
      for (int a = 0; a < graincount1; a++)
      {
        mutualinfo1[a] = 0;
        mutualinfo12[a] = new float[graincount2];
        for (int b = 0; b < graincount2; b++)
        {
          mutualinfo12[a][b] = 0;
          mutualinfo2[b] = 0;
        }
      }
    }
    if (alignmeth == DREAM3D::Reconstruction::OuterBoundary)
    {
      curxcentroid = 0;
      curycentroid = 0;
      count = 0;
      slice = (zpoints - 1) - iter;
      for (int l = 0; l < ypoints; l++)
      {
        for (int m = 0; m < xpoints; m++)
        {
          curposition = ((slice) * xpoints * ypoints) + (l * xpoints) + m;
          curxcentroid = curxcentroid + (((resx * m) + (resx / 2.0)) * (float(grain_indicies[curposition])));
          curycentroid = curycentroid + (((resy * l) + (resy / 2.0)) * (float(grain_indicies[curposition])));
          count = count + grain_indicies[curposition];
        }
      }
      curxcentroid = curxcentroid / float(count);
      curycentroid = curycentroid / float(count);
    }
	if (alignmeth == DREAM3D::Reconstruction::Misorientation  || alignmeth == DREAM3D::Reconstruction::MutualInformation)
    {
      oldxshift = -1;
      oldyshift = -1;
      newxshift = 0;
	  newyshift = 0;
	  for (int a = 0; a < xpoints; a++)
	  {
		for (int b = 0; b < ypoints; b++)
		{
		  misorients[a][b] = 0;
		}
	  }
      while(newxshift != oldxshift || newyshift != oldyshift)
      {
	    oldxshift = newxshift;
	    oldyshift = newyshift;
        for (int j = -3; j < 4; j++)
        {
          for (int k = -3; k < 4; k++)
          {
            disorientation = 0;
            count = 0;
			if(misorients[k+oldxshift+int(xpoints/2)][j+oldyshift+int(ypoints/2)] == 0 && abs(k+oldxshift) < (xpoints/2) && (j+oldyshift) < (ypoints/2))
			{
				for (int l = 0; l < ypoints; l=l+4)
				{
				  for (int m = 0; m < xpoints; m=m+4)
				  {
					count++;
					if ((l + j + oldyshift) >= 0 && (l + j + oldyshift) < ypoints && (m + k + oldxshift) >= 0 && (m + k + oldxshift) < xpoints)
					{
					  refposition = ((slice + 1) * xpoints * ypoints) + (l * xpoints) + m;
					  curposition = (slice * xpoints * ypoints) + ((l + j + oldyshift) * xpoints) + (m + k + oldxshift);
					  refgnum = grain_indicies[refposition];
					  curgnum = grain_indicies[curposition];
					  if (alignmeth == DREAM3D::Reconstruction::MutualInformation)
					  {
						if (curgnum >= 0 && refgnum >= 0)
						{
						  mutualinfo12[curgnum][refgnum]++;
						  mutualinfo1[curgnum]++;
						  mutualinfo2[refgnum]++;
						}
					  }
					  if (alignmeth == DREAM3D::Reconstruction::Misorientation)
					  {
						if (goodVoxels[refposition] == true && goodVoxels[curposition] == true)
						{
						  w = 10000.0;
						  q1[1] = quats[refposition*5 + 1];
						  q1[2] = quats[refposition*5 + 2];
						  q1[3] = quats[refposition*5 + 3];
						  q1[4] = quats[refposition*5 + 4];
						  phase1 = crystruct[phases[refposition]];
						  q2[1] = quats[curposition*5 + 1];
						  q2[2] = quats[curposition*5 + 2];
						  q2[3] = quats[curposition*5 + 3];
						  q2[4] = quats[curposition*5 + 4];
						  phase2 = crystruct[phases[curposition]];
						  if (phase1 == phase2) w = m_OrientationOps[phase1]->getMisoQuat( q1, q2, n1, n2, n3);
						  if (w > misorientationtolerance) disorientation++;
						}
						if (goodVoxels[refposition] == true && goodVoxels[curposition] == false) disorientation++;
						if (goodVoxels[refposition] == false && goodVoxels[curposition] == true) disorientation++;
					  }
					}
					else
					{
					  if (alignmeth == DREAM3D::Reconstruction::MutualInformation)
					  {
						mutualinfo12[0][0]++;
						mutualinfo1[0]++;
						mutualinfo2[0]++;
					  }
					}
				  }
				}
				if (alignmeth == DREAM3D::Reconstruction::MutualInformation)
				{
				  float ha = 0;
				  float hb = 0;
				  float hab = 0;
				  for (int b = 0; b < graincount1; b++)
				  {
					mutualinfo1[b] = mutualinfo1[b] / float(count);
					if (mutualinfo1[b] != 0) ha = ha + mutualinfo1[b] * log(mutualinfo1[b]);
				  }
				  for (int c = 0; c < graincount2; c++)
				  {
					mutualinfo2[c] = mutualinfo2[c] / float(count);
					if (mutualinfo2[c] != 0) hb = hb + mutualinfo2[c] * log(mutualinfo2[c]);
				  }
				  for (int b = 0; b < graincount1; b++)
				  {
					for (int c = 0; c < graincount2; c++)
					{
					  mutualinfo12[b][c] = mutualinfo12[b][c] / float(count);
					  if (mutualinfo12[b][c] != 0) hab = hab + mutualinfo12[b][c] * log(mutualinfo12[b][c]);
					  float value = 0;
					  if (mutualinfo1[b] > 0 && mutualinfo2[c] > 0) value = (mutualinfo12[b][c] / (mutualinfo1[b] * mutualinfo2[c]));
					  if (value != 0) disorientation = disorientation + (mutualinfo12[b][c] * log(value));
					}
				  }
				  for (int b = 0; b < graincount1; b++)
				  {
					for (int c = 0; c < graincount2; c++)
					{
					  mutualinfo12[b][c] = 0;
					  mutualinfo1[b] = 0;
					  mutualinfo2[c] = 0;
					}
				  }
				  //    float disorientation2 = ha + hb - hab;
				  disorientation = 1.0 / disorientation;
				}
				misorients[k+oldxshift+int(xpoints/2)][j+oldyshift+int(ypoints/2)] = disorientation;
				if (disorientation < mindisorientation)
				{
				  newxshift = k + oldxshift;
				  newyshift = j + oldyshift;
				  mindisorientation = disorientation;
				}
			}
          }
        }
      }
      shifts[iter][0] = shifts[iter - 1][0] + newxshift;
      shifts[iter][1] = shifts[iter - 1][1] + newyshift;
    }
    if (alignmeth == DREAM3D::Reconstruction::OuterBoundary)
    {
      xshift = int(((curxcentroid - refxcentroid) / resx) + 0.5);
      yshift = int(((curycentroid - refycentroid) / resy) + 0.5);
      shifts[iter][0] = xshift;
      shifts[iter][1] = yshift;
    }
  }
  for (int iter = 1; iter < zpoints; iter++)
  {
    slice = (zpoints - 1) - iter;
    for (int l = 0; l < ypoints; l++)
    {
      for (int m = 0; m < xpoints; m++)
      {
        if (shifts[iter][1] >= 0) yspot = l;
        if (shifts[iter][0] >= 0) xspot = m;
        if (shifts[iter][1] < 0) yspot = ypoints - 1 - l;
        if (shifts[iter][0] < 0) xspot = xpoints - 1 - m;
        position = (slice * xpoints * ypoints) + (yspot * xpoints) + xspot;
        tempposition = (slice * xpoints * ypoints) + ((yspot + shifts[iter][1]) * xpoints) + (xspot + shifts[iter][0]);
        if ((yspot + shifts[iter][1]) >= 0
            && (yspot + shifts[iter][1]) <= ypoints - 1
            && (xspot + shifts[iter][0]) >= 0
            && (xspot + shifts[iter][0]) <= xpoints - 1)
        {
          euler1s[position] = euler1s[tempposition];
          euler2s[position] = euler2s[tempposition];
          euler3s[position] = euler3s[tempposition];
          quats[position*5 + 0] = quats[tempposition*5 + 0];
          quats[position*5 + 1] = quats[tempposition*5 + 1];
          quats[position*5 + 2] = quats[tempposition*5 + 2];
          quats[position*5 + 3] = quats[tempposition*5 + 3];
          quats[position*5 + 4] = quats[tempposition*5 + 4];
          goodVoxels[position] = goodVoxels[tempposition];
		  phases[position] = phases[tempposition];
          grain_indicies[position] = grain_indicies[tempposition];
        }
        if ((yspot + shifts[iter][1]) < 0
            || (yspot + shifts[iter][1]) > ypoints - 1
            || (xspot + shifts[iter][0]) < 0
            || (xspot + shifts[iter][0]) > xpoints - 1)
        {
          euler1s[position] = 0.0;
          euler2s[position] = 0.0;
          euler3s[position] = 0.0;
          quats[position*5 + 0] = 0.0;
          quats[position*5 + 1] = 0.0;
          quats[position*5 + 2] = 0.0;
          quats[position*5 + 3] = 0.0;
          quats[position*5 + 4] = 1.0;
          goodVoxels[position] = false;
		  phases[position] = 0;
          grain_indicies[position] = 0;
        }
      }
    }
  }

  // Clean up the memory
  for (int a = 0; a < zpoints; a++)
  {
    delete [] shifts[a];
  }
  delete [] shifts;
}
void ReconstructionFunc::form_grains_sections()
{
  DREAM3D_RANDOMNG_NEW()

  int point = 0;
  int seed = 0;
  int noseeds = 0;
  int graincount = 1;
  int neighbor;
  // int currentpoint;
  float q1[5];
  float q2[5];
  float qs[5];
  float w;
  float n1;
  float n2;
  float n3;
  int randx = 0;
  int randy = 0;
  int good = 0;
  int x, y, z;
  int col, row;
  size_t size = 0;
  size_t initialVoxelsListSize = 1000;

  graincounts = m_GrainCounts->WritePointer(0, zpoints);

  std::vector<int > voxelslist(initialVoxelsListSize, -1);
  int neighpoints[8];
  neighpoints[0] = -xpoints - 1;
  neighpoints[1] = -xpoints;
  neighpoints[2] = -xpoints + 1;
  neighpoints[3] = -1;
  neighpoints[4] = 1;
  neighpoints[5] = xpoints - 1;
  neighpoints[6] = xpoints;
  neighpoints[7] = xpoints + 1;
  Ebsd::CrystalStructure phase1, phase2;
  for (int slice = 0; slice < zpoints; slice++)
  {
    graincount = 1;
    noseeds = 0;
    while (noseeds == 0)
    {
      seed = -1;
      randx = int(float(rg.genrand_res53()) * float(xpoints));
      randy = int(float(rg.genrand_res53()) * float(ypoints));
      for (int j = 0; j < ypoints; ++j)
      {
        for (int i = 0; i < xpoints; ++i)
        {
          x = randx + i;
          y = randy + j;
          z = slice;
          if (x > xpoints - 1) x = x - xpoints;
          if (y > ypoints - 1) y = y - ypoints;
          point = (z * xpoints * ypoints) + (y * xpoints) + x;
          if (goodVoxels[point] == true && grain_indicies[point] == -1)
          {
            seed = point;
          }
          if (seed > -1)
          {
            break;
          }
        }
        if (seed > -1)
        {
          break;
        }
      }
      if (seed == -1)
      {
        noseeds = 1;
      }
      if (seed >= 0)
      {
        size = 0;
        grain_indicies[seed] = graincount;
        voxelslist[size] = seed;
        size++;
        qs[0] = 0;
        qs[1] = quats[seed*5 + 1];
        qs[2] = quats[seed*5 + 2];
        qs[3] = quats[seed*5 + 3];
        qs[4] = quats[seed*5 + 4];
        for (size_t j = 0; j < size; ++j)
        {
          int currentpoint = voxelslist[j];
          col = currentpoint % xpoints;
          row = (currentpoint / xpoints) % ypoints;
          q1[0] = 0;
          q1[1] = quats[currentpoint*5 + 1];
          q1[2] = quats[currentpoint*5 + 2];
          q1[3] = quats[currentpoint*5 + 3];
          q1[4] = quats[currentpoint*5 + 4];
		  phase1 = crystruct[phases[currentpoint]];
          for (int i = 0; i < 8; i++)
          {
            good = 1;
            neighbor = currentpoint + neighpoints[i];
            if ((i == 0 || i == 1 || i == 2) && row == 0) good = 0;
            if ((i == 5 || i == 6 || i == 7) && row == (ypoints - 1)) good = 0;
            if ((i == 0 || i == 3 || i == 5) && col == 0) good = 0;
            if ((i == 2 || i == 4 || i == 7) && col == (xpoints - 1)) good = 0;
            if (good == 1 && grain_indicies[neighbor] <= 0)
            {
			  w = 10000.0;
              q2[0] = 0;
              q2[1] = quats[neighbor*5 + 1];
              q2[2] = quats[neighbor*5 + 2];
              q2[3] = quats[neighbor*5 + 3];
              q2[4] = quats[neighbor*5 + 4];
			  phase2 = crystruct[phases[neighbor]];
			  if (phase1 == phase2) w = m_OrientationOps[phase1]->getMisoQuat( q1, q2, n1, n2, n3);
              if (w < misorientationtolerance)
              {
                grain_indicies[neighbor] = graincount;
                voxelslist[size] = neighbor;
                size++;
                if (size >= voxelslist.size()) voxelslist.resize(size + initialVoxelsListSize, -1);
              }
            }
          }
        }
        voxelslist.erase(std::remove(voxelslist.begin(), voxelslist.end(), -1), voxelslist.end());
        graincount++;
        voxelslist.clear();
        voxelslist.resize(initialVoxelsListSize, -1);
      }
    }
    graincounts[slice] = graincount;
  }
}

void ReconstructionFunc::form_grains()
{
  DREAM3D_RANDOMNG_NEW()
  int seed = 0;
  int noseeds = 0;
  size_t graincount = 1;
  int neighbor;
  float q1[5];
  float q2[5];
  float qa[5];
  float qb[5];
  float w;
  float n1, n2, n3;
//  int point = 0;
  int randpoint = 0;
  int good = 0;
  int col, row, plane;
  size_t size = 0;
  size_t initialVoxelsListSize = 1000;
  size_t initialMergeListSize = 10;
  int vid;
  std::vector<int> voxelslist(initialVoxelsListSize, -1);
  std::vector<int>* vlist;
  std::vector<int> mergelist(initialMergeListSize, -1);
  int neighpoints[6];
  neighpoints[0] = -(xpoints * ypoints);
  neighpoints[1] = -xpoints;
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = xpoints;
  neighpoints[5] = (xpoints * ypoints);
  Ebsd::CrystalStructure phase1, phase2;

  // Precalculate some constants
  int totalPMinus1 = totalpoints - 1;


  // Copy all the grain names into a densly packed array

  // Create initial set of grain average quaternions
  size_t grainSize = 1000;
  std::vector<float> m_grainQuats(grainSize*5, 0.0);
  std::vector<int>   m_grainPhases(grainSize, 0);
  float* grainquats = &(m_grainQuats.front());
  int* gphases = &(m_grainPhases.front());

  notify("Form Grains - Growing/Aglomerating Grains", 0, Observable::UpdateProgressMessage);

  // Burn volume with tight orientation tolerance to simulate simultaneous growth/aglomeration
  while (noseeds == 0)
  {
    seed = -1;
    int counter = 0;
    randpoint = int(float(rg.genrand_res53()) * float(totalPMinus1));
    while (seed == -1 && counter < totalpoints)
    {
      if (randpoint > totalPMinus1) randpoint = randpoint - totalpoints;
      if (grain_indicies[randpoint] == -1 && goodVoxels[randpoint] == true) seed = randpoint;

      randpoint++;
      counter++;
    }
    if (seed == -1)
    {
      noseeds = 1;
    }
    if (seed >= 0)
    {
      size = 0;
      grain_indicies[seed] = graincount;
      voxelslist[size] = seed;
      size++;
      gphases[graincount] = phases[seed];
      for (int k = 0; k < 5; k++)
      {
        grainquats[graincount * 5 + k] = grainquats[graincount * 5 + k] + quats[seed*5 + k];
      }
      for (size_t j = 0; j < size; ++j)
      {
        int currentpoint = voxelslist[j];
        col = currentpoint % xpoints;
        row = (currentpoint / xpoints) % ypoints;
        plane = currentpoint / (xpoints * ypoints);
        phase1 = crystruct[phases[currentpoint]];
        for (int i = 0; i < 6; i++)
        {
          q1[0] = 1;
          q1[1] = grainquats[graincount * 5 + 1] / grainquats[graincount * 5];
          q1[2] = grainquats[graincount * 5 + 2] / grainquats[graincount * 5];
          q1[3] = grainquats[graincount * 5 + 3] / grainquats[graincount * 5];
          q1[4] = grainquats[graincount * 5 + 4] / grainquats[graincount * 5];
          good = 1;
          neighbor = currentpoint + neighpoints[i];
          if (i == 0 && plane == 0) good = 0;
          if (i == 5 && plane == (zpoints - 1)) good = 0;
          if (i == 1 && row == 0) good = 0;
          if (i == 4 && row == (ypoints - 1)) good = 0;
          if (i == 2 && col == 0) good = 0;
          if (i == 3 && col == (xpoints - 1)) good = 0;
          if (good == 1 && grain_indicies[neighbor] == -1)
          {
            w = 10000.0;
            q2[0] = 1;
            q2[1] = quats[neighbor*5 + 1];
            q2[2] = quats[neighbor*5 + 2];
            q2[3] = quats[neighbor*5 + 3];
            q2[4] = quats[neighbor*5 + 4];
            phase2 = crystruct[phases[neighbor]];
            if (phase1 == phase2) w = m_OrientationOps[phase1]->getMisoQuat( q1, q2, n1, n2, n3);
            if (w < 5.0)
            {
              grain_indicies[neighbor] = graincount;
              for (int k = 0; k < 5; k++)
              {
                qa[k] = quats[seed*5 + k];
                qb[k] = quats[neighbor*5 + k];
              }
              if (phase1 == phase2) m_OrientationOps[phase1]->getNearestQuat(qa, qb);
              for (int k = 0; k < 5; k++)
              {
                quats[neighbor*5 + k] = qb[k];
                grainquats[graincount * 5 + k] = grainquats[graincount * 5 + k] + quats[neighbor*5 + k];
              }
              voxelslist[size] = neighbor;
              size++;
              if (size >= voxelslist.size()) voxelslist.resize(size + initialVoxelsListSize, -1);
            }
          }
        }
      }
      voxelslist.erase(std::remove(voxelslist.begin(), voxelslist.end(), -1), voxelslist.end());
      if (m_Grains[graincount]->voxellist != NULL)
      {
        delete m_Grains[graincount]->voxellist;
      }
      m_Grains[graincount]->voxellist = new std::vector<int>(voxelslist.size());
      m_Grains[graincount]->voxellist->swap(voxelslist);
      m_Grains[graincount]->active = 1;
      m_Grains[graincount]->phase = phases[seed];
      graincount++;
      if (graincount >= m_Grains.size())
      {
        size_t oldSize = m_Grains.size();
        m_Grains.resize(m_Grains.size() + 100);
        for (size_t g = oldSize; g < m_Grains.size(); ++g)
        {
          m_Grains[g] = Grain::New();
        }
      }
      if (graincount >= grainSize)
      {
        // This will allocate a new array and copy all the old values to the new array
        m_grainQuats.resize((graincount + 100) * 5, 0.0);
        grainquats = &(m_grainQuats.front());
        m_grainPhases.resize((graincount + 100), 0);
        gphases = &(m_grainPhases.front());
      }
      voxelslist.clear();
      voxelslist.resize(initialVoxelsListSize, -1);
    }
  }

  notify("Form Grains -Merging Grains", 0, Observable::UpdateProgressMessage);

  SharedIntArray mergedgrainNames(new int[graincount]);
  int* mergedgrain_indicies = mergedgrainNames.get();
  SharedIntArray newgrainNames(new int[graincount]);
  int* newgrain_indicies = newgrainNames.get();

  for(size_t i=0;i<graincount;i++)
  {
   mergedgrain_indicies[i] = i;
   newgrain_indicies[i] = i;
  }
  for(size_t i=1;i<graincount;i++)
  {
    if (m_Grains[i]->active == 1)
    {
      size = 0;
      mergelist[size] = i;
      size++;
      for (size_t j = 0; j < size; j++)
      {
        vlist = m_Grains[mergelist[j]]->voxellist;
        size_t vlistSize = vlist->size();
        for (size_t k = 0; k < vlistSize; k++)
        {
          vid = vlist->at(k);
          col = vid % xpoints;
          row = (vid / xpoints) % ypoints;
          plane = vid / (xpoints * ypoints);
          for (int l = 0; l < 6; l++)
          {
            good = 1;
            neighbor = vid + neighpoints[l];
            if (l == 0 && plane == 0) good = 0;
            if (l == 5 && plane == (zpoints - 1)) good = 0;
            if (l == 1 && row == 0) good = 0;
            if (l == 4 && row == (ypoints - 1)) good = 0;
            if (l == 2 && col == 0) good = 0;
            if (l == 3 && col == (xpoints - 1)) good = 0;
            if (good == 1 && grain_indicies[neighbor] != i && grain_indicies[neighbor] > 0)
            {
              if (m_Grains[grain_indicies[neighbor]]->active == 1)
              {
                w = 10000.0;
                q1[0] = grainquats[i * 5];
                q1[1] = grainquats[i * 5 + 1] / grainquats[i * 5];
                q1[2] = grainquats[i * 5 + 2] / grainquats[i * 5];
                q1[3] = grainquats[i * 5 + 3] / grainquats[i * 5];
                q1[4] = grainquats[i * 5 + 4] / grainquats[i * 5];
                phase1 = crystruct[gphases[i]];
                q2[0] = grainquats[grain_indicies[neighbor] * 5];
                q2[1] = grainquats[grain_indicies[neighbor] * 5 + 1] / grainquats[grain_indicies[neighbor] * 5];
                q2[2] = grainquats[grain_indicies[neighbor] * 5 + 2] / grainquats[grain_indicies[neighbor] * 5];
                q2[3] = grainquats[grain_indicies[neighbor] * 5 + 3] / grainquats[grain_indicies[neighbor] * 5];
                q2[4] = grainquats[grain_indicies[neighbor] * 5 + 4] / grainquats[grain_indicies[neighbor] * 5];
                phase2 = crystruct[gphases[grain_indicies[neighbor]]];
                if (phase1 == phase2) w = m_OrientationOps[phase1]->getMisoQuat( q1, q2, n1, n2, n3);
                if (w < misorientationtolerance)
                {
                  mergelist[size] = grain_indicies[neighbor];
                  size++;
                  m_Grains[grain_indicies[neighbor]]->active = 0;
                  mergedgrain_indicies[grain_indicies[neighbor]] = i;
                  if (phase1 == phase2) m_OrientationOps[phase1]->getNearestQuat(qa, qb);
                  for (int m = 0; m < 5; m++)
                  {
                    q2[m] = q2[m] * q2[0];
                    grainquats[i * 5 + m] = grainquats[i * 5 + m] + q2[m];
                  }
                  if (size >= mergelist.size()) mergelist.resize(size + initialMergeListSize, -1);
                }
              }
            }
          }
        }
      }
      mergelist.clear();
      mergelist.resize(initialMergeListSize, -1);
    }
  }

  int goodgraincount = 1;
  for(size_t i = 0; i < graincount; ++i)
  {
    if(m_Grains[i]->active == 1)
    {
    newgrain_indicies[i] = goodgraincount;
    m_Grains[goodgraincount]->active = 1;
    goodgraincount++;
    }
  }
  // Copy the grain names back into the Voxel objects
  int mergedname;
  int newname;
  for (int i = 0; i < totalpoints; ++i)
  {
    if(grain_indicies[i] >= 0)
	{
	    mergedname = mergedgrain_indicies[grain_indicies[i]];
		while(mergedgrain_indicies[mergedname] != mergedname)
		{
		  mergedname = mergedgrain_indicies[mergedname];
		}
		newname = newgrain_indicies[mergedname];
	    grain_indicies[i] = newname;
	}
  }
  size_t oldSize = m_Grains.size();
  m_Grains.resize(goodgraincount);
  for(size_t g = oldSize; g < m_Grains.size(); ++g)
  {
    m_Grains[g] = Grain::New();
  }

  notify("Form Grains - Removing Small Grains", 0, Observable::UpdateProgressMessage);

  goodgraincount = remove_smallgrains(goodgraincount);
  oldSize = m_Grains.size();
  m_Grains.resize(goodgraincount);
  for(size_t g = oldSize; g < m_Grains.size(); ++g)
  {
    m_Grains[g] = Grain::New();
  }
 // m_grainQuats = AIMArray<float >::NullPointer(); // Clean up the array to release some memory

  assign_badpoints();
}

void ReconstructionFunc::assign_badpoints()
{
  vector<int > neighs;
  vector<int > remove;
  int count = 1;
  int good = 1;
  float x, y, z;
  int neighpoint;
  int neighpoints[6];
  size_t numgrains = m_Grains.size();
  std::vector<int > n(numgrains + 1);
  neighpoints[0] = -xpoints * ypoints;
  neighpoints[1] = -xpoints;
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = xpoints;
  neighpoints[5] = xpoints * ypoints;

  notify("Assigning Bad Voxels", 0, Observable::UpdateProgressMessage);

  while (count != 0)
  {
    count = 0;
    for (int i = 0; i < totalpoints; i++)
    {
      int grainname = grain_indicies[i];
      if (grainname <= -1)
      {
        count++;
        for (size_t c = 1; c < numgrains; c++)
        {
          n[c] = 0;
        }
        x = i % xpoints;
        y = (i / xpoints) % ypoints;
        z = i / (xpoints * ypoints);
        for (int j = 0; j < 6; j++)
        {
          good = 1;
          neighpoint = i + neighpoints[j];
          if (j == 0 && z == 0) good = 0;
          if (j == 5 && z == (zpoints - 1)) good = 0;
          if (j == 1 && y == 0) good = 0;
          if (j == 4 && y == (ypoints - 1)) good = 0;
          if (j == 2 && x == 0) good = 0;
          if (j == 3 && x == (xpoints - 1)) good = 0;
          if (good == 1)
          {
            int grain = grain_indicies[neighpoint];
            if (grain >= 0)
            {
              neighs.push_back(grain);
            }
          }
        }
        int current = 0;
        int most = 0;
        int curgrain = 0;
        int size = int(neighs.size());
        for (int k = 0; k < size; k++)
        {
          int neighbor = neighs[k];
          n[neighbor]++;
          current = n[neighbor];
          if (current > most)
          {
            most = current;
            curgrain = neighbor;
          }
        }
        if (size > 0)
        {
          neighbors[i] = curgrain;
          neighs.clear();
        }
      }
    }
    for (int j = 0; j < totalpoints; j++)
    {
      int grainname = grain_indicies[j];
      int neighbor = neighbors[j];
      if (grainname <= -1 && neighbor >= 0)
      {
        grain_indicies[j] = neighbor;
        m_Grains[neighbor]->numvoxels++;
      }
    }
//    std::stringstream ss;
//     ss << "Assigning Bad Voxels count = " << count;
//    notify(ss.str().c_str(), 0, Observable::UpdateProgressMessage);
  }
}


void ReconstructionFunc::merge_containedgrains()
{
  for (int i = 0; i < (xpoints * ypoints * zpoints); i++)
  {
    int grainname = grain_indicies[i];
    if (m_Grains[grainname]->numneighbors == 1)
    {
      m_Grains[grainname]->gotcontainedmerged = true;
      grain_indicies[i] = m_Grains[grainname]->neighborlist->at(0);
      m_Grains[m_Grains[grainname]->neighborlist->at(0)]->numvoxels++;
    }
	if (m_Grains[grainname]->numneighbors == 0)
	{
      m_Grains[grainname]->gotcontainedmerged = true;
      grain_indicies[i] = 0;
	}
  }

}

void ReconstructionFunc::reorder_grains()
{
  size_t initialVoxelsListSize = 1000;
  size_t size = 0;
  int neighpoints[26];
  int good = 0;
  int neighbor = 0;
  int col, row, plane;
  int gnum;
  float q1[5];
  float q2[5];
  float q[5];
  float ea1, ea2, ea3;
  size_t currentgrain = 1;
  Ebsd::CrystalStructure phase;

  neighpoints[0] = -(xpoints * ypoints) - xpoints - 1;
  neighpoints[1] = -(xpoints * ypoints) - xpoints;
  neighpoints[2] = -(xpoints * ypoints) - xpoints + 1;
  neighpoints[3] = -(xpoints * ypoints) - 1;
  neighpoints[4] = -(xpoints * ypoints);
  neighpoints[5] = -(xpoints * ypoints) + 1;
  neighpoints[6] = -(xpoints * ypoints) + xpoints - 1;
  neighpoints[7] = -(xpoints * ypoints) + xpoints;
  neighpoints[8] = -(xpoints * ypoints) + xpoints + 1;
  neighpoints[9] = -xpoints - 1;
  neighpoints[10] = -xpoints;
  neighpoints[11] = -xpoints + 1;
  neighpoints[12] = -1;
  neighpoints[13] = 1;
  neighpoints[14] = xpoints - 1;
  neighpoints[15] = xpoints;
  neighpoints[16] = xpoints + 1;
  neighpoints[17] = (xpoints * ypoints) - xpoints - 1;
  neighpoints[18] = (xpoints * ypoints) - xpoints;
  neighpoints[19] = (xpoints * ypoints) - xpoints + 1;
  neighpoints[20] = (xpoints * ypoints) - 1;
  neighpoints[21] = (xpoints * ypoints);
  neighpoints[22] = (xpoints * ypoints) + 1;
  neighpoints[23] = (xpoints * ypoints) + xpoints - 1;
  neighpoints[24] = (xpoints * ypoints) + xpoints;
  neighpoints[25] = (xpoints * ypoints) + xpoints + 1;
  size_t numgrains = m_Grains.size();

  size_t maxGrain = 0;
  // Reset all the Grain nucleus values to -1;
  for (size_t i = 1; i < numgrains; i++)
  {
    m_Grains[i]->nucleus = -1;
	m_Grains[i]->voxellist->resize(1,0);
	m_Grains[i]->gotcontainedmerged = false;
  }

  // Reset the "already checked" to 0 for all voxels
  for (int i = 0; i < totalpoints; i++)
  {
    alreadychecked[i] = false;
    gnum = grain_indicies[i];
    m_Grains[gnum]->nucleus = i;
  }
  for (size_t i = 1; i < numgrains; i++)
  {
    if(m_Grains[i]->nucleus != -1)
    {
      size = 0;
      int nucleus = m_Grains[i]->nucleus;
	  phase = crystruct[phases[nucleus]];
      if(m_Grains[currentgrain]->voxellist == NULL)
      {
        if (NULL != m_Grains[currentgrain]->voxellist)
        {
          delete m_Grains[currentgrain]->voxellist;
          m_Grains[currentgrain]->voxellist = NULL;
        }
        m_Grains[currentgrain]->voxellist = new std::vector<int>(initialVoxelsListSize,-1);
      }
      else
      {
        m_Grains[currentgrain]->voxellist->resize(initialVoxelsListSize,-1);
      }
      m_Grains[currentgrain]->voxellist->at(size) = nucleus;
      alreadychecked[nucleus] = true;
      grain_indicies[nucleus] = currentgrain;
      if (currentgrain > maxGrain) maxGrain = currentgrain;
      size++;
      for (size_t k = 0; k < 5; k++)
      {
        m_Grains[currentgrain]->avg_quat[k] = 0.0;
      }
      for (size_t j = 0; j < size; j++)
      {
        int currentpoint = m_Grains[currentgrain]->voxellist->at(j);
        col = currentpoint % xpoints;
        row = (currentpoint / xpoints) % ypoints;
        plane = currentpoint / (xpoints * ypoints);
        for (int k = 0; k < 5; k++)
        {
            q1[k] = quats[nucleus*5 + k];
            q2[k] = quats[currentpoint*5 + k];
        }
        m_OrientationOps[phase]->getNearestQuat(q1,q2);
        for (int k = 0; k < 5; k++)
        {
            quats[currentpoint*5 + k] = q2[k];
            m_Grains[currentgrain]->avg_quat[k] = m_Grains[currentgrain]->avg_quat[k] + quats[currentpoint*5 + k];
        }
        for (int k = 0; k < 26; k++)
        {
          good = 1;
          neighbor = currentpoint + neighpoints[k];
          if (k < 9 && plane == 0) good = 0;
          if (k > 16 && plane == (zpoints - 1)) good = 0;
          if ((k == 0 || k == 1 || k == 2 || k == 9 || k == 10 || k == 11 || k == 17 || k == 18 || k == 19) && row == 0) good = 0;
          if ((k == 6 || k == 7 || k == 8 || k == 14 || k == 15 || k == 16 || k == 23 || k == 24 || k == 25) && row == (ypoints - 1)) good = 0;
          if ((k == 0 || k == 3 || k == 6 || k == 9 || k == 12 || k == 14 || k == 17 || k == 20 || k == 23) && col == 0) good = 0;
          if ((k == 2 || k == 5 || k == 8 || k == 11 || k == 13 || k == 16 || k == 19 || k == 22 || k == 25) && col == (xpoints - 1)) good = 0;
          if (good == 1 && alreadychecked[neighbor] == false)
          {
            size_t grainname = grain_indicies[neighbor];
            if (grainname == i)
            {
              m_Grains[currentgrain]->voxellist->at(size) = neighbor;
              alreadychecked[neighbor] = true;
              grain_indicies[neighbor] = currentgrain;
              if (currentgrain > maxGrain) maxGrain = currentgrain;
              size++;
              if (size >= m_Grains[currentgrain]->voxellist->size())
              {
                m_Grains[currentgrain]->voxellist->resize(size + initialVoxelsListSize, -1);
              }
            }
          }
        }
      }
      m_Grains[currentgrain]->voxellist->erase(std::remove(m_Grains[currentgrain]->voxellist->begin(), m_Grains[currentgrain]->voxellist->end(), -1), m_Grains[currentgrain]->voxellist->end());
      m_Grains[currentgrain]->active = 1;
      m_Grains[currentgrain]->nucleus = nucleus;
	  m_Grains[currentgrain]->phase = phases[nucleus];
      q[1] = m_Grains[currentgrain]->avg_quat[1]/m_Grains[currentgrain]->avg_quat[0];
      q[2] = m_Grains[currentgrain]->avg_quat[2]/m_Grains[currentgrain]->avg_quat[0];
      q[3] = m_Grains[currentgrain]->avg_quat[3]/m_Grains[currentgrain]->avg_quat[0];
      q[4] = m_Grains[currentgrain]->avg_quat[4]/m_Grains[currentgrain]->avg_quat[0];
	  OrientationMath::QuattoEuler(q, ea1, ea2, ea3);
	  m_Grains[currentgrain]->euler1 = ea1;
      m_Grains[currentgrain]->euler2 = ea2;
      m_Grains[currentgrain]->euler3 = ea3;
      currentgrain++;
    }
  }

  for (int i = 0; i < totalpoints; i++)
  {
	if(grain_indicies[i] >= (int)(currentgrain) )
    {
      grain_indicies[i] = -2;
    }
  }
  assign_badpoints();

  numgrains = currentgrain;
  // Resize the m_Grains vector by the appropriate amount
  size_t oldSize = m_Grains.size();
  m_Grains.resize(currentgrain);
  for(size_t g = oldSize; g < m_Grains.size(); ++g)
  {
    m_Grains[g] = Grain::New();
  }

  find_neighbors();
}

void ReconstructionFunc::fillin_sample()
{
  int col, row, plane;
  int point;
  int mincol, maxcol, minrow, maxrow, minplane, maxplane;
  mincol = xpoints;
  minrow = ypoints;
  minplane = zpoints;
  maxcol = 0;
  maxrow = 0;
  maxplane = 0;

  for (int k = 0; k < zpoints; ++k)
  {
    for (int j = 0; j < ypoints; ++j)
    {
      for (int i = 0; i < xpoints; ++i)
      {
        point = (k * xpoints * ypoints) + (j * xpoints) + i;
        if (grain_indicies[point] > 0)
        {
          col = i;
          row = j;
          plane = k;
          if (col < mincol) mincol = col;
          if (col > maxcol) maxcol = col;
          if (row < minrow) minrow = row;
          if (row > maxrow) maxrow = row;
          if (plane < minplane) minplane = plane;
          if (plane > maxplane) maxplane = plane;
        }
      }
    }
  }
  int newvoxelcount = 0;
  int newxpoints = (maxcol - mincol) + 1;
  int newypoints = (maxrow - minrow) + 1;
  int newzpoints = (maxplane - minplane) + 1;
  sizex = (maxcol - mincol) * resx;
  sizey = (maxrow - minrow) * resy;
  sizez = (maxplane - minplane) * resz;
  if (newxpoints != xpoints || newypoints != ypoints || newzpoints != zpoints)
  {
    for (int k = minplane; k < maxplane + 1; ++k)
    {
      for (int j = minrow; j < maxrow + 1; ++j)
      {
        for (int i = mincol; i < maxcol + 1; ++i)
        {
          point = (k * xpoints * ypoints) + (j * xpoints) + i;
          if (grain_indicies[point] == 0)
          {
            grain_indicies[point] = -1;
            neighbors[point] = -1;
          }
          grain_indicies[newvoxelcount] = grain_indicies[point];
          neighbors[newvoxelcount] = neighbors[point];
          phases[newvoxelcount] = phases[point];
          euler1s[newvoxelcount] = euler1s[point];
          euler2s[newvoxelcount] = euler2s[point];
          euler3s[newvoxelcount] = euler3s[point];
          surfacevoxels[newvoxelcount] = surfacevoxels[point];
          goodVoxels[newvoxelcount] = goodVoxels[point];
          alreadychecked[newvoxelcount] = alreadychecked[point];
          newvoxelcount++;
        }
      }
    }
    xpoints = newxpoints;
    ypoints = newypoints;
    zpoints = newzpoints;
    totalpoints = (xpoints * ypoints * zpoints);
  }
  assign_badpoints();
}
int ReconstructionFunc::remove_smallgrains(size_t numgrains)
{
  size_t initialVoxelsListSize = 1000;
  std::vector<int > voxelslist;
  voxelslist.resize(initialVoxelsListSize,-1);
  size_t size = 0;
  int neighpoints[26];
  int good = 0;
  int neighbor = 0;
  int col, row, plane;
  int gnum;
  int currentgrain = 1;
  neighpoints[0] = -(xpoints * ypoints) - xpoints - 1;
  neighpoints[1] = -(xpoints * ypoints) - xpoints;
  neighpoints[2] = -(xpoints * ypoints) - xpoints + 1;
  neighpoints[3] = -(xpoints * ypoints) - 1;
  neighpoints[4] = -(xpoints * ypoints);
  neighpoints[5] = -(xpoints * ypoints) + 1;
  neighpoints[6] = -(xpoints * ypoints) + xpoints - 1;
  neighpoints[7] = -(xpoints * ypoints) + xpoints;
  neighpoints[8] = -(xpoints * ypoints) + xpoints + 1;
  neighpoints[9] = -xpoints - 1;
  neighpoints[10] = -xpoints;
  neighpoints[11] = -xpoints + 1;
  neighpoints[12] = -1;
  neighpoints[13] = 1;
  neighpoints[14] = xpoints - 1;
  neighpoints[15] = xpoints;
  neighpoints[16] = xpoints + 1;
  neighpoints[17] = (xpoints * ypoints) - xpoints - 1;
  neighpoints[18] = (xpoints * ypoints) - xpoints;
  neighpoints[19] = (xpoints * ypoints) - xpoints + 1;
  neighpoints[20] = (xpoints * ypoints) - 1;
  neighpoints[21] = (xpoints * ypoints);
  neighpoints[22] = (xpoints * ypoints) + 1;
  neighpoints[23] = (xpoints * ypoints) + xpoints - 1;
  neighpoints[24] = (xpoints * ypoints) + xpoints;
  neighpoints[25] = (xpoints * ypoints) + xpoints + 1;
  for (int i = 0; i < (xpoints * ypoints * zpoints); i++)
  {
    alreadychecked[i] = false;
	gnum = grain_indicies[i];
	if(gnum >= 0) m_Grains[gnum]->nucleus = i;
  }
  for (size_t i = 1; i < numgrains; i++)
  {
      size = 0;
      int nucleus = m_Grains[i]->nucleus;
      voxelslist[size] = nucleus;
      alreadychecked[nucleus] = true;
      grain_indicies[nucleus] = currentgrain;
      size++;
      for (size_t j = 0; j < size; j++)
      {
        int currentpoint = voxelslist[j];
        col = currentpoint % xpoints;
        row = (currentpoint / xpoints) % ypoints;
        plane = currentpoint / (xpoints * ypoints);
        for (size_t k = 0; k < 26; k++)
        {
          good = 1;
          neighbor = currentpoint + neighpoints[k];
          if (k < 9 && plane == 0) good = 0;
          if (k > 16 && plane == (zpoints - 1)) good = 0;
          if ((k == 0 || k == 1 || k == 2 || k == 9 || k == 10 || k == 11 || k == 17 || k == 18 || k == 19) && row == 0) good = 0;
          if ((k == 6 || k == 7 || k == 8 || k == 14 || k == 15 || k == 16 || k == 23 || k == 24 || k == 25) && row == (ypoints - 1)) good = 0;
          if ((k == 0 || k == 3 || k == 6 || k == 9 || k == 12 || k == 14 || k == 17 || k == 20 || k == 23) && col == 0) good = 0;
          if ((k == 2 || k == 5 || k == 8 || k == 11 || k == 13 || k == 16 || k == 19 || k == 22 || k == 25) && col == (xpoints - 1)) good = 0;
          if (good == 1 && alreadychecked[neighbor] == false)
          {
            size_t grainname = static_cast<size_t>(grain_indicies[neighbor]);
            if (grainname == i)
            {
              voxelslist[size] = neighbor;
              alreadychecked[neighbor] = true;
              grain_indicies[neighbor] = currentgrain;
              size++;
              if (size >= voxelslist.size()) voxelslist.resize(size + initialVoxelsListSize, -1);
            }
          }
        }
      }
      voxelslist.erase(std::remove(voxelslist.begin(), voxelslist.end(), -1), voxelslist.end());
    if(voxelslist.size() >= static_cast<size_t>(minallowedgrainsize) )
    {
      m_Grains[currentgrain]->active = true;
      m_Grains[currentgrain]->numvoxels = voxelslist.size();
      currentgrain++;
      voxelslist.clear();
      voxelslist.resize(initialVoxelsListSize,-1);
    }
    if(voxelslist.size() < static_cast<size_t>(minallowedgrainsize) )
    {
        for (size_t b = 0; b < voxelslist.size(); b++)
        {
          int index = voxelslist[b];
          grain_indicies[index] = -2;
        }
        voxelslist.resize(initialVoxelsListSize, -1);
    }
  }
  return currentgrain;
}

void ReconstructionFunc::merge_twins()
{
  float angcur = 180;
  vector<int > twinlist;
  float w;
  float n1, n2, n3;
  float angtol = 2.0;
  float axistol = 2.0*M_PI/180.0;
  float q1[5];
  float q2[5];
  size_t numgrains = m_Grains.size();
  Ebsd::CrystalStructure phase1, phase2;

  for (size_t i = 1; i < numgrains; i++)
  {
    if (m_Grains[i]->twinnewnumber == -1)
    {
      twinlist.push_back(i);
      for (size_t m = 0; m < twinlist.size(); m++)
      {
        int firstgrain = twinlist[m];
        std::vector<int>* nlist = m_Grains[firstgrain]->neighborlist;
        int size = int(nlist->size());
        for (int l = 0; l < size; l++)
        {
          angcur = 180;
          int twin = 0;
          size_t neigh = nlist->at(l);
          if (neigh != i && m_Grains[neigh]->twinnewnumber == -1)
          {
            w = 10000.0;
            q1[1] = m_Grains[firstgrain]->avg_quat[1]/m_Grains[firstgrain]->avg_quat[0];
            q1[2] = m_Grains[firstgrain]->avg_quat[2]/m_Grains[firstgrain]->avg_quat[0];
            q1[3] = m_Grains[firstgrain]->avg_quat[3]/m_Grains[firstgrain]->avg_quat[0];
            q1[4] = m_Grains[firstgrain]->avg_quat[4]/m_Grains[firstgrain]->avg_quat[0];
            phase1 = crystruct[m_Grains[firstgrain]->phase];
            q2[1] = m_Grains[neigh]->avg_quat[1]/m_Grains[neigh]->avg_quat[0];
            q2[2] = m_Grains[neigh]->avg_quat[2]/m_Grains[neigh]->avg_quat[0];
            q2[3] = m_Grains[neigh]->avg_quat[3]/m_Grains[neigh]->avg_quat[0];
            q2[4] = m_Grains[neigh]->avg_quat[4]/m_Grains[neigh]->avg_quat[0];
            phase2 = crystruct[m_Grains[neigh]->phase];
            if (phase1 == phase2) w = m_OrientationOps[phase1]->getMisoQuat( q1, q2, n1, n2, n3);
//			OrientationMath::axisAngletoRod(w, n1, n2, n3, r1, r2, r3);
			float axisdiff111 = acosf(fabs(n1)*0.57735+fabs(n2)*0.57735+fabs(n3)*0.57735);
			float angdiff60 = fabs(w-60);
            if (axisdiff111 < axistol && angdiff60 < angtol) twin = 1;
            if (twin == 1)
            {
              m_Grains[neigh]->gottwinmerged = true;
              m_Grains[neigh]->twinnewnumber = i;
              twinlist.push_back(neigh);
            }
          }
        }
      }
    }
    twinlist.clear();
  }
  for (int k = 0; k < (xpoints * ypoints * zpoints); k++)
  {
    int grainname = grain_indicies[k];
    if (m_Grains[grainname]->gottwinmerged == true)
    {
      int twinnewnumber = m_Grains[grainname]->twinnewnumber;
      grain_indicies[k] = twinnewnumber;
    }
  }
}

void ReconstructionFunc::merge_colonies()
{
  float angcur = 180;
  vector<int > colonylist;
  float w;
  float n1, n2, n3;
  float r1, r2, r3;
  float q1[5];
  float q2[5];
  size_t numgrains = m_Grains.size();
  Ebsd::CrystalStructure phase1, phase2;

  for (size_t i = 1; i < numgrains; i++)
  {
    if (m_Grains[i]->colonynewnumber != -1)
    {
      colonylist.push_back(i);
      int csize = int(colonylist.size());
      for (int m = 0; m < csize; m++)
      {
        csize = int(colonylist.size());
        int firstgrain = colonylist[m];
        std::vector<int>* nlist = m_Grains[firstgrain]->neighborlist;
        int size = int(nlist->size());
        for (int l = 0; l < size; l++)
        {
          angcur = 180;
          int colony = 0;
          size_t neigh = nlist->at(l);
          if (neigh != i && m_Grains[neigh]->colonynewnumber != -1)
          {
		    w = 10000.0;
            q1[1] = m_Grains[firstgrain]->avg_quat[1]/m_Grains[firstgrain]->avg_quat[0];
            q1[2] = m_Grains[firstgrain]->avg_quat[2]/m_Grains[firstgrain]->avg_quat[0];
            q1[3] = m_Grains[firstgrain]->avg_quat[3]/m_Grains[firstgrain]->avg_quat[0];
            q1[4] = m_Grains[firstgrain]->avg_quat[4]/m_Grains[firstgrain]->avg_quat[0];
            phase1 = crystruct[m_Grains[firstgrain]->phase];
            q2[1] = m_Grains[neigh]->avg_quat[1]/m_Grains[neigh]->avg_quat[0];
            q2[2] = m_Grains[neigh]->avg_quat[2]/m_Grains[neigh]->avg_quat[0];
            q2[3] = m_Grains[neigh]->avg_quat[3]/m_Grains[neigh]->avg_quat[0];
            q2[4] = m_Grains[neigh]->avg_quat[4]/m_Grains[neigh]->avg_quat[0];
            phase2 = crystruct[m_Grains[neigh]->phase];

			if (phase1 == phase2) w = m_OrientationOps[phase1]->getMisoQuat( q1, q2, n1, n2, n3);
			OrientationMath::axisAngletoRod(w, n1, n2, n3, r1, r2, r3);
			float vecttol = 0.03;
            if (fabs(r1) < vecttol && fabs(r2) < vecttol && fabs(fabs(r3) - 0.0919) < vecttol) colony = 1;
            if (fabs(fabs(r1) - 0.289) < vecttol && fabs(fabs(r2) - 0.5) < vecttol && fabs(r3) < vecttol) colony = 1;
            if (fabs(fabs(r1) - 0.57735) < vecttol && fabs(r2) < vecttol && fabs(r3) < vecttol) colony = 1;
            if (fabs(fabs(r1) - 0.33) < vecttol && fabs(fabs(r2) - 0.473) < vecttol && fabs(fabs(r3) - 0.093) < vecttol) colony = 1;
            if (fabs(fabs(r1) - 0.577) < vecttol && fabs(fabs(r2) - 0.053) < vecttol && fabs(fabs(r3) - 0.093) < vecttol) colony = 1;
            if (fabs(fabs(r1) - 0.293) < vecttol && fabs(fabs(r2) - 0.508) < vecttol && fabs(fabs(r3) - 0.188) < vecttol) colony = 1;
            if (fabs(fabs(r1) - 0.5866) < vecttol && fabs(r2) < vecttol && fabs(fabs(r3) - 0.188) < vecttol) colony = 1;
            if (fabs(fabs(r1) - 0.5769) < vecttol && fabs(fabs(r2) - 0.8168) < vecttol && fabs(r3) < vecttol) colony = 1;
            if (fabs(fabs(r1) - 0.9958) < vecttol && fabs(fabs(r2) - 0.0912) < vecttol && fabs(r3) < vecttol) colony = 1;
            if (w < angcur)
            {
              angcur = w;
            }
            if (colony == 1)
            {
              m_Grains[neigh]->gotcolonymerged = true;
              m_Grains[neigh]->colonynewnumber = i;
              colonylist.push_back(neigh);
            }
          }
        }
      }
    }
    colonylist.clear();
  }
  for (int k = 0; k < (xpoints * ypoints * zpoints); k++)
  {
    int grainname = grain_indicies[k];
    if (m_Grains[grainname]->gotcolonymerged == true)
    {
      int colonynewnumber = m_Grains[grainname]->colonynewnumber;
      grain_indicies[k] = colonynewnumber;
    }
  }
}

void ReconstructionFunc::characterize_twins()
{
  size_t numgrains = m_Grains.size();
  for (size_t i = 0; i < numgrains; i++)
  {

  }
}

void ReconstructionFunc::characterize_colonies()
{
  size_t numgrains = m_Grains.size();
  for (size_t i = 0; i < numgrains; i++)
  {

  }
}

void ReconstructionFunc::renumber_grains3()
{
  size_t numgrains = m_Grains.size();
  int graincount = 1;
  std::vector<int > newnames(numgrains);
  for (size_t i = 1; i < numgrains; i++)
  {
    if (m_Grains[i]->gottwinmerged != true)
    {
      newnames[i] = graincount;
      float ea1good = m_Grains[i]->euler1;
      float ea2good = m_Grains[i]->euler2;
      float ea3good = m_Grains[i]->euler3;
      int size = m_Grains[i]->numvoxels;
      int numneighbors = m_Grains[i]->numneighbors;
      std::vector<int>* nlist = m_Grains[i]->neighborlist;
      m_Grains[graincount]->numvoxels = size;
      m_Grains[graincount]->numneighbors = numneighbors;
      if (m_Grains[graincount]->neighborlist == NULL)
      {
        m_Grains[graincount]->neighborlist = new std::vector<int>(numneighbors);
      }
      if (NULL != nlist)
      {
        m_Grains[graincount]->neighborlist->swap(*nlist);
      }
      m_Grains[graincount]->euler1 = ea1good;
      m_Grains[graincount]->euler2 = ea2good;
      m_Grains[graincount]->euler3 = ea3good;
      graincount++;
    }
  }
#if 0
  tbb::parallel_for(tbb::blocked_range<size_t>(0, totalpoints ), ParallelRenumberGrains( this) );
#else
 for (int j = 0; j < totalpoints; j++)
  {
    int grainname = grain_indicies[j];
    if (grainname >= 1)
    {
      int newgrainname = newnames[grainname];
      grain_indicies[j] = newgrainname;
    }
  }
#endif
}

void ReconstructionFunc::find_neighbors()
{
  int neighpoints[6];
  neighpoints[0] = -(xpoints * ypoints);
  neighpoints[1] = -xpoints;
  neighpoints[2] = -1;
  neighpoints[3] = 1;
  neighpoints[4] = xpoints;
  neighpoints[5] = (xpoints * ypoints);
  float column, row, plane;
  int grain;
  size_t nnum;
  int onsurf = 0;
  int good = 0;
  int neighbor = 0;
  int nListSize = 100;

  // Copy all the grain names into a densely packed array
  size_t numgrains = m_Grains.size();
  std::set<int> leftOutGrains;

  for (size_t i = 0; i < numgrains; i++)
  {
    m_Grains[i]->numneighbors = 0;
    if (m_Grains[i]->neighborlist == NULL)
    {
      m_Grains[i]->neighborlist = new std::vector<int>(0);
    }
	m_Grains[i]->neighborlist->clear();
    m_Grains[i]->neighborlist->resize(nListSize, -1);
  }

  for(int j = 0; j < (xpoints*ypoints*zpoints); j++)
  {
    onsurf = 0;

    grain = grain_indicies[j];
    if (grain > 0)
    {
      column = j % xpoints;
      row = (j / xpoints) % ypoints;
      plane = j / (xpoints * ypoints);
      for (int k = 0; k < 6; k++)
      {
        good = 1;
        neighbor = j + neighpoints[k];
        if (k == 0 && plane == 0) good = 0;
        if (k == 5 && plane == (zpoints - 1)) good = 0;
        if (k == 1 && row == 0) good = 0;
        if (k == 4 && row == (ypoints - 1)) good = 0;
        if (k == 2 && column == 0) good = 0;
        if (k == 3 && column == (xpoints - 1)) good = 0;
        if (good == 1 && grain_indicies[neighbor] != grain && grain_indicies[neighbor] > 0)
        {
          onsurf++;
          nnum = m_Grains[grain]->numneighbors;
          std::vector<int>* nlist = m_Grains[grain]->neighborlist;
          if (nnum >= (nlist->size()))
          {
            nlist->resize(nnum + nListSize);
          }
          nlist->at(nnum) = grain_indicies[neighbor];
          nnum++;
          m_Grains[grain]->numneighbors = nnum;
        }
      }
    }
    surfacevoxels[j] = onsurf;
  }

  vector<int> nlistcopy;
  for (size_t i = 1; i < numgrains; i++)
  {
   // int phase = m_Grains[i]->phase;
    std::vector<int>* nlist = m_Grains[i]->neighborlist;

    std::map<int, int> neighToCount;
    int numneighs = int(nlist->size());

    // this increments the voxel counts for each grain
    for (int j = 0; j < numneighs; j++)
    {
      neighToCount[nlist->at(j)]++;
    }

    neighToCount.erase(0);
    neighToCount.erase(-1);
    //Resize the grains neighbor list to zero
    m_Grains[i]->neighborlist->resize(0);

    for (std::map<int, int>::iterator iter = neighToCount.begin(); iter != neighToCount.end(); ++iter )
    {
      int neigh = iter->first; // get the neighbor grain
      // Push the neighbor grain id back onto the list so we stay synced up
      m_Grains[i]->neighborlist->push_back(neigh);
    }
	m_Grains[i]->numneighbors = m_Grains[i]->neighborlist->size();
  }
  merge_containedgrains();

}


