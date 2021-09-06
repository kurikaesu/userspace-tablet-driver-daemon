//
// Created by aren on 9/5/21.
//

#ifndef XP_PEN_USERLAND_TRANSFER_HANDLER_H
#define XP_PEN_USERLAND_TRANSFER_HANDLER_H

#include <cstdint>
#include <libusb-1.0/libusb.h>

class transfer_handler {
public:
    virtual int sendInitKeyOnInterface() = 0;
    virtual bool attachToInterfaceId(int interfaceId) = 0;
    virtual bool attachDevice(libusb_device_handle* handle) = 0;
    virtual void detachDevice(libusb_device_handle* handle) = 0;
    virtual bool handleTransferData(libusb_device_handle* handle, unsigned char* data, size_t dataLen) = 0;

protected:
    virtual bool uinput_send(int fd, uint16_t type, uint16_t code, int32_t value);
};

#endif //XP_PEN_USERLAND_TRANSFER_HANDLER_H
