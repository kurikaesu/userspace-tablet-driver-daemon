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
#include "deco_large.h"

deco_large::deco_large()
: deco() {
    productIds.push_back(0x0935);
}

void deco_large::setOffsetPressure(int productId) {
    offsetPressure = -8192;
}

std::string deco_large::getProductName(int productId) {
    if (productId == 0x0935) {
        return "XP-Pen Deco Large";
    }

    return deco::getProductName(productId);
}
