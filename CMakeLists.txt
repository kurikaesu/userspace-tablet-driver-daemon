cmake_minimum_required(VERSION 3.20)
project(xp_pen_userland)

set(CMAKE_CXX_STANDARD 14)

add_executable(xp_pen_userland src/main.cpp src/usb_devices.cpp src/usb_devices.h src/vendor_handler.h src/xp_pen_handler.cpp src/xp_pen_handler.h src/device_interface_pair.h src/event_handler.cpp src/event_handler.h src/vendor_handler.cpp src/artist_22r_pro.cpp src/artist_22r_pro.h src/transfer_handler_pair.h src/transfer_handler.h src/transfer_handler.cpp)
target_link_libraries(xp_pen_userland usb-1.0)