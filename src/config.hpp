#pragma once

#include <boost/utility/string_ref.hpp>
#include <boost/version.hpp>

#if defined(BK_USE_BOOST_STRING_VIEW)
#   include <boost/version.hpp>
#   include <boost/utility/string_ref.hpp>
#   if BOOST_VERSION > 106000
    namespace boken {
        using string_view = boost::string_view;
    }
#   else
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
