import os,sys,time,datetime,threading,queue,random,copy,math, types, multiprocessing

import OpenVisus as ov
from OpenVisus.dashboards import Slice,Slices

	# //////////////////////////////////////////////////////////////////////////////////////
if __name__.startswith('bokeh'):

	import argparse
	parser = argparse.ArgumentParser()
	parser.add_argument("--multiple"     , help="enable multiple slices"  , action="store_true")
	parser.add_argument("--palette-range", help="palette range", type=str, required=False, default="0 255")
	parser.add_argument("url", type=str)
	args = parser.parse_args()
	print("Got arguments",args)
 
	db=ov.LoadDataset(args.url)
 
	m,M=[float(value) for value in args.palette_range.split()]
	palette_range=(m,M)
 
	# multiple slices
	if args.multiple:
		slices=Slices()
		slices.setDataset(db,layout=4)
		slices.setPalette("Greys256", palette_range=palette_range)
		layout=slices.layout
	# single slice
	else:
		slice=Slice()
		slice.setDataset(db, direction=2)
		slice.setPalette("Greys256", palette_range=palette_range)
		layout=slice.layout
 
	# examplle: multiple slices 
	import bokeh.io
	bokeh.io.curdoc().theme = 'caliber'
	bokeh.io.curdoc().add_root(layout) 




	 
 




