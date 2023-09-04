#pragma once

#include <random>
#include <string>
#include <string_view>
#include <optional>

#include "tagged.h"



// namespace detail {
// struct TokenTag {};
// }  // namespace detail


namespace app {

struct TokenTag {};
using Token = util::Tagged<std::string, TokenTag>;

static constexpr size_t TOKEN_SIZE = 32;

std::optional<Token> ParseBearerToken(std::string_view bearer);

class PlayerTokens {
public:
    static Token MakeToken();

private:
    PlayerTokens() = default;

    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
};

} // namespace app