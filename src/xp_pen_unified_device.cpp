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

xp_pen_unified_device::xp_pen_unified_device() {
    // Default constructor
}

xp_pen_unified_device::xp_pen_unified_device(const device_specification& spec) : deviceSpec(spec) {
    // Initialize from device specification
    
    // Register product IDs and names
    for (const auto& product : spec.productNames) {
        registerProduct(product.first, product.second);
        productIds.push_back(product.first);
    }
    
    // Initialize pad button aliases
    initializePadButtonAliases(spec.numButtons);
}

void xp_pen_unified_device::registerProduct(int productId, const std::string& name) {
    productNameMap[productId] = name;
}

std::string xp_pen_unified_device::getProductName(int productId) {
    auto it = productNameMap.find(productId);
    if (it != productNameMap.end()) {
        return it->second;
    }
    return "Unknown XP-Pen Device";
}

void xp_pen_unified_device::setConfig(nlohmann::json config) {
    if (!config.contains("mapping") || config["mapping"] == nullptr) {
        // Apply default configuration
        applyDefaultConfig(deviceSpec.hasDial);
    } else {
        jsonConfig = config;
    }
    
    submitMapping(jsonConfig);
}

void xp_pen_unified_device::applyDefaultConfig(bool withDial) {
    button_mapping_configuration buttonConfig;
    
    if (withDial) {
        buttonConfig = button_mapping_configuration::createDefaultXPPenConfigWithDial();
    } else {
        buttonConfig = button_mapping_configuration::createDefaultXPPenConfig();
    }
    
    buttonConfig.applyToJson(jsonConfig);
}

void xp_pen_unified_device::initializePadButtonAliases(int numButtons) {
    padButtonAliases.clear();
    for (int currentAssignedButton = BTN_0; currentAssignedButton < BTN_0 + numButtons; ++currentAssignedButton) {
        padButtonAliases.push_back(currentAssignedButton);
    }
}

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
    std::vector<unsigned char> buf(descriptorLength);

    // We need to get a few more bits of information
    if (libusb_get_string_descriptor(handle, 0x64, 0x0409, &buf[0], descriptorLength) != descriptorLength) {
        std::cout << "Could not get descriptor" << std::endl;
        return false;
    }

    // I do not how these work exactly, but the buf[12] access is out of bounds
    // and amounted to 0 if we're lucky and some random value otherwise
    int maxWidth = /*(buf[12] << 16)*/ + (buf[3] << 8) + buf[2];
    int maxHeight = (buf[5] << 8) + buf[4];
    maxPressure = (buf[9] << 8) + buf[8];
    int resolution = (buf[11] << 8) + buf[10];

    std::string deviceName = getProductName(productId);
    std::string padName = std::string(deviceName).append(" Pad");
    std::string penName = std::string(deviceName).append(" Pen");

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

    memcpy(penArgs.productName, penName.c_str(), penName.length());

    struct uinput_pad_args padArgs {
            .padButtonAliases = padButtonAliases,
            .hasWheel = deviceSpec.hasDial,
            .hasHWheel = deviceSpec.hasHorizontalDial,
            .wheelMax = 1,
            .hWheelMax = 1,
            .vendorId = vendorId,
            .productId = aliasedProductId,
            .versionId = versionId
    };

    memcpy(padArgs.productName, padName.c_str(), padName.length());

    auto pen_fd = create_pen(penArgs);
    auto pad_fd = create_pad(padArgs);
    if (pen_fd < 0 || pad_fd < 0)
        return false;
    uinputPens[handle] = pen_fd;
    uinputPads[handle] = pad_fd;

    return true;
}

void xp_pen_unified_device::handleDigitizerEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] <= 0xc0) {
        // Extract the X and Y position
        int penX = (data[3] << 8) + data[2];
        int penY = (data[5] << 8) + data[4];

        if (dataLen == 12) {
            // This is currently only used by the Artist 24 Pro tablet
            penX += data[10] << 16;
        }

        // Check to see if the pen is touching
        std::bitset<8> stylusTipAndButton(data[1]);
        int pressure = (data[7] << 8) + data[6];
        pressure += offsetPressure;

        // std::cout << "Pressure is (" << pressure << ")" << std::endl;

        const bool isInProximity = stylusTipAndButton.test(5) && !stylusTipAndButton.test(4);
        const bool isEraserBit = stylusTipAndButton.test(3);

        const bool hasEraserEnteredProximity = isInProximity && isEraserBit;
        const bool hasPenEnteredProximity = isInProximity && !isEraserBit;
        const bool hasPenExitedProximity = stylusTipAndButton.test(6) && !eraserInProximity;
        const bool hasEraserExitedProximity = stylusTipAndButton.test(6);

        // Handle pen coming into/out of proximity
        if (hasEraserEnteredProximity) {
            handleEraserEnteredProximity(handle);
        } else if (hasPenEnteredProximity) {
            handlePenEnteredProximity(handle);
        } else if (hasPenExitedProximity) {
            handlePenLeftProximity(handle);
        } else if (hasEraserExitedProximity) {
            handleEraserLeftProximity(handle);
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
        if (stylusTipAndButton.test(1)) {
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

void xp_pen_unified_device::handleGenericFrameEvent(
    libusb_device_handle* handle, 
    unsigned char* data, 
    size_t dataLen,
    int buttonByteIndex,
    int dialByteIndex
) {
    if (data[1] >= 0xf0) {
        long button = data[buttonByteIndex];
        // Get the position of the first set bit
        long position = ffsl(button);

        std::bitset<8> dialBits(data[dialByteIndex]);

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
