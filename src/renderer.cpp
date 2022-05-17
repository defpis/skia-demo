#include "renderer.h"


Renderer::Renderer() {
    paint.setAntiAlias(false);
}

Renderer::~Renderer() {
    context->unref();
    surface->unref();
}

int Renderer::init(std::string selector, int width, int height) {
    EmscriptenWebGLContextAttributes attributes;
    emscripten_webgl_init_context_attributes(&attributes);
    attributes.antialias = true;
    attributes.majorVersion = 2;
    attributes.preserveDrawingBuffer = true;
    attributes.enableExtensionsByDefault = true;
    attributes.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context(selector.c_str(), &attributes);
    emscripten_webgl_make_context_current(ctx);

    sk_sp<GrDirectContext> _context = GrDirectContext::MakeGL();
    context = _context;

    GrGLint buffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &buffer);

    GrGLint stencil;
    glGetIntegerv(GL_STENCIL_BITS, &stencil);

    GrGLFramebufferInfo info;
    info.fFBOID = buffer;
    info.fFormat = GL_RGBA8;

    GrBackendRenderTarget backendRenderTarget(width, height, 0, stencil, info);
    GrSurfaceOrigin origin = kBottomLeft_GrSurfaceOrigin;
    SkColorType colorType = kRGBA_8888_SkColorType;

    sk_sp<SkSurface> _surface(SkSurface::MakeFromBackendRenderTarget(context.get(), backendRenderTarget, origin,
                                                                     colorType, nullptr, nullptr));
    surface = _surface;
    surface->ref();

    canvas = surface->getCanvas();

    return 0;
}

void Renderer::render() {
    auto fn = [](double time, void* userData) {
        auto renderer = (Renderer*)userData;

        int saveCount = renderer->canvas->save();      // 保存状态
        renderer->canvas->clear(renderer->clearColor); // 清空画布

        renderer->draw();

        renderer->canvas->flush();                   // 绘制到屏幕
        renderer->canvas->restoreToCount(saveCount); // 回退状态

        return 1; // 返回true才能继续调用
    };
    emscripten_request_animation_frame_loop(fn, this);
}

void Renderer::draw() {
    paint.reset();

    // paint.setColor(SkColorSetARGB(0xFF, 0x00, 0x00, 0x00));
    // canvas->drawRect(SkRect::MakeXYWH(0, 0, 100, 100), paint);

    drawFn(0);
}


void startRender(std::string selector, int width, int height) {
    Renderer& renderer = Renderer::getInstance();
    renderer.init(selector, width, height);
    renderer.render();
}
