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
#include "deco_pro_medium_wireless.h"

deco_pro_medium_wireless::deco_pro_medium_wireless() {
    // Create a device specification for deco_pro_medium_wireless devices
    device_specification spec;
    spec.numButtons = 8;
    spec.hasDial = true;
    spec.hasHorizontalDial = true;
    spec.buttonByteIndex = 2;
    spec.dialByteIndex = 7;
    
    // Register product IDs and names
    spec.addProduct(0x093f, "XP-Pen Deco Pro Medium Wireless");
    
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

bool deco_pro_medium_wireless::handleTransferData(libusb_device_handle *handle, unsigned char *data, size_t dataLen, int productId) {
    switch (data[0]) {
        case 0x02:
            handleDigitizerEvent(handle, data, dataLen);
            // Use the generic frame event handler
            handleGenericFrameEvent(handle, data, dataLen, deviceSpec.buttonByteIndex, deviceSpec.dialByteIndex);
            break;

        default:
            break;
    }

    return true;
}
