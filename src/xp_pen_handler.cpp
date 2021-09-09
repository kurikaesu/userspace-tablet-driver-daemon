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

#include <iostream>
#include <algorithm>
#include "xp_pen_handler.h"
#include "transfer_handler_pair.h"
#include "artist_22r_pro.h"
#include "artist_13_3_pro.h"
#include "artist_24_pro.h"

xp_pen_handler::xp_pen_handler() {
    std::cout << "xp_pen_handler initialized" << std::endl;

    addHandler(new artist_22r_pro());
    addHandler(new artist_13_3_pro());
    addHandler(new artist_24_pro());
}

xp_pen_handler::~xp_pen_handler() {
    for (auto deviceInterface : deviceInterfaces) {
        cleanupDevice(deviceInterface);
    }
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

void xp_pen_handler::addHandler(transfer_handler *handler) {
    for (auto productId : handler->handledProductIds()) {
        productHandlers[productId] = handler;
        handledProducts.push_back(productId);
    }
}

bool xp_pen_handler::handleProductAttach(libusb_device* device, const libusb_device_descriptor descriptor) {
    libusb_device_handle* handle = NULL;
    device_interface_pair* interfacePair = NULL;

    if (std::find(handledProducts.begin(), handledProducts.end(), descriptor.idProduct) != handledProducts.end()) {
        std::cout << "Handling " << productHandlers[descriptor.idProduct]->getProductName(descriptor.idProduct) << std::endl;
        interfacePair = claimDevice(device, handle, descriptor);
        deviceInterfaces.push_back(interfacePair);
        deviceInterfaceMap[device] = interfacePair;

        return true;
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

device_interface_pair* xp_pen_handler::claimDevice(libusb_device *device, libusb_device_handle *handle, const libusb_device_descriptor descriptor) {
    device_interface_pair* deviceInterface = new device_interface_pair();
    int err;

    struct libusb_config_descriptor* configDescriptor;
    err = libusb_get_config_descriptor(device, 0, &configDescriptor);
    if (err != LIBUSB_SUCCESS) {
        std::cout << "Could not get config descriptor" << std::endl;
    }

    if ((err = libusb_open(device, &handle)) == LIBUSB_SUCCESS) {
        deviceInterface->deviceHandle = handle;
        unsigned char interfaceCount = configDescriptor->bNumInterfaces;

        for (unsigned char interface_number = 0; interface_number < interfaceCount; ++interface_number) {
            // Skip interfaces with more than 1 alt setting
            if (configDescriptor->interface[interface_number].num_altsetting != 1) {
                continue;
            }

            err = libusb_detach_kernel_driver(handle, interface_number);
            if (LIBUSB_SUCCESS == err) {
                deviceInterface->detachedInterfaces.push_back(interface_number);
            }

            if (libusb_claim_interface(handle, interface_number) == LIBUSB_SUCCESS) {
                deviceInterface->claimedInterfaces.push_back(interface_number);

                // Even though we claim the interface, we only actually care about specific ones. We still do
                // the claim so that no other driver mangles events while we are handling it
                if (productHandlers[descriptor.idProduct]->attachToInterfaceId(interface_number)) {
                    // Attach to our handler
                    productHandlers[descriptor.idProduct]->attachDevice(handle);

                    const libusb_interface_descriptor *interfaceDescriptor =
                            configDescriptor->interface[interface_number].altsetting;

                    if (!setupReportProtocol(handle, interface_number) ||
                        !setupInfiniteIdle(handle, interface_number)) {
                        continue;
                    }

                    const libusb_endpoint_descriptor *endpoint = interfaceDescriptor->endpoint;
                    const libusb_endpoint_descriptor *ep;
                    for (ep = endpoint; (ep - endpoint) < interfaceDescriptor->bNumEndpoints; ++ep) {
                        // Ignore any interface that isn't of an interrupt type
                        if ((ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) != LIBUSB_TRANSFER_TYPE_INTERRUPT)
                            continue;

                        // We only send the init key on the interface the handler says it should be on
                        if (productHandlers[descriptor.idProduct]->sendInitKeyOnInterface() == interface_number) {
                            if ((ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT) {
                                sendInitKey(handle, ep->bEndpointAddress);
                            }
                        }

                        if ((ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) {
                            setupTransfers(handle, ep->bEndpointAddress, ep->wMaxPacketSize, descriptor.idProduct);
                        }
                    }

                    std::cout << std::dec << "Setup completed on interface " << (int)interface_number << std::endl;
                }
            }
        }
    } else {
        std::cout << "libusb_open returned error " << err << std::endl;
    }

    return deviceInterface;
}

void xp_pen_handler::cleanupDevice(device_interface_pair *pair) {
    for (auto interface: pair->claimedInterfaces) {
        libusb_release_interface(pair->deviceHandle, interface);
    }

    for (auto interface: pair->detachedInterfaces) {
        libusb_attach_kernel_driver(pair->deviceHandle, interface);
    }
}

void xp_pen_handler::sendInitKey(libusb_device_handle *handle, int interface_number) {
    std::cout << "Sending init key on endpont " << interface_number << std::endl;

    unsigned char key[] = {0x02, 0xb0, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int sentBytes;
    int ret = libusb_interrupt_transfer(handle, interface_number | LIBUSB_ENDPOINT_OUT, key, sizeof(key), &sentBytes, 1000);
    if (ret != LIBUSB_SUCCESS) {
        std::cout << "Failed to send key on interface " << interface_number << " ret: " << ret << " errno: " << errno << std::endl;
        return;
    }

    if (sentBytes != sizeof(key)) {
        std::cout << "Didn't send all of the key on interface " << interface_number << " only sent " << sentBytes << std::endl;
        return;
    }
}

bool xp_pen_handler::setupTransfers(libusb_device_handle *handle, unsigned char interface_number, int maxPacketSize, int productId) {
    std::cout << "Setting up transfers on endpoint " << (int)interface_number << std::endl;

    struct libusb_transfer* transfer = libusb_alloc_transfer(0);
    if (transfer == NULL) {
        std::cout << "Could not allocate a transfer for interface " << interface_number << std::endl;
        return false;
    }

    transfer->user_data = NULL;
    unsigned char* buff = new unsigned char[maxPacketSize];

    struct transfer_handler_pair* dataPair = new transfer_handler_pair();
    dataPair->vendorHandler = this;
    dataPair->transferHandler = productHandlers[productId];

    libusb_fill_interrupt_transfer(transfer,
                                   handle, interface_number | LIBUSB_ENDPOINT_IN,
                                   buff, maxPacketSize,
                                   transferCallback, dataPair,
                                   60000);

    transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;
    int ret = libusb_submit_transfer(transfer);
    if (ret != LIBUSB_SUCCESS) {
        std::cout << "Could not submit transfer on interface " << (int)interface_number << " ret: " << ret << " errno: " << errno << std::endl;
        return false;
    }

    return true;
}

void xp_pen_handler::transferCallback(struct libusb_transfer *transfer) {
    int err;
    struct transfer_handler_pair* dataPair = (transfer_handler_pair*)transfer->user_data;

    switch (transfer->status) {
    case LIBUSB_TRANSFER_COMPLETED:
        // Send the packet data to the registered handler
        dataPair->transferHandler->handleTransferData(transfer->dev_handle, transfer->buffer, transfer->actual_length);

        // Resubmit the transfer
        err = libusb_submit_transfer(transfer);
        if (err != LIBUSB_SUCCESS) {
            std::cout << "Could not resubmit my transfer" << std::endl;
        }

        break;

    case LIBUSB_TRANSFER_TIMED_OUT:
        // Resubmit the transfer
        err = libusb_submit_transfer(transfer);
        if (err != LIBUSB_SUCCESS) {
            std::cout << "Could not resubmit my transfer" << std::endl;
        }

        break;

    default:
        std::cout << "Unknown status received " << transfer->status << std::endl;
        break;
    }
}
