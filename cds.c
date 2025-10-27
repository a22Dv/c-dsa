#include "ds/cmap.h"

#include <stdio.h>

int main() {
    printf("TEST\n");
    cmap_t *map = NULL;
    CMAP_INIT(map, char *, char *, 0, cmap_strhash, cmap_strcmp, NULL, NULL);
    char *kstr[10] = {
        "key1", "key2", "key3", "key4", "key5", "key6", "key7", "key8", "key9", "key10",
    };
    char *vstr[10] = {"test1", "test2", "test3", "test4", "test5",
                      "test6", "test7", "test8", "test9", "test10"};
    printf("INSERT\n");
    for (int i = 0; i < 10; ++i) {
        CMAP_INSERT(map, kstr[i], vstr[i]);
    }
    printf("GET\n");
    for (int i = 0; i < 10; ++i) {
        char* val = NULL;
        CMAP_GETVAL(map, kstr[i], val);
        printf("%s\n", val);
    }
    for (int i = 0; i < CMAP_CAPACITY(map); ++i) {
        printf("BKT%d: %zu\n", i, (size_t)CMAP_BUCKETS(map)[i]._total_entries);
    }
}