import sys

print("Starting OpenVisus",__file__,sys.version,sys.version_info,"...")

from OpenVisus.VisusKernelPy   import *
from OpenVisus.VisusDbPy       import *
from OpenVisus.VisusDataflowPy import *
from OpenVisus.VisusNodesPy    import *

SetCommandLine(list(it for it in [sys.executable] + sys.argv if it))

KernelModule.attach()
DbModule.attach()
DataflowModule.attach()
NodesModule.attach()

from OpenVisus.utils         import *
from OpenVisus.dataset       import *
from OpenVisus.convert       import *



