//
// Created by aren on 9/5/21.
//

#ifndef XP_PEN_USERLAND_TRANSFER_HANDLER_PAIR_H
#define XP_PEN_USERLAND_TRANSFER_HANDLER_PAIR_H

struct transfer_handler_pair {
public:
    vendor_handler* vendorHandler;
    transfer_handler* transferHandler;
};

#endif //XP_PEN_USERLAND_TRANSFER_HANDLER_PAIR_H
