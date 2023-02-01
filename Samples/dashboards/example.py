import os,sys,time,datetime,threading,queue,random,copy,math, types, multiprocessing
import argparse

import OpenVisus as ov
from OpenVisus.dashboards import Slice,Slices
import bokeh.io

	# //////////////////////////////////////////////////////////////////////////////////////
if __name__.startswith('bokeh'):

	doc=bokeh.io.curdoc()
	doc.theme = 'caliber'

	parser = argparse.ArgumentParser()
	parser.add_argument("--multiple"     , help="enable multiple slices"  , action="store_true")
	parser.add_argument("--palette-range", help="palette range", type=str, required=False, default="0 255")
	parser.add_argument("url", type=str)
	args = parser.parse_args()
	print("Got arguments",args)
 
	db=ov.LoadDataset(args.url)
 
	m,M=[float(value) for value in args.palette_range.split()]
	palette_range=(m,M)
 
	# single slice
	if not args.multiple:
		slice=Slice(doc, sizing_mode='stretch_both')
		slice.setDataset(db, direction=2)
		slice.setPalette("Greys256", palette_range=palette_range)
		slice.setTimestep(db.getTime())
		slice.setField(db.getField().name)  
		doc.add_root(slice.layout)

	# multiple slices
	else:
		slices=Slices(doc, sizing_mode='stretch_both')
		slices.setDataset(db,layout=4)
		slices.setPalette("Greys256", palette_range=palette_range)
		slices.setTimestep(db.getTime())
		slices.setField(db.getField().name) 
		doc.add_root(slices.layout)



	 
 




