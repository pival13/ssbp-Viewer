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
#define WIN_WIDTH 500
#endif
#ifndef WIN_HEIGHT
#define WIN_HEIGHT 500
#endif

static GLFWwindow *window;
Sprite sprite;
static Quad background; // the background quad/plane; has its own shader
static std::string backgroundPath;

static unsigned int windowWidth = WIN_WIDTH;
static unsigned int windowHeight = WIN_HEIGHT;
static float frame_rate = 60.0f;
static GLuint fbo, rbo;

static std::queue<std::function<void()>> savers;
static std::mutex saveMutex;

void screenshotThread();
void save_screen();
void save_animation();
void handleArgument(const std::string &arg);
void applyArgument();
GLFWwindow *initOpenGL();
std::string replace(const std::string &src, const std::string &what, const std::string &repl);


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

    sprite.overrided_parts["Wep_BaseR"] = {"", "a.png"};
    sprite.overrided_parts["Wep_BaseL"] = {"", "a.png"};

    try {
        window = initOpenGL();

        // initialize shaders & geometry
        sprite.init(shader_name_list[0], shader_name_list[1]);
        //background.init(shader_name_list[2], shader_name_list[3]);
        //background.texture = new Texture(backgroundPath.c_str(), true); // load background image

        sprite.ssPlayer = ss::Player::create(sprite.resman);
        std::thread fileSaver(screenshotThread);

        sprite.shader.use();
        sprite.shader.setMat4("u_View", glm::value_ptr(
            glm::scale(
                glm::translate(
                    glm::mat4(1.0f),
                    glm::vec3(0.f, -0.5f, 0.f)),
                glm::vec3(2.f / windowWidth, 2.f / windowHeight, 1.f))));

        // bin/ssbpViewer.exe $(ls ../BlueStacks/files/assets/Common/Unit/ch00_3*/*.ssbp | sed "s@../BlueStacks/@C:/ProgramData/BlueStacks_nxt/Engine/UserData/SharedFolder/@g")
        // render loop
        //for (char **arg = argv+1; arg - argv != argc; ++arg) {
        while (true) {
            std::string arg;
            std::getline(std::cin, arg);
            if (std::cin.eof())
                break;

            handleArgument(arg);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            try {
                sprite.file_name = sprite.resman->addData(sprite.file_name);
                std::cout << "Loading data from " << sprite.file_name << std::endl;
                std::vector<std::string> anims;
                sprite.ssPlayer->setData(sprite.file_name, &anims);
                sprite.ssPlayer->setGameFPS(frame_rate);
                for (const auto &anim : anims) {
                    static const std::vector<std::string> pairSubAnim = {"body_anim/Idle", "body_anim/Ok", "body_anim/Attack1", "body_anim/Damage"};// "body_anim/Start"
                    if (anim.substr(0, 10) != "body_anim/" || anim == "body_anim/Start" || anim == "body_anim/Idle" || anim.substr(anim.size()-4) == "Loop" ||
                        (anim == "body_anim/Pairpose" && sprite.file_name.substr(sprite.file_name.size()-4) != "Pair") ||
                        (sprite.file_name.substr(sprite.file_name.size()-7) == "PairSub" && std::find(pairSubAnim.begin(), pairSubAnim.end(), anim) == pairSubAnim.end())) {
                        std::cout << "  \33[1;30;103mIgnoring\33[0m " << anim << std::endl;
                        continue;
                    }
                    std::cout << "  Preparing " << anim << "..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    sprite.ssPlayer->play(anim, 1);
                    applyArgument();

                    static const std::vector<std::string> firstFrame = {"body_anim/Idle", "body_anim/Ok", "body_anim/Pairpose"};
                    static const std::vector<std::string> lastFrame = {"body_anim/Ready", "body_anim/Jump", "body_anim/Attack1", "body_anim/Attack2", "body_anim/AttackF", "body_anim/Damage"};
                    if (anim == "body_anim/Transform")
                        if (std::regex_search(sprite.file_name, std::regex("Dragon|TransBattle|TransMap")))
                            sprite.ssPlayer->setFrameNo(0);
                        else
                            sprite.ssPlayer->setFrameNo(sprite.resman->getMaxFrame(sprite.file_name, anim)-1);
                    else if (std::find(firstFrame.begin(), firstFrame.end(), anim) != firstFrame.end())
                        sprite.ssPlayer->setFrameNo(0);
                    else if (std::find(lastFrame.begin(), lastFrame.end(), anim) != lastFrame.end())
                        sprite.ssPlayer->setFrameNo(sprite.resman->getMaxFrame(sprite.file_name, anim)-1);
                    else if (anim == "body_anim/Cheer")
                        sprite.ssPlayer->setFrameNo(sprite.resman->getMaxFrame(sprite.file_name, anim)/2);
                    else {
                        std::cout << "  \33[1;30;106mUnknow anim: \33[0m" << anim << std::endl;
                        continue;
                    }
                    sprite.ssPlayer->update(0);
                    save_screen();
                }
            } catch (const std::exception &e) {
                std::cerr << e.what() << std::endl;
            }
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

void handleArgument(const std::string &arg) {
    sprite.file_name.clear();
    sprite.overrided_parts.clear();

    #define readString R"#((?:(\S+)|"((?:[^"]|\\")+)"))#"

    std::smatch m;
    std::string tmp = arg;
    while (!std::regex_match(tmp, std::regex("\\s*")))
        if (std::regex_search(tmp, m, std::regex("^\\s*" readString)) && std::regex_search(m[1].str(), std::regex("\\.ssbp\"?$"))) {
            sprite.file_name = m[1];
            tmp = m.suffix();
        } else if (std::regex_search(tmp, m, std::regex("^\\s*(?:-bg\\s+|--background(?:=|\\s+))" readString))) {
            backgroundPath = m[1];
            tmp = m.suffix();
        } else if (std::regex_search(tmp, m, std::regex("^\\s*(?:-b\\s+|--bind(?:=|\\s+))" readString))) {
            std::string bind = m[1];
            tmp = m.suffix();
            if (std::regex_match(bind, m, std::regex(R"(([^:]+):(.+(?:\.png|\.webp)))"))) {
                sprite.overrided_parts[m[1]] = {"", m[2]};
            } else if (std::regex_match(bind, m, std::regex(R"(([^:]+):(.+):([^:]+))"))) {
                sprite.overrided_parts[m[1]] = {m[2], m[3]};
            } else
                std::cerr << "Invalid binding command \"" << bind << "\"" << std::endl;
        } else if (std::regex_search(tmp, m, std::regex("^\\s*(?:-w\\s+|--width(?:=|\\s+))(\\d+)"))) {
            windowWidth = std::stoul(m[1]);
            glViewport(0, 0, windowWidth, windowHeight);            
            sprite.shader.use();
            sprite.shader.setMat4("u_View", glm::value_ptr(glm::scale(glm::translate(glm::mat4(1.0f),glm::vec3(0.f, -0.5f, 0.f)),glm::vec3(2.f / windowWidth, 2.f / windowHeight, 1.f))));
            tmp = m.suffix();
        } else if (std::regex_search(tmp, m, std::regex("^\\s*(?:-h\\s+|--height(?:=|\\s+))(\\d+)"))) {
            windowHeight = std::stoul(m[1]);
            glViewport(0, 0, windowWidth, windowHeight);
            sprite.shader.use();
            sprite.shader.setMat4("u_View", glm::value_ptr(glm::scale(glm::translate(glm::mat4(1.0f),glm::vec3(0.f, -0.5f, 0.f)),glm::vec3(2.f / windowWidth, 2.f / windowHeight, 1.f))));
            tmp = m.suffix();
        } else {
            std::cerr << "Invalid argument \"" << arg << "\"" << std::endl;
            tmp = tmp.substr(tmp.find_first_of(" \t\n")+1);
        }
}

void save_screen()
{
    _SSLOG("Drawing");
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_COLOR_BUFFER_BIT);
    sprite.draw();
    //glfwSwapBuffers(window);
    _SSLOG(", allocating buffer");
    GLubyte* image = new GLubyte[windowWidth * windowHeight * 4];
    _SSLOG(", reading");
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, windowWidth, windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // Reverse premultiplied alpha
    _SSLOG(", fixing alpha");
    for (int i = 0; i != windowWidth * windowHeight; ++i) {
        if (image[i*4+3] == 0) continue;
        image[i*4] = GLubyte(std::min(int(image[i*4] * 0xFF) / image[i*4+3], 0xFF));
        image[i*4+1] = GLubyte(std::min(int(image[i*4+1] * 0xFF) / image[i*4+3], 0xFF));
        image[i*4+2] = GLubyte(std::min(int(image[i*4+2] * 0xFF) / image[i*4+3], 0xFF));
    }
    _SSLOG(", creating image");
    Magick::Image img(windowWidth, windowHeight, "RGBA", Magick::CharPixel, image);
    img.flip();

    // Remove empty borders
    _SSLOG(", getting bbox");
    Magick::Geometry area = img.boundingBox();
    size_t extraW = 10 + area.width() / 10 * 10 - area.width();
    size_t extraH = 10 + area.height() / 10 * 10 - area.height();
    Magick::Geometry geo = Magick::Geometry(
        area.width() + extraW,
        area.height() + extraH,
        area.xOff() - extraW / 2,
        area.yOff() - extraH / 2
    );
    _SSLOG(", cropping");
    img.crop(geo);

    std::filesystem::create_directories("Screenshots/" + sprite.file_name);
    std::string image_name = "Screenshots/" + sprite.file_name + "/" + sprite.ssPlayer->getPlayAnimeName() +
                             "_" + std::to_string(sprite.ssPlayer->getFrameNo()) + ".png";
    SSLOG(", saving");
    std::unique_lock lock(saveMutex);
    savers.push([img,image,image_name]() {
        Magick::Image(img).write(image_name);
        delete[] image;
        std::cout << "  > Image saved: " << image_name << std::endl;
    });
}

void save_animation()
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
        std::cout << "  > Animation saved: " << image_name << std::endl;
    });
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

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(1, 1, "SSBP Saver", nullptr, nullptr);
    if (window == nullptr)
        throw std::runtime_error("Failed to create GLFW window");
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeLimits(window, 300, 300, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwSwapInterval(60 / int(frame_rate)); //test //0 = unlocked frame rate; 1=60fps; 2=30fps; 3=20fps

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("Failed to initialize GLAD");
    glViewport(0, 0, windowWidth, windowHeight);
    glEnable(GL_BLEND);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenRenderbuffers(1,&rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, 2000, 2000);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);

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