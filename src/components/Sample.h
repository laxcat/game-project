#pragma once
#include <cereal/cereal.hpp>


struct Sample {
    char name[16];

    Sample() : Sample("") {}

    Sample(char const * name) {
        strcpy(this->name, name);
    }

    template<class Archive>
    void serialize(Archive & archive) {
        archive(name);
    }
};
