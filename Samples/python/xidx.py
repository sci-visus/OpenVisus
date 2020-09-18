import OpenVisus
import unittest
import filecmp

from OpenVisus import *

def Print(*args, **kwargs):
	pass

class TextXIdx(unittest.TestCase):

	def testXIdx(self):
		self.filename="tmp/debug_xidx/test.xidx"
		self.verify_filename="tmp/debug_xidx/verify.xidx"
		self.WriteXIdx()
		self.ReadXIdx()
		self.VerifyXIdx()

	# WriteXIdx
	def WriteXIdx(self):
		n_ts = 10

		# create metadata file
		meta = XIdxFile()

		# create time group
		time_group = Group("TimeSeries", GroupType(GroupType.TEMPORAL_GROUP_TYPE))

		# set data source to the dataset file
		source = DataSource("data", "file_path")
		time_group.addDataSource(source)

		# create a list domain for the temporal group
		time_dom = ListDomain("Time")

		# add attributes for the domain
		time_dom.addAttribute("units", "days since 1980")
		time_dom.addAttribute("calendar", "gregorian")
		
		# add time values
		for i in range(0,n_ts):
			ret = time_dom.addDomainItem(float(i)*0.25)
			# you can also use a tuple (e.g., bounds for netcdf)
			#time_dom.addDomainItems(IndexSpace([float(i)*0.25,float(i)*0.50]))
			
		# set group time domain
		time_group.setDomain(time_dom)
		
		# create grid domain
		geo_dom = MultiAxisDomain("Geospatial")

		# create axis
		latitude_axis = Variable("latitude")
		longitude_axis = Variable("longitude")
		
		# Populate the axis with explicit values (will be written in the XML)
		for i in range(0,10):
			latitude_axis.addValue(float(i)*0.5)
			longitude_axis.addValue(float(i)*2*0.6)
			# you can also use a tuple (e.g., bounds for netcdf)
			#longitude_axis.addValues(IndexSpace([float(i)*2*0.6,float(i)*2*1.2]))

		# add attributes to axis
		latitude_axis.addAttribute("units", "degrees_north")
		latitude_axis.addAttribute("units", "degrees_east")

		# add axis to the domain
		geo_dom.addAxis(latitude_axis)
		geo_dom.addAxis(longitude_axis)

		geo_vars = Group("geo_vars", GroupType(GroupType.SPATIAL_GROUP_TYPE), geo_dom);

		# create and add a variable to the group
		geo_vars.addVariable("geo_temperature", DType.fromString("float32"));

		# pick last variable added
		temp = geo_vars.variables.back()

		# add attribute to the variable (key-value) pairs
		temp.addAttribute("unit", "Celsius");
		temp.addAttribute("valid_min", "-100.0");
		temp.addAttribute("valid_max", "200.0");

		time_group.addGroup(geo_vars);

		# set the root group of the metadata
		meta.addGroup(time_group);
		# write to disk
		meta.save(self.filename);

	# ReadXIdx
	def ReadXIdx(self):
		# read data from input file
		metadata = XIdxFile_load(self.filename)

		# get time group
		root_group = metadata.getGroup(GroupType(GroupType.TEMPORAL_GROUP_TYPE))
		self.assertIsNotNone(root_group)

		# get domain of the group
		domain = root_group.getDomain()
		self.assertIsNotNone(domain)

		Print("Time Domain[",domain.type.toString(),"]:")

		# Printattributes if any
		for att in domain.attributes:
			Print("\t\tAttribute:", att.name, "Value:", att.value)

		t_count=0
		# loop over the list of timesteps
		for t in domain.getLinearizedIndexSpace():
			Print("Timestep", t)

			# get the grid of timestep t
			grid = root_group.getGroup(t_count)
			self.assertIsNotNone(grid)

			t_count += 1

			# get domain of current grid
			grid_domain = grid.getDomain()
			self.assertIsNotNone(grid_domain)

			Print("\tGrid Domain[", grid_domain.type.toString(), "]")

			# Printattributes if any
			for att in grid_domain.attributes:
				Print("\t\tAttribute:", att.name, "Value:", att.value)

			if(grid_domain.type.value == DomainType.SPATIAL_DOMAIN_TYPE):
				topology =	grid_domain.topology
				geometry = grid_domain.geometry
				Print("\tTopology", topology.type.toString(), "volume ", grid_domain.getVolume())
				Print("\tGeometry", geometry.type.toString())
			elif(grid_domain.type.value == DomainType.MULTIAXIS_DOMAIN_TYPE):
				# loop over the axis
				for axis in grid_domain.axis:
					Print("\tAxis", axis.name,"volume", axis.getVolume(),": [ ", end='')

					for v in axis.getValues():
						Print(v, end='')
					Print(" ]")

					# Printattributes of the axis if any
					for att in axis.attributes:
						Print("\t\tAttribute:", att.name, "Value:", att.value)

				Print("\n");

				# loop over variables
				for var in grid.variables:
					# get datasource used by the first variable
					source = var.data_items[0].findDataSource();
					Print("\t\tVariable:", var.name)
					if(source):
						Print("\t\t\tdata source url: ", source.url)
					else:
						Print("\n")

					# Printattributes of the variable if any
					for att in var.attributes:
						Print("\t\tAttribute:", att.name, "Value:", att.value)

			# (Debug) Saving the content in a different file to compare with the original
			metadata.save(self.verify_filename);

	# VerifyXIdx
	def VerifyXIdx(self):
		self.assertTrue(filecmp.cmp(self.filename, self.verify_filename))


	# ////////////////////////////////////////////////////////
if __name__ == '__main__':
	unittest.main(verbosity=2,exit=True)
	Print("All done")
