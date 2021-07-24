#include <iostream>
#include <iomanip>
#include <filesystem>

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifdef _MSC_VER
#pragma warning(disable: 4251)
#pragma warning(disable: 4267)
#pragma warning(disable: 4275)
#endif

#include <Magick++.h>

#include "texture.h"
#include "quad.h"
#include "sprite.h"

#ifndef WIN_WIDTH
#define WIN_WIDTH 1200
#endif
#ifndef WIN_HEIGHT
#define WIN_HEIGHT 1024
#endif

Sprite sprite;
Quad background; // the background quad/plane; has its own shader

// Globals
//const unsigned int WIN_WIDTH = 512;
//const unsigned int WIN_HEIGHT = 512;
const float fivTwel = 2.0f / WIN_WIDTH; // for viewport scale
glm::vec3 mover(0.0f, -0.5f, 0.0f); // camera position
glm::vec3 scale(2.0f / WIN_WIDTH, 2.0f / WIN_HEIGHT, 1.0f); // camera view scale

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int modifier);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void drop_callback(GLFWwindow* window, int count, const char** paths);
void handleEvents(GLFWwindow* window);
void draw(GLFWwindow* window);
GLFWwindow *initOpenGL();
std::string replace(const std::string &src, const std::string &what, const std::string &repl);

float aspect_ratio = WIN_WIDTH / WIN_HEIGHT;
float frame_rate = 60.0f;

std::string help = // hotkeys
"\nA: previous animation\n"
"S: next animation\n"
"L: toggle animation loop\n"
"Space: pause / replay(when looping is disabled)\n"
"X: flip\n"
"C: center camera\n"
"Q: Save image as PNG (without background)\n"
"W: Save animation as GIF (with background)\n"
"1, 2, 3: change animation speed\n"
"H: display this hotkey list again\n";


int main(int argc, char* argv[]) {
    sprite.dir = argv[0];
    sprite.dir = replace(sprite.dir, "\\", "/").substr(0, sprite.dir.rfind("/") + 1);

    std::vector<std::string> shader_name_list{
        sprite.dir + "shaders/sprite.vert",
        sprite.dir + "shaders/sprite.frag",
        sprite.dir + "shaders/background.vert",
        sprite.dir + "shaders/background.frag"
    };
    /**/
    if (argc == 1) {
        std::cout << "Drag an ssbp file here then press enter.\n";
        std::cin >> sprite.file_name;
    } else
        sprite.file_name = argv[1];
    /*/
    //sprite.file_name = "ch00_27_Freya_F_Normal_TransBattle/ch00_27_Freya_F_Normal_TransBattle.ssbp";
    //sprite.file_name = "ch00_27_Freya_F_Normal/ch00_27_Freya_F_Normal.ssbp";
    sprite.file_name = "ch04_24_Marc_F_Dark04/ch04_24_Marc_F_Dark04.ssbp";
    /**/

    try {
        GLFWwindow *window = initOpenGL();

        // initialize shaders & geometry
        sprite.init(shader_name_list[0], shader_name_list[1]);
        background.init(shader_name_list[2], shader_name_list[3]);
        background.texture = new Texture((sprite.dir + "images/background.png").c_str(), false); // load background image

        sprite.resman = ss::ResourceManager::getInstance();
        sprite.ssPlayer = ss::Player::create(sprite.resman);

        sprite.file_name = sprite.resman->addData(replace(sprite.file_name, "\\", "/"));
        sprite.ssPlayer->setData(sprite.file_name, &sprite.animation_list);
        sprite.ssPlayer->play(sprite.animation_list[0], 1);
        sprite.ssPlayer->setGameFPS(frame_rate);

        std::cout << help << '\n' << sprite.file_name << "\nNumber of animations: " << sprite.animation_list.size() << "\n\n" << sprite.ssPlayer->getPlayAnimeName() << std::endl;

        // render loop
        while (!glfwWindowShouldClose(window)) {
            draw(window);
            handleEvents(window);
        }
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwTerminate();
    return 0;
}

void draw(GLFWwindow* window) {
    // calc delta time
    static GLfloat lastTime = 0.0f;
    GLfloat currentTime = float(glfwGetTime());
    GLfloat deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    // render
    // glClearColor(0.25f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    background.shader.use();
    background.shader.setTexture2D("u_Texture", background.texture->id);
    background.shader.setBool("u_UseTexture", background.texture->loaded);
    background.draw();

    sprite.shader.use();
    sprite.ssPlayer->update(deltaTime);    //Player update
    sprite.draw();    //Draw a layer

    glm::mat4 view(1.0f);
    view = glm::translate(view, mover);
    view = glm::scale(view, scale*glm::vec3(1.0f, aspect_ratio, 1.0f));
    sprite.shader.setMat4("u_View", glm::value_ptr(view));

    sprite.shader.setFloat("u_Time", currentTime);

    //glfw: swap buffers and poll IO events (keys pressed/releaed, mouse moved etc.)
    glfwSwapBuffers(window);
}

GLFWwindow *initOpenGL()
{
    if (!glfwInit())
        throw std::runtime_error("Failed to init GLFW");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "SSBP Viewer", nullptr, nullptr);
    if (window == nullptr)
        throw std::runtime_error("Failed to create GLFW window");
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetDropCallback(window, drop_callback);
    glfwSetWindowSizeLimits(window, WIN_WIDTH, WIN_HEIGHT, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSwapInterval(60 / int(frame_rate)); //test //0 = unlocked frame rate; 1=60fps; 2=30fps; 3=20fps
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GL_TRUE); // test

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("Failed to initialize GLAD");
    glEnable(GL_BLEND);

    return window;
}

void zoom(double delta) {
    float z = float(delta)*scale.y*2.0f;
    float sign = (scale.x > 0.0f) ? 1.0f : ((scale.x < 0.0f) ? -1.0f : 0.0f);
    scale += glm::vec3(z*sign, z, 0.0f);
    float threshold = 0.001f;
    if (scale.y < threshold) {
        scale.x = threshold * sign;
        scale.y = threshold;
    }
}

void scroll_callback(GLFWwindow * window, double xoffset, double yoffset)
{
    zoom(yoffset*0.06);
}

void handleEvents(GLFWwindow* window) {
    glfwPollEvents();
    // camera panning with mouse
    glm::dvec2 mousePos;
    glm::dvec2 deltaPos;
    glfwGetCursorPos(window, &mousePos.x, &mousePos.y);
    static glm::dvec2 lastPos = mousePos;

    int left_button = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    int right_button = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    if (left_button == GLFW_PRESS || right_button == GLFW_PRESS) {
        glm::ivec2 size;
        glfwGetFramebufferSize(window, &size.x, &size.y);
        deltaPos = mousePos - lastPos;
        lastPos = mousePos;
        deltaPos /= glm::dvec2(size.x, -size.y);
        if (left_button == GLFW_PRESS) {
            mover += glm::vec3(deltaPos*2.0, 0.0f);
        }
        if (right_button == GLFW_PRESS) {
            zoom(deltaPos.y);
        }
        return;
    }
    lastPos = mousePos;
}

void flip_view() {
    if (background.texture->loaded) {
        background.shader.use();
        background.shader.setVec2("flip", 1.0f, -1.0f);
    }
    sprite.shader.use();
    glm::mat4 view;
    scale.y = -scale.y;
    mover.y = -mover.y;
    view = glm::translate(view, mover);
    view = glm::scale(view, scale*glm::vec3(1.0f, aspect_ratio, 1.0f));
    sprite.shader.setMat4("u_View", glm::value_ptr(view));
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int modifier) {
    switch (key) {
    // Help
    case GLFW_KEY_H:
        if (action == GLFW_PRESS) {
            std::cout << help << std::endl;
        }
        break;
    // Pause / Resume
    case GLFW_KEY_SPACE:
        if (action == GLFW_PRESS) {
            if (sprite.is_looping() == false) {
                sprite.replay();
                break;
            }
            if (sprite.is_paused()) {
                sprite.unpause();
                std::cout << "Playing\n";
            }
            else {
                sprite.pause();
                std::cout << "Paused\n";
            }
        }
        break;
    // Next animation
    case GLFW_KEY_LEFT:
    case GLFW_KEY_A:
        if (action == GLFW_PRESS) {
            sprite.next_anim();
            std::cout << '\n' << sprite.get_anim_name() << std::endl;
        }
        break;
    // Prev animation
    case GLFW_KEY_RIGHT:
    case GLFW_KEY_S:
        if (action == GLFW_PRESS) {
            sprite.previous_anim();
            std::cout << '\n' << sprite.get_anim_name() << std::endl;
        }
        break;
    // Next frame
    case GLFW_KEY_UP:
    case GLFW_KEY_PERIOD:
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            unsigned int max_frame = sprite.ssPlayer->getMaxFrame() - 1;
            sprite.pause();
            if (modifier == GLFW_MOD_SHIFT) {
                sprite.ssPlayer->setFrameNo(max_frame);
                break;
            }
            unsigned int current_frame = sprite.ssPlayer->getFrameNo();
            sprite.ssPlayer->setFrameNo(current_frame + (current_frame < max_frame));
        }
        break;
    // Previous frame
    case GLFW_KEY_DOWN:
    case GLFW_KEY_COMMA:
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            sprite.pause();
            if (modifier == GLFW_MOD_SHIFT) {
                sprite.ssPlayer->setFrameNo(0);
                break;
            }
            unsigned int current_frame = sprite.ssPlayer->getFrameNo();
            sprite.ssPlayer->setFrameNo(current_frame - (current_frame > 0));
        }
        break;
    // Looping
    case GLFW_KEY_L:
        if (action == GLFW_PRESS) {
            sprite.toggle_looping();
            std::cout << "Looping " << (sprite.is_looping() ? "enabled" : "disabled") << std::endl;
        }
        break;
    // Decelerate
    case GLFW_KEY_1:
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            float play_speed = sprite.get_play_speed();
            if (play_speed <= -2.0f) break;
            sprite.set_play_speed(play_speed - 0.1f);
        }
        else if (action == GLFW_RELEASE)
            std::cout << setprecision(1) << std::fixed << "Play speed: " << sprite.get_play_speed() << '\n';
        break;
    // Accelerate
    case GLFW_KEY_2:
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            float play_speed = sprite.get_play_speed();
            if (play_speed >= 2.0f) break;
            sprite.set_play_speed(play_speed + 0.1f);
            sprite.ssPlayer->setStep(play_speed);
        }
        else if (action == GLFW_RELEASE)
            std::cout << setprecision(1) << std::fixed << "Play speed: " << sprite.get_play_speed() << '\n';
        break;
    // Reinitialize speed
    case GLFW_KEY_3:
        if (action == GLFW_PRESS) {
            sprite.set_play_speed(1.0f);
            std::cout << "Play speed reset\n";
        }
        break;
    // Reinitialize camera
    case GLFW_KEY_C:
        if (action == GLFW_PRESS) {
            mover = glm::vec3(0.0f, 0.0f, 0.0f);
            scale = glm::vec3(fivTwel, fivTwel, 1.0f);
        }
        break;
    // Flip
    case GLFW_KEY_X:
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
            scale.x *= -1.0;
        break;
    // screenshot
    case GLFW_KEY_Q:
        if (action == GLFW_PRESS) {
            std::string folder = "Screenshots/";
            std::filesystem::create_directory(folder);
            folder += sprite.file_name + "/";
            std::filesystem::create_directory(folder);

            GLint vp[4];
            glGetIntegerv(GL_VIEWPORT, vp);

            glClear(GL_COLOR_BUFFER_BIT);
            sprite.draw();

            GLubyte* image = new GLubyte[vp[2] * vp[3] * 4];
            glReadPixels(vp[0], vp[1], vp[2], vp[3], GL_RGBA, GL_UNSIGNED_BYTE, image);
            // Reverse premultiplied alpha
            for (int i = 0; i != vp[2] * vp[3]; ++i) {
                if (image[i*4+3] == 0) continue;
                image[i*4] = GLubyte(std::min(int(image[i*4] * 0xFF) / image[i*4+3], 0xFF));
                image[i*4+1] = GLubyte(std::min(int(image[i*4+1] * 0xFF) / image[i*4+3], 0xFF));
                image[i*4+2] = GLubyte(std::min(int(image[i*4+2] * 0xFF) / image[i*4+3], 0xFF));
            }
            Magick::Image img(vp[2], vp[3], "RGBA", Magick::CharPixel, image);
            img.flip();

            // Remove empty borders
            Magick::Geometry area = img.boundingBox();
            int w = int(std::ceil((area.width() + 10) / 10) * 10);
            int h = int(std::ceil((area.height() + 10) / 10) * 10);
            Magick::Geometry geo = Magick::Geometry(
                w, h,
                ssize_t(std::ceil(area.xOff() - (w - area.width()) / 2)),
                ssize_t(std::ceil(area.yOff() - (h - area.height()) / 2))
            );
            img.crop(geo);

            std::string image_name = sprite.ssPlayer->getPlayAnimeName() + '_' + std::to_string(sprite.ssPlayer->getFrameNo()) + ".png";
            img.write(folder + image_name);

            delete[] image;
            std::cout << "Image saved: " << folder + image_name << std::endl;
        }
        break;
    // Save animation
    case GLFW_KEY_W:
        if (action == GLFW_PRESS) {
            std::string folder = "Screenshots/";
            std::filesystem::create_directory(folder);
            folder += sprite.file_name + "/";
            std::filesystem::create_directory(folder);

            GLint vp[4];
            glGetIntegerv(GL_VIEWPORT, vp);

            GLubyte* image = new GLubyte[vp[2] * vp[3] * 4];
            std::vector<Magick::Image> images(sprite.ssPlayer->getMaxFrame());

            for (int frame = 0; frame != sprite.ssPlayer->getMaxFrame(); ++frame) {
                // Draw with background
                glClear(GL_COLOR_BUFFER_BIT);
                if (background.texture->loaded) {
                    background.shader.use();
                    background.shader.setTexture2D("u_Texture", background.texture->id);
                    background.draw();
                }
                sprite.shader.use();
                sprite.ssPlayer->setFrameNo(frame);
                sprite.ssPlayer->update(0);
                sprite.draw();

                // Read image
                glReadPixels(vp[0], vp[1], vp[2], vp[3], GL_RGBA, GL_UNSIGNED_BYTE, image);
                Magick::Image img(vp[2], vp[3], "RGBA", Magick::CharPixel, image);
                img.flip();

                // Add image to GIF buffer
                img.gifDisposeMethod(MagickCore::DisposeType::BackgroundDispose);
                images[frame] = img;
            }
            std::string image_name = sprite.ssPlayer->getPlayAnimeName() + ".gif";
            Magick::writeImages(images.begin(), images.end(), folder + image_name);

            delete[] image;
            std::cout << "Animation saved: " << folder + image_name << std::endl;
        }
        break;
    }
}

void drop_callback(GLFWwindow* window, int count, const char** paths)
{
    int i;
    for (i = 0; i < count; i++)
        //handle_dropped_file(paths[i]);
        std::cout << paths[i] << std::endl;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    aspect_ratio = float(width) / height;
    glViewport(0, 0, width, height);
    draw(window);
}

std::string replace(const std::string &src, const std::string &what, const std::string &repl)
{
    std::string cp = src;
    while (true) {
        const size_t pos = cp.find(what);
        if (pos == cp.npos) return cp;
        cp = cp.replace(pos, what.size(), repl);
    }
}