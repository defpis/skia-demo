#include "fallback.h"

std::unordered_map<UScriptCode, std::vector<std::string>> TextFallback::families = {
    {UScriptCode::USCRIPT_INVALID_CODE, {}},
    {UScriptCode::USCRIPT_COMMON, {}},
    {UScriptCode::USCRIPT_LATIN, {}},
    {UScriptCode::USCRIPT_HAN, {}},
};

std::vector<std::string>& TextFallback::getFallbackFamilies(UChar32 ch32) {
    UErrorCode err = U_ZERO_ERROR;
    UScriptCode code = uscript_getScript(ch32, &err);
    if (U_FAILURE(err)) {
        std::cout << "uscript_getScript failed: " << err << std::endl;
    }

    auto it = TextFallback::families.find(code);
    if (it == TextFallback::families.end()) {
        return TextFallback::families[UScriptCode::USCRIPT_INVALID_CODE];
    } else {
        return it->second;
    }
}

FontMeta& matchMostRelatedFontMeta(FontInfo& fontInfo, FontMeta& fontMeta) {
    return fontMeta;
}

bool TextFallback::checkCharIfNeedFallback(UChar32 ch32, std::string family, std::string weight) {
    // 无法回退
    auto it = cannotFallbackChars.find(ch32);
    if (it != cannotFallbackChars.end()) {
        return false;
    }

    FontLoader& fontLoader = FontLoader::getInstance();

    // 字体元数据
    FontMeta fontMeta = {"", weight, 400, false, ""};
    FontMeta* fontMetaPtr = fontLoader.getFontMeta(family, weight);
    if (fontMetaPtr != nullptr) {
        fontMeta = *fontMetaPtr;
    }

    // 用来找字体的数组
    std::vector<std::string>& _families = getFallbackFamilies(ch32);
    std::vector<std::string> families;

    families.push_back(family);
    for (auto& family : _families) {
        families.push_back(family);
    }

    for (int i = 0; i < families.size(); i++) {
        std::string& family = families[i];
        FontInfo* fontInfoPtr = fontLoader.getFontInfo(family);

        // 无法回退或没有字体信息
        auto it = unavailableFamilies.find(family);
        if (it != unavailableFamilies.end() || fontInfoPtr == nullptr) {
            continue;
        }

        // 匹配最相近字重
        FontMeta& fbFontMeta = matchMostRelatedFontMeta(*fontInfoPtr, fontMeta);

        std::shared_ptr<SkFont> font = fontLoader.getFont(fbFontMeta.family, fbFontMeta.style);

        if (font != nullptr) {
            SkGlyphID id = font->unicharToGlyph(ch32);
            if (id != 0) {
                return false;
            }
            // 继续检查下一个字体
        } else {
            // 如果不是最后一个证明需要回退
            return i != families.size() - 1;
        }
    }

    // 检查所有字体之后，证明不需要回退
    return false;
}

std::shared_ptr<const SkFont> TextFallback::getCharFallbackFontFromCache(UChar32 ch32, std::string family,
                                                                         std::string weight) {
    // 无法回退
    auto it = cannotFallbackChars.find(ch32);
    if (it != cannotFallbackChars.end()) {
        return nullptr;
    }

    FontLoader& fontLoader = FontLoader::getInstance();

    // 字体元数据
    FontMeta fontMeta = {"", weight, 400, false, ""};
    FontMeta* fontMetaPtr = fontLoader.getFontMeta(family, weight);
    if (fontMetaPtr != nullptr) {
        fontMeta = *fontMetaPtr;
    }

    // 用来找字体的数组
    std::vector<std::string>& _families = getFallbackFamilies(ch32);
    std::vector<std::string> families;

    families.push_back(family);
    for (auto& family : _families) {
        families.push_back(family);
    }

    for (int i = 0; i < families.size(); i++) {
        std::string& family = families[i];
        FontInfo* fontInfoPtr = fontLoader.getFontInfo(family);

        // 无法回退或没有字体信息
        auto it = unavailableFamilies.find(family);
        if (it != unavailableFamilies.end() || fontInfoPtr == nullptr) {
            continue;
        }

        // 匹配最相近字重
        FontMeta& fbFontMeta = matchMostRelatedFontMeta(*fontInfoPtr, fontMeta);

        std::shared_ptr<SkFont> font = fontLoader.getFont(fbFontMeta.family, fbFontMeta.style);

        if (font != nullptr) {
            SkGlyphID id = font->unicharToGlyph(ch32);
            if (id != 0) {
                return font;
            }
            // 继续检查下一个字体
        } else {
            // 回退链断开退出循环
            return nullptr;
        }
    }

    // 检查所有字体之后，找不到合适字体
    return nullptr;
}

void TextFallback::_getCharFallbackFont(const int idx, std::vector<std::string>& families, UChar32 ch32,
                                        FontMeta& fontMeta, std::function<void(std::shared_ptr<const SkFont>)> cb) {

    // 超出范围没找到
    if (idx >= families.size()) {
        // 记录无法回退的字符，下次直接跳过
        cannotFallbackChars.insert(ch32);

        // 打印fallback队列
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

    // 当前字体
    std::string& family = families[idx];
    FontInfo* fontInfoPtr = fontLoader.getFontInfo(family);

    // 无法回退或没有字体信息
    auto it = unavailableFamilies.find(family);
    if (it == unavailableFamilies.end() || fontInfoPtr == nullptr) {
        return _getCharFallbackFont(idx + 1, families, ch32, fontMeta, cb);
    }

    // 匹配最相近字重
    FontMeta& fbFontMeta = matchMostRelatedFontMeta(*fontInfoPtr, fontMeta);

    std::shared_ptr<SkFont> font = fontLoader.getFont(fbFontMeta.family, fbFontMeta.style);

    if (font == nullptr) {
        fontLoader.loadFont(fbFontMeta.family, fbFontMeta.style,
                            [this, &fbFontMeta, &fontLoader, idx, &families, ch32, &fontMeta, &cb](int err) {
                                if (err == 0) {
                                    // 异步加载完成，可以同步获取
                                    std::shared_ptr<SkFont> font =
                                        fontLoader.getFont(fbFontMeta.family, fbFontMeta.style);

                                    SkGlyphID id = font->unicharToGlyph(ch32);
                                    if (idx != 0) {
                                        return cb(font);
                                    }
                                }

                                std::cout << "Can't Load font " << fbFontMeta.family << "-" << fbFontMeta.style << "!"
                                          << std::endl;
                                this->unavailableFamilies.insert(fbFontMeta.family);
                                return this->_getCharFallbackFont(idx + 1, families, ch32, fontMeta, cb);
                            });
    } else {
        SkGlyphID id = font->unicharToGlyph(ch32);
        if (idx != 0) {
            return cb(font);
        }

        return _getCharFallbackFont(idx + 1, families, ch32, fontMeta, cb);
    }
}

void TextFallback::getCharFallbackFont(UChar32 ch32, std::string family, std::string weight,
                                       std::function<void(std::shared_ptr<const SkFont>)> cb) {
    // 无法回退
    auto it = cannotFallbackChars.find(ch32);
    if (it != cannotFallbackChars.end()) {
        return cb(nullptr);
    }

    FontLoader& fontLoader = FontLoader::getInstance();

    // 字体元数据
    FontMeta fontMeta = {"", weight, 400, false, ""};
    FontMeta* fontMetaPtr = fontLoader.getFontMeta(family, weight);
    if (fontMetaPtr != nullptr) {
        fontMeta = *fontMetaPtr;
    }

    // 用来找字体的数组
    std::vector<std::string>& _families = getFallbackFamilies(ch32);
    std::vector<std::string> families;

    families.push_back(family);
    for (auto& family : _families) {
        families.push_back(family);
    }

    // 递归加载和查询可用字体
    _getCharFallbackFont(0, families, ch32, fontMeta, cb);
}