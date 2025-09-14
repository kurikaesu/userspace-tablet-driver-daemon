/*
userspace_tablet_driver_daemon
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
#include "artist_15_6_pro.h"

artist_15_6_pro::artist_15_6_pro() {
    // Create a device specification for artist_15_6_pro devices
    device_specification spec;
    spec.numButtons = 10;
    spec.hasDial = true;
    spec.hasHorizontalDial = false;
    spec.buttonByteIndex = 2;
    spec.dialByteIndex = 7;
    
    // Register product IDs and names
    spec.addProduct(0x090d, "XP-Pen Artist 15.6 Pro");
    
    // Initialize the base class with the specification
    deviceSpec = spec;
    
    // Register products
    for (const auto& product : spec.productNames) {
        registerProduct(product.first, product.second);
        productIds.push_back(product.first);
    }
    
    // Initialize pad button aliases
    initializePadButtonAliases(spec.numButtons);
    
    // Apply default configuration with dial
    applyDefaultConfig(true);
}

bool artist_15_6_pro::handleTransferData(libusb_device_handle *handle, unsigned char *data, size_t dataLen, int productId) {
    switch (data[0]) {
        case 0x02:
            handleDigitizerEvent(handle, data, dataLen);
            handleFrameEvent(handle, data, dataLen);
            break;

        default:
            break;
    }

    return true;
}

void artist_15_6_pro::handleFrameEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] >= 0xf0) {
        // Extract the button being pressed (If there is one)
        long button = (data[4] << 16) + (data[3] << 8) + data[2];
        // Grab the first bit set in the button long which tells us the button number
        long position = ffsl(button);

        std::bitset<8> dialBits(data[7]);

        // Take the left dial
        short leftDialValue = 0;
        if (dialBits.test(0)) {
            leftDialValue = 1;
        } else if (dialBits.test(1)) {
            leftDialValue = -1;
        }

        bool shouldSyn = true;
        bool dialEvent = false;

        if (leftDialValue != 0) {
            handleDialEvent(handle, REL_WHEEL, leftDialValue);
            shouldSyn = false;
            dialEvent = true;
        }

        if (button != 0) {
            handlePadButtonPressed(handle, position);
        } else if (!dialEvent) {
            handlePadButtonUnpressed(handle);
        }

        if (shouldSyn) {
            uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);
        }
    }
}
