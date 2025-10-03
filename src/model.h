#ifndef MODEL_H
#define MODEL_H
#include <string>
#include <vector>

#include "objParser.h"
#include "glm/glm.hpp"

using namespace glm;

struct Material {
    vec3 color;
    float roughness;
    vec3 emissionColor;
    float emissionStrength;
    float alpha;
    float transmission;
    float ior;
    float metalness;
};
struct Transform {
    vec3 translation;
    mat3 rotation;
    vec3 scale;
};
struct SSBO_Model {
    uint triangleIndex;
    uint triangleCount;
    uint padding[2];

    vec4 boundMin;
    vec4 boundMax;

    vec4 color_roughness;
    vec4 emissionColor_emissionStrength;
    vec4 transmission_ior_metalness_tbd;

    vec4 translation;
    mat4 rotation;
    mat4 inverseRotation;
};

class Model {
    public:
        Model();
        Model(uint triangleIndex_, uint triangleCount_, Material material_, Transform transform_);
        Model(std::string filePath, Material material_, Transform transform_);

        SSBO_Model get_SSBO_Model();

        uint triangleIndex, triangleCount;
        vec3 boundMin, boundMax;
        Material material;
        Transform transform;
};

void loadTriangles(std::string filePath);

inline std::vector<Triangle> triangles;
inline std::vector<Model*> models;

#endif
