#pragma once
#include "ofRectangle.h"
#include "ofxTypoGlyph.h"
#include "ofxTypoTextStyle.h"
#include "ofxTypoFontFace.h"
#include <memory>
#include <vector>

class ofxTypoTextLayout {
public:
    std::vector<ofxTypoGlyph>&       glyphs()       { return glyphs_; }
    const std::vector<ofxTypoGlyph>& glyphs() const { return glyphs_; }

    ofRectangle getBounds() const { return bounds_; }

    ofxTypoFontFace*         getFace()  const { return face_.get(); }
    const ofxTypoTextStyle&  getStyle() const { return style_; }

    // internal use by ofxTypography
    void init(std::vector<ofxTypoGlyph> glyphs,
              std::shared_ptr<ofxTypoFontFace> face,
              const ofxTypoTextStyle& style,
              const ofRectangle& bounds);

private:
    std::vector<ofxTypoGlyph>        glyphs_;
    std::shared_ptr<ofxTypoFontFace> face_;
    ofxTypoTextStyle                 style_;
    ofRectangle                      bounds_;
};
