/**
 * @file pps-list-nodes.c
 * @brief list all the nodes
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "config.h" // for MAX_MSG_SIZE
#include "system.h" // for get_socket, get_server_addr & bind_server
#include "error.h"
#include "node.h"
#include "node_list.h"
#include "network.h"
#include "ring.h"

#define MAX_PORT 65535
#define MAX_IP_SIZE 15

int main(void) {
	
    // Set up socket (with timeout of 1).
    int s = get_socket(1);
    M_EXIT_IF(s == -1, ERR_NETWORK, "get socket", "%s", "problem with socket");

    ring_t *nodes_to_print = ring_alloc();
    M_REQUIRE_NON_NULL(nodes_to_print);
    ring_init(nodes_to_print);
    
   //Send all the requests to the server
    /** problem here, error on send?? */
	for (size_t i = 0; i < nodes_to_print->size; i++) {	
		socklen_t addr_len = sizeof(nodes_to_print->nodes[i]);
		node_t * node = &(nodes_to_print->nodes[i]);
			
		//Send an empty datagram to a server
		M_EXIT_IF((sendto(s, NULL, 0, 0, (struct sockaddr *) &(node->addr), addr_len) == -1),
				ERR_NETWORK, "pps-list-nodes", " %s", "error on send");
				
	}
        

    //Check confirmation received
	char response[MAX_MSG_ELEM_SIZE];

	struct sockaddr_in srv_addr_rcv;
	socklen_t          addr_len_rcv;
		
	size_t i = nodes_to_print->size;      
    while (i > 0 && recvfrom(s, response, MAX_MSG_ELEM_SIZE, 0, (struct sockaddr *) &srv_addr_rcv, &addr_len_rcv) == 0) {
		
		/** loop on nodes_to_print to set the node corresponding to the response as responsive  */
		for (size_t i = 0; i < nodes_to_print->size; i++) {
			if (memcmp(&(nodes_to_print->nodes[i].addr), &(srv_addr_rcv), addr_len_rcv) == 0 && nodes_to_print->nodes[i].responsive == 0) {
				nodes_to_print->nodes[i].responsive = 1;
				continue;
			}
		}
	}
	
	qsort(nodes_to_print->nodes, nodes_to_print->size, sizeof(node_t), (int (*)(void const *, void const *))compare_port);
	
	for (size_t i = 0; i < nodes_to_print->size; i++) {

		struct sockaddr_in *ipv4 = (struct sockaddr_in *) &(nodes_to_print->nodes[i].addr);
		uint16_t port = ((struct sockaddr_in*) &(nodes_to_print->nodes[i].addr))->sin_port;
		char ipAddress[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(ipv4->sin_addr), ipAddress, INET_ADDRSTRLEN);

		printf("%s %d (", ipAddress, ntohs(port));
		print_sha(nodes_to_print->nodes[i].sha);

		if (nodes_to_print->nodes[i].responsive == 1) {
			printf(") OK\n");		
		} else {
			printf(") FAIL\n");
		}
	}
	
    return 0;
}

