// unity includes
#include "ofxTypoFontFace.cpp"
#include "ofxTypoSkiaRenderer.cpp"
#include "ofxTypoTextLayout.cpp"

#include "ofxTypography.h"
#include "ofxTypoSkiaRenderer.h"
#include "ofxHbShaper.h"
#include "ofMain.h"
#include "ofLog.h"
#include <algorithm>
#include <cmath>

// ── UTF-8 helpers ─────────────────────────────────────────────────────────────

static uint32_t utf8Codepoint(const std::string& s, size_t off) {
    if (off >= s.size()) return 0;
    auto c = (unsigned char)s[off];
    if (c < 0x80) return c;
    if (c < 0xE0) return ((c & 0x1F) << 6)  | ((unsigned char)s[off+1] & 0x3F);
    if (c < 0xF0) return ((c & 0x0F) << 12) | (((unsigned char)s[off+1] & 0x3F) << 6)
                                             | ((unsigned char)s[off+2] & 0x3F);
    return ((c & 0x07) << 18) | (((unsigned char)s[off+1] & 0x3F) << 12)
                              | (((unsigned char)s[off+2] & 0x3F) << 6)
                              | ((unsigned char)s[off+3] & 0x3F);
}

static bool isLineBreakOpportunity(uint32_t cp) {
    if (cp == 0x20 || cp == 0x09) return true;  // space, tab
    if (cp >= 0x3040 && cp <= 0x30FF) return true;  // Hiragana / Katakana
    if (cp >= 0x4E00 && cp <= 0x9FFF) return true;  // CJK Unified Ideographs
    if (cp >= 0x3400 && cp <= 0x4DBF) return true;  // CJK Extension A
    if (cp >= 0xF900 && cp <= 0xFAFF) return true;  // CJK Compat Ideographs
    if (cp >= 0xFF00 && cp <= 0xFFEF) return true;  // Fullwidth / Halfwidth
    return false;
}

static bool isSpaceChar(uint32_t cp) { return cp == 0x20 || cp == 0x09; }
static bool isForcedBreak(uint32_t cp) { return cp == 0x0A || cp == 0x0D; }

// ── ofxTypography ─────────────────────────────────────────────────────────────

SkCanvas* ofxTypography::ensureSurface() {
    int w = ofGetWidth();
    int h = ofGetHeight();
    if (!surface_.isAllocated() || surface_.getWidth() != w || surface_.getHeight() != h) {
        surface_.allocate(w, h);
    }
    surface_.clear(ofColor(0, 0, 0, 0));
    return surface_.getCanvas();
}

void ofxTypography::loadFont(const std::string& name, const std::filesystem::path& path) {
    auto face = std::make_shared<ofxTypoFontFace>();
    if (!face->load(path)) {
        ofLogError("ofxTypography::loadFont") << "Failed to load: " << path;
        return;
    }
    fonts_[name] = std::move(face);
    ofLogNotice("ofxTypography::loadFont") << "Loaded font '" << name << "' from " << path;
}

// ── Phase 3 ───────────────────────────────────────────────────────────────────

void ofxTypography::draw(const std::string& utf8, float x, float y,
                          const ofxTypoTextStyle& style) {
    auto it = fonts_.find(style.font);
    if (it == fonts_.end()) { ofLogError("ofxTypography") << "Font not found: " << style.font; return; }
    auto& face = *it->second;

    SkCanvas* canvas = ensureSurface();
    if (!canvas) return;

    ofxHbShapeOptions opts { style.language, style.script, {}, style.features };
    ofxHbShaper shaper;
    ofxHbGlyphRun run = shaper.shape(utf8, face.getHbFont(style.size), opts);

    ofxTypoSkiaRenderer().draw(canvas, run, face, x, y, style);
    surface_.updateTexture();
    surface_.draw(0, 0);
}

// ── Phase 4 ───────────────────────────────────────────────────────────────────

ofxTypoTextLayout ofxTypography::layout(const std::string& utf8,
                                         const ofxTypoTextStyle& style) {
    auto it = fonts_.find(style.font);
    if (it == fonts_.end()) { ofLogError("ofxTypography") << "Font not found: " << style.font; return {}; }
    auto& facePtr = it->second;

    ofxHbShapeOptions opts { style.language, style.script, {}, style.features };
    ofxHbGlyphRun run = ofxHbShaper().shape(utf8, facePtr->getHbFont(style.size), opts);

    std::vector<ofxTypoGlyph> glyphs;
    glyphs.reserve(run.glyphs.size());
    float cx = 0.0f;
    for (const auto& g : run.glyphs) {
        glyphs.push_back({ g.glyphId, {cx + g.offset.x, -g.offset.y}, {g.advance.x, g.advance.y}, g.cluster });
        cx += g.advance.x;
    }

    ofxTypoTextLayout lay;
    lay.init(std::move(glyphs), facePtr, style, ofRectangle(0, -style.size, cx, style.size));
    return lay;
}

void ofxTypography::draw(ofxTypoTextLayout& layout, float x, float y) {
    if (layout.glyphs().empty()) return;
    SkCanvas* canvas = ensureSurface();
    if (!canvas) return;
    ofxTypoSkiaRenderer().draw(canvas, layout, x, y);
    surface_.updateTexture();
    surface_.draw(0, 0);
}

// ── Phase 5 ───────────────────────────────────────────────────────────────────

ofxTypoParagraphLayout ofxTypography::layoutParagraph(const std::string& utf8,
                                                       const ofxTypoTextStyle& style,
                                                       const ofxTypoParagraphStyle& para) {
    auto it = fonts_.find(style.font);
    if (it == fonts_.end()) { ofLogError("ofxTypography") << "Font not found: " << style.font; return {}; }
    auto& facePtr = it->second;

    ofxHbShapeOptions opts { style.language, style.script, {}, style.features };
    const auto& hbGlyphs = ofxHbShaper().shape(utf8, facePtr->getHbFont(style.size), opts).glyphs;
    int n = (int)hbGlyphs.size();

    // ── greedy line breaking ──────────────────────────────────────────────────
    struct LineRange { int start, end; float width; };
    std::vector<LineRange> ranges;

    int   lineStart      = 0;
    float lineWidth      = 0.0f;
    int   lastBreakIdx   = -1;
    float widthAtBreak   = 0.0f;

    for (int i = 0; i < n; ++i) {
        uint32_t cp = utf8Codepoint(utf8, hbGlyphs[i].cluster);

        if (isForcedBreak(cp)) {
            ranges.push_back({ lineStart, i, lineWidth });
            lineStart = i + 1;
            lineWidth = 0.0f;
            lastBreakIdx = -1;
            continue;
        }

        lineWidth += hbGlyphs[i].advance.x;

        if (isLineBreakOpportunity(cp)) {
            lastBreakIdx = i;
            widthAtBreak = lineWidth;
        }

        if (para.width > 0.0f && lineWidth > para.width) {
            if (lastBreakIdx >= lineStart) {
                ranges.push_back({ lineStart, lastBreakIdx + 1, widthAtBreak });
                int next = lastBreakIdx + 1;
                while (next < n && isSpaceChar(utf8Codepoint(utf8, hbGlyphs[next].cluster))) ++next;
                lineStart = next;
                lineWidth = 0.0f;
                for (int j = lineStart; j <= i; ++j) lineWidth += hbGlyphs[j].advance.x;
                lastBreakIdx = -1;
            } else {
                ranges.push_back({ lineStart, i, lineWidth - hbGlyphs[i].advance.x });
                lineStart = i;
                lineWidth = hbGlyphs[i].advance.x;
                lastBreakIdx = -1;
            }
        }
    }
    if (lineStart < n) ranges.push_back({ lineStart, n, lineWidth });

    // ── build TextLayout per line ─────────────────────────────────────────────
    float lineStride = style.size * para.lineHeight;
    ofxTypoParagraphLayout result;
    result.setLineStride(lineStride);

    float maxWidth = 0.0f;
    for (auto& lr : ranges) {
        std::vector<ofxTypoGlyph> tg;
        float cx = 0.0f;
        for (int i = lr.start; i < lr.end; ++i) {
            const auto& g = hbGlyphs[i];
            tg.push_back({ g.glyphId, {cx + g.offset.x, -g.offset.y}, {g.advance.x, g.advance.y}, g.cluster });
            cx += g.advance.x;
        }

        float alignOff = 0.0f;
        if (para.width > 0.0f) {
            if (para.align == ofxTypoAlign::Center) alignOff = (para.width - lr.width) * 0.5f;
            else if (para.align == ofxTypoAlign::Right) alignOff = para.width - lr.width;
        }
        for (auto& g : tg) g.pos.x += alignOff;

        maxWidth = std::max(maxWidth, lr.width);
        ofxTypoTextLayout lineLayout;
        lineLayout.init(std::move(tg), facePtr, style, ofRectangle(alignOff, -style.size, lr.width, style.size));
        result.addLine(std::move(lineLayout));
    }

    result.setBounds(ofRectangle(0, -style.size, maxWidth,
                                 lineStride * (float)result.lines().size()));
    return result;
}

void ofxTypography::draw(ofxTypoParagraphLayout& layout, float x, float y) {
    float lineY = 0.0f;
    for (auto& line : layout.lines()) {
        draw(line, x, y + lineY);
        lineY += layout.getLineStride();
    }
}
