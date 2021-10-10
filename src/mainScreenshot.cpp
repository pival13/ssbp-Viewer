#include <iostream>
#include <iomanip>
#include <filesystem>
#include <variant>
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

static std::vector<uint8_t> buffer;

static std::queue<std::pair<std::variant<Magick::Image, std::vector<Magick::Image>>, std::string>> saveImages;
static std::mutex saveMutex;

static const std::vector<std::string> pairSubAnim = {"body_anim/Idle", "body_anim/Start", "body_anim/Ok", "body_anim/Attack1", "body_anim/Attack1_Loop", "body_anim/Damage"};
static const std::vector<std::string> usedSprite = {"body_anim/Idle", "body_anim/Ok", "body_anim/Ready", "body_anim/Jump", "body_anim/Attack1", "body_anim/Attack2", "body_anim/AttackF", "body_anim/Damage", "body_anim/Pairpose", "body_anim/Cheer", "body_anim/Transform"};
                    
void screenshotThread();

// Return sprites without background, with background and their bounds
std::tuple<std::vector<Magick::Image>, std::vector<Magick::Image>, std::vector<Magick::Geometry>> getAnimation(const std::string &anim);

void saveSprite(Magick::Image image, const Magick::Geometry &bound, const std::string &name);
void saveAnimation(const std::vector<Magick::Image> &images, const std::vector<Magick::Geometry> &bounds, const std::string &name);
void combineAnimation(const std::map<std::string, std::pair<std::vector<Magick::Image>, std::vector<Magick::Geometry>>> &anims);

void handleArgument(const std::string &arg);
void applyArgument();
GLFWwindow *initOpenGL();

std::string replace(std::string &src, char oldC, char newC) { std::replace(src.begin(), src.end(), oldC, newC); return src; }
std::regex operator ""_r(const char *s, size_t l) {return std::regex(s, std::regex_constants::icase);}

int main(int argc, char* argv[]) {
    sprite.dir = argv[0];
    sprite.dir = replace(sprite.dir, '\\', '/').substr(0, sprite.dir.rfind("/") + 1);

    sprite.resman = ss::ResourceManager::getInstance();
    backgroundPath = sprite.dir + "background.png";

    std::vector<std::string> shader_name_list{
        //#include "../shaders/sprite.vert"
        //,
        //#include "../shaders/sprite.frag"
        //,
        //#include "../shaders/background.vert"
        //,
        //#include "../shaders/background.frag"
        sprite.dir + "shaders/sprite.vert",
        sprite.dir + "shaders/sprite.frag",
        sprite.dir + "shaders/background.vert",
        sprite.dir + "shaders/background.frag"
    };

    buffer.resize(windowWidth * windowHeight * 4);
    try {
        window = initOpenGL();

        // initialize shaders & geometry
        sprite.init(shader_name_list[0], shader_name_list[1]);
        sprite.ssPlayer = ss::Player::create(sprite.resman);
        background.init(shader_name_list[2], shader_name_list[3]);
        background.shader.use();
        background.shader.setInt("u_BgType", 1);
        background.shader.setVec2("u_Shift", 0, 0);
        std::thread fileSaver(screenshotThread);

        sprite.shader.use();
        sprite.shader.setMat4("u_View", glm::value_ptr(
            glm::scale(
                glm::translate(
                    glm::mat4(1.0f),
                    glm::vec3(0.f, -0.5f, 0.f)),
                glm::vec3(2.f / windowWidth, 2.f / windowHeight, 1.f))));

        // render loop
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
                std::map<std::string, std::pair<std::vector<Magick::Image>, std::vector<Magick::Geometry>>> sprites;
                for (const auto &anim : anims) {
                    if (!std::regex_match(anim, "body_anim/.*"_r) ||
                        (std::regex_match(sprite.file_name, ".*_PairSub"_r) && std::find(pairSubAnim.begin(), pairSubAnim.end(), anim) == pairSubAnim.end()) ||
                        (std::regex_match(sprite.file_name, ".*_PairMain"_r) && anim == "body_anim/Pairpose")) {
                            std::cout << "  \33[1;30;103mIgnoring\33[0m " << anim << std::endl;
                            continue;
                    }
                    std::cout << " " << anim << ": " << sprite.resman->getMaxFrame(sprite.file_name, anim) << " frames" << std::endl;
                    auto [rgbas, rgbs, bounds] = getAnimation(anim);
                    sprites[sprite.ssPlayer->getPlayAnimeName()] = {rgbs, bounds};
                    if (std::find(usedSprite.begin(), usedSprite.end(), anim) != usedSprite.end()) {
                        int frame = sprite.resman->getMaxFrame(sprite.file_name, anim) - 1;
                        if (anim == "body_anim/Cheer")
                            frame /= 2;
                        else if (anim == "body_anim/Idle" || anim == "body_anim/Ok" || anim == "body_anim/Pairpose")
                            frame = 0;
                        else if (anim == "body_anim/Transform" && std::regex_search(sprite.file_name, "Dragon|TransBattle|TransMap"_r))
                            frame = 0;
                        //saveSprite(rgbas.at(frame), bounds.at(frame), "Screenshots/" + sprite.file_name + "/" + sprite.ssPlayer->getPlayAnimeName() + "_" + std::to_string(frame) + ".png");
                    }
                    saveAnimation(rgbs, bounds, "Screenshots/" + sprite.file_name + "/" + sprite.ssPlayer->getPlayAnimeName() + ".gif");
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                sprite.overrided_parts = {
                    {"Wep_BaseR", {"", "blank.png"}},
                    {"Wep_BaseR_Add", {"", "blank.png"}},
                    {"Wep_BaseL", {"", "blank.png"}},
                    {"Wep_BaseL_Add", {"", "blank.png"}}
                };
                //auto [rgbas, _, bounds] = getAnimation("body_anim/Idle");
                //saveSprite(rgbas.at(0), bounds.at(0), "Screenshots/" + sprite.file_name + "/Idle_no_wep.png");
                //combineAnimation(sprites);
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
    while (!std::regex_match(tmp, "\\s*"_r))
        if (std::regex_search(tmp, m, "^\\s*" readString ""_r) && std::regex_search(m[1].str(), "\\.ssbp\"?$"_r)) {
            sprite.file_name = m[1];
            tmp = m.suffix();
        } else if (std::regex_search(tmp, m, "^\\s*(?:-bg\\s+|--background(?:=|\\s+))" readString ""_r)) {
            backgroundPath = m[1];
            background.texture = new Texture(backgroundPath.c_str(), true);
            tmp = m.suffix();
        } else if (std::regex_search(tmp, m, "^\\s*(?:-b\\s+|--bind(?:=|\\s+))" readString ""_r)) {
            std::string bind = m[1];
            tmp = m.suffix();
            if (std::regex_match(bind, m, R"(([^:]+):(.+(?:\.png|\.webp)))"_r)) {
                sprite.overrided_parts[m[1]] = {"", m[2]};
            } else if (std::regex_match(bind, m, R"(([^:]+):(.+):([^:]+))"_r)) {
                sprite.overrided_parts[m[1]] = {m[2], m[3]};
            } else
                std::cerr << "Invalid binding command \"" << bind << "\"" << std::endl;
        } else if (std::regex_search(tmp, m, "^\\s*(?:-s\\s+|--stretch(?:=|\\s+))\\s*(\\d+)"_r)) {
            background.shader.use();
            background.shader.setInt("u_BgType", stoul(m[1]));
            tmp = m.suffix();
        } else if (std::regex_search(tmp, m, "^\\s*(?:-p\\s+|--position(?:=|\\s+))" "(-?\\d*\\.\\d+|-?\\d+)(px|%|),(-?\\d*\\.\\d+|-?\\d+)(px|%|)"_r)) {
            if (background.texture && m[2].first[0] == 'p' && m[4].first[0] == 'p') {
                background.shader.use();
                background.shader.setVec2("u_Shift",
                    (stof(m[1]) - 0.5 * windowWidth) / background.texture->width,
                    (-stof(m[3]) - 0.75 * windowHeight) / background.texture->height);
            } else {
                std::cout << "Unsupported offset" << std::endl;
            }
            tmp = m.suffix();
        } else if (std::regex_search(tmp, m, "^\\s*(?:-w\\s+|--width(?:=|\\s+))(\\d+)"_r)) {
            windowWidth = std::stoul(m[1]);
            glViewport(0, 0, windowWidth, windowHeight);
            if (buffer.size() < windowHeight * windowWidth * 4)
                buffer.resize(windowHeight * windowWidth * 4);
            sprite.shader.use();
            sprite.shader.setMat4("u_View", glm::value_ptr(glm::scale(glm::translate(glm::mat4(1.0f),glm::vec3(0.f, -0.5f, 0.f)),glm::vec3(2.f / windowWidth, 2.f / windowHeight, 1.f))));
            tmp = m.suffix();
        } else if (std::regex_search(tmp, m, "^\\s*(?:-h\\s+|--height(?:=|\\s+))(\\d+)"_r)) {
            windowHeight = std::stoul(m[1]);
            glViewport(0, 0, windowWidth, windowHeight);
            if (buffer.size() < windowHeight * windowWidth * 4)
                buffer.resize(windowHeight * windowWidth * 4);
            sprite.shader.use();
            sprite.shader.setMat4("u_View", glm::value_ptr(glm::scale(glm::translate(glm::mat4(1.0f),glm::vec3(0.f, -0.5f, 0.f)),glm::vec3(2.f / windowWidth, 2.f / windowHeight, 1.f))));
            tmp = m.suffix();
        } else {
            std::cerr << "Invalid argument \"" << tmp << "\"" << std::endl;
            auto idx = tmp.find_first_of(" \t\n");
            tmp = tmp.substr(idx != tmp.npos ? idx+1 : tmp.size());
        }
}

void combineAnimation(const std::map<std::string, std::pair<std::vector<Magick::Image>, std::vector<Magick::Geometry>>> &anims)
{
    auto endIt = anims.cend();
    std::vector<std::map<std::string, std::pair<std::vector<Magick::Image>, std::vector<Magick::Geometry>>>::const_iterator> its;
    its = {anims.find("Idle"), anims.find("Ready")/*, anims.find("Jump")*/, anims.find("Attack1"), anims.find("Attack2")};
    if (std::none_of(its.begin(), its.end(), [&endIt](auto it) {return it == endIt;})) {
        std::vector<Magick::Image> imgs; std::vector<Magick::Geometry> bounds;
        for (auto &it : its) {
            imgs.insert(imgs.end(), it->second.first.begin(), it->second.first.end());
            imgs.back().animationDelay(50);
            bounds.insert(bounds.end(), it->second.second.begin(), it->second.second.end());
        }
        saveAnimation(imgs, bounds, "Screenshots/" + sprite.file_name + "/test1.gif");
    }
}

static Magick::Geometry getBound(const Magick::Image &image)
{
    try {
        Magick::Geometry area = image.boundingBox();
        size_t extraW = 10 + area.width() / 10 * 10 - area.width();
        size_t extraH = 10 + area.height() / 10 * 10 - area.height();
        return Magick::Geometry(
            area.width() + extraW,
            area.height() + extraH,
            area.xOff() - extraW / 2,
            area.yOff() - extraH / 2
        );
    } catch (const std::exception &e) {
        std::cerr << "Error: Empty image" << std::endl;
        return Magick::Geometry("0x0");
    }
}

static Magick::Geometry getBound(const std::vector<Magick::Geometry> &bounds)
{
    glm::u64mat2x2 size = glm::u64mat2x2(windowWidth, windowHeight, 0, 0);
    for (const auto &bound : bounds) {
        if (size[0].x > (size_t)bound.xOff())        size[0].x = bound.xOff();
        if (size[1].x < bound.xOff()+bound.width())  size[1].x = bound.xOff()+bound.width();
        if (size[0].y > (size_t)bound.yOff())        size[0].y = bound.yOff();
        if (size[1].y < bound.yOff()+bound.height()) size[1].y = bound.yOff()+bound.height();
    }
    size_t extraW = 10 + (size[1].x - size[0].x) / 10 * 10 - (size[1].x - size[0].x);
    size_t extraH = 10 + (size[1].y - size[0].y) / 10 * 10 - (size[1].y - size[0].y);
    return Magick::Geometry(
        size[1].x - size[0].x + extraW,
        size[1].y - size[0].y + extraH,
        size[0].x - extraW / 2,
        size[0].y - extraH / 2
    );
}

static Magick::Image getSprite(bool useBackground)
{
    _SSLOG("Drawing");
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_COLOR_BUFFER_BIT);
    if (useBackground && background.texture && background.texture->loaded) {
        _SSLOG(" with background");
        background.shader.use();
        background.shader.setVec2("u_Coef", float(background.texture->width) / windowWidth,
                                            float(background.texture->height) / windowHeight);
        background.shader.setTexture2D("u_Texture", background.texture->id);
        background.shader.setBool("u_UseTexture", true);
        background.draw();
    }
    sprite.shader.use();
    sprite.draw();
    _SSLOG(", reading");
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, windowWidth, windowHeight, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

    // Reverse premultiplied alpha
    _SSLOG(", fixing alpha");
    for (int i = 0; i != windowWidth * windowHeight; ++i) {
        if (buffer[i*4+3] == 0) continue;
        buffer[i*4] = GLubyte(std::min(int(buffer[i*4] * 0xFF) / buffer[i*4+3], 0xFF));
        buffer[i*4+1] = GLubyte(std::min(int(buffer[i*4+1] * 0xFF) / buffer[i*4+3], 0xFF));
        buffer[i*4+2] = GLubyte(std::min(int(buffer[i*4+2] * 0xFF) / buffer[i*4+3], 0xFF));
    }
    _SSLOG(", creating image");
    Magick::Image img = Magick::Image(windowWidth, windowHeight, "RGBA", Magick::CharPixel, buffer.data());
    img.flip();
    
    return img;
}

std::tuple<std::vector<Magick::Image>, std::vector<Magick::Image>, std::vector<Magick::Geometry>>
    getAnimation(const std::string &anim)
{
    sprite.ssPlayer->play(anim, 1);
    applyArgument();
    int animFps = sprite.ssPlayer->getAnimFps();
    int nbFrame = sprite.ssPlayer->getMaxFrame();

    std::vector<Magick::Image> imagesRGB; imagesRGB.reserve(nbFrame);
    std::vector<Magick::Image> imagesRGBA; imagesRGBA.reserve(nbFrame);
    std::vector<Magick::Geometry> bounds; bounds.reserve(nbFrame);
    for (int i = 0; i != nbFrame; ++i) {
        sprite.ssPlayer->setFrameNo(i);
        sprite.ssPlayer->update(0);
        _SSLOG("Frame %d: ", i);
        imagesRGB.emplace_back(getSprite(true));
        _SSLOG("\n    ");
        imagesRGBA.emplace_back(getSprite(false));
        SSLOG("");
        bounds.emplace_back(getBound(imagesRGBA.at(i)));
        imagesRGBA.at(i).gifDisposeMethod(MagickCore::DisposeType::BackgroundDispose);
        imagesRGB.at(i).gifDisposeMethod(MagickCore::DisposeType::BackgroundDispose);
        imagesRGB.at(i).animationDelay(100 * i / animFps - 100 * (i-1) / animFps);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return std::make_tuple(imagesRGBA, imagesRGB, bounds);
}

void saveSprite(Magick::Image image, const Magick::Geometry &bound, const std::string &name)
{
    std::filesystem::create_directories(name.substr(0, name.find_last_of("/\\")));

    image.crop(bound);

    std::unique_lock lock(saveMutex);
    saveImages.emplace(image, name);
}

void saveAnimation(const std::vector<Magick::Image> &images, const std::vector<Magick::Geometry> &bounds, const std::string &name)
{
    std::filesystem::create_directories(name.substr(0, name.find_last_of("/\\")));
    std::string anim = name.substr(name.find_last_of("\\/")+1);
    anim = anim.substr(0, anim.find_last_of('.'));

    Magick::Geometry bound = getBound(bounds);
    std::vector<Magick::Image> imgs;
    for (auto &image : images) {
        imgs.push_back(Magick::Image(image));
        imgs.back().crop(bound);
        imgs.back().page(Magick::Geometry(bound.width(), bound.height()));
    }

    if (anim != "Ok" && anim != "Idle" && anim != "Pairpose" && (anim.size() <= 5 || anim.substr(anim.size()-5) != "_Loop")) {
        imgs.front().animationDelay(30);
        imgs.back().animationDelay(100);
    }

    std::unique_lock lock(saveMutex);
    saveImages.emplace(std::move(imgs), name);
}

void applyArgument()
{
    for (auto &[part, pair] : sprite.overrided_parts) {
        if (std::regex_search(pair.second, R"(\.png$|\.webp$)"_r)) {
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
                ssbp = sprite.resman->addData(replace(ssbp, '\\', '/'));
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
    while (window != nullptr || !saveImages.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!saveImages.empty()) {
            auto &[toSave, name] = saveImages.front();
            if (std::holds_alternative<Magick::Image>(toSave))
                std::get<Magick::Image>(toSave).write(name);
            else
                Magick::writeImages(std::get<std::vector<Magick::Image>>(toSave).begin(), std::get<std::vector<Magick::Image>>(toSave).end(), name);
            std::cout << "  > File saved: " << name << std::endl;
            std::unique_lock lock(saveMutex);
            saveImages.pop();
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