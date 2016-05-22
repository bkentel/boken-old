#include "random.hpp"

#include <boost/predef/other/endian.h>

#if defined (BOOST_ENDIAN_LITTLE_BYTE_AVAILABLE)
#   define PCG_LITTLE_ENDIAN 1
#endif

#include <pcg_random.hpp>
#include <pcg_extras.hpp>

#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_smallint.hpp>
#include <boost/random/normal_distribution.hpp>

#include <random>

namespace boken {

random_state::~random_state() = default;
uint32_t random_state::min() noexcept { return pcg32::min(); }
uint32_t random_state::max() noexcept { return pcg32::max(); }

class random_state_impl final : public random_state {
public:
    random_state_impl() = default;

    result_type generate() noexcept final override;

    boost::random::uniform_smallint<int32_t>         dist_coin    {0, 1};
    boost::random::uniform_int_distribution<int32_t> dist_uniform {};
    boost::random::normal_distribution<>             dist_normal  {};

    pcg32 state {};
};

random_state::result_type random_state_impl::generate() noexcept {
    return state();
}

std::unique_ptr<random_state> make_random_state() {
    return std::make_unique<random_state_impl>();
}

bool random_coin_flip(random_state& rng) noexcept {
    auto& r = reinterpret_cast<random_state_impl&>(rng);
    return !!r.dist_coin(r.state);
}

int32_t random_uniform_int(random_state& rng, int32_t const lo, int32_t const hi) noexcept {
    auto& r = reinterpret_cast<random_state_impl&>(rng);

    using param_t = decltype(r.dist_uniform)::param_type;

    r.dist_uniform.param(param_t {lo, hi});

    return r.dist_uniform(r.state);
}

bool random_chance_in_x(random_state& rng, int32_t const num, int32_t const den) noexcept {
    return random_uniform_int(rng, 0, den - 1) < num;
}

double random_normal(random_state& rs, double const m, double const v) noexcept {
    auto& r = reinterpret_cast<random_state_impl&>(rs);

    using param_t = decltype(r.dist_normal)::param_type;

    r.dist_normal.param(param_t {m, v});

    return r.dist_normal(r.state);
}

uint32_t random_color(random_state& rng) noexcept {
    auto const random_color_comp = [&]() noexcept {
        return static_cast<uint32_t>(random_uniform_int(rng, 0, 255));
    };

    return 0xFFu               << 24
         | random_color_comp() << 16
         | random_color_comp() << 8
         | random_color_comp() << 0;
}

} //namespace boken
