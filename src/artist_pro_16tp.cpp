/*
userspace-tablet-driver-daemon
Copyright (C) 2022 - Aren Villanueva <https://github.com/kurikaesu/>

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
#include <iomanip>
#include "artist_pro_16tp.h"

artist_pro_16tp::artist_pro_16tp() {
    productIds.push_back(0x092e);
}

std::string artist_pro_16tp::getProductName(int productId) {
    if (productId == 0x092e) {
        return "XP-Pen Artist Pro 16TP";
    }

    return "Unknown XP-Pen Device";
}

unsigned short artist_pro_16tp::getDescriptorLength() {
    return 13;
}

void artist_pro_16tp::setConfig(nlohmann::json config) {

    jsonConfig = config;

    submitMapping(jsonConfig);
}

bool artist_pro_16tp::handleTransferData(libusb_device_handle *handle, unsigned char *data, size_t dataLen, int productId) {
    /*for(int i = 0; i < dataLen; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("\r");
    std::cout << std::flush;*/
    switch (data[0]) {
        case 0x02:
            handleDigitizerEvent(handle, data, dataLen);
            break;

        default:
            break;
    }

    return true;
}