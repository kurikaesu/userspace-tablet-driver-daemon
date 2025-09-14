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
#include "deco_02.h"

deco_02::deco_02() {
    // Create a device specification for Deco 02 devices
    device_specification spec;
    spec.numButtons = 6;
    spec.hasDial = true;
    spec.hasHorizontalDial = false;
    spec.buttonByteIndex = 2;
    spec.dialByteIndex = 7;
    
    // Register product IDs and names
    spec.addProduct(0x0803, "XP-Pen Deco 02");
    
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

bool deco_02::handleTransferData(libusb_device_handle *handle, unsigned char *data, size_t dataLen, int productId) {
    switch (data[0]) {
        case 0x02:
            handleDigitizerEvent(handle, data, dataLen);
            
            // Use the generic frame event handler for button events
            if (data[1] >= 0xf0) {
                long button = data[deviceSpec.buttonByteIndex];
                long position = ffsl(button);

                if (button != 0) {
                    handlePadButtonPressed(handle, position);
                } else {
                    handlePadButtonUnpressed(handle);
                }

                uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);
            }
            break;

        case 0x03:
            handleNonUnifiedDialEvent(handle, data, dataLen);
            break;

        default:
            break;
    }

    return true;
}

void deco_02::handleNonUnifiedDialEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] == 0x01) {
        // Take the dial
        short dialValue = 0;

        switch (data[2]) {
            case 0x2e:
                dialValue = 1;
                break;

            case 0x2d:
                dialValue = -1;
                break;

            default:
                break;
        }

        if (dialValue != 0) {
            handleDialEvent(handle, REL_WHEEL, dialValue);
        }
    }
}
