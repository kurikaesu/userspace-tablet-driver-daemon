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

#ifndef USERSPACE_TABLET_DRIVER_DAEMON_VENDOR_HANDLER_H
#define USERSPACE_TABLET_DRIVER_DAEMON_VENDOR_HANDLER_H

#include <vector>
#include <set>
#include <libusb-1.0/libusb.h>
#include "includes/json.hpp"
#include "unix_socket_message_queue.h"
#include "device_interface_pair.h"
#include "transfer_handler.h"
#include "transfer_setup_data.h"

class vendor_handler {
public:
    virtual ~vendor_handler();

    virtual int getVendorId() { return 0x0000; };
    virtual std::vector<int> getProductIds() { return std::vector<int>(); };
    virtual std::string vendorName() = 0;
    virtual void setConfig(nlohmann::json config) {};
    virtual nlohmann::json getConfig() { return nlohmann::json({}); };
    virtual void setMessageQueue(unix_socket_message_queue* queue);
    virtual void handleMessages() { };
    virtual std::set<short> getConnectedDevices() { return std::set<short>(); }
    virtual bool handleProductAttach(libusb_device* device, const struct libusb_device_descriptor descriptor) { return false; };
    virtual void handleProductDetach(libusb_device* device, const struct libusb_device_descriptor descriptor) {};
protected:
    virtual bool setupReportProtocol(libusb_device_handle* handle, unsigned char interface_number);
    virtual bool setupInfiniteIdle(libusb_device_handle* handle, unsigned char interface_number);

    virtual void addHandler(transfer_handler*);

    virtual void cleanupDevice(device_interface_pair* pair);
    virtual device_interface_pair* claimDevice(libusb_device* device, libusb_device_handle* handle, const libusb_device_descriptor descriptor);

    virtual void sendInitKey(libusb_device_handle* handle, int interface_number, transfer_handler* productHandler) {}

    virtual bool setupTransfers(libusb_device_handle* handle, unsigned char interface_number, int maxPacketSize, int productId);
    static void LIBUSB_CALL transferCallback(struct libusb_transfer* transfer);

    unix_socket_message_queue* messageQueue;

    std::map<libusb_device*, device_interface_pair*> deviceInterfaceMap;
    std::vector<device_interface_pair*> deviceInterfaces;
    std::map<int, transfer_handler*> productHandlers;

    std::vector<int> handledProducts;
    nlohmann::json jsonConfig;

    std::vector<transfer_setup_data> transfersSetUp;
    std::vector<libusb_transfer*> libusbTransfers;
};

#endif //USERSPACE_TABLET_DRIVER_DAEMON_VENDOR_HANDLER_H
