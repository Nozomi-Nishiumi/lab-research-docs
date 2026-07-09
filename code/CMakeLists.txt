# capture_min / capture_gl のクロスプラットフォームビルド(主に Windows + vcpkg 用)
#
#   vcpkg install opencv4 glfw3      # capture_min だけなら opencv4 のみで可
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
#   cmake --build build --config Release
#
# フォルダにあるソースだけをビルドする(片方しか置いていなくてもよい)。
# macOS では解説ページの1行コンパイルでも可。
cmake_minimum_required(VERSION 3.16)
project(capture_samples CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/capture_min.cpp)
  add_executable(capture_min capture_min.cpp)
  target_link_libraries(capture_min PRIVATE ${OpenCV_LIBS})
  target_include_directories(capture_min PRIVATE ${OpenCV_INCLUDE_DIRS})
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/capture_gl.cpp)
  find_package(glfw3 CONFIG REQUIRED)
  find_package(OpenGL REQUIRED)
  add_executable(capture_gl capture_gl.cpp)
  target_link_libraries(capture_gl PRIVATE ${OpenCV_LIBS} glfw OpenGL::GL)
  target_include_directories(capture_gl PRIVATE ${OpenCV_INCLUDE_DIRS})
endif()
