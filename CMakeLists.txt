cmake_minimum_required(VERSION 3.16)
project(lc-ns-kmp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(OpenMP REQUIRED)
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(jsoncpp REQUIRED)
find_package(piqp REQUIRED)

add_library(GMR STATIC src/gmr.cpp)
target_include_directories(GMR
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(GMR
  PUBLIC
    Eigen3::Eigen
    JsonCpp::JsonCpp
    OpenMP::OpenMP_CXX
)

add_library(KMP STATIC src/lc-ns-kmp.cpp)
target_include_directories(KMP
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(KMP
  PUBLIC
    Eigen3::Eigen
    piqp::piqp
    JsonCpp::JsonCpp
    OpenMP::OpenMP_CXX
)

add_executable(inference examples/inference.cpp)
target_link_libraries(inference PRIVATE GMR KMP)

# inference_extended.cpp and inference_dyn.cpp are kept for future work on
# separate branches; they are not built by default on main.
option(LC_NS_KMP_BUILD_WIP_EXAMPLES "Build WIP examples (inference_extended)" OFF)
if(LC_NS_KMP_BUILD_WIP_EXAMPLES)
  add_executable(inference_extended examples/inference_extended.cpp)
  target_link_libraries(inference_extended PRIVATE GMR KMP)
endif()

install(TARGETS GMR KMP inference
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)
