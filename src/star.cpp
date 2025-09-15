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
#include <iomanip>
#include "star.h"

star::star() {
    // Initialize with default device specification
    device_specification spec;
    spec.numButtons = 0;  // Star devices focus on stylus buttons rather than pad buttons
    spec.hasDial = false;
    spec.hasHorizontalDial = false;
    spec.buttonByteIndex = 2;
    spec.dialByteIndex = 7;
    
    // Initialize the base class with the specification
    deviceSpec = spec;
    
    // Initialize pad button aliases (even though not used for pad buttons)
    initializePadButtonAliases(spec.numButtons);
}

std::string star::getProductName(int productId) {
    return "Unknown XP-Pen device";
}

void star::setConfig(nlohmann::json config) {
    // Star devices have a different configuration approach focusing on stylus buttons
    if (!config.contains("mapping") || config["mapping"] == nullptr) {
        config["mapping"] = nlohmann::json({});

        auto addToStylusMap = [&config](int key, int eventType, std::vector<int> codes) {
            std::string evstring = std::to_string(eventType);
            config["mapping"]["stylus_buttons"][std::to_string(key)][evstring] = codes;
        };

        // Default stylus button mappings
        addToStylusMap(BTN_STYLUS, EV_KEY, {});
        addToStylusMap(BTN_STYLUS2, EV_KEY, {});
    }
    jsonConfig = config;

    submitMapping(jsonConfig);
}

int star::sendInitKeyOnInterface() {
    return 0x02;
}

bool star::attachToInterfaceId(int interfaceId) {
    return true;
}

bool star::handleTransferData(libusb_device_handle *handle, unsigned char *data, size_t dataLen, int productId) {
    switch (data[0]) {
        case 0x07:
            handleDigitizerEvent(handle, data, dataLen);
            break;

        case 0x02:
            handleDigitizerEvent(handle, data, dataLen);
            break;

        default:
            std::cout << "Received unknown message" << std::endl;
    }
    return true;
}
