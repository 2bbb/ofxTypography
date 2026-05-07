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
    std::vector<SkGlyphID> ids(n);
    std::vector<SkPoint>   positions(n);

    float cx = 0.0f, cy = 0.0f;
    for (int i = 0; i < n; ++i) {
        const auto& g = run.glyphs[i];
        ids[i]       = static_cast<SkGlyphID>(g.glyphId);
        positions[i] = SkPoint::Make(cx + g.offset.x, cy - g.offset.y);
        cx += g.advance.x;
        cy -= g.advance.y;
    }

    canvas->drawGlyphs(n, ids.data(), positions.data(),
                       SkPoint::Make(x, y), makeFont(face, style.size), makePaint(style));
}

void ofxTypoSkiaRenderer::draw(SkCanvas* canvas,
                                const ofxTypoTextLayout& layout,
                                float x, float y) {
    if (!canvas) return;

    const auto& faces  = layout.getFaces();
    const auto& glyphs = layout.glyphs();
    if (faces.empty() || glyphs.empty()) return;

    const auto& style = layout.getStyle();
    SkPoint     origin = SkPoint::Make(x, y);

    // Draw one group per face index to allow mixed-font layouts
    for (int fi = 0; fi < (int)faces.size(); ++fi) {
        if (!faces[fi]) continue;

        std::vector<SkGlyphID> ids;
        std::vector<SkPoint>   positions;
        for (const auto& g : glyphs) {
            if (g.faceIndex != fi) continue;
            ids.push_back(static_cast<SkGlyphID>(g.glyphId));
            positions.push_back(SkPoint::Make(g.pos.x, g.pos.y));
        }
        if (ids.empty()) continue;

        canvas->drawGlyphs((int)ids.size(), ids.data(), positions.data(),
                           origin, makeFont(*faces[fi], style.size), makePaint(style));
    }
}
