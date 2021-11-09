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

#include "xp_pen_unified_device.h"

void xp_pen_unified_device::handleDigitizerEvent(libusb_device_handle *handle, unsigned char *data, size_t dataLen) {
    if (data[1] <= 0xc0) {
        // Extract the X and Y position
        int penX = (data[3] << 8) + data[2];
        int penY = (data[5] << 8) + data[4];

        if (dataLen == 12) {
            // This is currently only used by the Artist 24 Pro tablet
            penX += data[10] << 16;
        }

        // Check to see if the pen is touching
        std::bitset<sizeof(data)> stylusTipAndButton(data[1]);
        int pressure = (data[7] << 8) + data[6];

        // Handle pen coming into/out of proximity
        if (stylusTipAndButton.test(5)) {
            handlePenEnteredProximity(handle);
        } else if (stylusTipAndButton.test(6)) {
            handlePenLeftProximity(handle);
        }

        // Handle actual stylus to digitizer contact
        if (stylusTipAndButton.test(0) | stylusTipAndButton.test(5)) {
            handlePenTouchingDigitizer(handle, applyPressureCurve(pressure));
        }

        // Grab the tilt values
        short tiltx = (char)data[8];
        short tilty = (char)data[9];

        // Check to see if the stylus buttons are being pressed
        if (stylusTipAndButton.test(1)) {
            handleStylusButtonsPressed(handle, BTN_STYLUS);
        } else if (stylusTipAndButton.test(2)) {
            handleStylusButtonsPressed(handle, BTN_STYLUS2);
        } else if (stylusButtonPressed > 0){
            handleStylusButtonUnpressed(handle);
        }

        handleCoordsAndTilt(handle, penX, penY, tiltx, tilty);

        uinput_send(uinputPens[handle], EV_SYN, SYN_REPORT, 1);
    }
}
