set(DEPS "${CMAKE_SOURCE_DIR}/android/native/deps/${ANDROID_ABI}")

# for IrrlichtMt:
set(PNG_PNG_INCLUDE_DIR ${DEPS}/libpng/include)
set(PNG_LIBRARY ${DEPS}/libpng/libpng.a)

set(CURL_INCLUDE_DIR ${DEPS}/Curl/include)
set(CURL_LIBRARY ${DEPS}/Curl/libcurl.a;${DEPS}/Curl/libmbedcrypto.a;${DEPS}/Curl/libmbedtls.a;${DEPS}/Curl/libmbedx509.a)
set(FREETYPE_INCLUDE_DIR_ft2build ${DEPS}/Freetype/include/freetype2)
set(FREETYPE_INCLUDE_DIR_freetype2 ${FREETYPE_INCLUDE_DIR_ft2build}/freetype)
set(FREETYPE_LIBRARY ${DEPS}/Freetype/libfreetype.a)
set(GETTEXT_INCLUDE_DIR ${DEPS}/Gettext/include)
set(GETTEXT_LIBRARY ${DEPS}/Gettext/libintl.a)
# decouple this with gettext when gettext is disabled
include_directories(${DEPS}/Iconv/include)
set(ICONV_LIBRARY ${DEPS}/Iconv/libiconv.a;${DEPS}/Iconv/libcharset.a)
set(LUA_INCLUDE_DIR ${DEPS}/LuaJIT/include)
set(LUA_LIBRARY ${DEPS}/LuaJIT/libluajit.a)
set(OGG_INCLUDE_DIR ${DEPS}/Vorbis/include)
set(OGG_LIBRARY ${DEPS}/Vorbis/libogg.a)
set(OPENAL_INCLUDE_DIR ${DEPS}/OpenAL-Soft/include)
set(OPENAL_LIBRARY ${DEPS}/OpenAL-Soft/libopenal.a;OpenSLES)
set(SQLITE3_INCLUDE_DIR ${DEPS}/SQLite/include)
set(SQLITE3_LIBRARY ${DEPS}/SQLite/libsqlite3.a)
set(VORBIS_INCLUDE_DIR ${DEPS}/Vorbis/include)
set(VORBISFILE_LIBRARY ${DEPS}/Vorbis/libvorbisfile.a)
set(VORBIS_LIBRARY ${DEPS}/Vorbis/libvorbis.a)
set(ZSTD_INCLUDE_DIR ${DEPS}/Zstd/include)
set(ZSTD_LIBRARY ${DEPS}/Zstd/libzstd.a)
