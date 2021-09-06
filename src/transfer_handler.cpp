//
// Created by aren on 9/5/21.
//

#include <linux/uinput.h>
#include <unistd.h>
#include "transfer_handler.h"

bool transfer_handler::uinput_send(int fd, uint16_t type, uint16_t code, int32_t value) {
    struct input_event event = {
            .type = type,
            .code = code,
            .value = value
    };
    if (write(fd, &event, sizeof(event)) < 0) {
        return false;
    }

    return true;
}
