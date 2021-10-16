//
// Generated by CMAKE. DO NOT EDIT.
// Edit Components enum in src/components/_list.h
//

#pragma once
#include "Info.h"
#include "Mesh.h"


enum class Components {
    Info = 1,
    Mesh = 3,
    
};


inline constexpr int componentCount = 2; 
inline constexpr char const * allComponents[componentCount] = {
    "Info",
    "Mesh",
    
};


inline int componentId(char const * name) {
    if (strcmp(name, allComponents[0]) == 0) return 1;
    if (strcmp(name, allComponents[1]) == 0) return 3;
    
    return 0;
}

inline int componentIndex(char const * name) {
    if (strcmp(name, allComponents[0]) == 0) return 0;
    if (strcmp(name, allComponents[1]) == 0) return 1;
    
    return INT_MAX;
}

inline void emplaceComponentByName(char const * name, entt::registry & r, entt::entity const e) {
    if (strcmp(name, allComponents[0]) == 0) r.emplace<Info>(e);
    if (strcmp(name, allComponents[1]) == 0) r.emplace<Mesh>(e);
    
}

inline void emplaceComponentByIndex(int index, entt::registry & r, entt::entity const e) {
    if (index == 0) r.emplace<Info>(e);
    if (index == 1) r.emplace<Mesh>(e);
    
}

inline void emplaceComponentById(int id, entt::registry & r, entt::entity const e) {
    if (index == 1) r.emplace<Info>(e);
    if (index == 3) r.emplace<Mesh>(e);
    
}
