cmake_minimum_required(VERSION 3.16)
project(lc-ns-kmp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(OpenMP REQUIRED)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(jsoncpp REQUIRED)
find_package(piqp REQUIRED)

add_library(gmr STATIC src/gmr.cpp)
add_library(lc-ns-kmp::gmr ALIAS gmr)
target_include_directories(gmr
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(gmr
  PUBLIC
    Eigen3::Eigen
    JsonCpp::JsonCpp
    OpenMP::OpenMP_CXX
)

add_library(lc-ns-kmp STATIC src/lc-ns-kmp.cpp)
add_library(lc-ns-kmp::lc-ns-kmp ALIAS lc-ns-kmp)
target_include_directories(lc-ns-kmp
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(lc-ns-kmp
  PUBLIC
    Eigen3::Eigen
    piqp::piqp
    JsonCpp::JsonCpp
    OpenMP::OpenMP_CXX
)

add_executable(inference examples/inference.cpp)
target_link_libraries(inference PRIVATE gmr lc-ns-kmp)

# inference_extended.cpp and inference_dyn.cpp are kept for future work on
# separate branches; they are not built by default on main.
option(LC_NS_KMP_BUILD_WIP_EXAMPLES "Build WIP examples (inference_extended)" OFF)
if(LC_NS_KMP_BUILD_WIP_EXAMPLES)
  add_executable(inference_extended examples/inference_extended.cpp)
  target_link_libraries(inference_extended PRIVATE gmr lc-ns-kmp)
endif()

option(LC_NS_KMP_INSTALL_EXAMPLES "Install example binaries" OFF)

set(_lc_ns_kmp_install_targets gmr lc-ns-kmp)
if(LC_NS_KMP_INSTALL_EXAMPLES)
  list(APPEND _lc_ns_kmp_install_targets inference)
  if(LC_NS_KMP_BUILD_WIP_EXAMPLES)
    list(APPEND _lc_ns_kmp_install_targets inference_extended)
  endif()
endif()

install(TARGETS ${_lc_ns_kmp_install_targets}
  EXPORT lc-ns-kmpTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT lc-ns-kmpTargets
  FILE lc-ns-kmpTargets.cmake
  NAMESPACE lc-ns-kmp::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lc-ns-kmp
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lc-ns-kmpConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/lc-ns-kmpConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lc-ns-kmp
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/lc-ns-kmpConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/lc-ns-kmpConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/lc-ns-kmpConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lc-ns-kmp
)
