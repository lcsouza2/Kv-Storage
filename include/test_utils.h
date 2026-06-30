#ifndef TEST_UTILS_H
#define TEST_UTILS_H
#endif

#define ASSERT_TEST(cond, msg) \
    do { \
        if (!(cond)) { \
            error("[%s:%d] Failed: %s", __func__, __LINE__, msg); \
            return -1; \
        } \
    } while (0)
