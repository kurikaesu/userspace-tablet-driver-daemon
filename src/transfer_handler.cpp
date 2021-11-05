/*
userspace-tablet-driver-daemon
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

#include <linux/uinput.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include "transfer_handler.h"
#include "socket_server.h"

transfer_handler::transfer_handler() {
    penInProximity = false;
    penWasDown = false;
    stylusButtonPressed = 0;
}

transfer_handler::~transfer_handler() {
    for (auto pen : uinputPens) {
        destroy_uinput_device(pen.second);
    }

    for (auto pad : uinputPads) {
        destroy_uinput_device(pad.second);
    }
}

std::vector<int> transfer_handler::handledProductIds() {
    return productIds;
}

nlohmann::json transfer_handler::getConfig() {
    return jsonConfig;
}

bool transfer_handler::uinput_send(int fd, uint16_t type, uint16_t code, int32_t value) {
    struct timeval timestamp;
    gettimeofday(&timestamp, NULL);
    struct input_event event = {
            .time = timestamp,
            .type = type,
            .code = code,
            .value = value
    };
    if (write(fd, &event, sizeof(event)) < 0) {
        return false;
    }

    return true;
}

void transfer_handler::detachDevice(libusb_device_handle *handle) {
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

std::vector<unix_socket_message*> transfer_handler::handleMessage(unix_socket_message *message) {
    std::vector<unix_socket_message*> responses;

    for (auto pens : uinputPens) {
        int sentBytes;
        int ret = libusb_interrupt_transfer(pens.first, message->interface | LIBUSB_ENDPOINT_OUT, message->data, message->length, &sentBytes, 1000);
        if (ret != LIBUSB_SUCCESS) {
            std::cout << "Failed to send message on interface " << message->interface << " ret: " << ret << " errno: " << errno << std::endl;
            return std::vector<unix_socket_message*>();
        }

        if (sentBytes != message->length) {
            std::cout << "Didn't send all of the message on interface " << message->interface << " only sent " << sentBytes << std::endl;
            return std::vector<unix_socket_message*>();
        }

        if (message->expectResponse) {
            unix_socket_message* response = new unix_socket_message();
            response->destination = message_destination::gui;
            response->vendor = message->vendor;
            response->device = message->device;
            response->interface = message->interface;
            response->length = message->responseLength;
            response->originatingSocket = message->originatingSocket;
            response->signature = socket_server::versionSignature;
            response->data = new unsigned char[response->length];
            int actual_length;
            int ret = libusb_interrupt_transfer(pens.first, message->responseInterface | LIBUSB_ENDPOINT_IN, response->data, response->length, &actual_length, 1000);
            if (ret != LIBUSB_SUCCESS) {
                std::cout << "Could not receive response on interface " << message->responseInterface << " ret: " << ret << " errno: " << errno << std::endl;
                delete[] response->data;
                delete response;
            } else {
                if (actual_length != message->responseLength) {
                    std::cout << "Got a response of " << actual_length << " bytes. Expected " << message->responseLength
                              << std::endl;
                } else {
                    responses.push_back(response);
                }
            }
        }
    }

    return responses;
}

int transfer_handler::create_pen(const uinput_pen_args& penArgs) {
    int fd = -1;
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        std::cout << "Could not create uinput pen" << std::endl;
        return false;
    }

    auto set_evbit = [&fd](int evBit) {
        ioctl(fd, UI_SET_EVBIT, evBit);
    };

    auto set_keybit = [&fd](int evBit) {
        ioctl(fd, UI_SET_KEYBIT, evBit);
    };

    auto set_absbit = [&fd](int evBit) {
        ioctl(fd, UI_SET_ABSBIT, evBit);
    };

    auto set_relbit = [&fd](int evBit) {
        ioctl(fd, UI_SET_RELBIT, evBit);
    };

    auto set_mscbit = [&fd](int evBit) {
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
    set_absbit(ABS_PRESSURE);
    set_absbit(ABS_TILT_X);
    set_absbit(ABS_TILT_Y);

    set_relbit(REL_WHEEL);

    set_mscbit(MSC_SERIAL);

    // Setup X Axis
    struct uinput_abs_setup uinput_abs_setup = (struct uinput_abs_setup) {
            .code = ABS_X,
            .absinfo = {
                    .value = 0,
                    .minimum = 0,
                    .maximum = penArgs.maxWidth,
                    .fuzz = 0,
                    .flat = 0,
                    .resolution = penArgs.resolution
            },
    };

    ioctl(fd, UI_ABS_SETUP, &uinput_abs_setup);

    // Setup Y Axis
    uinput_abs_setup = (struct uinput_abs_setup) {
            .code = ABS_Y,
            .absinfo = {
                    .value = 0,
                    .minimum = 0,
                    .maximum = penArgs.maxHeight,
                    .fuzz = 0,
                    .flat = 0,
                    .resolution = penArgs.resolution
            },
    };

    ioctl(fd, UI_ABS_SETUP, &uinput_abs_setup);

    // Setup Pressure Axis
    uinput_abs_setup = (struct uinput_abs_setup) {
            .code = ABS_PRESSURE,
            .absinfo = {
                    .value = 0,
                    .minimum = 0,
                    .maximum = penArgs.maxPressure,
            },
    };

    ioctl(fd, UI_ABS_SETUP, &uinput_abs_setup);

    /* Setup tilt X axis */
    uinput_abs_setup = (struct uinput_abs_setup){
            .code = ABS_TILT_X,
            .absinfo = {
                    .value = 0,
                    .minimum = -penArgs.maxTiltX,
                    .maximum = penArgs.maxTiltX,
            },
    };

    ioctl(fd, UI_ABS_SETUP, &uinput_abs_setup);

    /* Setup tilt Y axis */
    uinput_abs_setup = (struct uinput_abs_setup){
            .code = ABS_TILT_Y,
            .absinfo = {
                    .value = 0,
                    .minimum = -penArgs.maxTiltY,
                    .maximum = penArgs.maxTiltY,
            },
    };

    ioctl(fd, UI_ABS_SETUP, &uinput_abs_setup);

    struct uinput_setup uinput_setup = (struct uinput_setup) {
        .id = {
            .bustype = BUS_USB,
            .vendor = penArgs.vendorId,
            .product = penArgs.productId,
            .version = penArgs.versionId,
        },
    };

    memcpy(uinput_setup.name, penArgs.productName, UINPUT_MAX_NAME_SIZE);

    ioctl(fd, UI_DEV_SETUP, &uinput_setup);
    ioctl(fd, UI_DEV_CREATE);

    return fd;
}

int transfer_handler::create_pad(const uinput_pad_args& padArgs) {
    int fd = -1;
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        std::cout << "Could not create uinput pad" << std::endl;
        return false;
    }

    auto set_evbit = [&fd](int evBit) {
        ioctl(fd, UI_SET_EVBIT, evBit);
    };

    auto set_keybit = [&fd](int evBit) {
        ioctl(fd, UI_SET_KEYBIT, evBit);
    };

    auto set_absbit = [&fd](int evBit) {
        ioctl(fd, UI_SET_ABSBIT, evBit);
    };

    auto set_relbit = [&fd](int evBit) {
        ioctl(fd, UI_SET_RELBIT, evBit);
    };

    auto set_mscbit = [&fd](int evBit) {
        ioctl(fd, UI_SET_MSCBIT, evBit);
    };

    set_evbit(EV_SYN);
    set_evbit(EV_KEY);
    set_evbit(EV_ABS);
    set_evbit(EV_REL);

    // This is for all of the pad buttons
    for (int index = 0; index < padArgs.padButtonAliases.size(); ++index) {
        set_keybit(padArgs.padButtonAliases[index]);
    }

    // But we also send through all the keys since they can be mapped
    for (int index = KEY_RESERVED; index <= KEY_MICMUTE; ++index) {
        set_keybit(index);
    }

    set_relbit(REL_X);
    set_relbit(REL_Y);

    if (padArgs.hasWheel) {
        set_relbit(REL_WHEEL);

        // Setup relative wheel
        struct uinput_abs_setup uinput_abs_setup = (struct uinput_abs_setup) {
            .code = REL_WHEEL,
            .absinfo = {
                    .value = 0,
                    .minimum = -padArgs.wheelMax,
                    .maximum = padArgs.wheelMax,
            },
        };

        ioctl(fd, UI_ABS_SETUP, uinput_abs_setup);
    }

    if (padArgs.hasHWheel) {
        set_relbit(REL_HWHEEL);

        // Setup relative hwheel
        struct uinput_abs_setup uinput_abs_setup = (struct uinput_abs_setup) {
                .code = REL_HWHEEL,
                .absinfo = {
                        .value = 0,
                        .minimum = -padArgs.hWheelMax,
                        .maximum = padArgs.hWheelMax,
                },
        };

        ioctl(fd, UI_ABS_SETUP, uinput_abs_setup);
    }

    struct uinput_setup uinput_setup = (struct uinput_setup) {
        .id ={
                .bustype = BUS_USB,
                .vendor = padArgs.vendorId,
                .product = padArgs.productId,
                .version = padArgs.versionId,
        },
    };

    memcpy(uinput_setup.name, padArgs.productName, UINPUT_MAX_NAME_SIZE);

    ioctl(fd, UI_DEV_SETUP, &uinput_setup);
    ioctl(fd, UI_DEV_CREATE);

    return fd;
}

int transfer_handler::create_pointer(const uinput_pointer_args &pointerArgs) {
    int fd = -1;
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        std::cout << "Could not create uinput pointer" << std::endl;
        return false;
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(fd, UI_SET_EVBIT, EV_REL);
    ioctl(fd, UI_SET_RELBIT, REL_X);
    ioctl(fd, UI_SET_RELBIT, REL_Y);
    ioctl(fd, UI_SET_RELBIT, REL_WHEEL);

    // Setup relative wheel
    struct uinput_abs_setup uinput_abs_setup = (struct uinput_abs_setup) {
            .code = REL_WHEEL,
            .absinfo = {
                    .value = 0,
                    .minimum = -pointerArgs.wheelMax,
                    .maximum = pointerArgs.wheelMax,
            },
    };

    ioctl(fd, UI_ABS_SETUP, uinput_abs_setup);

    struct uinput_setup uinput_setup = (struct uinput_setup) {
            .id ={
                    .bustype = BUS_USB,
                    .vendor = pointerArgs.vendorId,
                    .product = pointerArgs.productId,
                    .version = pointerArgs.versionId,
            },
    };

    memcpy(uinput_setup.name, pointerArgs.productName, UINPUT_MAX_NAME_SIZE);

    ioctl(fd, UI_DEV_SETUP, &uinput_setup);
    ioctl(fd, UI_DEV_CREATE);

    return fd;
}

void transfer_handler::destroy_uinput_device(int fd) {
    ioctl(fd, UI_DEV_DESTROY);
}

void transfer_handler::submitMapping(const nlohmann::json& config) {
    std::vector<aliased_input_event> scanCodes;
    for (auto mapping : config["mapping"].items()) {
        if (mapping.key() == "stylus_buttons") {
            for (auto mappingStylusButtons : mapping.value().items()) {
                for (auto events : mappingStylusButtons.value().items()) {
                    for (auto codes: events.value().items()) {
                        aliased_input_event newEvent{
                                std::atoi(events.key().c_str()),
                                codes.value()
                        };
                        scanCodes.push_back(newEvent);
                    }
                }
                stylusButtonMapping.setStylusButtonMap(std::atoi(mappingStylusButtons.key().c_str()), scanCodes);
                scanCodes.clear();
            }
        } else if (mapping.key() == "buttons") {
            for (auto mappingButtons : mapping.value().items()) {
                for (auto events : mappingButtons.value().items()) {
                    for (auto codes: events.value().items()) {
                        aliased_input_event newEvent{
                                std::atoi(events.key().c_str()),
                                codes.value()
                        };
                        scanCodes.push_back(newEvent);
                    }
                }
                padMapping.setPadMap(std::atoi(mappingButtons.key().c_str()), scanCodes);
                scanCodes.clear();
            }
        } else if (mapping.key() == "dials") {
            for (auto mappingDials : mapping.value().items()) {
                for (auto interceptValues : mappingDials.value().items()) {
                    for (auto events : interceptValues.value().items()) {
                        for (auto codes: events.value().items()) {
                            aliased_input_event newEvent{
                                    std::atoi(events.key().c_str()),
                                    codes.value()
                            };
                            if (newEvent.event_type == EV_KEY){
                                newEvent.event_data = 1;
                            }
                            scanCodes.push_back(newEvent);
                        }
                    }
                    dialMapping.setDialMap(std::atoi(mappingDials.key().c_str()), interceptValues.key(), scanCodes);
                    scanCodes.clear();
                }
            }
        }
    }

    // Handle pressure configuration
    pressureCurve.clear();
    if (config.contains("pressure_curve")) {
        for (auto curvePoints: config["pressure_curve"].items()) {
            pressureCurve.emplace_back(
                    std::pair(
                            curvePoints.value().at(0),
                            curvePoints.value().at(1)
                    ));
        }
    }

    if (pressureCurve.empty()) {
        pressureCurve.emplace_back(std::pair(0, 0));
        pressureCurve.emplace_back(std::pair(100, 100));
    }
}

void transfer_handler::handlePenEnteredProximity(libusb_device_handle* handle) {
    if (!penInProximity) {
        uinput_send(uinputPens[handle], EV_KEY, BTN_TOOL_PEN, 1);
        penInProximity = true;
    }
}

void transfer_handler::handlePenLeftProximity(libusb_device_handle* handle) {
    uinput_send(uinputPens[handle], EV_KEY, BTN_TOOL_PEN, 0);
    penInProximity = false;
}

void transfer_handler::handlePenTouchingDigitizer(libusb_device_handle *handle, int pressure) {
    uinput_send(uinputPens[handle], EV_ABS, ABS_PRESSURE, pressure);
}

void transfer_handler::handleStylusButtonsPressed(libusb_device_handle *handle, int stylusButton) {
    auto stylusButtonMap = stylusButtonMapping.getStylusButtonMap(stylusButton);
    if (!stylusButtonMap.empty()) {
        for (auto sbMap: stylusButtonMap) {
            uinput_send(uinputPads[handle], sbMap.event_type, sbMap.event_value, 1);
        }
        uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);
    } else {
        uinput_send(uinputPens[handle], EV_KEY, stylusButton, 1);
    }

    stylusButtonPressed = stylusButton;
}

void transfer_handler::handleStylusButtonUnpressed(libusb_device_handle *handle) {
    auto stylusButtonMap = stylusButtonMapping.getStylusButtonMap(stylusButtonPressed);
    if (!stylusButtonMap.empty()) {
        for (auto sbMap: stylusButtonMap) {
            uinput_send(uinputPads[handle], sbMap.event_type, sbMap.event_value, 0);
        }
        uinput_send(uinputPads[handle], EV_SYN, SYN_REPORT, 1);
    } else {
        uinput_send(uinputPens[handle], EV_KEY, stylusButtonPressed, 0);
    }

    stylusButtonPressed = 0;
}

void transfer_handler::handleCoordsAndTilt(libusb_device_handle *handle, int penX, int penY, short tiltX, short tiltY) {
    handleCoords(handle, penX, penY);
    uinput_send(uinputPens[handle], EV_ABS, ABS_TILT_X, tiltX);
    uinput_send(uinputPens[handle], EV_ABS, ABS_TILT_Y, tiltY);
}

void transfer_handler::handleCoords(libusb_device_handle *handle, int penX, int penY) {
    uinput_send(uinputPens[handle], EV_ABS, ABS_X, penX);
    uinput_send(uinputPens[handle], EV_ABS, ABS_Y, penY);
}

void transfer_handler::handlePadButtonPressed(libusb_device_handle *handle, int button) {
    auto padMap = padMapping.getPadMap(padButtonAliases[button - 1]);
    for (auto pmap : padMap) {
        uinput_send(uinputPads[handle], pmap.event_type, pmap.event_value, 1);
    }
    lastPressedButton[handle] = button;
}

void transfer_handler::handlePadButtonUnpressed(libusb_device_handle *handle) {
    if (lastPressedButton.find(handle) != lastPressedButton.end() && lastPressedButton[handle] > 0) {
        auto padMap = padMapping.getPadMap(padButtonAliases[lastPressedButton[handle] - 1]);
        for (auto pmap : padMap) {
            uinput_send(uinputPads[handle], pmap.event_type, pmap.event_value, 0);
        }
        lastPressedButton[handle] = -1;
    }
}

void transfer_handler::handleDialEvent(libusb_device_handle* handle, int dial, short value) {
    bool send_reset = false;
    auto dialMap = dialMapping.getDialMap(EV_REL, REL_WHEEL, value);
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
}

float transfer_handler::getPoint(float n1, float n2, float dt) {
    float diff = n2 - n1;

    return n1 + (diff * dt);
}

int transfer_handler::applyPressureCurve(int pressure) {
    if (pressureCurve.empty() || pressure == 0) {
        return pressure;
    }

    // Normalize the pressure first
    float normalizedPressure = (float)pressure / maxPressure;

    float adjustedPressure;
    auto controlPointCount = pressureCurve.size();

    // Single control point is invalid
    switch (controlPointCount) {
    case 2:
        adjustedPressure = (pressureCurve[1].second - pressureCurve[0].second) * normalizedPressure;
        break;
    case 3: {
        auto y1 = getPoint(pressureCurve[0].second, pressureCurve[1].second, normalizedPressure);
        auto y2 = getPoint(pressureCurve[1].second, pressureCurve[2].second, normalizedPressure);
        adjustedPressure = getPoint(y1, y2, normalizedPressure);
        break;
    }

    case 4: {
        auto y1 = getPoint(pressureCurve[0].second, pressureCurve[1].second, normalizedPressure);
        auto y2 = getPoint(pressureCurve[1].second, pressureCurve[2].second, normalizedPressure);
        auto y3 = getPoint(pressureCurve[2].second, pressureCurve[3].second, normalizedPressure);

        auto m1 = getPoint(y1, y2, normalizedPressure);
        auto m2 = getPoint(y2, y3, normalizedPressure);

        adjustedPressure = getPoint(m1, m2, normalizedPressure);
        break;
    }

    default:
        return pressure;
    }

    auto returnedPressure = (adjustedPressure / 100.0f) * maxPressure;
    return (int)returnedPressure;
}
