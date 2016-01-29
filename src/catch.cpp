#if !defined(BK_NO_TESTS)
#   define CATCH_CONFIG_RUNNER
#   include "catch.hpp"

void boken::run_unit_tests() {
    Catch::Session().run();
}

#else

void boken::run_unit_tests() {

}
#endif //BK_NO_TESTS
