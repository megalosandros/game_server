#include "../sdk.h"
#include <sstream>
#include <iomanip>

#include "player_tokens.h"

namespace app {

using namespace std::literals;

std::optional<Token> ParseBearerToken(std::string_view bearer)
{
    static constexpr auto BEARER = "Bearer "sv;

    if (!bearer.starts_with(BEARER)) {
        return std::nullopt;
    }

    bearer.remove_prefix(BEARER.size());

    if (bearer.size() != TOKEN_SIZE) {
        return std::nullopt;
    }

    return Token(std::string(bearer.data(), bearer.size()));
}

/*static*/ Token PlayerTokens::MakeToken()
{
    //
    // Чтобы сгенерировать токен, получите из generator1_ и generator2_
    // два 64-разрядных числа и, переведя их в hex-строки, склейте в одну.
    // Вы можете поэкспериментировать с алгоритмом генерирования токенов,
    // чтобы сделать их подбор ещё более затруднительным
    //

    static PlayerTokens playerTokens;

    std::stringstream ss;

    auto r1 = playerTokens.generator1_();
    auto r2 = playerTokens.generator2_();

    ss << std::hex << std::setfill('0') << std::setw(TOKEN_SIZE/2) << r1 << std::setw(TOKEN_SIZE/2) << r2;

    return Token{ ss.str() };

}

} // namespace app