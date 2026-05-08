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

// ── Phase 9: 禁則処理 ─────────────────────────────────────────────────────────

// 行頭禁則: these characters must not appear at the start of a line
static bool isKinsokuLineStart(uint32_t cp) {
    switch (cp) {
        // Closing brackets / punctuation
        case 0x3001: case 0x3002:  // 、。
        case 0xFF0C: case 0xFF0E:  // ，．
        case 0x30FB: case 0xFF65:  // ・
        case 0xFF1A: case 0xFF1B:  // ：；
        case 0xFF1F: case 0xFF01:  // ？！
        case 0x3009: case 0x300B:  // 〉》
        case 0x300D: case 0x300F:  // 」』
        case 0x3011: case 0x3015:  // 】〕
        case 0xFF09: case 0xFF3D:  // ）］
        case 0xFF5D:               // ｝
        case 0x2026: case 0x2025:  // …‥
        case 0x30FC:               // ー (long vowel — should stay with preceding kana)
        case 0x2019: case 0x201D:  // '' ""  (closing quotes)
            return true;
        default: break;
    }
    // Small kana: ぁぃぅぇぉっゃゅょゎ / ァィゥェォッャュョヮ
    if (cp >= 0x3041 && cp <= 0x3043) return true;  // ぁぃぅ
    if (cp >= 0x3045 && cp <= 0x3049) return true;  // ぇぉ... (odd codepoints)
    if (cp == 0x3063) return true;   // っ
    if (cp >= 0x3083 && cp <= 0x3087) return true;  // ゃゅょ
    if (cp == 0x308E) return true;   // ゎ
    if (cp >= 0x30A1 && cp <= 0x30A3) return true;  // ァィゥ
    if (cp >= 0x30A5 && cp <= 0x30A9) return true;  // ェォ...
    if (cp == 0x30C3) return true;   // ッ
    if (cp >= 0x30E3 && cp <= 0x30E7) return true;  // ャュョ
    if (cp == 0x30EE) return true;   // ヮ
    if (cp == 0x30F5 || cp == 0x30F6) return true;  // ヵヶ
    return false;
}

// 行末禁則: these characters must not appear at the end of a line
static bool isKinsokuLineEnd(uint32_t cp) {
    switch (cp) {
        case 0x3008: case 0x300A:  // 〈《
        case 0x300C: case 0x300E:  // 「『
        case 0x3010: case 0x3014:  // 【〔
        case 0xFF08: case 0xFF3B:  // （［
        case 0xFF5B:               // ｛
        case 0x2018: case 0x201C:  // '' ""  (opening quotes)
            return true;
        default: break;
    }
    return false;
}

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

// Shape segments (LTR/RTL); cx_out receives total x advance
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

// Shape segments for TTB vertical text; cy_out receives total y advance
static std::vector<ofxTypoGlyph> shapeSegmentsVertical(
    const std::vector<FaceSegment>& segs,
    const std::vector<std::shared_ptr<ofxTypoFontFace>>& faces,
    const ofxTypoTextStyle& style,
    const ofxHbShapeOptions& opts,
    float& cy_out)
{
    std::vector<ofxTypoGlyph> glyphs;
    float cy = 0.0f;
    for (const auto& seg : segs) {
        auto& face = *faces[seg.faceIndex];
        ofxHbGlyphRun run = ofxHbShaper().shape(seg.text, face.getHbFont(style.size), opts);
        for (const auto& g : run.glyphs) {
            // HarfBuzz TTB: y_advance is negative (moving down in font coords = +y in screen)
            float advY = (g.advance.y != 0.0f) ? -g.advance.y : style.size;
            glyphs.push_back({
                g.glyphId,
                { g.offset.x, cy - g.offset.y },
                { g.advance.x, g.advance.y },
                g.cluster + (uint32_t)seg.byteStart,
                seg.faceIndex
            });
            cy += advY;
        }
    }
    cy_out = cy;
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

// ── Phase 5 + Phase 9: paragraph layout ──────────────────────────────────────

ofxTypoParagraphLayout ofxTypography::layoutParagraph(const std::string& utf8,
                                                       const ofxTypoTextStyle& style,
                                                       const ofxTypoParagraphStyle& para) {
    auto faces = resolveFaces(style.font);
    if (faces.empty()) { ofLogError("ofxTypography") << "Font not found: " << style.font; return {}; }

    // ── Phase 9: vertical (縦書き) path ───────────────────────────────────────
    if (para.vertical) {
        ofxHbShapeOptions opts {
            style.language, style.script,
            ofxHbTextDirection::TopToBottom,
            style.features
        };

        auto segs = (faces.size() == 1)
                  ? std::vector<FaceSegment>{{ utf8, 0, 0 }}
                  : segmentByFace(utf8, faces);

        float totalHeight = 0.0f;
        std::vector<ofxTypoGlyph> allGlyphs =
            shapeSegmentsVertical(segs, faces, style, opts, totalHeight);


        int n = (int)allGlyphs.size();

        // Column breaking (height-based)
        struct ColRange { int start, end; float height; };
        std::vector<ColRange> ranges;

        int   colStart    = 0;
        float colHeight   = 0.0f;
        int   lastBreakIdx = -1;
        float heightAtBreak = 0.0f;

        for (int i = 0; i < n; ++i) {
            uint32_t cp = utf8Codepoint(utf8, allGlyphs[i].cluster);

            if (isForcedBreak(cp)) {
                ranges.push_back({ colStart, i, colHeight });
                colStart = i + 1; colHeight = 0.0f; lastBreakIdx = -1;
                continue;
            }

            float advY = (allGlyphs[i].advance.y != 0.0f)
                       ? -allGlyphs[i].advance.y : style.size;
            colHeight += advY;

            if (isLineBreakOpportunity(cp)) {
                lastBreakIdx = i;
                heightAtBreak = colHeight;
            }

            if (para.height > 0.0f && colHeight > para.height) {
                if (lastBreakIdx >= colStart) {
                    int breakAt = lastBreakIdx;

                    // 行頭禁則: char after break must not start a column
                    if (breakAt + 1 < n) {
                        uint32_t nextCp = utf8Codepoint(utf8, allGlyphs[breakAt + 1].cluster);
                        if (isKinsokuLineStart(nextCp) && breakAt > colStart)
                            --breakAt;
                    }
                    // 行末禁則: break char must not end a column
                    if (breakAt >= colStart) {
                        uint32_t endCp = utf8Codepoint(utf8, allGlyphs[breakAt].cluster);
                        if (isKinsokuLineEnd(endCp) && breakAt > colStart)
                            --breakAt;
                    }

                    // recompute height for the adjusted range
                    float usedHeight = 0.0f;
                    for (int j = colStart; j <= breakAt; ++j) {
                        float a = (allGlyphs[j].advance.y != 0.0f)
                                ? -allGlyphs[j].advance.y : style.size;
                        usedHeight += a;
                    }

                    ranges.push_back({ colStart, breakAt + 1, usedHeight });
                    colStart = breakAt + 1;

                    // 行頭禁則 post-fix: absorb leading forbidden chars into the previous column
                    while (colStart < n
                           && isKinsokuLineStart(utf8Codepoint(utf8, allGlyphs[colStart].cluster))
                           && !ranges.empty()) {
                        float a = (allGlyphs[colStart].advance.y != 0.0f)
                                ? -allGlyphs[colStart].advance.y : style.size;
                        ranges.back().end = colStart + 1;
                        ranges.back().height += a;
                        ++colStart;
                    }

                    colHeight = 0.0f;
                    for (int j = colStart; j <= i; ++j) {
                        float a = (allGlyphs[j].advance.y != 0.0f)
                                ? -allGlyphs[j].advance.y : style.size;
                        colHeight += a;
                    }
                    lastBreakIdx = -1;
                } else {
                    ranges.push_back({ colStart, i, colHeight - advY });
                    colStart = i; colHeight = advY; lastBreakIdx = -1;
                }
            }
        }
        if (colStart < n) ranges.push_back({ colStart, n, colHeight });

        // Build column layouts
        float colStride = style.size * para.lineHeight;
        ofxTypoParagraphLayout result;
        result.setLineStride(colStride);
        result.setVertical(true);

        // glyph0.pos.y = -offset.y (cursor_start=0 → pos = 0 - offset.y)
        // cursor_at_range_start = allGlyphs[cr.start].pos.y - glyph0.pos.y
        // This assumes constant y_offset per font (true for a single CJK face).
        float glyph0PosY = allGlyphs[0].pos.y;

        float maxHeight = 0.0f;
        for (auto& cr : ranges) {
            std::vector<ofxTypoGlyph> tg;
            float cursorAtStart = allGlyphs[cr.start].pos.y - glyph0PosY;
            for (int i = cr.start; i < cr.end; ++i) {
                const auto& g = allGlyphs[i];
                tg.push_back({ g.glyphId, { g.pos.x, g.pos.y - cursorAtStart },
                               g.advance, g.cluster, g.faceIndex });
            }
            maxHeight = std::max(maxHeight, cr.height);
            ofxTypoTextLayout colLayout;
            colLayout.init(std::move(tg), faces, style,
                           ofRectangle(0, 0, style.size, cr.height));
            result.addLine(std::move(colLayout));
        }

        result.setBounds(ofRectangle(0, 0,
                                     colStride * (float)result.lines().size(),
                                     maxHeight));
        return result;
    }

    // ── Horizontal path (Phase 5 + Phase 9 kinsoku) ───────────────────────────
    ofxHbShapeOptions opts { style.language, style.script, {}, style.features };

    auto segs = (faces.size() == 1)
              ? std::vector<FaceSegment>{{ utf8, 0, 0 }}
              : segmentByFace(utf8, faces);
    float totalWidth = 0.0f;
    std::vector<ofxTypoGlyph> allGlyphs = shapeSegments(segs, faces, style, opts, totalWidth);

    int n = (int)allGlyphs.size();

    // Greedy line breaking with kinsoku
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
            lineStart = i + 1; lineWidth = 0.0f; lastBreakIdx = -1;
            continue;
        }

        lineWidth += allGlyphs[i].advance.x;

        if (isLineBreakOpportunity(cp)) {
            lastBreakIdx = i;
            widthAtBreak = lineWidth;
        }

        if (para.width > 0.0f && lineWidth > para.width) {
            if (lastBreakIdx >= lineStart) {
                int breakAt = lastBreakIdx;

                // Phase 9 kinsoku: 行頭禁則 — shift break back so forbidden char stays on line
                if (breakAt + 1 < n) {
                    uint32_t nextCp = utf8Codepoint(utf8, allGlyphs[breakAt + 1].cluster);
                    if (isKinsokuLineStart(nextCp) && breakAt > lineStart) {
                        // Absorb forbidden char onto current line by moving break back one
                        breakAt--;
                        widthAtBreak -= allGlyphs[breakAt + 1].advance.x;
                    }
                }
                // Phase 9 kinsoku: 行末禁則 — forbidden char must not end the line
                if (breakAt >= lineStart) {
                    uint32_t endCp = utf8Codepoint(utf8, allGlyphs[breakAt].cluster);
                    if (isKinsokuLineEnd(endCp) && breakAt > lineStart) {
                        breakAt--;
                        widthAtBreak -= allGlyphs[breakAt + 1].advance.x;
                    }
                }

                ranges.push_back({ lineStart, breakAt + 1, widthAtBreak });
                int next = breakAt + 1;
                while (next < n && isSpaceChar(utf8Codepoint(utf8, allGlyphs[next].cluster))) ++next;
                lineStart = next;

                // 行頭禁則 post-fix: if lineStart lands on a forbidden char, absorb into prev line
                while (lineStart < n
                       && isKinsokuLineStart(utf8Codepoint(utf8, allGlyphs[lineStart].cluster))
                       && !ranges.empty()) {
                    ranges.back().end = lineStart + 1;
                    ranges.back().width += allGlyphs[lineStart].advance.x;
                    ++lineStart;
                }

                lineWidth = 0.0f;
                for (int j = lineStart; j <= i; ++j) lineWidth += allGlyphs[j].advance.x;
                lastBreakIdx = -1;
            } else {
                ranges.push_back({ lineStart, i, lineWidth - allGlyphs[i].advance.x });
                lineStart = i; lineWidth = allGlyphs[i].advance.x; lastBreakIdx = -1;
            }
        }
    }
    if (lineStart < n) ranges.push_back({ lineStart, n, lineWidth });

    // Build TextLayout per line
    float lineStride = style.size * para.lineHeight;
    ofxTypoParagraphLayout result;
    result.setLineStride(lineStride);

    float maxWidth = 0.0f;
    for (auto& lr : ranges) {
        std::vector<ofxTypoGlyph> tg;
        float originX = allGlyphs[lr.start].pos.x;
        for (int i = lr.start; i < lr.end; ++i) {
            const auto& g = allGlyphs[i];
            // pos.x is already the absolute advance from text start; normalize to line start.
            tg.push_back({ g.glyphId, { g.pos.x - originX, g.pos.y },
                           g.advance, g.cluster, g.faceIndex });
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
    if (layout.isVertical()) {
        // Traditional Japanese vertical text: columns progress right → left
        int n = (int)layout.lines().size();
        for (int ci = 0; ci < n; ++ci) {
            draw(layout.lines()[ci], x - ci * layout.getLineStride(), y);
        }
    } else {
        float lineY = 0.0f;
        for (auto& line : layout.lines()) {
            draw(line, x, y + lineY);
            lineY += layout.getLineStride();
        }
    }
}

// ── Phase 8: PDF draw ─────────────────────────────────────────────────────────

void ofxTypography::drawToPdf(ofxTypoPdfExporter& pdf,
                               ofxTypoTextLayout& layout, float x, float y) {
    SkCanvas* canvas = pdf.getCanvas();
    if (!canvas || layout.glyphs().empty()) return;
    ofxTypoSkiaRenderer().draw(canvas, layout, x, y);
}

void ofxTypography::drawToPdf(ofxTypoPdfExporter& pdf,
                               ofxTypoParagraphLayout& layout, float x, float y) {
    if (!pdf.isOpen()) return;
    if (layout.isVertical()) {
        float colX = 0.0f;
        for (auto& col : layout.lines()) {
            drawToPdf(pdf, col, x + colX, y);
            colX += layout.getLineStride();
        }
    } else {
        float lineY = 0.0f;
        for (auto& line : layout.lines()) {
            drawToPdf(pdf, line, x, y + lineY);
            lineY += layout.getLineStride();
        }
    }
}
