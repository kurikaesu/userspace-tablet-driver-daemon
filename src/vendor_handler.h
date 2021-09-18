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

#ifndef XP_PEN_USERLAND_VENDOR_HANDLER_H
#define XP_PEN_USERLAND_VENDOR_HANDLER_H

#include <vector>
#include <libusb-1.0/libusb.h>
#include "includes/json.hpp"
#include "unix_socket_message_queue.h"

class vendor_handler {
public:
    virtual ~vendor_handler() {};

    virtual int getVendorId() { return 0x0000; };
    virtual std::vector<int> getProductIds() { return std::vector<int>(); };
    virtual std::string vendorName() = 0;
    virtual void setConfig(nlohmann::json config) {};
    virtual nlohmann::json getConfig() { return nlohmann::json({}); };
    virtual void setMessageQueue(unix_socket_message_queue* queue);
    virtual void handleMessages() { };
    virtual bool handleProductAttach(libusb_device* device, const struct libusb_device_descriptor descriptor) { return false; };
    virtual void handleProductDetach(libusb_device* device, const struct libusb_device_descriptor descriptor) {};
protected:
    virtual bool setupReportProtocol(libusb_device_handle* handle, unsigned char interface_number);
    virtual bool setupInfiniteIdle(libusb_device_handle* handle, unsigned char interface_number);

    virtual bool setupTransfers(libusb_device_handle* handle, unsigned char interface_number, int maxPacketSize, int productId) { return false; };

    unix_socket_message_queue* messageQueue;
};

#endif //XP_PEN_USERLAND_VENDOR_HANDLER_H
