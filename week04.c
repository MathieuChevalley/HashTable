/**
 * @file test-hashtable.c
 * @brief test code for hashtables
 *
 * @author Valérian Rousset & Jean-Cédric Chappelier
 * @date 02 Oct 2017
 */

#include <stdio.h> // for puts(). to be removed when no longer needed.

#include <check.h>

#include "tests.h"
#include "hashtable.h"

START_TEST(add_value_does_retrieve_same_value)
{
    puts("add_value does retrieve same value?");
    Htable_t table;
    const pps_key_t cle = 'c';
    const pps_value_t valeur_mise = 42;
    add_Htable_value(table, cle, valeur_mise);
    const pps_value_t valeur_lue = get_Htable_value(table, cle);
    ck_assert_int_eq(valeur_mise, valeur_lue);
    
    puts("add_value overwrites if same key?");
    const pps_value_t valeur_mise2 = 32;
    add_Htable_value(table, cle, valeur_mise2);
    const pps_value_t valeur_lue2 = get_Htable_value(table, cle);
    ck_assert_int_eq(valeur_mise2, valeur_lue2);
       
}
END_TEST



Suite *hashtable_suite()
{

    Suite *s = suite_create("hashtable.h");

    TCase *tc_ht = tcase_create("hashtable");
    suite_add_tcase(s, tc_ht);

    tcase_add_test(tc_ht, add_value_does_retrieve_same_value);

    return s;
}

TEST_SUITE(hashtable_suite)

