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
#include "vendor_handler.h"
#include "transfer_handler_pair.h"

vendor_handler::~vendor_handler() {
    for (auto deviceInterface : deviceInterfaces) {
        cleanupDevice(deviceInterface);
    }
}

void vendor_handler::setMessageQueue(unix_socket_message_queue *queue) {
    messageQueue = queue;
}

bool vendor_handler::setupReportProtocol(libusb_device_handle* handle, unsigned char interface_number) {
    int err = libusb_control_transfer(handle,
                                  0x21,
                                  0x0b,
                                  1,
                                  interface_number,
                                  NULL, 0,
                                  1000);
    if (err != LIBUSB_SUCCESS && err != LIBUSB_ERROR_PIPE) {
        std::cout << "Could not set report protocol on interface " << interface_number << " errno: " << err << std::endl;
        return false;
    }

    return true;
}

bool vendor_handler::setupInfiniteIdle(libusb_device_handle* handle, unsigned char interface_number) {
    int err = libusb_control_transfer(handle,
                                  0x21,
                                  0x0a,
                                  0 << 8,
                                  interface_number,
                                  NULL, 0,
                                  1000);

    if (err != LIBUSB_SUCCESS && err != LIBUSB_ERROR_PIPE) {
        std::cout << "Could not set infinite idle on interface " << interface_number << " errno: " << err << std::endl;
        return false;
    }

    return true;
}

void vendor_handler::cleanupDevice(device_interface_pair *pair) {
    for (auto interface: pair->claimedInterfaces) {
        libusb_release_interface(pair->deviceHandle, interface);
    }

    for (auto interface: pair->detachedInterfaces) {
        libusb_attach_kernel_driver(pair->deviceHandle, interface);
    }
}

void vendor_handler::addHandler(transfer_handler *handler) {
    for (auto productId : handler->handledProductIds()) {
        productHandlers[productId] = handler;
        handledProducts.push_back(productId);
    }
}

device_interface_pair* vendor_handler::claimDevice(libusb_device *device, libusb_device_handle *handle, const libusb_device_descriptor descriptor) {
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

            if (libusb_kernel_driver_active(handle, interface_number)) {
                err = libusb_detach_kernel_driver(handle, interface_number);
                if (LIBUSB_SUCCESS == err) {
                    deviceInterface->detachedInterfaces.push_back(interface_number);
                } else {
                    std::cout << "Got " << err << " when detaching kernel driver" << std::endl;
                }
            }

            err = libusb_claim_interface(handle, interface_number);
            if (LIBUSB_SUCCESS == err) {
                deviceInterface->claimedInterfaces.push_back(interface_number);

                // Even though we claim the interface, we only actually care about specific ones. We still do
                // the claim so that no other driver mangles events while we are handling it
                if (productHandlers[descriptor.idProduct]->attachToInterfaceId(interface_number)) {
                    // Attach to our handler
                    if (!productHandlers[descriptor.idProduct]->attachDevice(handle, interface_number)) {
                        delete deviceInterface;
                        return nullptr;
                    }

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
                            struct transfer_setup_data setupData {
                                    handle,
                                    ep->bEndpointAddress,
                                    ep->wMaxPacketSize,
                                    descriptor.idProduct
                            };
                            transfersSetUp.push_back(setupData);
                            setupTransfers(handle, ep->bEndpointAddress, ep->wMaxPacketSize, descriptor.idProduct);
                        }
                    }

                    std::cout << std::dec << "Setup completed on interface " << (int)interface_number << std::endl;
                }
            } else {
                std::cout << "Could not claim interface " << (int)interface_number << " retcode: " << err << " errno: " << errno << std::endl;
                delete deviceInterface;
                return nullptr;
            }
        }
    } else {
        std::cout << "libusb_open returned error " << err << std::endl;
        if (err == LIBUSB_ERROR_ACCESS) {
            std::cout << "This was an access denied error. Is the correct udev rule set up?" << std::endl;
        }
    }

    deviceInterface->productId = descriptor.idProduct;
    return deviceInterface;
}

bool vendor_handler::setupTransfers(libusb_device_handle *handle, unsigned char interface_number, int maxPacketSize, int productId) {
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

    libusbTransfers.push_back(transfer);

    return true;
}

void vendor_handler::transferCallback(struct libusb_transfer *transfer) {
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

        case LIBUSB_TRANSFER_CANCELLED:
            break;

        default:
            std::cout << "Unknown status received " << transfer->status << std::endl;
            break;
    }
}
