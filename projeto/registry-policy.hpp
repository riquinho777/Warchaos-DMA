// C++17 / registry-policy.hpp / bounded discovery retention; samples still validate identity and life.
#pragma once
#include <cstdint>
#include <vector>
#include <unordered_map>
template<class Node> class CandidateRegistry {
    struct Entry{Node node;std::uint64_t seen;};
    std::unordered_map<std::uint32_t,Entry> entries;
    std::uint64_t system=0;
public:
    void clear(){entries.clear();system=0;}
    void update(std::uint64_t nextSystem,const std::vector<Node>& next,std::uint64_t now){
        if(system!=nextSystem){clear();system=nextSystem;}
        for(const auto& node:next){
            for(auto i=entries.begin();i!=entries.end();){
                if(i->first!=node.id&&(i->first&0xFFFF)==(node.id&0xFFFF))i=entries.erase(i);else ++i;
            }
            entries.insert_or_assign(node.id,Entry{node,now});
        }
        for(auto i=entries.begin();i!=entries.end();){
            if(now<i->second.seen||now-i->second.seen>3000)i=entries.erase(i);else ++i;
        }
    }
    std::vector<Node> snapshot()const{std::vector<Node> result;for(const auto& entry:entries)result.push_back(entry.second.node);return result;}
};
