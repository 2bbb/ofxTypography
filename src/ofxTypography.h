#pragma once
#include "ofxSkiaSurface.h"
#include "ofxTypoFontFace.h"
#include "ofxTypoTextStyle.h"
#include "ofxTypoTextLayout.h"
#include "ofxTypoParagraphStyle.h"
#include "ofxTypoParagraphLayout.h"
#include "ofxTypoPdfExporter.h"
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class ofxTypography {
public:
    // Single font
    void loadFont(const std::string& name, const std::filesystem::path& path);

    // Phase 6: ordered fallback collection
    void loadFontCollection(const std::string& name,
                             const std::vector<std::filesystem::path>& paths);

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

    // Phase 8: PDF export — draw layout onto a PDF canvas
    void drawToPdf(ofxTypoPdfExporter& pdf, ofxTypoTextLayout& layout, float x, float y);
    void drawToPdf(ofxTypoPdfExporter& pdf, ofxTypoParagraphLayout& layout, float x, float y);

private:
    std::unordered_map<std::string, std::shared_ptr<ofxTypoFontFace>>               fonts_;
    std::unordered_map<std::string, std::vector<std::shared_ptr<ofxTypoFontFace>>>  collections_;
    ofxSkiaSurface surface_;

    // Returns face list for a name (collection first, then single font)
    std::vector<std::shared_ptr<ofxTypoFontFace>> resolveFaces(const std::string& name) const;

    SkCanvas* ensureSurface();
};
