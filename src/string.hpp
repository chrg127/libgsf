#pragma once

#include <algorithm>
#include <cstring>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <limits>

namespace string {

/*
 * Replacements for isspace, isalpha, isdigit and tolower. These exist because
 * the versions from the C standard library use locales.
 */
inline bool is_space(char c) { return c == ' ' || c == '\t' || c == '\r'; }
inline bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
inline bool is_digit(char c) { return c >= '0' && c <= '9'; }
template <typename T> T to_lower(T c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; }

/* Splits a string into multiple strings, lambda version. */
template <typename From = std::string>
inline void split(const From &s, char delim, auto &&fn)
{
    for (std::size_t i = 0, p = 0; i != s.size(); i = p+1) {
        p = s.find(delim, i);
        fn(s.substr(i, (p == s.npos ? s.size() : p) - i));
        if (p == s.npos)
            break;
    }
}

/* Splits a string into multiple strings, vector version. */
template <typename From = std::string, typename To = std::string>
inline std::vector<To> split(const From &s, char delim = ',')
{
    std::vector<To> res;
    split<From>(s, delim, [&] (const auto &substr) {
        res.emplace_back(substr);
    });
    return res;
}

/* Helper for split() with std::string_view. */
inline std::vector<std::string_view> split_view(std::string_view s, char delim = ',')
{
    return split<std::string_view, std::string_view>(s, delim);
}

/*
 * Splits a single string in multiple lines of maximum `col` width. Tries to
 * split at word boundary, where a word is simply a part of a string delimited
 * by spaces.
 */
template <typename From = std::string, typename To = std::string>
inline std::vector<To> split_lines(const From &s, std::size_t col)
{
    std::vector<To> result;
    auto it = s.begin();
    while (it != s.end()) {
        it = std::find_if_not(it, s.end(), is_space);
        auto start = it;
        it += std::min(size_t(s.end() - it), size_t(col));
        it = std::find_if(it, s.end(), is_space);
        result.emplace_back(start, it);
    }
    return result;
}

/* Removes space on start and end of a string, pure version. */
template <typename From = std::string, typename To = std::string>
inline To trim(const From &s)
{
    auto i = std::find_if_not(s.begin(),  s.end(),  is_space);
    auto j = std::find_if_not(s.rbegin(), s.rend(), is_space).base();
    return {i, j};
}

/* Helper for trim() with std::string_view. */
inline std::string_view trim_view(std::string_view s) { return trim<std::string_view, std::string_view>(s); }

/* Removes space on start and end of a string, in-place version. */
template <typename T = std::string>
inline void trim_in_place(T &s)
{
    auto j = std::find_if_not(s.rbegin(), s.rend(), is_space).base();
    s.erase(j, s.end());
    auto i = std::find_if_not(s.begin(), s.end(), is_space);
    s.erase(s.begin(), i);
}

namespace detail {

template <typename T>
std::from_chars_result from_chars_double(const char *first, const char *, double &value)
{
    char *endptr;
    value = strtod(first, &endptr);
    std::from_chars_result res;
    res.ptr = endptr,
    res.ec = static_cast<std::errc>(errno);
    return res;
}

} // namespace detail

/*
 * Converts a string to number using std::from_chars and returning the result
 * in an std::optional. Works for integer and floating-point numbers.
 */
template <typename T = int, typename TStr = std::string>
inline std::optional<T> to_number(const TStr &str, unsigned base = 10)
    requires std::is_integral_v<T> || std::is_floating_point_v<T>
{
    auto helper = [](const char *start, const char *end, unsigned base) -> std::optional<T> {
        T value = 0;
        std::from_chars_result res;
        if constexpr(std::is_floating_point_v<T>)
            // GCC version < 11 doesn't have std::from_chars<double>, and this version
            // is still installed in some modern distros (debian stable, WSL ubuntu)
            res = detail::from_chars_double<T>(start, end, value);
        else
            res = std::from_chars(start, end, value, base);
        if (res.ec != std::errc() || res.ptr != end)
            return std::nullopt;
        return value;
    };
    if constexpr(std::is_same<std::decay_t<TStr>, char *>::value)
        return helper(str, str + std::strlen(str), base);
    else
        return helper(str.data(), str.data() + str.size(), base);
}

/* Converts a number to string, using std::to_chars. */
template <typename TStr = std::string, typename T = int>
inline TStr from_number(const T &n, int base = 10)
{
    constexpr auto maxbuf = std::numeric_limits<T>::digits10 + 1 + std::is_signed<T>::value;
    std::array<char, maxbuf> buf;
    auto res = std::to_chars(buf.data(), buf.data() + buf.size(), n, base);
    auto str = TStr(buf.data(), res.ptr);
    return str;
}

/* Insensitive comparison of two string. */
template <typename T>
bool iequals(const T& a, const T& b)
{
    return a.size() == b.size()
        && std::equal(a.begin(), a.end(), b.begin(), [&](auto ac, auto bc) {
            return to_lower(ac) == to_lower(bc);
        });
}

} // namespace string
