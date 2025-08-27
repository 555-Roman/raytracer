#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

#include "objParser.h"
#include "shader.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"

using namespace glm;

void sendObjects(Shader shader);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
float deltaTime = 0.0f;
bool isStill = true;
unsigned int frameCount = 0;

const float cameraMoveSpeed = 8;
const float cameraRotateSpeed = 60;

// #define FULLSCREEN

std::vector<Triangle> triangles;

// vec3 cameraPosition = vec3(2.0, 3.0, -5.0);
// vec3 cameraForward = vec3(-0.3244428422615251, -0.48666426339228763, 0.8111071056538127);
// vec3 cameraUp = vec3(-0.18074256993863339, 0.8735890880367281, 0.45185642484658345);
// vec3 cameraRight = vec3(0.9284766908852593, 0.0, 0.3713906763541037);

vec3 cameraPosition = vec3(0);
vec3 cameraForward = vec3(0, 0, 1);
vec3 cameraUp = vec3(0, 1, 0);
vec3 cameraRight = vec3(1, 0, 0);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
#ifdef FULLSCREEN
    const unsigned int SCR_WIDTH = mode->width;
    const unsigned int SCR_HEIGHT = mode->height;
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", monitor, NULL);
#else
    const unsigned int SCR_WIDTH = mode->width / 2;
    const unsigned int SCR_HEIGHT = mode->height / 2;
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
#endif
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
         1.0f,  1.0f, 0.0f,  1.0, 1.0,  // top right
         1.0f, -1.0f, 0.0f,  1.0, 0.0,  // bottom right
        -1.0f, -1.0f, 0.0f,  0.0, 0.0,  // bottom left
        -1.0f,  1.0f, 0.0f,  0.0, 1.0   // top left
    };
    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,  // first Triangle
        1, 2, 3   // second Triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // Create 2 textures for ping-pong accumulation
    GLuint accumTextures[2];
    glGenTextures(2, accumTextures);
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, accumTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    // Framebuffer for rendering accumulation
    GLuint fbo;
    glGenFramebuffers(1, &fbo);


    Shader shader(RESOURCES_PATH "/default.vert", RESOURCES_PATH "/raytrace.frag");

    Shader displayShader(RESOURCES_PATH "/default.vert", RESOURCES_PATH "/display.frag");

    std::vector<Triangle> trianglesFromModel = getTrianglesFromOBJ(RESOURCES_PATH "/model.obj");
    for (Triangle triangle : trianglesFromModel) {
        // std::cout << "pushing triangle" << std::endl;
        // std::cout << " posA: " << triangle.posA.x << ", " << triangle.posA.y << ", " << triangle.posA.z << std::endl;
        // std::cout << " posB: " << triangle.posB.x << ", " << triangle.posB.y << ", " << triangle.posB.z << std::endl;
        // std::cout << " posC: " << triangle.posC.x << ", " << triangle.posC.y << ", " << triangle.posC.z << std::endl;
        // std::cout << " normalA: " << triangle.normalA.x << ", " << triangle.normalA.y << ", " << triangle.normalA.z << std::endl;
        // std::cout << " normalB: " << triangle.normalB.x << ", " << triangle.normalB.y << ", " << triangle.normalB.z << std::endl;
        // std::cout << " normalC: " << triangle.normalC.x << ", " << triangle.normalC.y << ", " << triangle.normalC.z << std::endl;
        triangles.push_back(triangle);
    }

    // uncomment this call to draw in wireframe polygons.
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        std::chrono::time_point<std::chrono::system_clock> startFrame = std::chrono::high_resolution_clock::now();
        // input
        // -----
        processInput(window);

        int readIdx  = frameCount % 2;
        int writeIdx = (frameCount + 1) % 2;

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // draw

        // ---------- Pass 1: Raytrace + Accumulate to Texture ----------
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumTextures[writeIdx], 0);

        shader.use();
        shader.setUint("uResolution", SCR_WIDTH, SCR_HEIGHT);
        shader.setFloat("uFocalLength", tan(45.0 / 180.0 * 3.1415926)*.5 * (float)SCR_HEIGHT);

        shader.setFloat("cameraPosition", cameraPosition.x, cameraPosition.y, cameraPosition.z);
        shader.setFloat("cameraForward", cameraForward.x, cameraForward.y, cameraForward.z);
        shader.setFloat("cameraUp", cameraUp.x, cameraUp.y, cameraUp.z);
        shader.setFloat("cameraRight", cameraRight.x, cameraRight.y, cameraRight.z);

        shader.setInt("maxBounces", 4);
        shader.setInt("samplesPerPixel", 3);
        shader.setUint("renderedFrames", frameCount);
        shader.setBool("accumulate", isStill);

        sendObjects(shader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, accumTextures[readIdx]);

        glBindVertexArray(VAO);
        //glDrawArrays(GL_TRIANGLES, 0, 6);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        // glBindVertexArray(0); // no need to unbind it every time

        // ---------- Pass 2: Display to Screen ----------
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        displayShader.use();

        displayShader.setInt("uTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, accumTextures[writeIdx]);

        glBindVertexArray(VAO);
        //glDrawArrays(GL_TRIANGLES, 0, 6);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        // glBindVertexArray(0); // no need to unbind it every time

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();

        frameCount++;
        deltaTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - startFrame).count();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}



std::vector<Sphere> spheres;
void SetSphereUniform(Shader shader, const char* propertyName, int sphereIndex, const float value) {
    std::ostringstream ss;
    ss << "spheres[" << sphereIndex << "]." << propertyName;
    std::string uniformName = ss.str();

    shader.setFloat(uniformName.c_str(), value);
}
void SetSphereUniform(Shader shader, const char* propertyName, int sphereIndex, const vec3 value) {
    std::ostringstream ss;
    ss << "spheres[" << sphereIndex << "]." << propertyName;
    std::string uniformName = ss.str();

    shader.setFloat(uniformName.c_str(), value.x, value.y, value.z);
}
void sendSpheres(Shader shader) {
    Material sphereMat0 = {vec3(0.8, 0.0, 0.8),  vec3(0.0), 0.0,  0.0};
    Material sphereMat1 = {vec3(0.8, 0.0, 0.0),  vec3(0.0), 0.0,  0.0};
    Material sphereMat2 = {vec3(0.8, 0.8, 0.0),  vec3(0.0), 0.0,  0.0};
    Material sphereMat3 = {vec3(0.0, 0.8, 0.0),  vec3(0.0), 0.0,  0.9};
    Material sphereMat4 = {vec3(0.8, 0.8, 0.8),  vec3(0.0), 0.0,  0.0};
    Material sphereMat5 = {vec3(0.0, 0.0, 0.0),  vec3(1.0), 3.5,  0.0};
    Sphere sphere0 = {vec3(0.0, -101.0, 0.0), 100.0, sphereMat0};
    Sphere sphere1 = {vec3(0.0, 1.0, 1.0), 1.0, sphereMat1};
    Sphere sphere2 = {vec3(-2.0, 0.75, 0.5), 0.75, sphereMat2};
    Sphere sphere3 = {vec3(-3.5, 0.5, 0.0), 0.5, sphereMat3};
    Sphere sphere4 = {vec3(2.5, 1.25, 0.0), 1.25, sphereMat4};
    Sphere sphere5 = {vec3(-100.0, 50.0, 100.0), 100.0, sphereMat5};
    spheres = std::vector<Sphere>{};
    spheres.push_back(sphere0);
    // spheres.push_back(sphere1);
    // spheres.push_back(sphere2);
    // spheres.push_back(sphere3);
    // spheres.push_back(sphere4);
    spheres.push_back(sphere5);

    shader.setInt("numSpheres", spheres.size());

    for (int i = 0; i < spheres.size(); ++i) {
        SetSphereUniform(shader, "pos", i, spheres[i].pos);
        SetSphereUniform(shader, "radius", i, spheres[i].radius);

        SetSphereUniform(shader, "material.color", i, spheres[i].material.color);
        SetSphereUniform(shader, "material.emissionColor", i, spheres[i].material.emissionColor);
        SetSphereUniform(shader, "material.emissionStrength", i, spheres[i].material.emissionStrength);
        SetSphereUniform(shader, "material.smoothness", i, spheres[i].material.smoothness);
    }
}

void SetTriangleUniform(Shader shader, const char* propertyName, int triangleIndex, const float value) {
    std::ostringstream ss;
    ss << "triangles[" << triangleIndex << "]." << propertyName;
    std::string uniformName = ss.str();

    shader.setFloat(uniformName.c_str(), value);
}
void SetTriangleUniform(Shader shader, const char* propertyName, int sphereIndex, const vec3 value) {
    std::ostringstream ss;
    ss << "triangles[" << sphereIndex << "]." << propertyName;
    std::string uniformName = ss.str();

    shader.setFloat(uniformName.c_str(), value.x, value.y, value.z);
}
void sendTriangles(Shader shader) {
    Material triangleMat0 = {vec3(1.0), vec3(0.0), 0.0, 1.0};
    Triangle triangle0 = {
        //      vec3(0.0, 0.0, -1.0), vec3(0.0, 0.0, -4.0), vec3(3.0, 0.0, -1.0),
        //      vec3(0.0, 0.0, -1.0), vec3(3.0, 0.0, -1.0), vec3(0.0, 0.0, -4.0),
        //      vec3(0.0, 0.5, -1.0), vec3(0.0, 0.5, -4.0), vec3(3.0, 0.5, -1.0),
        vec3(0.0, 0.5, -1.0), vec3(3.0, 0.5, -1.0), vec3(0.0, 0.5, -4.0),
  //      vec3(0.0, 0.5, -1.0), vec3(3.0, 0.5, 0.0), vec3(0.0, 0.5, -4.0),
     normalize(vec3(0.0, 1.0, 0.0)), normalize(vec3(0.0, 1.0, 0.0)), normalize(vec3(0.0, 1.0, 0.0)),
  //      normalize(vec3(-0.1, 1.0, 0.1)), normalize(vec3(0.0, 1.0, 0.0)), normalize(vec3(0.0, 1.0, 0.0)),
        triangleMat0
    };
    // triangles = std::vector<Triangle>{};
    // triangles.push_back(triangle0);

    shader.setInt("numTriangles", triangles.size());

    for (int i = 0; i < triangles.size(); ++i) {
        SetTriangleUniform(shader, "posA", i, triangles[i].posA);
        SetTriangleUniform(shader, "posB", i, triangles[i].posB);
        SetTriangleUniform(shader, "posC", i, triangles[i].posC);
        SetTriangleUniform(shader, "normalA", i, triangles[i].normalA);
        SetTriangleUniform(shader, "normalB", i, triangles[i].normalB);
        SetTriangleUniform(shader, "normalC", i, triangles[i].normalC);

        SetTriangleUniform(shader, "material.color", i, triangles[i].material.color);
        SetTriangleUniform(shader, "material.emissionColor", i, triangles[i].material.emissionColor);
        SetTriangleUniform(shader, "material.emissionStrength", i, triangles[i].material.emissionStrength);
        SetTriangleUniform(shader, "material.smoothness", i, triangles[i].material.smoothness);
    }
}

void sendObjects(Shader shader) {
    sendSpheres(shader);
    sendTriangles(shader);
}

vec3 rotateX(vec3 vector, float angle) {
    float rad = radians(angle);
    return vec3(vector.x, vector.y*cos(rad) - vector.z*sin(rad), vector.y*sin(rad) + vector.z*cos(rad));
}
vec3 rotateY(vec3 vector, float angle) {
    float rad = radians(angle);
    return vec3(vector.x*cos(rad) - vector.z*sin(rad), vector.y, vector.x*sin(rad) + vector.z*cos(rad));
}
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
float cameraPitch = 0, cameraYaw = 0;
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    vec3 toAdd = vec3(0, 0, 0);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        toAdd += normalize(vec3(cameraForward.x, 0, cameraForward.z)) * cameraMoveSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        toAdd -= normalize(vec3(cameraForward.x, 0, cameraForward.z)) * cameraMoveSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        toAdd += normalize(vec3(cameraRight.x, 0, cameraRight.z)) * cameraMoveSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        toAdd -= normalize(vec3(cameraRight.x, 0, cameraRight.z)) * cameraMoveSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        toAdd += normalize(vec3(0, cameraUp.y, 0)) * cameraMoveSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        toAdd -= normalize(vec3(0, cameraUp.y, 0)) * cameraMoveSpeed * deltaTime;
    cameraPosition += toAdd;

    float pitch = 0;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        pitch -= cameraRotateSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        pitch += cameraRotateSpeed * deltaTime;
    cameraPitch += pitch;
    cameraForward = rotateX(vec3(0, 0, 1), cameraPitch);
    cameraUp = rotateX(vec3(0, 1,0 ), cameraPitch);
    cameraRight = rotateX(vec3(1, 0, 0), cameraPitch);
    float yaw = 0;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        yaw -= cameraRotateSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        yaw += cameraRotateSpeed * deltaTime;
    cameraYaw += yaw;
    cameraForward = rotateY(cameraForward, cameraYaw);
    cameraUp = rotateY(cameraUp, cameraYaw);
    cameraRight = rotateY(cameraRight, cameraYaw);

    isStill = toAdd == vec3(0) && pitch == 0 && yaw == 0;
    frameCount = isStill ? frameCount : 0;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
