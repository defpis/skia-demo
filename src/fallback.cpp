#include "fallback.h"

std::unordered_map<UScriptCode, std::vector<std::string>> TextFallback::families = {
    {UScriptCode::USCRIPT_INVALID_CODE, {"Noto Sans SC"}},
    {UScriptCode::USCRIPT_COMMON, {"Noto Sans SC"}},
    {UScriptCode::USCRIPT_LATIN, {"Are You Serious"}},
    {UScriptCode::USCRIPT_HAN, {"Noto Serif SC"}},
};

std::vector<std::string>& TextFallback::getFallbackFamilies(UChar32 ch32) {
    UErrorCode err = U_ZERO_ERROR;
    UScriptCode code = uscript_getScript(ch32, &err);
    if (U_FAILURE(err)) {
        std::cout << "uscript_getScript failed: " << err << std::endl;
    }

    // std::cout << ch32 << " | " << code << std::endl;

    auto it = TextFallback::families.find(code);
    if (it == TextFallback::families.end()) {
        return TextFallback::families[UScriptCode::USCRIPT_INVALID_CODE];
    } else {
        return it->second;
    }
}

FontMeta& matchMostRelatedFontMeta(FontInfo& fontInfo, FontMeta& fontMeta) {
    auto it = fontInfo.find(fontMeta.style);
    if (it != fontInfo.end()) {
        return it->second;
    }

    bool filterByItalic = false;
    for (auto it = fontInfo.begin(); it != fontInfo.end(); it++) {
        if (it->second.italic == fontMeta.italic) {
            filterByItalic = true;
            break;
        }
    }

    FontMeta* matched = &(fontInfo.begin()->second);
    int min = std::numeric_limits<int>::max();

    for (auto it = fontInfo.begin(); it != fontInfo.end(); it++) {
        if (filterByItalic && it->second.italic != fontMeta.italic) {
            continue;
        }

        int diff = std::abs(it->second.weight - fontMeta.weight);

        if (diff == 0) {
            matched = &(it->second);
            break;
        } else if (diff < min) {
            matched = &(it->second);
            min = diff;
        } else if (diff == min && fontMeta.weight <= 400 ? it->second.weight < matched->weight
                                                         : it->second.weight > matched->weight) {
            matched = &(it->second);
        }
    }

    return *matched;
}

bool TextFallback::checkCharIfNeedFallback(UChar32 ch32, std::string family, std::string weight) {
    // ????????????
    auto it = cannotFallbackChars.find(ch32);
    if (it != cannotFallbackChars.end()) {
        return false;
    }

    FontLoader& fontLoader = FontLoader::getInstance();

    // ???????????????
    FontMeta fontMeta = {"", weight, 400, false, ""};
    FontMeta* fontMetaPtr = fontLoader.getFontMeta(family, weight);
    if (fontMetaPtr != nullptr) {
        fontMeta = *fontMetaPtr;
    }

    // ????????????????????????
    std::vector<std::string>& _families = getFallbackFamilies(ch32);
    std::vector<std::string> families;

    families.push_back(family);
    for (auto& family : _families) {
        families.push_back(family);
    }

    for (int i = 0; i < families.size(); i++) {
        std::string& family = families[i];
        FontInfo* fontInfoPtr = fontLoader.getFontInfo(family);

        // ?????????????????????????????????
        auto it = unavailableFamilies.find(family);
        if (it != unavailableFamilies.end() || fontInfoPtr == nullptr) {
            continue;
        }

        // ?????????????????????
        FontMeta& fbFontMeta = matchMostRelatedFontMeta(*fontInfoPtr, fontMeta);

        std::shared_ptr<SkFont> font = fontLoader.getFont(fbFontMeta.family, fbFontMeta.style);

        if (font != nullptr) {
            SkGlyphID id = font->unicharToGlyph(ch32);
            if (id != 0) {
                return false;
            }
            // ???????????????????????????
        } else {
            // ??????????????????????????????????????????
            return i != families.size() - 1;
        }
    }

    // ????????????????????????????????????????????????
    return false;
}

std::shared_ptr<const SkFont> TextFallback::getCharFallbackFontFromCache(UChar32 ch32, std::string family,
                                                                         std::string weight) {
    // ????????????
    auto it = cannotFallbackChars.find(ch32);
    if (it != cannotFallbackChars.end()) {
        return nullptr;
    }

    FontLoader& fontLoader = FontLoader::getInstance();

    // ???????????????
    FontMeta fontMeta = {"", weight, 400, false, ""};
    FontMeta* fontMetaPtr = fontLoader.getFontMeta(family, weight);
    if (fontMetaPtr != nullptr) {
        fontMeta = *fontMetaPtr;
    }

    // ????????????????????????
    std::vector<std::string>& _families = getFallbackFamilies(ch32);
    std::vector<std::string> families;

    families.push_back(family);
    for (auto& family : _families) {
        families.push_back(family);
    }

    for (int i = 0; i < families.size(); i++) {
        std::string& family = families[i];
        FontInfo* fontInfoPtr = fontLoader.getFontInfo(family);

        // ?????????????????????????????????
        auto it = unavailableFamilies.find(family);
        if (it != unavailableFamilies.end() || fontInfoPtr == nullptr) {
            continue;
        }

        // ?????????????????????
        FontMeta& fbFontMeta = matchMostRelatedFontMeta(*fontInfoPtr, fontMeta);

        std::shared_ptr<SkFont> font = fontLoader.getFont(fbFontMeta.family, fbFontMeta.style);

        if (font != nullptr) {
            SkGlyphID id = font->unicharToGlyph(ch32);
            if (id != 0) {
                return font;
            }
            // ???????????????????????????
        } else {
            // ???????????????????????????
            return nullptr;
        }
    }

    // ????????????????????????????????????????????????
    return nullptr;
}

void TextFallback::_getCharFallbackFont(const int idx, std::vector<std::string> families, UChar32 ch32,
                                        FontMeta fontMeta, std::function<void(std::shared_ptr<const SkFont>)> cb) {

    // ?????????????????????
    if (idx >= families.size()) {
        // ????????????????????????????????????????????????
        cannotFallbackChars.insert(ch32);

        // ??????fallback??????
        std::cout << "Can't fallback char Unicode<" << ch32 << "> by ";
        for (int i = 0; i < families.size(); i++) {
            std::cout << families[i];
            if (i < families.size() - 1) {
                std::cout << ",";
            } else {
                std::cout << std::endl;
            }
        }

        return cb(nullptr);
    }

    FontLoader& fontLoader = FontLoader::getInstance();

    // ????????????
    std::string& family = families[idx];
    FontInfo* fontInfoPtr = fontLoader.getFontInfo(family);

    // ?????????????????????????????????
    auto it = unavailableFamilies.find(family);
    if (it != unavailableFamilies.end() || fontInfoPtr == nullptr) {
        return _getCharFallbackFont(idx + 1, families, ch32, fontMeta, cb);
    }

    // ?????????????????????
    FontMeta& fbFontMeta = matchMostRelatedFontMeta(*fontInfoPtr, fontMeta);

    std::shared_ptr<SkFont> font = fontLoader.getFont(fbFontMeta.family, fbFontMeta.style);

    if (font == nullptr) {
        fontLoader.loadFont(fbFontMeta.family, fbFontMeta.style,
                            [this, fbFontMeta, &fontLoader, idx, families, ch32, fontMeta, cb](int err) {
                                if (err == 0) {
                                    // ???????????????????????????????????????
                                    std::shared_ptr<SkFont> font =
                                        fontLoader.getFont(fbFontMeta.family, fbFontMeta.style);

                                    SkGlyphID id = font->unicharToGlyph(ch32);
                                    if (id != 0) {
                                        return cb(font);
                                    }
                                } else {
                                    std::cout << "Can't Load font " << fbFontMeta.family << "-" << fbFontMeta.style
                                              << "!" << std::endl;
                                    this->unavailableFamilies.insert(fbFontMeta.family);
                                }

                                return this->_getCharFallbackFont(idx + 1, families, ch32, fontMeta, cb);
                            });
    } else {
        SkGlyphID id = font->unicharToGlyph(ch32);
        if (id != 0) {
            return cb(font);
        }

        return _getCharFallbackFont(idx + 1, families, ch32, fontMeta, cb);
    }
}

void TextFallback::getCharFallbackFont(UChar32 ch32, std::string family, std::string weight,
                                       std::function<void(std::shared_ptr<const SkFont>)> cb) {
    // ????????????
    auto it = cannotFallbackChars.find(ch32);
    if (it != cannotFallbackChars.end()) {
        return cb(nullptr);
    }

    FontLoader& fontLoader = FontLoader::getInstance();

    // ???????????????
    FontMeta fontMeta = {"", weight, 400, false, ""};
    FontMeta* fontMetaPtr = fontLoader.getFontMeta(family, weight);
    if (fontMetaPtr != nullptr) {
        fontMeta = *fontMetaPtr;
    }

    // ????????????????????????
    std::vector<std::string>& _families = getFallbackFamilies(ch32);
    std::vector<std::string> families;

    families.push_back(family);
    for (auto& family : _families) {
        families.push_back(family);
    }

    // ?????????????????????????????????
    _getCharFallbackFont(0, families, ch32, fontMeta, cb);
}