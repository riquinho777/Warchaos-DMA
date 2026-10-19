// C++17 / team-policy.hpp / explicit match mode filtering.
#pragma once
#include <cstdint>
inline bool enemyInMode(bool freeForAll,std::uint32_t local,std::uint32_t target,std::uint32_t localTeam,std::uint32_t targetTeam){
    if(!local||!target||target==local)return false;
    return freeForAll||(localTeam&&targetTeam&&localTeam!=targetTeam);
}
