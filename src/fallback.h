#pragma once

#include "FontLoader.h"
#include "libicu.h"
#include "libskia.h"
#include <functional>
#include <set>
#include <unordered_map>
#include <vector>


class TextFallback : public Singleton<TextFallback> {
public:
    static std::unordered_map<UScriptCode, std::vector<std::string>> families;

    std::set<UChar32> cannotFallbackChars;
    std::set<std::string> unavailableFamilies;

private:
    void _getCharFallbackFont(const int idx, std::vector<std::string>& families, UChar32 ch32, FontMeta& fontMeta,
                              std::function<void(std::shared_ptr<const SkFont>)> cb);

public:
    std::vector<std::string>& getFallbackFamilies(UChar32 ch32);

    bool checkCharIfNeedFallback(UChar32 ch32, std::string family, std::string weight);

    std::shared_ptr<const SkFont> getCharFallbackFontFromCache(UChar32 ch32, std::string family, std::string weight);


    void getCharFallbackFont(UChar32 ch32, std::string family, std::string weight,
                             std::function<void(std::shared_ptr<const SkFont>)> cb);
};