cmake_minimum_required(VERSION 3.12)

project(StaticServer VERSION 1.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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