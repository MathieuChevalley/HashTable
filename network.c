/**
 * @file network.c
 * @brief Implementation of network.h
 *
 */

#include "network.h"
#include "client.h"
#include "config.h"
#include "error.h"
#include "args.h"
#include "hashtable.h"
#include "util.h"
#include "system.h"
#include "node_list.h"

#include <sys/types.h> //for sendto
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>


#define TIMEOUT 1


error_code
send_messages(const void *message, size_t size_message, int socket, node_list_t *servers_to_contact) {

    for (size_t i = 0; i < servers_to_contact->size; ++i) {

        socklen_t addr_len     = sizeof(servers_to_contact->nodes[i]);
        node_t    *server_node = &(servers_to_contact->nodes[i]);

        //Send a request to a server
        M_EXIT_IF(
                (sendto(socket, message, size_message, 0, &(server_node->addr), addr_len) == -1),
                ERR_NETWORK, "send_message", " %s", "error on send");

    }


    return ERR_NONE;
}

error_code receive_message(int socket, char *response, ssize_t *len_receive, node_list_t *servers_to_contact) {

    //Check confirmation received
    socklen_t       addr_len_received = sizeof(struct sockaddr);
    struct sockaddr addr_sender;

    *len_receive = recvfrom(socket, response, MAX_MSG_ELEM_SIZE, 0, &(addr_sender), &addr_len_received);

    M_EXIT_IF(*len_receive == -1, ERR_NETWORK, "send_message", " %s", "timeout");

    for (size_t i = 0; i < servers_to_contact->size; ++i) {
        if (memcmp(&addr_sender, &(servers_to_contact->nodes[i].addr), addr_len_received) == 0) {
            return ERR_NONE;
        }
    }

    return ERR_NETWORK;

}

int increment_and_test(Htable_t table, pps_key_t key, size_t R) {
    //get_value is the current number of reads we have for the given key
    pps_value_t get_value = get_Htable_value(table, key);

    if (get_value == NULL) {
        char put_value[2] = {(char) 1, '\0'};
        add_Htable_value(table, key, put_value);
        return R == 1;

    } else {

        size_t count = (size_t) get_value[0];
        ++count;

        //update the current number of reads and the local Htable
        char put_value[2] = {(char) count, '\0'};
        add_Htable_value(table, key, put_value);

        free_const_ptr(get_value);
        return count >= R;

    }
}

error_code network_get(client_t client, pps_key_t key, pps_value_t *value) {

    M_REQUIRE_NON_NULL(value);
    M_REQUIRE_NON_NULL(key);
    M_EXIT_IF_TOO_LONG(key, MAX_MSG_ELEM_SIZE, "key too long");

    int socket = get_socket(TIMEOUT);
    M_EXIT_IF(socket == -1, ERR_NETWORK, "network", "%s", "no socket");

    char *response = calloc(MAX_MSG_ELEM_SIZE + 1, sizeof(char)); //response is freed by the caller if we get no errors
    M_REQUIRE_NON_NULL_CUSTOM_ERR(response, ERR_NOMEM);

    //Create a local Htable in order to store the number of counts associated to each value received
    Htable_t table = construct_Htable(client.args->N);
    if (table == NULL) {
        free(response);
        return ERR_NOMEM;
    }

    node_list_t *servers_to_contact = ring_get_nodes_for_key(client.server, client.args->N, key);
    M_EXIT_IF(servers_to_contact->size < client.args->R, ERR_BAD_PARAMETER, "network_get", " %s", "not enough servers");

    error_code error_on_send = send_messages(key, strlen(key), socket, servers_to_contact);
    M_EXIT_IF_ERR(error_on_send, "send all messages");


    //If no errors and the response is not the nul char, we have a valid response
    for (size_t i = 0; i < servers_to_contact->size; ++i) {

        memset(response, '\0', sizeof(*response));
        ssize_t len_receive = 0;

        error_code error_on_receive = receive_message(socket, response, &len_receive, servers_to_contact);

        if (error_on_receive == ERR_NONE && (len_receive != 1 || response[0] != '\0')) {
            response[len_receive] = '\0';
            if (increment_and_test(table, response, client.args->R)) {
                *value = response;
                delete_Htable_and_content(&table);

                return ERR_NONE;
            }
        }
    }

    delete_Htable_and_content(&table);
    free(response);

    return ERR_NETWORK;
}


error_code network_put(client_t client, pps_key_t key, pps_value_t value) {
    M_REQUIRE_NON_NULL(key);
    M_REQUIRE_NON_NULL(value);
    M_EXIT_IF_TOO_LONG(key, MAX_MSG_ELEM_SIZE, "too long key");
    M_EXIT_IF_TOO_LONG(value, MAX_MSG_ELEM_SIZE, "too long value");

    int socket = get_socket(TIMEOUT);
    M_EXIT_IF(socket == -1, ERR_NETWORK, "network", "%s", "no socket");

    //Send pair key/value to put
    size_t key_len      = strlen(key);
    size_t value_len    = strlen(value);
    size_t size_message = key_len + value_len + 1;
    char   message[size_message];
    (void) memset(message, '\0', size_message);

    //Concatenate key and value
    strncpy(message, key, key_len);
    strncpy(message + key_len + 1, value, value_len);

    size_t nb_writes = 0;

    char response[MAX_MSG_ELEM_SIZE];
    (void) memset(response, '\0', MAX_MSG_ELEM_SIZE);

    node_list_t *servers_to_contact = ring_get_nodes_for_key(client.server, client.args->N, key);
    M_EXIT_IF(servers_to_contact->size < client.args->W, ERR_BAD_PARAMETER, "network_put", " %s", "not enough servers");

    error_code error_on_send = send_messages(message, size_message, socket, servers_to_contact);
    M_EXIT_IF_ERR(error_on_send, "sending all messages failed");

    for (size_t i = 0; i < servers_to_contact->size; i++) {

        ssize_t    len_receive      = 0;
        error_code error_on_receive = receive_message(socket, response, &len_receive, servers_to_contact);

        if (error_on_receive == ERR_NONE && len_receive == 0) {

            nb_writes += 1;
            //Check if enough writes have been performed
            if (nb_writes >= client.args->W) {
                return ERR_NONE;
            }
        }

    }

    return ERR_NETWORK;

}


