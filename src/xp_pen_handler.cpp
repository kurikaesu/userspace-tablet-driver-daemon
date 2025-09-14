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

#include <iostream>
#include <algorithm>
#include <thread>
#include <set>
#include "xp_pen_handler.h"
#include "artist_22r_pro.h"
#include "artist_22e_pro.h"
#include "artist_16_pro.h"
#include "artist_pro_16tp.h"
#include "artist_pro_16.h"
#include "artist_13_3_pro.h"
#include "artist_24_pro.h"
#include "artist_12_pro.h"
#include "deco_pro_small.h"
#include "deco_pro_medium.h"
#include "deco_pro_medium_wireless.h"
#include "deco_01v2.h"
#include "star_g430s.h"
#include "star_g640.h"
#include "ac19.h"
#include "artist_12.h"
#include "deco_03.h"
#include "deco_mini7.h"
#include "innovator_16.h"
#include "generic_xp_pen_device.h"
#include "artist_15_6_pro.h"
#include "deco_02.h"
#include "deco_large.h"

xp_pen_handler::xp_pen_handler() {
    std::cout << "xp_pen_handler initialized" << std::endl;

    addHandler(new artist_22r_pro());
    addHandler(new artist_22e_pro());
    addHandler(new artist_16_pro());
	addHandler(new artist_pro_16tp());
    addHandler(new artist_pro_16());
    addHandler(new artist_13_3_pro());
    addHandler(new artist_15_6_pro());
    addHandler(new artist_24_pro());
    addHandler(new artist_12_pro());
    addHandler(new artist_12());
    addHandler(new innovator_16());
    addHandler(new deco_pro_small());
    addHandler(new deco_pro_medium());
    addHandler(new deco_pro_medium_wireless());
    addHandler(new deco_01v2());
    addHandler(new deco_03());
    addHandler(new deco_mini7());
    addHandler(new star_g430s());
    addHandler(new star_g640());
    addHandler(new ac19());
    addHandler(new deco_02());
    addHandler(new deco_large());
}

int xp_pen_handler::getVendorId() {
    return 0x28bd;
}

std::vector<int> xp_pen_handler::getProductIds() {
    return handledProducts;
}

std::string xp_pen_handler::vendorName() {
    return "XP-Pen";
}

void xp_pen_handler::setConfig(nlohmann::json config) {
    for (auto product : productHandlers) {
        auto productString = std::to_string(product.first);
        if (!config.contains(productString) || config[productString] == nullptr) {
            config[productString] = nlohmann::json({});
        }

        product.second->setConfig(config[productString]);
    }

    jsonConfig = config;
}

nlohmann::json xp_pen_handler::getConfig() {
    for (auto product : productHandlers) {
        jsonConfig[std::to_string(product.first)] = product.second->getConfig();
    }

    return jsonConfig;
}

void xp_pen_handler::handleMessages() {
    auto messages = messageQueue->getMessagesFor(message_destination::driver, getVendorId());
    size_t handledMessages = 0;
    size_t totalMessages = messages.size();

    if (totalMessages > 0) {
        // Cancel transfers first
        for (auto transfer: libusbTransfers) {
            libusb_cancel_transfer(transfer);
        }

        libusbTransfers.clear();

        for (auto message: messages) {
            auto handler = productHandlers.find(message->device);
            if (handler != productHandlers.end()) {
                auto responses = handler->second->handleMessage(message);
                delete message;

                for (auto response: responses) {
                    messageQueue->addMessage(response);
                }

                handledMessages++;
            }
        }

        // Re-enable transfers
        for (auto setupData: transfersSetUp) {
            setupTransfers(setupData.handle, setupData.interface_number, setupData.maxPacketSize, setupData.productId);
        }

        std::cout << "Handled " << handledMessages << " out of " << totalMessages << " messages." << std::endl;
    }
}

std::set<short> xp_pen_handler::getConnectedDevices() {
    std::set<short> connectedDevices;

    for (auto device : deviceInterfaceMap) {
        connectedDevices.insert(device.second->productId);
    }

    return connectedDevices;
}

bool xp_pen_handler::handleProductAttach(libusb_device* device, const libusb_device_descriptor descriptor) {
    libusb_device_handle* handle = NULL;
    device_interface_pair* interfacePair = nullptr;
    const int maxRetries = 5;
    int currentAttept = 0;

    if (std::find(handledProducts.begin(), handledProducts.end(), descriptor.idProduct) != handledProducts.end()) {
        std::cout << "Handling " << productHandlers[descriptor.idProduct]->getProductName(descriptor.idProduct) << std::endl;
        while (interfacePair == nullptr  && currentAttept < maxRetries) {
            interfacePair = claimDevice(device, handle, descriptor);
            if (interfacePair == nullptr) {
                std::cout << "Could not claim device on attempt " << currentAttept << ". Detaching and then waiting" << std::endl;
                handleProductDetach(device, descriptor);
                std::this_thread::sleep_for(std::chrono::seconds(1));
                ++currentAttept;
            }
        }

        if (interfacePair != nullptr) {
            deviceInterfaces.push_back(interfacePair);
            deviceInterfaceMap[device] = interfacePair;
            return true;
        }

        std::cout << "Giving up" << std::endl;
        return false;
    } else {
        // We will attempt a generic handler instead
        auto genericHandler = new generic_xp_pen_device(descriptor.idProduct);
        addHandler(genericHandler);
        while (interfacePair == nullptr && currentAttept < maxRetries) {
            interfacePair = claimDevice(device, handle, descriptor);
            if (interfacePair == nullptr) {
                std::cout << "Could not claim device on attempt " << currentAttept << ". Detaching and then waiting" << std::endl;
                handleProductDetach(device, descriptor);
                std::this_thread::sleep_for(std::chrono::seconds(1));
                ++currentAttept;
            }
        }
    }

    std::cout << "Unknown product " << descriptor.idProduct << std::endl;

    return false;
}

void xp_pen_handler::handleProductDetach(libusb_device *device, struct libusb_device_descriptor descriptor) {
    for (auto deviceObj : deviceInterfaceMap) {
        if (deviceObj.first == device) {
            std::cout << "Handling device detach" << std::endl;

            if (productHandlers.find(descriptor.idProduct) != productHandlers.end()) {
                productHandlers[descriptor.idProduct]->detachDevice(deviceObj.second->deviceHandle);
            }

            cleanupDevice(deviceObj.second);
            libusb_close(deviceObj.second->deviceHandle);

            auto deviceInterfacesIterator = std::find(deviceInterfaces.begin(), deviceInterfaces.end(), deviceObj.second);
            if (deviceInterfacesIterator != deviceInterfaces.end()) {
                deviceInterfaces.erase(deviceInterfacesIterator);
            }

            auto deviceMapIterator = std::find(deviceInterfaceMap.begin(), deviceInterfaceMap.end(), deviceObj);
            if (deviceMapIterator != deviceInterfaceMap.end()) {
                deviceInterfaceMap.erase(deviceMapIterator);
            }

            break;
        }
    }
}

void xp_pen_handler::sendInitKey(libusb_device_handle *handle, int interface_number, transfer_handler* productHandler) {
    std::cout << "Sending init key on endpont " << interface_number << std::endl;

    std::string key = productHandler->getInitKey();
    int sentBytes;
    int ret = libusb_interrupt_transfer(handle, interface_number | LIBUSB_ENDPOINT_OUT, (unsigned char *) key.c_str(), key.length(), &sentBytes, 1000);
    if (ret != LIBUSB_SUCCESS) {
        std::cout << "Failed to send key on interface " << interface_number << " ret: " << ret << " errno: " << errno << std::endl;
        return;
    }

    if (sentBytes != key.length()) {
        std::cout << "Didn't send all of the key on interface " << interface_number << " only sent " << sentBytes << std::endl;
        return;
    }
}
