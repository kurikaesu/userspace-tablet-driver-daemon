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
#include <thread>
#include "huion_handler.h"
#include "device_interface_pair.h"
#include "huion_tablet.h"

huion_handler::huion_handler() {
    std::cout << "huion_handler initialized" << std::endl;

    // Physical product ids
    addHandler(new huion_tablet(0x006e));
    addHandler(new huion_tablet(0x006d));

    // Aliased product ids

    // HUION Tablets
    addHandler(new huion_tablet(0x0188));
    addHandler(new huion_tablet(0x0191));
    addHandler(new huion_tablet(0x0153));
    addHandler(new huion_tablet(0x0200));
    addHandler(new huion_tablet(0x0182));

    // GAOMON Tablets
    addHandler(new huion_tablet(0x0311));
}

huion_handler::~huion_handler() noexcept {

}

int huion_handler::getVendorId() {
    return 0x256c;
}

std::vector<int> huion_handler::getProductIds() {
    return handledProducts;
}

std::string huion_handler::vendorName() {
    return "Huion";
}

void huion_handler::setConfig(nlohmann::json config) {
    for (auto product : productHandlers) {
        auto productString = std::to_string(product.first);
        if (!config.contains(productString) || config[productString] == nullptr) {
            config[productString] = nlohmann::json({});
        }

        product.second->setConfig(config[productString]);
    }

    jsonConfig = config;
}

nlohmann::json huion_handler::getConfig() {
    for (auto product : productHandlers) {
        jsonConfig[std::to_string(product.first)] = product.second->getConfig();
    }

    return jsonConfig;
}

void huion_handler::handleMessages() {

}

std::set<short> huion_handler::getConnectedDevices() {
    std::set<short> connectedDevices;

    // We only want aliased devices
    for (auto devInterface : deviceInterfaceMap) {
        auto aliasedDevices = static_cast<huion_tablet*>(productHandlers[devInterface.second->productId])->getConnectedAliasedDevices();
        connectedDevices.insert(aliasedDevices.begin(), aliasedDevices.end());
    }

    return connectedDevices;
}

bool huion_handler::handleProductAttach(libusb_device* device, const libusb_device_descriptor descriptor) {
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
    }

    std::cout << "Unknown product " << descriptor.idProduct << std::endl;

    return false;
}

void huion_handler::handleProductDetach(libusb_device *device, struct libusb_device_descriptor descriptor) {
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
