/**
 * @file pps-launch-server.c
 * @brief A server in the DHT
 *
 */

// standard includes (printf, exit, ...)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> // for itoa

// for basic socket communication
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "config.h" // for PPS_DEFAULT_IP and PPS_DEFAULT_PORT
#include "system.h" // for get_socket, get_server_addr & bind_server
#include "hashtable.h" // for add_Htable_value & get_Htable_value

#define MAX_IP_SIZE 15
#define PORT_SIZE 1

void serve_get_request(Htable_t table, char *in_msg, int s, struct sockaddr_in cli_addr, socklen_t addr_len) {

    //Get value corresponding to key
    pps_value_t value = get_Htable_value(table, in_msg);

    if (value != NULL) {
        sendto(s, value, strlen(value), 0, (struct sockaddr *) &cli_addr, addr_len);
    } else {
        //No value found
        sendto(s, '\0', 1, 0, (struct sockaddr *) &cli_addr, addr_len);
    }

}

void serve_write_request(Htable_t table, char *in_msg, int s, struct sockaddr_in cli_addr, socklen_t addr_len) {

    //in_msg is initialized with zeros, only overwritten by the key and the value
    if (add_Htable_value(table, in_msg, in_msg + strlen(in_msg) + 1) == ERR_NONE) {
        // Send response back to sender (an empty datagram)
        sendto(s, NULL, 0, 0, (struct sockaddr *) &cli_addr, addr_len);
    }
}

error_code serve_dump_node(Htable_t table, int s, struct sockaddr_in cli_addr, socklen_t addr_len) {

    kv_list_t* list = get_Htable_content(table);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(list, ERR_NOMEM);

    char msg[MAX_MSG_SIZE];
    (void) memset(msg, '\0', MAX_MSG_SIZE);

    sprintf(msg, "%zu", list->size);
    size_t index = strlen(msg) + 1;

    size_t index_list = 0;
    size_t next_len   = 0;

    while (index_list < list->size) {
        do {
            kv_pair_t current = list->elems[index_list];

            size_t key_len = strlen(current.key);
            strncpy(msg + index, current.key, key_len);
            index += key_len + 1;

            size_t value_len = strlen(current.value);
            strncpy(msg + index, current.value, value_len);
            index += value_len + 1;

            index_list += 1;
            if (index_list < list->size) {
                next_len = strlen(list->elems[index_list].key) + strlen(list->elems[index_list].value) + 2;
            }
        } while (index_list < list->size && next_len < MAX_MSG_SIZE - index);

        sendto(s, msg, index, 0, (struct sockaddr *) &cli_addr, addr_len);

        (void) memset(msg, '\0', MAX_MSG_SIZE);
        index = 0;
    }

    kv_list_free(list);
    return ERR_NONE;
}

int main(void) {

    char ip_addr[MAX_IP_SIZE + 1];

    int port = 0;
    printf("IP port?");
    fflush(stdout);

    M_EXIT_IF((scanf("%s %u", ip_addr, &port) != 2), ERR_NOT_FOUND, "scanf", "%s", "error on reading IP address");

    M_EXIT_IF(port < 0 || port > UINT16_MAX, ERR_BAD_PARAMETER, "port", "%s", "wrong size");

    // Set up socket (without timeout, for servers).
    int s = get_socket(0);
    M_EXIT_IF(s == -1, ERR_NETWORK, "get socket", "%s", "problem with socket");

    // Load server address.
    /** sockadrr is a generic descriptor for any kind of socket operation, whereas sockaddr_in is specific 
     * for IP-based communication
     */
    struct sockaddr_in srv_addr;
    error_code error = get_server_addr(ip_addr, (uint16_t) port, &srv_addr);
    M_EXIT_IF_ERR(error, "failed to get server address");

    // Bind server to the address:port
    error = bind_server(s, ip_addr, (uint16_t) port);
    M_EXIT_IF_ERR(error, "failed to bind server address");


    // Create and initialize new empty Htable
    Htable_t table = construct_Htable(HTABLE_SIZE);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(table, ERR_NOMEM);

    char in_msg[MAX_MSG_SIZE];

    // Receive messages forever.
    while (1) {

        (void) memset(in_msg, '\0', MAX_MSG_SIZE);

        // Receive message and get return address.
        struct sockaddr_in cli_addr;
        socklen_t          addr_len = sizeof(cli_addr);

        // Create appropriate buffer to receive message
        ssize_t in_msg_len = recvfrom(s, in_msg, MAX_MSG_SIZE, 0, (struct sockaddr *) &cli_addr, &addr_len);
        if (in_msg_len != -1) {

            char *nul = memchr(in_msg, '\0', (size_t) in_msg_len);

            /** Here, we check if the message is empty -> it's a message to check if the server is responsive (pps-list-nodes) */
            if (in_msg_len == 0) {
                sendto(s, NULL, 0, 0, (struct sockaddr *) &cli_addr, addr_len);
                /** Here, we check if the message is of length 1 -> print all key-value pairs associated to the node (pps-dump-node) */
            } else if (in_msg_len == 1 && strncmp("\0", in_msg, 1) == 0) {
                serve_dump_node(table, s, cli_addr, addr_len);

            } else {
                /** Here, we check if the message contains a nul character.
                 * If it does, it's a write request -> add the value associated with the key to the Htable.
                 * If it doesn't, it's a read request -> send the value associated with the key received.
                 */
                if (nul != NULL) {
                    serve_write_request(table, in_msg, s, cli_addr, addr_len);
                } else {
                    serve_get_request(table, in_msg, s, cli_addr, addr_len);
                }
            }
        }

    }


    return 0;
}

