DisableAllWarnings()

add_definitions(
  -DPURIFY
  -D_REENTRANT
  -DOPENSSL_NO_ASM
)

add_subdirectory(crypto)
add_subdirectory(ssl)
