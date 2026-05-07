#pragma once
#include "ofxTypoTextLayout.h"
#include "ofxTypoParagraphLayout.h"
#include "include/core/SkDocument.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkStream.h"
#include "ofColor.h"
#include <filesystem>
#include <memory>

// Phase 8: PDF export via Skia SkDocument/SkPDF.
// Requires Skia built with skia_enable_pdf=true.
// With the "none" PDF stub, begin() returns false and no file is written.
class ofxTypoPdfExporter {
public:
    // Open a PDF at path with the given page dimensions (points = 1/72 inch).
    // Returns false if SkPDF::MakeDocument returned nullptr (PDF disabled in Skia build).
    bool begin(const std::filesystem::path& path, float width, float height,
               const ofColor& background = ofColor(0, 0, 0, 0));

    // Returns the canvas for the current page; nullptr if not open.
    SkCanvas* getCanvas() const { return canvas_; }

    // Finish current page and start a new one with the given dimensions.
    // Returns the new canvas or nullptr on error.
    SkCanvas* nextPage(float width, float height);

    // Close and flush the document to disk.
    void end();

    bool isOpen() const { return doc_ != nullptr; }

private:
    sk_sp<SkDocument>               doc_;
    std::unique_ptr<SkFILEWStream>  stream_;
    SkCanvas*                       canvas_ = nullptr;
};
