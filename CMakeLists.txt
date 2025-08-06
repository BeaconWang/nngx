cmake_minimum_required (VERSION 3.8)

project ("nngx")

include_directories("D:/LanguageLibs/vcpkg-2025.06.13/installed/x64-windows/include")
link_directories("D:/LanguageLibs/vcpkg-2025.06.13/installed/x64-windows/lib")

include_directories(./nngx)

if (MSVC)
    add_compile_options(/utf-8)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

# Include sub-projects.
add_subdirectory ("nngx-caller")