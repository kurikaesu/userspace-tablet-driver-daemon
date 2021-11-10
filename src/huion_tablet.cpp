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

huion_tablet::huion_tablet(int productId) {
    productIds.push_back(productId);

    for (int currentAssignedButton = BTN_0; currentAssignedButton <= BTN_9; ++currentAssignedButton) {
        padButtonAliases.push_back(currentAssignedButton);
    }

    for (int currentAssignedButton = BTN_A; currentAssignedButton <= BTN_SELECT; ++currentAssignedButton) {
        padButtonAliases.push_back(currentAssignedButton);
    }
}

std::string huion_tablet::getProductName(int productId) {
    if (productId == 0x006e || productId == 0x006d) {
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

        // Mapping the touch-strips
        addToDialMap(REL_WHEEL, -1, EV_KEY, {KEY_LEFTCTRL, KEY_MINUS});
        addToDialMap(REL_WHEEL, 1, EV_KEY, {KEY_LEFTCTRL, KEY_EQUAL});
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

std::string huion_tablet::getDeviceNameFromFirmware(std::wstring firmwareName) {
    // HUION tablets
    if (firmwareName == L"HUION_T188_180718") {
        return "Huion WH1409 v2";
    } else if (firmwareName == L"HUION_T191_190619") {
        return "Huion H1161";
    } else if (firmwareName == L"HUION_T153_160524") {
        return "Huion WH1409 (2048)";
    } else if (firmwareName == L"HUION_T200_210309") {
        return "Huion KD100 mini Keydial";
    } else if (firmwareName == L"HUION_M182_200605") {
        return "Huion Kamvas Pro 13";
    }

    // GAOMON tablets
    else if (firmwareName == L"OEM02_T19n_200311") {
        return "Gaomon M10K Pro";
    }

    return "Unknown device";
}

int huion_tablet::getAliasedDeviceIdFromFirmware(std::wstring firmwareName) {
    // HUION tablets
    if (firmwareName == L"HUION_T188_180718") {
        return 0x0188;
    } else if (firmwareName == L"HUION_T191_190619") {
        return 0x0191;
    } else if (firmwareName == L"HUION_T153_160524") {
        return 0x0153;
    } else if (firmwareName == L"HUION_T200_210309") {
        return 0x0200;
    } else if (firmwareName == L"HUION_M182_200605") {
        return 0x0182;
    }

    // GAOMON tablets
    else if (firmwareName == L"OEM02_T19n_200311") {
        return 0x0311;
    }

    return 0x0000;
}

int huion_tablet::getAliasedProductId(libusb_device_handle *handle, int originalId) {
    auto firmware = getDeviceFirmwareName(handle);
    auto productId = getAliasedDeviceIdFromFirmware(firmware);
    if (productId == 0x0000) {
        productId = originalId;
    }

    return productId;
}

std::set<int> huion_tablet::getConnectedAliasedDevices() {
    std::set<int> connectedAliases;
    for (auto pair : handleToAliasedDeviceId) {
        connectedAliases.insert(pair.second);
        std::cout << "Connected device: " << pair.second << std::endl;
    }

    return connectedAliases;
}

std::wstring huion_tablet::getDeviceFirmwareName(libusb_device_handle *handle) {
    auto *buffer = new unsigned char[200];
    memset(buffer, 0, 200);

    // Extract the firmware name
    int descriptorLength = libusb_get_string_descriptor(handle, 0xc9, 0x0409, buffer, 130);
    if (descriptorLength < 36) {
        std::cout << "Could not get firmware descriptor. Returned descriptor length was " << descriptorLength
                  << std::endl;
        delete[] buffer;
        return L"";
    }

    if (buffer[1] != 0x03) {
        std::cout << "Descriptor response wasn't a string" << std::endl;
    }
    int dataLength = buffer[0];
    auto *firmwareName = new wchar_t[dataLength];
    wmemset(firmwareName, 0, dataLength);
    for (int i = 0, j = 2; i < descriptorLength; ++i, j += 2) {
        firmwareName[i] = (buffer[j + 1] << 8) + buffer[j];
    }

    std::wstring firmware((wchar_t *) firmwareName);
    delete[] firmwareName;
    delete [] buffer;

    return firmware;
}

bool huion_tablet::attachDevice(libusb_device_handle *handle, int interfaceId) {
    auto *buffer = new unsigned char[200];
    memset(buffer, 0, 200);
    auto firmware = getDeviceFirmwareName(handle);
    std::wcout << "Got firmware " << firmware << std::endl;

    std::string deviceName = getDeviceNameFromFirmware(firmware);
    std::cout << "Resolved device name to " << deviceName << std::endl;
    // Store the device name relationship to the handle
    handleToDeviceName[handle] = deviceName;
    handleToAliasedDeviceId[handle] = getAliasedDeviceIdFromFirmware(firmware);

    // We need to get a few more bits of information
    if (libusb_get_string_descriptor(handle, 200, 0x0409, buffer, 32) < 18) {
        std::cout << "Could not get descriptor" << std::endl;
        // Let's see which descriptors are actually available
        for (int i = 1; i < 0xff; ++i) {
            memset(buffer, 0, 12);
            int stringLength = libusb_get_string_descriptor(handle, i, 0x0409, buffer, 12);
            if (stringLength < 0) {
                std::cout << "Could not get descriptor on index " << i << std::endl;
            } else {
                std::cout << "Descriptor " << i << " has length " << stringLength << std::endl;
            }
        }

        delete[] buffer;
        return false;
    }

    int maxWidth = (buffer[4] << 16) + (buffer[3] << 8) + buffer[2];
    int maxHeight = (buffer[7] << 16) + (buffer[6] << 8) + buffer[5];
    maxPressure = (buffer[9] << 8) + buffer[8];
    int resolution = (buffer[11] << 8) + buffer[10];

    unsigned short vendorId = 0x256c;
    unsigned short productId = 0xf06e;
    unsigned short versionId = 0x0001;

    if (maxWidth != 1 && maxHeight != 1 && maxPressure != 0) {
        std::cout << deviceName << " configured with max-width: " << maxWidth << " max-height: " << maxHeight
                  << " max-pressure: " << maxPressure << std::endl;

        struct uinput_pen_args penArgs{
                .maxWidth = maxWidth,
                .maxHeight = maxHeight,
                .maxPressure = maxPressure,
                .resolution = resolution,
                .maxTiltX = 60,
                .maxTiltY = 60,
                .vendorId = vendorId,
                .productId = productId,
                .versionId = versionId,
        };

        memset(penArgs.productName, 0, UINPUT_MAX_NAME_SIZE);
        memcpy(penArgs.productName, deviceName.c_str(), deviceName.length());

        uinputPens[handle] = create_pen(penArgs);
    }

    struct uinput_pad_args padArgs{
            .padButtonAliases = padButtonAliases,
            .hasWheel = true,
            .hasHWheel = true,
            .wheelMax = 1,
            .hWheelMax = 1,
            .vendorId = vendorId,
            .productId = productId,
            .versionId = versionId,
    };

    std::stringstream padName;
    padName << deviceName;
    padName << " Pad";
    std::string padNameString = padName.str();

    memset(padArgs.productName, 0, UINPUT_MAX_NAME_SIZE);
    memcpy(padArgs.productName, padNameString.c_str(), padNameString.length());

    uinputPads[handle] = create_pad(padArgs);

    delete[] buffer;

    return true;
}

bool huion_tablet::handleTransferData(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
//    std::cout << std::dec << "Got transfer of data length: " << (int)dataLen << " data: ";
//    for (int i = 0; i < dataLen; ++i) {
//        std::cout << std::hex << std::setfill('0')  << std::setw(2) << (int)data[i] << ":";
//    }
//    std::cout << std::endl;

    switch (data[0]) {
        case 0x07:
            handleDigitizerEventV3(handle, data, dataLen);
            handlePadEventV1(handle, data, dataLen);
            return true;

        case 0x08:
        case 0x0a:
            break;

        default:

            return false;
    }

    switch (data[1]) {
        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83:
        case 0x84:
        case 0x85:
            handleDigitizerEventV2(handle, data, dataLen);
            break;

        case 0xc0:
        case 0xc1:
        case 0xc2:
        case 0xc3:
        case 0xc4:
        case 0xc5:
            handleDigitizerEventV1(handle, data, dataLen);
            break;

        case 0xe0:
            handlePadEventV1(handle, data, dataLen);

            break;

        case 0xf0:
            handleTouchStripEvent(handle, data, dataLen);

            break;

        case 0xf1:
            handleTabletDialEvent(handle, data, dataLen);

        default:
            return false;
    }

    return true;
}

void huion_tablet::handleDigitizerEventV1(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    int penX = (data[3] << 8) + data[2];
    int penY = (data[5] << 8) + data[4];

    std::bitset<sizeof(data)> stylusTipAndButton(data[1]);

    // Check to see if the pen is touching
    int pressure = (data[7] << 8) + data[6];
    if (stylusTipAndButton.test(0)) {
        // Grab the pressure amount
        handlePenEnteredProximity(handle);
        handlePenTouchingDigitizer(handle, pressure);
    } else {
        handlePenTouchingDigitizer(handle, pressure);
    }

    // Check to see if the stylus buttons are being pressed
    if (stylusTipAndButton.test(1)) {
        handleStylusButtonsPressed(handle, BTN_STYLUS);
    } else if (stylusTipAndButton.test(2)) {
        handleStylusButtonsPressed(handle, BTN_STYLUS2);
    } else {
        handleStylusButtonUnpressed(handle);
    }

    handleCoords(handle, penX, penY);

    uinput_send(uinputPens[handle], EV_SYN, SYN_REPORT, 1);
}

void huion_tablet::handleDigitizerEventV2(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    // Extract the X and Y position
    int penX = (data[8] << 16) + (data[3] << 8) + data[2];
    int penY = (data[5] << 8) + data[4];

    // Check to see if the pen is touching
    std::bitset<sizeof(data)> stylusTipAndButton(data[1]);
    int pressure = (data[7] << 8) + data[6];

    if (stylusTipAndButton.test(0)) {
        handlePenEnteredProximity(handle);
        handlePenTouchingDigitizer(handle, pressure);
    } else {
        handlePenTouchingDigitizer(handle, pressure);
    }

    // Grab the tilt values
    short tiltx = (char)data[10];
    short tilty = (char)data[11];

    // Check to see if the stylus buttons are being pressed
    if (stylusTipAndButton.test(1)) {
        handleStylusButtonsPressed(handle, BTN_STYLUS);
    } else if (stylusTipAndButton.test(2)) {
        handleStylusButtonsPressed(handle, BTN_STYLUS2);
    } else {
        handleStylusButtonUnpressed(handle);
    }

    handleCoordsAndTilt(handle, penX, penY, tiltx, tilty);

    uinput_send(uinputPens[handle], EV_SYN, SYN_REPORT, 1);
}

void huion_tablet::handleDigitizerEventV3(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] < 0xa0) {
        // Extract the X and Y position
        int penX = (data[3] << 8) + data[2];
        int penY = (data[5] << 8) + data[4];

        // Check to see if the pen is touching
        std::bitset<sizeof(data)> stylusTipAndButton(data[1]);
        int pressure = (data[7] << 8) + data[6];

        if (stylusTipAndButton.test(0)) {
            handlePenEnteredProximity(handle);
            handlePenTouchingDigitizer(handle, pressure);
        } else {
            handlePenTouchingDigitizer(handle, pressure);
        }

        // Check to see if the stylus buttons are being pressed
        if (stylusTipAndButton.test(1)) {
            handleStylusButtonsPressed(handle, BTN_STYLUS);
        } else if (0x04 & data[1]) {
            handleStylusButtonsPressed(handle, BTN_STYLUS2);
        } else {
            handleStylusButtonUnpressed(handle);
        }

        handleCoords(handle, penX, penY);

        uinput_send(uinputPens[handle], EV_SYN, SYN_REPORT, 1);
    }
}

void huion_tablet::handlePadEventV1(libusb_device_handle* handle, unsigned char* data, size_t dataLen) {
    if (data[1] == 0xe0) {
        // Extract the button being pressed (If there is one)
        long button = (data[6] << 16) +  (data[5] << 8) + data[4];
        // Grab the first bit set in the button long which tells us the button number
        long position = ffsl(button);

        bool shouldSyn = true;
        bool dialEvent = false;

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

void huion_tablet::handleTouchStripEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] == 0xf0) {
        short touchValue = data[5];
        // Check if we let go
        if (touchValue == 0) {
            touchStripLastValue = -1;
            return;
        }

        if (touchValue != touchStripLastValue) {
            int sendValue = 0;
            if (touchStripLastValue == -1) {
                touchStripLastValue = touchValue;
                return;
            }

            if (touchValue < touchStripLastValue) {
                sendValue = -1;
            } else if (touchValue > touchStripLastValue) {
                sendValue = 1;
            }

            if (sendValue != 0) {
                bool send_reset = false;
                auto dialMap = dialMapping.getDialMap(EV_REL, REL_WHEEL, sendValue);
                for (auto dmap : dialMap) {
                    uinput_send(uinputPads[handle], dmap.event_type, dmap.event_value, dmap.event_data);
                    if (dmap.event_type == EV_KEY) {
                        send_reset = true;
                    }
                }

                uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);

                if (send_reset) {
                    for (auto dmap : dialMap) {
                        // We have to handle key presses manually here because this device does not send reset events
                        if (dmap.event_type == EV_KEY) {
                            uinput_send(uinputPads[handle], dmap.event_type, dmap.event_value, 0);
                        }
                    }
                }
                uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);
            }

            touchStripLastValue = touchValue;
        }
    }
}

void huion_tablet::handleTabletDialEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] == 0xf1) {
        short dialValue = 0;
        if (data[5] == 0x01) {
            dialValue = -1;
        } else if (data[5] == 0x02) {
            dialValue = 1;
        }

        if (dialValue != 0) {
            handleDialEvent(handle, REL_WHEEL, dialValue);
        }
    }
}
