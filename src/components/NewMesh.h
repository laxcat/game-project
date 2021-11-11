#pragma once
#include <vector>
#include <entt/entity/entity.hpp>
#include "../common/types.h"
#include "../common/bgfx_extra.h"


class NewMesh {
public:
    std::vector<bgfx::VertexBufferHandle> vbufs;
    std::vector<bgfx::Memory const *> dvbufRefs;
    std::vector<bgfx::DynamicVertexBufferHandle> dvbufs;
    bgfx::IndexBufferHandle ibuf;
    
    entt::entity nextSibling;

    // template<class Archive>
    // void serialize(Archive & archive) {
    //     archive(vbufs, dvbufs, ibuf, nextSibling);
    // }
};
