#pragma once

#include "FontLoader.h"
#include "layout.h"
#include <algorithm>


float getLetterSpacing(std::shared_ptr<CharToken> prevToken, std::shared_ptr<CharToken> nextToken);

struct MeasureResult {
    float width;
    float height;
    float lineHeight;
    float baseLine;
};

void measureCharTokens(std::vector<std::shared_ptr<CharToken>>& charTokens, std::shared_ptr<CharToken> prevToken,
                       MeasureResult& measureResult);