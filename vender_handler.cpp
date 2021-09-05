/*
xp-pen-userland
Copyright (C) 2021 - Aren Villanueva <https://github.com/kurikaesu/>

This program is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include "vendor_handler.h"

bool vendor_handler::setupReportProtocol(libusb_device_handle* handle, int interface_number) {
    int err = libusb_control_transfer(handle,
                                  0x21,
                                  0x0b,
                                  1,
                                  interface_number,
                                  NULL, 0,
                                  1000);
    if (err != LIBUSB_SUCCESS && err != LIBUSB_ERROR_PIPE) {
        std::cout << "Could not set report protocol on interface " << interface_number << " errno: " << err << std::endl;
        return false;
    }

    return true;
}

bool vendor_handler::setupInfiniteIdle(libusb_device_handle* handle, int interface_number) {
    int err = libusb_control_transfer(handle,
                                  0x21,
                                  0x0a,
                                  0 << 8,
                                  interface_number,
                                  NULL, 0,
                                  1000);

    if (err != LIBUSB_SUCCESS && err != LIBUSB_ERROR_PIPE) {
        std::cout << "Could not set infinite idle on interface " << interface_number << " errno: " << err << std::endl;
        return false;
    }

    return true;
}
