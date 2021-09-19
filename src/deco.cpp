/*
xp_pen_userland
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

#include "deco.h"

deco::deco() {
    for (int currentAssignedButton = BTN_0; currentAssignedButton < BTN_8; ++currentAssignedButton) {
        padButtonAliases.push_back(currentAssignedButton);
    }
}

std::string deco::getProductName(int productId) {
    return "Unknown XP-Pen Device";
}

void deco::setConfig(nlohmann::json config) {
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
    }
    jsonConfig = config;

    submitMapping(jsonConfig);
}

int deco::sendInitKeyOnInterface() {
    return 0x02;
}

bool deco::attachToInterfaceId(int interfaceId) {
    switch (interfaceId){
        case 2:
            return true;

        default:
            return false;
    }
}

bool deco::handleTransferData(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    switch (data[0]) {
        case 0x02:
            handleDigitizerEvent(handle, data, dataLen);
            handleUnifiedFrameEvent(handle, data, dataLen);
            break;

        default:
            break;
    }

    return true;
}

void deco::handleDigitizerEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] < 0xb0) {
        // Extract the X and Y position
        int penX = (data[3] << 8) + data[2];
        int penY = (data[5] << 8) + data[4];

        // Check to see if the pen is touching
        int pressure;
        if (0x01 & data[1]) {
            // Grab the pressure amount
            pressure = (data[7] << 8) + data[6];

            uinput_send(uinputPens[handle], EV_KEY, BTN_TOOL_PEN, 1);
            uinput_send(uinputPens[handle], EV_ABS, ABS_PRESSURE, pressure);
        } else {
            uinput_send(uinputPens[handle], EV_KEY, BTN_TOOL_PEN, 0);
        }

        // Grab the tilt values
        short tiltx = (char)data[8];
        short tilty = (char)data[9];

        // Check to see if the stylus buttons are being pressed
        if (0x02 & data[1]) {
            uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS, 1);
            uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS2, 0);
        } else if (0x04 & data[1]) {
            uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS, 0);
            uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS2, 1);
        } else {
            uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS, 0);
            uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS2, 0);
        }

        uinput_send(uinputPens[handle], EV_ABS, ABS_X, penX);
        uinput_send(uinputPens[handle], EV_ABS, ABS_Y, penY);
        uinput_send(uinputPens[handle], EV_ABS, ABS_TILT_X, tiltx);
        uinput_send(uinputPens[handle], EV_ABS, ABS_TILT_Y, tilty);

        uinput_send(uinputPens[handle], EV_SYN, SYN_REPORT, 1);
    }
}

void deco::handleUnifiedFrameEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] >= 0xf0) {
        long button = data[2];
        // Only 8 buttons on this device
        long position = ffsl(data[2]);

        // Take the dial
        short dialValue = 0;
        if (0x01 & data[7]) {
            dialValue = 1;
        } else if (0x02 & data[7]) {
            dialValue = -1;
        }

        if (button != 0) {
            auto padMap = padMapping.getPadMap(padButtonAliases[position - 1]);
            for (auto pmap : padMap) {
                uinput_send(uinputPads[handle], pmap.event_type, pmap.event_value, 1);
            }
            lastPressedButton[handle] = position;
        }

        // This should always send the SYN no matter what
        uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);
    }
}
