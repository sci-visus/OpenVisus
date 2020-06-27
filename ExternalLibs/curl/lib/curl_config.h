

#if WIN32
	#include "curl_config.win32.h"
#elif __clang__
	#include "curl_config.apple.h"
#else
	#include "curl_config.linux.h"
#endif
