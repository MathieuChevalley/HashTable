/**
 * @file pps-client-put.c
 * @brief Client to add a pair key value to the DHT
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "network.h"
#include "config.h"
#include "util.h" // for argv_size
#include "client.h"

#define NB_ARGS 2

int main(int argc, char *argv[]) {
    //Client initialization
    client_t client;

    client_init_args_t init = {&argv, (size_t) argc, NB_ARGS, TOTAL_SERVERS | PUT_NEEDED, &client};

    error_code error = client_init(init);
    M_EXIT_IF_ERR(error, "problem while initializing the client");

    char *key   = argv[0];
    char *value = argv[1];

    if (strlen(key) > MAX_MSG_ELEM_SIZE || strlen(value) > MAX_MSG_ELEM_SIZE) {
        printf("FAIL\n");
        client_end(&client);
        return ERR_BAD_PARAMETER;
    }

    //Send pair key/value
    if (network_put(client, key, value) == ERR_NONE) {
        printf("OK\n");
    } else {
        printf("FAIL\n");
    }

    //Client closing
    client_end(&client);

    return 0;
}

