#include "bindings.h"


void afterJSCallbackMounted() {
    FontLoader& fontLoader = FontLoader::getInstance();
    fontLoader.loadFontMeta([&fontLoader](int err) {
        fontLoader.loadFont(DEFAULT_FONT_FAMILY, DEFAULT_FONT_WEIGHT, [](int err) { layout(); });
    });
}

int main() {
    std::cout << "wasm inited!" << std::endl;

    UErrorCode code = setCommonData("/icudt68l.dat");
    if (U_FAILURE(code)) {
        std::cout << "setCommonData failed: " << code << std::endl;
        return -1;
    }

    return 0;
}
