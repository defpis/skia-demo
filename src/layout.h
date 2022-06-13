#pragma once

#include "FontLoader.h"
#include "fallback.h"
#include "libskia.h"
#include "renderer.h"
#include "rstring.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <vector>

void layout();

enum class SizeModeType {
    AutoWidth,
    AutoHeight,
    Fixed,
};

enum class HorizontalAlignType {
    Left,
    Center,
    Right,
};

enum class VerticalAlignType {
    Top,
    Mid,
    Bottom,
};

enum class TextCaseType {
    Origin,
    Title,
    Upper,
    Lower,
};

struct TextCharConfig {
    float fontSize = 10.0;

    std::string fontFamily = "Inter";
    std::string fontWeight = "Regular";

    float lineHeight = 0.0;
    float letterSpacing = 0.0;

    TextCaseType textCase = TextCaseType::Origin;

    TextCharConfig();
    TextCharConfig(int fontSize);
};

struct TextOverallConfig {
    float width = 0.0;
    float height = 0.0;

    bool wordWrap = true;
    float wordWrapWidth = 200;

    float paragraphSpacing = 0.0;

    HorizontalAlignType horizontalAlign = HorizontalAlignType::Left;
    VerticalAlignType verticalAlign = VerticalAlignType::Mid;
};

struct TextConfig {
    std::string text;

    std::vector<int> charConfigIndexs;

    std::unordered_map<int, std::shared_ptr<TextCharConfig>> charConfigMap;

    TextOverallConfig overallConfig;
};

struct CharToken {
    std::string text;

    int index = 0.0;

    // 兼容js侧Array.from和[...text]获取的长度与索引
    int u32Index = 0;
    int u32Length = 0;

    bool isEmoji = false;
    bool visible = true;

    float x = 0.0;
    float y = 0.0;
    float width = 0.0;
    float height = 0.0;
    float lineHeight = 0.0;
    float baseLine = 0.0;

    std::shared_ptr<const TextCharConfig> charConfig;

    std::shared_ptr<const SkFont> font = nullptr;
    SkGlyphID glyphId = 0;
};

struct SegmentToken {
    std::string text;
    std::vector<std::shared_ptr<CharToken>> children;
};

struct ParagraphToken {
    std::string text;
    std::vector<SegmentToken> children;
};

struct TextToken {
    std::string text;
    std::vector<ParagraphToken> children;
};

struct TextLine {
    std::string text;
    std::vector<std::shared_ptr<CharToken>> tokens;

    float x = 0.0;
    float y = 0.0;
    float width = 0.0;
    float height = 0.0;
    float lineHeight = 0.0;
    float baseLine = 0.0;
};

struct TextBounds {
    float minX = -std::numeric_limits<float>::infinity();
    float minY = -std::numeric_limits<float>::infinity();
    float maxX = std::numeric_limits<float>::infinity();
    float maxY = std::numeric_limits<float>::infinity();
};
