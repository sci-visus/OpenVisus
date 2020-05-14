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
#include <Visus/DatasetFilter.h>
#include <Visus/File.h>

namespace Visus {

//////////////////////////////////////////////////////////////////////////////////////
static SharedPtr<IdxDataset> createDatasetFromImage(String filename,Array img,DType in_dtype,PointNi offset,int bitsperblock,String default_layout,String filter)
{
  int NS=img.dtype.ncomponents();DType Sdtype=img.dtype.get(0);
  int ND=in_dtype .ncomponents();DType Ddtype=in_dtype.get(0);

  VisusReleaseAssert(Sdtype==(DTypes::UINT8));

  Int64 tot=img.getTotalNumberOfSamples();
  VisusReleaseAssert(tot>=0);

  //create the IdxDataset
  BoxNi userbox(offset, offset+img.dims);

  IdxFile idxfile;
  idxfile.logic_box=userbox;
  {
    Field field("myfield",in_dtype);
    field.default_layout=default_layout;
    field.filter=filter;
    idxfile.fields.push_back(field);
  }
  idxfile.bitmask=DatasetBitmask();
  idxfile.bitsperblock=bitsperblock;
  idxfile.save(filename);

  auto dataset= LoadDataset<IdxDataset>(filename);

  auto access=dataset->createAccess();
  
  auto write=std::make_shared<BoxQuery>(dataset.get(), dataset->getDefaultField(), dataset->getDefaultTime(), 'w');
  write->logic_box=userbox;
  dataset->beginQuery(write);
  VisusReleaseAssert(write->isRunning());
  VisusReleaseAssert(write->getNumberOfSamples()==img.dims);

  int N=std::min(NS,ND);

  write->buffer=Array(img.dims,write->field.dtype);

  //uint8
  if (Ddtype==(DTypes::UINT8))
  {
    const unsigned char* Src=img.c_ptr();
    unsigned char*       Dst=(unsigned char*)write->buffer.c_ptr();
    for (int I=0; I<tot; I++,Src+=NS,Dst+=ND)
    {
      for (int J=0;J<N;J++) 
      {
        VisusReleaseAssert(&Dst[J]<(write->buffer.c_ptr()+write->buffer.c_size()) && &Src[J]<(img.c_ptr()+img.c_size()));
        Dst[J]=Src[J];
      }
    }
  }
  //float32
  else if (Ddtype==(DTypes::FLOAT32))
  {
    const unsigned char* Src=img.c_ptr();
    float*               Dst=(float*)write->buffer.c_ptr();
    for (int I=0; I<tot; I++,Src+=NS,Dst+=ND)
    {
      for (int J=0;J<N;J++) Dst[J]=Src[J]/255.0f;
    }
  }
  //float64
  else if (Ddtype==(DTypes::FLOAT64))
  {
    const unsigned char* Src=img.c_ptr();
    double* Dst=(double*)write->buffer.c_ptr();
    for (int I=0; I<tot; I++,Src+=NS,Dst+=ND)
    {
      for (int J=0;J<N;J++) Dst[J]=Src[J]/255.0;
    }
  }
  //not supported
  else
  {
    VisusReleaseAssert(false);
    return nullptr;
  }

  //write to disk
  VisusReleaseAssert(dataset->executeQuery(access,write));
  return dataset;
}


//////////////////////////////////////////////////////////////////////////////////////
Array createImageFromBuffer(Array src)
{
  PointNi dims=src.dims;
  Int64 tot=dims.innerProduct();
  int ncomponents=src.dtype.ncomponents();
  DType dtype=src.dtype;

  // only 2d images grayscale or RGB
  VisusReleaseAssert(tot>0);
  VisusReleaseAssert(tot==(Int64)dims[0]*(Int64)dims[1]);
  VisusReleaseAssert(ncomponents==1 || ncomponents==3);

  if (dtype.isVectorOf(DTypes::UINT8))
    return src;

  Array dst;
  VisusReleaseAssert(dst.resize(dims,DType(ncomponents,DTypes::UINT8),__FILE__,__LINE__));
  Uint8* DST=(Uint8*)dst.c_ptr();

  if (dtype.isVectorOf(DTypes::FLOAT32))
  {
    Float32* SRC=(Float32*)src.c_ptr();
    for (int Sample=0;Sample<tot;Sample++)
    {
      for (int N=0;N<ncomponents;N++) 
      {
        int value=(int)floor(255.0f*(*SRC++)+0.5f); //example 100.9 -> 101.0
        *DST++=(Uint8)Utils::clamp(value,0,255);
      }
    }
    return dst;
  }

  if (dtype.isVectorOf(DTypes::FLOAT64))
  {
    Float64* SRC=(Float64*)src.c_ptr();
    for (int Sample=0;Sample<tot;Sample++)
    {
      for (int N=0;N<ncomponents;N++) 
      {
        int value=(int)floor(255.0*(*SRC++)+0.5);
        *DST++=(Uint8)Utils::clamp(value,0,255);
      }
    }
    return dst;
  }

  //unsupported
  VisusReleaseAssert(false);
  return Array();
}



//////////////////////////////////////////////////////////////////////////////////////
//show how to apply the De Haar DatasetFilter on IDX dataset
//////////////////////////////////////////////////////////////////////////////////////
void Tutorial_6(String default_layout)
{
  auto sliding_window_size=PointNi(32,32);

  int bitsperblock=12;  //I want certain number of blocks

  PointNi dataset_offset(2);
  #if 1
  {
    dataset_offset[0]=12; //disalignment X
    dataset_offset[1]=11; //disalignment Y
  }
  #endif

  std::vector<String> filters;
  filters.push_back("min");
  filters.push_back("max");
  filters.push_back("identity");
  filters.push_back("dehaar");

  for (int NFilter  =0;NFilter   <(int)filters.size();NFilter++)
  for (int Discrete =1;Discrete >=0;Discrete-- )
  for (int GrayScale=1;GrayScale>=0;GrayScale--)
  for (int Overall  =1;Overall  >=0;Overall--  )
  {
    PrintInfo(
      "running filter",filters[NFilter],
      GrayScale?"GrayScale":"RGB",
      Discrete?"Discrete":"Continuous",
      Overall?"Overall":"Piece");

    int ncomponents=GrayScale?1:3;
         if (filters[NFilter]=="min"     ) ncomponents++;
    else if (filters[NFilter]=="max"     ) ncomponents++;
    else if (filters[NFilter]=="identity") ;
    else if (filters[NFilter]=="dehaar"  ) ncomponents+=Discrete?1:0;
    else VisusReleaseAssert(false);

    DType dtype(ncomponents,Discrete?DTypes::UINT8:DTypes::FLOAT64);

    //read the data
    String img_filename=GrayScale? "datasets/cat/gray.png" :"datasets/cat/rgb.png";

    Array src_image=ArrayUtils::loadImage(img_filename);
    VisusReleaseAssert(src_image);
    auto dataset=createDatasetFromImage("temp/tutorial_6.idx",src_image,dtype,dataset_offset,bitsperblock,default_layout,filters[NFilter]);
    VisusReleaseAssert(dataset);
    Field field=dataset->getDefaultField();

    //now that I have an IDX file apply the filter
    
    auto access=dataset->createAccess();

    //apply the filter on a IDX file (i.e. rewrite all samples)
    {
      auto filter=dataset->createFilter(field);
      VisusReleaseAssert(filter);
      filter->computeFilter(dataset->getDefaultTime(),field,access,sliding_window_size);
    }

    BoxNi world_box=dataset->getLogicBox();
    Int64 Width =world_box.p2[0]-world_box.p1[0];VisusReleaseAssert(Width ==src_image.dims[0]);
    Int64 Height=world_box.p2[1]-world_box.p1[1];VisusReleaseAssert(Height==src_image.dims[1]);

    //query box,example of generic query of generic bounding box (i.e. apply the inverse filter)
    BoxNi query_box;

    if (Overall)
    {
      query_box = BoxNi(
        PointNi(dataset_offset[0]        , dataset_offset[1]),
        PointNi(dataset_offset[0] + Width, dataset_offset[1] + Height));
    }
    else
    {
      // show that the query is reconstructed at all levels
      query_box = BoxNi(
        PointNi((int)(dataset_offset[0] + 3.0f*Width / 6.0f), (int)(dataset_offset[1] + 3.0f*Height / 6.0f)),
        PointNi((int)(dataset_offset[0] + 4.0f*Width / 6.0f), (int)(dataset_offset[1] + 4.0f*Height / 6.0f)));
    }
    
    auto query=std::make_shared<BoxQuery>(dataset.get(), dataset->getDefaultField(), dataset->getDefaultTime(), 'r');
    query->logic_box= query_box;
    query->filter.enabled=true;
    query->merge_mode= InsertSamples;

    //I go level by level for debugging
    for (int H=0;H<=dataset->getMaxResolution();H++)
      query->end_resolutions.push_back(H);

    dataset->beginQuery(query);
    VisusReleaseAssert(query->isRunning());

    while (query->isRunning())
    {
      VisusReleaseAssert(dataset->executeQuery(access,query));
      
      auto buffer=query->buffer;
      buffer=query->filter.dataset_filter->dropExtraComponentIfExists(buffer);
      auto reconstructed=createImageFromBuffer(buffer);
      VisusReleaseAssert(reconstructed);
      
      //save the image
      {
        int H=query->getCurrentResolution();

        Path filename=Path(String("temp/")  + filters[NFilter] + "/" + StringUtils::replaceFirst(dtype.toString(),"*","_") + String(Overall ?"_all":"_piece"))
          .getChild("snapshot" + String(H<10?"0":"") + cstring(H)  + (".png"));

        FileUtils::createDirectory(filename.getParent());
        VisusReleaseAssert(ArrayUtils::saveImage(filename.toString(),reconstructed));
      }

      //verify the data only If I'm reading the final resolution
      if (query->getCurrentResolution()==dataset->getMaxResolution())
      {
        BoxNi logic_box= query->filter.adjusted_logic_box;

        //need to shrink the query, at the final resolution the box of filtered query can be larger than what the user want
        auto original=std::make_shared<Array>();
        {
          BoxNi crop_box= logic_box.translate(-dataset_offset);
          original=std::make_shared<Array>(ArrayUtils::crop(src_image,crop_box));
        }

        Int64 c_size = original->c_size();
        VisusReleaseAssert(c_size>0);
        VisusReleaseAssert(c_size==original->dtype.getByteSize(original->dims.innerProduct()));
        VisusReleaseAssert(c_size==reconstructed.dtype.getByteSize(original->dims.innerProduct()));
        VisusReleaseAssert(c_size==reconstructed.c_size());

        //to better debug I want to know the different byte
        if (memcmp(original->c_ptr(),reconstructed.c_ptr(),(size_t)c_size)!=0)
        {
          Uint8* ORIGINAL     =original->c_ptr();
          Uint8* RECONSTRUCTED=reconstructed.c_ptr();
          for (int I=0;I<c_size;I++)
            VisusReleaseAssert(ORIGINAL[I]==RECONSTRUCTED[I]);
        }
      }

      dataset->nextQuery(query);
    }

    dataset->removeFiles();
  }
}

} //namespace Visus
