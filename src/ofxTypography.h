#pragma once
#include "ofxSkiaSurface.h"
#include "ofxTypoFontFace.h"
#include "ofxTypoTextStyle.h"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

class ofxTypography {
public:
    void loadFont(const std::string& name, const std::filesystem::path& path);
    void draw(const std::string& utf8, float x, float y, const ofxTypoTextStyle& style);

private:
    std::unordered_map<std::string, std::shared_ptr<ofxTypoFontFace>> fonts_;
    ofxSkiaSurface surface_;
};
