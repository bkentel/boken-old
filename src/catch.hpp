#if !defined(BK_NO_TESTS)

#if defined(_MSC_VER) && _MSC_FULL_VER >= 190023506
#   define CATCH_CONFIG_CPP11_NULLPTR
#   define CATCH_CONFIG_CPP11_NOEXCEPT
#   define CATCH_CONFIG_CPP11_GENERATED_METHODS
#   define CATCH_CONFIG_CPP11_IS_ENUM
#   define CATCH_CONFIG_CPP11_TUPLE
#   define CATCH_CONFIG_CPP11_LONG_LONG
#   define CATCH_CONFIG_CPP11_OVERRIDE
#   define CATCH_CONFIG_CPP11_UNIQUE_PTR
#   define CATCH_CONFIG_CPP11_OR_GREATER
#   define CATCH_CONFIG_VARIADIC_MACROS
#endif

#include <Catch/catch.hpp>

#endif // BK_NO_TESTS

namespace boken {

void run_unit_tests();

} //namespace boken
