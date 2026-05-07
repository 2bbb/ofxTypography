#include "ofxTypoSkiaRenderer.h"
#include "ofxTypoFontFace.h"
#include "ofxTypoTextLayout.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPoint.h"
#include <vector>

static SkPaint makePaint(const ofxTypoTextStyle& style) {
    const ofColor& c = style.color;
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SkColorSetARGB(c.a, c.r, c.g, c.b));
    return paint;
}

static SkFont makeFont(ofxTypoFontFace& face, float size) {
    SkFont font(face.getSkTypeface(), size);
    font.setEdging(SkFont::Edging::kSubpixelAntiAlias);
    font.setSubpixel(true);
    return font;
}

void ofxTypoSkiaRenderer::draw(SkCanvas* canvas,
                                const ofxHbGlyphRun& run,
                                ofxTypoFontFace& face,
                                float x, float y,
                                const ofxTypoTextStyle& style) {
    if (!canvas || run.glyphs.empty()) return;

    int n = (int)run.glyphs.size();
    std::vector<SkGlyphID> glyphIds(n);
    std::vector<SkPoint>   positions(n);

    float cx = 0.0f, cy = 0.0f;
    for (int i = 0; i < n; ++i) {
        const auto& g = run.glyphs[i];
        glyphIds[i]  = static_cast<SkGlyphID>(g.glyphId);
        positions[i] = SkPoint::Make(cx + g.offset.x, cy - g.offset.y);
        cx += g.advance.x;
        cy -= g.advance.y;
    }

    canvas->drawGlyphs(n, glyphIds.data(), positions.data(),
                       SkPoint::Make(x, y), makeFont(face, style.size), makePaint(style));
}

void ofxTypoSkiaRenderer::draw(SkCanvas* canvas,
                                const ofxTypoTextLayout& layout,
                                float x, float y) {
    if (!canvas) return;
    auto* face = layout.getFace();
    if (!face) return;

    const auto& glyphs = layout.glyphs();
    if (glyphs.empty()) return;

    int n = (int)glyphs.size();
    std::vector<SkGlyphID> glyphIds(n);
    std::vector<SkPoint>   positions(n);

    for (int i = 0; i < n; ++i) {
        glyphIds[i]  = static_cast<SkGlyphID>(glyphs[i].glyphId);
        positions[i] = SkPoint::Make(glyphs[i].pos.x, glyphs[i].pos.y);
    }

    const auto& style = layout.getStyle();
    canvas->drawGlyphs(n, glyphIds.data(), positions.data(),
                       SkPoint::Make(x, y), makeFont(*face, style.size), makePaint(style));
}
