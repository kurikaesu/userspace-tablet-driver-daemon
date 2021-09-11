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
#include <filesystem>
#include "event_handler.h"
#include "xp_pen_handler.h"
#include "vendor_handler.h"
#include "usb_devices.h"

bool event_handler::running = true;
event_handler* event_handler::instance = nullptr;

event_handler::event_handler() {
    if (instance != nullptr) {
        throw instance;
    }

    instance = this;
    devices = new usb_devices();

    loadConfiguration();
    addHandler(new xp_pen_handler());
    saveConfiguration();
}

event_handler::~event_handler() {
    for(auto handler : vendorHandlers) {
        driverConfigJson[handler.second->vendorName()] = handler.second->getConfig();
        delete handler.second;
    }

    saveConfiguration();
    delete devices;
}

void event_handler::sigHandler(int signo) {
    if (signo == SIGINT) {
        std::cout << "Caught SIGINT" << std::endl;
        running = false;
    }

    if (signo == SIGTERM) {
        std::cout << "Caught SIGTERM" << std::endl;
        running = false;
    }

    if (signo == SIGHUP) {
        std::cout << "Reloading configuration" << std::endl;
        instance->loadConfiguration();
    }
}

std::string event_handler::getConfigLocation() {
    std::stringstream  configLocation;
    configLocation << getenv("HOME");
    configLocation << "/.local/share/xp_pen_userland";

    return configLocation.str();
}

std::string event_handler::getConfigFileLocation() {
    std::stringstream configLocation;
    configLocation << getConfigLocation();
    configLocation << "/driver.cfg";

    return configLocation.str();
}

void event_handler::loadConfiguration() {
    std::ifstream driverConfig(getConfigFileLocation(), std::ifstream::in);

    try {
        driverConfig >> driverConfigJson;
    } catch (nlohmann::detail::parse_error) {
        std::cout << "No existing config so we will be creating a new one" << std::endl;
    }

    for(auto handler : vendorHandlers) {
        if (!driverConfigJson.contains(handler.second->vendorName()) ||
            driverConfigJson[handler.second->vendorName()] == nullptr) {

            driverConfigJson[handler.second->vendorName()] = nlohmann::json({});
        }

        handler.second->setConfig(driverConfigJson[handler.second->vendorName()]);
    }
}

void event_handler::saveConfiguration() {
    std::filesystem::create_directories(getConfigLocation());

    std::ofstream driverConfig;
    driverConfig.open(getConfigFileLocation(), std::ofstream::out);

    driverConfig << driverConfigJson;
    driverConfig.close();
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
    event_handler* eventHandler = (event_handler*)user_data;
    eventHandler->hotplugEvents.push_back({event, device});
    return 0;
}

int event_handler::run() {
    auto supportedDevices = devices->getCandidateDevices(vendorHandlers);

    std::vector<libusb_hotplug_callback_handle> callbackHandles;
    for (auto vendorProducts : supportedDevices) {
        for (auto product : vendorProducts.second) {
            libusb_hotplug_callback_handle callbackHandle;
            if (libusb_hotplug_register_callback(devices->getContext(),
                                                 static_cast<libusb_hotplug_event>(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
                                                                                   LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
                                                 static_cast<libusb_hotplug_flag>(0), vendorProducts.first, product, LIBUSB_HOTPLUG_MATCH_ANY,
                                                 hotplugCallback, this, &callbackHandle) == LIBUSB_SUCCESS) {
                callbackHandles.push_back(callbackHandle);
            }
        }
    }

    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
    signal(SIGHUP, sigHandler);

    while (running) {
        devices->handleEvents();
        // Handle all new device attach events
        while (hotplugEvents.size() > 0) {
            auto event = hotplugEvents.front();
            if (event.event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
                devices->handleDeviceAttach(vendorHandlers, event.device);
            } else if (event.event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
                devices->handleDeviceDetach(vendorHandlers, event.device);
            }
            hotplugEvents.pop_front();
        }
    }

    std::cout << "Shutting down" << std::endl;

    for (auto callbackHandle : callbackHandles) {
        libusb_hotplug_deregister_callback(NULL, callbackHandle);
    }

    return 0;
}