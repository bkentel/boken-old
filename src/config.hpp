#pragma once

#if defined(BK_USE_BOOST_STRING_VIEW)
#   include <boost/version.hpp>
#   if BOOST_VERSION > 106000
#   include <boost/utility/string_view.hpp>
    namespace boken {
        using string_view = boost::string_view;
    }
#   else
#   include <boost/utility/string_ref.hpp>
    namespace boken {
        using string_view = boost::string_ref;
    }
#   endif
#elif defined(BK_USE_STD_EXP_STRING_VIEW)
#   include <experimental/string_view>
    namespace boken {
        using string_view = std::experimental::string_view;
    }
#elif defined(BK_USE_STD_STRING_VIEW)
#   include <string_view>
    namespace boken {
        using string_view = std::string_view;
    }
#else
#   error Please define a string_view implementation to use.
#endif

#if defined(_MSC_VER) && (_MSC_FULL_VER > 190023918)
#   define BK_ATTRIBUTE_FALLTHROUGH [[fallthrough]]
#elif defined(__clang__)
#   if __has_cpp_attribute(fallthrough)
#       define BK_ATTRIBUTE_FALLTHROUGH [[fallthrough]]
#   elif __has_cpp_attribute(clang::fallthrough)
#       define BK_ATTRIBUTE_FALLTHROUGH [[clang::fallthrough]]
#   endif
#else
#   define BK_ATTRIBUTE_FALLTHROUGH
#endif

#if defined(__clang__)
#   define BK_DISABLE_WSWITCH_ENUM_BEGIN \
        _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Wswitch-enum\"")
#   define BK_DISABLE_WSWITCH_ENUM_END \
        _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
#   define BK_DISABLE_WSWITCH_ENUM_BEGIN \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Wswitch-enum\"")
#   define BK_DISABLE_WSWITCH_ENUM_END \
        _Pragma("GCC diagnostic pop")
#else
#   define BK_DISABLE_WSWITCH_ENUM_BEGIN
#   define BK_DISABLE_WSWITCH_ENUM_END
#endif
