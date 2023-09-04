#pragma once

#include <string>
#include <functional>

namespace util {

/**
 *  Вспомогательный класс case insensitive string 
 *  Класс позволяет использовать строку в ассоциативных контейнерах 
 *  с операциями сравнения строк без учета регистра
 * 
**/

struct ci_char_traits : public std::char_traits<char> {
    static constexpr bool eq(char c1, char c2) { return toupper(c1) == toupper(c2); }
    static constexpr bool lt(char c1, char c2) { return toupper(c1) <  toupper(c2); }
    static constexpr int compare(const char* s1, const char* s2, size_t n) {
        while( n-- != 0 ) {
            if( toupper(*s1) < toupper(*s2) ) return -1;
            if( toupper(*s1) > toupper(*s2) ) return 1;
            ++s1; ++s2;
        }
        return 0;
    }
    static constexpr const char* find(const char* s, int n, char a) {
        while( n-- > 0 && toupper(*s) != toupper(a) ) {
            ++s;
        }
        return s;
    }
};

using ci_string = std::basic_string<char, ci_char_traits>;

inline ci_string to_ci_string(std::string const& src)
{
    return ci_string(src.begin(), src.end());
}

inline ci_string to_ci_string(std::string_view src)
{
    return ci_string(src.begin(), src.end());
}

inline std::string to_string(std::string_view src)
{
    return {src.data(), src.size()};
}


} // namespace util

//
//  специализация std::hash для ci_string - чтобы ее можно было
//  без манипуляций использовать как ключ в хэш таблице
//
template<>
struct std::hash<util::ci_string>
{
    std::size_t operator()(util::ci_string const& str) const noexcept
    {
#if defined(_M_AMD64) || defined(_M_X64) || defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(__arch64__)
      const size_t FNV_prime = 1099511628211ULL;
      const size_t offset_basis = 14695981039346656037ULL;
#else
      const size_t FNV_prime = 16777619;
      const size_t offset_basis = 2166136261;
#endif
      size_t hash = offset_basis;
      for (util::ci_string::const_iterator it = str.begin(); it != str.end(); ++it)
      {
        char ch = tolower(*it);
        hash ^= static_cast<size_t>(ch);
        hash *= FNV_prime;
      }
      return hash;    
    }
};
