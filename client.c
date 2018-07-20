/**
 * @file client.c
 * @brief Implementation of client.h
 *
 */

#include <stdlib.h>
#include <stddef.h>
#include "client.h"
#include "config.h" // for PPS_DEFAULT_IP and PPS_DEFAULT_PORT
#include "system.h"
#include "util.h"
#include "args.h"
#include "node_list.h"
#include "ring.h"


error_code client_init(client_init_args_t init_args) {
    
    client_t *client = init_args.client;

    M_REQUIRE_NON_NULL(client);
    M_REQUIRE_NON_NULL(init_args.argv);
    M_REQUIRE_NON_NULL(*init_args.argv);

    error_code error = ERR_NONE;

    char *name = **init_args.argv;
    // increment the pointer to next argument (first argument is the name)
    ++(*init_args.argv);
    init_args.argc -= 1;

    M_EXIT_IF(init_args.nb_args != SIZE_MAX && init_args.nb_args > init_args.argc, ERR_BAD_PARAMETER, "argc too short",
              "%s", "not enough arguments");

    char **index_before = *init_args.argv;

    args_t *args = parse_opt_args(init_args.supported_args, init_args.argv);
    M_REQUIRE_NON_NULL(args);

    ptrdiff_t diff = *(init_args.argv) - index_before;
    init_args.argc -= diff;

    if (init_args.nb_args != SIZE_MAX && init_args.nb_args != init_args.argc) {
        error = ERR_BAD_PARAMETER;
        free(args);
    }

    M_EXIT_IF_ERR(error, "not enough arguments");


    // Create the nodes from the file
    ring_t *server = ring_alloc();
    error = ring_init(server);
    if (error != ERR_NONE) {
        free(args);
    }
    M_EXIT_IF_ERR(error, "problem while initializing the nodes");

    if (init_args.supported_args & TOTAL_SERVERS && args->N > server->size) {
        free(args);
        ring_free(server);
        return ERR_BAD_PARAMETER;
    }

    client->args   = args;
    client->server = server;
    client->name   = name;

    return ERR_NONE;
}

void client_end(client_t *client) {
    ring_free(client->server);
    free(client->args);
}
