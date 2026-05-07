#pragma once
#include "ofxSkiaSurface.h"
#include "ofxTypoFontFace.h"
#include "ofxTypoTextStyle.h"
#include "ofxTypoTextLayout.h"
#include "ofxTypoParagraphStyle.h"
#include "ofxTypoParagraphLayout.h"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

class ofxTypography {
public:
    void loadFont(const std::string& name, const std::filesystem::path& path);

    // Phase 3: immediate draw
    void draw(const std::string& utf8, float x, float y, const ofxTypoTextStyle& style);

    // Phase 4: single-line layout → manipulate → draw
    ofxTypoTextLayout layout(const std::string& utf8, const ofxTypoTextStyle& style);
    void draw(ofxTypoTextLayout& layout, float x, float y);

    // Phase 5: paragraph layout with wrapping and alignment
    ofxTypoParagraphLayout layoutParagraph(const std::string& utf8,
                                            const ofxTypoTextStyle& style,
                                            const ofxTypoParagraphStyle& para = {});
    void draw(ofxTypoParagraphLayout& layout, float x, float y);

private:
    std::unordered_map<std::string, std::shared_ptr<ofxTypoFontFace>> fonts_;
    ofxSkiaSurface surface_;

    SkCanvas* ensureSurface();
};
