#pragma once
// Precompailed headers
#include <condition_variable>
#include <initializer_list>
#include <unordered_set>
#include <xmmintrin.h>
#include <functional>
#include <filesystem>
#include <exception>
#include <execution>
#include <iostream>
#include <fstream>
#include <variant>
#include <sstream>
#include <vector>
#include <limits>
#include <format>
#include <string>
#include <future>
#include <thread>
#include <memory>
#include <atomic>
#include <random>
#include <regex>
#include <queue>
#include <array>
#include <map>
#include <set>
#include <optional>

#include <typeindex>
#include <typeinfo>

// TODO: remove additional
#include <algorithm>
#include <cassert>
#include <type_traits>

template <typename T>
using Ref = std::shared_ptr<T>;

template <typename T, typename... Args>
std::shared_ptr<T> CreateRef(Args &&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T>
using Scope = std::unique_ptr<T>;

template <typename T, typename... Args>
std::unique_ptr<T> CreateScope(Args &&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

#include <core/log/log.h>

#ifndef SKY_INLINE
    #define SKY_INLINE __forceinline
#endif // SKY_INLINE

#define SKY_BIND_EVENT_FN(fn)                                                                                           \
    [this](auto &&...args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }
