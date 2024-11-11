cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(sw6106mon LANGUAGES CXX)

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)

add_executable(${PROJECT_NAME} main.cpp)

cmake_policy(SET CMP0076 NEW)

target_sources(${PROJECT_NAME} PRIVATE
  byte_util.h
  i2c.h i2c.cpp
  sw6106.h sw6106.cpp
  config.h config.cpp
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(libgpiodcxx REQUIRED)

target_link_libraries(${PROJECT_NAME}
                      PUBLIC libgpiodcxx::libgpiodcxx
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BIN})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/extra/sw6106mon.conf DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/extra/sw6106mon.service DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/system/)

include(package)

if(NOT SW6106_DEFAULT_CONFIG_PATH)
  set(SW6106_DEFAULT_CONFIG_PATH "\"/${CMAKE_INSTALL_SYSCONFDIR}/sw6106mon.conf\"")
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE SW6106_DEFAULT_CONFIG_PATH=${SW6106_DEFAULT_CONFIG_PATH})
