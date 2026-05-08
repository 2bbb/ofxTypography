# ofxTypography

High-quality multilingual creative typography for openFrameworks.

Combines [ofxHarfBuzz](https://github.com/2bbb/ofxHarfBuzz) (text shaping) and [ofxSkia](https://github.com/2bbb/ofxSkia) (rendering) to draw Unicode text with correct glyph substitution, ligatures, and kerning — for any language, including Japanese, Arabic, Hebrew, and Indic scripts.

## Architecture

```
UTF-8 string
  → ofxTypography     (layout, fallback, line breaking)
  → ofxHarfBuzz       (shaping → glyph IDs + advances + clusters)
  → ofxSkia           (render glyphs → SkCanvas → ofTexture → OF window)
```

`ofxTypoFontFace` owns the font bytes and creates both `hb_face_t` and `SkTypeface` from the same data, ensuring HarfBuzz and Skia are always in sync.

## Quick start

```cpp
ofxTypography typo;
typo.loadFont("noto", "fonts/NotoSansCJK-Regular.ttf");

// in draw():
ofxTypoTextStyle style;
style.font = "noto";
style.size = 48.0f;
style.color = ofColor::white;
typo.draw("こんにちは openFrameworks", 40, 100, style);
```

## API

### `ofxTypography`

```cpp
// Load a font by name
void loadFont(const std::string& name, const std::filesystem::path& path);

// Load an ordered fallback collection (Phase 6+)
void loadFontCollection(const std::string& name,
                        const std::vector<std::filesystem::path>& paths);

// Immediate draw (Phase 3)
void draw(const std::string& utf8, float x, float y,
          const ofxTypoTextStyle& style);

// Layout → manipulate glyphs → draw (Phase 4)
ofxTypoTextLayout layout(const std::string& utf8,
                          const ofxTypoTextStyle& style);
void draw(ofxTypoTextLayout& layout, float x, float y);

// Paragraph layout with wrapping and alignment (Phase 5)
ofxTypoParagraphLayout layoutParagraph(const std::string& utf8,
                                        const ofxTypoTextStyle& style,
                                        const ofxTypoParagraphStyle& para = {});
void draw(ofxTypoParagraphLayout& layout, float x, float y);

// PDF export (Phase 8)
void drawToPdf(ofxTypoPdfExporter& pdf, ofxTypoTextLayout& layout,   float x, float y);
void drawToPdf(ofxTypoPdfExporter& pdf, ofxTypoParagraphLayout& layout, float x, float y);
```

### `ofxTypoTextStyle`

```cpp
struct ofxTypoTextStyle {
    std::string font     = "";           // name registered with loadFont()
    float       size     = 48.0f;        // pixels
    ofColor     color    = ofColor::white;
    std::string language = "und";        // BCP 47 e.g. "ja", "ar", "en"
    std::string script   = "";           // ISO 15924 e.g. "Hira", "Arab"
    std::vector<std::string> features;  // OpenType e.g. {"liga=1", "palt=1"}
};
```

### `ofxTypoTextLayout`
Holds shaped glyphs and their font faces. Glyphs are mutable so you can animate positions before drawing.

```cpp
ofxTypoTextLayout layout = typo.layout("Hello", style);
for (auto& g : layout.glyphs()) {
    g.pos.y += sinf(ofGetElapsedTimef() + g.cluster * 0.3f) * 10.0f;
}
typo.draw(layout, 40, 100);
```

### `ofxTypoParagraphStyle`

```cpp
struct ofxTypoParagraphStyle {
    float        width      = 0.0f;              // wrap width in pixels (0 = no wrap)
    float        lineHeight = 1.4f;              // em multiplier
    ofxTypoAlign align      = ofxTypoAlign::Left; // Left / Center / Right
    bool         vertical   = false;             // vertical writing (Phase 9)
    float        height     = 0.0f;              // column height limit
};
```

### `ofxTypoFontFace`
Owns font bytes, `hb_face_t`, and `SkTypeface`. Used internally; expose if you need direct HarfBuzz or Skia access.

```cpp
ofxHbFont&        face.getHbFont(float sizePixels);
sk_sp<SkTypeface> face.getSkTypeface();
bool              face.supports(uint32_t codepoint);
```

### `ofxTypoPdfExporter` (Phase 8)
Exports layouts to a multi-page PDF using Skia's PDF backend.

```cpp
ofxTypoPdfExporter pdf;
if (pdf.begin("output.pdf", 595, 842)) {   // A4 in points
    typo.drawToPdf(pdf, layout, 40, 60);
    pdf.nextPage(595, 842);
    typo.drawToPdf(pdf, layout2, 40, 60);
    pdf.end();
}
```

## Dependencies

- [ofxHarfBuzz](https://github.com/2bbb/ofxHarfBuzz) — text shaping
- [ofxSkia](https://github.com/2bbb/ofxSkia) — rendering
- openFrameworks 0.12.0+

Both dependencies must have their libraries built before this addon compiles. See their respective READMEs.

## `addons.make`

```
ofxTypography
ofxSkia
ofxHarfBuzz
```

## License

MIT
