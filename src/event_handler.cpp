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

#include <csignal>
#include <iostream>
#include <fstream>
#include "event_handler.h"
#include "xp_pen_handler.h"
#include "vendor_handler.h"
#include "usb_devices.h"

bool event_handler::running = true;

event_handler::event_handler() {
    devices = new usb_devices();

    std::ifstream driverConfig("/home/aren/.local/share/xp_pen_userland/driver.cfg", std::ifstream::in);

    try {
        driverConfig >> driverConfigJson;
    } catch (nlohmann::detail::parse_error) {
        std::cout << "No existing config so we will be creating a new one" << std::endl;
    }

    addHandler(new xp_pen_handler());
}

event_handler::~event_handler() {
    for(auto handler : vendorHandlers) {
        driverConfigJson[handler.second->vendorName()] = handler.second->getConfig();
        delete handler.second;
    }

    std::ofstream driverConfig;
    driverConfig.open("/home/aren/.local/share/xp_pen_userland/driver.cfg", std::ofstream::out);

    driverConfig << driverConfigJson;
    driverConfig.close();

    delete devices;
}

void event_handler::sigHandler(int signo) {
    if (signo == SIGINT) {
        std::cout << "Caught SIGINT" << std::endl;
        running = false;
    }
}

void event_handler::addHandler(vendor_handler *handler) {
    vendorHandlers[handler->getVendorId()] = handler;
    if (!driverConfigJson.contains(handler->vendorName()) ||
        driverConfigJson[handler->vendorName()] == nullptr) {

        driverConfigJson[handler->vendorName()] = nlohmann::json({});
    }

    handler->setConfig(driverConfigJson[handler->vendorName()]);
}

int event_handler::hotplugCallback(struct libusb_context* context, struct libusb_device* device,
                    libusb_hotplug_event event, void* user_data) {
    std::cout << "Got hotplug event" << std::endl;
    if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
        event_handler* eventHandler = (event_handler*)user_data;
        eventHandler->devices->handleDeviceAttach(eventHandler->vendorHandlers, device);
    } else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
        event_handler* eventHandler = (event_handler*)user_data;
        eventHandler->devices->handleDeviceDetach(eventHandler->vendorHandlers, device);
    }
    return 0;
}

int event_handler::run() {
    auto supportedDevices = devices->getCandidateDevices(vendorHandlers);

    std::vector<libusb_hotplug_callback_handle> callbackHandles;
    for (auto vendorProducts : supportedDevices) {
        for (auto product : vendorProducts.second) {
            libusb_hotplug_callback_handle callbackHandle;
            if (libusb_hotplug_register_callback(NULL,
                                                 static_cast<libusb_hotplug_event>(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
                                                                                   LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
                                                 static_cast<libusb_hotplug_flag>(0), vendorProducts.first, product, LIBUSB_HOTPLUG_MATCH_ANY,
                                                 hotplugCallback, this, &callbackHandle) == LIBUSB_SUCCESS) {
                callbackHandles.push_back(callbackHandle);
            }
        }
    }

    signal(SIGINT, sigHandler);

    while (running) {
        devices->handleEvents();
    }

    std::cout << "Shutting down" << std::endl;

    for (auto callbackHandle : callbackHandles) {
        libusb_hotplug_deregister_callback(NULL, callbackHandle);
    }

    return 0;
}