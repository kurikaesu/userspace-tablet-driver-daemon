/*
userspace_tablet_driver_daemon
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
#include <iomanip>
#include "star.h"

star::star() {

}

std::string star::getProductName(int productId) {
    return "Unknown XP-Pen device";
}

void star::setConfig(nlohmann::json config) {

}

int star::sendInitKeyOnInterface() {
    return 0x02;
}

bool star::attachToInterfaceId(int interfaceId) {
    return true;
}

bool star::handleTransferData(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    switch (data[0]) {
        case 0x07:
            handleDigitizerEvent(handle, data, dataLen);
            break;

        case 0x02:
            xp_pen_unified_device::handleDigitizerEvent(handle, data, dataLen);
            break;

        default:
            std::cout << "Received unknown message" << std::endl;
    }
    return true;
}
