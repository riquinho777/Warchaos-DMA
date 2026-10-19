// C++17 / health.hpp / Windows x64 / CActor GetHealth value decoding.
#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
enum class LifeState{Unknown,Dead,Alive};
inline LifeState decodeLife(std::uint32_t encoded,std::uint32_t key){
    if(!key)return LifeState::Dead;
    const std::uint32_t bits=encoded^key;float value=0;std::memcpy(&value,&bits,4);
    if(!std::isfinite(value)||std::fabs(value)>100000)return LifeState::Unknown;
    return value>0?LifeState::Alive:LifeState::Dead;
}
