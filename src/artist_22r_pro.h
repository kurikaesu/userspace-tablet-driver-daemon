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

#ifndef XP_PEN_USERLAND_ARTIST_22R_PRO_H
#define XP_PEN_USERLAND_ARTIST_22R_PRO_H

#include <vector>
#include <libusb-1.0/libusb.h>
#include <map>
#include "transfer_handler.h"

class artist_22r_pro : public transfer_handler {
public:
    artist_22r_pro();

    std::vector<int> handledProductIds();
    int sendInitKeyOnInterface();
    bool attachToInterfaceId(int interfaceId);
    bool attachDevice(libusb_device_handle* handle);
    void detachDevice(libusb_device_handle* handle);
    bool handleTransferData(libusb_device_handle* handle, unsigned char* data, size_t dataLen);
private:
    std::vector<int> productIds;
    std::map<libusb_device_handle*, long> lastPressedButton;
    std::map<libusb_device_handle*, int> uinputPens;
    std::map<libusb_device_handle*, int> uinputPads;

    std::vector<int> padButtonAliases;
};


#endif //XP_PEN_USERLAND_ARTIST_22R_PRO_H
