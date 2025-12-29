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
#include "ac19.h"

ac19::ac19() {
    productIds.push_back(0x0201);

    for (int currentAssignedButton = BTN_0; currentAssignedButton < BTN_9; ++currentAssignedButton) {
        padButtonAliases.push_back(currentAssignedButton);
    }
}

std::string ac19::getProductName(int productId) {
    if (productId == 0x0201) {
        return "XP-Pen AC19 Shortcut Remote";
    }

    return "Unknown XP-Pen device";
}

void ac19::setConfig(nlohmann::json config) {
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
        addToButtonMap(BTN_8, EV_KEY, {KEY_V});
        addToButtonMap(BTN_9, EV_KEY, {KEY_L});

        addToDialMap(REL_WHEEL, -1, EV_KEY, {KEY_LEFTCTRL, KEY_MINUS});
        addToDialMap(REL_WHEEL, 1, EV_KEY, {KEY_LEFTCTRL, KEY_EQUAL});
    }
    jsonConfig = config;

    submitMapping(jsonConfig);
}

int ac19::sendInitKeyOnInterface() {
    return -1;
}

bool ac19::attachToInterfaceId(int interfaceId) {
    return true;
}

bool ac19::attachDevice(libusb_device_handle *handle, int interfaceId, int productId) {
    if (interfaceId == 2) {
        unsigned short vendorId = 0x28bd;
        unsigned short productId = 0xf201;
        unsigned short versionId = 0x0001;

        struct uinput_pad_args padArgs{
                .padButtonAliases = padButtonAliases,
                .hasWheel = true,
                .hasHWheel = true,
                .wheelMax = 1,
                .hWheelMax = 1,
                .vendorId = vendorId,
                .productId = productId,
                .versionId = versionId,
                {"XP-Pen AC19 Shortcut Remote"},
        };

        int pad_fd = create_pad(padArgs);
        if (pad_fd < 0)
            return false;
        uinputPads[handle] = pad_fd;
    }

    return true;
}

bool ac19::handleTransferData(libusb_device_handle *handle, unsigned char *data, size_t dataLen, int productId) {
    switch (data[0]) {
        case 0x02:
            handleFrameEvent(handle, data, dataLen);
            break;

        default:
            break;
    }

    return true;
}

void ac19::handleFrameEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    long button = 0;

    // We're going to use a lookup here because the buttons aren't logical
    switch (data[3]) {
        case 0x19:
            button = 1;
            break;

        case 0x0c:
            button = 2;
            break;

        case 0x2c:
            button = 3;
            break;

        case 0x05:
            button = 4;
            break;

        case 0x28:
            button = 5;
            break;

        case 0x1d:
            button = 6;
            break;

        case 0x10:
            button = 7;
            break;

        case 0x13:
            button = 8;
            break;

        case 0x18:
            button = 9;
            break;

        default:
            break;
    }

    // Handle the dial middle button
    if (data[1] == 0x02) {
        button = 10;
    }

    // Now we take the dial
    short dialValue = 0;
    if (0x01 & data[1] && 0x1d ^ data[3]) {
        switch (data[3]) {
            case 0x56:
                dialValue = -1;
                break;

            case 0x57:
                dialValue = 1;
                break;

            default:
                break;
        }
    }

    bool shouldSyn = true;
    bool dialEvent = false;

    if (dialValue != 0) {
        handleDialEvent(handle, REL_WHEEL, dialValue);
        shouldSyn = false;
        dialEvent = true;
    }

    if (button != 0) {
        handlePadButtonPressed(handle, button);
    } else if (!dialEvent) {
        handlePadButtonUnpressed(handle);
    }

    if (shouldSyn) {
        uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);
    }
}
