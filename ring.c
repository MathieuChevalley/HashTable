/**
 * @file ring.c
 * @brief Implementation of ring.h
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "ring.h"
#include "node_list.h"

ring_t *ring_alloc() {

	ring_t *ring = calloc(1, sizeof(ring_t));

	if (ring != NULL) {
		ring = get_nodes();
	}

	return ring;
}


error_code ring_init(ring_t *ring) {

    M_REQUIRE_NON_NULL(ring);
    
    node_list_sort(ring, node_cmp_sha);
	
    return ERR_NONE;
}

node_list_t *ring_get_nodes_for_key(const ring_t *ring, size_t wanted_list_size, pps_key_t key) {
	
	M_REQUIRE_NON_NULL_CUSTOM_ERR(ring, NULL);
	
    unsigned char hash[SHA_DIGEST_LENGTH];
    (void)memset(hash, 0, SHA_DIGEST_LENGTH);
    SHA1((const unsigned char *) key, strlen(key), hash);
    
    const node_t node_key = {.sha = {*hash}};
	
	node_list_t *result = node_list_new();
	result->size = 0;
	
	size_t i = 0;
	// find first node such that its sha is greater than the key's sha 
	while (node_cmp_sha(&(node_key), &(ring->nodes[i])) > 0) {
		i++;
	}

	while (result->size < wanted_list_size) { 
		
		if (server_different(result, &(ring->nodes[i]))) {
			if (node_list_add(result, ring->nodes[i]) != ERR_NONE) {
				fprintf(stderr, "error while adding node to node_list");
				node_list_free(result);
				return NULL;
			} 
		}
		i++;
		i = i % ring->size;
	}
	
	return result;
}

int server_different(const ring_t *ring, node_t *node) {
    for (size_t i = 0; i < ring->size; i++) {
        if (memcmp(&(node->addr), &(ring->nodes[i].addr), sizeof(struct sockaddr)) == 0) {
            return 0;
        }
    }

	return 1;
}

void ring_free(ring_t *ring) {
    node_list_free(ring);
}

// ======================================================================
void print_sha(unsigned char* hash) {
	if (hash != NULL) {
		for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
			printf("%02x", hash[i]);
		}
	}
}	
	
	
	
	
		
