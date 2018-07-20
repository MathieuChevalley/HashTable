/**
 * @file args.c
 * @brief Implementation of args.h
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "args.h"
#include "error.h"

#define DEFAULT_N 3
#define DEFAULT_W 2
#define DEFAULT_R 2;

#define increment(X) do { if(**(X) != NULL) ++(*(X));}while(0);

args_t *parse_opt_args(size_t supported_args, char ***rem_argv) {

    M_REQUIRE_NON_NULL_CUSTOM_ERR(rem_argv, NULL);
    M_REQUIRE_NON_NULL_CUSTOM_ERR(*rem_argv, NULL);

    args_t *result = malloc(sizeof(args_t));
    M_REQUIRE_NON_NULL_CUSTOM_ERR(result, NULL);

    //Default values
    result->N = DEFAULT_N;
    result->W = DEFAULT_W;
    result->R = DEFAULT_R;

    int cont = 1;

    while (cont && **rem_argv != NULL && strncmp(**rem_argv, "--", 2) != 0) {


        if (supported_args & TOTAL_SERVERS && strncmp(**rem_argv, "-n", 2) == 0) {

            increment(rem_argv);
            if (**rem_argv == NULL || sscanf(**rem_argv, "%zu", &result->N) != 1) {
                free(result);
                return NULL;
            }
            increment(rem_argv);

        } else if (supported_args & PUT_NEEDED && strncmp(**rem_argv, "-w", 2) == 0) {

            increment(rem_argv);
            if (**rem_argv == NULL || sscanf(**rem_argv, "%zu", &result->W) != 1) {
                free(result);
                return NULL;
            }
            increment(rem_argv);

        } else if (supported_args & GET_NEEDED && strncmp(**rem_argv, "-r", 2) == 0) {

            increment(rem_argv);
            if (**rem_argv == NULL || sscanf(**rem_argv, "%zu", &result->R) != 1) {
                free(result);
                return NULL;
            }
            increment(rem_argv);

        } else {
            cont = 0;
        }

    }

    // Unless they come after "--", "-n" "w" and "r" should be treated as options
    if (**rem_argv != NULL && strncmp(**rem_argv, "--", 2) == 0) {
        increment(rem_argv);
    }

    //Check if there is no error
    if (supported_args & TOTAL_SERVERS && supported_args & PUT_NEEDED && result->W > result->N) {
        free(result);
        return NULL;
    }

    if (supported_args & TOTAL_SERVERS && supported_args & GET_NEEDED && result->R > result->N) {
        free(result);
        return NULL;
    }


    return result;

}
