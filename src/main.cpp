#include <filesystem>
#include <functional>
#include <iostream>
#include <variant>
#include <iomanip>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <regex>

#include "ssbpPlayer.h"
#include "ssbpResource.h"
#include "Screenshot.hpp"

#include <GLFW/glfw3.h>
#include <Magick++.h>

#ifdef _DEBUG
#define debug(s, ...) printf(s, __VA_ARGS__)
#else
#define debug(s, ...) {}
#endif

static int windowWidth;
static int windowHeight;
static std::vector<uint8_t> buffer;

static glm::vec3 mover(0.0f, -0.5f, 0.0f);
static glm::vec3 scaler;

static Ssbp *ssbp = nullptr;
static SsbpPlayer player;
static Saver saver(player);

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

static std::list<std::string> _anims;
static std::list<std::string>::iterator _anim;
void loadAnim(Ssbp &ssbp) { player = ssbp; _anims.clear(); for (auto pack : ssbp.animePacks) for (auto anim : pack.animations) _anims.push_back(pack.name+"/"+anim.name); _anim = _anims.end(); }
void nextAnim() { if (_anim == _anims.end()) return; if (++_anim == _anims.end()) _anim = _anims.begin(); player.play(*_anim, player.loop); }
void prevAnim() { if (_anim == _anims.end()) return; if (_anim == _anims.begin()) _anim = _anims.end(); player.play(*--_anim, player.loop); }
void playAnim(const std::string &anim) { for (auto it = _anims.begin(); it != _anims.end(); ++it) if (*it == anim) _anim = it; player.play(anim, player.loop); }

void framebuffer_size_callback(GLFWwindow*, int width, int height) {
    scaler *= glm::vec3(float(windowWidth) / float(width), float(windowHeight) / float(height), 1.f);
    SsbpResource::quad.set("u_View", glm::scale(glm::translate(glm::mat4(1), mover), scaler));
    windowWidth = width; windowHeight = height;
    glViewport(0, 0, width, height);
    //draw();
    glClear(GL_COLOR_BUFFER_BIT);
    player.draw();
    glfwSwapBuffers(SsbpResource::window);
}

void key_callback(GLFWwindow*, int key, int scancode, int action, int modifier) {
    if (key == GLFW_KEY_H && action == GLFW_PRESS) {
        std::cout << help << std::endl;
    } else if ((key == GLFW_KEY_UP || key == GLFW_KEY_W) && action == GLFW_PRESS) {
        nextAnim();
    } else if ((key == GLFW_KEY_DOWN || key == GLFW_KEY_S) && action == GLFW_PRESS) {
        prevAnim();
    } else if ((key == GLFW_KEY_RIGHT || key == GLFW_KEY_D) && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        player.pause = true;
        size_t frame = player.getFrame();
        if (frame < player.getMaxFrame()-1)
            player.setFrame(frame+1);
    } else if ((key == GLFW_KEY_LEFT || key == GLFW_KEY_A) && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
            player.pause = true;
            size_t frame = player.getFrame();
            if (frame > 0)
                player.setFrame(frame-1);
    } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        if (!player.loop && player.getFrame() == player.getMaxFrame()-1) {
            player.setFrame(0);
            player.pause = false;
        } else
            player.pause = !player.pause;
    } else if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        player.loop = !player.loop;
        player.pause = !player.loop && player.pause;
        std::cout << "Looping " << (player.loop ? "enabled" : "disabled") << std::endl;
    } else if (key == GLFW_KEY_1 && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        player.speed = std::max(player.speed - 0.1f, -2.0f);
        std::cout << "Play speed: " << player.speed << '\n';
    } else if (key == GLFW_KEY_2 && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        player.speed = 1.0f;
        std::cout << "Play speed reset\n";
    } else if (key == GLFW_KEY_3 && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        player.speed = std::min(player.speed + 0.1f, 2.0f);
        std::cout << "Play speed: " << player.speed << '\n';
    } else if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        mover = glm::vec3(0.0f, -0.5f, 0.0f);
        scaler = glm::vec3(2.0f / windowWidth, 2.0f / windowHeight, 1.0f);
        SsbpResource::quad.set("u_View", glm::scale(glm::translate(glm::mat4(1), mover), scaler));
    } else if (key == GLFW_KEY_X && action == GLFW_PRESS) {
        scaler.x *= -1.0;
        SsbpResource::quad.set("u_View", glm::scale(glm::translate(glm::mat4(1), mover), scaler));
    } else if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
        float colors[4];
        glGetFloatv(GL_COLOR_CLEAR_VALUE, colors);
        glClearColor(0,0,0,0);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(colors[0], colors[1], colors[2], colors[3]);
        player.draw();
        Magick::Image img = saver.screen();
        saver.save("Screenshots/" + player.getFileName() + "/" + player.getAnimeName() + "_" + std::to_string(player.getFrame()) + ".png", img, saver.bounds(img));
    } else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        std::vector<Magick::Image> imgs;
        for (size_t i = 0; i < player.getMaxFrame(); ++i) {
            glClear(GL_COLOR_BUFFER_BIT);
            player.setFrame(i);
            player.draw();
            imgs.emplace_back(Magick::Image(saver.screen()));
            imgs.at(i).animationDelay(int(100 * (i+1) / player.getFps() - 100 * i / player.getFps()));
        }
        saver.save("Screenshots/" + player.getFileName() + "/" + player.getAnimeName() + "_Fix.gif", imgs, saver.bounds(imgs), Saver::SlowLoop);
    }
}

void scroll_callback(GLFWwindow *, double /*xoff*/, double yoff)
{
    scaler *= glm::vec3(1 + yoff * 0.12, 1 + yoff * 0.12, 1);
    if (scaler.y < 0.001f) {
        scaler.x = 0.001f * scaler.x / scaler.y;
        scaler.y = 0.001f;
    }
    SsbpResource::quad.set("u_View", glm::scale(glm::translate(glm::mat4(1), mover), scaler));
}

void handleEvent()
{
    glfwPollEvents();

    static glm::dvec2 lastPos;
    glm::dvec2 mousePos;
    glfwGetCursorPos(SsbpResource::window, &mousePos.x, &mousePos.y);
    int left_button = glfwGetMouseButton(SsbpResource::window, GLFW_MOUSE_BUTTON_LEFT);
    int right_button = glfwGetMouseButton(SsbpResource::window, GLFW_MOUSE_BUTTON_RIGHT);
    if (left_button == GLFW_PRESS) {
        mover += glm::vec3((mousePos - lastPos) / glm::dvec2(windowWidth, -windowHeight) * 2.0, 0.0f);
        SsbpResource::quad.set("u_View", glm::scale(glm::translate(glm::mat4(1), mover), scaler));
    }
    if (right_button == GLFW_PRESS)
        scroll_callback(nullptr, 0, (mousePos.y - lastPos.y) / -windowHeight * 20);
    lastPos = mousePos;
}

int main(int n, char **argv)
{
    glfwSetFramebufferSizeCallback(SsbpResource::window, framebuffer_size_callback);
    glfwSetKeyCallback(SsbpResource::window, key_callback);
    glfwSetScrollCallback(SsbpResource::window, scroll_callback);
    //glfwSetDropCallback(SsbpResource::window, drop_callback);

    windowWidth = 500; windowHeight = 500;
    scaler = glm::vec3(2.f/500, 2.f/500, 1);
    SsbpResource::quad.set("u_View", glm::scale(glm::translate(glm::mat4(1), mover), scaler));

    Ssbp *ssbp = &Ssbp::create(argv[1]);
    loadAnim(*ssbp);
    playAnim(ssbp->animePacks.front().name + "/" + ssbp->animePacks.front().animations.front().name);

    static float lastTime = float(glfwGetTime());
    while (!glfwWindowShouldClose(SsbpResource::window)) {
        float currentTime = float(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
    
        handleEvent();
        glClear(GL_COLOR_BUFFER_BIT);
        player.update(deltaTime);
        player.draw();
        glfwSwapBuffers(SsbpResource::window);
        glfwPollEvents();
    }
    SsbpResource::window = nullptr;
    glfwTerminate();
}

//TODO draw, args, drop_callback

char *_ = R"==(
int main(int argc, char* argv[]) {
    std::filesystem::path currentDir = std::filesystem::path(argv[0]).parent_path();
    std::filesystem::path backgroundPath = currentDir / "background.png";

    glfwGetWindowSize(SsbpResource::window, &windowWidth, &windowHeight);
    scaler = glm::vec3(2.0f / windowWidth, 2.0f / windowHeight, 1.0f);

    /**/
    handleArguments(argc, argv+1);
    /*/
    //ssbp = &Ssbp::create("images/ch00_27_Freya_F_Normal_TransBattle/ch00_27_Freya_F_Normal_TransBattle.ssbp");
    //ssbp = &Ssbp::create("images/ch00_27_Freya_F_Normal/ch00_27_Freya_F_Normal.ssbp");
    //ssbp = &Ssbp::create("images/ch04_24_Marc_F_Dark04/ch04_24_Marc_F_Dark04.ssbp");
    ssbp = &Ssbp::create("images/ch04_12_Tiki_F_Normal/ch04_12_Tiki_F_Normal.ssbp");
    /**/

    SsbpPlayer player;
    try {
        std::cout << help << std::endl;
        applyArgument();
        std::thread fileSaver(screenshotThread);
        player.play(ssbp->animePacks.front().name + "/" + ssbp->animePacks.front().animations.front().name);
        // render loop
        while (!glfwWindowShouldClose(SsbpResource::window)) {
            handleEvents();
            draw();
            std::this_thread::sleep_for(std::chrono::milliseconds(int(1000 / 60)));
        }
        fileSaver.join();
        glfwTerminate();
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        glfwTerminate();
        return 1;
    }
    return 0;
}

void draw()
{
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
        } else if ((i != argc-1 && strcmp(*argv, "-s") == 0) || strncmp(*argv, "--stretch=", 10) == 0) {
            const char *arg;
            if (strncmp(*argv, "--", 2) != 0) {
                arg = *(++argv); ++i;
            } else
                arg = *argv + 13;
            backgroundStretch = atoi(arg);
            background.shader.use();
            background.shader.setInt("u_BgType", backgroundStretch);
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

void drop_callback(int count, const char** paths)
{
    int i;
    for (i = 0; i < count; i++)
        //handle_dropped_file(paths[i]);
        std::cout << paths[i] << std::endl;
}

GLFWwindow *initOpenGL()
{
    if (!glfwInit())
        throw std::runtime_error("Failed to init GLFW");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

     glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "SSBP Viewer", nullptr, nullptr);
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
)==";