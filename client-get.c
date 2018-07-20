/**
 * @file pps-client-get.c
 * @brief Client to get a key from the servers
 *
 */

#include <stdio.h> //for standard input
#include <stdlib.h>
#include "network.h" //network get
#include "config.h"
#include "util.h" // for argv_size
#include "client.h"
#include "error.h"

#define NUMBER_ARGS 1

int main(int argc, char *argv[]) {

    //Client initialization
    client_t client;

    client_init_args_t init = {&argv, (size_t) argc, NUMBER_ARGS, TOTAL_SERVERS | GET_NEEDED, &client};

    error_code error = client_init(init);
    M_EXIT_IF_ERR(error, "problem while initializing the client");

    char *request = argv[0];
    char *value   = NULL;

    if (strlen(request) > MAX_MSG_ELEM_SIZE) {
        printf("FAIL\n");
        client_end(&client);
        return ERR_BAD_PARAMETER;
    }

    if (network_get(client, request, (pps_value_t *) &value) == ERR_NONE) {
        printf("OK %s\n", value);
    } else {
        printf("FAIL\n");
    }

    free(value);

    //Client closing
    client_end(&client);

    return 0;
}
