
# Add source to this project's executable.
add_executable (nngx-caller "main.cpp" "nngUtil.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET nngx-caller PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
target_link_libraries(nngx-caller nng)



target_link_libraries(nngx-caller nng)

if (WIN32)
    target_link_libraries(nngx-caller Ws2_32 Mswsock)
else()
    target_link_libraries(nngx-caller pthread)
endif()
