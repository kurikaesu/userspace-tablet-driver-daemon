//
// Created by aren on 9/5/21.
//

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
    bool attachDevice(libusb_device_handle* handle);
    void detachDevice(libusb_device_handle* handle);
    bool handleTransferData(libusb_device_handle* handle, unsigned char* data, size_t dataLen);
private:
    std::vector<int> productIds;
    std::map<libusb_device_handle*, long> lastPressedButton;
    std::map<libusb_device_handle*, int> uinputPens;
};


#endif //XP_PEN_USERLAND_ARTIST_22R_PRO_H
