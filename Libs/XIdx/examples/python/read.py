import sys
import OpenVisus

from VisusXIdxPy import *

if(len(sys.argv) < 2):
  print("usage: ", sys.argv[0], "<input file>")
  exit(1)

XIdxModule.attach()

filepath = sys.argv[1]

# read data from input file
metadata = XIdxFile_load(filepath)

# get time group
root_group = metadata.get().getGroup(GroupType(GroupType.TEMPORAL_GROUP_TYPE))

# get domain of the group
domain = root_group.get().domain

print ("Time Domain[",domain.get().type.toString(),"]:")

# print attributes if any
for att in domain.get().attributes:
  print ("\t\tAttribute:", att.name, "Value:", att.value)

t_count=0
# loop over the list of timesteps
for t in domain.get().getLinearizedIndexSpace():
  print ("Timestep", t)
    
  # get the grid of timestep t
  grid = root_group.get().getGroup(t_count).get()
  t_count += 1

  # get domain of current grid
  grid_domain = grid.domain.get()

  print ("\tGrid Domain[", grid_domain.type, "]")

  # print attributes if any
  for att in grid_domain.attributes:
    print ("\t\tAttribute:", att.name, "Value:", att.value)

  if(grid_domain.type.value == DomainType.SPATIAL_DOMAIN_TYPE):
    topology =  grid_domain.topology.get()
    geometry = grid_domain.geometry.get()
    print ("\tTopology", topology.type, "volume ", grid_domain.getVolume())
    print ("\tGeometry", geometry.type)
  elif(grid_domain.type.value == DomainType.MULTIAXIS_DOMAIN_TYPE):
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


