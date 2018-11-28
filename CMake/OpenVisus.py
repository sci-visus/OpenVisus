import os,sys

__this_dir__=os.path.abspath(os.path.dirname(os.path.abspath(__file__)))
if not __this_dir__ in sys.path:
  sys.path.append(__this_dir__)
  
__bin_dir__=os.path.abspath(__this_dir__+ "/bin")
if not __bin_dir__ in sys.path:
  sys.path.append(__bin_dir__)

from VisusKernelPy import *

