#pragma once
#include "glm/glm.hpp"
#include <cstdint>

struct ofxTypoGlyph {
    uint32_t  glyphId = 0;
    glm::vec2 pos     = {0, 0};  // relative to layout origin — modifiable for animation
    glm::vec2 advance = {0, 0};
    uint32_t  cluster = 0;
};
