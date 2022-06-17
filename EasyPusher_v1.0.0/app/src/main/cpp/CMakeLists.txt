# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.10.2)

add_subdirectory(librtmp)

include_directories(librtmp)

include_directories(x264/include)

include_directories(libfaac/include)

# CMAKE_CXX_FLAGS变量（对C++源代码编译） 中引入x264库
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/x264/lib/${CMAKE_ANDROID_ARCH_ABI}")
# CMAKE_CXX_FLAGS变量（对C++源代码编译） 中引入faac库
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/libfaac/lib/${CMAKE_ANDROID_ARCH_ABI}")

add_library(
        native-lib
        SHARED
        native-lib.cpp
        VideoChannel.cpp
        AudioChannel.cpp
)

target_link_libraries(
        native-lib
        rtmp
        x264
        faac
        log
        android
        z
)


