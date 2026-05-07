#pragma once
#include "ofxHbTypes.h"
#include "ofxTypoTextStyle.h"
#include "include/core/SkCanvas.h"

class ofxTypoFontFace;
class ofxTypoTextLayout;

class ofxTypoSkiaRenderer {
public:
    void draw(SkCanvas* canvas,
              const ofxHbGlyphRun& run,
              ofxTypoFontFace& face,
              float x, float y,
              const ofxTypoTextStyle& style);

    void draw(SkCanvas* canvas,
              const ofxTypoTextLayout& layout,
              float x, float y);
};
