#include "layout.h"
#include "measure.h"

TextCharConfig::TextCharConfig() {
}

TextCharConfig::TextCharConfig(int fontSize) : fontSize(fontSize) {
}


void paragraphSplit(std::string& text, std::vector<std::string>& separators, std::vector<std::string>& paragraphs) {
    std::sort(separators.begin(), separators.end(),
              [](std::string& first, std::string& second) { return first.size() > second.size(); });

    std::string paragraph;

    int i = 0;
    while (i < text.length()) {
        bool split = false;

        for (auto& separator : separators) {
            int j = 0;
            while (i + j < text.length() && j < separator.length()) {
                char ch = text.at(i + j);
                char sp = separator.at(j);

                if (ch == sp) {
                    j++;
                } else {
                    break;
                }
            }

            if (j == separator.length()) {
                paragraphs.push_back(paragraph + separator);
                paragraph.clear();
                i = i + j;
                split = true;
                break;
            }
        }

        if (!split) {
            paragraph.push_back(text.at(i));
            i = i + 1;
        }
    }

    if (paragraph.length() > 0) {
        paragraphs.push_back(paragraph);
    }
}

void segmentBreak(std::string& text, std::vector<std::string>& segments) {
    auto uText = icu::UnicodeString(text.c_str());

    auto locale = icu::Locale("zh_CN");
    UErrorCode code = U_ZERO_ERROR;

    icu::BreakIterator* iter = icu::BreakIterator::createLineInstance(locale, code);
    if (U_FAILURE(code)) {
        std::cout << "create line iterator error: " << code << std::endl;
        delete iter;
        return;
    }

    std::string str;
    int prevIdx = 0;
    int currIdx = 0;

    iter->setText(uText);

    while ((currIdx = iter->next()) != -1) {
        auto uChar = icu::UnicodeString(uText, prevIdx, currIdx - prevIdx);

        uChar.toUTF8String(str);
        segments.push_back(str);
        str.clear();

        prevIdx = currIdx;
    }
}

void characterBreak(std::string& text, std::vector<std::string>& characters) {
    auto uText = icu::UnicodeString(text.c_str());

    auto locale = icu::Locale("zh_CN");
    UErrorCode code = U_ZERO_ERROR;

    icu::BreakIterator* iter = icu::BreakIterator::createCharacterInstance(locale, code);
    if (U_FAILURE(code)) {
        std::cout << "create char iterator error: " << code << std::endl;
        delete iter;
        return;
    }

    std::string str;
    int prevIdx = 0;
    int currIdx = 0;

    iter->setText(uText);

    while ((currIdx = iter->next()) != -1) {
        auto uChar = icu::UnicodeString(uText, prevIdx, currIdx - prevIdx);

        uChar.toUTF8String(str);
        characters.push_back(str);
        str.clear();

        prevIdx = currIdx;
    }
}

bool isEmoji(std::string& text) {
    auto uText = icu::UnicodeString(text.c_str());
    return u_getIntPropertyValue_68(uText.char32At(0), UCHAR_EMOJI);
}

bool isVisible(std::string& text) {
    const char* chars = text.c_str();
    std::vector<std::string> invisible = {"\n", "\r\n"};
    for (auto& item : invisible) {
        if (std::equal(item.begin(), item.end(), chars)) {
            return false;
        }
    }
    return true;
}

void setFontAndGlyphId(CharToken& charToken) {
    auto& charConfig = charToken.charConfig;

    FontLoader& fontLoader = FontLoader::getInstance();
    TextFallback& textFallback = TextFallback::getInstance();

    auto uText = icu::UnicodeString(charToken.text.c_str());
    SkUnichar unichar = uText.char32At(0);

    auto font = textFallback.getCharFallbackFontFromCache(unichar, charConfig->fontFamily, charConfig->fontWeight);

    if (font == nullptr) {
        auto key = FontLoader::genKey(DEFAULT_FONT_FAMILY, DEFAULT_FONT_WEIGHT);
        font = fontLoader.fontMap[key];
    }

    charToken.font = font;
    charToken.glyphId = font->unicharToGlyph(unichar);
}

void tokenize(std::string& text, std::vector<int>& charConfigIndexs,
              std::unordered_map<int, std::shared_ptr<TextCharConfig>>& charConfigMap, TextToken& textToken) {
    textToken.text = text;

    int index = 0;
    int u32Index = 0; // Array.from 使用的此种索引

    std::vector<std::string> separators = {"\n", "\r\n"};

    std::vector<std::string> paragraphs;
    paragraphSplit(text, separators, paragraphs);

    for (auto& paragraph : paragraphs) {
        ParagraphToken paragraphToken;
        paragraphToken.text = paragraph;

        std::vector<std::string> segments;
        segmentBreak(paragraph, segments);

        for (auto& segment : segments) {
            SegmentToken segmentToken;
            segmentToken.text = segment;

            std::vector<std::string> characters;
            characterBreak(segment, characters);

            for (auto& character : characters) {
                auto charToken = std::make_shared<CharToken>();
                charToken->text = character;

                charToken->index = index;
                index += 1;

                auto uCharacter = icu::UnicodeString(character.c_str());

                int charConfigIndex = charConfigIndexs[u32Index];
                charToken->charConfig = charConfigMap[charConfigIndex];

                charToken->u32Length = uCharacter.countChar32();

                charToken->u32Index = u32Index;
                u32Index += charToken->u32Length;

                charToken->isEmoji = isEmoji(character);

                charToken->visible = isVisible(character);

                if (charToken->visible && !charToken->isEmoji) {
                    setFontAndGlyphId(*charToken); // 设置字体和字形
                }

                segmentToken.children.push_back(charToken);
            }
            paragraphToken.children.push_back(segmentToken);
        }
        textToken.children.push_back(paragraphToken);
    }
}

void lineBreak(ParagraphToken& paragraphToken, TextOverallConfig& overallConfig,
               std::vector<TextLine>& paragraphLines) {
    TextLine paragraphLine;
    std::shared_ptr<CharToken> prevToken = nullptr;

    float wordWrapWidth = overallConfig.wordWrap ? overallConfig.wordWrapWidth : std::numeric_limits<float>::infinity();

    for (auto& segmentToken : paragraphToken.children) {
        std::vector<std::shared_ptr<CharToken>>& charTokens = segmentToken.children;
        float segmentPrevCharLetterSpacing = getLetterSpacing(prevToken, charTokens.at(0));

        MeasureResult segmentMeasureResult;
        measureCharTokens(charTokens, prevToken, segmentMeasureResult);

        if (paragraphLine.width + segmentPrevCharLetterSpacing + segmentMeasureResult.width > wordWrapWidth) {
            if (segmentMeasureResult.width > wordWrapWidth) {
                for (auto& charToken : charTokens) {
                    float charPrevCharLetterSpacing = getLetterSpacing(prevToken, charToken);

                    MeasureResult charMeasureResult;
                    std::vector<std::shared_ptr<CharToken>> charTokens = {charToken};
                    measureCharTokens(charTokens, prevToken, charMeasureResult);

                    if (paragraphLine.width + charPrevCharLetterSpacing + charMeasureResult.width > wordWrapWidth) {
                        // 为什么这里还需要重新测量？因为换行之后，相邻字偶间距变化导致字符宽度改变
                        MeasureResult charMeasureResult;
                        std::vector<std::shared_ptr<CharToken>> charTokens = {charToken};
                        measureCharTokens(charTokens, prevToken, charMeasureResult);

                        // addNewLine
                        if (!paragraphLine.text.empty()) {
                            paragraphLines.push_back(paragraphLine); // copied!
                        }

                        paragraphLine.text = charToken->text;
                        paragraphLine.width = charMeasureResult.width;
                        paragraphLine.height = charMeasureResult.height;
                        paragraphLine.lineHeight = charMeasureResult.lineHeight;
                        paragraphLine.baseLine = charMeasureResult.baseLine;

                        paragraphLine.tokens.clear();
                        paragraphLine.tokens.push_back(charToken);

                        prevToken = charToken;
                    } else {
                        // appendLine
                        paragraphLine.text += segmentToken.text;
                        paragraphLine.width += charPrevCharLetterSpacing + charMeasureResult.width;
                        paragraphLine.height = std::max(paragraphLine.height, charMeasureResult.height);
                        paragraphLine.lineHeight = std::max(paragraphLine.lineHeight, charMeasureResult.lineHeight);
                        paragraphLine.baseLine = std::max(paragraphLine.baseLine, charMeasureResult.baseLine);

                        paragraphLine.tokens.push_back(charToken);

                        if (prevToken != nullptr) {
                            prevToken->width += charPrevCharLetterSpacing;
                        }

                        prevToken = charToken;
                    }
                }

            } else {
                // addNewLine
                if (!paragraphLine.text.empty()) {
                    paragraphLines.push_back(paragraphLine); // copied!
                }

                paragraphLine.text = segmentToken.text;
                paragraphLine.width = segmentMeasureResult.width;
                paragraphLine.height = segmentMeasureResult.height;
                paragraphLine.lineHeight = segmentMeasureResult.lineHeight;
                paragraphLine.baseLine = segmentMeasureResult.baseLine;

                paragraphLine.tokens.clear();
                for (auto& token : charTokens) {
                    paragraphLine.tokens.push_back(token);
                }

                prevToken = charTokens.at(charTokens.size() - 1);
            }
        } else {
            // appendLine
            paragraphLine.text += segmentToken.text;
            paragraphLine.width += segmentPrevCharLetterSpacing + segmentMeasureResult.width;
            paragraphLine.height = std::max(paragraphLine.height, segmentMeasureResult.height);
            paragraphLine.lineHeight = std::max(paragraphLine.lineHeight, segmentMeasureResult.lineHeight);
            paragraphLine.baseLine = std::max(paragraphLine.baseLine, segmentMeasureResult.baseLine);

            for (auto& token : charTokens) {
                paragraphLine.tokens.push_back(token);
            }

            if (prevToken != nullptr) {
                prevToken->width += segmentPrevCharLetterSpacing;
            }

            prevToken = charTokens.at(charTokens.size() - 1);
        }
    }

    paragraphLines.push_back(paragraphLine);
}


float getLeftOffset(float width, float lineWidth, HorizontalAlignType horizontalAlign) {
    float leftOffset = 0.0;

    if (horizontalAlign == HorizontalAlignType::Center) {
        leftOffset = (width - lineWidth) / 2;
    }

    if (horizontalAlign == HorizontalAlignType::Right) {
        leftOffset = width - lineWidth;
    }

    return leftOffset;
}


float getTopOffset(float height, float textHeight, VerticalAlignType verticalAlign) {
    float topOffset = 0.0;

    if (verticalAlign == VerticalAlignType::Mid) {
        topOffset = (height - textHeight) / 2;
    }

    if (verticalAlign == VerticalAlignType::Bottom) {
        topOffset = height - textHeight;
    }

    return topOffset;
}

void layout() {
    TextConfig textConfig;
    textConfig.text = "Hello World!\n你好，世界！";
    textConfig.charConfigIndexs = {
        0, 0, 0, 0, 0, // hello
        4,             //
        1, 1, 1, 1, 1, // World
        4,             // !
        4,             // \n
        2, 2,          // 你好
        4,             // ，
        3, 3,          //世界
        4,             // ！
    };
    textConfig.charConfigMap[0] = std::make_shared<TextCharConfig>(20);
    textConfig.charConfigMap[1] = std::make_shared<TextCharConfig>(40);
    textConfig.charConfigMap[2] = std::make_shared<TextCharConfig>(60);
    textConfig.charConfigMap[3] = std::make_shared<TextCharConfig>(80);
    textConfig.charConfigMap[4] = std::make_shared<TextCharConfig>(50);

    TextToken textToken;
    tokenize(textConfig.text, textConfig.charConfigIndexs, textConfig.charConfigMap, textToken);

    std::vector<std::vector<TextLine>> textLines;

    for (auto& paragraphToken : textToken.children) {
        std::vector<TextLine> paragraphLines;
        lineBreak(paragraphToken, textConfig.overallConfig, paragraphLines);
        textLines.push_back(paragraphLines);
    }

    TextBounds layoutBounds;
    TextBounds dirtyBounds;

    float layoutWidth = 0.0;
    float layoutHeight = 0.0;

    for (auto& paragraphLines : textLines) {
        for (auto& paragraphLine : paragraphLines) {
            layoutWidth = std::max(layoutWidth, paragraphLine.width);
            layoutHeight += paragraphLine.lineHeight;
        }
    }

    TextOverallConfig& overallConfig = textConfig.overallConfig;

    layoutHeight +=
        std::max(static_cast<int>(textLines.size()) - 1, 0) * overallConfig.paragraphSpacing; // FIXME: 为啥还需要装换？

    float top = getTopOffset(overallConfig.height || layoutHeight, layoutHeight, overallConfig.verticalAlign);

    layoutBounds.minY = top;
    layoutBounds.maxY = layoutBounds.minY + layoutHeight;

    std::vector<std::shared_ptr<CharToken>> visibleTokens;

    for (auto& paragraphLines : textLines) {
        for (auto& paragraphLine : paragraphLines) {
            float left =
                getLeftOffset(overallConfig.width || layoutWidth, paragraphLine.width, overallConfig.horizontalAlign);

            paragraphLine.x = left;
            paragraphLine.y = top - (paragraphLine.height - paragraphLine.lineHeight) / 2;

            layoutBounds.minX = std::min(layoutBounds.minX, paragraphLine.x);
            layoutBounds.maxX = std::max(layoutBounds.maxX, paragraphLine.x + paragraphLine.width);

            // TODO: 横向暂时和LayoutBounds没有任何区别！
            dirtyBounds.minX = std::min(dirtyBounds.minX, paragraphLine.x);
            dirtyBounds.maxX = std::max(dirtyBounds.maxX, paragraphLine.x + paragraphLine.width);

            float renderHeight = std::max(paragraphLine.height, paragraphLine.lineHeight);
            float renderOffset = (renderHeight - paragraphLine.height) / 2;

            dirtyBounds.minY = std::min(dirtyBounds.minY, paragraphLine.y - renderOffset);
            dirtyBounds.maxY = std::max(dirtyBounds.maxY, paragraphLine.y - renderOffset + renderHeight);

            for (auto& token : paragraphLine.tokens) {
                if (token->isEmoji) {
                    token->x = left;
                    token->y = paragraphLine.y + paragraphLine.baseLine - token->baseLine;
                } else {
                    token->x = left;
                    token->y = paragraphLine.y + paragraphLine.baseLine;
                }

                left += token->width;

                if (token->visible) {
                    visibleTokens.push_back(token);
                }
            }
            top += paragraphLine.lineHeight;
        }
        top += overallConfig.paragraphSpacing;
    }

    TextFallback& textFallback = TextFallback::getInstance();
    // std::vector<std::shared_ptr<CharToken>> fallbackTokens;

    for (auto& charToken : visibleTokens) {
        auto uText = icu::UnicodeString(charToken->text.c_str());
        SkUnichar unichar = uText.char32At(0);

        auto& charConfig = charToken->charConfig;

        bool needFallback =
            textFallback.checkCharIfNeedFallback(unichar, charConfig->fontFamily, charConfig->fontWeight);

        // fallbackTokens.push_back(charToken);
        if (needFallback) {
            textFallback.getCharFallbackFont(
                unichar, charConfig->fontFamily, charConfig->fontWeight,
                [charToken](std::shared_ptr<const SkFont> font) { setFontAndGlyphId(*charToken); });
        }
    }


    Renderer& renderer = Renderer::getInstance();
    renderer.drawFn = [&renderer, visibleTokens](int err) {
        for (auto& token : visibleTokens) {
            SkPath path;
            token->font->getPath(token->glyphId, &path);
            float scale = token->charConfig->fontSize / FONT_SIZE;
            path.transform(SkMatrix().setScale(scale, scale));
            path.transform(SkMatrix().setTranslate(token->x + 400, token->y + 400));
            renderer.paint.setColor(SkColorSetARGB(0xFF, 0x00, 0x00, 0x00));
            renderer.canvas->drawPath(path, renderer.paint);

            renderer.paint.setColor(SkColorSetARGB(0x22, 0x00, 0xFF, 0x00));
            renderer.canvas->drawRect(
                SkRect::MakeXYWH(token->x + 400, token->y - token->baseLine + 400, token->width, token->height),
                renderer.paint);
        }
    };
}