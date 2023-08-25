import xarray as xr
import numpy  as np
import pandas as pd


# !pip install OpenVisusNoGui
import OpenVisus as ov

# see https://xarray.pydata.org/en/stable/internals/how-to-add-new-backend.html


# ////////////////////////////////////////////////////////////
class OpenVisusBackendArray(xr.backends.common.BackendArray):
#     TODO: add num_refinements,quality
#     TODO: adding it for normalized coordinates

    # constructor
    def __init__(self,db, shape, dtype, timesteps,resolution,fieldname):
        self.db    = db
        self.shape = shape
        self.fieldname=fieldname
        self.dtype = dtype
        self.pdim=db.getPointDim()
        self.timesteps=timesteps
        self.resolution=resolution

    # _getKeyRange
    def _getXRange(self, value):
        if self.pdim==2:
            A = value.start if isinstance(value, slice) else value    ; A = int(0)             if A is None else A
            B = value.stop  if isinstance(value, slice) else value + 1; B = int(self.shape[2]) if B is None else B
        if self.pdim==3:
            A = value.start if isinstance(value, slice) else value    ; A = int(0)             if A is None else A
            B = value.stop  if isinstance(value, slice) else value + 1; B = int(self.shape[3]) if B is None else B
        return (A,B)
    def _getYRange(self, value):
        if self.pdim==2:
            A = value.start if isinstance(value, slice) else value    ; A = 0             if A is None else A
            B = value.stop  if isinstance(value, slice) else value + 1; B = int(self.shape[1]) if B is None else B
        if self.pdim==3:
            A = value.start if isinstance(value, slice) else value    ; A = int(0)             if A is None else A
            B = value.stop  if isinstance(value, slice) else value + 1; B = int(self.shape[2]) if B is None else B
        return (A,B)
    
    def _getZRange(self, value):
      
        A = value.start if isinstance(value, slice) else value    ; A = int(0)             if A is None else A
        B = value.stop  if isinstance(value, slice) else value + 1; B = int(self.shape[1]) if B is None else B
        return (A,B)

    def _getResRange(self, value):
        A = value.start if isinstance(value, slice) else value    ; A =0         if A is None else A
        B = value.stop  if isinstance(value, slice) else value + 1; B =  self.db.getMaxResolution()  +1 if B is None else B
        return (A,B)
    
    def _getTRange(self, value):

        A =  value.start if isinstance(value, slice) else value    ;A= int(self.shape[0])-1 if A is None else A
        B =  value.stop  if isinstance(value, slice) else value + 1; B=1 if B is None else B

        return (A,B)
    # __readSamples
    def _raw_indexing_method(self, key: tuple) -> np.typing.ArrayLike:
        print("_raw_indexing_method","key",key)
        if self.pdim==2:
            t1,t2=self._getTRange(key[0])
            y1,y2=self._getYRange(key[1])
            x1,x2=self._getXRange(key[2])
            
            if isinstance(self.resolution,int):
                res=self.resolution
            else:
                res,res1=self._getResRange(key[3])
                if res==0:  
                    res= self.db.getMaxResolution()
                    print('Using Max Resolution: ',res)

            if isinstance(self.timesteps,int):
                data=self.db.read(time=self.timesteps,max_resolution=res, logic_box=[(x1,y1),(x2,y2)],field=self.fieldname)
            else:
                if isinstance(t1,int) and isinstance(res,int):
                    data=self.db.read(time=t1,max_resolution=res,logic_box=[(x1,y1),(x2,y2)],field=self.fieldname)
                else:
                    data=self.db.read(logic_box=[(x1,y1),(x2,y2)],max_resolution=self.db.getMaxResolution(),field=self.fieldname)
                
        elif self.pdim==3:
            
            t1,t2=self._getTRange(key[0])
            z1,z2=self._getZRange(key[1])
            y1,y2=self._getYRange(key[2])
            print(self.resolution)
            
            
            if isinstance(self.resolution,int):
                res=self.resolution
            else:
                res,res1=self._getResRange(key[4])

                if res==0:
                    self.shape.pop()
                    res= self.db.getMaxResolution()
                    print('Using Max Resolution: ',res)

            if isinstance(self.timesteps,int):
                x1,x2=self._getXRange(key[3])
                data=self.db.read(time=self.timesteps,max_resolution=res, logic_box=[(x1,y1,z1),(x2,y2,z2)],field=self.fieldname)
            elif len(self.timesteps)==1:
                x1,x2=self._getXRange(key[3])
                data=self.db.read(max_resolution=res,logic_box=[(x1,y1,z1),(x2,y2,z2)],field=self.fieldname)

 
            else:
                
                if isinstance(t1, int) and isinstance(res,int):
                    x1,x2=self._getXRange(key[3])

                    data=self.db.read(time=t1, max_resolution=res,logic_box=[(x1,y1,z1),(x2,y2,z2)])
                else:
                    data=self.db.read(logic_box=[(x1,y1,z1),(x2,y2,z2)],field=self.fieldname)
                    
                    
        else:
            raise Exception("dimension error")


        
        return np.array(data)
    # __getitem__
    def __getitem__(self, key: xr.core.indexing.ExplicitIndexer) -> np.typing.ArrayLike:
        return xr.core.indexing.explicit_indexing_adapter(key,self.shape,
                                                          xr.core.indexing.IndexingSupport.BASIC,
                                                          self._raw_indexing_method)


class OpenVisusBackendEntrypoint(xr.backends.common.BackendEntrypoint):

    open_dataset_parameters = ["filename_or_obj", "drop_variables", "resolution", "timesteps","coordinates"]
    
    # open_dataset
    def open_dataset(self,filename_or_obj,*, resolution=None, timesteps=None,drop_variables=None,coords=None,attrs=None,dims=None):

        self.resolution=resolution
        
        self.coordinates=coords
        data_vars={}

        ds=xr.open_dataset(filename_or_obj,decode_times=False)
        if drop_variables!= None:
            for i in drop_variables:
                ds=ds.drop(i)
        if 'time' in ds:
            ds=ds.drop('time')
            
        if 'idx_url' not in ds.attrs:
            raise Exception("IDX url not found in dataset attributes")
        
        idx_file=ds.attrs['idx_url']
        db=ov.LoadDataset(idx_file)
        # if self.resolution==None:
        #     self.resolution=db.getMaxResolution()
        self.timesteps=timesteps
        dim=db.getPointDim()
        dims=db.getLogicSize()
        
        if self.timesteps==None:
            self.timesteps=db.getTimesteps()
            
        # convert OpenVisus fields into xarray variables
        for fieldname in db.getFields():
            field=db.getField(fieldname)
            
            ncomponents=field.dtype.ncomponents()
            atomic_dtype=field.dtype.get(0)

            dtype=self.toNumPyDType(atomic_dtype)
            shape=list(reversed(dims))
           
            
            if self.coordinates==None:
                if ds[fieldname].coords:
                    labels=[i for i in ds[fieldname].coords]
                else:
                    labels=[i for i in ds[fieldname].dims]

            

                if ncomponents>1:
                    labels.append("channel")
                    shape.append(ncomponents)
                labels.insert(0,"time")
                labels.append("resolution")
                if isinstance(self.resolution,int):
                    
                    shape.append(self.resolution+1)
                else:
                    shape.append(db.getMaxResolution()+1)
                if isinstance(self.timesteps, int):
                    shape.insert(0,self.timesteps+1)
                else:
                    shape.insert(0,len(self.timesteps))
                

        
            data_vars[fieldname]=xr.Variable(
                labels,
                xr.core.indexing.LazilyIndexedArray(OpenVisusBackendArray(db=db, shape=shape,dtype=dtype,
                fieldname=fieldname,
                                                                          timesteps=self.timesteps,
                                                                          resolution=self.resolution)),
                attrs=ds[fieldname].attrs
            )
            print(resolution)
            print("Adding field ",fieldname,"shape ",shape,"dtype ",dtype,"labels ",labels,
                 "Max Resolution ", db.getMaxResolution())
        
            
        ds1 = xr.Dataset(data_vars=data_vars,attrs=ds.attrs)
        coord_name=[i for i in ds.coords]
        # a=data_vars['A'].shape
        # print(a)
        for coord in coord_name:
#             if self.resolution==db.getMaxResolution():
            ds1[coord]=ds.coords[coord].values
            if coord in ds1.coords:
                ds1[coord].attrs=ds[coord].attrs
            
        ds1.attrs=ds.attrs

        ds1.set_close(self.close_method)
        
        return ds1
    
    # toNumPyDType (always pass the atomic OpenVisus type i.e. uint8[8] should not be accepted)
    def toNumPyDType(self,atomic_dtype):
        """
        convert an Openvisus dtype to numpy dtype
        """

        # dtype  (<: little-endian, >: big-endian, |: not-relevant) ; integer providing the number of bytes  ; i (integer) u (unsigned integer) f (floating point)
        return np.dtype("".join([
            "|" if atomic_dtype.getBitSize()==8 else "<",
            "f" if atomic_dtype.isDecimal() else ("u" if atomic_dtype.isUnsigned() else "i"),
            str(int(atomic_dtype.getBitSize()/8))
        ]))

    # close_method
    def close_method(self):
        print("nothing to do here")
    
    # guess_can_open
    def guess_can_open(self, filename_or_obj):
        print("guess_can_open",filename_or_obj)

        # todo: extend to S3 datasets
        if "mod_visus" in filename_or_obj:
            return True
         
        # local files
        try:
            _, ext = os.path.splitext(filename_or_obj)
        except TypeError:
            return False
        return ext.lower()==".idx"


def open_dataset(filename,time=None,quality=0,field=None):
    d=xr.open_dataset(filename,decode_times=False)
    idx_file=d.attrs['idx_url']
    if 'time' in d or 'Time' in d:
        print('Time variables detected!!!')
        time=time
        try:
            d=d.drop('time')
        except ValueError:
            d=d.drop('Time')
    ds=xr.Dataset()
    db=ov.LoadDataset(idx_file)
    if time==None:
        time=0
        print(f'loading time: {time}')
    if field==None:
        field=db.getFields()[0]
    if quality==0:
        for f in db.getFields():
            data=db.read(field=f,time=time,quality=quality)
            data=[data]
            ds[f]=xr.DataArray(data,dims=d[f].dims,coords=d[f].coords,attrs=d[f].attrs)
    else:
        for f in db.getFields():
            data=db.read(field=f,time=time,quality=quality)
            data=[data]
            original_lat_values=d[f].lat.values
            num_coordinates=len(data[0])
            indices = np.linspace(0, len(original_lat_values) - 1, num_coordinates, dtype=int)
            downsampled_lat_values = original_lat_values[indices]
            original_lon_values=d[f].lon.values
            num_coordinates=len(data[0][0])
            indices = np.linspace(0, len(original_lon_values) - 1, num_coordinates, dtype=int)
            downsampled_lon_values = original_lon_values[indices]
  

            ds[f]=xr.DataArray(data,dims=d[f].dims,coords={'lat':downsampled_lat_values,'lon':downsampled_lon_values},attrs=d[f].attrs)
            ds[f]['lat'].attrs=d[f]['lat'].attrs
            ds[f]['lon'].attrs=d[f]['lon'].attrs
    ds.attrs=d.attrs
    return ds


import xarray as xr

def reset_coordinates(ds):
    new_coords = {}
    for var_name, var_data in ds.data_vars.items():
        for dim_name, dim_size in zip(var_data.dims, var_data.shape):
            if dim_name in ds.coords:
                factor = len(ds[dim_name]) // dim_size
                if factor > 1:
                    new_coords[dim_name] = ds[dim_name][::factor][:dim_size]
                else:
                    new_coords[dim_name] = ds[dim_name]

    # Create a new dataset with the updated coordinates
    new_ds = xr.Dataset(coords=new_coords)

    # Copy the data variables from the old dataset to the new dataset
    for var_name, var_data in ds.data_vars.items():
        new_var = xr.Variable(
            dims=var_data.dims,
            data=var_data.values,
            attrs=var_data.attrs
        )
        new_ds[var_name] = new_var

    # Copy the global attributes from the old dataset to the new dataset
    new_ds.attrs = ds.attrs

    return new_ds








