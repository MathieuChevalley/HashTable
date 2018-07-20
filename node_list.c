/**
 * @file node_list.c
 * @brief Implementation of node_list.h
 *
 */

#include "node_list.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "config.h"

#define MAX_PORT 65535
#define MAX_IP_SIZE 15

node_list_t *node_list_new() {
    node_list_t *nodes = calloc(1, sizeof(node_list_t));
    return nodes;
}

error_code node_list_add(node_list_t *list, node_t node) {

    M_REQUIRE_NON_NULL(list);

    //Grow the list size
    list->size += 1;

    node_t *old = list->nodes;

    if ((list->size > SIZE_MAX / sizeof(node_t)) ||
        ((list->nodes = realloc(list->nodes, list->size * sizeof(node_t)))
         == NULL)) {
        list->nodes = old;
        list->size -= 1;
        return ERR_NOMEM;

    } else {
        //Add the node to the list
        list->nodes[list->size - 1] = node;

        return ERR_NONE;
    }
}

void node_list_free(node_list_t *list) {

    if (list != NULL) {
        //Closing all nodes
        for (size_t i = 0; i < list->size; ++i) {
            node_end(&list->nodes[i]);
        }

        //Emptying the list
        list->size = 0;
        free(list->nodes);
        list->nodes = NULL;
        free(list);
    }
}

void node_list_sort(node_list_t *list, int (*comparator)(const node_t *, const node_t *)) {
    qsort(list->nodes, list->size, sizeof(node_t), (int (*)(void const *, void const *)) comparator);
}

node_list_t *get_nodes() {

    node_list_t *node_list1 = node_list_new();

    //Error on creation
    M_REQUIRE_NON_NULL_CUSTOM_ERR(node_list1, NULL);

    //Open the file
    FILE *file = fopen(PPS_SERVERS_LIST_FILENAME, "r");

    //Error on opening
    if (file == NULL) {
        node_list_free(node_list1);
        return NULL;
    }

#define free_and_close(X, Y) node_list_free((X)); fclose((Y));
    //Read the file
    while (!feof(file) && !ferror(file)) {

        char ip[MAX_IP_SIZE + 1];
        memset(ip, '\0', MAX_IP_SIZE + 1);

        int port = 0;
        size_t nb_nodes = 0;

        int i = fscanf(file, "%"xstr(MAX_IP_SIZE)"s %d %lu", ip, &port, &nb_nodes);

        //Add the node
        if (i == 3) {

            //Wrong port size
            if (port < 0 || port > MAX_PORT) {
                free_and_close(node_list1, file);
                return NULL;
            }

            //Initialize the nodes
            for (size_t i = 1; i <= nb_nodes; i++) {
				node_t node;
				if (node_init(&node, ip, (uint16_t) port, i) != ERR_NONE) {
					free_and_close(node_list1, file);
					return NULL;
				}

				//Add the node
				if (node_list_add(node_list1, node) != ERR_NONE) {
					free_and_close(node_list1, file);
					return NULL;
				}
			}

        }
            //File has wrong format
        else if (i != EOF) {
            free_and_close(node_list1, file);
            return NULL;
        }

    }

    fclose(file);

    return node_list1;
}
