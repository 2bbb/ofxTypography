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

static size_t utf8CharLen(const std::string& s, size_t off) {
    if (off >= s.size()) return 0;
    auto c = (unsigned char)s[off];
    if (c < 0x80) return 1;
    if (c < 0xE0) return 2;
    if (c < 0xF0) return 3;
    return 4;
}

static bool isLineBreakOpportunity(uint32_t cp) {
    if (cp == 0x20 || cp == 0x09) return true;
    if (cp >= 0x3040 && cp <= 0x30FF) return true;  // Hiragana / Katakana
    if (cp >= 0x4E00 && cp <= 0x9FFF) return true;  // CJK Unified Ideographs
    if (cp >= 0x3400 && cp <= 0x4DBF) return true;  // CJK Extension A
    if (cp >= 0xF900 && cp <= 0xFAFF) return true;  // CJK Compat
    if (cp >= 0xFF00 && cp <= 0xFFEF) return true;  // Fullwidth / Halfwidth
    return false;
}

static bool isSpaceChar(uint32_t cp)   { return cp == 0x20 || cp == 0x09; }
static bool isForcedBreak(uint32_t cp) { return cp == 0x0A || cp == 0x0D; }

// ── Fallback segmentation ─────────────────────────────────────────────────────

struct FaceSegment { std::string text; int faceIndex; size_t byteStart; };

static std::vector<FaceSegment> segmentByFace(
    const std::string& utf8,
    const std::vector<std::shared_ptr<ofxTypoFontFace>>& faces)
{
    std::vector<FaceSegment> segs;
    size_t i = 0;
    while (i < utf8.size()) {
        size_t start  = i;
        size_t cpLen  = utf8CharLen(utf8, i);
        uint32_t cp   = utf8Codepoint(utf8, i);
        i += cpLen;

        int fi = 0;
        for (int f = 0; f < (int)faces.size(); ++f) {
            if (faces[f]->supports(cp)) { fi = f; break; }
        }

        if (segs.empty() || segs.back().faceIndex != fi) {
            segs.push_back({ utf8.substr(start, cpLen), fi, start });
        } else {
            segs.back().text += utf8.substr(start, cpLen);
        }
    }
    return segs;
}

// Shape segments and return combined glyph vector; cx_out receives final x
static std::vector<ofxTypoGlyph> shapeSegments(
    const std::vector<FaceSegment>& segs,
    const std::vector<std::shared_ptr<ofxTypoFontFace>>& faces,
    const ofxTypoTextStyle& style,
    const ofxHbShapeOptions& opts,
    float& cx_out)
{
    std::vector<ofxTypoGlyph> glyphs;
    float cx = 0.0f;
    for (const auto& seg : segs) {
        auto& face = *faces[seg.faceIndex];
        ofxHbGlyphRun run = ofxHbShaper().shape(seg.text, face.getHbFont(style.size), opts);
        for (const auto& g : run.glyphs) {
            glyphs.push_back({
                g.glyphId,
                { cx + g.offset.x, -g.offset.y },
                { g.advance.x, g.advance.y },
                g.cluster + (uint32_t)seg.byteStart,
                seg.faceIndex
            });
            cx += g.advance.x;
        }
    }
    cx_out = cx;
    return glyphs;
}

// ── ofxTypography ─────────────────────────────────────────────────────────────

std::vector<std::shared_ptr<ofxTypoFontFace>>
ofxTypography::resolveFaces(const std::string& name) const {
    auto cit = collections_.find(name);
    if (cit != collections_.end()) return cit->second;
    auto fit = fonts_.find(name);
    if (fit != fonts_.end()) return { fit->second };
    return {};
}

SkCanvas* ofxTypography::ensureSurface() {
    int w = ofGetWidth(), h = ofGetHeight();
    if (!surface_.isAllocated() || surface_.getWidth() != w || surface_.getHeight() != h)
        surface_.allocate(w, h);
    surface_.clear(ofColor(0, 0, 0, 0));
    return surface_.getCanvas();
}

void ofxTypography::loadFont(const std::string& name, const std::filesystem::path& path) {
    auto face = std::make_shared<ofxTypoFontFace>();
    if (!face->load(path)) {
        ofLogError("ofxTypography::loadFont") << "Failed to load: " << path;
        return;
    }
    fonts_[name] = face;
    ofLogNotice("ofxTypography::loadFont")
        << "Loaded '" << name << "' [" << face->getFamilyName() << "] from " << path;
}

void ofxTypography::loadFontCollection(const std::string& name,
                                        const std::vector<std::filesystem::path>& paths) {
    std::vector<std::shared_ptr<ofxTypoFontFace>> faces;
    for (const auto& p : paths) {
        auto face = std::make_shared<ofxTypoFontFace>();
        if (face->load(p)) {
            ofLogNotice("ofxTypography::loadFontCollection")
                << "  [" << faces.size() << "] " << face->getFamilyName() << " <- " << p;
            faces.push_back(face);
        } else {
            ofLogError("ofxTypography::loadFontCollection") << "Failed to load: " << p;
        }
    }
    if (!faces.empty()) {
        collections_[name] = std::move(faces);
        ofLogNotice("ofxTypography::loadFontCollection")
            << "Collection '" << name << "' ready with " << collections_[name].size() << " face(s)";
    }
}

// ── Phase 3 ───────────────────────────────────────────────────────────────────

void ofxTypography::draw(const std::string& utf8, float x, float y,
                          const ofxTypoTextStyle& style) {
    auto faces = resolveFaces(style.font);
    if (faces.empty()) { ofLogError("ofxTypography") << "Font not found: " << style.font; return; }

    SkCanvas* canvas = ensureSurface();
    if (!canvas) return;

    ofxHbShapeOptions opts { style.language, style.script, {}, style.features };
    auto segs   = (faces.size() == 1)
                ? std::vector<FaceSegment>{{ utf8, 0, 0 }}
                : segmentByFace(utf8, faces);
    float cx = 0.0f;
    auto glyphs = shapeSegments(segs, faces, style, opts, cx);

    ofxTypoTextLayout lay;
    lay.init(std::move(glyphs), faces, style, ofRectangle(0, -style.size, cx, style.size));
    ofxTypoSkiaRenderer().draw(canvas, lay, x, y);
    surface_.updateTexture();
    surface_.draw(0, 0);
}

// ── Phase 4 ───────────────────────────────────────────────────────────────────

ofxTypoTextLayout ofxTypography::layout(const std::string& utf8,
                                         const ofxTypoTextStyle& style) {
    auto faces = resolveFaces(style.font);
    if (faces.empty()) { ofLogError("ofxTypography") << "Font not found: " << style.font; return {}; }

    ofxHbShapeOptions opts { style.language, style.script, {}, style.features };
    auto segs   = (faces.size() == 1)
                ? std::vector<FaceSegment>{{ utf8, 0, 0 }}
                : segmentByFace(utf8, faces);
    float cx = 0.0f;
    auto glyphs = shapeSegments(segs, faces, style, opts, cx);

    ofxTypoTextLayout lay;
    lay.init(std::move(glyphs), faces, style, ofRectangle(0, -style.size, cx, style.size));
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
    auto faces = resolveFaces(style.font);
    if (faces.empty()) { ofLogError("ofxTypography") << "Font not found: " << style.font; return {}; }

    ofxHbShapeOptions opts { style.language, style.script, {}, style.features };

    // Shape entire text with fallback segmentation; cluster values map to original utf8
    auto segs = (faces.size() == 1)
              ? std::vector<FaceSegment>{{ utf8, 0, 0 }}
              : segmentByFace(utf8, faces);
    float totalWidth = 0.0f;
    std::vector<ofxTypoGlyph> allGlyphs = shapeSegments(segs, faces, style, opts, totalWidth);

    // Store un-positioned advances for line-break scan (use allGlyphs[i].advance.x)
    // Note: allGlyphs[i].pos.x is already accumulated; use it for splitting.

    int n = (int)allGlyphs.size();

    // ── greedy line breaking on allGlyphs ─────────────────────────────────────
    struct LineRange { int start, end; float width; };
    std::vector<LineRange> ranges;

    int   lineStart    = 0;
    float lineWidth    = 0.0f;
    int   lastBreakIdx = -1;
    float widthAtBreak = 0.0f;

    for (int i = 0; i < n; ++i) {
        uint32_t cp = utf8Codepoint(utf8, allGlyphs[i].cluster);

        if (isForcedBreak(cp)) {
            ranges.push_back({ lineStart, i, lineWidth });
            lineStart = i + 1;
            lineWidth = 0.0f;
            lastBreakIdx = -1;
            continue;
        }

        lineWidth += allGlyphs[i].advance.x;

        if (isLineBreakOpportunity(cp)) {
            lastBreakIdx = i;
            widthAtBreak = lineWidth;
        }

        if (para.width > 0.0f && lineWidth > para.width) {
            if (lastBreakIdx >= lineStart) {
                ranges.push_back({ lineStart, lastBreakIdx + 1, widthAtBreak });
                int next = lastBreakIdx + 1;
                while (next < n && isSpaceChar(utf8Codepoint(utf8, allGlyphs[next].cluster))) ++next;
                lineStart = next;
                lineWidth = 0.0f;
                for (int j = lineStart; j <= i; ++j) lineWidth += allGlyphs[j].advance.x;
                lastBreakIdx = -1;
            } else {
                ranges.push_back({ lineStart, i, lineWidth - allGlyphs[i].advance.x });
                lineStart = i;
                lineWidth = allGlyphs[i].advance.x;
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
            const auto& g = allGlyphs[i];
            tg.push_back({ g.glyphId, { cx + (g.pos.x - allGlyphs[lr.start].pos.x), g.pos.y },
                           g.advance, g.cluster, g.faceIndex });
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
        lineLayout.init(std::move(tg), faces, style,
                        ofRectangle(alignOff, -style.size, lr.width, style.size));
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
