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

#ifndef USERSPACE_TABLET_DRIVER_DAEMON_UNIX_SOCKET_MESSAGE_H
#define USERSPACE_TABLET_DRIVER_DAEMON_UNIX_SOCKET_MESSAGE_H

enum message_destination {
    driver = 0,
    eventHandler,
    gui
};

struct unix_socket_message {
    message_destination destination;
    short vendor;
    short device;
    short interface;
    long length;
    bool expectResponse;
    long responseLength;
    short responseInterface;
    int originatingSocket;
    long signature;
    unsigned char* data;
};

struct unix_socket_message_header {
    message_destination destination;
    short vendor;
    short device;
    short interface;
    long length;
    bool expectResponse;
    long responseLength;
    short responseInterface;
    int originatingSocket;
    long signature;
};

#endif //USERSPACE_TABLET_DRIVER_DAEMON_UNIX_SOCKET_MESSAGE_H
