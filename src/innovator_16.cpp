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
#include "innovator_16.h"

innovator_16::innovator_16() {
    productIds.push_back(0x092c);

    for (int currentAssignedButton = BTN_0; currentAssignedButton < BTN_8; ++currentAssignedButton) {
        padButtonAliases.push_back(currentAssignedButton);
    }
}

std::string innovator_16::getProductName(int productId) {
    if (productId == 0x092c) {
        return "XP-Pen Innovator 16";
    }

    return "Unknown XP-Pen Device";
}

void innovator_16::setConfig(nlohmann::json config) {
    if (!config.contains("mapping") || config["mapping"] == nullptr) {
        config["mapping"] = nlohmann::json({});

        auto addToButtonMap = [&config](int key, int eventType, std::vector<int> codes) {
            std::string evstring = std::to_string(eventType);
            config["mapping"]["buttons"][std::to_string(key)][evstring] = codes;
        };

        auto addToDialMap = [&config](int dial, int value, int eventType, std::vector<int> codes) {
            std::string strvalue = std::to_string(value);
            std::string evstring = std::to_string(eventType);
            config["mapping"]["dials"][std::to_string(dial)][strvalue][evstring] = codes;
        };

        addToButtonMap(BTN_0, EV_KEY, {KEY_B});
        addToButtonMap(BTN_1, EV_KEY, {KEY_E});
        addToButtonMap(BTN_2, EV_KEY, {KEY_SPACE});
        addToButtonMap(BTN_3, EV_KEY, {KEY_LEFTALT});
        addToButtonMap(BTN_4, EV_KEY, {KEY_V});
        addToButtonMap(BTN_5, EV_KEY, {KEY_LEFTCTRL, KEY_S});
        addToButtonMap(BTN_6, EV_KEY, {KEY_LEFTCTRL, KEY_Z});
        addToButtonMap(BTN_7, EV_KEY, {KEY_LEFTCTRL, KEY_LEFTALT, KEY_N});

        addToDialMap(REL_WHEEL, -1, EV_KEY, {KEY_LEFTCTRL, KEY_MINUS});
        addToDialMap(REL_WHEEL, 1, EV_KEY, {KEY_LEFTCTRL, KEY_EQUAL});
    }
    jsonConfig = config;

    submitMapping(jsonConfig);
}

int innovator_16::sendInitKeyOnInterface() {
    return 0x02;
}

bool innovator_16::attachToInterfaceId(int interfaceId) {
    return interfaceId == 2;
}

bool innovator_16::attachDevice(libusb_device_handle *handle, int interfaceId, int productId) {
    auto *buf = new unsigned char[13];

    // We need to get a few more bits of information
    if (libusb_get_string_descriptor(handle, 0x64, 0x0409, buf, 13) != 13) {
        std::cout << "Could not get descriptor" << std::endl;
        return false;
    }

    int maxWidth = (buf[12] << 16) + (buf[3] << 8) + buf[2];
    int maxHeight = (buf[5] << 8) + buf[4];
    maxPressure = (buf[9] << 8) + buf[8];
    int resolution = (buf[11] << 8) + buf[10];

    std::cout << "Probed maxWidth: (" << maxWidth << ") maxHeight: (" << maxHeight << ") resolution: (" << resolution << ")" << std::endl;

    unsigned short vendorId = 0x28bd;
    unsigned short aliasedProductId = 0xf92c;
    unsigned short versionId = 0x0001;

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
            {"XP-Pen Innovator 16"},
    };

    struct uinput_pad_args padArgs{
            .padButtonAliases = padButtonAliases,
            .hasWheel = true,
            .hasHWheel = true,
            .wheelMax = 1,
            .hWheelMax = 1,
            .vendorId = vendorId,
            .productId = aliasedProductId,
            .versionId = versionId,
            {"XP-Pen Innovator 16 Pad"},
    };

    uinputPens[handle] = create_pen(penArgs);
    uinputPads[handle] = create_pad(padArgs);

    return true;
}

bool innovator_16::handleTransferData(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
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

void innovator_16::handleFrameEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] >= 0xf0) {
        long button = data[2];
        // Only 8 buttons on this device
        long position = ffsl(data[2]);

        std::bitset<sizeof(data)> dialBits(data[7]);

        // Take the dial
        short dialValue = 0;
        if (dialBits.test(0)) {
            dialValue = 1;
        } else if (dialBits.test(1)) {
            dialValue = -1;
        }

        bool shouldSyn = true;
        bool dialEvent = false;

        if (dialValue != 0) {
            handleDialEvent(handle, REL_WHEEL, dialValue);
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
