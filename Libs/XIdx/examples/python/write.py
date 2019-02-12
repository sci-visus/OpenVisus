import OpenVisus

from VisusXIdxPy import *
from OpenVisus import *

n_ts = 10
filepath = "test.xidx"

# create metadata file
meta = XIdxFilePtr(XIdxFile())

# create time group
time_group = GroupPtr(Group("TimeSeries", GroupType(GroupType.TEMPORAL_GROUP_TYPE)))

# set data source to the dataset file
source = DataSourcePtr(DataSource("data", "file_path"))
time_group.get().addDataSource(source)

# create a list domain for the temporal group
time_dom = ListDomainPtr(ListDomain("Time"))

# add attributes for the domain
time_dom.get().addAttribute("units", "days since 1980")
time_dom.get().addAttribute("calendar", "gregorian")

# add time values
for i in range(0,n_ts):
  ret = time_dom.get().addDomainItem(float(i)*0.25)
  # you can also use a tuple (e.g., bounds for netcdf)
  #time_dom.addDomainItems(IndexSpace([float(i)*0.25,float(i)*0.50]))

# set group time domain
time_group.get().setDomain(DomainPtr(time_dom.get()))

# create grid domain 
geo_dom = MultiAxisDomainPtr(MultiAxisDomain("Geospatial"))

# create axis
latitude_axis = VariablePtr(Variable("latitude"));
longitude_axis = VariablePtr(Variable("longitude"));

# Populate the axis with explicit values (will be written in the XML)
for i in range(0,10):
  latitude_axis.get().addValue(float(i)*0.5)
  longitude_axis.get().addValue(float(i)*2*0.6)
  # you can also use a tuple (e.g., bounds for netcdf)
  #longitude_axis.addValues(IndexSpace([float(i)*2*0.6,float(i)*2*1.2]))

# add attributes to axis
latitude_axis.get().addAttribute("units", "degrees_north")
latitude_axis.get().addAttribute("units", "degrees_east")

# add axis to the domain
geo_dom.get().addAxis(latitude_axis)
geo_dom.get().addAxis(longitude_axis)

geo_vars = GroupPtr(Group("geo_vars", GroupType(GroupType.SPATIAL_GROUP_TYPE), DomainPtr(geo_dom.get())));
  
# create and add a variable to the group
geo_vars.get().addVariable("geo_temperature", DType.fromString("float32"));

# pick last variable added
temp = geo_vars.get().variables.back().get()

# add attribute to the variable (key-value) pairs
temp.addAttribute("unit", "Celsius");
temp.addAttribute("valid_min", "-100.0");
temp.addAttribute("valid_max", "200.0");

time_group.get().addGroup(geo_vars);
  
# set the root group of the metadata
meta.get().addGroup(time_group);
# write to disk
meta.get().save(filepath);

print ("write done")