//
// Created by aren on 9/5/21.
//

#ifndef XP_PEN_USERLAND_TRANSFER_HANDLER_H
#define XP_PEN_USERLAND_TRANSFER_HANDLER_H

class transfer_handler {
public:
    virtual bool handleTransferData(libusb_device_handle* handle, unsigned char* data, size_t dataLen) = 0;
};

#endif //XP_PEN_USERLAND_TRANSFER_HANDLER_H
