cmake_minimum_required (VERSION 3.8)

project ("nngx")

include_directories("D:/LanguageLibs/nng-1.11/include")
link_directories("D:/LanguageLibs/nng-1.11/out/Debug")

include_directories(./nngx)

if (MSVC)
    add_compile_options(/utf-8)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

# Include sub-projects.
add_subdirectory ("nngx-caller")