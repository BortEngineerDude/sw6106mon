find_library(
    libgpiodcxx_LIBRARY
    NAMES libgpiodcxx.so
    HINTS "${CMAKE_PREFIX_PATH}"
)

find_path(
  libgpiodcxx_INCLUDE_DIR
  NAMES gpiod.hpp
  HINTS "${CMAKE_PREFIX_PATH}"
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(libgpiodcxx DEFAULT_MSG
                                  libgpiodcxx_LIBRARY
                                  libgpiodcxx_INCLUDE_DIR)

mark_as_advanced(libgpiodcxx_LIBRARY libgpiodcxx_INCLUDE_DIR)

if(libgpiodcxx_FOUND AND NOT TARGET libgpiodcxx::libgpiodcxx)
  add_library(libgpiodcxx::libgpiodcxx SHARED IMPORTED)
  set_target_properties(
    libgpiodcxx::libgpiodcxx
    PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${libgpiodcxx_INCLUDE_DIR}"
      IMPORTED_LOCATION ${libgpiodcxx_LIBRARY})
endif()
