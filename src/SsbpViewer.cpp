#include <iostream>
#include <regex>

#include <GLFW/glfw3.h>

#include "ssbpViewer.h"
#include "ssbpResource.h"

const char *helpString = R"(
A / D:    Previous / Next frame
W / S:    Previous / Next animation
Space:    Pause / Resume
L:        Enable / Disable animation loop
X:        Flip image (Horizontal flip)
C:        Reinitialize camera
1, 2, 3:  Decelerate, Reinitialize or Accelerate animation speed
Q:        Save current image as PNG (transparent background)
E:        Save current animation as GIF (current of white background)
H:        Display this hotkey list
)";

static SsbpViewer *_viewer;

SsbpViewer::SsbpViewer(int argc, char **argv)
{
    background = nullptr;
    _viewer = this;
    glfwGetFramebufferSize(SsbpResource::window, &width, &height);
    mover = glm::vec3(0, -0.75, 0);
    scaler = glm::vec3(2.f / width, 2.f / height, 1);
    setViewMatrix();

    glfwSetFramebufferSizeCallback(SsbpResource::window, [](GLFWwindow*, int w, int h) { _viewer->resizeCallback(w,h); });
    glfwSetScrollCallback(SsbpResource::window, [](GLFWwindow*, double, double scroll) { _viewer->scrollCallback(scroll); });
    glfwSetKeyCallback(SsbpResource::window, [](GLFWwindow*, int key, int code, int action, int modifier) { _viewer->keyCallback(key, code, action, modifier); });

    handleArguments(argc, argv);
}

SsbpViewer::~SsbpViewer()
{
    SsbpResource::window = nullptr;
    glfwTerminate();
    _viewer = nullptr;
}

void SsbpViewer::run()
{
    std::cout << helpString << std::endl;
    time = glfwGetTime();
    glfwGetCursorPos(SsbpResource::window, &mouse.x, &mouse.y);

    double currentTime;
    while (!glfwWindowShouldClose(SsbpResource::window)) {
        currentTime = glfwGetTime();
        update(float(currentTime - time));
        render();
        handleEvents();
        time = currentTime;
    }
}

void SsbpViewer::render(bool useBackground)
{
    glClear(GL_COLOR_BUFFER_BIT);

    if (useBackground && background && background->loaded) {
        SsbpResource::quad.set("u_Texture", *background);
        SsbpResource::quad.draw({
            glm::vec3{(-1 - mover.x) / scaler.x, (-1 - mover.y) / scaler.y, 0},
            glm::vec3{(-1 - mover.x) / scaler.x, (1 - mover.y) / scaler.y, 0},
            glm::vec3{(1 - mover.x) / scaler.x, (-1 - mover.y) / scaler.y, 0},
            glm::vec3{(1 - mover.x) / scaler.x, (1 - mover.y) / scaler.y, 0},
        },
        { glm::vec2{0,1}, {0,0}, {1,1}, {1,0} },
        { glm::vec4{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {1,1,1,1} }
        );
    }

    SsbpPlayer::draw();
    
    glfwSwapBuffers(SsbpResource::window);
}

void SsbpViewer::handleArguments(int argc, char **argv)
{
    std::string args;
    for (int i = 1; i < argc; ++i)
        args += " " + std::regex_replace(argv[i], std::regex(" "), "\\ ");
    handleArguments(args);
}

void SsbpViewer::handleArguments(std::string args)
{
    #define stringPattern R"(((?:[^\s\\]|\\\s|\\)+))"
    #define argPattern(smallArg, longArg, expected) std::regex("^\\s*(?:-" smallArg "\\s+|--" longArg "(?:=\\s*|\\s+))" expected, std::regex_constants::icase)
    std::smatch m;
    while (!std::regex_match(args, std::regex("^\\s*$")))
        if        (std::regex_search(args, m, argPattern("w", "width", "(\\d+)"))) {
            width = std::stol(m[1]);
            glfwSetWindowSize(SsbpResource::window, width, height);
            glViewport(0, 0, width, height);
            scaler = glm::vec3(2.f / width, 2.f / height, 1);
            setViewMatrix();
            args = m.suffix();
        } else if (std::regex_search(args, m, argPattern("h", "height", "(\\d+)"))) {
            height = std::stol(m[1]);
            glfwSetWindowSize(SsbpResource::window, width, height);
            glViewport(0, 0, width, height);
            scaler = glm::vec3(2.f / width, 2.f / height, 1);
            setViewMatrix();
            args = m.suffix();
        } else if (std::regex_search(args, m, argPattern("bg", "background", stringPattern))) {
            SsbpResource::addTexture("","",m[1]);
            background = &SsbpResource::getTexture("","",m[1]);
            args = m.suffix();
        /*} else if (std::regex_search(tmp, m, argPattern("s", "stretch"))) {
            tmp = m.suffix();
            background.shader.use();
            background.shader.setInt("u_BgType", stoul(m[1]));
        } else if (std::regex_search(tmp, m, argPattern("p", "position"))) {
            tmp = m.suffix();
            if (background.texture && m[2].first[0] == 'p' && m[4].first[0] == 'p') {
                background.shader.use();
                background.shader.setVec2("u_Shift",
                    (stof(m[1]) - 0.5 * windowWidth) / background.texture->width,
                    (-stof(m[3]) - 0.75 * windowHeight) / background.texture->height);
            } else {
                std::cout << "Unsupported offset" << std::endl;
            }*/
        } else if (std::regex_search(args, m, std::regex("^\\s*" stringPattern)) && std::regex_match(m[1].str(), std::regex("^~\\s*.+,.*"))) {
            argumentReplace(std::regex_replace(m[1].str(), std::regex("^~\\s*"), ""));
            args = m.suffix();
        //} else if (std::regex_search(args, m, std::regex("^\\s*\\+\\s*" stringPattern)) && std::regex_search(m[1].str(), std::regex("^(.+,){6}"))) {
        //    // Add arg
        //    // head,images\\accessory\\test.png,0,10,0,1,1
        } else if (std::regex_search(args, m, std::regex("^\\s*" stringPattern)) && std::regex_search(m[1].str(), std::regex("\\.ssbp$"))) {
            try {
                _ssbp = &Ssbp::create(m[1]);
            } catch (const std::invalid_argument &e) {
                std::cerr << "Invalid SSBP file: " << m[1] << std::endl;
                args = m.suffix();
                continue;
            }
            play(_ssbp->animePacks.front().name, _ssbp->animePacks.front().animations.front().name, false);
            args = m.suffix();
        } else {
            std::regex_search(args, m, std::regex("^\\s*" stringPattern));
            std::cerr << "Invalid argument \"" << m[1] << "\"" << std::endl;
            args = m.suffix();
        }

    if (!_ssbp) {
        std::cout << "Drag an ssbp file here then press enter.\n";
        std::string s;
        std::getline(std::cin, s);
        if (std::cin.eof())
            glfwSetWindowShouldClose(SsbpResource::window, 1);
        else
            handleArguments(s);
    } else
        glfwShowWindow(SsbpResource::window);
}

void SsbpViewer::argumentReplace(const std::string &arg)
{
    if (!_ssbp) return;
    size_t breakPos1 = arg.find(','),
            breakPos2 = arg.find(',', breakPos1+1);
    std::string toReplace = arg.substr(0, breakPos1),
                source = arg.substr(breakPos1+1, breakPos2-breakPos1-1);

    if (breakPos2 == std::string::npos && std::regex_match(source, std::regex(".*\\.png|.*\\.webp"))) {
        std::filesystem::path relativePath = std::filesystem::relative(source, _ssbp->_path.parent_path() / _ssbp->imageBaseDir);
        SsbpResource::addTexture(_ssbp->_path, _ssbp->imageBaseDir, relativePath.string());
        bool found = false;
        for (Cell &cell : _ssbp->cells)
            if (cell.textureName == toReplace) {
                cell.texturePath = relativePath.string();
                cell.textureName = relativePath.stem().string();
                found = true;
            }
        if (found) return;
        for (Part &part : _animpack->parts)
            if (part.name == toReplace && _animation->initialParts.at(part.index).cellIndex >= 0) {
                _ssbp->cells.at(_animation->initialParts.at(part.index).cellIndex).texturePath = relativePath.string();
                _ssbp->cells.at(_animation->initialParts.at(part.index).cellIndex).textureName = relativePath.stem().string();
            }
    } else if (breakPos2 != std::string::npos) {
        std::string name = arg.substr(breakPos2+1),
                    packName = name.substr(0, name.find('/')),
                    animName = name.substr(name.find('/')+1);
        auto partIt = std::find_if(_animpack->parts.begin(), _animpack->parts.end(), [&toReplace](const Part &part) { return part.name == toReplace; });
        if (partIt == _animpack->parts.end()) return;
        try {
            Ssbp &ssbp = Ssbp::create(source);
            auto packIt = std::find_if(ssbp.animePacks.begin(), ssbp.animePacks.end(), [&packName](const AnimePack &pack) { return pack.name == packName; });
            if (packIt == ssbp.animePacks.end()) throw std::range_error("");
            auto animIt = std::find_if(packIt->animations.begin(), packIt->animations.end(), [&animName](const Animation &anim) { return anim.name == animName; });
            if (animIt == packIt->animations.end()) throw std::range_error("");
            partIt->_anime = &ssbp;
        } catch (const std::invalid_argument &e) {
            std::cerr << "Failed to open requested SSBP file: " << source << std::endl; return;
        } catch (const std::range_error &e) {
            std::cerr << "Failed to access requested animation: " << name << std::endl; return;
        }
        partIt->type = PartType::Instance;
        partIt->extAnime = name;
        _partsAnime[partIt->index] = SsbpPlayer(*partIt->_anime);
        _partsAnime[partIt->index].play(name);
    } else {
        std::cerr << "Invalid replace command: \"" << arg << '"' << std::endl;
    }
}

void SsbpViewer::handleEvents()
{
    glfwPollEvents();

    glm::dvec2 pos;
    glfwGetCursorPos(SsbpResource::window, &pos.x, &pos.y);
    int left_button = glfwGetMouseButton(SsbpResource::window, GLFW_MOUSE_BUTTON_LEFT);
    int right_button = glfwGetMouseButton(SsbpResource::window, GLFW_MOUSE_BUTTON_RIGHT);
    if (left_button == GLFW_PRESS) {
        mover += glm::vec3((pos - mouse) / glm::dvec2(width, -height) * 2.0, 0.0f);
        setViewMatrix();
    }
    if (right_button == GLFW_PRESS)
        scrollCallback((pos.y - mouse.y) / -height * 20);
    mouse = pos;
}

void SsbpViewer::resizeCallback(int w, int h)
{
    scaler *= glm::vec3(float(width) / w, float(height) / h, 1);
    width = w; height = h;
    setViewMatrix();
    glViewport(0, 0, width, height);
    render();
}

void SsbpViewer::scrollCallback(double y)
{
    scaler *= glm::vec3(1 + y * 0.12, 1 + y * 0.12, 1);
    if (scaler.y < 0.001f) {
        scaler.x = 0.001f * scaler.x / scaler.y;
        scaler.y = 0.001f;
    }
    setViewMatrix();
}

void SsbpViewer::keyCallback(int key, int scancode, int action, int modifier)
{
    if (key == GLFW_KEY_H && action == GLFW_PRESS) {
        std::cout << helpString << std::endl;
    } else if ((key == GLFW_KEY_UP || key == GLFW_KEY_W) && action == GLFW_PRESS) {
        if (_animation - _animpack->animations.data() == _animpack->animations.size() - 1) {
            const AnimePack &pack = _animpack - _ssbp->animePacks.data() == _ssbp->animePacks.size() - 1 ? _ssbp->animePacks.front() : _animpack[1];
            play(pack.name, pack.animations.front().name, loop);
        } else {
            play(_animpack->name, _animation[1].name, loop);
        }
    } else if ((key == GLFW_KEY_DOWN || key == GLFW_KEY_S) && action == GLFW_PRESS) {
        if (_animation == _animpack->animations.data()) {
            const AnimePack &pack = _animpack == _ssbp->animePacks.data() ? _ssbp->animePacks.back() : _animpack[-1];
            play(pack.name, pack.animations.back().name, loop);
        } else {
            play(_animpack->name, _animation[-1].name, loop);
        }
    } else if ((key == GLFW_KEY_RIGHT || key == GLFW_KEY_D) && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        pause = true;
        size_t frame = getFrame();
        if (frame < getMaxFrame()-1)
            setFrame(frame+1);
    } else if ((key == GLFW_KEY_LEFT || key == GLFW_KEY_A) && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
            pause = true;
            size_t frame = getFrame();
            if (frame > 0)
                setFrame(frame-1);
    } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        if (!loop && getFrame() == getMaxFrame()-1) {
            setFrame(0);
            pause = false;
        } else
            pause = !pause;
    } else if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        loop = !loop;
        pause = !loop && pause;
        std::cout << "Looping " << (loop ? "enabled" : "disabled") << std::endl;
    } else if (key == GLFW_KEY_1 && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        speed = std::max(speed - 0.1f, -2.0f);
        std::cout << "Play speed: " << speed << '\n';
    } else if (key == GLFW_KEY_2 && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        speed = 1.0f;
        std::cout << "Play speed reset\n";
    } else if (key == GLFW_KEY_3 && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        speed = std::min(speed + 0.1f, 2.0f);
        std::cout << "Play speed: " << speed << '\n';
    } else if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        mover = glm::vec3(0.0f, -0.5f, 0.0f);
        scaler = glm::vec3(2.0f / width, 2.0f / height, 1.0f);
        setViewMatrix();
    } else if (key == GLFW_KEY_X && action == GLFW_PRESS) {
        scaler.x *= -1.0;
        setViewMatrix();
    } else if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
        float colors[4];
        glGetFloatv(GL_COLOR_CLEAR_VALUE, colors);
        glClearColor(0,0,0,0);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(colors[0], colors[1], colors[2], colors[3]);
        draw();
        Magick::Image img = saver.screen();
        saver.save("Screenshots/" + getFileName() + "/" + getAnimeName() + "_" + std::to_string(getFrame()) + ".png", img, saver.bounds(img));
    } else if (key == GLFW_KEY_E && action == GLFW_PRESS) {
        std::vector<Magick::Image> imgs;
        for (size_t i = 0; i < getMaxFrame(); ++i) {
            setFrame(i);
            render();
            imgs.emplace_back(saver.screen());
            imgs.at(i).animationDelay(100 * (i+1) / getFps() - 100 * i / getFps());
        }
        saver.save("Screenshots/" + getFileName() + "/" + getAnimeName() + ".gif", imgs, saver.bounds(imgs), loop ? Saver::Loop : Saver::SlowLoop);
    }
}
