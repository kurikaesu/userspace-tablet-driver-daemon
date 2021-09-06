/*
xp-pen-userland
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
#include "artist_22r_pro.h"

artist_22r_pro::artist_22r_pro() {
    productIds.push_back(0x091b);

    for (int currentAssignedButton = BTN_0; currentAssignedButton <= BTN_9; ++currentAssignedButton) {
        padButtonAliases.push_back(currentAssignedButton);
    }

    for (int currentAssignedButton = BTN_A; currentAssignedButton <= BTN_SELECT; ++currentAssignedButton) {
        padButtonAliases.push_back(currentAssignedButton);
    }
}

std::vector<int> artist_22r_pro::handledProductIds() {
    return productIds;
}

std::string artist_22r_pro::getProductName(int productId) {
    if (productId == 0x091b) {
        return "XP-Pen Artist 22R Pro";
    }

    return "Unknown XP-Pen Device";
}

int artist_22r_pro::sendInitKeyOnInterface() {
    return 0x03;
}

bool artist_22r_pro::attachToInterfaceId(int interfaceId) {
    switch (interfaceId) {
    case 2:
        return true;
    default:
        return false;
    }
}

bool artist_22r_pro::attachDevice(libusb_device_handle *handle) {
    unsigned char* buf = new unsigned char[12];

    // We need to get a few more bits of information
    if (libusb_get_string_descriptor(handle, 0x64, 0x0409, buf, 12) != 12) {
        std::cout << "Could not get descriptor";
    }

    int maxWidth = (buf[3] << 8) + buf[2];
    int maxHeight = (buf[5] << 8) + buf[4];
    int maxPressure = (buf[9] << 8) + buf[8];

    unsigned short vendorId = 0x28bd;
    unsigned short productId = 0xf91b;
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
        {"XP-Pen Artist 22R Pro"},
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
        {"XP-Pen Artist 22R Pro Pad"},
    };

    uinputPens[handle] = create_pen(penArgs);
    uinputPads[handle] = create_pad(padArgs);

    return true;
}

void artist_22r_pro::detachDevice(libusb_device_handle *handle) {
    auto lastButtonRecord = lastPressedButton.find(handle);
    if (lastButtonRecord != lastPressedButton.end()) {
        lastPressedButton.erase(lastButtonRecord);
    }

    auto uinputPenRecord = uinputPens.find(handle);
    if (uinputPenRecord != uinputPens.end()) {
        close(uinputPens[handle]);
        uinputPens.erase(uinputPenRecord);
    }

    auto uinputPadRecord = uinputPads.find(handle);
    if (uinputPadRecord != uinputPads.end()) {
        close(uinputPads[handle]);
        uinputPads.erase(uinputPadRecord);
    }
}

bool artist_22r_pro::handleTransferData(libusb_device_handle* handle, unsigned char *data, size_t dataLen) {
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

void artist_22r_pro::handleDigitizerEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
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

void artist_22r_pro::handleFrameEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] >= 0xf0) {
        // Extract the button being pressed (If there is one)
        long button = (data[4] << 16) + (data[3] << 8) + data[2];
        // Grab the first bit set in the button long which tells us the button number
        long position = ffsl(button);

        // Take the left dial
        short leftDialValue = 0;
        if (0x01 & data[7]) {
            leftDialValue = 1;
        } else if (0x02 & data[7]) {
            leftDialValue = -1;
        }

        // Take the right dial
        short rightDialValue = 0;
        if (0x10 & data[7]) {
            rightDialValue = 1;
        } else if (0x20 & data[7]) {
            rightDialValue = -1;
        }

        // Reset back to decimal
        std::cout << std::dec;

        bool dialEvent = false;
        if (leftDialValue != 0) {
            uinput_send(uinputPads[handle], EV_REL, REL_WHEEL, leftDialValue);
            dialEvent = true;
        } else if (rightDialValue != 0) {
            uinput_send(uinputPads[handle], EV_REL, REL_HWHEEL, rightDialValue);
            dialEvent = true;
        }

        if (button != 0) {
            uinput_send(uinputPads[handle], EV_KEY, padButtonAliases[position], 1);
            lastPressedButton[handle] = position;
        } else if (!dialEvent) {
            if (lastPressedButton.find(handle) != lastPressedButton.end() && lastPressedButton[handle] > 0) {
                uinput_send(uinputPads[handle], EV_KEY, padButtonAliases[lastPressedButton[handle]], 0);
                lastPressedButton[handle] = -1;
            } else {
                std::cout << "Got a phantom button up event" << std::endl;
            }
        }

        uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);
    }
}
