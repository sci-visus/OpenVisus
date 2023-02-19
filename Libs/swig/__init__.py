import os,sys,logging

logger = logging.getLogger(__name__)
logger.info(f"Starting OpenVisus {__file__} {sys.version} {sys.version_info} ...")

from OpenVisus.VisusKernelPy   import *
from OpenVisus.VisusDbPy       import *
from OpenVisus.VisusDataflowPy import *
from OpenVisus.VisusNodesPy    import *

if cbool(os.environ.get("VISUS_CPP_VERBOSE","0")):
	# keep C++ logs
	pass
else:
	# silent C++ logs
	SetLoggingFunction(lambda message: logger.info(message.rstrip("\n")))

SetCommandLine(list(it for it in [sys.executable] + sys.argv if it))

KernelModule.attach()
DbModule.attach()
DataflowModule.attach()
NodesModule.attach()

from OpenVisus.utils         import *
from OpenVisus.dataset       import *
from OpenVisus.convert       import *



