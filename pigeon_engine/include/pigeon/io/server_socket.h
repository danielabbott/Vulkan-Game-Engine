#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pigeon/util.h>
#include "socket.h"


PIGEON_ERR_RET pigeon_network_create_server_socket_blocking(PigeonNetworkSocket*,
    PigeonNetworkPort port, PigeonNetworkProtocol);

void pigeon_network_close_socket_blocking(PigeonNetworkSocket*);
#define pigeon_network_close_socket_blocking pigeon_network_close_socket_blocking