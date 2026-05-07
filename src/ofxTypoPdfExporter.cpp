#include "ofxTypoPdfExporter.h"
#include "include/docs/SkPDFDocument.h"
#include "ofLog.h"

bool ofxTypoPdfExporter::begin(const std::filesystem::path& path, float width, float height) {
    end();

    stream_ = std::make_unique<SkFILEWStream>(path.c_str());
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
    return canvas_ != nullptr;
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
