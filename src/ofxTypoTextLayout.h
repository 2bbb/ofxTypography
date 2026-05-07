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

    const std::vector<std::shared_ptr<ofxTypoFontFace>>& getFaces() const { return faces_; }
    ofxTypoFontFace* getFace(int index = 0) const {
        return (index < (int)faces_.size()) ? faces_[index].get() : nullptr;
    }

    ofRectangle             getBounds() const { return bounds_; }
    const ofxTypoTextStyle& getStyle()  const { return style_; }

    // multi-face init (Phase 6+)
    void init(std::vector<ofxTypoGlyph> glyphs,
              std::vector<std::shared_ptr<ofxTypoFontFace>> faces,
              const ofxTypoTextStyle& style,
              const ofRectangle& bounds);

    // single-face convenience (Phase 4 compat)
    void init(std::vector<ofxTypoGlyph> glyphs,
              std::shared_ptr<ofxTypoFontFace> face,
              const ofxTypoTextStyle& style,
              const ofRectangle& bounds);

private:
    std::vector<ofxTypoGlyph>                    glyphs_;
    std::vector<std::shared_ptr<ofxTypoFontFace>> faces_;
    ofxTypoTextStyle                              style_;
    ofRectangle                                   bounds_;
};
