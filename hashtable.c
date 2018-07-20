/**
 * @file hashtable.c
 * @brief Implementation of hashtable.h
 *
 */

#include "hashtable.h"
#include <stdlib.h>
#include <stdio.h>
#include "util.h"

typedef struct node node_t;

//Element of a linked list
struct node {
    kv_pair_t elem;
    node_t    *next;
};

//Linked list with a pointer to the first element
struct bucket_t {
    node_t *head;
};

//Create a new node given a key and a value
node_t *create_node(pps_key_t key, pps_value_t value);

//Delete the current node and all its successors in the list
void delete_node(node_t *current);

Htable_t construct_Htable(size_t size) {
    Htable_t table = calloc(1, sizeof(struct Htable_t));
    if (table != NULL) {
        table->elements = calloc(size, sizeof(bucket_t));
        if (table->elements != NULL) {
            table->size = size;
        } else {
            free(table);
            return NULL;
        }
    }
    return table;
}

void kv_pair_free(kv_pair_t *kv) {
    //Free the memory of the pair key/value
    free_const_ptr(kv->key);
    free_const_ptr(kv->value);
    kv->key   = NULL;
    kv->value = NULL;
}

void delete_Htable_and_content(Htable_t *table) {

    if (table == NULL) return;

    //Delete all nodes in the table
    for (size_t i = 0; i < (*table)->size; ++i) {
        delete_node((*table)->elements[i].head);
    }

    free((*table)->elements);
    (*table)->elements = NULL;

    (*table)->size = 0;

    free(*table);
    *table = NULL;

}

node_t *create_node(pps_key_t key, pps_value_t value) {
    node_t *node = calloc(1, sizeof(node));

    if (node != NULL) {
        node->elem.key = strdup(key);
        if (node->elem.key == NULL) {
            free(node);
            return NULL;
        }
        node->elem.value = strdup(value);
        if (node->elem.value == NULL) {
            free_const_ptr(node->elem.value);
            free(node);
            return NULL;
        }
    }

    return node;
}

void delete_node(node_t *current) {
    if (current != NULL) {
        kv_pair_free(&current->elem);

        //Delete the nodes recursively
        delete_node(current->next);
        current->next = NULL;
    }
}


error_code add_Htable_value(Htable_t table, pps_key_t key, pps_value_t value) {

    M_REQUIRE_NON_NULL(table);
    M_REQUIRE_NON_NULL(table->elements);
    M_REQUIRE_NON_NULL(key);
    M_REQUIRE_NON_NULL(value);

    //Find index of the key
    size_t index = hash_function(key, table->size);

    bucket_t *bucket = &(table->elements[index]);

    //First element of the bucket
    if (bucket->head == NULL) {
        node_t *node = create_node(key, value);
        M_REQUIRE_NON_NULL_CUSTOM_ERR(node, ERR_NOMEM);

        bucket->head = node;
        return ERR_NONE;
    }

    //If key already present, just update its value
    for (node_t *current = bucket->head; current != NULL; current = current->next) {
        if (strncmp(current->elem.key, key, strlen(key)) == 0) {

            pps_value_t newValue = strdup(value);
            M_REQUIRE_NON_NULL_CUSTOM_ERR(newValue, ERR_NOMEM);

            free_const_ptr(current->elem.value);
            current->elem.value = newValue;

            return ERR_NONE;
        }
    }

    //Just add a new node to the list
    node_t *node = create_node(key, value);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(node, ERR_NOMEM);

    node->next   = bucket->head;
    bucket->head = node;

    return ERR_NONE;
}


pps_value_t get_Htable_value(Htable_t table, pps_key_t key) {

    M_REQUIRE_NON_NULL_CUSTOM_ERR(table->elements, NULL);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(key, NULL);

    //Find the index of the key
    size_t index = hash_function(key, table->size);

    //Try to find the key in the table
    for (node_t *current = table->elements[index].head; current != NULL; current = current->next) {
        if (strncmp(current->elem.key, key, strlen(key)) == 0) {
            return strdup(current->elem.value);
        }
    }

    return NULL;
}

size_t hash_function(pps_key_t key, size_t size) {
    M_REQUIRE(size != 0, SIZE_MAX, "size == %d", 0);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(key, SIZE_MAX);

    size_t       hash    = 0;
    const size_t key_len = strlen(key);
    for (size_t  i       = 0; i < key_len; ++i) {
        hash += (unsigned char) key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash % size;

}

#define KV_LIST_SIZE HTABLE_SIZE

kv_list_t *create_kv_list() {

    kv_list_t *list = calloc(1, sizeof(kv_list_t));
    if (list != NULL) {
        list->elems = calloc(KV_LIST_SIZE, sizeof(kv_pair_t));

        if (list->elems != NULL) {
            list->allocated = KV_LIST_SIZE;
        }
    }

    return list;
}

error_code get_kv_pair(kv_list_t *list, Htable_t table) {

    M_REQUIRE_NON_NULL(list);
    M_REQUIRE_NON_NULL(table->elements);
    M_REQUIRE(list->allocated != 0, ERR_NOMEM, "%s", "list has no memory allocated");

    for (size_t i = 0; i < table->size; ++i) {
        for (node_t *current = table->elements[i].head; current != NULL; current = current->next) {

            list->size += 1;

            while (list->size >= list->allocated) {
                list->allocated *= 2;
                kv_pair_t *old = list->elems;
                if (list->allocated > SIZE_MAX / sizeof(kv_pair_t) ||
                    (list->elems = realloc(list->elems, list->allocated * sizeof(kv_pair_t))) == NULL) {
                    list->elems = old;
                    list->allocated /= 2;
                    list->size -= 1;
                    return ERR_NOMEM;
                }
            }

            kv_pair_t elem = {NULL, NULL};
            elem.key = strdup(current->elem.key);
            if (elem.key == NULL) {
                return ERR_NOMEM;
            }
            elem.value = strdup(current->elem.value);
            if (elem.value == NULL) {
                free_const_ptr(elem.key);
                return ERR_NOMEM;
            }
            
            list->elems[list->size - 1] = elem;
        }
    }

    return ERR_NONE;

}

void kv_list_free(kv_list_t *list) {
    if (list != NULL) {
        for (size_t i = 0; i < list->size; ++i) {
            kv_pair_free(&list->elems[i]);
        }
        free(list->elems);
        list->elems     = NULL;
        list->size      = 0;
        list->allocated = 0;
    }
}

kv_list_t *get_Htable_content(Htable_t table) {

    M_REQUIRE_NON_NULL_CUSTOM_ERR(table, NULL);

    kv_list_t *list = create_kv_list();
    if (list != NULL) {
        error_code error = get_kv_pair(list, table);
        if (error != ERR_NONE) {
            free(list);
            return NULL;
        }
    }
    return list;

}

error_code del_Htable_key(Htable_t table, pps_key_t key) {

    M_REQUIRE_NON_NULL(table->elements);
    M_REQUIRE_NON_NULL(key);

    //Find the index of the key
    size_t index = hash_function(key, table->size);

    //Try to find the key in the table
    node_t *last = NULL;
    for (node_t *current = table->elements[index].head; current != NULL;
         last = current, current = current->next) {
        if (strncmp(current->elem.key, key, strlen(key)) == 0) {
            last->next    = current->next;
            current->next = NULL;
            delete_node(current);
            return ERR_NONE;
        }
    }

    return ERR_NONE;
}
 

 
 
		

