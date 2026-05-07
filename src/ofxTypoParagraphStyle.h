#pragma once

enum class ofxTypoAlign { Left, Center, Right };

struct ofxTypoParagraphStyle {
    float        width      = 0.0f;  // 0 = no wrap
    float        lineHeight = 1.4f;  // em multiplier
    ofxTypoAlign align      = ofxTypoAlign::Left;
};
