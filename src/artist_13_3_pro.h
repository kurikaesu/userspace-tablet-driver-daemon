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

#ifndef XP_PEN_USERLAND_ARTIST_13_3_PRO_H
#define XP_PEN_USERLAND_ARTIST_13_3_PRO_H


#include "transfer_handler.h"
#include "pad_mapping.h"
#include "dial_mapping.h"

class artist_13_3_pro : public transfer_handler {
public:
    artist_13_3_pro();
    ~artist_13_3_pro();

    std::vector<int> handledProductIds();
    std::string getProductName(int productId);
    void setConfig(nlohmann::json config);
    nlohmann::json getConfig();
    int sendInitKeyOnInterface();
    bool attachToInterfaceId(int interfaceId);
    bool attachDevice(libusb_device_handle* handle);
    void detachDevice(libusb_device_handle* handle);
    bool handleTransferData(libusb_device_handle* handle, unsigned char* data, size_t dataLen);
private:
    void handleDigitizerEvent(libusb_device_handle* handle, unsigned char* data, size_t dataLen);
    void handleFrameEvent(libusb_device_handle* handle, unsigned char* data, size_t dataLen);

    std::vector<int> productIds;
    std::map<libusb_device_handle*, long> lastPressedButton;
    std::map<libusb_device_handle*, int> uinputPens;
    std::map<libusb_device_handle*, int> uinputPads;

    std::vector<int> padButtonAliases;

    pad_mapping padMapping;
    dial_mapping dialMapping;
    nlohmann::json jsonConfig;
};


#endif //XP_PEN_USERLAND_ARTIST_13_3_PRO_H
