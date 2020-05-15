import sys
import argparse
from OpenVisus import *
import numpy as np
import scipy.misc
from os.path import abspath,splitext,split

def load_image(img_file):
        """
        Load the image given by filename and return the approriate 
        numpy array 
        """
        
        print("Opening file {}".format(img_file))
        if splitext(img_file)[1] == '.tif' or splitext(img_file)[1] == '.tiff':
                from PIL import Image

                data = np.array(Image.open(img_file))
                
                #data = data / data.max() * 255
                #data = data.astype(np.uint8)
                #Image.fromarray(data).save('test.jpg')
        else:
                raise TypeError('Image format not found')

        return data

def guess_index(name):
        """ 
        Given a file name which contains a single index number try to 
        guess that number for sorting oddly name files
        """
        

if __name__ == '__main__':
        
        """ 
        Somewhat general data conversion tool from slices to 3D stacks
        
        Arguments:
        
                --i <image0> ... <imageN>: A sorted list of input slices
                --o <output.idx>: The output idx file
                --sort <template> : The filename template used to sort 
        
        """

        parser = argparse.ArgumentParser()
        parser.add_argument('--i', nargs='*', dest='input_files', type=str)
        parser.add_argument('--o', nargs='?', dest='idx_name', default='output.idx', type=str)
        parser.add_argument('--sort', dest='template', type=str)
        parser.add_argument('--fieldnames', nargs='+', dest='field_names', type=str)
        
        
        args = parser.parse_args()

        if args.template:
                print(args.template)
                prefix = len(args.template.split('{')[0])
                postfix = len(args.template.split('}')[1])
                
                args.input_files.sort(key = lambda name: int(split(name)[1][prefix:-postfix]))
                
        
        SetCommandLine("__main__")
        DbModule.attach()

        # trick to speed up the conversion
        os.environ["VISUS_DISABLE_WRITE_LOCK"]="1"

        data = load_image(args.input_files[0])
        
        width = data.shape[0]
        height = data.shape[1]
        depth = len(args.input_files)


        print("Final image will have width: {}  height: {} depth: {}".format(width,height,depth))
        
        # numpy dtype -> OpenVisus dtype
        typestr=data.__array_interface__["typestr"]
        dtype=DType(typestr[1]=="u", typestr[1]=="f", int(typestr[2])*8)        
        
        dims = PointNi(int(width),int(height),int(depth))
        idx_file = IdxFile()
        idx_file.logic_box = BoxNi(PointNi(0,0,0),dims)
        
        if args.field_names:
                for fields in args.field_name:
                        idx_file.fields.push_back(Field(fields,dtype))
        else:
                idx_file.fields.push_back(Field('value',dtype))

        idx_file.save(args.idx_name)
                        
        print("Created IDX file")

        # Load the dataset back into memory     
        dataset = LoadDataset(args.idx_name)
        
        # And create a handle to manipulate the data
        access = dataset.createAccess()
        if not dataset:
                raise Exception("Assert failed")        
  
        for z,img in enumerate(args.input_files):                       
                print("Processing slice %d" % z)
                data = load_image(img)
                
                # Create the correct logical box where to put this data
                slice_box = dataset.getLogicBox().getZSlab(z,z+1)
                if not (slice_box.size()[0]==dims[0] and slice_box.size()[1]==dims[1]):
                        raise Exception("Image dimensions do not match")        
                        
                # Create a query into the dataset. Note the 'w' to indicate a write query
                # and the fact that for now we assume a single time step
                query = BoxQuery(dataset,dataset.getDefaultField(),dataset.getDefaultTime(),ord('w'))
                query.logic_box = slice_box
                
                # Start the query and make sure that it works
                dataset.beginQuery(query)
                if not query.isRunning():
                        raise Exception("Could not start write query")  
                
                # Assign the data
                query.buffer = Array.fromNumPy(data)
                
                
                # Actually commit the data 
                if not (dataset.executeQuery(access,query))     :
                        raise Exception("Could not execute write query")
                
                # enable/disable these two lines after debugging
                # ArrayUtils.saveImageUINT8("tmp/slice%d.orig.png" % (Z,),Array.fromNumPy(data))
                # ArrayUtils.saveImageUINT8("tmp/slice%d.offset.png" % (Z,),Array.fromNumPy(fill))
        
        DbModule.detach()
        print("Done with conversion")
        sys.exit(0)
