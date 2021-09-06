/*
xp-pen-userland
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

#ifndef XP_PEN_USERLAND_TRANSFER_HANDLER_PAIR_H
#define XP_PEN_USERLAND_TRANSFER_HANDLER_PAIR_H

struct transfer_handler_pair {
public:
    vendor_handler* vendorHandler;
    transfer_handler* transferHandler;
};

#endif //XP_PEN_USERLAND_TRANSFER_HANDLER_PAIR_H
