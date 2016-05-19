#pragma once

#include <memory>
#include <cstdint>

namespace boken {
    template <typename Weight, typename Result> class weight_list;
}

namespace boken {

class random_state {
public:
    using result_type = uint32_t;

    virtual ~random_state();

    static result_type min() noexcept;
    static result_type max() noexcept;
    virtual result_type generate() noexcept = 0;

    result_type operator()() noexcept {
        return generate();
    }
};

std::unique_ptr<random_state> make_random_state();

//===------------------------------------------------------------------------===
//                          Primitive algorithms
//===------------------------------------------------------------------------===

bool random_coin_flip(random_state& rng) noexcept;
bool random_chance_in_x(random_state& rng, int32_t num, int32_t den) noexcept;
int32_t random_uniform_int(random_state& rng, int32_t lo, int32_t hi) noexcept;

double random_normal(random_state& rng, double mean, double variance = 1.0) noexcept;

//===------------------------------------------------------------------------===
//                          Derivative algorithms
//===------------------------------------------------------------------------===
uint32_t random_color(random_state& rng) noexcept;

template <typename Weight, typename Result>
Result const& random_weighted(random_state& rng, weight_list<Weight, Result> const& weights) noexcept {
    auto const roll = random_uniform_int(rng, 0, weights.max() - 1);
    return weights[roll];
}

} //namespace boken
