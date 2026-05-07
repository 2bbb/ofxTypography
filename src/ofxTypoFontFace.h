#pragma once
#include "ofxHarfBuzz.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkRefCnt.h"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

class ofxTypoFontFace {
public:
    bool load(const std::filesystem::path& path);

    ofxHbFont&         getHbFont(float sizePixels);
    sk_sp<SkTypeface>  getSkTypeface() const { return skTypeface_; }

    bool        supports(uint32_t codepoint) const;
    std::string getFamilyName() const;
    bool        isLoaded() const { return skTypeface_ != nullptr; }

private:
    std::shared_ptr<ofxHbFace> hbFace_;
    sk_sp<SkTypeface>           skTypeface_;
    std::unordered_map<int, std::unique_ptr<ofxHbFont>> hbFontCache_;
};
