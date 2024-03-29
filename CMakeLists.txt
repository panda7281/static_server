cmake_minimum_required(VERSION 3.12)

project(StaticServer VERSION 1.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# create a option for unit test
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    message(STATUS "Generating tests")
    enable_testing()
    add_subdirectory(tests)
endif()

include(FetchContent)
find_package(spdlog QUIET)
if(spdlog_FOUND)
    message(STATUS "Packaged version of spdlog will be used.")
else()
    message(STATUS "Bundled version of spdlog will be downloaded and used.")
    FetchContent_Declare(spdlog
            GIT_REPOSITORY https://github.com/gabime/spdlog.git
            GIT_TAG v1.x
    )
    FetchContent_MakeAvailable(spdlog)
endif()

find_package(nlohmann_json QUIET)
if(nlohmann_json_FOUND)
    message(STATUS "Packaged version of nlohmann_json will be used.")
else()
    message(STATUS "Bundled version of nlohmann_json will be downloaded and used.")
    FetchContent_Declare(nlohmann_json
            GIT_REPOSITORY  https://github.com/nlohmann/json
            GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(include)

add_executable(${PROJECT_NAME} 
    main.cpp 
    src/staticserver.cpp
    src/httpheaderparser.cpp 
    src/threadpool.cpp 
    src/hashedwheeltimer.cpp
    src/httpclienttask.cpp
    src/filecachepool.cpp
    src/utils.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)