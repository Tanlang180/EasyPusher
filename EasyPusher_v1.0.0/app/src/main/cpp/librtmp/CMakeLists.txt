cmake_minimum_required(VERSION 3.10.2)
project("librtmp")

file(GLOB rtmp_srcs *.c)

#-D 定义宏
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_CRYPTO")  # 纯C环境  定义一个宏 跳过 fatal error: 'openssl/ssl.h' file not found 只要有一个.cpp文件就要用CMAKE_CXX_FLAGS

add_library(
        rtmp    #librtmp.a
        STATIC
        ${rtmp_srcs}

)
