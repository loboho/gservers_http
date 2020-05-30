CMAKE_MINIMUM_REQUIRED(VERSION 2.8.10)

project(gservers_http)


#设置编译器
#SET(CMAKE_CXX_COMPILER "g++")
#设置构建类型，及相应的编译选项
#set (CMAKE_CXX_FLAGS " -std=c++17")
SET(CMAKE_BUILD_TYPE "Release")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -g -Wall -fmessage-length=0 -MMD -MP")

#设置执行文件输出目录
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})
#设置库输出路径
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#头文件搜索目录
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/include)
#链接库搜索路径
LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/lib)
SET(LIBS_LIST pthread -Wl,--whole-archive gbase -Wl,--no-whole-archive lua dl)

MESSAGE("++++++++++++++Add Projects+++++++++++++++++")
add_subdirectory(gbase gbase)

AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR} ROOT_SRCS)
# 设置生成可执行文件的名称
SET(EXECUTABLE_FILE_NAME gservers_http)
ADD_EXECUTABLE(${EXECUTABLE_FILE_NAME} ${ROOT_SRCS})
# 设置可执行文件链接的库名称
TARGET_LINK_LIBRARIES(${EXECUTABLE_FILE_NAME} ${LIBS_LIST})
