
import os,sys,logging

os.environ["BOKEH_ALLOW_WS_ORIGIN"]="*"
os.environ["BOKEH_LOG_LEVEL"]="debug" 

import bokeh
import bokeh.io 
import bokeh.io.notebook 
import bokeh.models.widgets  
import bokeh.core.validation
import bokeh.plotting
import bokeh.core.validation.warnings
import bokeh.layouts
from bokeh.models import Slider,Div,Button,TextInput,Select,Row,Column

bokeh.core.validation.silence(bokeh.core.validation.warnings.EMPTY_LAYOUT, True)
bokeh.io.output_notebook()

sys.path.append(r"C:\projects\OpenVisus\build\RelWithDebInfo")
import OpenVisus
from OpenVisus.dashboards import LoadDataset, Slice, Slices, DIRECTIONS, SetupLogger

# ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
def MyApp(doc):
    
	os.environ["VISUS_NETSERVICE_VERBOSE"]="1"
	os.environ["VISUS_CPP_VERBOSE"]="1"
	os.environ["VISUS_DASHBOARDS_VERBOSE"]="0" 

	DIRECTIONS=[('0','Long'),('1','Lat'),('2','Depth')]
	
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

	slices=Slices(show_options=["num_views","palette","timestep","timestep_delta","field","viewdep","quality","num_refinements","!direction","!offset","play-button", "play-msec"])
	slices.logic_to_pixel=logic_to_pixel
	slices.slice_show_options=["direction","offset","viewdep","status_bar"]

	db=LoadDataset(urls[0])
	print(db.getDatasetBody().toString())
	timesteps=db.getTimesteps()

	slices.setDataset(db)
	slices.setNumberOfViews(3)
	slices.setQuality(-3)
	slices.setNumberOfRefinements(3)
	slices.setPalette(palette, palette_range=palette_range) 

	slices.setTimestepDelta(10)

	if field is not None:
		slices.setField(field)

	# change zone
	if len(urls)>0:
		url = Select(title="Zone", options=[str(I) for I in range(len(urls))],width=100) 
		def onUrlChanged(attr, old, new):
			nonlocal slices
			url=urls[int(new)]
			print(f"Setting url={url}")
			db=LoadDataset(url)
			print(db.getDatasetBody().toString())
			slices.setDataset(db,compatible=True)
		url.on_change("value",onUrlChanged)
	else:
		url=None


	# add the zone at the beginning of first row
	first_row=slices.layout.children[0]
	if url:
		first_row.children.insert(0,url)

	main_layout=Column(
		slices.layout,
  		sizing_mode='stretch_both'
	)

	doc.add_root(main_layout)
 
# //////////////////////////////////////////////////////////////////////////////////////
if __name__.startswith('bokeh'):
	# python -m bokeh serve "Samples/jupyter/notebooks/10-bokeh-nasa.py"  --dev --log-level=debug --address localhost --port 8888 
	SetupLogger(logging.getLogger("OpenVisus.dashboards"))
	doc=bokeh.io.curdoc()
	doc.theme = 'caliber'
	MyApp(doc)

	

		 