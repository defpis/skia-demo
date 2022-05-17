#include "measure.h"


float getLetterSpacing(std::shared_ptr<CharToken> currToken, std::shared_ptr<CharToken> nextToken) {
    float letterSpacing = 0.0;

    if (currToken != nullptr && nextToken != nullptr && currToken->visible) {
        letterSpacing = currToken->charConfig->letterSpacing;
    }

    return letterSpacing;
}

float getKerning(std::shared_ptr<CharToken> currToken, std::shared_ptr<CharToken> nextToken) {
    float kerning = 0.0;

    if (currToken != nullptr && nextToken != nullptr && !currToken->isEmoji && nextToken->isEmoji &&
        currToken->font == nextToken->font) {
        const int size = 2;
        SkGlyphID glyphs[size] = {currToken->glyphId, nextToken->glyphId};
        int adjustments[size - 1] = {0};
        SkTypeface* typeFace = currToken->font->getTypeface();
        typeFace->getKerningPairAdjustments(glyphs, 2,
                                            adjustments); // TODO: 始终返回`0`，不确定如何才能生效
        kerning = adjustments[0] / typeFace->getUnitsPerEm();
    }

    return kerning;
}

float getAutoLineHeight(std::shared_ptr<CharToken> currToken) {
    SkFontMetrics metrics;
    currToken->font->getMetrics(&metrics);
    float autoLineHeight =
        (metrics.fDescent - metrics.fAscent + metrics.fLeading) * currToken->charConfig->fontSize / FONT_SIZE;
    return autoLineHeight;
}

float getLineHeight(std::shared_ptr<CharToken> currToken) {
    // TODO: 暂时返回自动高度
    float lineHeight = getAutoLineHeight(currToken);
    return lineHeight;
}

float getBaseLine(std::shared_ptr<CharToken> currToken) {
    SkFontMetrics metrics;
    currToken->font->getMetrics(&metrics);
    float baseLine = (-metrics.fAscent + metrics.fLeading / 2) * currToken->charConfig->fontSize / FONT_SIZE;
    return baseLine;
}

float getAdvanceWidth(std::shared_ptr<CharToken> currToken) {
    const SkGlyphID glyphs[1] = {currToken->glyphId};
    float widths[1] = {0.0};
    currToken->font->getWidths(glyphs, 1, widths);
    float advanceWidth = widths[0] * currToken->charConfig->fontSize / FONT_SIZE;
    return advanceWidth;
}

void measureCharTokens(std::vector<std::shared_ptr<CharToken>>& charTokens, std::shared_ptr<CharToken> prevToken,
                       MeasureResult& measureResult) {
    float width = 0.0;
    float height = 0.0;
    float lineHeight = 0.0;
    float baseLine = 0.0;

    std::shared_ptr<CharToken> firstToken = prevToken;
    width += getKerning(prevToken, firstToken);

    for (int i = 0; i < charTokens.size(); i++) {
        float glyphWidth = 0.0;
        float glyphHeight = 0.0;
        float glyphLineHeight = 0.0;
        float glyphBaseLine = 0.0;

        std::shared_ptr<CharToken> currToken = charTokens.at(i);

        if (currToken->isEmoji) {
            float fontSize = currToken->charConfig->fontSize;

            glyphWidth = fontSize;
            glyphHeight = fontSize;
            glyphLineHeight = fontSize;
            glyphBaseLine = fontSize * 0.92; // TODO: 通过基线调整Emoji的上下位置
        } else {
            if (currToken->visible) {
                std::shared_ptr<CharToken> nextToken = i + 1 < charTokens.size() ? charTokens.at(i + 1) : nullptr;

                float kerning = getKerning(currToken, nextToken);
                float advanceWidth = getAdvanceWidth(currToken);
                float letterSpacing = getLetterSpacing(currToken, nextToken);
                glyphWidth = kerning + advanceWidth + letterSpacing;

                // TODO: \n等特殊符号也需要计算以下数值
                glyphHeight = getAutoLineHeight(currToken);
                glyphLineHeight = getLineHeight(currToken);
                glyphBaseLine = getBaseLine(currToken);
            }
        }

        width += glyphWidth;
        height = std::max(height, glyphHeight);
        lineHeight = std::max(lineHeight, glyphLineHeight);
        baseLine = std::max(baseLine, glyphBaseLine);

        currToken->width = glyphWidth;
        currToken->height = glyphHeight;
        currToken->lineHeight = glyphLineHeight;
        currToken->baseLine = glyphBaseLine;
    }

    measureResult.width = width;
    measureResult.height = height;
    measureResult.lineHeight = lineHeight;
    measureResult.baseLine = baseLine;
}