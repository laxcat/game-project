#pragma once
#include <cereal/cereal.hpp>


struct Info {
    char name[16];

    Info() : Info("") {}

    Info(char const * name) {
        strcpy(this->name, name);
    }

    template<class Archive>
    void serialize(Archive & archive) {
        archive(name);
    }
};


    

