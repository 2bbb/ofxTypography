// unity includes
#include "ofxTypoFontFace.cpp"
#include "ofxTypoSkiaRenderer.cpp"

#include "ofxTypography.h"
#include "ofxTypoSkiaRenderer.h"
#include "ofxHbShaper.h"
#include "ofMain.h"
#include "ofLog.h"

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

    int w = ofGetWidth();
    int h = ofGetHeight();
    if (!surface_.isAllocated() || surface_.getWidth() != w || surface_.getHeight() != h) {
        surface_.allocate(w, h);
    }

    surface_.clear(ofColor(0, 0, 0, 0));

    ofxHbShapeOptions opts;
    opts.language  = style.language;
    opts.script    = style.script;
    opts.features  = style.features;

    ofxHbFont& hbFont = face.getHbFont(style.size);
    ofxHbShaper shaper;
    ofxHbGlyphRun run = shaper.shape(utf8, hbFont, opts);

    SkCanvas* canvas = surface_.getCanvas();
    if (!canvas) {
        ofLogError("ofxTypography::draw") << "no canvas";
        return;
    }

    ofxTypoSkiaRenderer renderer;
    renderer.draw(canvas, run, face, x, y, style);

    surface_.updateTexture();
    surface_.draw(0, 0);
}
