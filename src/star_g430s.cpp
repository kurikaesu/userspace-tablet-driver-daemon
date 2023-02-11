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
#include "star_g430s.h"

star_g430s::star_g430s() {
    productIds.push_back(0x0913);
}

std::string star_g430s::getProductName(int productId) {
    if (productId == 0x0913) {
        return "XP-Pen Star G430S";
    }

    return star::getProductName(productId);
}

std::string star_g430s::getInitKey() {
    return {0x02, static_cast<char>(0xb0), 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
}

void star_g430s::setConfig(nlohmann::json config) {
    if (!config.contains("mapping") || config["mapping"] == nullptr) {
        config["mapping"] = nlohmann::json({});

        auto addToButtonMap = [&config](int key, int eventType, std::vector<int> codes) {
            std::string evstring = std::to_string(eventType);
            config["mapping"]["stylus_buttons"][std::to_string(key)][evstring] = codes;
        };

        addToButtonMap(BTN_STYLUS, EV_KEY, {});
        addToButtonMap(BTN_STYLUS2, EV_KEY, {});
    }
    jsonConfig = config;

    submitMapping(jsonConfig);
}


bool star_g430s::attachDevice(libusb_device_handle *handle, int interfaceId, int productId) {
    unsigned int descriptorLength = 12;
    unsigned char* buf = new unsigned char[descriptorLength];

    // We need to get a few more bits of information
    if (libusb_get_string_descriptor(handle, 0x64, 0x0409, buf, descriptorLength) != descriptorLength) {
        std::cout << "Could not get descriptor" << std::endl;
        return false;
    }

    // Hard coding these values in because the probe returns physical values.
    int maxWidth = 0x7fff;
    int maxHeight = 0x7fff;
    maxPressure = (buf[9] << 8) + buf[8];
    int resolution = (buf[11] << 8) + buf[10];

    std::string deviceName = "Star G430S";
    std::cout << "Device: " << std::dec << deviceName << " - Probed maxWidth: (" << maxWidth << ") maxHeight: (" << maxHeight << ") resolution: (" << resolution << ") pressure: " << maxPressure << std::endl;

    unsigned short vendorId = 0x28bd;
    unsigned short aliasedProductId = 0xf913;
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
                {"XP-Pen Star G430S"},
        };

        uinputPens[handle] = create_pen(penArgs);
    }

    return true;
}

