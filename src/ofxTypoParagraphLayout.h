#pragma once
#include "ofRectangle.h"
#include "ofxTypoTextLayout.h"
#include <vector>

class ofxTypoParagraphLayout {
public:
    std::vector<ofxTypoTextLayout>&       lines()       { return lines_; }
    const std::vector<ofxTypoTextLayout>& lines() const { return lines_; }

    ofRectangle getBounds()     const { return bounds_; }
    float       getLineStride() const { return lineStride_; }
    bool        isVertical()    const { return vertical_; }

    void addLine(ofxTypoTextLayout line)  { lines_.push_back(std::move(line)); }
    void setBounds(const ofRectangle& b)  { bounds_ = b; }
    void setLineStride(float s)           { lineStride_ = s; }
    void setVertical(bool v)              { vertical_ = v; }

private:
    std::vector<ofxTypoTextLayout> lines_;
    ofRectangle bounds_;
    float       lineStride_ = 0.0f;
    bool        vertical_   = false;
};
