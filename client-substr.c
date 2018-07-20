/**
 * @file pps-client-substr.c
 * @brief extract a substring and store it in a new key
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network.h"
#include "config.h"
#include "util.h" // for argv_size
#include "client.h"

#define NB_ARGS 2

int main(int argc, char *argv[]) {

    //Client initialization
    client_t client;

    client_init_args_t init = {&argv, (size_t) argc, NB_ARGS, TOTAL_SERVERS | GET_NEEDED | PUT_NEEDED, &client};

    error_code error = client_init(init);
    M_EXIT_IF_ERR(error, "problem while initializing the client");


    char *key1   = argv[0];
    char *key2   = argv[1];
    char *value1 = NULL;
    char *value2 = NULL;

    M_EXIT_IF(strlen(key1) > MAX_MSG_ELEM_SIZE || strlen(key2) > MAX_MSG_ELEM_SIZE, ERR_BAD_PARAMETER, "argv", "%s", "key too long");

    if (network_get(client, key1, (pps_value_t *) &value1) != ERR_NONE) {
        printf("FAIL\n");
        return 1;
    }

    if (network_get(client, key2, (pps_value_t *) &value2) != ERR_NONE) {
        free(value1);
        printf("FAIL\n");
        return 1;
    }

    int response = 0;

    char *position = strstr(value1, value2);

    if (position == NULL) {
        response = -1;
    } else {
        response = (int) (position - value1);
    }

    printf("OK %d\n", response);

    free(value1);
    free(value2);

    client_end(&client);

    return 0;
}
