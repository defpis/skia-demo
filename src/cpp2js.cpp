#include "cpp2js.h"


EM_JS(void, runJSFunc, (const char* funcName, const char* strData, int seq),
      { Module.runJSFunc(UTF8ToString(funcName), UTF8ToString(strData), seq); })

int callbackSeq = 0;
std::unordered_map<int, std::vector<std::function<void(int, uintptr_t, size_t)>>> callbackMap;

void runCPPCallback(int err, uintptr_t ptr, size_t size, int seq) {
    auto cb = callbackMap.find(seq);
    if (cb == callbackMap.end()) {
        return;
    }

    for (auto& fn : cb->second) {
        fn(err, ptr, size);
    }

    callbackMap.erase(seq);
}

int callJSFunc(const char* funcName, const char* strData, std::function<void(int, uintptr_t, size_t)> cb) {
    int seq = callbackSeq;
    callbackSeq++;
    callbackMap[seq] = {cb};
    runJSFunc(funcName, strData, seq);
    return seq;
}

void appendCallback(int seq, std::function<void(int, uintptr_t, size_t)> cb) {
    auto it = callbackMap.find(seq);
    if (it == callbackMap.end()) {
        return;
    }
    it->second.push_back(cb);
}
