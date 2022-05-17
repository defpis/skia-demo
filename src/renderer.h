#pragma once

#include "FontLoader.h"
#include "GL/gl.h"
#include "emscripten/html5.h"
#include "libskia.h"
#include "rstring.h"

void startRender(std::string selector, int width, int height);

class Renderer : public Singleton<Renderer> {
private:
    SkColor clearColor = SkColorSetARGB(0xFF, 0xEE, 0xEE, 0xEE);

public:
    sk_sp<GrDirectContext> context = nullptr;
    sk_sp<SkSurface> surface = nullptr;

    SkCanvas* canvas = nullptr;
    SkPaint paint;

    std::function<void(int)> drawFn = noop;

public:
    Renderer();

    ~Renderer() override;

    int init(std::string selector, int width, int height);

    void render();

    void draw();
};