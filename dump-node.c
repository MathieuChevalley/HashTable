/**
 * @file pps-dump-node.c
 * @brief list the content of a node
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "config.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "config.h" // for MAX_MSG_SIZE
#include "system.h" // for get_socket, get_server_addr & bind_server
#include "error.h"

#define MAX_PORT 65535
#define MAX_IP_SIZE 15

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Not enough arguments.\n");
        return ERR_BAD_PARAMETER;
    }

    // Set up socket (with timeout of 1).
    int s = get_socket(1);

    char *ip_addr = argv[1];
    int  port = 0;
    M_EXIT_IF(sscanf(argv[2], "%d", &port) != 1, ERR_BAD_PARAMETER, "port", "%s", "wrong program input");

    //Wrong port size
    if (port < 0 || port > MAX_PORT) {
        fprintf(stderr, "Wrong port size");
        return ERR_BAD_PARAMETER;
    }

    struct sockaddr_in srv_addr;
    M_EXIT_IF_ERR(get_server_addr(ip_addr, (uint16_t) port, &srv_addr), "failed to get server address");
    socklen_t addr_len = sizeof(srv_addr);

    //Send a message containing only the nul character '\0'
    const char *message = "\0";
    M_EXIT_IF((sendto(s, message, 1, 0, (struct sockaddr *) &srv_addr, addr_len) == -1),
              ERR_NETWORK, "pps-list-nodes", "%s", "error on send");

    //Check confirmation received
    char response[MAX_MSG_SIZE + 1];
    (void) memset(response, '\0', MAX_MSG_SIZE + 1);
    struct sockaddr_in srv_addr_recv = srv_addr;
    socklen_t          addr_len_recv = addr_len;

    size_t total_kv = 0;
    size_t nb_kv    = 0;
    size_t index    = 0;

    while (recvfrom(s, response, MAX_MSG_SIZE, 0, (struct sockaddr *) &srv_addr_recv, &addr_len_recv) != -1
           && memcmp(&srv_addr, &srv_addr_recv, sizeof(struct sockaddr_in)) == 0) {
        index = 0;

        if ((nb_kv == 0) && (sscanf(response, "%lu", &total_kv) == 1)) {
            index = strlen(response) + 1;
        }

        while (nb_kv < total_kv && strlen(&response[index]) != 0) {
            printf("%s = ", response + index);
            index += strlen(response + index) + 1;

            printf("%s\n", response + index);
            index += strlen(response + index) + 1;

            nb_kv += 1;
        }

        fflush(stdout);
        (void) memset(response, '\0', MAX_MSG_SIZE);
    }


    return 0;
}

