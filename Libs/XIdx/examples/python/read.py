import xidx
import sys

from xidx import *

if(len(sys.argv) < 2):
  print("usage: ", sys.argv[0], "<input file>")
  exit(1)

filepath = sys.argv[1]

# create metadata file object
metadata = MetadataFile(filepath)
# read data from input file
ret = metadata.Load()

# get root group (e.g. Time series)
root_group = metadata.getRootGroup()

# get domain of the group
domain = root_group.getDomain()

print ("Time Domain[",Domain.toString(domain.getType()),"]:")

# print attributes if any
for att in domain.getAttributes():
  print ("\t\tAttribute:", att.name, "Value:", att.value)

t_count=0
# loop over the list of timesteps
for t in domain.getLinearizedIndexSpace():
  print ("Timestep", t)
    
  # get the grid of timestep t
  grid = root_group.getGroup(t_count)
  t_count += 1

  # get domain of current grid
  grid_domain = grid.getDomain()

  print ("\tGrid Domain[", Domain.toString(grid_domain.getType()), "]")

  # print attributes if any
  for att in grid_domain.getAttributes():
    print ("\t\tAttribute:", att.name, "Value:", att.value)

  if(grid_domain.getType() == Domain.SPATIAL_DOMAIN_TYPE):
    topology =  grid_domain.topology
    geometry = grid_domain.geometry
    print ("\tTopology", Topology.toString(topology.type), "volume ", grid_domain.getVolume())
    print ("\tGeometry", Geometry.toString(geometry.type))
  elif(grid_domain.getType() == Domain.MULTIAXIS_DOMAIN_TYPE):
    # loop over the axis
    for a in range(0,grid_domain.getNumberOfAxis()):
      # get axis
      axis = grid_domain.getAxis(a);
      print ("\tAxis", axis.name,"volume", axis.getVolume(),": [ ", end='')

      for v in axis.getValues():
        print (v, end='')

      print(" ]")

      # print attributes of the axis if any
      for att in axis.getAttributes():
        print ("\t\tAttribute:", att.name, "Value:", att.value)

    print("\n");

    # loop over variables
    for var in grid.getVariables():
      # get datasource used by the first variable
      source = var.getDataItems()[0].getDataSource();
      print("\t\tVariable:", var.name)
      if(source):
        print("\t\t\tdata source url: ", source.getUrl())
      else:
        print("\n")

      # print attributes of the variable if any
      for att in var.getAttributes():
        print ("\t\tAttribute:", att.name, "Value:", att.value)
        
# (Debug) Saving the content in a different file to compare with the original
metadata.save("verify.xidx");
print("debug saved into verify.xidx")


