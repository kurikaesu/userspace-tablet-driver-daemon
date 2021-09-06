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

#ifndef XP_PEN_USERLAND_TRANSFER_HANDLER_H
#define XP_PEN_USERLAND_TRANSFER_HANDLER_H

#include <cstdint>
#include <libusb-1.0/libusb.h>
#include <vector>
#include <string>
#include "uinput_pen_args.h"
#include "uinput_pad_args.h"

class transfer_handler {
public:
    virtual std::vector<int> handledProductIds() = 0;
    virtual std::string getProductName(int productId) = 0;
    virtual int sendInitKeyOnInterface() = 0;
    virtual bool attachToInterfaceId(int interfaceId) = 0;
    virtual bool attachDevice(libusb_device_handle* handle) = 0;
    virtual void detachDevice(libusb_device_handle* handle) = 0;
    virtual bool handleTransferData(libusb_device_handle* handle, unsigned char* data, size_t dataLen) = 0;

protected:
    virtual bool uinput_send(int fd, uint16_t type, uint16_t code, int32_t value);
    virtual int create_pen(const uinput_pen_args& penArgs);
    virtual int create_pad(const uinput_pad_args& padArgs);
};

#endif //XP_PEN_USERLAND_TRANSFER_HANDLER_H
