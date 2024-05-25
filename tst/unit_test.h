#ifndef MINUNIT_H_
#define MINUNIT_H_


#define run_test(test_func)  \
    do {                     \
        if (!(test_func)) {  \
            return false;    \
        }                    \
    } while (0)


#define expect_true(message, test)                         \
    do {                                                   \
        if (!(test)) {                                     \
            std::cout << "Expected true, but got false: "  \
                << message << ". At: " << __FILE__ << ":"  \
                << __LINE__ << std::endl;                  \
            return false;                                  \
        }                                                  \
    } while (0)


#define fail(message)                        \
    std::cout << "Test failed: " << message  \
        << ". At: " << __FILE__ << ":"       \
        << __LINE__ << std::endl;            \
    return false;


#endif
