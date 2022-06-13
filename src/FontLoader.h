#pragma once

#include "cpp2js.h"
#include "libskia.h"
#include <iostream>
#include <unordered_map>

#define FONT_SIZE 1000.0

static void noop(int err){};

template <typename T> class Singleton {
public:
    static inline T& getInstance() {
        static std::unique_ptr<T> _ptr = std::make_unique<T>();
        return *_ptr;
    }

    virtual ~Singleton() = default;

    Singleton(const Singleton&) = delete;

    Singleton& operator=(const Singleton) = delete;

protected:
    Singleton() = default;
};


struct FontMeta {
    std::string family;
    std::string style;
    int weight;
    bool italic;
    std::string path;
};

using FontInfo = std::unordered_map<std::string, FontMeta>;

class FontLoader : public Singleton<FontLoader> {
public:
    std::unordered_map<std::string, FontInfo> fontMetaMap;
    std::unordered_map<std::string, std::shared_ptr<SkFont>> fontMap;
    std::unordered_map<std::string, int> fontLoadingMap;

public:
    FontLoader();

    ~FontLoader() override;

    static std::string genKey(const std::string& fontFamily, const std::string& fontWeight);

    void loadFontMeta(std::function<void(int)> cb = noop);

    void loadFont(const std::string& fontFamily, const std::string& fontWeight, std::function<void(int)> cb = noop);

    FontInfo* getFontInfo(const std::string& fontFamily);

    FontMeta* getFontMeta(const std::string& fontFamily, const std::string& fontWeight);

    std::shared_ptr<SkFont> getFont(const std::string& fontFamily, const std::string& fontWeight);
};
