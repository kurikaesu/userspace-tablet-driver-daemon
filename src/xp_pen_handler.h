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

#ifndef XP_PEN_USERLAND_XP_PEN_HANDLER_H
#define XP_PEN_USERLAND_XP_PEN_HANDLER_H

#include <libusb-1.0/libusb.h>
#include <vector>
#include <map>
#include "vendor_handler.h"
#include "device_interface_pair.h"
#include "transfer_handler.h"

class xp_pen_handler : public vendor_handler {
public:
    xp_pen_handler();
    ~xp_pen_handler();

    int getVendorId();
    std::vector<int> getProductIds();
    bool handleProductAttach(libusb_device* device, const libusb_device_descriptor descriptor);
    void handleProductDetach(libusb_device* device, struct libusb_device_descriptor descriptor);
private:
    std::vector<device_interface_pair*> deviceInterfaces;
    std::map<libusb_device*, device_interface_pair*> deviceInterfaceMap;
    std::vector<int> handledProducts;
    std::map<int, transfer_handler*> productHandlers;

    device_interface_pair* claimDevice(libusb_device* device, libusb_device_handle* handle, const libusb_device_descriptor descriptor);
    void cleanupDevice(device_interface_pair* pair);

    void sendInitKey(libusb_device_handle* handle, int interface_number);
    bool setupTransfers(libusb_device_handle* handle, unsigned char interface_number, int maxPacketSize, int productId);

    static void LIBUSB_CALL transferCallback(struct libusb_transfer* transfer);
};


#endif //XP_PEN_USERLAND_XP_PEN_HANDLER_H
