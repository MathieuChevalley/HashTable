/**
 * @file node.c
 * @brief Implementation of node.h
 *
 */

#include <stdlib.h>
#include <string.h>
#include <openssl/sha.h>
#include <stdio.h>
#include "system.h" //for get_server_addr
#include "node.h"

#define SIZE 54 //15 + 5 + 2 + 32


/** problem here: doesn't give the right sha for the string input? (missmatch?) */
error_code node_init(node_t *node, const char *ip, uint16_t port, size_t _unused node_id) {

    error_code error = get_server_addr(ip, port, (struct sockaddr_in *) &node->addr);
    M_EXIT_IF_ERR(error, "node init");

    char input [SIZE];
    memset(input, 0, SIZE);
    
    snprintf(input, SIZE, "%s %d %lu", ip, port, node_id);
    
    SHA1((unsigned char*) input, strlen(input), node->sha);
    
    node->responsive = 0;

    return ERR_NONE;
}

void node_end(node_t *node) {

}


int node_cmp_sha(const node_t *first, const node_t *second) {
    
    char* sha1 = (char *) first->sha;
    char* sha2 = (char *) second->sha;

    return strncmp(sha1, sha2, SHA_DIGEST_LENGTH);
}

int node_cmp_server_addr(const node_t *first, const node_t *second) {
    return strncmp(first->addr.sa_data, second->addr.sa_data, 14);
}

int compare_port(const node_t *first, const node_t *second) {
	int port_first = ((struct sockaddr_in*)&(first->addr))->sin_port;
	int port_second = ((struct sockaddr_in*)&(second->addr))->sin_port;
	return port_first > port_second;
}
