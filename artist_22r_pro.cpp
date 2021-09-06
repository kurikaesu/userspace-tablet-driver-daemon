//
// Created by aren on 9/5/21.
//

#include <iostream>
#include <iomanip>
#include <cstring>
#include "artist_22r_pro.h"

artist_22r_pro::artist_22r_pro() {
    productIds.push_back(0x091b);
}

std::vector<int> artist_22r_pro::handledProductIds() {
    return productIds;
}

int artist_22r_pro::sendInitKeyOnInterface() {
    return 0x03;
}

bool artist_22r_pro::handleTransferData(libusb_device_handle* handle, unsigned char *data, size_t dataLen) {
    for (size_t i = 0; i < dataLen; ++i) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)data[i] << " ";
    }

    std::cout << std::endl;

    switch (data[0]) {
    // Unified interface
    case 0x02:
        // Stylus / digitizer events
        if (data[1] < 0xb0) {
            std::cout << "Got pen movement" << std::endl;

            // Extract the X and Y position
            int penX = (data[3] << 8) + data[2];
            int penY = (data[5] << 8) + data[4];

            // Check to see if the pen is touching
            int pressure = 0;
            if (0x01 & data[1]) {
                std::cout << "Stylus touching" << std::endl;

                // Grab the pressure amount
                pressure = (data[7] << 8) + data[6];
            }

            // Grab the tilt values
            short tiltx = (char)data[8];
            short tilty = (char)data[9];

            int buttonPressed = 0;
            // Check to see if the stylus buttons are being pressed
            if (0x02 & data[1]) {
                buttonPressed = 1;
            } else if (0x04 & data[1]) {
                buttonPressed = 2;
            }

            std::cout << std::dec << "X: " << penX << " Y: " << penY <<
                " tilt-x: " << tiltx << " tilt-y: " << tilty <<
                " pressure: " << pressure << " button_pressed: " << buttonPressed << std::endl;
        }

        // Frame keys / dials events
        if (data[1] >= 0xb0) {
            std::cout << "Got frame events" << std::endl;

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
                std::cout << "Left dial: " << leftDialValue << std::endl;
                dialEvent = true;
            } else if (rightDialValue != 0) {
                std::cout << "Right dial: " << rightDialValue << std::endl;
                dialEvent = true;
            }

            if (button != 0) {
                std::cout << "Button " << position << " pressed" << std::endl;
                lastPressedButton[handle] = position;
            } else if (!dialEvent) {
                if (lastPressedButton.find(handle) != lastPressedButton.end() && lastPressedButton[handle] > 0) {
                    std::cout << "Button " << lastPressedButton[handle] << " released" << std::endl;
                    lastPressedButton[handle] = -1;
                } else {
                    std::cout << "Got a phantom button up event" << std::endl;
                }
            }
        }

        break;

    default:
        break;
    }

    return true;
}
