find_path(GMIC_INCLUDE_DIR
  NAMES gmic.h
  DOC "gmic include directory")
mark_as_advanced(GMIC_INCLUDE_DIR)
find_library(GMIC_LIBRARY
  NAMES gmic libgmic
  DOC "gmic library")
mark_as_advanced(GMIC_LIBRARY)

if (GMIC_INCLUDE_DIR)
  file(STRINGS "${GMIC_INCLUDE_DIR}/gmic.h" _gmic_version_lines
    REGEX "#define[ \t]+gmic_version")
  string(REGEX REPLACE ".*gmic_version *\([0-9]*\).*" "\\1" _gmic_version "${_gmic_version_lines}")
  set(GMIC_VERSION "${_gmic_version}")
  unset(_gmic_version)
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GMIC
  REQUIRED_VARS GMIC_LIBRARY GMIC_INCLUDE_DIR
  VERSION_VAR GMIC_VERSION)

if (GMIC_FOUND)
  set(GMIC_INCLUDE_DIRS "${GMIC_INCLUDE_DIR}")
  set(GMIC_LIBRARIES "${GMIC_LIBRARY}")

  if (NOT TARGET GMIC::GMIC)
    add_library(GMIC::GMIC UNKNOWN IMPORTED)
    set_target_properties(GMIC::GMIC PROPERTIES
      IMPORTED_LOCATION "${GMIC_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${GMIC_INCLUDE_DIR}")
  endif ()
endif ()
