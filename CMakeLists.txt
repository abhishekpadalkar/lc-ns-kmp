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
set_target_properties(gmr PROPERTIES POSITION_INDEPENDENT_CODE ON)
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
set_target_properties(lc-ns-kmp PROPERTIES POSITION_INDEPENDENT_CODE ON)
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

# ---------------------------------------------------------------------------
# Python bindings (pybind11). Enabled by -DLC_NS_KMP_BUILD_PYTHON=ON or when
# building via scikit-build / pip.
# ---------------------------------------------------------------------------
option(LC_NS_KMP_BUILD_PYTHON "Build Python bindings" OFF)
if(DEFINED SKBUILD)
  set(LC_NS_KMP_BUILD_PYTHON ON)
endif()

if(LC_NS_KMP_BUILD_PYTHON)
  set(PYBIND11_FINDPYTHON ON)
  find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)
  find_package(pybind11 CONFIG QUIET)
  if(NOT pybind11_FOUND)
    include(FetchContent)
    FetchContent_Declare(
      pybind11
      GIT_REPOSITORY https://github.com/pybind/pybind11.git
      GIT_TAG v2.13.6
    )
    FetchContent_MakeAvailable(pybind11)
  endif()

  pybind11_add_module(_core MODULE python/bindings/module.cpp)
  target_link_libraries(_core PRIVATE gmr lc-ns-kmp)
  target_compile_definitions(_core PRIVATE VERSION_INFO="${PROJECT_VERSION}")

  # Embed piqp's library directory so libpiqp.so resolves at import time.
  set(_piqp_loc "")
  foreach(_cfg RELEASE RELWITHDEBINFO MINSIZEREL DEBUG)
    get_target_property(_cand piqp::piqp IMPORTED_LOCATION_${_cfg})
    if(_cand)
      set(_piqp_loc "${_cand}")
      break()
    endif()
  endforeach()
  if(NOT _piqp_loc)
    get_target_property(_piqp_loc piqp::piqp IMPORTED_LOCATION)
  endif()
  if(_piqp_loc)
    get_filename_component(_piqp_libdir "${_piqp_loc}" DIRECTORY)
    set_target_properties(_core PROPERTIES
      BUILD_RPATH "${_piqp_libdir}"
      INSTALL_RPATH "${_piqp_libdir}"
    )
  endif()

  if(SKBUILD)
    install(TARGETS _core DESTINATION lc_ns_kmp)
  else()
    # Local/dev: drop the extension next to the pure-Python package.
    set(_lc_ns_kmp_py_dir "${CMAKE_CURRENT_SOURCE_DIR}/python/lc_ns_kmp")
    set_target_properties(_core PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY "${_lc_ns_kmp_py_dir}"
      RUNTIME_OUTPUT_DIRECTORY "${_lc_ns_kmp_py_dir}"
    )
  endif()
endif()
