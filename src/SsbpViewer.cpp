#include <iostream>

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
    _viewer = this;
    glfwGetFramebufferSize(SsbpResource::window, &width, &height);
    mover = glm::vec3(0, -0.75, 0);
    scaler = glm::vec3(2.f / width, 2.f / height, 1);
    setViewMatrix();

    glfwSetFramebufferSizeCallback(SsbpResource::window, [](GLFWwindow*, int w, int h) { _viewer->resizeCallback(w,h); });
    glfwSetScrollCallback(SsbpResource::window, [](GLFWwindow*, double, double scroll) { _viewer->scrollCallback(scroll); });
    glfwSetKeyCallback(SsbpResource::window, [](GLFWwindow*, int key, int code, int action, int modifier) { _viewer->keyCallback(key, code, action, modifier); });

    _ssbp = &Ssbp::create(argv[1]);
    play(_ssbp->animePacks.front().name, _ssbp->animePacks.front().animations.front().name, false);
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
    while (!glfwWindowShouldClose(SsbpResource::window)) {
        double currentTime = glfwGetTime();

        handleEvents();
        glClear(GL_COLOR_BUFFER_BIT);
        update(float(currentTime - time));
        draw();
        glfwSwapBuffers(SsbpResource::window);
        glfwPollEvents();
        time = currentTime;
    }
}

void SsbpViewer::resizeCallback(int w, int h)
{
    scaler *= glm::vec3(float(width) / w, float(height) / h, 1);
    width = w; height = h;
    setViewMatrix();
    glViewport(0, 0, width, height);
    //draw();
    glClear(GL_COLOR_BUFFER_BIT);
    draw();
    glfwSwapBuffers(SsbpResource::window);
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
            glClear(GL_COLOR_BUFFER_BIT);
            setFrame(i);
            draw();
            imgs.emplace_back(saver.screen());
            imgs.at(i).animationDelay(int(100 * (i+1) / getFps() - 100 * i / getFps()));
        }
        saver.save("Screenshots/" + getFileName() + "/" + getAnimeName() + ".gif", imgs, saver.bounds(imgs), loop ? Saver::Loop : Saver::SlowLoop);
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