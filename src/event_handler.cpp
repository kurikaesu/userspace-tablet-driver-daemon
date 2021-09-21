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

#if __has_include(<filesystem>)
  #include <filesystem>
  namespace filesystem = std::filesystem;
#elif __has_include(<experimental/filesystem>)
  #include <experimental/filesystem>
  namespace filesystem = std::experimental::filesystem;
#else
  error "Missing the <filesystem> header."
#endif


#include <csignal>
#include <iostream>
#include <fstream>
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
    saveConfiguration();
    for(auto handler : vendorHandlers) {
        delete handler.second;
    }

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

    if (!driverConfigJson.contains("deviceConfigurations")) {
        driverConfigJson["deviceConfigurations"] = nlohmann::json({});
    }

    // Upgrade the previous version of the config file if it exists
    if (driverConfigJson.contains("XP-Pen")) {
        driverConfigJson["deviceConfigurations"]["10429"] = nlohmann::json(driverConfigJson["XP-Pen"]);
        driverConfigJson.erase("XP-Pen");
    }

    for(auto handler : vendorHandlers) {
        auto vendorIdString = std::to_string(handler.second->getVendorId());
        if (!driverConfigJson["deviceConfigurations"].contains(vendorIdString) ||
            driverConfigJson["deviceConfigurations"][vendorIdString] == nullptr) {

            driverConfigJson["deviceConfigurations"][vendorIdString] = nlohmann::json({});
        }

        handler.second->setConfig(driverConfigJson["deviceConfigurations"][vendorIdString]);
    }
}

void event_handler::saveConfiguration() {
    for(auto handler : vendorHandlers) {
        auto vendorIdString = std::to_string(handler.second->getVendorId());
        driverConfigJson["deviceConfigurations"][vendorIdString] = handler.second->getConfig();
    }

    filesystem::create_directories(getConfigLocation());

    std::ofstream driverConfig;
    driverConfig.open(getConfigFileLocation(), std::ofstream::out);

    driverConfig << driverConfigJson;
    driverConfig.close();
}

void event_handler::addHandler(vendor_handler *handler) {
    vendorHandlers[handler->getVendorId()] = handler;
    auto vendorIdString = std::to_string(handler->getVendorId());
    if (!driverConfigJson["deviceConfigurations"].contains(vendorIdString) ||
        driverConfigJson["deviceConfigurations"][vendorIdString] == nullptr) {
        
        driverConfigJson["deviceConfigurations"][vendorIdString] = nlohmann::json({});
    }

    handler->setConfig(driverConfigJson["deviceConfigurations"][vendorIdString]);
    handler->setMessageQueue(&messageQueue);
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

        // Handle any new incoming socket communications
        socketServer.handleConnections();
        socketServer.handleMessages(&messageQueue);

        // Have all the vendor handlers process messages
        for (auto handler: vendorHandlers) {
            handler.second->handleMessages();
        }

        // Handle messages directed to the event handler
        handleMessages();

        // Handle any responses to socket comms
        socketServer.handleResponses(&messageQueue);
    }

    std::cout << "Shutting down" << std::endl;

    for (auto callbackHandle : callbackHandles) {
        libusb_hotplug_deregister_callback(NULL, callbackHandle);
    }

    return 0;
}

void event_handler::handleMessages() {
    auto messages = messageQueue.getMessagesFor(message_destination::eventHandler, 0x0000);
    for (auto message : messages) {
        auto response = new unix_socket_message;
        response->destination = message_destination::gui;
        response->vendor = message->vendor;
        response->device = message->device;
        response->interface = message->interface;
        response->length = message->responseLength;
        response->originatingSocket = message->originatingSocket;
        response->signature = socket_server::versionSignature;
        unsigned char* writePointer = nullptr;

        switch (message->device) {
            // Get connected devices
            case 0x0001:
                std::cout << "Handling get connected devices request" << std::endl;
                response->data = new unsigned char[4096];
                memset(response->data, 0, 4096);
                writePointer = response->data;
                for (auto handler: vendorHandlers) {
                    auto devices = handler.second->getConnectedDevices();
                    for (auto device : devices) {
                        memcpy(writePointer, &handler.first, sizeof(handler.first));
                        writePointer+=sizeof(handler.first);
                        memcpy(writePointer, &device, sizeof(device));
                        writePointer+=sizeof(device);
                    }
                }
                response->length = writePointer - response->data;

                messageQueue.addMessage(response);

                break;

            case 0x0002:
                std::cout << "Handling reload configuration request" << std::endl;
                loadConfiguration();

                break;

            default:
                break;
        }
    }
}
