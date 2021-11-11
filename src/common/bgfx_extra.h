#pragma once


template<class Archive>
void serialize(Archive & archive, bgfx::VertexBufferHandle & m) { archive(m.idx); } 
template<class Archive>
void serialize(Archive & archive, bgfx::DynamicVertexBufferHandle & m) { archive(m.idx); } 
template<class Archive>
void serialize(Archive & archive, bgfx::IndexBufferHandle & m) { archive(m.idx); } 

