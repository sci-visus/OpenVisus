/*-----------------------------------------------------------------------------
Copyright(c) 2010 - 2018 ViSUS L.L.C.,
Scientific Computing and Imaging Institute of the University of Utah

ViSUS L.L.C., 50 W.Broadway, Ste. 300, 84101 - 2044 Salt Lake City, UT
University of Utah, 72 S Central Campus Dr, Room 3750, 84112 Salt Lake City, UT

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met :

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

For additional information about this project contact : pascucci@acm.org
For support : support@visus.net
-----------------------------------------------------------------------------*/

#include <Visus/IdxDataset.h>

using namespace Visus;

/////////////////////////////////////////////////////////////////////////////////////
void TilingExample(IdxDataset* vf)
{
  int maxh=vf->getMaxResolution();

  int            bitsperblock    =  vf->getDefaultBitsPerBlock();
  DatasetBitmask bitmask         =  vf->getBitmask();
  int            samplesperblock = 1<<bitsperblock;

  int pdim = bitmask.getPointDim();

  //these are your levels, a level is a set of tiles 
  //the 1st level contains 1 tile  (block=0)
  //the 2nd level contains 1 tile  (block=1)
  //the 3rd level contains 2 tiles (hzblocks=2,3)
  //the 4th level contains 4 tiles (hzblocks=4,5,6,7)
  //... and so on...
  for (int Level=0,H=bitsperblock;H<=maxh;Level++,H++)
  {
    //calculate number of tiles in this level
    //NOTE: total_number_of_tiles_inside_this_level=VisusInnerProduct(ntiles)
    //for example:
    //   the 1st level -> ntiles (1,1,1)
    //   the 2nd level -> ntiles (1,1,1)
    //   the 3rd level -> ntiles ...depends on the bitmask, could be (2,1,1) or (1,2,1) or whatever....
    NdPoint ntiles=NdPoint::one(pdim);
    int K=std::max(0,H-bitsperblock-1);
    for (int N=K;N>0;N--) ntiles[bitmask[N]]<<=1;

    //calculate number of samples inside each tile 
    NdPoint dims=NdPoint::one(pdim);
    for (int N=H-((H==bitsperblock)?0:1),M=0;M<bitsperblock;N--,M++) dims[bitmask[N]]<<=1;

    //calculate dimension of each tile
    NdPoint dim=NdPoint::one(pdim);
    for (int D=0;D<pdim;D++)
      dim[D]=bitmask.getPow2Dims()[D]/ntiles[D];
  
    //this FOR LOOP iterate in all tiles inside the current level
    //the tile (         0,         0,         0) is the tile in the left-down corner
    //the tile (ntiles.x-1,ntiles.y-1,ntiles.z-1) is the tile in the right-up corner
    for (auto tile = ForEachPoint(ntiles); !tile.end(); tile.next())
    {
      //this is the tile bounding box
      NdPoint p1(pdim),p2=NdPoint::one(pdim);
      for (int D=0;D<pdim;D++) 
      {
        p1[D]=(tile.pos[D]  )*dim[D];
        p2[D]=(tile.pos[D]+1)*dim[D];
      }
        
      NdBox box(p1,p2);

      //what is the corresponding Visus block number 
      BigInt block = (H==bitsperblock)? (0) : ((((BigInt)1)<<K) + HzOrder(bitmask,K).interleave(tile.pos));

      //my check code (verify that the tile is really a query of a single block)
      #ifdef VISUS_DEBUG
      {
        auto query=std::make_shared<Query>(vf,'r');
        query->position=box;
        query->start_resolution=(H==bitsperblock?0:H);
        query->end_resolutions={H};
        VisusReleaseAssert(vf->beginQuery(query));
        VisusReleaseAssert(query->nsamples==dims);
      }
      #endif
    }
  }
}


///////////////////////////////////////////////////////////////////////////////////////
void Tutorial_Tiling(String default_layout)
{
  String filename="temp/tiling.idx";

  //create a sample IdxDataset
  IdxFile idxfile;
  idxfile.box = NdBox(NdPoint(0,0), NdPoint::one(32,8));
  {
    Field field("DATA",DTypes::UINT8);
    field.default_layout=default_layout;
    idxfile.fields.push_back(field);
  }
  idxfile.bitmask=DatasetBitmask("V00101010");
  idxfile.bitsperblock=2;
  idxfile.blocksperfile=1;
  VisusReleaseAssert(idxfile.save(filename));

  auto vf= LoadDataset<IdxDataset>(filename);
  VisusReleaseAssert(vf && vf->valid());

  TilingExample(vf.get());
}

