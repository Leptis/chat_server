cmake_minimum_required(VERSION 3.14)
project(chat_server)

set(CMAKE_CXX_STANDARD 14)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11 -lfcgi++ -lfcgi -lpq -lpqxx")


file(GLOB_RECURSE SOURCE ../chat_server/src/*)
file(GLOB_RECURSE FASTCGI ../chat_server/fastCGI/*)

set(SOURCE_FILE src src/main.cpp)

#add_executable(chat_server src/main.cpp)

add_executable(
        ${PROJECT_NAME}
        ${SOURCE_FILES}
        ${HEAD}
        ${SOURCE}
        ${FASTCGI}
)

#target_link_libraries(${PROJECT_NAME})