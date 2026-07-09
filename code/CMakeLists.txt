# capture_gl のクロスプラットフォームビルド(主に Windows + vcpkg 用)
#
#   vcpkg install opencv4 glfw3
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
#   cmake --build build --config Release
#
# macOS / Linux では解説ページの1行コンパイルでも可。
cmake_minimum_required(VERSION 3.16)
project(capture_gl CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(capture_gl capture_gl.cpp)
target_link_libraries(capture_gl PRIVATE ${OpenCV_LIBS} glfw OpenGL::GL)
target_include_directories(capture_gl PRIVATE ${OpenCV_INCLUDE_DIRS})
