```bash
# depot_tools
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH="${PWD}/depot_tools:${PATH}"

# bazel
brew install bazelisk

# skia
git clone https://skia.googlesource.com/skia.git
python tools/git-sync-deps
python tools/embed_resources.py --name SK_EMBEDDED_FONTS --input modules/canvaskit/fonts/NotoMono-Regular.ttf --output modules/canvaskit/fonts/NotoMono-Regular.ttf.cpp --align 4

# emcc
source "/Users/bytedance/Desktop/skia_demo/_download/skia/third_party/externals/emsdk/emsdk_env.sh"

emcc bindings.cpp -s USE_ICU
emcc bindings.cpp -s USE_FREETYPE
```
