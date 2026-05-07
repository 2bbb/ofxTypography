#include "ofxTypoTextLayout.h"

void ofxTypoTextLayout::init(std::vector<ofxTypoGlyph> glyphs,
                              std::shared_ptr<ofxTypoFontFace> face,
                              const ofxTypoTextStyle& style,
                              const ofRectangle& bounds) {
    glyphs_ = std::move(glyphs);
    face_   = std::move(face);
    style_  = style;
    bounds_ = bounds;
}
