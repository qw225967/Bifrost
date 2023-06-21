#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libuv::uv_a" for configuration ""
set_property(TARGET libuv::uv_a APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(libuv::uv_a PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "C"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libuv.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS libuv::uv_a )
list(APPEND _IMPORT_CHECK_FILES_FOR_libuv::uv_a "${_IMPORT_PREFIX}/lib/libuv.a" )

# Import target "libuv::uv" for configuration ""
set_property(TARGET libuv::uv APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(libuv::uv PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libuv.1.0.0.dylib"
  IMPORTED_SONAME_NOCONFIG "@rpath/libuv.1.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS libuv::uv )
list(APPEND _IMPORT_CHECK_FILES_FOR_libuv::uv "${_IMPORT_PREFIX}/lib/libuv.1.0.0.dylib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
