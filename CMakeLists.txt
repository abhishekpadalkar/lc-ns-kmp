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
include(FetchContent)

option(LC_NS_KMP_BUILD_PYTHON "Build Python bindings" OFF)
option(LC_NS_KMP_FETCH_DEPS
  "Fetch Eigen/jsoncpp/PIQP via FetchContent when not found (used for wheels)"
  OFF)
if(DEFINED SKBUILD)
  set(LC_NS_KMP_BUILD_PYTHON ON)
  set(LC_NS_KMP_FETCH_DEPS ON)
endif()

find_package(OpenMP REQUIRED)

# --- Eigen ---
find_package(Eigen3 3.3 QUIET NO_MODULE)
if(NOT Eigen3_FOUND)
  if(NOT LC_NS_KMP_FETCH_DEPS)
    message(FATAL_ERROR "Eigen3 not found. Install it or enable LC_NS_KMP_FETCH_DEPS.")
  endif()
  message(STATUS "Fetching Eigen3...")
  FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3.4.0
    GIT_SHALLOW TRUE
  )
  set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(eigen)
endif()

# --- jsoncpp ---
find_package(jsoncpp QUIET)
if(NOT jsoncpp_FOUND AND NOT TARGET JsonCpp::JsonCpp)
  if(NOT LC_NS_KMP_FETCH_DEPS)
    message(FATAL_ERROR "jsoncpp not found. Install it or enable LC_NS_KMP_FETCH_DEPS.")
  endif()
  message(STATUS "Fetching jsoncpp...")
  set(_lc_ns_kmp_saved_shared "${BUILD_SHARED_LIBS}")
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    jsoncpp
    GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
    GIT_TAG 1.9.6
    GIT_SHALLOW TRUE
  )
  set(JSONCPP_WITH_TESTS OFF CACHE BOOL "" FORCE)
  set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(jsoncpp)
  if(TARGET jsoncpp_static)
    set_target_properties(jsoncpp_static PROPERTIES POSITION_INDEPENDENT_CODE ON)
  endif()
  if(NOT TARGET JsonCpp::JsonCpp AND TARGET jsoncpp_static)
    add_library(JsonCpp::JsonCpp ALIAS jsoncpp_static)
  elseif(NOT TARGET JsonCpp::JsonCpp AND TARGET jsoncpp_lib)
    add_library(JsonCpp::JsonCpp ALIAS jsoncpp_lib)
  endif()
  set(BUILD_SHARED_LIBS "${_lc_ns_kmp_saved_shared}" CACHE BOOL "" FORCE)
endif()

# --- PIQP (prefer static when fetching for wheels) ---
find_package(piqp QUIET)
if(NOT piqp_FOUND AND NOT TARGET piqp::piqp)
  if(NOT LC_NS_KMP_FETCH_DEPS)
    message(FATAL_ERROR "piqp not found. Install it or enable LC_NS_KMP_FETCH_DEPS.")
  endif()
  message(STATUS "Fetching PIQP (static)...")
  set(_lc_ns_kmp_saved_shared "${BUILD_SHARED_LIBS}")
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_C_INTERFACE OFF CACHE BOOL "" FORCE)
  set(BUILD_PYTHON_INTERFACE OFF CACHE BOOL "" FORCE)
  set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    piqp
    GIT_REPOSITORY https://github.com/PREDICT-EPFL/piqp.git
    GIT_TAG v0.6.3
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(piqp)
  if(TARGET piqp)
    set_target_properties(piqp PROPERTIES POSITION_INDEPENDENT_CODE ON)
  endif()
  set(BUILD_SHARED_LIBS "${_lc_ns_kmp_saved_shared}" CACHE BOOL "" FORCE)
endif()

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

if(NOT SKBUILD)
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
endif()

# ---------------------------------------------------------------------------
# Python bindings (pybind11)
# ---------------------------------------------------------------------------
if(LC_NS_KMP_BUILD_PYTHON)
  set(PYBIND11_FINDPYTHON ON)
  find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)
  find_package(pybind11 CONFIG QUIET)
  if(NOT pybind11_FOUND)
    FetchContent_Declare(
      pybind11
      GIT_REPOSITORY https://github.com/pybind/pybind11.git
      GIT_TAG v2.13.6
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(pybind11)
  endif()

  pybind11_add_module(_core MODULE python/bindings/module.cpp)
  target_link_libraries(_core PRIVATE gmr lc-ns-kmp)
  target_compile_definitions(_core PRIVATE VERSION_INFO="${PROJECT_VERSION}")

  # Shared PIQP only: embed its lib dir. Static-fetched PIQP needs no RPATH.
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
    set(_lc_ns_kmp_py_dir "${CMAKE_CURRENT_SOURCE_DIR}/python/lc_ns_kmp")
    set_target_properties(_core PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY "${_lc_ns_kmp_py_dir}"
      RUNTIME_OUTPUT_DIRECTORY "${_lc_ns_kmp_py_dir}"
    )
  endif()
endif()

# ---------------------------------------------------------------------------
# Documentation (Sphinx; optional Doxygen XML for Breathe)
# ---------------------------------------------------------------------------
option(LC_NS_KMP_BUILD_DOCS "Add a 'docs' target (Sphinx)" OFF)
if(LC_NS_KMP_BUILD_DOCS)
  find_program(SPHINX_EXECUTABLE NAMES sphinx-build)
  find_program(DOXYGEN_EXECUTABLE NAMES doxygen)
  if(NOT SPHINX_EXECUTABLE)
    message(FATAL_ERROR "LC_NS_KMP_BUILD_DOCS=ON but sphinx-build was not found")
  endif()
  if(DOXYGEN_EXECUTABLE)
    add_custom_target(docs
      COMMAND ${CMAKE_COMMAND} -E echo "Running Doxygen"
      COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Doxyfile
      COMMAND ${SPHINX_EXECUTABLE} -b html
              ${CMAKE_SOURCE_DIR}/docs
              ${CMAKE_SOURCE_DIR}/docs/_build/html
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Building Sphinx HTML documentation"
      VERBATIM
    )
  else()
    add_custom_target(docs
      COMMAND ${SPHINX_EXECUTABLE} -b html
              ${CMAKE_SOURCE_DIR}/docs
              ${CMAKE_SOURCE_DIR}/docs/_build/html
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Building Sphinx HTML documentation (no Doxygen)"
      VERBATIM
    )
  endif()
endif()
