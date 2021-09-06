cmake_minimum_required(VERSION 3.20)
project(xp_pen_userland)

set(CMAKE_CXX_STANDARD 14)

add_executable(xp_pen_userland main.cpp usb_devices.cpp usb_devices.h vendor_handler.h xp_pen_handler.cpp xp_pen_handler.h device_interface_pair.h event_handler.cpp event_handler.h vendor_handler.cpp artist_22r_pro.cpp artist_22r_pro.h transfer_handler_pair.h transfer_handler.h transfer_handler.cpp)
target_link_libraries(xp_pen_userland usb-1.0)