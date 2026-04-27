cmake_minimum_required(VERSION 3.10)
project(FishingFleet)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/includes)

find_package(SQLite3 REQUIRED)

set(SOURCES
    src/main.cpp
    src/database.cpp
    src/auth.cpp
    src/trip.cpp
    src/statistics.cpp
    src/bonus.cpp
    src/utils.cpp
)

add_executable(fishing_fleet ${SOURCES})

target_link_libraries(fishing_fleet SQLite::SQLite3)
