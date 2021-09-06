//
// Created by aren on 9/5/21.
//

#include <iostream>
#include <iomanip>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <linux/uinput.h>
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

bool artist_22r_pro::attachDevice(libusb_device_handle *handle) {
    int fd = -1;
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        std::cout << "Could not create uinput pen" << std::endl;
        return false;
    }

    auto set_evbit = [fd](int evBit) {
        ioctl(fd, UI_SET_EVBIT, evBit);
    };

    auto set_keybit = [fd](int evBit) {
        ioctl(fd, UI_SET_KEYBIT, evBit);
    };

    auto set_absbit = [fd](int evBit) {
        ioctl(fd, UI_SET_ABSBIT, evBit);
    };

    auto set_relbit = [fd](int evBit) {
        ioctl(fd, UI_SET_RELBIT, evBit);
    };

    auto set_mscbit = [fd](int evBit) {
        ioctl(fd, UI_SET_MSCBIT, evBit);
    };

    set_evbit(EV_SYN);
    set_evbit(EV_KEY);
    set_evbit(EV_ABS);
    set_evbit(EV_REL);
    set_evbit(EV_MSC);

    set_keybit(BTN_LEFT);
    set_keybit(BTN_RIGHT);
    set_keybit(BTN_MIDDLE);
    set_keybit(BTN_SIDE);
    set_keybit(BTN_EXTRA);
    set_keybit(BTN_TOOL_PEN);
    set_keybit(BTN_TOOL_RUBBER);
    set_keybit(BTN_TOOL_BRUSH);
    set_keybit(BTN_TOOL_PENCIL);
    set_keybit(BTN_TOOL_AIRBRUSH);
    set_keybit(BTN_TOOL_MOUSE);
    set_keybit(BTN_TOOL_LENS);
    set_keybit(BTN_TOUCH);
    set_keybit(BTN_STYLUS);
    set_keybit(BTN_STYLUS2);

    set_absbit(ABS_X);
    set_absbit(ABS_Y);
    set_absbit(ABS_Z);
    set_absbit(ABS_RZ);
    set_absbit(ABS_THROTTLE);
    set_absbit(ABS_WHEEL);
    set_absbit(ABS_PRESSURE);
    set_absbit(ABS_DISTANCE);
    set_absbit(ABS_TILT_X);
    set_absbit(ABS_TILT_Y);
    set_absbit(ABS_MISC);

    set_relbit(REL_WHEEL);

    set_mscbit(MSC_SERIAL);

    // Setup X Axis
    struct uinput_abs_setup uinput_abs_setup = (struct uinput_abs_setup) {
        .code = ABS_X,
        .absinfo = {
                .value = 0,
                .minimum = 0,
                .maximum = 1,
        },
    };

    ioctl(fd, UI_ABS_SETUP, &uinput_abs_setup);

    // Setup Y Axis
    uinput_abs_setup = (struct uinput_abs_setup) {
        .code = ABS_Y,
        .absinfo = {
                .value = 0,
                .minimum = 0,
                .maximum = 1,
        },
    };

    ioctl(fd, UI_ABS_SETUP, &uinput_abs_setup);

    // Setup Pressure Axis
    uinput_abs_setup = (struct uinput_abs_setup) {
            .code = ABS_PRESSURE,
            .absinfo = {
                    .value = 0,
                    .minimum = 0,
                    .maximum = 8191,
            },
    };

    ioctl(fd, UI_ABS_SETUP, &uinput_abs_setup);

    /* Setup tilt X axis */
    uinput_abs_setup = (struct uinput_abs_setup){
            .code = ABS_TILT_X,
            .absinfo = {
                    .value = 0,
                    .minimum = -60,
                    .maximum = 60,
            },
    };

    ioctl(fd, UI_ABS_SETUP, &uinput_abs_setup);

    /* Setup tilt Y axis */
    uinput_abs_setup = (struct uinput_abs_setup){
            .code = ABS_TILT_Y,
            .absinfo = {
                    .value = 0,
                    .minimum = -60,
                    .maximum = 60,
            },
    };

    ioctl(fd, UI_ABS_SETUP, &uinput_abs_setup);

    struct uinput_setup uinput_setup = (struct uinput_setup) {
        .id = {
            .bustype = BUS_USB,
            .vendor = 0x28bd,
            .product = 0xf91b,
            .version = 0x0001,
        },
        {.name = "XP-Pen Artist 22R Pro"},
    };

    ioctl(fd, UI_DEV_SETUP, &uinput_setup);
    ioctl(fd, UI_DEV_CREATE);

    uinputPens[handle] = fd;

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

                uinput_send(uinputPens[handle], EV_KEY, BTN_TOOL_PEN, 1);
                uinput_send(uinputPens[handle], EV_ABS, ABS_PRESSURE, pressure);
            } else {
                uinput_send(uinputPens[handle], EV_KEY, BTN_TOOL_PEN, 0);
            }

            // Grab the tilt values
            short tiltx = (char)data[8];
            short tilty = (char)data[9];

            int buttonPressed = 0;
            // Check to see if the stylus buttons are being pressed
            if (0x02 & data[1]) {
                buttonPressed = 1;
                uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS, 1);
                uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS2, 0);
            } else if (0x04 & data[1]) {
                buttonPressed = 2;
                uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS, 0);
                uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS2, 1);
            } else {
                uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS, 0);
                uinput_send(uinputPens[handle], EV_KEY, BTN_STYLUS2, 0);
            }

            std::cout << std::dec << "X: " << penX << " Y: " << penY <<
                " tilt-x: " << tiltx << " tilt-y: " << tilty <<
                " pressure: " << pressure << " button_pressed: " << buttonPressed << std::endl;

            uinput_send(uinputPens[handle], EV_ABS, ABS_X, penX);
            uinput_send(uinputPens[handle], EV_ABS, ABS_Y, penY);
            uinput_send(uinputPens[handle], EV_ABS, ABS_TILT_X, tiltx);
            uinput_send(uinputPens[handle], EV_ABS, ABS_TILT_Y, tilty);

            uinput_send(uinputPens[handle], EV_SYN, SYN_REPORT, 1);
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
