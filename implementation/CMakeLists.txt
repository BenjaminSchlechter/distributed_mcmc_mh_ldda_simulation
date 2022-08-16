cmake_minimum_required(VERSION 3.4.3)

project(PROBSAT)

set(BITSERY_DIRECTORY "./lib/bitsery/include/")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_BUILD_TYPE "Release")

SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g -Wall -Wextra -D DEBUG_ASSERTIONS=1")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -Wextra -funroll-loops -fexpensive-optimizations")

find_package(MPI REQUIRED)
# find_package(Threads REQUIRED)

include_directories(${MPI_CXX_INCLUDE_PATH})
link_directories(${MPI_CXX_LINK_FLAG})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

# find src |grep .cpp |parallel -I{} echo '\"{}\"'
add_executable(main
	"src/util/shared_tmpfile.cpp"
	"src/util/util.cpp"
	"src/util/mpi/mpi_async_communication.cpp"
	"src/util/mpi/mpi_util.cpp"
	"src/util/mpi/mpi_shared_tmp_workgroup.cpp"
	"src/metropolis_hastings/value_computation/utility/resolvents_manager.cpp"
	"src/metropolis_hastings/value_computation/utility/probsat-execution.cpp"
	"src/config.cpp"
	"src/main.cpp"
)

target_include_directories(main PRIVATE ${BITSERY_DIRECTORY})
target_link_libraries(main ${MPI_LIBRARIES}) # ${CMAKE_THREAD_LIBS_INIT}

