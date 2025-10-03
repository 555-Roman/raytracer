#include "model.h"
#include <string>
#include <vector>

#include "objParser.h"


Model::Model() {
    models.push_back(this);
    triangleIndex = 0;
    triangleCount = 1;
    boundMin = vec3(-0.5);
    boundMax = vec3(0.5);
    material.color = vec3(1.0, 0.0, 1.0);
    material.roughness = 1.0f;
    material.emissionColor = vec3(0.0);
    material.emissionStrength = 0.0;
    material.alpha = 1.0;
    material.transmission = 0.0;
    material.ior = 1.0;
    material.metalness = 0.0;
    transform.translation = vec3(0.0);
    transform.rotation = mat3(1.0);
    transform.scale = vec3(1.0);
}
Model::Model(uint triangleIndex_, uint triangleCount_, Material material_, Transform transform_) {
    models.push_back(this);
    triangleIndex = triangleIndex_;
    triangleCount = triangleCount_;
    boundMin = vec3(triangles[triangleIndex_].pos_uvx_A);
    boundMax = vec3(triangles[triangleIndex_].pos_uvx_A);
    for (uint i = triangleIndex_; i < triangleIndex_ + triangleCount_; i++) {
        boundMin = min(boundMin, vec3(triangles[i].pos_uvx_A));
        boundMin = min(boundMin, vec3(triangles[i].pos_uvx_B));
        boundMin = min(boundMin, vec3(triangles[i].pos_uvx_C));
        boundMax = max(boundMax, vec3(triangles[i].pos_uvx_A));
        boundMax = max(boundMax, vec3(triangles[i].pos_uvx_B));
        boundMax = max(boundMax, vec3(triangles[i].pos_uvx_C));
    }
    material = material_;
    transform = transform_;
}
Model::Model(std::string filePath, Material material_, Transform transform_) {
    models.push_back(this);
    std::vector<Triangle> modelTriangles = getTrianglesFromOBJ(filePath);
    triangleIndex = triangles.size();
    triangleCount = modelTriangles.size();
    boundMin = vec3(modelTriangles[0].pos_uvx_A);
    boundMax = vec3(modelTriangles[0].pos_uvx_A);
    for (Triangle triangle : modelTriangles) {
        triangles.push_back(triangle);
        boundMin = min(boundMin, vec3(triangle.pos_uvx_A));
        boundMin = min(boundMin, vec3(triangle.pos_uvx_B));
        boundMin = min(boundMin, vec3(triangle.pos_uvx_C));
        boundMax = max(boundMax, vec3(triangle.pos_uvx_A));
        boundMax = max(boundMax, vec3(triangle.pos_uvx_B));
        boundMax = max(boundMax, vec3(triangle.pos_uvx_C));
    }
    material = material_;
    transform = transform_;
}
SSBO_Model Model::get_SSBO_Model() {
    SSBO_Model returnType = {
        triangleIndex, triangleCount, 0u, 0u,
        vec4(boundMin, 0.0), vec4(boundMax, 0.0),
        vec4(material.color, material.roughness), vec4(material.emissionColor, material.emissionStrength), vec4(material.transmission, material.ior, material.metalness, 0.0),
        vec4(transform.translation, 0.0), mat4(transform.rotation), inverse(mat4(transform.rotation))
    };

    return returnType;
}

void loadTriangles(std::string filePath) {
    std::vector<Triangle> modelTriangles = getTrianglesFromOBJ(filePath);
    for (Triangle triangle : modelTriangles) {
        triangles.push_back(triangle);
    }
}
