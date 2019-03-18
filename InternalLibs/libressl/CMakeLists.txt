set(LIBRESSL_SKIP_INSTALL        1)
set(LIBRESSL_APPS                0)
set(LIBRESSL_TESTS               0)
set(ENABLE_ASM "Enable assembly" 0)
set(ENABLE_EXTRATESTS            0)
set(ENABLE_NC                    0)
set(OPENSSLDIR                   "")
set(BUILD_NC                     0)


DisableAllWarnings()

include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckLibraryExists)
include(CheckIncludeFiles)
include(CheckTypeSize)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_MODULE_PATH})
include(cmake_export_symbol)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/ssl/VERSION SSL_VERSION)
string(STRIP ${SSL_VERSION} SSL_VERSION)
string(REPLACE ":" "." SSL_VERSION ${SSL_VERSION})
string(REGEX REPLACE "\\..*" "" SSL_MAJOR_VERSION ${SSL_VERSION})

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/crypto/VERSION CRYPTO_VERSION)
string(STRIP ${CRYPTO_VERSION} CRYPTO_VERSION)
string(REPLACE ":" "." CRYPTO_VERSION ${CRYPTO_VERSION})
string(REGEX REPLACE "\\..*" "" CRYPTO_MAJOR_VERSION ${CRYPTO_VERSION})

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/tls/VERSION TLS_VERSION)
string(STRIP ${TLS_VERSION} TLS_VERSION)
string(REPLACE ":" "." TLS_VERSION ${TLS_VERSION})
string(REGEX REPLACE "\\..*" "" TLS_MAJOR_VERSION ${TLS_VERSION})

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	add_definitions(-fno-common)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
	add_definitions(-DHAVE_ATTRIBUTE__BOUNDED__)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	add_definitions(-D_DEFAULT_SOURCE)
	add_definitions(-D_BSD_SOURCE)
	add_definitions(-D_POSIX_SOURCE)
	add_definitions(-D_GNU_SOURCE)
	set(PLATFORM_LIBS ${PLATFORM_LIBS} pthread)
endif()


add_definitions(-DLIBRESSL_INTERNAL)
add_definitions(-DOPENSSL_NO_HW_PADLOCK)
add_definitions(-D__BEGIN_HIDDEN_DECLS=)
add_definitions(-D__END_HIDDEN_DECLS=)

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
	add_definitions(-Wno-pointer-sign)
endif()

if(WIN32)
	add_definitions(-Drestrict)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_CRT_DEPRECATED_NO_WARNINGS)
	add_definitions(-D_REENTRANT -D_POSIX_THREAD_SAFE_FUNCTIONS)
	add_definitions(-DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x0600)
	add_definitions(-DCPPFLAGS -DOPENSSL_NO_SPEED -DNO_SYSLOG -DNO_CRYPT)
	set(PLATFORM_LIBS ${PLATFORM_LIBS} ws2_32)
endif()

check_function_exists(asprintf HAVE_ASPRINTF)
if(HAVE_ASPRINTF)
	add_definitions(-DHAVE_ASPRINTF)
endif()

check_function_exists(reallocarray HAVE_REALLOCARRAY)
if(HAVE_REALLOCARRAY)
	add_definitions(-DHAVE_REALLOCARRAY)
endif()

check_function_exists(strcasecmp HAVE_STRCASECMP)
if(HAVE_STRCASECMP)
	add_definitions(-DHAVE_STRCASECMP)
endif()

check_function_exists(strlcat HAVE_STRLCAT)
if(HAVE_STRLCAT)
	add_definitions(-DHAVE_STRLCAT)
endif()

check_function_exists(strlcpy HAVE_STRLCPY)
if(HAVE_STRLCPY)
	add_definitions(-DHAVE_STRLCPY)
endif()

check_function_exists(strndup HAVE_STRNDUP)
if(HAVE_STRNDUP)
	add_definitions(-DHAVE_STRNDUP)
endif()

if(WIN32)
	set(HAVE_STRNLEN true)
	add_definitions(-DHAVE_STRNLEN)
else()
	check_function_exists(strnlen HAVE_STRNLEN)
	if(HAVE_STRNLEN)
		add_definitions(-DHAVE_STRNLEN)
	endif()
endif()

check_function_exists(strsep HAVE_STRSEP)
if(HAVE_STRSEP)
	add_definitions(-DHAVE_STRSEP)
endif()

check_function_exists(timegm HAVE_TIMEGM)
if(HAVE_TIMEGM)
	add_definitions(-DHAVE_TIMEGM)
endif()

check_function_exists(arc4random_buf HAVE_ARC4RANDOM_BUF)
if(HAVE_ARC4RANDOM_BUF)
	add_definitions(-DHAVE_ARC4RANDOM_BUF)
endif()

check_function_exists(arc4random_uniform HAVE_ARC4RANDOM_UNIFORM)
if(HAVE_ARC4RANDOM_UNIFORM)
	add_definitions(-DHAVE_ARC4RANDOM_UNIFORM)
endif()

check_function_exists(explicit_bzero HAVE_EXPLICIT_BZERO)
if(HAVE_EXPLICIT_BZERO)
	add_definitions(-DHAVE_EXPLICIT_BZERO)
endif()

check_function_exists(getauxval HAVE_GETAUXVAL)
if(HAVE_GETAUXVAL)
	add_definitions(-DHAVE_GETAUXVAL)
endif()

check_function_exists(getentropy HAVE_GETENTROPY)
if(HAVE_GETENTROPY)
	add_definitions(-DHAVE_GETENTROPY)
endif()

check_symbol_exists(getpagesize unistd.h HAVE_GETPAGESIZE)
if(HAVE_GETPAGESIZE)
	add_definitions(-DHAVE_GETPAGESIZE)
endif()

check_function_exists(timingsafe_bcmp HAVE_TIMINGSAFE_BCMP)
if(HAVE_TIMINGSAFE_BCMP)
	add_definitions(-DHAVE_TIMINGSAFE_BCMP)
endif()

check_function_exists(timingsafe_memcmp HAVE_TIMINGSAFE_MEMCMP)
if(HAVE_MEMCMP)
	add_definitions(-DHAVE_MEMCMP)
endif()

check_function_exists(memmem HAVE_MEMMEM)
if(HAVE_MEMMEM)
	add_definitions(-DHAVE_MEMMEM)
endif()

check_include_files(err.h HAVE_ERR_H)
if(HAVE_ERR_H)
	add_definitions(-DHAVE_ERR_H)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	# Check if we need -lrt to get clock_gettime on Linux
	check_library_exists(rt clock_gettime "time.h" HAVE_CLOCK_GETTIME)
	if (HAVE_CLOCK_GETTIME)
		set(PLATFORM_LIBS ${PLATFORM_LIBS} rt)
	endif()
else()
	# Otherwise, simply check if it exists
	check_function_exists(clock_gettime HAVE_CLOCK_GETTIME)
endif()
if(HAVE_CLOCK_GETTIME)
	add_definitions(-DHAVE_CLOCK_GETTIME)
endif()

check_type_size(time_t SIZEOF_TIME_T)
if(SIZEOF_TIME_T STREQUAL "4")
	set(SMALL_TIME_T true)
	add_definitions(-DSMALL_TIME_T)
	message(WARNING " ** Warning, this system is unable to represent times past 2038 It will behave incorrectly when handling valid RFC5280 dates")
endif()
add_definitions(-DSIZEOF_TIME_T=${SIZEOF_TIME_T})

set(OPENSSL_LIBS tls ssl crypto ${PLATFORM_LIBS})

add_subdirectory(crypto)
add_subdirectory(ssl)
add_subdirectory(tls)


set_target_properties(crypto PROPERTIES FOLDER "InternalLibs/libressl")
set_target_properties(ssl    PROPERTIES FOLDER "InternalLibs/libressl")
set_target_properties(tls    PROPERTIES FOLDER "InternalLibs/libressl")



