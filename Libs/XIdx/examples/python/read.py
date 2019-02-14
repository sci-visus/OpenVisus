import sys
import OpenVisus

from VisusXIdxPy import *

if(len(sys.argv) < 2):
  print("usage: ", sys.argv[0], "<input file>")
  exit(1)

filepath = sys.argv[1]

# read data from input file
metadata = XIdxFile_load(filepath)


print(metadata)

# get time group
root_group = metadata.getGroup(GroupType(GroupType.TEMPORAL_GROUP_TYPE))

# get domain of the group
domain = root_group.getDomain()

print ("Time Domain[",domain.type.toString(),"]:")

# print attributes if any
for att in domain.attributes:
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

  print ("\tGrid Domain[", grid_domain.type.toString(), "]")

  # print attributes if any
  for att in grid_domain.attributes:
    print ("\t\tAttribute:", att.get().name, "Value:", att.get().value)

  if(grid_domain.type.value == DomainType.SPATIAL_DOMAIN_TYPE):
    topology =  grid_domain.topology
    geometry = grid_domain.geometry
    print ("\tTopology", topology.type.toString(), "volume ", grid_domain.getVolume())
    print ("\tGeometry", geometry.type.toString())
  elif(grid_domain.type.value == DomainType.MULTIAXIS_DOMAIN_TYPE):
    # loop over the axis
    for axis in grid_domain.axis:
      print ("\tAxis", axis.get().name,"volume", axis.get().getVolume(),": [ ", end='')

      for v in axis.get().getValues():
        print (v, end='')

      print(" ]")

      # print attributes of the axis if any
      for att in axis.get().attributes:
        print ("\t\tAttribute:", att.get().name, "Value:", att.get().value)

    print("\n");

    # loop over variables
    for var in grid.variables:
      # get datasource used by the first variable
      source = var.get().data_items[0].get().findDataSource();
      print("\t\tVariable:", var.get().name)
      if(source):
        print("\t\t\tdata source url: ", source.get().getUrl())
      else:
        print("\n")

      # print attributes of the variable if any
      for att in var.get().attributes:
        print ("\t\tAttribute:", att.get().name, "Value:", att.get().value)
        
# (Debug) Saving the content in a different file to compare with the original
metadata.save("verify.xidx");
print("debug saved into verify.xidx")


