cmake_minimum_required(VERSION 3.26)
project(maestro)

set(CMAKE_CXX_STANDARD 14)

# Find Boost
find_package(Boost 1.82.0 REQUIRED COMPONENTS program_options filesystem system)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Boost not found. Please install Boost.")
endif()

include_directories(cost-model/include)
include_directories(cost-model/include/abstract-hardware-model)
include_directories(cost-model/include/base)
include_directories(cost-model/include/cost-analysis)
include_directories(cost-model/include/dataflow-analysis)
include_directories(cost-model/include/dataflow-specification-language)
include_directories(cost-model/include/design-space-exploration)
include_directories(cost-model/include/tools)
include_directories(cost-model/include/user-api)
link_directories(/opt/homebrew/lib)

add_executable(maestro
        cost-model/src/BASE_base-objects.cpp
        maestro-top.cpp)

target_link_libraries(maestro
        Boost::program_options
        Boost::filesystem
        Boost::system
)
