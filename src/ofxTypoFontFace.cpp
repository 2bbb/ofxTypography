#include "ofxTypoFontFace.h"
#include "include/core/SkData.h"
#include "include/core/SkFontMgr.h"
#include "include/ports/SkFontMgr_data.h"
#include "include/core/SkString.h"
#include "ofLog.h"

bool ofxTypoFontFace::load(const std::filesystem::path& path) {
    hbFace_ = std::make_shared<ofxHbFace>();
    if (!hbFace_->load(path)) {
        ofLogError("ofxTypoFontFace") << "Failed to load font: " << path;
        return false;
    }

    const auto& bytes = hbFace_->getFontData();
    sk_sp<SkData> skData = SkData::MakeWithCopy(bytes.data(), bytes.size());
    sk_sp<SkFontMgr> mgr = SkFontMgr_New_Custom_Data({&skData, 1});
    skTypeface_ = mgr->makeFromData(skData, 0);

    if (!skTypeface_) {
        ofLogError("ofxTypoFontFace") << "SkTypeface creation failed for: " << path;
        return false;
    }
    return true;
}

ofxHbFont& ofxTypoFontFace::getHbFont(float sizePixels) {
    int key = static_cast<int>(sizePixels * 10.0f + 0.5f);
    auto it = hbFontCache_.find(key);
    if (it == hbFontCache_.end()) {
        auto font = std::make_unique<ofxHbFont>();
        font->create(*hbFace_, sizePixels);
        it = hbFontCache_.emplace(key, std::move(font)).first;
    }
    return *it->second;
}

bool ofxTypoFontFace::supports(uint32_t codepoint) const {
    if (!skTypeface_) return false;
    return skTypeface_->unicharToGlyph(static_cast<SkUnichar>(codepoint)) != 0;
}

std::string ofxTypoFontFace::getFamilyName() const {
    if (!skTypeface_) return "";
    SkString name;
    skTypeface_->getFamilyName(&name);
    return std::string(name.c_str());
}
