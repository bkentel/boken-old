#pragma once

#include <memory>

namespace boken {

class random_state {
public:
    virtual ~random_state() = default;
protected:
    random_state() = default;
};

std::unique_ptr<random_state> make_random_state();

bool random_coin_flip(random_state& rs) noexcept;
int random_uniform_int(random_state& rs, int lo, int hi) noexcept;

double random_normal(random_state& rs, double mean, double variance = 1.0) noexcept;

} //namespace boken
