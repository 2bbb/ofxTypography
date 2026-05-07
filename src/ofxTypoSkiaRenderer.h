#pragma once
#include "ofxHbTypes.h"
#include "ofxTypoTextStyle.h"
#include "include/core/SkCanvas.h"

class ofxTypoFontFace;

class ofxTypoSkiaRenderer {
public:
    void draw(SkCanvas* canvas,
              const ofxHbGlyphRun& run,
              ofxTypoFontFace& face,
              float x, float y,
              const ofxTypoTextStyle& style);
};
