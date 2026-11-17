cmake_minimum_required(VERSION 3.16)
project(creditcurve LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_library(creditcurve STATIC
  src/discount_curve.cpp
  src/hazard_curve.cpp
  src/root_finder.cpp
  src/cds_pricer.cpp
  src/bootstrap.cpp
  src/risk.cpp
)
target_include_directories(creditcurve PUBLIC include)

add_executable(bootstrap_cli apps/main.cpp)
target_link_libraries(bootstrap_cli PRIVATE creditcurve)

enable_testing()
add_executable(run_tests
  tests/main.cpp
  tests/test_discount_curve.cpp
  tests/test_hazard_curve.cpp
  tests/test_root_finder.cpp
  tests/test_cds_pricer.cpp
  tests/test_bootstrap.cpp
  tests/test_risk.cpp
  tests/test_cross_language.cpp
)
target_link_libraries(run_tests PRIVATE creditcurve)
target_include_directories(run_tests PRIVATE tests)
add_test(NAME unit_tests COMMAND run_tests)
