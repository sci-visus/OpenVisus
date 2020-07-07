#if WIN32
#pragma warning (disable:4244)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>

 
/////////////////////////////////////////////////////////////////
class volume
{
public:
	
	
	float** data=nullptr;
	int N_x=0, N_y=0, N_z=0;
	int z_offset=0;	
	
	//constructor
	volume(){
	}
	
	//destructor
	~volume(){
		clearAll();
	}
	
	//clearAll
	void clearAll()
	{
		if (data != NULL)
		{
			for (int i = 0; i < N_x; i++)
			{
				free(data[i]);
				data[i] = NULL;
			}
			free(data);
			data = NULL;
			
		}
		N_x = 0;
		N_y = 0;
		N_z = 0;
		z_offset = 0;
	}		
	
	//get
	const float& get(int i,int j,int k) const {
		return data[i][j*N_z+k];
	}
	
	//get
	float& get(int i,int j,int k) {
		return data[i][j*N_z+k];
	}	
	
	//extractZSlice
	float* extractZSlice(int k) const 
	{
		float* ret = new float[N_x*N_y];
		int cont=0;
		for (int j = 0; j < N_y; j++)
		{
			for (int i = 0; i < N_x; i++)
				ret[cont++] = get(i,j,k);
		}		
		return ret;
	}	
	
	//init
	bool init(int numX, int numY, int numZ, int z_min)
	{
		if (numX <= 0 || numY <= 0 || numZ <= 0)
			return false;
			
		if (data != NULL)
			return true;
		
		N_x = numX;
		N_y = numY;
		N_z = numZ;
		z_offset = z_min;
		data = (float**) malloc(N_x*sizeof(float*)); 
		for (int i = 0; i < N_x; i++)
			data[i] = (float*) malloc(N_y*N_z*sizeof(float));
		
		return true;
	}
	
	//reconstruct
	bool reconstruct()
	{
		if (data == NULL)
			return false;

		float x_max = float(N_x-1)/2.0;
		float y_max = float(N_y-1)/2.0;
		float r = std::min(x_max, y_max);
			
		for (int i = 0; i < N_x; i++)
		{
			for (int j = 0; j < N_y; j++)
			{
				for (int k = 0; k < N_z; k++)
				{
					float x = (float(i) - float(N_x-1)/2.0);
					float y = (float(j) - float(N_y-1)/2.0);
					float z = float(k+z_offset);
					get(i,j,k)=(x*x + y*y < r*r)? 1.0 + z : 0.0;
				}
			}
		}
		return true;
	}	

	
	//rawwrite
	bool rawwrite(const char* fileName)
	{
		if (data == NULL || fileName == NULL || fileName[0] == 0)
			return false;

		for (int k = 0; k < N_z; k++)
		{
			char fullName[1024];
			sprintf(fullName, "%s_%d.raw", fileName, k+z_offset);
			FILE* fdes = NULL;
			if ((fdes = fopen(fullName,"wb")) == NULL)
			{
				printf("Error writing file: %s\n", fullName);
				return false;
			}
			
			float* zSlice=extractZSlice(k);
			fwrite(zSlice, sizeof(float), N_x*N_y, fdes);
			delete [] zSlice;
			
			fclose(fdes);
		}
		return true;
	}	

};


//////////////////////////////////////////////////////////////////////
#include <Visus/IdxFile.h>
#include <Visus/IdxDataset.h>
#include <Visus/IdxDiskAccess.h>

using namespace Visus;

/////////////////////////////////////////////////////////////////
void CreateIdx(std::string idx_filename,std::string dtype, int N_x,int N_y,int N_z)
{
  IdxFile idxfile;
  idxfile.logic_box = BoxNi(PointNi(0, 0, 0), PointNi(N_x, N_y, N_z));
  Field field("myfield", DType::fromString(dtype));
  field.default_compression = ""; 
  field.default_layout = "row_major";
  idxfile.fields.push_back(field);
  idxfile.save(idx_filename);	
}


/////////////////////////////////////////////////////////////////
void WriteIDX(std::shared_ptr<IdxDataset> dataset,SharedPtr<IdxDiskAccess> access,BoxNi box, void* buffer, int buffer_size)
{
	std::shared_ptr<BoxQuery> query = dataset->createBoxQuery(box, 'w');
	dataset->beginBoxQuery(query);
	VisusReleaseAssert(query->isRunning());
	VisusReleaseAssert(buffer_size==query->field.dtype.getByteSize(box.size()));
	query->buffer = Array(query->getNumberOfSamples(), query->field.dtype, HeapMemory::createUnmanaged(buffer, buffer_size));
	VisusReleaseAssert(dataset->executeBoxQuery(access, query));
}

/////////////////////////////////////////////////////////////////
Array ReadIdx(std::shared_ptr<IdxDataset> dataset,SharedPtr<IdxDiskAccess> access,BoxNi box)
{
	std::shared_ptr<BoxQuery> query = dataset->createBoxQuery(box, 'r');
	dataset->beginBoxQuery(query);
	VisusReleaseAssert(query->isRunning());
	VisusReleaseAssert(dataset->executeBoxQuery(access, query));
	return query->buffer;
}


/////////////////////////////////////////////////////////////////
int main()
{
	const int N_x = 256;
	const int N_y = 128;
	const int N_z = 299;
	const int numChunks = 2;
	
	DbModule::attach();
		
	int maxSlicesPerChunk = int(ceil(float(N_z)/float(numChunks)));
	for (int ichunk = 0; ichunk < numChunks; ichunk++)
	{
		std::cout<<"Chunk "<<ichunk<<std::endl;
		
		int z_offset = ichunk*maxSlicesPerChunk;
		int z_max = std::min(z_offset + maxSlicesPerChunk - 1, N_z-1);
		int N_z_chunk = z_max - z_offset + 1;
		
		volume* f = new volume();
		f->init(N_x, N_y, N_z_chunk, z_offset);
		
		
		std::cout<<"   reconstruct..."<<std::endl;
		f->reconstruct();
		
		std::cout<<"   rawwrite..."<<std::endl;
		f->rawwrite("zSlice");
		
		//create OpenVius dataset
		std::string idx_filename=concatenate("volume.",ichunk,".idx");
		CreateIdx(idx_filename, "float32", N_x, N_y, N_z_chunk);
		
		//NOTE: writing/reading slice by slice is not the best for OpenVisus
		//better is to write all the memory in one-shot but we need a different memory layout
		std::cout<<"   Idx read/write..."<<std::endl;
	  std::shared_ptr<IdxDataset> dataset = LoadIdxDataset(idx_filename);
		SharedPtr<IdxDiskAccess> access=IdxDiskAccess::create(dataset.get());
		access->disableAsync(); 
		access->disableWriteLock();
		 			
		for (Int64 Z = 0; Z < f->N_z; Z++)
  	{
  		float* zSlice=f->extractZSlice(Z);
  		int  zSlice_csize=sizeof(float)*f->N_x*f->N_y;
  		
  		//this is the bounding box of a single slice 
  		BoxNi slice_box(PointNi(0,0,Z),PointNi(f->N_x, f->N_y, Z + 1));
  		
			WriteIDX(dataset, access, slice_box, zSlice, zSlice_csize);
			
			auto read_back=ReadIdx(dataset, access, slice_box);
			
			//just check you get back the same data
			VisusReleaseAssert(memcmp(read_back.c_ptr(),zSlice, zSlice_csize)==0);
			
			delete [] zSlice;
		}
		
		delete f;
	}
	
	DbModule::detach();
	
	return 0;
}

