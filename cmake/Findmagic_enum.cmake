# Helper for finding magic_enum

find_path(MAGIC_ENUM_INCLUDE_DIR magic_enum.hpp HINTS "${CMAKE_CURRENT_SOURCE_DIR}/magic_enum/include/")
mark_as_advanced(MAGIC_ENUM_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MagicEnum
    REQUIRED_VARS MAGIC_ENUM_INCLUDE_DIR)

if(PFR_FOUND AND NOT TARGET magic_enum::magic_enum)
    add_library(magic_enum::magic_enum INTERFACE IMPORTED)
    set_target_properties(magic_enum::magic_enum PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MAGIC_ENUM_INCLUDE_DIR}")

    target_compile_features(magic_enum::magic_enum INTERFACE cxx_std_14)
endif()
