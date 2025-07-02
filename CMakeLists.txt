cmake_minimum_required (VERSION 3.8)

project ("nngx")

include_directories("D:/LanguageLibs/nng-1.11/include")
link_directories("D:/LanguageLibs/nng-1.11/out/Debug")

include_directories(./nngx)

# Include sub-projects.
add_subdirectory ("nngx-caller")