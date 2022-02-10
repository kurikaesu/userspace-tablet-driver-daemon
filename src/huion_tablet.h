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

#ifndef USERSPACE_TABLET_DRIVER_DAEMON_HUION_TABLET_H
#define USERSPACE_TABLET_DRIVER_DAEMON_HUION_TABLET_H


#include <set>
#include "transfer_handler.h"

class huion_tablet : public transfer_handler {
public:
    huion_tablet(int productId);

    std::string getProductName(int productId);
    void setConfig(nlohmann::json config);
    int sendInitKeyOnInterface();
    bool attachToInterfaceId(int interfaceId);
    bool attachDevice(libusb_device_handle* handle, int interfaceId, int productId);
    bool handleTransferData(libusb_device_handle* handle, unsigned char* data, size_t dataLen, int productId);
    std::set<int> getConnectedAliasedDevices();
    std::wstring getDeviceFirmwareName(libusb_device_handle* device);
    int getAliasedDeviceIdFromFirmware(std::wstring firmwareName);
    int getAliasedProductId(libusb_device_handle* handle, int originalId);
    std::string getInitKey() { return ""; }
private:
    void handleDigitizerEventV1(libusb_device_handle* handle, unsigned char* data, size_t dataLen);
    void handleDigitizerEventV2(libusb_device_handle* handle, unsigned char* data, size_t dataLen);
    void handleDigitizerEventV3(libusb_device_handle* handle, unsigned char* data, size_t dataLen);
    void handlePadEventV1(libusb_device_handle* handle, unsigned char* data, size_t dataLen);
    void handleTouchStripEvent(libusb_device_handle* handle, unsigned char* data, size_t dataLen);
    void handleTabletDialEvent(libusb_device_handle* handle, unsigned char* data, size_t dataLen);

    std::string getDeviceNameFromFirmware(std::wstring firmwareName);

    std::map<libusb_device_handle*, std::string> handleToDeviceName;
    std::map<libusb_device_handle*, int> handleToAliasedDeviceId;
    short touchStripLastValue;
};


#endif //USERSPACE_TABLET_DRIVER_DAEMON_HUION_TABLET_H
