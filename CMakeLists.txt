cmake_minimum_required(VERSION 3.22)
project(skia_demo)

set(CMAKE_CXX_STANDARD 20)

aux_source_directory(${CMAKE_SOURCE_DIR}/src SOURCES)

add_executable(${PROJECT_NAME} bindings.cpp ${SOURCES})

include_directories(
        ${CMAKE_SOURCE_DIR}/_download/json/single_include
        ${CMAKE_SOURCE_DIR}/_download/skia
        ${CMAKE_SOURCE_DIR}/_download/skia/third_party/externals/emsdk/upstream/emscripten/cache/sysroot/include
)

target_link_libraries(${PROJECT_NAME}
        ${CMAKE_SOURCE_DIR}/_download/skia/out/canvaskit_wasm/libskia.a
        ${CMAKE_SOURCE_DIR}/_download/skia/out/canvaskit_wasm/modules/canvaskit/fonts/NotoMono-Regular.ttf.ninja.cpp
        )

set(CMAKE_BUILD_TYPE Debug) # 同时设置CMAKE_BUILD_TYPE=Debug和-g3开启浏览器调试

# https://emscripten.org/docs/tools_reference/emcc.html#emccdoc
# https://github.com/emscripten-core/emscripten/blob/main/src/settings.js
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "\
    -g3 \
    -l embind \
    --pre-js ${CMAKE_SOURCE_DIR}/src/js/utils.js \
    -s NO_DISABLE_EXCEPTION_CATCHING \
    -s LLD_REPORT_UNDEFINED \
    -s MODULARIZE \
    -s EXPORT_ES6 \
    -s USE_ICU \
    -s USE_FREETYPE \
    -s ALLOW_MEMORY_GROWTH \
    -s MAX_WEBGL_VERSION=2 \
    -s INITIAL_MEMORY=128MB \
    -s EXPORTED_FUNCTIONS=['_main','_malloc','_free'] \
    --embed-file ${CMAKE_SOURCE_DIR}/_download/icudt68l-lite.dat@/icudt68l.dat \
")

# icudt68l.dat => ${CMAKE_SOURCE_DIR}/_download/skia/third_party/externals/emsdk/upstream/emscripten/cache/ports/icu/icu/source/data/in/icudt68l.dat
