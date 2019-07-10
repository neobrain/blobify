# Helper for finding Boost.Precise and Flat Reflection (also known as magic_get)

find_path(BOOST_PFR_INCLUDE_DIR boost/pfr.hpp HINTS "${CMAKE_CURRENT_SOURCE_DIR}/magic_get/include/")
mark_as_advanced(BOOST_PFR_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PFR
    REQUIRED_VARS BOOST_PFR_INCLUDE_DIR)

if(PFR_FOUND AND NOT TARGET pfr::pfr)
    add_library(pfr::pfr INTERFACE IMPORTED)
    set_target_properties(pfr::pfr PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${BOOST_PFR_INCLUDE_DIR}")

    target_compile_features(pfr::pfr INTERFACE cxx_std_14)
endif()
