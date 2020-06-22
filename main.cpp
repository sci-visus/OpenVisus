#if WIN32
#pragma warning (disable:4244)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>



class volume
{
public:
    volume();
    ~volume();
    
    bool init(int, int, int, int);
    void clearAll();
    
    bool reconstruct();
    bool rawwrite(const char*);
    
    float** data;
    int N_x, N_y, N_z;
    int z_offset;
};

volume::volume()
{
    data = NULL;
    N_x = 0;
    N_y = 0;
    N_z = 0;
    z_offset = 0;
}

volume::~volume()
{
    clearAll();
}

bool volume::init(int numX, int numY, int numZ, int z_min)
{
    if (numX <= 0 || numY <= 0 || numZ <= 0)
        return false;
    else if (data != NULL)
        return true;
    else
    {
        N_x = numX;
        N_y = numY;
        N_z = numZ;
        z_offset = z_min;
        data = (float**) malloc(N_x*sizeof(float)); //scrgiorgio, should it be sizeof(float*)?
        for (int i = 0; i < N_x; i++)
            data[i] = (float*) malloc(N_y*N_z*sizeof(float));
        return true;
    }
}

void volume::clearAll()
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

bool volume::reconstruct()
{
    if (data == NULL)
        return false;
    else
    {
        float x_max = float(N_x-1)/2.0;
        float y_max = float(N_y-1)/2.0;
        float r = std::min(x_max, y_max);
        for (int i = 0; i < N_x; i++)
        {
            float x = (float(i) - float(N_x-1)/2.0);
            for (int j = 0; j < N_y; j++)
            {
                float y = (float(j) - float(N_y-1)/2.0);
                for (int k = 0; k < N_z; k++)
                {
                    float z = float(k+z_offset);
                    if (x*x + y*y < r*r)
                        data[i][j*N_z+k] = 1.0 + z;
                    else
                        data[i][j*N_z+k] = 0.0;
                }
            }
        }
        return true;
    }
}

bool volume::rawwrite(const char* fileName)
{
    if (data == NULL || fileName == NULL || fileName[0] == 0)
        return false;
    else
    {
        float* zSlice = (float*) malloc(N_x*N_y*sizeof(float));
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
            for (int j = 0; j < N_y; j++)
            {
                for (int i = 0; i < N_x; i++)
                    zSlice[j*N_x+i] = data[i][j*N_z+k];
            }
            fwrite(zSlice, sizeof(float), N_x*N_y, fdes);
            fclose(fdes);
        }
        free(zSlice);
        return true;
    }
}

bool writeIDX(volume*, char*);

int main()
{
    int N_x = 256;
    int N_y = 128;
    int N_z = 299;
    int numChunks = 2;
    
    int maxSlicesPerChunk = int(ceil(float(N_z)/float(numChunks)));
    for (int ichunk = 0; ichunk < numChunks; ichunk++)
    {
        int z_offset = ichunk*maxSlicesPerChunk;
        int z_max = std::min(z_offset + maxSlicesPerChunk - 1, N_z-1);
        int N_z_chunk = z_max - z_offset + 1;
        
        volume* f = new volume;
        f->init(N_x, N_y, N_z_chunk, z_offset);
        f->reconstruct();
        f->rawwrite("zSlice");
        //writeIDX(f,"volume.idx");
        delete f;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////
//OPEN VISUS EXAMPLE:
#include <Visus/IdxFile.h>
#include <Visus/Dataset.h>
using namespace Visus;

bool writeIDX(volume* f, const char* filename)
{
  //create the *.idx file 
  IdxFile idxfile;
  idxfile.logic_box = BoxNi(PointNi(0, 0, 0), PointNi(f->N_x, f->N_y, f->N_z));
  Field field("myfield", DTypes::FLOAT32);
  field.default_compression = "zip";
  field.default_layout = "row_major";
  idxfile.fields.push_back(field);
  idxfile.save(filename);

  //write data slice by slice 
  //NOTE: in order to speed up things, it's possible to write 3d data, but your memory buffers must be contigous
  std::shared_ptr<Dataset> dataset = LoadDataset(filename);

  std::shared_ptr<Access> access = dataset->createAccess();

  for (Int64 Z = 0; Z < f->N_z; Z++)
  {
    PointNi p1(     0,      0, Z + 0);
    PointNi p2(f->N_x, f->N_y, Z + 1);

    std::shared_ptr<BoxQuery> query = dataset->createBoxQuery(BoxNi(p1,p2), 'w');
    dataset->beginBoxQuery(query);

    if (!query->isRunning())
      throw std::exception("query failed");

    //scrgiorgio: Here I'm assuming f->data[Z] will point to a chunk of CONTIGUOUS memory of size== sizeof(Float32) * f->N_x * f->N_y (i.e. a slice of your data)
    query->buffer = Array(query->getNumberOfSamples(), query->field.dtype, HeapMemory::createUnmanaged(f->data[Z], sizeof(Float32) * f->N_x * f->N_y));

    if (!dataset->executeBoxQuery(access, query))
      throw "error here";
  }

  return true;
}
