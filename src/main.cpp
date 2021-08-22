#include <iostream>
#include <iomanip>
#include <filesystem>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <regex>

#include <Magick++.h>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "texture.h"
#include "quad.h"
#include "sprite.h"

#ifndef WIN_WIDTH
#define WIN_WIDTH 1200
#endif
#ifndef WIN_HEIGHT
#define WIN_HEIGHT 800
#endif

static GLFWwindow *window;
Sprite sprite;
static Quad background; // the background quad/plane; has its own shader
static std::string backgroundPath;

static unsigned int windowWidth = WIN_WIDTH;
static unsigned int windowHeight = WIN_HEIGHT;
static float frame_rate = 60.0f;

static glm::vec3 mover(0.0f, -0.5f, 0.0f); // camera position
// Map window coordinate (0~WIN_WIDTH,0~WIN_HEIGHT) with OpenGL coordinate (-1~1)
static glm::vec3 scale(2.0f / WIN_WIDTH, 2.0f / WIN_HEIGHT, 1.0f); // camera view scale

static std::queue<std::function<void()>> savers;
static std::mutex saveMutex;

void screenshotThread();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int modifier);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void drop_callback(GLFWwindow* window, int count, const char** paths);
void applyArgument();
void handleArguments(int argc, char **argv);
void handleEvents(GLFWwindow* window);
void draw(GLFWwindow* window);
GLFWwindow *initOpenGL();
std::string replace(const std::string &src, const std::string &what, const std::string &repl);

static std::string help = // hotkeys
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
    sprite.dir = replace(argv[0], "\\", "/");
    sprite.dir = sprite.dir.substr(0, sprite.dir.rfind("/") + 1);
    sprite.resman = ss::ResourceManager::getInstance();
    backgroundPath = sprite.dir + "background.png";

    std::vector<std::string> shader_name_list{
        sprite.dir + "shaders/sprite.vert",
        sprite.dir + "shaders/sprite.frag",
        sprite.dir + "shaders/background.vert",
        sprite.dir + "shaders/background.frag"
    };
    /**/
    handleArguments(argc, argv+1);
    /*/
    //sprite.file_name = "images/ch00_27_Freya_F_Normal_TransBattle/ch00_27_Freya_F_Normal_TransBattle.ssbp";
    //sprite.file_name = "images/ch00_27_Freya_F_Normal/ch00_27_Freya_F_Normal.ssbp";
    //sprite.file_name = "images/ch04_24_Marc_F_Dark04/ch04_24_Marc_F_Dark04.ssbp";
    sprite.file_name = "images/ch04_12_Tiki_F_Normal/ch04_12_Tiki_F_Normal.ssbp";
    /**/

    try {
        window = initOpenGL();

        // initialize shaders & geometry
        sprite.init(shader_name_list[0], shader_name_list[1]);
        background.init(shader_name_list[2], shader_name_list[3]);
        background.texture = new Texture(backgroundPath.c_str(), true); // load background image

        sprite.ssPlayer = ss::Player::create(sprite.resman);

        sprite.file_name = sprite.resman->addData(sprite.file_name);
        sprite.ssPlayer->setData(sprite.file_name, &sprite.animation_list);
        sprite.ssPlayer->play(sprite.animation_list[0], 1);
        sprite.ssPlayer->setGameFPS(frame_rate);

        applyArgument();

        std::cout << help << '\n' << sprite.file_name << "\nNumber of animations: " << sprite.animation_list.size() << "\n\n" << sprite.ssPlayer->getPlayAnimeName() << std::endl;

        std::thread fileSaver(screenshotThread);

        // render loop
        while (!glfwWindowShouldClose(window)) {
            draw(window);
            handleEvents(window);
            std::this_thread::sleep_for(std::chrono::milliseconds(int(1000 / frame_rate)));
        }
        glfwTerminate();
        window = nullptr;
        fileSaver.join();
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        glfwTerminate();
        return 1;
    }

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
    background.shader.setVec2("u_Coef", float(background.texture->width) / windowWidth,
                                        float(background.texture->height) / windowHeight);
    background.shader.setTexture2D("u_Texture", background.texture->id);
    background.shader.setBool("u_UseTexture", background.texture->loaded);
    background.draw();

    sprite.shader.use();
    sprite.ssPlayer->update(deltaTime);    //Player update
    sprite.draw();    //Draw a layer

    glm::mat4 view = glm::scale(glm::translate(glm::mat4(1.0f), mover), scale);
    sprite.shader.setMat4("u_View", glm::value_ptr(view));

    sprite.shader.setFloat("u_Time", currentTime);

    //glfw: swap buffers and poll IO events (keys pressed/releaed, mouse moved etc.)
    glfwSwapBuffers(window);
}

void zoom(double delta) {
    float x = float(delta)*scale.x*2.0f;
    float y = float(delta)*scale.y*2.0f;
    scale += glm::vec3(x, y, 0.0f);
    float threshold = 0.001f;
    if (scale.y < threshold) {
        scale.x = threshold * scale.x / scale.y;
        scale.y = threshold;
    }
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

void handleArguments(int argc, char **argv) {
    for (int i = 1; i != argc; ++i, ++argv) {
        if ((i != argc-1 && strcmp(*argv, "-b") == 0) || strncmp(*argv, "--bind=", 7) == 0) {
            const char *arg;
            if (strncmp(*argv, "--", 2) != 0) {
                arg = *(++argv); ++i;
            } else
                arg = *argv + 7;
            const char *separator = strchr(arg, ':');
            if (separator != nullptr) {
                std::string part(arg, separator - arg);
                std::string ssbp;
                std::string newElem = separator + 1;
                separator = strrchr(newElem.c_str(), ':');
                if (separator != nullptr) {
                    ssbp = newElem.substr(0, separator - newElem.c_str());
                    newElem = separator + 1;
                }
                sprite.overrided_parts[replace(part, "\\", "/")] = {replace(ssbp, "\\", "/"), replace(newElem, "\\", "/")};
            }
        } else if ((i != argc-1 && strcmp(*argv, "-bg") == 0) || strncmp(*argv, "--background=", 13) == 0) {
            const char *arg;
            if (strncmp(*argv, "--", 2) != 0) {
                arg = *(++argv); ++i;
            } else
                arg = *argv + 13;
            backgroundPath = arg;
        } else if ((i != argc-1 && strcmp(*argv, "-p") == 0) || strncmp(*argv, "--position=", 11) == 0) {
            const char *arg;
            if (strncmp(*argv, "--", 2) != 0) {
                arg = *(++argv); ++i;
            } else
                arg = *argv + 11;
            std::cmatch m;
            if (!std::regex_match(arg, m, std::regex(R"((-?\d+|-?\d*\.\d+)(px|%|),(-?\d+|-?\d*\.\d+)(px|%|))"))) {
                std::cerr << "Invalid argument " << arg << " for position" << std::endl;
                continue;
            }
            float v1 = (float)atof(m[1].first);
            float v2 = (float)atof(m[3].first);
            mover.x = v1 / (*m[2].first == 'p' ? windowWidth : *m[2].first == '%' ? 100 : 1) * 2 - 1;
            mover.y = v2 / (*m[4].first == 'p' ? windowHeight : *m[4].first == '%' ? 100 : 1) * 2 - 1;
        } else
            sprite.file_name = replace(*argv, "\\", "/");
    }
    if (sprite.file_name.empty()) {
        std::cout << "Drag an ssbp file here then press enter.\n";
        std::cin >> sprite.file_name;
    }
}

void scroll_callback(GLFWwindow * window, double xoffset, double yoffset)
{
    zoom(yoffset*0.06);
}

void flip_view() {
    if (background.texture->loaded) {
        background.shader.use();
        background.shader.setVec2("flip", 1.0f, -1.0f);
    }
    sprite.shader.use();
    scale.y = -scale.y;
    mover.y = -mover.y;
    glm::mat4 view = glm::scale(glm::translate(glm::mat4(1.0f), mover), scale);
    sprite.shader.setMat4("u_View", glm::value_ptr(view));
}

void key_save_screen()
{
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
    size_t extraW = 10 + area.width() / 10 * 10 - area.width();
    size_t extraH = 10 + area.height() / 10 * 10 - area.height();
    Magick::Geometry geo = Magick::Geometry(
        area.width() + extraW,
        area.height() + extraH,
        area.xOff() - extraW / 2,
        area.yOff() - extraH / 2
    );
    img.crop(geo);

    std::filesystem::create_directories("Screenshots/" + sprite.file_name);
    std::string image_name = "Screenshots/" + sprite.file_name + "/" + sprite.ssPlayer->getPlayAnimeName() +
                             "_" + std::to_string(sprite.ssPlayer->getFrameNo()) + ".png";
    std::unique_lock lock(saveMutex);
    savers.push([img,image,image_name]() {
        Magick::Image(img).write(image_name);
        delete[] image;
        std::cout << "Image saved: " << image_name << std::endl;
    });
}

void key_save_animation()
{
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    GLubyte* image = new GLubyte[vp[2] * vp[3] * 4];
    std::vector<Magick::Image> *images = new std::vector<Magick::Image>(sprite.ssPlayer->getMaxFrame());
    glm::u64mat2x2 size(vp[2], vp[3], 0, 0);

    int animFps = sprite.ssPlayer->getAnimFps();
    bool looping = sprite.is_looping();
    int nbFrame = sprite.ssPlayer->getMaxFrame();
    for (int frame = 0; frame != nbFrame; ++frame) {
        glfwPollEvents();
        // First draw to get size
        SSLOG("%d / %d", frame, nbFrame);
        glClear(GL_COLOR_BUFFER_BIT);
        sprite.shader.use();
        sprite.ssPlayer->setFrameNo(frame);
        sprite.ssPlayer->update(0);
        sprite.draw();
        glReadPixels(vp[0], vp[1], vp[2], vp[3], GL_RGBA, GL_UNSIGNED_BYTE, image);
        
        Magick::Image img(vp[2], vp[3], "RGBA", Magick::CharPixel, image);
        Magick::Geometry tmpSize = img.boundingBox();
        if (size[0].x > (size_t)tmpSize.xOff())             size[0].x = tmpSize.xOff();
        if (size[1].x < tmpSize.xOff()+tmpSize.width())     size[1].x = tmpSize.xOff()+tmpSize.width();
        if (size[0].y > (size_t)tmpSize.yOff())             size[0].y = tmpSize.yOff();
        if (size[1].y < tmpSize.yOff()+tmpSize.height())    size[1].y = tmpSize.yOff()+tmpSize.height();

        // Actual draw with background
        if (background.texture->loaded) {
            glClear(GL_COLOR_BUFFER_BIT);
            background.shader.use();
            background.shader.setTexture2D("u_Texture", background.texture->id);
            background.draw();
            sprite.shader.use();
            sprite.draw();
            glReadPixels(vp[0], vp[1], vp[2], vp[3], GL_RGBA, GL_UNSIGNED_BYTE, image);
            img = Magick::Image(vp[2], vp[3], "RGBA", Magick::CharPixel, image);
        }
        glfwSwapBuffers(window);

        // Add image to GIF buffer
        img.flip();
        img.gifDisposeMethod(MagickCore::DisposeType::BackgroundDispose);
        img.animationDelay(100 * frame / animFps - 100 * (frame-1) / animFps);
        img.animationIterations(looping ? 0 : 1);
        (*images)[frame] = img;
    }

    // Remove useless borders
    size_t extraW = 10 + (size[1].x - size[0].x) / 10 * 10 - (size[1].x - size[0].x);
    size_t extraH = 10 + (size[1].y - size[0].y) / 10 * 10 - (size[1].y - size[0].y);
    Magick::Geometry cropArea(
        size[1].x - size[0].x + extraW,
        size[1].y - size[0].y + extraH,
        size[0].x - extraW / 2,
        vp[3] - size[1].y - extraH / 2 // size use unflipped coordinate, so the y offset must be reversed
    );
    for (size_t i = 0; i != images->size(); ++i) {
        (*images)[i].modifyImage();
        (*images)[i].crop(cropArea);
        (*images)[i].page(Magick::Geometry(cropArea.width(), cropArea.height()));
    }

    std::string image_name = "Screenshots/" + sprite.file_name + "/" +
                             sprite.ssPlayer->getPlayAnimeName() + ".gif";
    std::filesystem::create_directories("Screenshots/" + sprite.file_name);
    std::unique_lock lock(saveMutex);
    savers.push([images,image,image_name]() {
        std::cout << "Saving " << image_name << "..." << std::endl;
        Magick::writeImages(images->begin(), images->end(), image_name);
        delete[] image;
        delete images;
        std::cout << "Animation saved: " << image_name << std::endl;
    });
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
            applyArgument();
            std::cout << '\n' << sprite.get_anim_name() << std::endl;
        }
        break;
    // Prev animation
    case GLFW_KEY_RIGHT:
    case GLFW_KEY_D:
        if (action == GLFW_PRESS) {
            sprite.previous_anim();
            applyArgument();
            std::cout << '\n' << sprite.get_anim_name() << std::endl;
        }
        break;
    // Next frame
    case GLFW_KEY_UP:
    case GLFW_KEY_W:
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
    case GLFW_KEY_S:
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
            mover = glm::vec3(0.0f, -0.5f, 0.0f);
            scale = glm::vec3(2.0f / windowWidth, 2.0f / windowHeight, 1.0f);
        }
        break;
    // Flip
    case GLFW_KEY_X:
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
            scale.x *= -1.0;
        break;
    // screenshot
    case GLFW_KEY_Q:
        if (action == GLFW_PRESS)
            key_save_screen();
        break;
    // Save animation
    case GLFW_KEY_E:
        if (action == GLFW_PRESS)
            key_save_animation();
        break;
    }
}

void applyArgument()
{
    for (auto &[part, pair] : sprite.overrided_parts) {
        if (std::regex_search(pair.second, std::regex(R"(\.png$|\.webp$)"))) {
            int textureId = -1;
            for (int i = 0; i < sprite.textures.size() && sprite.textures[i]; ++i)
                if (sprite.textures[i]->file_name == pair.second)
                    textureId = i+1;
            if (textureId == -1)
                textureId = ss::SSTextureLoad(pair.second.c_str(), ss::SsTexWrapMode::clamp, ss::SsTexFilterMode::linear);
            sprite.ssPlayer->setPartTexture(part, textureId, sprite.textures[textureId-1]->width, sprite.textures[textureId-1]->height);
        } else {
            std::string &ssbp = pair.first.empty() ? sprite.file_name : pair.first;
            if (ssbp.length() > 5 && strcmp(ssbp.c_str()+ssbp.length()-5, ".ssbp") == 0)
                ssbp = sprite.resman->addData(replace(ssbp, "\\", "/"));
            ss::Instance param;
            param.clear();
            param.refEndframe = sprite.resman->getMaxFrame(ssbp, pair.second) - 1;
            if (param.refEndframe < 0) continue;
            param.refloopNum = 0;
            param.infinity = true;
            param.independent = true;
            sprite.ssPlayer->setPartAnime(part, ssbp, pair.second, &param);
        }
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
    scale *= glm::vec3(float(windowWidth) / float(width), float(windowHeight) / float(height), 1.f);
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
    draw(window);
}

void screenshotThread()
{
    while (window != nullptr || !savers.empty()) {
        if (savers.empty())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        else {
            savers.front()();
            std::unique_lock lock(saveMutex);
            savers.pop();
        }
    }
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
    glfwSetWindowSizeLimits(window, 300, 300, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSwapInterval(60 / int(frame_rate)); //test //0 = unlocked frame rate; 1=60fps; 2=30fps; 3=20fps
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GL_TRUE); // test

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("Failed to initialize GLAD");
    glEnable(GL_BLEND);

    return window;
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