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
#include <unistd.h>
#include <iomanip>
#include "deco_pro.h"

deco_pro::deco_pro() {
    for (int currentAssignedButton = BTN_0; currentAssignedButton < BTN_8; ++currentAssignedButton) {
        padButtonAliases.push_back(currentAssignedButton);
    }
}

std::string deco_pro::getProductName(int productId) {
    return "Unknown XP-Pen Device";
}

void deco_pro::setConfig(nlohmann::json config) {
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
        addToDialMap(REL_HWHEEL, -1, EV_KEY, {KEY_LEFTCTRL, KEY_MINUS});
        addToDialMap(REL_HWHEEL, 1, EV_KEY, {KEY_LEFTCTRL, KEY_EQUAL});
    }
    jsonConfig = config;

    submitMapping(jsonConfig);
}

int deco_pro::sendInitKeyOnInterface() {
    return 0x02;
}

bool deco_pro::attachToInterfaceId(int interfaceId) {
    switch (interfaceId){
        case 0:
        case 2:
            return true;
        default:
            return false;
    }
}

bool deco_pro::handleTransferData(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    switch (data[0]) {
        case 0x02:
            handleDigitizerEvent(handle, data, dataLen);
            handleUnifiedFrameEvent(handle, data, dataLen);
            break;

        case 0x01:
            handleNonUnifiedFrameEvent(handle, data, dataLen);
            break;

        default:
            break;
    }

    return true;
}

void deco_pro::handleUnifiedFrameEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] >= 0xf0) {
        long button = data[2];
        // Only 8 buttons on this device
        long position = ffsl(data[2]);

        std::bitset<sizeof(data)> touchAndDialBits(data[7]);

        // Take the dial
        short dialValue = 0;
        if (touchAndDialBits.test(0)) {
            dialValue = 1;
        } else if (touchAndDialBits.test(1)) {
            dialValue = -1;
        }

        // Take the touch value
        short touchValue = 0;
        if (touchAndDialBits.test(2)) {
            touchValue = 1;
        } else if (touchAndDialBits.test(3)) {
            touchValue = -1;
        }

        bool shouldSyn = true;
        bool dialEvent = false;

        if (dialValue != 0) {
            handleDialEvent(handle, REL_WHEEL, dialValue);
            shouldSyn = false;
            dialEvent = true;
        }

        if (touchValue != 0) {
            handleDialEvent(handle, REL_HWHEEL, touchValue);
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

void deco_pro::handleNonUnifiedFrameEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    long touchX = data[2] - data[3];
    long touchY = data[4] - data[5];

    bool tapped = data[1] & 0x01;

    char rollerValue = data[6];

    if (touchX != 0 || touchY != 0) {
        uinput_send(uinputPointers[handle], EV_REL, REL_X, touchX);
        uinput_send(uinputPointers[handle], EV_REL, REL_Y, touchY);
        uinput_send(uinputPointers[handle], EV_SYN, SYN_REPORT, 1);
    }

    if (tapped) {
        uinput_send(uinputPointers[handle], EV_KEY, BTN_LEFT, 1);
        wasTapping = true;
    } else if (wasTapping) {
        uinput_send(uinputPointers[handle], EV_KEY, BTN_LEFT, 0);
        wasTapping = false;
    }

    if (rollerValue != 0) {
        uinput_send(uinputPointers[handle], EV_REL, REL_WHEEL, rollerValue);
    }

    uinput_send(uinputPointers[handle], EV_SYN, SYN_REPORT, 1);
}
