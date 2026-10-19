// C++17 / frame-policy.hpp / Windows x64 / bounded transient frame retention.
#pragma once
#include <cstdint>
inline bool canReuseFrame(bool transient,std::uint64_t previous,std::uint64_t now){
    return transient&&previous!=0&&now>=previous&&now-previous<=60;
}
