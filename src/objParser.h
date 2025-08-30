#ifndef OBJPARSER_H
#define OBJPARSER_H
#include <fstream>
#include <iosfwd>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "glm/glm.hpp"

using namespace glm;

struct Material {
    vec3 color;
    vec3 emissionColor;
    float emissionStrength;
    float smoothness;
};
Material defaultMaterial = {vec3(1.0), vec3(0.0), 0.0, 0.0};
/*struct Sphere {
    vec3 pos;
    float radius;
    Material material;
};*/
struct Sphere {
    vec4 pos_radius;
    vec4 color_smoothness;
    vec4 emissionColor_emissionStrength;
};
struct Triangle {
    vec4 posA;
    vec4 posB;
    vec4 posC;
    vec4 normalA;
    vec4 normalB;
    vec4 normalC;
};
struct Model {
    uint triangleIndex;
    uint triangleCount;
    uint padding[2];
    vec4 boundMin;
    vec4 boundMax;
    vec4 color_smoothness;
    vec4 emissionColor_emissionStrength;
};


std::vector<Triangle> getTrianglesFromOBJ(const char* filePath) {
    std::string contents;
    std::ifstream file;
    // file.exceptions (std::ifstream::failbit | std::ifstream::badbit);

    try {
        file.open(filePath);
    } catch (std::ifstream::failure& e) {
        std::cout << "ERROR::MODEL::FILE_NOT_SUCCESSFULLY_OPENED: " << e.what();
        return std::vector<Triangle>();
    }

    std::vector<vec3> vertices;
    std::vector<vec3> normals;
    std::vector<vec3> uvs;
    std::vector<std::vector<ivec3>> faces;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        // std::cout << "found line '" << line << "'" << std::endl;
        if (line.at(0) == 'v') {
            if (line.at(1) == ' ') {
                // std::cout << " decoding vertex" << std::endl;
                vec3 vertex;
                int vertexIndex = 0;
                int charIdx = 2;
                std::string str;
                while (charIdx < line.size()) {
                    // std::cout << "  found char '" << line.at(charIdx) << "'" << std::endl;
                    if (line.at(charIdx) == ' ') {
                        // std::cout << "   converting vertex from string '" << str << "' to '" << std::stof(str) << "'" << std::endl;
                        vertex[vertexIndex] = std::stof(str);
                        str = "";
                        vertexIndex++;
                    } else {
                        // std::cout << "   appending char '" << line.at(charIdx) << "'" << std::endl;
                        str += line.at(charIdx);
                    }
                    charIdx++;
                }
                // std::cout << "   converting vertex from string '" << str << "' to '" << std::stof(str) << "'" << std::endl;
                vertex[vertexIndex] = std::stof(str);
                // std::cout << " found vertex with position: " << vertex.x << ", " << vertex.y << ", " << vertex.z << std::endl;
                vertices.push_back(vertex);
            } else if (line.at(1) == 'n') {
                // std::cout << " decoding normal" << std::endl;
                vec3 normal;
                int normalIndex = 0;
                int charIdx = 3;
                std::string str;
                while (charIdx < line.size()) {
                    // std::cout << "  found char '" << line.at(charIdx) << "'" << std::endl;
                    if (line.at(charIdx) == ' ') {
                        // std::cout << "   converting normal from string '" << str << "' to '" << std::stof(str) << "'" << std::endl;
                        normal[normalIndex] = std::stof(str);
                        str = "";
                        normalIndex++;
                    } else {
                        // std::cout << "   appending char '" << line.at(charIdx) << "'" << std::endl;
                        str += line.at(charIdx);
                    }
                    charIdx++;
                }
                // std::cout << "   converting normal from string '" << str << "' to '" << std::stof(str) << "'" << std::endl;
                normal[normalIndex] = std::stof(str);
                // std::cout << " found normal with position: " << normal.x << ", " << normal.y << ", " << normal.z << std::endl;
                normals.push_back(normal);
            }
        } else if (line.at(0) == 'f') {
            // std::cout << " decoding face" << std::endl;
            std::vector<ivec3> faceVertices;
            int faceVertexIndex = 0;
            int charIdx = 2;
            std::string str;
            ivec3 faceVertex;
            while (charIdx < line.size()) {
                // std::cout << "  found char '" << line.at(charIdx) << "'" << std::endl;
                if (line.at(charIdx) == ' ') {
                    // std::cout << "   converting int from string '" << str << "' to '" << std::stoi(str) << "'" << std::endl;
                    faceVertex[faceVertexIndex] = std::stoi(str);
                    str = "";
                    faceVertexIndex++;
                    faceVertexIndex %= 3;
                    // std::cout << "   appending face vertex" << std::endl;
                    faceVertices.push_back(faceVertex);
                } else if (line.at(charIdx) == '/') {
                    // std::cout << "   converting int from string '" << str << "' to '" << std::stoi(str) << "'" << std::endl;
                    faceVertex[faceVertexIndex] = std::stoi(str);
                    str = "";
                    faceVertexIndex++;
                    faceVertexIndex %= 3;
                } else {
                    // std::cout << "   appending char '" << line.at(charIdx) << "'" << std::endl;
                    str += line.at(charIdx);
                }
                charIdx++;
            }
            // std::cout << "   converting int from string '" << str << "' to '" << std::stoi(str) << "'" << std::endl;
            faceVertex[faceVertexIndex] = std::stoi(str);
            // std::cout << "   appending face vertex" << std::endl;
            faceVertices.push_back(faceVertex);
            faces.push_back(faceVertices);
        }
    }

    file.close();

    // for (auto & vertex : vertices)
        // std::cout << "! decoded vertex: " << vertex.x << ", " << vertex.y << ", " << vertex.z << std::endl;
    // for (auto & normal : normals)
        // std::cout << "! decoded normal: " << normal.x << ", " << normal.y << ", " << normal.z << std::endl;
    // for (auto & face : faces) {
        // std::cout << "! decoded face" << std::endl;
        // for (auto & faceVertex : face)
            // std::cout << " decoded face vertex: " << faceVertex.x << ", " << faceVertex.y << ", " << faceVertex.z << std::endl;
    // }
    // std::cout << "! done" << std::endl;

    std::vector<Triangle> triangles;
    for (auto & face : faces) {
        ivec3 faceIndices = ivec3(0, 1, 2);
        for (int i = 0; i < face.size()-2; i++) {
            vec3 posA = vertices[face[faceIndices.x][0]-1];
            vec3 posB = vertices[face[faceIndices.y][0]-1];
            vec3 posC = vertices[face[faceIndices.z][0]-1];

            vec3 normalA = normals[face[faceIndices.x][2]-1];
            vec3 normalB = normals[face[faceIndices.y][2]-1];
            vec3 normalC = normals[face[faceIndices.z][2]-1];

            // std::cout << "pushing triangle" << std::endl;
            // std::cout << " vertexPosAIdx: " << face[faceIndices.x][0]-1;
            // std::cout << " vertexPosBIdx: " << face[faceIndices.y][0]-1;
            // std::cout << " vertexPosCIdx: " << face[faceIndices.z][0]-1 << std::endl;
            // std::cout << " posA: " << posA.x << ", " << posA.y << ", " << posA.z << std::endl;
            // std::cout << " posB: " << posB.x << ", " << posB.y << ", " << posB.z << std::endl;
            // std::cout << " posC: " << posC.x << ", " << posC.y << ", " << posC.z << std::endl;
            // std::cout << " vertexNormalAIdx: " << face[faceIndices.x][2]-1;
            // std::cout << " vertexNormalBIdx: " << face[faceIndices.y][2]-1;
            // std::cout << " vertexNormalCIdx: " << face[faceIndices.z][2]-1 << std::endl;
            // std::cout << " normalA: " << normalA.x << ", " << normalA.y << ", " << normalA.z << std::endl;
            // std::cout << " normalB: " << normalB.x << ", " << normalB.y << ", " << normalB.z << std::endl;
            // std::cout << " normalC: " << normalC.x << ", " << normalC.y << ", " << normalC.z << std::endl;

            Triangle toPush = {
                vec4(posA, 0.0), vec4(posB, 0.0), vec4(posC, 0.0),
                vec4(normalA, 0.0), vec4(normalB, 0.0), vec4(normalC, 0.0)
            };
            triangles.push_back(toPush);

            faceIndices = ivec3(0, faceIndices.z, faceIndices.z+1);
        }
    }

    // std::cout << "! returning " << triangles.size() << " triangles" << std::endl;
    return triangles;
}

#endif
