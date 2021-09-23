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
#include "huion_tablet.h"

huion_tablet::huion_tablet() {
    productIds.push_back(0x006e);

    for (int currentAssignedButton = BTN_0; currentAssignedButton <= BTN_9; ++currentAssignedButton) {
        padButtonAliases.push_back(currentAssignedButton);
    }

    for (int currentAssignedButton = BTN_A; currentAssignedButton <= BTN_SELECT; ++currentAssignedButton) {
        padButtonAliases.push_back(currentAssignedButton);
    }
}

huion_tablet::~huion_tablet() noexcept {

}

std::string huion_tablet::getProductName(int productId) {
    if (productId == 0x006e) {
        return "Huion tablet";
    }

    return "Unknown Huion device";
}

void huion_tablet::setConfig(nlohmann::json config) {
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
        addToButtonMap(BTN_0, EV_KEY, {KEY_LEFTCTRL, KEY_S});
        addToButtonMap(BTN_1, EV_KEY, {KEY_F5});
        addToButtonMap(BTN_2, EV_KEY, {KEY_B});
        addToButtonMap(BTN_3, EV_KEY, {KEY_E});
        addToButtonMap(BTN_4, EV_KEY, {KEY_RIGHTBRACE});
        addToButtonMap(BTN_5, EV_KEY, {KEY_LEFTBRACE});
        addToButtonMap(BTN_6, EV_KEY, {KEY_LEFTCTRL, KEY_EQUAL});
        addToButtonMap(BTN_7, EV_KEY, {KEY_LEFTCTRL, KEY_MINUS});
        addToButtonMap(BTN_8, EV_KEY, {KEY_SPACE});
        addToButtonMap(BTN_9, EV_KEY, {KEY_F6});
        addToButtonMap(BTN_SOUTH, EV_KEY, {KEY_LEFTCTRL, KEY_LEFTALT, KEY_Z});
        addToButtonMap(BTN_EAST, EV_KEY, {KEY_LEFTCTRL, KEY_LEFTSHIFT, KEY_N});
    }
    jsonConfig = config;

    submitMapping(jsonConfig);
}

int huion_tablet::sendInitKeyOnInterface() {
    return -1;
}

bool huion_tablet::attachToInterfaceId(int interfaceId) {
    return interfaceId == 0;
}

bool huion_tablet::attachDevice(libusb_device_handle *handle, int interfaceId) {
    unsigned char* buf = new unsigned char[32];

    // We need to get a few more bits of information
    if (libusb_get_string_descriptor(handle, 200, 0x0409, buf, 32) < 18) {
        std::cout << "Could not get descriptor" << std::endl;
        // Let's see which descriptors are actually available
        for (int i = 1; i < 0xff ; ++i) {
            memset(buf, 0, 12);
            int stringLength = libusb_get_string_descriptor(handle, i, 0x0409, buf, 12);
            if (stringLength < 0) {
                std::cout << "Could not get descriptor on index " << i << std::endl;
            } else {
                std::cout << "Descriptor " << i << " has length " << stringLength << std::endl;
            }
        }

        return false;
    }

    int maxWidth = (buf[4] << 16) + (buf[3] << 8) + buf[2];
    int maxHeight = (buf[7] << 16) + (buf[6] << 8) + buf[5];
    int maxPressure = (buf[9] << 8) + buf[8];

    std::cout << "Huion tablet configured with max-width: " << maxWidth << " max-height: " << maxHeight << " max-pressure: " << maxPressure << std::endl;

    unsigned short vendorId = 0x256c;
    unsigned short productId = 0xf06e;
    unsigned short versionId = 0x0001;

    struct uinput_pen_args penArgs {
            .maxWidth = maxWidth,
            .maxHeight = maxHeight,
            .maxPressure = maxPressure,
            .maxTiltX = 60,
            .maxTiltY = 60,
            .vendorId = vendorId,
            .productId = productId,
            .versionId = versionId,
            {"Huion Tablet"},
    };

    struct uinput_pad_args padArgs {
            .padButtonAliases = padButtonAliases,
            .hasWheel = true,
            .hasHWheel = true,
            .wheelMax = 1,
            .hWheelMax = 1,
            .vendorId = vendorId,
            .productId = productId,
            .versionId = versionId,
            {"Huion Tablet Pad"},
    };

    uinputPens[handle] = create_pen(penArgs);
    uinputPads[handle] = create_pad(padArgs);

    return true;
}

bool huion_tablet::handleTransferData(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
//    std::cout << std::dec << "Got transfer of data length: " << (int)dataLen << " data: ";
//    for (int i = 0; i < dataLen; ++i) {
//        std::cout << std::hex << std::setfill('0')  << std::setw(2) << (int)data[i] << ":";
//    }
//    std::cout << std::endl;

    switch (data[0]) {
        case 0x08:

            break;

        default:

            return false;
    }

    switch (data[1]) {
        case 0x80:
        case 0x81:
        case 0x82:
        case 0x84:
            handleDigitizerEvent(handle, data, dataLen);
            break;

        case 0xe0:
            handlePadEvent(handle, data, dataLen);

            break;
        default:
            return false;
    }

    return true;
}

void huion_tablet::handleDigitizerEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    // Extract the X and Y position
    int penX = (data[8] << 16) + (data[3] << 8) + data[2];
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
    short tiltx = (char)data[10];
    short tilty = (char)data[11];

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

void huion_tablet::handlePadEvent(libusb_device_handle* handle, unsigned char* data, size_t dataLen) {
    // Extract the button being pressed (If there is one)
    long button = (data[5] << 8) + data[4];
    // Grab the first bit set in the button long which tells us the button number
    long position = ffsl(button);

    bool shouldSyn = true;
    bool dialEvent = false;

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
        } else {
            std::cout << "Got a phantom button up event" << std::endl;
        }
    }

    if (shouldSyn) {
        uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);
    }
}
