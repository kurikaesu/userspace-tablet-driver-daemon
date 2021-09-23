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
        case 2:
            return true;

        case 0:
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

void deco_pro::handleDigitizerEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
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

void deco_pro::handleUnifiedFrameEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
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

        // Take the touch value
        short touchValue = 0;
        if (0x04 & data[7]) {
            touchValue = 1;
        } else if (0x08 & data[7]) {
            touchValue = -1;
        }

        bool shouldSyn = true;
        bool dialEvent = false;

        if (dialValue != 0) {
            bool send_reset = false;
            auto dialMap = dialMapping.getDialMap(EV_REL, REL_WHEEL, dialValue);
            for (auto dmap: dialMap) {
                uinput_send(uinputPads[handle], dmap.event_type, dmap.event_value, dmap.event_data);
                if (dmap.event_type == EV_KEY) {
                    send_reset = true;
                }
            }

            uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);

            if (send_reset) {
                for (auto dmap: dialMap) {
                    // We have to handle key presses manually here because this device does not send reset events
                    if (dmap.event_type == EV_KEY) {
                        uinput_send(uinputPads[handle], dmap.event_type, dmap.event_value, 0);
                    }
                }
            }
            uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);

            shouldSyn = false;
            dialEvent = true;
        }

        if (touchValue != 0) {
            bool send_reset = false;
            auto touchMap = dialMapping.getDialMap(EV_REL, REL_HWHEEL, touchValue);
            for (auto dmap: touchMap) {
                uinput_send(uinputPads[handle], dmap.event_type, dmap.event_value, dmap.event_data);
                if (dmap.event_type == EV_KEY) {
                    send_reset = true;
                }
            }

            uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);

            if (send_reset) {
                for (auto dmap: touchMap) {
                    // We have to handle key presses manually here because this device does not send reset events
                    if (dmap.event_type == EV_KEY) {
                        uinput_send(uinputPads[handle], dmap.event_type, dmap.event_value, 0);
                    }
                }
            }
            uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);

            shouldSyn = false;
            dialEvent = true;
        }

        if (button != 0) {
            auto padMap = padMapping.getPadMap(padButtonAliases[position - 1]);
            for (auto pmap : padMap) {
                uinput_send(uinputPads[handle], pmap.event_type, pmap.event_value, 1);
            }
            lastPressedButton[handle] = position;
        } else if (!dialEvent) {
            if (lastPressedButton.find(handle) != lastPressedButton.end() && lastPressedButton[handle] > 0) {
                auto padMap = padMapping.getPadMap(padButtonAliases[lastPressedButton[handle] - 1]);
                for (auto pmap : padMap) {
                    uinput_send(uinputPads[handle], pmap.event_type, pmap.event_value, 0);
                }
                lastPressedButton[handle] = -1;
            }
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
