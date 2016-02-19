#pragma once

#include <memory>
#include <cstdint>

namespace boken {

class random_state {
public:
    virtual ~random_state();
protected:
    random_state() = default;
};

std::unique_ptr<random_state> make_random_state();

bool random_coin_flip(random_state& rng) noexcept;
bool random_chance_in_x(random_state& rng, int32_t num, int32_t den) noexcept;
int  random_uniform_int(random_state& rng, int32_t lo, int32_t hi) noexcept;

double random_normal(random_state& rng, double mean, double variance = 1.0) noexcept;

} //namespace boken
