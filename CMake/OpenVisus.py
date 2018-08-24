
import os,sys

__this_dir__=os.path.dirname(os.path.abspath(__file__))
if not __this_dir__ in sys.path:
  sys.path.append(__this_dir__)

# /////////////////////////////////////////////////////////
def check():
  import VisusKernelPy
  import VisusDbPy
  import VisusIdxPy
  import VisusDataflowPy
  import VisusNodesPy
  import VisusGuiPy
  import VisusGuiNodesPy
  import VisusAppKitPy
    
from VisusKernelPy import *  
