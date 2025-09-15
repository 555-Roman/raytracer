#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

#include "objParser.h"
#include "shader.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace glm;

void sendSpheres();
void sendTriangles();
void sendModels();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void saveScreenshot(int x, int y, int width, int height, char name[]);
void processInput(GLFWwindow *window);
float deltaTime = 0.0f;
unsigned int frameCount = 0;

const float cameraMoveSpeed = 1;
const float cameraRotateSpeed = 60;
float cameraPitch = 0, cameraYaw = 180;

bool START_RENDER = false;
bool ZERO_TOGGLE = true;

// #define FULLSCREEN
#ifdef FULLSCREEN
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;
#else
const unsigned int SCR_WIDTH = 1920 / 4;
const unsigned int SCR_HEIGHT = 1080 / 4;
#endif

GLuint sphereSSBO;
GLuint triangleSSBO;
std::vector<Triangle> triangles;
GLuint modelSSBO;

vec3 cameraPosition = vec3(0, 0, 4);
vec3 cameraForward = vec3(0, 0, 1);
vec3 cameraUp = vec3(0, 1, 0);
vec3 cameraRight = vec3(1, 0, 0);

int main() {
    stbi_flip_vertically_on_write(1);

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
#ifdef FULLSCREEN
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", monitor, NULL);
#else
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

    glGenBuffers(1, &sphereSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphereSSBO);
    sendSpheres();

    glGenBuffers(1, &triangleSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleSSBO);
    std::vector<Triangle> trianglesFromModel = getTrianglesFromOBJ(RESOURCES_PATH "/box.obj");
    for (Triangle triangle : trianglesFromModel) {
        triangles.push_back(triangle);
    }
    trianglesFromModel = getTrianglesFromOBJ(RESOURCES_PATH "/model.obj");
    for (Triangle triangle : trianglesFromModel) {
        triangles.push_back(triangle);
    }
    sendTriangles();

    glGenBuffers(1, &modelSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, modelSSBO);
    sendModels();

    // uncomment this call to draw in wireframe polygons.
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        if (START_RENDER) {
            if (frameCount == 10) { saveScreenshot(0, 0, SCR_WIDTH, SCR_HEIGHT, SCREENSHOTS_PATH "10_samples_white_strong.png"); }
            if (frameCount == 100) { saveScreenshot(0, 0, SCR_WIDTH, SCR_HEIGHT, SCREENSHOTS_PATH "100_samples_white_strong.png"); }
            if (frameCount == 1000) { saveScreenshot(0, 0, SCR_WIDTH, SCR_HEIGHT, SCREENSHOTS_PATH "1000_samples_white_strong.png"); }
            if (frameCount == 10000) { saveScreenshot(0, 0, SCR_WIDTH, SCR_HEIGHT, SCREENSHOTS_PATH "10000_samples_white_strong.png"); }
        }
        // if (frameCount == 1000) { saveScreenshot(0, 0, SCR_WIDTH, SCR_HEIGHT, "1000_samples_screenshot_100_bounces.png"); }

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
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sphereSSBO);
        shader.setUint("uResolution", SCR_WIDTH, SCR_HEIGHT);
        shader.setFloat("uFocalLength", tan(45.0 / 180.0 * 3.1415926)*.5 * (float)SCR_HEIGHT);

        shader.setFloat("cameraPosition", cameraPosition.x, cameraPosition.y, cameraPosition.z);
        shader.setFloat("cameraForward", cameraForward.x, cameraForward.y, cameraForward.z);
        shader.setFloat("cameraUp", cameraUp.x, cameraUp.y, cameraUp.z);
        shader.setFloat("cameraRight", cameraRight.x, cameraRight.y, cameraRight.z);

        shader.setInt("maxBounces_reflection", 10);
        shader.setInt("maxBounces_transmission", 10);
        shader.setInt("samplesPerPixel", 1);
        shader.setUint("renderedFrames", frameCount * ZERO_TOGGLE);

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



void sendSpheres() {
    std::vector<Sphere> spheres;
    Sphere mySphere0 = {vec4(0.0, 0.0, 0.0, 1.0), vec4(1.0, 1.0, 1.0, 0.0), vec4(0.0), vec4(0.0, 1.5, 0.0, 0.0)};
    Sphere mySphere1 = {vec4(-0.5, -1.0, -1.5, 0.25), vec4(1.0, 0.0, 1.0, 1.0), vec4(0.0), vec4(1.0, 1.0, 0.0, 0.0)};
    spheres.push_back(mySphere0);
    spheres.push_back(mySphere1);

    glBufferData(GL_SHADER_STORAGE_BUFFER, spheres.size() * sizeof(Sphere), &(spheres[0]), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sphereSSBO);
}

void sendTriangles() {
    // Triangle triangle0 = {
        // vec4(0.0, 0.5, -1.0, 0.0), vec4(3.0, 0.5, -1.0, 0.0), vec4(0.0, 0.5, -4.0, 0.0),
        // normalize(vec4(0.0, 1.0, 0.0, 0.0)), normalize(vec4(0.0, 1.0, 0.0, 0.0)), normalize(vec4(0.0, 1.0, 0.0, 0.0)),
        // vec4(1.0), vec4(0.0)
    // };
    // triangles = std::vector<Triangle>();
    // triangles.push_back(triangle0);

    glBufferData(GL_SHADER_STORAGE_BUFFER, triangles.size() * sizeof(Triangle), &(triangles[0]), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, triangleSSBO);
}
mat4 defaultRotation = mat4(1);
void sendModels() {
    vec3 Mmin = vec3(triangles[60].posA);
    vec3 Mmax = vec3(triangles[60].posA);
    for (int i = 60; i < 62; i++) {
        Mmin = min(Mmin, vec3(triangles[i].posA));
        Mmin = min(Mmin, vec3(triangles[i].posB));
        Mmin = min(Mmin, vec3(triangles[i].posC));
        Mmax = max(Mmax, vec3(triangles[i].posA));
        Mmax = max(Mmax, vec3(triangles[i].posB));
        Mmax = max(Mmax, vec3(triangles[i].posC));
    }
    std::cout << "Mmin: " << Mmin.x << ", " << Mmin.y << ", " << Mmin.z << std::endl;
    std::cout << "Mmax: " << Mmax.x << ", " << Mmax.y << ", " << Mmax.z << std::endl;
    Model model0 = {
        0, 12, {0, 0},
        vec4(-2.4, -2, -1.98333, 0.0f),
        vec4(-2, 2, 2.01667, 0.0),
        vec4(0.0, 1.0, 0.0, 1.0), vec4(0.0),
        vec4(0.0), defaultRotation, inverse(defaultRotation)
    };
    Model model1 = {
        12, 12, {0, 0},
        vec4(2, -2, -1.98333, 0.0f),
        vec4(2.4, 2, 2.01667, 0.0),
        vec4(1.0, 0.0, 0.0, 1.0), vec4(0.0),
        vec4( 0.0), defaultRotation, inverse(defaultRotation)
    };
    Model model2 = {
        24, 36, {0, 0},
        vec4(-2, -2.4, -2.38333, 0.0f),
        vec4(2, 2.4, 2.01667, 0.0),
        vec4(1.0, 1.0, 1.0, 1.0), vec4(0.0),
        vec4(0.0), defaultRotation, inverse(defaultRotation)
    };
    Model wallQuad = {
        60, 2, {0, 0},
        vec4(-2, -2, 2.01667, 0.0f),
        vec4(2, 2, 2.01667, 0.0),
        vec4(1.0, 1.0, 1.0, 1.0), vec4(0.0),
        vec4(0.0), defaultRotation, inverse(defaultRotation)
    };
    Model lightQuad = {
        60, 2, {0, 0},
        vec4(-2, -2, 2.01667, 0.0f),
        vec4(2, 2, 2.01667, 0.0),
        vec4(0.0), vec4(1.0),
        vec4(0.0, 0.0, -5.0, 0.0), rotate(defaultRotation, 3.1415926f, vec3(0.0f, 1.0f, 0.0f)), inverse(rotate(defaultRotation, 180.0f, vec3(0.0f, 1.0f, 0.0f)))
    };
    Model model3 = {
        62, 12, {0, 0},
        vec4(-0.5, 1.95, -0.483333, 0.0f),
        vec4(0.5, 2.05, 0.516667, 0.0),
        vec4(1.0, 1.0, 1.0, 1.0), vec4(1.0, 1.0, 1.0, 5.0),
        vec4(0.0), defaultRotation, inverse(defaultRotation)
    };
    Model model4 = {
        74, 968, {0, 0},
        vec4(-1.36719, -0.984375, -0.851562, 0.0),
        vec4(1.36719, 0.984375, 0.851562, 0.0),
        vec4(1.0, 1.0, 1.0, 0.0), vec4(0.0),
        vec4(0.0, -0.5, 0.0, 0.0), defaultRotation, inverse(defaultRotation)
    };
    std::vector<Model> models;
    models.push_back(model0);
    models.push_back(model1);
    models.push_back(model2);
    models.push_back(wallQuad);
    // models.push_back(lightQuad);
    models.push_back(model3);
    // models.push_back(model4);
    glBufferData(GL_SHADER_STORAGE_BUFFER, models.size() * sizeof(Model), &(models[0]), GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, modelSSBO);
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
unsigned char buffer[3 * SCR_WIDTH * SCR_HEIGHT];
void saveScreenshot(int x, int y, int width, int height, char name[]) {
    std::cout << "saving screenshot with name \"" << name << "\"..." << std::endl;
    glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, &buffer);
    stbi_write_png(name, SCR_WIDTH, SCR_HEIGHT, 3, &buffer, 3 * SCR_WIDTH);
}
auto lastClicked = std::chrono::high_resolution_clock::now();
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
        saveScreenshot(0, 0, SCR_WIDTH, SCR_HEIGHT, "screenshot.png");
    }
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        std::cout << "Camera Data:" << std::endl;
        std::cout << cameraPosition.x << ", " << cameraPosition.y << ", " << cameraPosition.z << std::endl;
        std::cout << cameraPitch << ", " << cameraYaw << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - lastClicked).count() < 1) return;
        lastClicked = std::chrono::high_resolution_clock::now();
        std::cout << "toggled rendering" << std::endl;
        START_RENDER = !START_RENDER;
        frameCount = 0;
    }
    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
        if (std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - lastClicked).count() < 1) return;
        lastClicked = std::chrono::high_resolution_clock::now();
        std::cout << "toggled zero" << std::endl;
        ZERO_TOGGLE = !ZERO_TOGGLE;
        frameCount = 0;
    }

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
    frameCount = toAdd == vec3(0) && pitch == 0 && yaw == 0 ? frameCount : 0;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
