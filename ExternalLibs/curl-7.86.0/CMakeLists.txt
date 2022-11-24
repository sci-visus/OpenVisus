
SET(CURL_USE_OPENSSL       1)


set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake;${CMAKE_MODULE_PATH}")
include(Utilities)
include(Macros)
include(CMakeDependentOption)
include(CheckCCompilerFlag)

project(CURL C)

file(STRINGS ${CURL_SOURCE_DIR}/include/curl/curlver.h CURL_VERSION_H_CONTENTS REGEX "#define LIBCURL_VERSION( |_NUM )")
string(REGEX MATCH "#define LIBCURL_VERSION \"[^\"]*"
  CURL_VERSION ${CURL_VERSION_H_CONTENTS})
string(REGEX REPLACE "[^\"]+\"" "" CURL_VERSION ${CURL_VERSION})
string(REGEX MATCH "#define LIBCURL_VERSION_NUM 0x[0-9a-fA-F]+"
  CURL_VERSION_NUM ${CURL_VERSION_H_CONTENTS})
string(REGEX REPLACE "[^0]+0x" "" CURL_VERSION_NUM ${CURL_VERSION_NUM})

message(STATUS "curl version=[${CURL_VERSION}]")
set(OPERATING_SYSTEM "${CMAKE_SYSTEM_NAME}")
if(CMAKE_C_COMPILER_TARGET)
  set(OS "\"${CMAKE_C_COMPILER_TARGET}\"")
else()
  set(OS "\"${CMAKE_SYSTEM_NAME}\"")
endif()

include_directories(${CURL_SOURCE_DIR}/include)

set(CURL_WERROR 0)
set(PICKY_COMPILER 0)
set(BUILD_CURL_EXE 0)
set(BUILD_SHARED_LIBS 0)
SET(ENABLE_ARES 0)

if(WIN32)
  SET(CURL_STATIC_CRT 0)
  SET(ENABLE_UNICODE 0)
  set(CURL_TARGET_WINDOWS_VERSION "")
endif()

SET(CURL_LTO 0)
SET(ENABLE_THREADED_RESOLVER 1)
SET(ENABLE_DEBUG 0)
SET(ENABLE_CURLDEBUG 0)

if(NOT DEFINED CMAKE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX "-d")
endif()

set(CURL_LIBS "")

include(CurlSymbolHiding)

SET(CURL_ENABLE_EXPORT_TARGET 0)
SET(CURL_DISABLE_ALTSVC 0)
SET(CURL_DISABLE_COOKIES 0)
SET(CURL_DISABLE_CRYPTO_AUTH 0)
SET(CURL_DISABLE_DICT 1)
SET(CURL_DISABLE_DOH 0)
SET(CURL_DISABLE_FILE 1)
SET(CURL_DISABLE_FTP 1)
SET(CURL_DISABLE_GETOPTIONS 0)
SET(CURL_DISABLE_GOPHER 1)
SET(CURL_DISABLE_HSTS 0)
SET(CURL_DISABLE_HTTP 0)
SET(CURL_DISABLE_HTTP_AUTH 0)
SET(CURL_DISABLE_IMAP 1)
SET(CURL_DISABLE_LDAP 1)
SET(CURL_DISABLE_LDAPS 1)
SET(CURL_DISABLE_LIBCURL_OPTION 0)
SET(CURL_DISABLE_MIME 0)
SET(CURL_DISABLE_MQTT 1)
SET(CURL_DISABLE_NETRC 0)
SET(CURL_DISABLE_NTLM 0)
SET(CURL_DISABLE_PARSEDATE 0)
SET(CURL_DISABLE_POP3 1)
SET(CURL_DISABLE_PROGRESS_METER 0)
SET(CURL_DISABLE_PROXY 0)
SET(CURL_DISABLE_RTSP 1)
SET(CURL_DISABLE_SHUFFLE_DNS 0)
SET(CURL_DISABLE_SMB 1)
SET(CURL_DISABLE_SMTP 1)
SET(CURL_DISABLE_SOCKETPAIR 0)
SET(CURL_DISABLE_TELNET 1)
SET(CURL_DISABLE_TFTP 1)
SET(CURL_DISABLE_VERBOSE_STRINGS 0)
SET(HTTP_ONLY 1)
SET(ENABLE_IPV6 0)
SET(USE_MANUAL 0)

# Disable warnings on Borland to avoid changing 3rd party code.
if(BORLAND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w-")
endif()

# If we are on AIX, do the _ALL_SOURCE magic
if(${CMAKE_SYSTEM_NAME} MATCHES AIX)
  set(_ALL_SOURCE 1)
endif()

# Include all the necessary files for macros
include(CMakePushCheckState)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckTypeSize)
include(CheckCSourceCompiles)

# On windows preload settings
if(WIN32)
  set(CMAKE_REQUIRED_DEFINITIONS "${CMAKE_REQUIRED_DEFINITIONS} -D_WINSOCKAPI_=")
  include(${CMAKE_CURRENT_SOURCE_DIR}/CMake/Platforms/WindowsCache.cmake)
endif()

if(ENABLE_THREADED_RESOLVER)
  find_package(Threads REQUIRED)
  if(WIN32)
    set(USE_THREADS_WIN32 ON)
  else()
    set(USE_THREADS_POSIX ${CMAKE_USE_PTHREADS_INIT})
    set(HAVE_PTHREAD_H ${CMAKE_USE_PTHREADS_INIT})
  endif()
  set(CURL_LIBS ${CURL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif()

# Check for all needed libraries
check_library_exists_concat("socket" connect      HAVE_LIBSOCKET)

check_function_exists(gethostname HAVE_GETHOSTNAME)

if(WIN32)
  check_library_exists_concat("ws2_32" getch        HAVE_LIBWS2_32)
  check_library_exists_concat("winmm"  getch        HAVE_LIBWINMM)
endif()

# This check below for use of deprecated symbols is only temporary and is to
# be removed again after a year's service. Remove after November 25, 2022.
set(CURL_RECONFIG_REQUIRED 0)
foreach(_LIB GSSAPI OPENLDAP LIBSSH LIBSSH2 BEARSSL MBEDTLS NSS OPENSSL
        SCHANNEL SECTRANSP WOLFSSL)
  if(CMAKE_USE_${_LIB})
    set(CURL_RECONFIG_REQUIRED 1)
    message(SEND_ERROR "The option CMAKE_USE_${_LIB} was renamed to CURL_USE_${_LIB}.")
  endif()
endforeach()
if(CMAKE_USE_WINSSL)
  set(CURL_RECONFIG_REQUIRED 1)
  message(SEND_ERROR "The option CMAKE_USE_WINSSL was renamed to CURL_USE_SCHANNEL.")
endif()
if(CURL_RECONFIG_REQUIRED)
  message(FATAL_ERROR "Reconfig required")
endif()

# check SSL libraries
# TODO support GnuTLS
set(CURL_ENABLE_SSL 1N)

if(APPLE)
  SET(CURL_USE_SECTRANSP 0)
endif()
if(WIN32)
  SET(CURL_USE_SCHANNEL 0)
  SET(CURL_WINDOWS_SSPI 0)
endif()

if(use_core_foundation)
  find_library(COREFOUNDATION_FRAMEWORK "CoreFoundation")
  if(NOT COREFOUNDATION_FRAMEWORK)
      message(FATAL_ERROR "CoreFoundation framework not found")
  endif()
  list(APPEND CURL_LIBS "-framework CoreFoundation")
endif()

if(1)
  set(SSL_ENABLED ON)
  set(USE_OPENSSL ON)

  SET(OPENSSL_INCLUDE_DIR   "${CMAKE_SOURCE_DIR}/ExternalLibs/libressl-3.6.1/include")
  SET(CURL_DISABLE_OPENSSL_AUTO_LOAD_CONFIG 1)

  include_directories(${OPENSSL_INCLUDE_DIR})
  set(CMAKE_REQUIRED_INCLUDES ${OPENSSL_INCLUDE_DIR})
  if(NOT DEFINED HAVE_RAND_EGD)
    check_symbol_exists(RAND_egd "${CURL_INCLUDES}" HAVE_RAND_EGD)
  endif()
  if(NOT DEFINED HAVE_BORINGSSL)
    check_symbol_exists(OPENSSL_IS_BORINGSSL "openssl/base.h" HAVE_BORINGSSL)
  endif()
  add_definitions(-DOPENSSL_SUPPRESS_DEPRECATED)
endif()

SET(CURL_USE_MBEDTLS 0)
SET(CURL_USE_BEARSSL 0)
SET(CURL_USE_WOLFSSL 0)
SET(CURL_USE_NSS     0)
SET(USE_NGHTTP2      0)
SET(USE_NGTCP2 0)
set(USE_QUICHE 0)
set(USE_MSH3 0)
SET(USE_LIBIDN2 0)

if(WIN32)
  SET(USE_WIN32_IDN 0)
endif()

set(HAVE_LIBZ OFF)
set(USE_ZLIB OFF)
SET(CURL_BROTLI 0)
set(HAVE_BROTLI 0)
SET(CURL_ZSTD 0)
set(HAVE_ZSTD 0)
set(CURL_USE_LIBPSL 0)
set(USE_LIBPSL 0)
set(CURL_USE_LIBSSH2 0)
set(USE_LIBSSH2 0)
set(CURL_USE_LIBSSH 0)
set(CURL_USE_GSSAPI 0)
set(ENABLE_UNIX_SOCKETS 1)

if(ENABLE_UNIX_SOCKETS)
  include(CheckStructHasMember)
  if(WIN32)
    set(USE_UNIX_SOCKETS ON)
  else()
    check_struct_has_member("struct sockaddr_un" sun_path "sys/un.h" USE_UNIX_SOCKETS)
  endif()
else()
  unset(USE_UNIX_SOCKETS CACHE)
endif()

set(CURL_CA_FALLBACK 0)
unset(CURL_CA_BUNDLE CACHE)
unset(CURL_CA_PATH CACHE)

# Check for header files
if(NOT UNIX)
  check_include_file_concat("windows.h"      HAVE_WINDOWS_H)
  check_include_file_concat("ws2tcpip.h"     HAVE_WS2TCPIP_H)
  check_include_file_concat("winsock2.h"     HAVE_WINSOCK2_H)
  check_include_file_concat("wincrypt.h"     HAVE_WINCRYPT_H)
endif()

check_include_file_concat("inttypes.h"       HAVE_INTTYPES_H)
check_include_file_concat("sys/filio.h"      HAVE_SYS_FILIO_H)
check_include_file_concat("sys/ioctl.h"      HAVE_SYS_IOCTL_H)
check_include_file_concat("sys/param.h"      HAVE_SYS_PARAM_H)
check_include_file_concat("sys/poll.h"       HAVE_SYS_POLL_H)
check_include_file_concat("sys/resource.h"   HAVE_SYS_RESOURCE_H)
check_include_file_concat("sys/select.h"     HAVE_SYS_SELECT_H)
check_include_file_concat("sys/socket.h"     HAVE_SYS_SOCKET_H)
check_include_file_concat("sys/sockio.h"     HAVE_SYS_SOCKIO_H)
check_include_file_concat("sys/stat.h"       HAVE_SYS_STAT_H)
check_include_file_concat("sys/time.h"       HAVE_SYS_TIME_H)
check_include_file_concat("sys/types.h"      HAVE_SYS_TYPES_H)
check_include_file_concat("sys/un.h"         HAVE_SYS_UN_H)
check_include_file_concat("sys/utime.h"      HAVE_SYS_UTIME_H)
check_include_file_concat("sys/xattr.h"      HAVE_SYS_XATTR_H)
check_include_file_concat("arpa/inet.h"      HAVE_ARPA_INET_H)
check_include_file_concat("arpa/tftp.h"      HAVE_ARPA_TFTP_H)
check_include_file_concat("assert.h"         HAVE_ASSERT_H)
check_include_file_concat("errno.h"          HAVE_ERRNO_H)
check_include_file_concat("fcntl.h"          HAVE_FCNTL_H)
check_include_file_concat("idn2.h"           HAVE_IDN2_H)
check_include_file_concat("ifaddrs.h"        HAVE_IFADDRS_H)
check_include_file_concat("io.h"             HAVE_IO_H)
check_include_file_concat("libgen.h"         HAVE_LIBGEN_H)
check_include_file_concat("locale.h"         HAVE_LOCALE_H)
check_include_file_concat("net/if.h"         HAVE_NET_IF_H)
check_include_file_concat("netdb.h"          HAVE_NETDB_H)
check_include_file_concat("netinet/in.h"     HAVE_NETINET_IN_H)
check_include_file_concat("netinet/tcp.h"    HAVE_NETINET_TCP_H)
check_include_file("linux/tcp.h"      HAVE_LINUX_TCP_H)

check_include_file_concat("poll.h"           HAVE_POLL_H)
check_include_file_concat("pwd.h"            HAVE_PWD_H)
check_include_file_concat("setjmp.h"         HAVE_SETJMP_H)
check_include_file_concat("signal.h"         HAVE_SIGNAL_H)
check_include_file_concat("ssl.h"            HAVE_SSL_H)
check_include_file_concat("stdatomic.h"      HAVE_STDATOMIC_H)
check_include_file_concat("stdbool.h"        HAVE_STDBOOL_H)
check_include_file_concat("stdint.h"         HAVE_STDINT_H)
check_include_file_concat("stdlib.h"         HAVE_STDLIB_H)
check_include_file_concat("string.h"         HAVE_STRING_H)
check_include_file_concat("strings.h"        HAVE_STRINGS_H)
check_include_file_concat("stropts.h"        HAVE_STROPTS_H)
check_include_file_concat("termio.h"         HAVE_TERMIO_H)
check_include_file_concat("termios.h"        HAVE_TERMIOS_H)
check_include_file_concat("time.h"           HAVE_TIME_H)
check_include_file_concat("unistd.h"         HAVE_UNISTD_H)
check_include_file_concat("utime.h"          HAVE_UTIME_H)

check_include_file_concat("process.h"        HAVE_PROCESS_H)
check_include_file_concat("stddef.h"         HAVE_STDDEF_H)
check_include_file_concat("stdint.h"        HAVE_STDINT_H)
check_include_file_concat("sys/utsname.h"   HAVE_SYS_UTSNAME_H)

check_type_size(size_t  SIZEOF_SIZE_T)
check_type_size(ssize_t  SIZEOF_SSIZE_T)
check_type_size("long long"  SIZEOF_LONG_LONG)
check_type_size("long"  SIZEOF_LONG)
check_type_size("int"  SIZEOF_INT)
check_type_size("__int64"  SIZEOF___INT64)
check_type_size("time_t"  SIZEOF_TIME_T)
if(NOT HAVE_SIZEOF_SSIZE_T)
  if(SIZEOF_LONG EQUAL SIZEOF_SIZE_T)
    set(ssize_t long)
  endif()
  if(NOT ssize_t AND SIZEOF___INT64 EQUAL SIZEOF_SIZE_T)
    set(ssize_t __int64)
  endif()
endif()
# off_t is sized later, after the HAVE_FILE_OFFSET_BITS test

if(HAVE_SIZEOF_LONG_LONG)
  set(HAVE_LONGLONG 1)
endif()

if(NOT CMAKE_CROSSCOMPILING)
  find_file(RANDOM_FILE urandom /dev)
  mark_as_advanced(RANDOM_FILE)
endif()

# Check for some functions that are used
if(HAVE_LIBWS2_32)
  set(CMAKE_REQUIRED_LIBRARIES ws2_32)
elseif(HAVE_LIBSOCKET)
  set(CMAKE_REQUIRED_LIBRARIES socket)
endif()

check_symbol_exists(fchmod        "${CURL_INCLUDES}" HAVE_FCHMOD)
check_symbol_exists(basename      "${CURL_INCLUDES}" HAVE_BASENAME)
check_symbol_exists(socket        "${CURL_INCLUDES}" HAVE_SOCKET)
check_symbol_exists(socketpair    "${CURL_INCLUDES}" HAVE_SOCKETPAIR)
check_symbol_exists(recv          "${CURL_INCLUDES}" HAVE_RECV)
check_symbol_exists(send          "${CURL_INCLUDES}" HAVE_SEND)
check_symbol_exists(select        "${CURL_INCLUDES}" HAVE_SELECT)
check_symbol_exists(strdup        "${CURL_INCLUDES}" HAVE_STRDUP)
check_symbol_exists(strtok_r      "${CURL_INCLUDES}" HAVE_STRTOK_R)
check_symbol_exists(strcasecmp    "${CURL_INCLUDES}" HAVE_STRCASECMP)
check_symbol_exists(stricmp       "${CURL_INCLUDES}" HAVE_STRICMP)
check_symbol_exists(strcmpi       "${CURL_INCLUDES}" HAVE_STRCMPI)
check_symbol_exists(alarm         "${CURL_INCLUDES}" HAVE_ALARM)
check_symbol_exists(getppid       "${CURL_INCLUDES}" HAVE_GETPPID)
check_symbol_exists(utimes        "${CURL_INCLUDES}" HAVE_UTIMES)

check_symbol_exists(gettimeofday  "${CURL_INCLUDES}" HAVE_GETTIMEOFDAY)
check_symbol_exists(closesocket   "${CURL_INCLUDES}" HAVE_CLOSESOCKET)
check_symbol_exists(sigsetjmp     "${CURL_INCLUDES}" HAVE_SIGSETJMP)
check_symbol_exists(getpass_r     "${CURL_INCLUDES}" HAVE_GETPASS_R)
check_symbol_exists(getpwuid      "${CURL_INCLUDES}" HAVE_GETPWUID)
check_symbol_exists(getpwuid_r    "${CURL_INCLUDES}" HAVE_GETPWUID_R)
check_symbol_exists(geteuid       "${CURL_INCLUDES}" HAVE_GETEUID)
check_symbol_exists(utime         "${CURL_INCLUDES}" HAVE_UTIME)
check_symbol_exists(gmtime_r      "${CURL_INCLUDES}" HAVE_GMTIME_R)

check_symbol_exists(gethostbyname_r "${CURL_INCLUDES}" HAVE_GETHOSTBYNAME_R)

check_symbol_exists(signal         "${CURL_INCLUDES}" HAVE_SIGNAL)
check_symbol_exists(strtoll        "${CURL_INCLUDES}" HAVE_STRTOLL)
check_symbol_exists(_strtoi64      "${CURL_INCLUDES}" HAVE__STRTOI64)
check_symbol_exists(strerror_r     "${CURL_INCLUDES}" HAVE_STRERROR_R)
check_symbol_exists(siginterrupt   "${CURL_INCLUDES}" HAVE_SIGINTERRUPT)
check_symbol_exists(getaddrinfo    "${CURL_INCLUDES}" HAVE_GETADDRINFO)
if(NOT HAVE_GETADDRINFO)
  set(HAVE_GETADDRINFO_THREADSAFE OFF)
endif()
check_symbol_exists(freeaddrinfo   "${CURL_INCLUDES}" HAVE_FREEADDRINFO)
check_symbol_exists(pipe           "${CURL_INCLUDES}" HAVE_PIPE)
check_symbol_exists(ftruncate      "${CURL_INCLUDES}" HAVE_FTRUNCATE)
check_symbol_exists(getpeername    "${CURL_INCLUDES}" HAVE_GETPEERNAME)
check_symbol_exists(getsockname    "${CURL_INCLUDES}" HAVE_GETSOCKNAME)
check_symbol_exists(if_nametoindex "${CURL_INCLUDES}" HAVE_IF_NAMETOINDEX)
check_symbol_exists(getrlimit      "${CURL_INCLUDES}" HAVE_GETRLIMIT)
check_symbol_exists(setlocale      "${CURL_INCLUDES}" HAVE_SETLOCALE)
check_symbol_exists(setmode        "${CURL_INCLUDES}" HAVE_SETMODE)
check_symbol_exists(setrlimit      "${CURL_INCLUDES}" HAVE_SETRLIMIT)

if(NOT MSVC OR (MSVC_VERSION GREATER_EQUAL 1900))
  # earlier MSVC compilers had faulty snprintf implementations
  check_symbol_exists(snprintf       "${CURL_INCLUDES}" HAVE_SNPRINTF)
endif()
check_function_exists(mach_absolute_time HAVE_MACH_ABSOLUTE_TIME)
check_symbol_exists(inet_ntop      "${CURL_INCLUDES}" HAVE_INET_NTOP)
if(MSVC AND (MSVC_VERSION LESS_EQUAL 1600))
  set(HAVE_INET_NTOP OFF)
endif()
check_symbol_exists(inet_pton      "${CURL_INCLUDES}" HAVE_INET_PTON)

check_symbol_exists(fsetxattr "${CURL_INCLUDES}" HAVE_FSETXATTR)
if(HAVE_FSETXATTR)
  foreach(CURL_TEST HAVE_FSETXATTR_5 HAVE_FSETXATTR_6)
    curl_internal_test(${CURL_TEST})
  endforeach()
endif()

set(CMAKE_EXTRA_INCLUDE_FILES   "sys/socket.h")
check_type_size("sa_family_t"   SIZEOF_SA_FAMILY_T)
set(HAVE_SA_FAMILY_T            ${HAVE_SIZEOF_SA_FAMILY_T})
set(CMAKE_EXTRA_INCLUDE_FILES   "")

set(CMAKE_EXTRA_INCLUDE_FILES   "ws2def.h")
check_type_size("ADDRESS_FAMILY"    SIZEOF_ADDRESS_FAMILY)
set(HAVE_ADDRESS_FAMILY         ${HAVE_SIZEOF_ADDRESS_FAMILY})
set(CMAKE_EXTRA_INCLUDE_FILES   "")

# sigaction and sigsetjmp are special. Use special mechanism for
# detecting those, but only if previous attempt failed.
if(HAVE_SIGNAL_H)
  check_symbol_exists(sigaction "signal.h" HAVE_SIGACTION)
endif()

if(NOT HAVE_SIGSETJMP)
  if(HAVE_SETJMP_H)
    check_symbol_exists(sigsetjmp "setjmp.h" HAVE_MACRO_SIGSETJMP)
    if(HAVE_MACRO_SIGSETJMP)
      set(HAVE_SIGSETJMP 1)
    endif()
  endif()
endif()

# If there is no stricmp(), do not allow LDAP to parse URLs
if(NOT HAVE_STRICMP)
  set(HAVE_LDAP_URL_PARSE 1)
endif()

# Do curl specific tests
foreach(CURL_TEST
    HAVE_FCNTL_O_NONBLOCK
    HAVE_IOCTLSOCKET
    HAVE_IOCTLSOCKET_CAMEL
    HAVE_IOCTLSOCKET_CAMEL_FIONBIO
    HAVE_IOCTLSOCKET_FIONBIO
    HAVE_IOCTL_FIONBIO
    HAVE_IOCTL_SIOCGIFADDR
    HAVE_SETSOCKOPT_SO_NONBLOCK
    TIME_WITH_SYS_TIME
    HAVE_O_NONBLOCK
    HAVE_GETHOSTBYNAME_R_3
    HAVE_GETHOSTBYNAME_R_5
    HAVE_GETHOSTBYNAME_R_6
    HAVE_GETHOSTBYNAME_R_3_REENTRANT
    HAVE_GETHOSTBYNAME_R_5_REENTRANT
    HAVE_GETHOSTBYNAME_R_6_REENTRANT
    HAVE_IN_ADDR_T
    HAVE_BOOL_T
    STDC_HEADERS
    HAVE_FILE_OFFSET_BITS
    HAVE_VARIADIC_MACROS_C99
    HAVE_VARIADIC_MACROS_GCC
    HAVE_ATOMIC
    )
  curl_internal_test(${CURL_TEST})
endforeach()

if(HAVE_FILE_OFFSET_BITS)
  set(_FILE_OFFSET_BITS 64)
  set(CMAKE_REQUIRED_FLAGS "-D_FILE_OFFSET_BITS=64")
endif()
check_type_size("off_t"  SIZEOF_OFF_T)

# include this header to get the type
set(CMAKE_REQUIRED_INCLUDES "${CURL_SOURCE_DIR}/include")
set(CMAKE_EXTRA_INCLUDE_FILES "curl/system.h")
check_type_size("curl_off_t"  SIZEOF_CURL_OFF_T)
set(CMAKE_EXTRA_INCLUDE_FILES "")

if(WIN32)
  # detect actual value of _WIN32_WINNT and store as HAVE_WIN32_WINNT
  curl_internal_test(HAVE_WIN32_WINNT)
  if(HAVE_WIN32_WINNT)
    string(REGEX MATCH ".*_WIN32_WINNT=0x[0-9a-fA-F]+" OUTPUT "${OUTPUT}")
    string(REGEX REPLACE ".*_WIN32_WINNT=" "" HAVE_WIN32_WINNT "${OUTPUT}")
    message(STATUS "Found _WIN32_WINNT=${HAVE_WIN32_WINNT}")
  endif()
  # avoid storing HAVE_WIN32_WINNT in CMake cache
  unset(HAVE_WIN32_WINNT CACHE)
endif()

set(CMAKE_REQUIRED_FLAGS)

set(ENABLE_WEBSOCKETS 0)

foreach(CURL_TEST
    HAVE_GLIBC_STRERROR_R
    HAVE_POSIX_STRERROR_R
    )
  curl_internal_test(${CURL_TEST})
endforeach()

# Check for reentrant
foreach(CURL_TEST
    HAVE_GETHOSTBYNAME_R_3
    HAVE_GETHOSTBYNAME_R_5
    HAVE_GETHOSTBYNAME_R_6)
  if(NOT ${CURL_TEST})
    if(${CURL_TEST}_REENTRANT)
      set(NEED_REENTRANT 1)
    endif()
  endif()
endforeach()

if(NEED_REENTRANT)
  foreach(CURL_TEST
      HAVE_GETHOSTBYNAME_R_3
      HAVE_GETHOSTBYNAME_R_5
      HAVE_GETHOSTBYNAME_R_6)
    set(${CURL_TEST} 0)
    if(${CURL_TEST}_REENTRANT)
      set(${CURL_TEST} 1)
    endif()
  endforeach()
endif()

# Check clock_gettime(CLOCK_MONOTONIC, x) support
curl_internal_test(HAVE_CLOCK_GETTIME_MONOTONIC)

# Check compiler support of __builtin_available()
curl_internal_test(HAVE_BUILTIN_AVAILABLE)

# Some other minor tests

if(NOT HAVE_IN_ADDR_T)
  set(in_addr_t "unsigned long")
endif()

# Check for nonblocking
set(HAVE_DISABLED_NONBLOCKING 1)
if(HAVE_FIONBIO OR
    HAVE_IOCTLSOCKET OR
    HAVE_IOCTLSOCKET_CASE OR
    HAVE_O_NONBLOCK)
  set(HAVE_DISABLED_NONBLOCKING)
endif()

if(CMAKE_COMPILER_IS_GNUCC AND APPLE)
  include(CheckCCompilerFlag)
  check_c_compiler_flag(-Wno-long-double HAVE_C_FLAG_Wno_long_double)
  if(HAVE_C_FLAG_Wno_long_double)
    # The Mac version of GCC warns about use of long double.  Disable it.
    get_source_file_property(MPRINTF_COMPILE_FLAGS mprintf.c COMPILE_FLAGS)
    if(MPRINTF_COMPILE_FLAGS)
      set(MPRINTF_COMPILE_FLAGS "${MPRINTF_COMPILE_FLAGS} -Wno-long-double")
    else()
      set(MPRINTF_COMPILE_FLAGS "-Wno-long-double")
    endif()
    set_source_files_properties(mprintf.c PROPERTIES
      COMPILE_FLAGS ${MPRINTF_COMPILE_FLAGS})
  endif()
endif()

# TODO test which of these headers are required
if(WIN32)
  set(CURL_PULL_WS2TCPIP_H ${HAVE_WS2TCPIP_H})
else()
  set(CURL_PULL_SYS_TYPES_H ${HAVE_SYS_TYPES_H})
  set(CURL_PULL_SYS_SOCKET_H ${HAVE_SYS_SOCKET_H})
  set(CURL_PULL_SYS_POLL_H ${HAVE_SYS_POLL_H})
endif()
set(CURL_PULL_STDINT_H ${HAVE_STDINT_H})
set(CURL_PULL_INTTYPES_H ${HAVE_INTTYPES_H})

include(CMake/OtherTests.cmake)

add_definitions(-DHAVE_CONFIG_H)

# For Windows, all compilers used by CMake should support large files
if(WIN32)
  set(USE_WIN32_LARGE_FILES ON)

  # Use the manifest embedded in the Windows Resource
  set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} -DCURL_EMBED_MANIFEST")

  # Check if crypto functions in wincrypt.h are actually available
  if(HAVE_WINCRYPT_H)
    check_symbol_exists(CryptAcquireContext "${CURL_INCLUDES}" USE_WINCRYPT)
  endif()
  if(USE_WINCRYPT)
    set(USE_WIN32_CRYPTO ON)
  endif()

  # Link required libraries for USE_WIN32_CRYPTO or USE_SCHANNEL
  if(USE_WIN32_CRYPTO OR USE_SCHANNEL)
    list(APPEND CURL_LIBS "advapi32" "crypt32")
  endif()

  # Matching logic used for Curl_win32_random()
  if(MINGW)
    check_c_source_compiles("
      #include <_mingw.h>
      #if defined(__MINGW64_VERSION_MAJOR)
      #error
      #endif
      int main(void) {
        return 0;
      }"
      HAVE_MINGW_ORIGINAL)
  endif()

  if(NOT HAVE_MINGW_ORIGINAL)
    list(APPEND CURL_LIBS "bcrypt")
  else()
    set(HAVE_FTRUNCATE OFF)
  endif()
endif()

if(MSVC)
  # Disable default manifest added by CMake
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")

  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
  if(CMAKE_C_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
  endif()

  # Use multithreaded compilation on VS 2008+
  if(MSVC_VERSION GREATER_EQUAL 1500)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
  endif()
endif()


# Ugly (but functional) way to include "Makefile.inc" by transforming it (= regenerate it).
function(transform_makefile_inc INPUT_FILE OUTPUT_FILE)
  file(READ ${INPUT_FILE} MAKEFILE_INC_TEXT)
  string(REPLACE "$(top_srcdir)"   "\${CURL_SOURCE_DIR}" MAKEFILE_INC_TEXT ${MAKEFILE_INC_TEXT})
  string(REPLACE "$(top_builddir)" "\${CURL_BINARY_DIR}" MAKEFILE_INC_TEXT ${MAKEFILE_INC_TEXT})

  string(REGEX REPLACE "\\\\\n" "!π!α!" MAKEFILE_INC_TEXT ${MAKEFILE_INC_TEXT})
  string(REGEX REPLACE "([a-zA-Z_][a-zA-Z0-9_]*)[\t ]*=[\t ]*([^\n]*)" "SET(\\1 \\2)" MAKEFILE_INC_TEXT ${MAKEFILE_INC_TEXT})
  string(REPLACE "!π!α!" "\n" MAKEFILE_INC_TEXT ${MAKEFILE_INC_TEXT})

  string(REGEX REPLACE "\\$\\(([a-zA-Z_][a-zA-Z0-9_]*)\\)" "\${\\1}" MAKEFILE_INC_TEXT ${MAKEFILE_INC_TEXT})    # Replace $() with ${}
  string(REGEX REPLACE "@([a-zA-Z_][a-zA-Z0-9_]*)@" "\${\\1}" MAKEFILE_INC_TEXT ${MAKEFILE_INC_TEXT})    # Replace @@ with ${}, even if that may not be read by CMake scripts.
  file(WRITE ${OUTPUT_FILE} ${MAKEFILE_INC_TEXT})
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${INPUT_FILE}")
endfunction()

include(GNUInstallDirs)

set(CURL_INSTALL_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
add_subdirectory(lib)

set_target_properties(libcurl PROPERTIES FOLDER "ExternalLibs/VISUS_NET")