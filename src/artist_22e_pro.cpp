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
#include <cstring>
#include <unistd.h>
#include <linux/uinput.h>
#include "artist_22e_pro.h"

artist_22e_pro::artist_22e_pro() {
    productIds.push_back(0x090b);

    for (int currentAssignedButton = BTN_0; currentAssignedButton <= BTN_9; ++currentAssignedButton) {
        padButtonAliases.push_back(currentAssignedButton);
    }

    for (int currentAssignedButton = BTN_A; currentAssignedButton <= BTN_SELECT; ++currentAssignedButton) {
        padButtonAliases.push_back(currentAssignedButton);
    }
}

std::string artist_22e_pro::getProductName(int productId) {
    if (productId == 0x090b) {
        return "XP-Pen Artist 22E Pro";
    }

    return "Unknown XP-Pen Device";
}

void artist_22e_pro::setConfig(nlohmann::json config) {
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

        // We are going to emulate the default mapping of the device
        addToButtonMap(BTN_0, EV_KEY, {KEY_B});
        addToButtonMap(BTN_1, EV_KEY, {KEY_E});
        addToButtonMap(BTN_2, EV_KEY, {KEY_LEFTALT});
        addToButtonMap(BTN_3, EV_KEY, {KEY_SPACE});
        addToButtonMap(BTN_4, EV_KEY, {KEY_LEFTCTRL, KEY_S});
        addToButtonMap(BTN_5, EV_KEY, {KEY_LEFTCTRL, KEY_Z});
        addToButtonMap(BTN_6, EV_KEY, {KEY_LEFTCTRL, KEY_LEFTALT, KEY_Z});
        addToButtonMap(BTN_7, EV_KEY, {KEY_LEFTCTRL, KEY_LEFTSHIFT, KEY_Z});
        addToButtonMap(BTN_8, EV_KEY, {KEY_V});
        addToButtonMap(BTN_9, EV_KEY, {KEY_L});
        addToButtonMap(BTN_SOUTH, EV_KEY, {KEY_LEFTCTRL, KEY_0});
        addToButtonMap(BTN_EAST, EV_KEY, {KEY_LEFTCTRL, KEY_N});
        addToButtonMap(BTN_C, EV_KEY, {KEY_LEFTCTRL, KEY_LEFTSHIFT, KEY_N});
        addToButtonMap(BTN_NORTH, EV_KEY, {KEY_LEFTCTRL, KEY_E});
        addToButtonMap(BTN_WEST, EV_KEY, {KEY_F});
        addToButtonMap(BTN_Z, EV_KEY, {KEY_D});
    }
    jsonConfig = config;

    submitMapping(jsonConfig);
}

bool artist_22e_pro::handleTransferData(libusb_device_handle* handle, unsigned char *data, size_t dataLen, int productId) {
    switch (data[0]) {
        // Unified interface
        case 0x02:
            handleDigitizerEvent(handle, data, dataLen);
            handleFrameEvent(handle, data, dataLen);

            break;

        default:
            break;
    }

    return true;
}

void artist_22e_pro::handleFrameEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] >= 0xf0) {
        // Extract the button being pressed (If there is one)
        long button = (data[4] << 16) + (data[3] << 8) + data[2];
        // Grab the first bit set in the button long which tells us the button number
        long position = ffsl(button);

        if (button != 0) {
            handlePadButtonPressed(handle, position);
        } else {
            handlePadButtonUnpressed(handle);
        }

        uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);
    }
}
