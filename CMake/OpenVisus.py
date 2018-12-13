import sys,os

__this_dir__=os.path.dirname(os.path.abspath(__file__))

for it in (".","bin"):
	dir = os.path.join(__this_dir__,it)
	if not dir in sys.path and os.path.isdir(dir):
		sys.path.append(dir)

import VisusKernelPy


