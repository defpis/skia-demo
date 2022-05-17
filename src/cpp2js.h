#pragma once

#include "emscripten.h"
#include "nlohmann/json.hpp"
#include <iostream>
#include <unordered_map>


void runCPPCallback(int err, uintptr_t ptr, size_t size, int seq);

void callJSFunc(const char* funcName, const char* strData, std::function<void(int, uintptr_t, size_t)> cb);
