#include <check.h>
#include <stdlib.h>
#include <stdint.h>
#include "Patches/Common/GameAPI/CExoArrayList.h"

START_TEST(test_allocate_memcpy_bounds)
{
    // Invariant: memcpy byte count must not exceed allocated buffer size
    const struct {
        int initial_capacity;
        int initial_size;
        int new_capacity;
    } payloads[] = {
        // Exploit case: overflow causing oversized copy
        {10, INT_MAX / sizeof(int) + 1, 5},
        // Boundary case: zero capacity
        {5, 3, 0},
        // Valid case: normal resize
        {5, 3, 10}
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        CExoArrayList<int> list;
        
        // Initialize list with given capacity
        list.Allocate(payloads[i].initial_capacity);
        
        // Set size to initial_size (simulating populated elements)
        // Note: This assumes SetSizeInternal is accessible or we use Add()
        for (int j = 0; j < payloads[i].initial_size && j < payloads[i].initial_capacity; j++) {
            list.Add(0);
        }
        
        // This should either succeed safely or fail gracefully without buffer overflow
        list.Allocate(payloads[i].new_capacity);
        
        // Post-condition: list should be in valid state
        ck_assert(list.GetCapacity() >= 0);
        ck_assert(list.GetSize() >= 0);
        ck_assert(list.GetSize() <= list.GetCapacity());
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_allocate_memcpy_bounds);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}