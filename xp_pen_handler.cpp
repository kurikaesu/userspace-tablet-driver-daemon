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
#include <algorithm>
#include "xp_pen_handler.h"

xp_pen_handler::xp_pen_handler() {
    std::cout << "xp_pen_handler initialized" << std::endl;

    handledProducts.push_back(0x091b);
}

xp_pen_handler::~xp_pen_handler() {
    for (auto deviceInterfaces : deviceInterfaces) {
        cleanupDevice(deviceInterfaces);
    }
}

int xp_pen_handler::getVendorId() {
    return 0x28bd;
}

std::vector<int> xp_pen_handler::getProductIds() {
    return handledProducts;
}

bool xp_pen_handler::handleProduct(libusb_device* device, struct libusb_device_descriptor descriptor) {
    std::cout << "xp_pen_handler" << std::endl;
    libusb_device_handle* handle = NULL;
    device_interface_pair* interfacePair = NULL;
    switch (descriptor.idProduct) {
        case 0x091b:
            std::cout << "Got known device" << std::endl;
            interfacePair = claimDevice(device, handle);
            deviceInterfaces.push_back(interfacePair);
            deviceInterfaceMap[device] =interfacePair;

            return true;

        default:
            std::cout << "Unknown device" << std::endl;

            break;
    }

    return false;
}

void xp_pen_handler::handleProductDetach(libusb_device *device, struct libusb_device_descriptor descriptor) {
    for (auto deviceObj : deviceInterfaceMap) {
        if (deviceObj.first == device) {
            std::cout << "Handling device detach" << std::endl;
            cleanupDevice(deviceObj.second);
            libusb_close(deviceObj.second->deviceHandle);

            auto deviceInterfacesIterator = std::find(deviceInterfaces.begin(), deviceInterfaces.end(), deviceObj.second);
            if (deviceInterfacesIterator != deviceInterfaces.end()) {
                deviceInterfaces.erase(deviceInterfacesIterator);
            }

            auto deviceMapIterator = std::find(deviceInterfaceMap.begin(), deviceInterfaceMap.end(), deviceObj);
            if (deviceMapIterator != deviceInterfaceMap.end()) {
                deviceInterfaceMap.erase(deviceMapIterator);
            }

            break;
        }
    }
}

device_interface_pair* xp_pen_handler::claimDevice(libusb_device *device, libusb_device_handle *handle) {
    device_interface_pair* deviceInterface = new device_interface_pair();
    int err;

    if ((err = libusb_open(device, &handle)) == LIBUSB_SUCCESS) {
        deviceInterface->deviceHandle = handle;

        for (short interface_number = 0; interface_number <= 2; ++interface_number) {
            err = libusb_detach_kernel_driver(handle, interface_number);
            if (LIBUSB_SUCCESS == err) {
                std::cout << "Detached interface from kernel " << interface_number << std::endl;
                deviceInterface->detachedInterfaces.push_back(interface_number);
            }

            if (libusb_claim_interface(handle, interface_number) == LIBUSB_SUCCESS) {
                std::cout << "Claimed interface " << interface_number << std::endl;
                deviceInterface->claimedInterfaces.push_back(interface_number);
            }
        }
    } else {
        std::cout << "libusb_open returned error " << err << std::endl;
    }

    return deviceInterface;
}

void xp_pen_handler::cleanupDevice(device_interface_pair *pair) {
    for (auto interface: pair->claimedInterfaces) {
        libusb_release_interface(pair->deviceHandle, interface);
        std::cout << "Releasing interface " << interface << std::endl;
    }

    for (auto interface: pair->detachedInterfaces) {
        libusb_attach_kernel_driver(pair->deviceHandle, interface);
        std::cout << "Reattaching to kernel interface " << interface << std::endl;
    }
}
