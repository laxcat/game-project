#pragma once
#include <cereal/cereal.hpp>


struct Mesh {
    char name[16];

    Mesh() : Mesh("") {}

    Mesh(char const * name) {
        strcpy(this->name, name);
    }

    template<class Archive>
    void serialize(Archive & archive) {
        archive(name);
    }
};
