#pragma once
#include <glm/vec3.hpp>
#include "../shader/shared_defines.h"
#include "../common/utils.h"
#include "../dev/print.h"


// The lighting uniforms and data
// Supports point lights and directional lights (see shared_defines.h for limits)
// Point lights are often refered to as by "id", which are app-lifetime unique 
// and never reissued. Don't confuse it with "index" which is the actual 
// location in the data array.


static constexpr bool LightsShowPrint = false;

class Lights {
public:
    // data structurs used to send to uniforms
    // directional data
    glm::vec2 dirDataDirAsEuler[MAX_DIRECTIONAL_LIGHTS];
    glm::vec4 dirDataDir[MAX_DIRECTIONAL_LIGHTS];
    glm::vec4 dirDataStrength[MAX_DIRECTIONAL_LIGHTS]; // ambient, diffuse, specular, overall
    float & dirStrengthAmbientAt (size_t index) { return dirDataStrength[index][0]; }
    float & dirStrengthDiffuseAt (size_t index) { return dirDataStrength[index][1]; }
    float & dirStrengthSpecularAt(size_t index) { return dirDataStrength[index][2]; }
    float & dirStrengthOverallAt (size_t index) { return dirDataStrength[index][3]; }
    glm::vec4 dirDataColor[MAX_DIRECTIONAL_LIGHTS];
    // point data
    glm::vec4 pointDataPosAndRadius[MAX_POINT_LIGHTS];
    glm::vec3 & pointPosAt(size_t index) { return *((glm::vec3 *)&pointDataPosAndRadius[index][0]); }
    float & pointRadiusAt(size_t index) { return pointDataPosAndRadius[index][3]; }
    glm::vec4 pointDataStrength[MAX_POINT_LIGHTS]; // ambient, diffuse, specular, overall
    float & pointStrengthAmbientAt (size_t index) { return pointDataStrength[index][0]; }
    float & pointStrengthDiffuseAt (size_t index) { return pointDataStrength[index][1]; }
    float & pointStrengthSpecularAt(size_t index) { return pointDataStrength[index][2]; }
    float & pointStrengthOverallAt (size_t index) { return pointDataStrength[index][3]; }
    glm::vec4 pointDataColor[MAX_POINT_LIGHTS];
    glm::vec3 & pointColorAt(size_t index) { return *((glm::vec3 *)&pointDataColor[index][0]); }
    // additional data
    glm::vec4 extraData; // dir count, point count, point atten cutoff, (not used)
    float & pointAttenCutoff = extraData[2];

    // uniform handles
    bgfx::UniformHandle dirDir;
    bgfx::UniformHandle dirStrength;
    bgfx::UniformHandle dirColor;
    bgfx::UniformHandle pointPos;
    bgfx::UniformHandle pointStrength;
    bgfx::UniformHandle pointColor;
    bgfx::UniformHandle extra;

    // counts
    size_t directionalCount;
    size_t pointCount;

    // caches and temps
    // stores current stregth settings each frame so they can be easily restored after ignoring lights
    float pointDataOverallStrength[MAX_POINT_LIGHTS]; 


    void init() {
        directionalCount = 1;
        pointCount = 0;

        resetDirectional(0);

        pointAttenCutoff = 0.5;

        dirDir        = bgfx::createUniform("u_dirLightDir",        bgfx::UniformType::Vec4, MAX_DIRECTIONAL_LIGHTS);
        dirStrength   = bgfx::createUniform("u_dirLightStrength",   bgfx::UniformType::Vec4, MAX_DIRECTIONAL_LIGHTS);
        dirColor      = bgfx::createUniform("u_dirLightColor",      bgfx::UniformType::Vec4, MAX_DIRECTIONAL_LIGHTS);
        pointPos      = bgfx::createUniform("u_pointLightPos",      bgfx::UniformType::Vec4, MAX_POINT_LIGHTS);
        pointStrength = bgfx::createUniform("u_pointLightStrength", bgfx::UniformType::Vec4, MAX_POINT_LIGHTS);
        pointColor    = bgfx::createUniform("u_pointLightColor",    bgfx::UniformType::Vec4, MAX_POINT_LIGHTS);
        extra         = bgfx::createUniform("u_lightExtra",         bgfx::UniformType::Vec4);

        updateGlobalFromEuler();
    }

    void shutdown() {
        bgfx::destroy(dirDir);
        bgfx::destroy(dirStrength);
        bgfx::destroy(dirColor);
        bgfx::destroy(pointPos);
        bgfx::destroy(pointStrength);
        bgfx::destroy(pointColor);
        bgfx::destroy(extra);
    }

    void resetDirectional(size_t index) {
        dirDataDirAsEuler[index] = {M_PI/8, M_PI/8};
        dirDataStrength[index] = {
            0.2f, // ambient
            0.5f, // diffuse
            0.5f, // specular
            1.0f  // overall
        };
        dirDataColor[index] = {1.f, 1.f, 1.f, 1.f};
    }

    void resetPoint(size_t index) {
        pointDataPosAndRadius[index] = {
            0.0f, 2.0f, 0.0f,   // pos
            5.0f                // radius
        };
        pointDataStrength[index] = {
            0.2f, // ambient
            0.5f, // diffuse
            0.5f, // specular
            0.5f  // overall
        };
        pointDataColor[index] = {1.0f, 0.0f, 0.0f, 1.0f};
    }

    void preDraw() {
        if (directionalCount) {
            bgfx::setUniform(dirDir,        &dirDataDir,        directionalCount);
            bgfx::setUniform(dirStrength,   &dirDataStrength,   directionalCount);
            bgfx::setUniform(dirColor,      &dirDataColor,      directionalCount);
        }
        if (pointCount) {
            bgfx::setUniform(pointPos,      &pointDataPosAndRadius, pointCount);
            bgfx::setUniform(pointColor,    &pointDataColor,        pointCount);

            // point-light strength gets set right before each mesh draw with 
            // ignorePointLights. so we don't call it here because bgfx 
            // complains if you send the same uniform twice before an acutal 
            // draw, and the mesh draw might get skipped if it isn't loaded yet
            //
            // bgfx::setUniform(pointStrength, &pointDataStrength, pointCount);

            // cache point strength every frame to restore after ignored lights
            for (size_t i = 0; i < MAX_POINT_LIGHTS; ++i) {
                pointDataOverallStrength[i] = pointStrengthOverallAt(i);
            }
        }

        extraData[0] = (float)directionalCount;
        extraData[1] = (float)pointCount;
        bgfx::setUniform(extra, &extraData);
    }

    void postDraw() {
        restorePointStrength();
    }

    void sendPointLightStrength(size_t count) {
        printc(LightsShowPrint, "sending to point strength uniform:\n");
        if constexpr (LightsShowPrint) { for (size_t i = 0; i < count; ++i) {
            printPointLight(pointIdByIndex[i]);
        }}
        bgfx::setUniform(pointStrength, &pointDataStrength, count);
    }

    // 
    // returns index of highest altered light
    size_t ignorePointLights(std::vector<size_t> const & ignoreLights) {
        if (pointCount == 0) return 0;

        size_t count = 0;

        for (size_t pointId : ignoreLights) {
            if (pointIndexById.count(pointId) == 0) continue;
            size_t pointIndex = pointIndexById[pointId];
            // it only counts if we actually change the light! It might have been
            // set already by another renderable.
            if (count < pointIndex+1 && pointStrengthOverallAt(pointIndex) != 0) {
                printc(LightsShowPrint, "ignoring point light at %zu (id:%zu)\n", pointIndex, pointId);
                count = pointIndex+1;
                pointStrengthOverallAt(pointIndex) = 0.f;
            }
        }

        // if (count) {
            printc(LightsShowPrint, "sent to point strength uniform:\n");
            if constexpr (LightsShowPrint) {for (size_t i = 0; i < pointCount; ++i) {
                printPointLight(pointIdByIndex[i]);
            }}
            bgfx::setUniform(pointStrength, &pointDataStrength, pointCount);
        // }
        return count;
    }

    // restores pointDataStrength[].w from cache
    void restorePointStrength(size_t count = MAX_POINT_LIGHTS) {
        for (size_t i = 0; i < count; ++i) {
            pointStrengthOverallAt(i) = pointDataOverallStrength[i];
        }
    }

    void updateGlobalFromEuler() {
        for (size_t i = 0; i < directionalCount; ++i) {
            dirDataDir[i] = 
                glm::vec4{0.f, 1.f, 0.f, 1.f} * 
                glm::rotate(glm::mat4{1.f}, dirDataDirAsEuler[i].x, glm::vec3{1.f, 0.f, 0.f}) *
                glm::rotate(glm::mat4{1.f}, dirDataDirAsEuler[i].y, glm::vec3{0.f, 0.f, 1.f});
        }
    }

    void addDirectional() {
        if (directionalCount == MAX_DIRECTIONAL_LIGHTS) {
            print("WARNING LIGHT NOT ADDED. Max directional count (%zu) already reached.\n", MAX_DIRECTIONAL_LIGHTS);
            return;
        }
        resetDirectional(directionalCount++);
    }

    // issues an id.
    // keeps track of what index the id is tied to in pointIndexById
    size_t addPoint() {
        if (pointCount == MAX_POINT_LIGHTS) {
            print("WARNING LIGHT NOT ADDED. Max point count (%zu) already reached.\n", MAX_POINT_LIGHTS);
            return SIZE_MAX;
        }

        // set data for next point
        resetPoint(pointCount);
        
        // create and id and keep track of related index
        static size_t nextId = 1;
        pointIndexById[nextId] = pointCount++;
        pointIdByIndex[pointCount-1] = nextId;
        
        // return then increment id
        return nextId++;
    }

    // takes id, returns actual index into array
    size_t pointIndex(size_t id) {
        return pointIndexById.at(id);
    }

    // takes id not index!
    void removePoint(size_t id) {
        size_t index = pointIndex(id);
        --pointCount;

        // copy everything higher in array down
        for (size_t i = index; i < pointCount; ++i) {
            pointDataPosAndRadius[i] = pointDataPosAndRadius[i+1];
            pointDataStrength[i] = pointDataStrength[i+1];
            pointDataColor[i] = pointDataColor[i+1];
        }

        // update the ids and id/index lookups
        pointIndexById.erase(id);
        pointIdByIndex.clear();
        for (auto & idIndexPair : pointIndexById) {
            if (idIndexPair.second > index) {
                idIndexPair.second -= 1;
                // print("adjusting index of id %zu from %zu to %zu\n", idIndexPair.first, idIndexPair.second+1, idIndexPair.second);
            }
            pointIdByIndex[idIndexPair.second] = idIndexPair.first;
        }
    }

    void printAllPointLights() {
        for (size_t i = 0; i < pointCount; ++i) {
            printPointLight(pointIdByIndex.at(i));
        }
    }

    void printPointLight(size_t id) {
        size_t index = pointIndex(id);
        print("PointLight(%zu): ", id);
        printPointLightDataAt(index);
    }
    void printPointLightDataAt(size_t index, bool condition = true) {
        if (!condition) return;

        print("i:%zu, s:(%.2f, %.2f, %.2f, %.2f), color:%08x, pos:%.1f,%.1f,%.1f, r:%.1f\n",
            index,
            pointDataStrength[index].x,
            pointDataStrength[index].y,
            pointDataStrength[index].z,
            pointDataStrength[index].w,
            colorVec4ToUint((float *)&pointDataColor[index]),
            pointDataPosAndRadius[index].x,
            pointDataPosAndRadius[index].y,
            pointDataPosAndRadius[index].z,
            pointDataPosAndRadius[index].w
        );
    }

private:
    friend class Editor;
    friend class RenderSystem;
    std::map<size_t, size_t> pointIndexById; // id is key, index is value
    std::map<size_t, size_t> pointIdByIndex; // index is key, id is value
};
