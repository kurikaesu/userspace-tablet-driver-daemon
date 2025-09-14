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

#ifndef USERSPACE_TABLET_DRIVER_DAEMON_XP_PEN_UNIFIED_DEVICE_H
#define USERSPACE_TABLET_DRIVER_DAEMON_XP_PEN_UNIFIED_DEVICE_H

#include "transfer_handler.h"
#include "button_mapping_configuration.h"
#include "device_specification.h"
#include <map>

class xp_pen_unified_device : public transfer_handler {
public:
    // Default constructor
    xp_pen_unified_device();
    
    // Parameterized constructor with device specification
    xp_pen_unified_device(const device_specification& spec);
    
    // Virtual destructor
    virtual ~xp_pen_unified_device() = default;
    
    // Common implementation of getProductName using the product registration system
    virtual std::string getProductName(int productId) override;
    
    // Common implementation of setConfig using button_mapping_configuration
    virtual void setConfig(nlohmann::json config) override;
    
    // Register a product ID and name
    void registerProduct(int productId, const std::string& name);
    
    // Apply a default configuration
    void applyDefaultConfig(bool withDial = false);
    
protected:
    virtual int sendInitKeyOnInterface();
    virtual bool attachToInterfaceId(int interfaceId);
    virtual unsigned short getDescriptorLength();
    bool attachDevice(libusb_device_handle* handle, int interfaceId, int productId);
    void handleDigitizerEvent(libusb_device_handle* handle, unsigned char* data, size_t dataLen);
    virtual std::string getInitKey() override;
    
    // Common implementation for handling frame events
    void handleGenericFrameEvent(
        libusb_device_handle* handle, 
        unsigned char* data, 
        size_t dataLen,
        int buttonByteIndex = 2,
        int dialByteIndex = 7
    );
    
    // Initialize pad button aliases based on number of buttons
    void initializePadButtonAliases(int numButtons);
    
    // Product registration system
    std::map<int, std::string> productNameMap;
    
    // Device specification
    device_specification deviceSpec;
};


#endif //USERSPACE_TABLET_DRIVER_DAEMON_XP_PEN_UNIFIED_DEVICE_H
