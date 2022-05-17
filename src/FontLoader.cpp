#include "FontLoader.h"


FontLoader::FontLoader() {
}

FontLoader::~FontLoader() {
    fontMetaMap.clear();
    fontMap.clear();
}

std::string FontLoader::genKey(const std::string& fontFamily, const std::string& fontWeight) {
    return fontFamily + '-' + fontWeight;
}

// TODO: 是否可以预先生成对应的TypeFace然后进行匹配？

void FontLoader::loadFontMeta(std::function<void(int)> cb) {
    nlohmann::json data;
    callJSFunc("loadFontMeta", data.dump().c_str(), [this, cb](int err, uintptr_t ptr, size_t size) {
        if (err) {
            std::cout << "loadFontMeta error!" << std::endl;
            return cb(err);
        }

        auto buffer = reinterpret_cast<uint8_t*>(ptr);
        nlohmann::json jsonResp = nlohmann::json::parse(buffer, buffer + size);

        FontMeta fontMeta;
        for (auto& i : jsonResp.items()) {
            std::string family = i.key();
            nlohmann::json& styles = i.value();

            this->fontMetaMap[family] = std::unordered_map<std::string, FontMeta>();

            for (auto& j : styles.items()) {
                std::string style = j.key();
                nlohmann::json& item = j.value();

                fontMeta.family = item["family"];
                fontMeta.style = item["style"];
                fontMeta.weight = item["weight"];
                fontMeta.italic = item["italic"];
                fontMeta.path = item["path"];

                this->fontMetaMap[family][style] = fontMeta;
            }
        }

        cb(err);
    });
}

void FontLoader::loadFont(const std::string& fontFamily, const std::string& fontWeight, std::function<void(int)> cb) {
    // TODO: 存在值为空的问题
    std::string& path = this->fontMetaMap[fontFamily][fontWeight].path;
    nlohmann::json data = {{"path", path}};
    auto key = FontLoader::genKey(fontFamily, fontWeight);

    callJSFunc("loadFont", data.dump().c_str(), [this, key, cb](int err, uintptr_t ptr, size_t size) {
        if (err) {
            std::cout << "loadFont error!" << std::endl;
            return cb(err);
        }

        auto buffer = reinterpret_cast<uint8_t*>(ptr);
        auto data = SkData::MakeFromMalloc(buffer, size);
        auto typeFace = SkFontMgr::RefDefault()->makeFromData(data);
        auto font = std::make_shared<SkFont>();
        font->setTypeface(typeFace);
        font->setSize(FONT_SIZE);
        this->fontMap[key] = font;
        std::cout << key << " loaded!" << std::endl;

        cb(err);
    });
}

FontInfo* FontLoader::getFontInfo(const std::string& fontFamily) {
    auto it = fontMetaMap.find(fontFamily);
    if (it == fontMetaMap.end()) {
        return nullptr;
    }
    return &(it->second);
}

FontMeta* FontLoader::getFontMeta(const std::string& fontFamily, const std::string& fontWeight) {
    FontInfo* fontInfo = getFontInfo(fontFamily);
    if (fontInfo == nullptr) {
        return nullptr;
    }
    auto it = (*fontInfo).find(fontWeight);
    if (it == (*fontInfo).end()) {
        return nullptr;
    }
    return &(it->second);
}


std::shared_ptr<SkFont> FontLoader::getFont(const std::string& fontFamily, const std::string& fontWeight) {
    auto key = FontLoader::genKey(fontFamily, fontWeight);
    auto it = fontMap.find(key);
    if (it == fontMap.end()) {
        return nullptr;
    }
    return (it->second);
}
