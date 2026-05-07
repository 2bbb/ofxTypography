// unity includes
#include "ofxTypoFontFace.cpp"
#include "ofxTypoSkiaRenderer.cpp"
#include "ofxTypoTextLayout.cpp"

#include "ofxTypography.h"
#include "ofxTypoSkiaRenderer.h"
#include "ofxHbShaper.h"
#include "ofMain.h"
#include "ofLog.h"

SkCanvas* ofxTypography::ensureSurface() {
    int w = ofGetWidth();
    int h = ofGetHeight();
    if (!surface_.isAllocated() || surface_.getWidth() != w || surface_.getHeight() != h) {
        surface_.allocate(w, h);
    }
    surface_.clear(ofColor(0, 0, 0, 0));
    return surface_.getCanvas();
}

void ofxTypography::loadFont(const std::string& name, const std::filesystem::path& path) {
    auto face = std::make_shared<ofxTypoFontFace>();
    if (!face->load(path)) {
        ofLogError("ofxTypography::loadFont") << "Failed to load: " << path;
        return;
    }
    fonts_[name] = std::move(face);
    ofLogNotice("ofxTypography::loadFont") << "Loaded font '" << name << "' from " << path;
}

void ofxTypography::draw(const std::string& utf8, float x, float y,
                          const ofxTypoTextStyle& style) {
    auto it = fonts_.find(style.font);
    if (it == fonts_.end()) {
        ofLogError("ofxTypography::draw") << "Font not found: '" << style.font << "'";
        return;
    }
    auto& face = *it->second;

    SkCanvas* canvas = ensureSurface();
    if (!canvas) return;

    ofxHbShapeOptions opts;
    opts.language = style.language;
    opts.script   = style.script;
    opts.features = style.features;

    ofxHbShaper shaper;
    ofxHbGlyphRun run = shaper.shape(utf8, face.getHbFont(style.size), opts);

    ofxTypoSkiaRenderer renderer;
    renderer.draw(canvas, run, face, x, y, style);

    surface_.updateTexture();
    surface_.draw(0, 0);
}

ofxTypoTextLayout ofxTypography::layout(const std::string& utf8,
                                         const ofxTypoTextStyle& style) {
    auto it = fonts_.find(style.font);
    if (it == fonts_.end()) {
        ofLogError("ofxTypography::layout") << "Font not found: '" << style.font << "'";
        return {};
    }
    auto& facePtr = it->second;

    ofxHbShapeOptions opts;
    opts.language = style.language;
    opts.script   = style.script;
    opts.features = style.features;

    ofxHbShaper shaper;
    ofxHbGlyphRun run = shaper.shape(utf8, facePtr->getHbFont(style.size), opts);

    std::vector<ofxTypoGlyph> glyphs;
    glyphs.reserve(run.glyphs.size());

    float cx = 0.0f;
    float minX = 0.0f, maxX = 0.0f;
    for (const auto& g : run.glyphs) {
        ofxTypoGlyph tg;
        tg.glyphId = g.glyphId;
        tg.pos     = {cx + g.offset.x, -g.offset.y};
        tg.advance = {g.advance.x, g.advance.y};
        tg.cluster = g.cluster;
        glyphs.push_back(tg);
        cx += g.advance.x;
    }
    maxX = cx;

    ofRectangle bounds(minX, -style.size, maxX - minX, style.size);

    ofxTypoTextLayout layout;
    layout.init(std::move(glyphs), facePtr, style, bounds);
    return layout;
}

void ofxTypography::draw(ofxTypoTextLayout& layout, float x, float y) {
    if (layout.glyphs().empty()) return;

    SkCanvas* canvas = ensureSurface();
    if (!canvas) return;

    ofxTypoSkiaRenderer renderer;
    renderer.draw(canvas, layout, x, y);

    surface_.updateTexture();
    surface_.draw(0, 0);
}
