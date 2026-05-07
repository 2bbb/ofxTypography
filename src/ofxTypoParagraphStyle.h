#pragma once

enum class ofxTypoAlign { Left, Center, Right };

struct ofxTypoParagraphStyle {
    float        width      = 0.0f;  // 0 = no wrap (horizontal) / column width (vertical)
    float        lineHeight = 1.4f;  // em multiplier

    // Horizontal layout
    ofxTypoAlign align      = ofxTypoAlign::Left;

    // Phase 9: vertical writing (縦書き)
    bool         vertical   = false;   // true = top-to-bottom columns
    float        height     = 0.0f;    // column height limit; 0 = no break
};
