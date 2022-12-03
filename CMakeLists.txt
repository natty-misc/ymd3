cmake_minimum_required(VERSION 3.17)
project(ymd3)

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig)

pkg_check_modules(GTKMM gtkmm-4.0)
pkg_check_modules(CURL libcurl)
pkg_check_modules(V8 v8)
pkg_check_modules(V8PLATFORM v8_libplatform)

link_directories(${GTKMM_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS} ${V8_LIBRARY_DIRS} ${V8PLATFORM_LIBRARY_DIRS})
include_directories(${GTKMM_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${V8_INCLUDE_DIRS} ${V8PLATFORM_INCLUDE_DIRS})

add_compile_definitions(WIN32_LEAN_AND_MEAN
        GTKMM_DISABLE_DEPRECATED
        GDKMM_DISABLE_DEPRECATED
        GLIBMM_DISABLE_DEPRECATED
        GIOMM_DISABLE_DEPRECATED
        V8_COMPRESS_POINTERS
        V8_31BIT_SMIS_ON_64BIT_ARCH )

add_executable(ymd3 src/main.cpp src/shared.h src/mainwindow.cpp src/mainwindow.h src/youtuberetriever.cpp src/youtuberetriever.h src/retrieverscript.cpp src/retrieverscript.h src/version.h)
target_link_libraries(ymd3 pthread stdc++ stdc++fs ${GTKMM_LIBRARIES} ${CURL_LIBRARIES} ${V8_LIBRARIES} ${V8PLATFORM_LIBRARIES})