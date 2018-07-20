/**
 * @file pps-client-cat.c
 * @brief concatenate value of the dht
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network.h"
#include "config.h"
#include "util.h" // for argv_size
#include "client.h"
#include "args.h"

#define MIN_NB_ARGS 2

int main(int argc, char *argv[]) {

    //Client initialization
    client_t client;

    client_init_args_t init = {&argv, (size_t) argc, SIZE_MAX, TOTAL_SERVERS | PUT_NEEDED | GET_NEEDED, &client};
    
    error_code error = client_init(init);
    M_EXIT_IF_ERR(error, "problem while initializing the client");


    /** The call to client_init, and the subsequent call to parse_opt_args on argv will increment the pointer
     * argv to the first argument that is not optional. All the arguments left are keys. */
    char **keys = argv;

    size_t number_keys = argv_size(argv);

    M_EXIT_IF(number_keys < MIN_NB_ARGS, ERR_BAD_PARAMETER, "input", " %s ", "not enough keys");

    char concatenation [MAX_MSG_ELEM_SIZE + 1];
    memset(concatenation, 0, MAX_MSG_ELEM_SIZE + 1);

    /** We need to get and concatenate the values of all the keys except the last one */
    for (size_t i = 0; i < number_keys - 1; ++i) {

        char *value = NULL;

        if (network_get(client, keys[i], (pps_value_t *) &value) != ERR_NONE) {
            printf("FAIL\n");
            client_end(&client);
            return ERR_NETWORK;
        }

        if (strlen(value) + strlen(concatenation)  >= MAX_MSG_ELEM_SIZE) {
            printf("FAIL\n");
            client_end(&client);
            return ERR_NOMEM;
        }

        strncat(concatenation, value, MAX_MSG_ELEM_SIZE);

        free(value);
    }


    if (network_put(client, keys[number_keys - 1], concatenation) != ERR_NONE) {
        printf("FAIL\n");
        return 1;
    } else {
        printf("OK\n");
    }


    client_end(&client);

    return 0;
}
