#include "ofxTypoPdfExporter.h"
#include "include/docs/SkPDFDocument.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "ofLog.h"

bool ofxTypoPdfExporter::begin(const std::filesystem::path& path, float width, float height,
                                const ofColor& background) {
    end();

    // path.c_str() returns const wchar_t* on Windows; SkFILEWStream only accepts
    // const char*, so convert via string() which uses the native 8-bit encoding.
    stream_ = std::make_unique<SkFILEWStream>(path.string().c_str());
    if (!stream_->isValid()) {
        ofLogError("ofxTypoPdfExporter::begin") << "Cannot open file: " << path;
        stream_.reset();
        return false;
    }

    SkPDF::Metadata meta;
    doc_ = SkPDF::MakeDocument(stream_.get(), meta);
    if (!doc_) {
        ofLogError("ofxTypoPdfExporter::begin")
            << "SkPDF::MakeDocument returned null — Skia built without skia_enable_pdf?";
        stream_.reset();
        return false;
    }

    canvas_ = doc_->beginPage(width, height);
    if (!canvas_) return false;

    if (background.a > 0) {
        SkPaint bg;
        bg.setColor(SkColorSetARGB(background.a, background.r, background.g, background.b));
        canvas_->drawRect(SkRect::MakeWH(width, height), bg);
    }
    return true;
}

SkCanvas* ofxTypoPdfExporter::nextPage(float width, float height) {
    if (!doc_) return nullptr;
    doc_->endPage();
    canvas_ = doc_->beginPage(width, height);
    return canvas_;
}

void ofxTypoPdfExporter::end() {
    if (!doc_) return;
    doc_->close();
    doc_.reset();
    stream_.reset();
    canvas_ = nullptr;
}
