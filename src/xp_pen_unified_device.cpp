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
#include <sstream>
#include "xp_pen_unified_device.h"

std::string xp_pen_unified_device::getInitKey() {
    return {0x02, static_cast<char>(0xb0), 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
}

int xp_pen_unified_device::sendInitKeyOnInterface() {
    return 0x02;
}

bool xp_pen_unified_device::attachToInterfaceId(int interfaceId) {
    return interfaceId == 2;
}

unsigned short xp_pen_unified_device::getDescriptorLength() {
    return 12;
}

bool xp_pen_unified_device::attachDevice(libusb_device_handle *handle, int interfaceId, int productId) {
    const int descriptorLength = getDescriptorLength();
    auto* buf = new unsigned char[descriptorLength];

    // We need to get a few more bits of information
    if (libusb_get_string_descriptor(handle, 0x64, 0x0409, buf, descriptorLength) != descriptorLength) {
        std::cout << "Could not get descriptor" << std::endl;
        return false;
    }

    int maxWidth = (buf[12] << 16) + (buf[3] << 8) + buf[2];
    int maxHeight = (buf[5] << 8) + buf[4];
    maxPressure = (buf[9] << 8) + buf[8];
    int resolution = (buf[11] << 8) + buf[10];

    std::string deviceName = getProductName(productId);
    std::stringstream padNameBuilder;
    padNameBuilder << deviceName;
    padNameBuilder << " Pad";
    std::string padName = padNameBuilder.str();

    std::cout << "Device: " << deviceName << " - Probed maxWidth: (" << maxWidth << ") maxHeight: (" << maxHeight << ") resolution: (" << resolution << ")" << std::endl;

    unsigned short vendorId = 0x28bd;
    unsigned short aliasedProductId = productId + 0xf000;
    unsigned short versionId = 0x0001;

    struct uinput_pen_args penArgs {
            .maxWidth = maxWidth,
            .maxHeight = maxHeight,
            .maxPressure = maxPressure,
            .resolution = resolution,
            .maxTiltX = 60,
            .maxTiltY = 60,
            .vendorId = vendorId,
            .productId = aliasedProductId,
            .versionId = versionId,
    };

    memcpy(penArgs.productName, deviceName.c_str(), deviceName.length());

    struct uinput_pad_args padArgs {
            .padButtonAliases = padButtonAliases,
            .hasWheel = true,
            .hasHWheel = true,
            .wheelMax = 1,
            .hWheelMax = 1,
            .vendorId = vendorId,
            .productId = aliasedProductId,
            .versionId = versionId,
    };

    memcpy(padArgs.productName, deviceName.c_str(), deviceName.length());

    uinputPens[handle] = create_pen(penArgs);
    uinputPads[handle] = create_pad(padArgs);

    return true;
}

void xp_pen_unified_device::handleDigitizerEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen, int offsetPressure) {
    if (data[1] <= 0xc0) {
        // Extract the X and Y position
        int penX = (data[3] << 8) + data[2];
        int penY = (data[5] << 8) + data[4];

        if (dataLen == 12) {
            // This is currently only used by the Artist 24 Pro tablet
            penX += data[10] << 16;
        }

        // Check to see if the pen is touching
        std::bitset<sizeof(data)> stylusTipAndButton(data[1]);
        int pressure = (data[7] << 8) + data[6];
        pressure += offsetPressure;

        // Handle pen coming into/out of proximity
        if (stylusTipAndButton.test(5)) {
            handlePenEnteredProximity(handle);
        } else if (stylusTipAndButton.test(6)) {
            handlePenLeftProximity(handle);
        }

        // Handle actual stylus to digitizer contact
        if (stylusTipAndButton.test(0) && stylusTipAndButton.test(5)) {
            handlePenTouchingDigitizer(handle, applyPressureCurve(pressure));
        } else {
            handlePenTouchingDigitizer(handle, 0);
        }

        // Grab the tilt values
        short tiltx = (char)data[8];
        short tilty = (char)data[9];

        // Check to see if the stylus buttons are being pressed
        if (stylusTipAndButton.test(3)) {
            handleStylusButtonsPressed(handle, BTN_TOOL_RUBBER);
        } else if (stylusTipAndButton.test(1)) {
            handleStylusButtonsPressed(handle, BTN_STYLUS);
        } else if (stylusTipAndButton.test(2)) {
            handleStylusButtonsPressed(handle, BTN_STYLUS2);
        } else if (stylusButtonPressed > 0) {
            handleStylusButtonUnpressed(handle);
        }

        handleCoordsAndTilt(handle, penX, penY, tiltx, tilty);

        uinput_send(uinputPens[handle], EV_SYN, SYN_REPORT, 1);
    }
}
