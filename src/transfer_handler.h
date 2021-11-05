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

#ifndef USERSPACE_TABLET_DRIVER_DAEMON_TRANSFER_HANDLER_H
#define USERSPACE_TABLET_DRIVER_DAEMON_TRANSFER_HANDLER_H

#include <cstdint>
#include <libusb-1.0/libusb.h>
#include <vector>
#include <string>
#include "uinput_pen_args.h"
#include "uinput_pad_args.h"
#include "uinput_pointer_args.h"
#include "includes/json.hpp"
#include "pad_mapping.h"
#include "dial_mapping.h"
#include "unix_socket_message.h"

class transfer_handler {
public:
    transfer_handler();
    virtual ~transfer_handler();

    virtual std::vector<int> handledProductIds();
    virtual std::string getProductName(int productId) = 0;
    virtual void setConfig(nlohmann::json config) = 0;
    virtual nlohmann::json getConfig();
    virtual int sendInitKeyOnInterface() = 0;
    virtual bool attachToInterfaceId(int interfaceId) = 0;
    virtual bool attachDevice(libusb_device_handle* handle, int interfaceId) = 0;
    virtual void detachDevice(libusb_device_handle* handle);
    virtual bool handleTransferData(libusb_device_handle* handle, unsigned char* data, size_t dataLen) = 0;
    virtual std::vector<unix_socket_message*> handleMessage(unix_socket_message* message);
    virtual bool isAliasedProduct(int productId) { return false; }
    virtual int getAliasedProductId(libusb_device_handle* handle, int originalId) { return originalId; }
protected:
    virtual bool uinput_send(int fd, uint16_t type, uint16_t code, int32_t value);
    virtual int create_pen(const uinput_pen_args& penArgs);
    virtual int create_pad(const uinput_pad_args& padArgs);
    virtual int create_pointer(const uinput_pointer_args& pointerArgs);
    virtual void destroy_uinput_device(int fd);

    virtual void submitMapping(const nlohmann::json& config);

    std::vector<int> productIds;

    std::map<libusb_device_handle*, int> uinputPens;
    std::map<libusb_device_handle*, int> uinputPads;
    std::map<libusb_device_handle*, int> uinputPointers;

    std::map<libusb_device_handle*, long> lastPressedButton;

    std::vector<int> padButtonAliases;

    pad_mapping padMapping;
    dial_mapping dialMapping;
    nlohmann::json jsonConfig;

    bool penInProximity = false;
    bool penWasDown = false;
};

#endif //USERSPACE_TABLET_DRIVER_DAEMON_TRANSFER_HANDLER_H
