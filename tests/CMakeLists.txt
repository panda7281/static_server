cmake_minimum_required(VERSION 3.12)
project(StaticServer_utests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find Catch2
find_package(Catch2 3 QUIET)
if(Catch2_FOUND)
    message(STATUS "Packaged version of Catch will be used.")
else()
    message(STATUS "Bundled version of Catch will be downloaded and used.")
    include(FetchContent)
    FetchContent_Declare(Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG 53d0d913a422d356b23dd927547febdf69ee9081 # v3.5.0
    )
    FetchContent_MakeAvailable(Catch2)
endif()

include_directories(../include)

enable_testing()

add_executable(StaticServer_utests
    test_filecachepool.cpp
    test_hashedwheeltimer.cpp
    test_httpheaderparser.cpp
    test_threadpool.cpp
    ../src/filecachepool.cpp
    ../src/hashedwheeltimer.cpp
    ../src/httpheaderparser.cpp
    ../src/threadpool.cpp
    )

add_executable(StaticServer_stests
    test_system.cpp
    ../src/staticserver.cpp
    ../src/httpheaderparser.cpp 
    ../src/threadpool.cpp 
    ../src/hashedwheeltimer.cpp
    ../src/httpclienttask.cpp
    ../src/filecachepool.cpp
    ../src/utils.cpp
    )

target_link_libraries(StaticServer_utests PRIVATE Catch2::Catch2WithMain)
target_link_libraries(StaticServer_stests PRIVATE Catch2::Catch2WithMain)
add_test(NAME StaticServer_utests COMMAND StaticServer_utests)
add_test(NAME StaticServer_stests COMMAND StaticServer_stests)
set_tests_properties(StaticServer_utests PROPERTIES RUN_SERIAL ON)
set_tests_properties(StaticServer_stests PROPERTIES RUN_SERIAL ON)