/*
xp_pen_userland
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

#ifndef XP_PEN_USERLAND_ARTIST_12_PRO_H
#define XP_PEN_USERLAND_ARTIST_12_PRO_H


#include "transfer_handler.h"

class artist_12_pro : public transfer_handler {
public:
    artist_12_pro();
    ~artist_12_pro();

    std::string getProductName(int productId);
    void setConfig(nlohmann::json config);
    int sendInitKeyOnInterface();
    bool attachToInterfaceId(int interfaceId);
    bool attachDevice(libusb_device_handle* handle, int interfaceId);
    bool handleTransferData(libusb_device_handle* handle, unsigned char* data, size_t dataLen);
private:
    void handleDigitizerEvent(libusb_device_handle* handle, unsigned char* data, size_t dataLen);
    void handleFrameEvent(libusb_device_handle* handle, unsigned char* data, size_t dataLen);
};


#endif //XP_PEN_USERLAND_ARTIST_12_PRO_H
