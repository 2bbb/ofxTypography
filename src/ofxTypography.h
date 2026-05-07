#pragma once
#include "ofxSkiaSurface.h"
#include "ofxTypoFontFace.h"
#include "ofxTypoTextStyle.h"
#include "ofxTypoTextLayout.h"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

class ofxTypography {
public:
    void loadFont(const std::string& name, const std::filesystem::path& path);

    // Phase 3: immediate draw
    void draw(const std::string& utf8, float x, float y, const ofxTypoTextStyle& style);

    // Phase 4: layout → manipulate → draw
    ofxTypoTextLayout layout(const std::string& utf8, const ofxTypoTextStyle& style);
    void draw(ofxTypoTextLayout& layout, float x, float y);

private:
    std::unordered_map<std::string, std::shared_ptr<ofxTypoFontFace>> fonts_;
    ofxSkiaSurface surface_;

    SkCanvas* ensureSurface();
};
