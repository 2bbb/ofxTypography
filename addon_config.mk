meta:
	ADDON_NAME = ofxTypography
	ADDON_DESCRIPTION = High-quality multilingual creative typography for openFrameworks
	ADDON_AUTHOR = 2bbb
	ADDON_TAGS = "typography" "text" "multilingual" "japanese"
	ADDON_URL = http://github.com/2bbb/ofxTypography

common:
	ADDON_INCLUDES = src
	ADDON_DEPENDENCIES = ofxSkia ofxHarfBuzz
	# ofxTypography.cpp is the unity-build entry for Xcode; exclude individual TUs from make
	ADDON_SOURCES_EXCLUDE = src/ofxTypoFontFace.cpp
	ADDON_SOURCES_EXCLUDE += src/ofxTypoSkiaRenderer.cpp
	ADDON_SOURCES_EXCLUDE += src/ofxTypoTextLayout.cpp
	ADDON_SOURCES_EXCLUDE += src/ofxTypoPdfExporter.cpp

osx:

linux64:

linuxaarch64:

vs:
	# /utf-8      : treat source files as UTF-8 (required for Japanese string literals)
	# _ITERATOR_DEBUG_LEVEL=0 : match Release-built Skia/HarfBuzz static libs;
	#               without this, Debug builds trigger LNK2038 mismatch.
	ADDON_CFLAGS = /utf-8 /D_ITERATOR_DEBUG_LEVEL=0

msys2:

emscripten:

android:
