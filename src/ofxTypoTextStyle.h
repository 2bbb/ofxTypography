#pragma once
#include "ofColor.h"
#include <string>
#include <vector>

struct ofxTypoTextStyle {
    std::string font     = "";
    float       size     = 48.0f;
    ofColor     color    = ofColor::white;
    std::string language = "und";
    std::string script   = "";
    std::vector<std::string> features;
};
