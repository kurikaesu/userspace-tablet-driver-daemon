/*
userspace-tablet-driver-daemon
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
#include "artist_pro_16tp.h"

artist_pro_16tp::artist_pro_16tp() {
    // Create a device specification for artist_pro_16tp devices
    device_specification spec;
    spec.numButtons = 0;
    spec.hasDial = false;
    spec.hasHorizontalDial = false;
    spec.buttonByteIndex = 2;
    spec.dialByteIndex = 7;
    
    // Register product IDs and names
    spec.addProduct(0x092e, "XP-Pen Artist Pro 16TP");
    
    // Initialize the base class with the specification
    deviceSpec = spec;
    
    // Register products
    for (const auto& product : spec.productNames) {
        registerProduct(product.first, product.second);
        productIds.push_back(product.first);
    }
    
    // Initialize pad button aliases
    initializePadButtonAliases(spec.numButtons);
    
    // Apply default configuration
    applyDefaultConfig(false);
}

bool artist_pro_16tp::handleTransferData(libusb_device_handle *handle, unsigned char *data, size_t dataLen, int productId) {
    switch (data[0]) {
        case 0x02:
            handleDigitizerEvent(handle, data, dataLen);
            break;

        default:
            break;
    }

    return true;
}

bool artist_pro_16tp::attachDevice(libusb_device_handle *handle, int interfaceId, int productId) {
    unsigned int descriptorLength = 12;
    std::vector<unsigned char> buf_(descriptorLength);
    auto* buf = &buf_[0];

    // We need to get a few more bits of information
    if (libusb_get_string_descriptor(handle, 0x64, 0x0409, buf, descriptorLength) != descriptorLength) {
        std::cout << "Could not get descriptor" << std::endl;
        return false;
    }

    // Hard coding these values in because the probe returns erroneous values.
    int maxWidth = 0x10e24;
    int maxHeight = 0x97dd;
    maxPressure = (buf[9] << 8) + buf[8];
    int resolution = (buf[11] << 8) + buf[10];

    std::string deviceName = "Artist Pro 16TP";
    std::cout << "Device: " << std::dec << deviceName << " - Probed maxWidth: (" << maxWidth << ") maxHeight: (" << maxHeight << ") resolution: (" << resolution << ") pressure: " << maxPressure << std::endl;

    unsigned short vendorId = 0x28bd;
    unsigned short aliasedProductId = 0x092e;
    unsigned short versionId = 0x0001;

    if (interfaceId == 2) {
        struct uinput_pen_args penArgs{
                .maxWidth = maxWidth,
                .maxHeight = maxHeight,
                .maxPressure = maxPressure,
                .resolution = resolution,
                .maxTiltX = 60,
                .maxTiltY = 60,
                .vendorId = vendorId,
                .productId = aliasedProductId,
                .versionId = versionId,
                {"XP-Pen Artist Pro 16TP"},
        };

        auto pen_fd = create_pen(penArgs);
        if (pen_fd < 0)
            return false;

        uinputPens[handle] = pen_fd;
    }

    return true;
}
