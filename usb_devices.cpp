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
#include <vector>
#include "usb_devices.h"

usb_devices::usb_devices() {
    libusb_init(&context);
//    libusb_set_option(context, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
}

void usb_devices::handleEvents() {
    libusb_handle_events(context);
}

std::map<short, std::vector<short> > usb_devices::getCandidateDevices(const std::map<short, vendor_handler*> vendorHandlers) {
    std::map<short, std::vector<short> > supportedDevices;
    ssize_t num = libusb_get_device_list(context, &lusb_list);
    if (LIBUSB_ERROR_NO_MEM == num) {
        return supportedDevices;
    }

    // Handle any already connected devices
    for (ssize_t index = 0; index < num; ++index) {
        libusb_device *lusb_dev = lusb_list[index];

        handleDeviceAttach(vendorHandlers, lusb_dev);
    }

    // Return a full list of handle-able devices
    for (auto handler : vendorHandlers) {
        std::vector<int> productIds = handler.second->getProductIds();
        supportedDevices[handler.first].insert(supportedDevices[handler.first].end(),
                                               productIds.begin(),
                                               productIds.end());
    }

    return supportedDevices;
}

void usb_devices::handleDeviceAttach(const std::map<short, vendor_handler *> vendorHandlers, struct libusb_device* device) {
    struct libusb_device_descriptor descriptor;

    libusb_get_device_descriptor(device, &descriptor);
    if (vendorHandlers.find(descriptor.idVendor) != vendorHandlers.end()) {
        if (vendorHandlers.at(descriptor.idVendor)->handleProductAttach(device, descriptor)) {

        }
    }
}

void usb_devices::handleDeviceDetach(const std::map<short, vendor_handler *> vendorHandlers, struct libusb_device *device) {
    struct libusb_device_descriptor descriptor;

    libusb_get_device_descriptor(device, &descriptor);
    if (vendorHandlers.find(descriptor.idVendor) != vendorHandlers.end()) {
        vendorHandlers.at(descriptor.idVendor)->handleProductDetach(device, descriptor);
    }
}
