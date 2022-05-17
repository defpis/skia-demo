#pragma once

#include "emscripten/bind.h"
#include "src/FontLoader.h"
#include "src/cpp2js.h"
#include "src/layout.h"
#include "src/renderer.h"
#include "src/rstring.h"


void afterJSCallbackMounted();

EMSCRIPTEN_BINDINGS(module) {
    emscripten::function("afterJSCallbackMounted", &afterJSCallbackMounted);

    emscripten::function("startRender", &startRender);

    emscripten::function("layout", &layout);
    emscripten::function("runCPPCallback", &runCPPCallback);
}
