#include "ofxTypoSkiaRenderer.h"
#include "ofxTypoFontFace.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPoint.h"
#include <vector>

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

    SkFont skFont(face.getSkTypeface(), style.size);
    skFont.setEdging(SkFont::Edging::kSubpixelAntiAlias);
    skFont.setSubpixel(true);

    const ofColor& c = style.color;
    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SkColorSetARGB(c.a, c.r, c.g, c.b));

    canvas->drawGlyphs(n, glyphIds.data(), positions.data(),
                       SkPoint::Make(x, y), skFont, paint);
}
