DisableAllWarnings()

add_definitions(
  -DPURIFY
  -D_REENTRANT
  -DOPENSSL_NO_ASM
)

if (WIN32)
  add_definitions(-DPSAPI_VERSION=1)
  add_definitions(-DFD_SETSIZE=4096)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS )
  add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

add_subdirectory(crypto)
add_subdirectory(ssl)
