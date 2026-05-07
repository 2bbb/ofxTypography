#include "ofxTypoTextLayout.h"

void ofxTypoTextLayout::init(std::vector<ofxTypoGlyph> glyphs,
                              std::vector<std::shared_ptr<ofxTypoFontFace>> faces,
                              const ofxTypoTextStyle& style,
                              const ofRectangle& bounds) {
    glyphs_ = std::move(glyphs);
    faces_  = std::move(faces);
    style_  = style;
    bounds_ = bounds;
}

void ofxTypoTextLayout::init(std::vector<ofxTypoGlyph> glyphs,
                              std::shared_ptr<ofxTypoFontFace> face,
                              const ofxTypoTextStyle& style,
                              const ofRectangle& bounds) {
    init(std::move(glyphs),
         std::vector<std::shared_ptr<ofxTypoFontFace>>{ std::move(face) },
         style, bounds);
}
