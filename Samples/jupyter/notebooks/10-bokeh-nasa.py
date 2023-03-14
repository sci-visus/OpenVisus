
import os,sys,logging

os.environ["BOKEH_ALLOW_WS_ORIGIN"]="*"
os.environ["BOKEH_LOG_LEVEL"]="debug" 
import bokeh

sys.path.append(r"C:\projects\OpenVisus\build\RelWithDebInfo")
import OpenVisus
from OpenVisus.dashboards import Slice, Slices, SetupLogger

# ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
def MyApp(doc=bokeh.io.curdoc()):
	
	os.environ["VISUS_NETSERVICE_VERBOSE"]="0"
	os.environ["VISUS_CPP_VERBOSE"]="0"
	os.environ["VISUS_DASHBOARDS_VERBOSE"]="0" 

	# profile=sealstorage
	os.environ["AWS_ACCESS_KEY_ID"]="any"
	os.environ["AWS_SECRET_ACCESS_KEY"]="any"
	os.environ["AWS_ENDPOINT_URL"]="https://maritime.sealstorage.io/api/v0/s3"

	urls=[f"https://maritime.sealstorage.io/api/v0/s3/utah/nasa/dyamond/idx_arco/face{zone}/u_face_{zone}_depth_52_time_0_10269.idx?cached=1" for zone in range(6)]
	palette,palette_range="Turbo256",(-30,60)
	field=None
	logic_to_pixel=[(0.0,1.0), (0.0,1.0), (0.0,10.0)]
	
	# urls=["https://maritime.sealstorage.io/api/v0/s3/utah/nasa/dyamond/mit_output/llc2160_arco/visus.idx?cached=1"]
	# palette,palette_range="Turbo256",(-1.3,1.7)
	# field=None
	# logic_to_pixel=[(0.0,1.0), (0.0,1.0), (0.0,20.0)]


	# urls=["http://atlantis.sci.utah.edu/mod_visus?dataset=cmip6_cm2&cached=idx"]
	# palette,palette_range="Turbo256",(-1.3,1.7)
	# field="ssp585_tasmax"
	# logic_to_pixel=[(0.0,1.0), (0.0,1.0), (0.0,20.0)]

	slices=Slices(
				doc=doc,
			   show_options=["num_views","palette","dataset","timestep","timestep-delta","field","viewdep","quality","num_refinements","play-button", "play-sec"],
			   #slice_show_options=["num_views","palette","dataset","timestep","timestep-delta","field","viewdep","quality","num_refinements","play-button", "play-sec"],
			   slice_show_options=["direction","offset","viewdep","status_bar"],
			   )
 
	slices.setNumberOfViews(3)
	slices.setDatasets([(url,str(I)) for I,url in enumerate(urls)],"Zone")
	slices.setDataset(urls[0])
	slices.setQuality(-3)
	slices.setNumberOfRefinements(3)
	slices.setPalette(palette) 
	slices.setPaletteRange(palette_range)
	slices.setTimestepDelta(10)
	slices.setField(field)
	slices.setLogicToPixel(logic_to_pixel)
	slices.setDirections([('0','Long'),('1','Lat'),('2','Depth')])
 
	doc.add_root(slices.layout)
 
# //////////////////////////////////////////////////////////////////////////////////////
if __name__.startswith('bokeh'):
	# python -m bokeh serve "Samples/jupyter/notebooks/10-bokeh-nasa.py"  --dev --log-level=debug --address localhost --port 8888 
	logger=logging.getLogger("OpenVisus.dashboards")
	logger.setLevel(logging.INFO)
	MyApp()

	

		 