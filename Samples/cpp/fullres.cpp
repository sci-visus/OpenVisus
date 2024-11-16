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
#include <Visus/File.h>

namespace Visus {

////////////////////////////////////////////////////////////////////
void CppSamples_FullRes()
{
  std::vector<Array> images;
  for (auto filename : { "datasets/cat/rgb.png", "datasets/lion.jpg", "datasets/tiger.jpg" })
    images.push_back(ArrayUtils::loadImage(filename));

  IdxFile file;
  file.bitmask = DatasetBitmask::fromString("F"); 
  file.logic_box = BoxNi(PointNi(2), PointNi(2048, 1600));
  file.fields.push_back(Field::fromString("data uint8[3]"));
  file.save("tmp/fullres/visus.idx");

  auto dataset = LoadDataset("tmp/fullres/visus.idx");
  auto MaxH = dataset->getMaxResolution();
  auto bitmask = dataset->getBitmask();
  auto bitsperblock = dataset->getDefaultBitsPerBlock();
  auto LOGIC_BOX = dataset->getLogicBox();

  for (auto mode : { 'w','r' })
  {
    auto access = dataset->createAccessForBlockQuery(); 
    if (mode == 'w')
    {
      access->disableWriteLocks();
      access->disableCompression();
    }

    for (int H = MaxH; H >= bitsperblock; H--)
    {
      auto img = images[(MaxH - H) % (int)images.size()];

      //logic distance (note that for each bit I double the distance)
			PointNi LOGIC_DELTA = img.dims;
      for (int K = MaxH; K > H; K--)
        LOGIC_DELTA[bitmask[K]] <<= 1;

      for (auto it = ForEachPoint(LOGIC_BOX.p1, LOGIC_BOX.p2, LOGIC_DELTA); !it.end(); it.next())
      {
				auto logic_box = BoxNi(it.pos, it.pos + LOGIC_DELTA).getIntersection(LOGIC_BOX);

        auto query = dataset->createBoxQuery(logic_box, mode);
        query->setResolutionRange(0, H);
        dataset->beginBoxQuery(query);
        auto cropped = ArrayUtils::crop(img, BoxNi(PointNi(0, 0), query->getNumberOfSamples()));

				if (mode == 'w')
        {
          query->buffer = cropped;
          VisusReleaseAssert(dataset->executeBoxQuery(access, query))
        }
        else
        {
          VisusReleaseAssert(dataset->executeBoxQuery(access, query));
          VisusAssert(HeapMemory::equals(query->buffer.heap, cropped.heap));
        }
      }

      //uncomment to debug
#if 0
      if (mode == 'w')
      {
        auto query = dataset->createBoxQuery(LOGIC_BOX, 'r');
        query->setResolutionRange(0, H);
        dataset->beginBoxQuery(query);
        VisusReleaseAssert(dataset->executeBoxQuery(access, query));
        ArrayUtils::saveImage(concatenate("tmp/fullres/check.", H, ".png"), query->buffer);
      }
#endif

    }
  }

  FileUtils::removeDirectory(Path("tmp/fullres"));
}

} //namespace Visus