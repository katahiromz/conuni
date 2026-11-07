// conuni_test.cpp --- Tests for conuni (Console Unicode Input/Output for C++/Win32)
// License: MIT
//
// 非ASCII文字は \u / \x エスケープで記述しているため、
// ソースファイルの文字コードに依存しません。
// ファイル入出力を使うので、テスト中にテンポラリファイルを作成します。

#include "conuni.h"
#include <cstdio>
#include <string>

//////////////////////////////////////////////////////////////////
// Mini test framework

static int g_nTotal = 0;
static int g_nFailed = 0;

#define CHECK(expr) \
    do { \
        ++g_nTotal; \
        if (!(expr)) { \
            ++g_nFailed; \
            std::fprintf(stderr, "FAILED: %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
        } \
    } while (0)

//////////////////////////////////////////////////////////////////
// Helpers

static std::string MakeTempPath() {
    char szDir[MAX_PATH], szPath[MAX_PATH];
    szDir[0] = szPath[0] = 0;
    ::GetTempPathA(MAX_PATH, szDir);
    ::GetTempFileNameA(szDir, "cun", 0, szPath);
    return szPath;
}

static bool WriteBytes(const std::string& path, const std::string& bytes) {
    FILE* fp = std::fopen(path.c_str(), "wb");
    if (!fp)
        return false;
    std::fwrite(bytes.data(), 1, bytes.size(), fp);
    std::fclose(fp);
    return true;
}

static std::string ReadBytes(const std::string& path) {
    std::string bytes;
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp)
        return bytes;
    char buf[1024];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof(buf), fp)) > 0)
        bytes.append(buf, n);
    std::fclose(fp);
    return bytes;
}

// UTF-8 literals
static const char UTF8_A[]      = "\xE3\x81\x82"; // U+3042 (HIRAGANA A)
static const char UTF8_I[]      = "\xE3\x81\x84"; // U+3044 (HIRAGANA I)
static const char UTF8_U[]      = "\xE3\x81\x86"; // U+3046 (HIRAGANA U)
static const char UTF8_EMOJI[]  = "\xF0\x9F\x98\x80"; // U+1F600

//////////////////////////////////////////////////////////////////
// Tests: Conversion

static void TestConversion() {
    const std::wstring wide = L"A\u3042\u3044\u3046";
    const std::string utf8 = std::string("A") + UTF8_A + UTF8_I + UTF8_U;

    CHECK(Utf8FromWide(wide) == utf8);
    CHECK(WideFromUtf8(utf8) == wide);

    // round trip
    CHECK(WideFromUtf8(Utf8FromWide(wide)) == wide);

    // surrogate pair (U+1F600)
    const std::wstring emoji = L"\U0001F600";
    CHECK(emoji.size() == 2);
    CHECK(Utf8FromWide(emoji) == UTF8_EMOJI);
    CHECK(WideFromUtf8(std::string(UTF8_EMOJI)) == emoji);

    // Shift_JIS
    CHECK(SjisFromWide(L"\u3042") == "\x82\xA0");
    CHECK(WideFromSjis(std::string("\x82\xA0")) == L"\u3042");

    // empty
    CHECK(Utf8FromWide(std::wstring()).empty());
    CHECK(WideFromUtf8(std::string()).empty());
    CHECK(AnsiFromWide(std::wstring()).empty());
    CHECK(WideFromAnsi(std::string()).empty());

    // ASCII with the default code page
    CHECK(AnsiFromWide(L"hello") == "hello");
    CHECK(WideFromAnsi("hello") == L"hello");
}

// ASCII 高速パス / スタックバッファ / ヒープの境界をまたぐ長さと、
// 非ASCII文字の位置を変えて確認する。
static void TestConversionBoundaries() {
    const size_t lengths[] = {
        1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 255, 256, 257, 340, 341,
        511, 512, 513, 1023, 1024, 1025, 2000, 5000
    };
    for (size_t len : lengths) {
        for (int mode = 0; mode < 4; ++mode) {
            std::wstring wide(len, L'a');
            std::string expected(len, 'a');
            if (mode > 0) {
                size_t pos = (mode == 1) ? 0 : (mode == 2) ? len / 2 : len - 1;
                wide[pos] = L'\u3042';
                expected = std::string(pos, 'a') + UTF8_A + std::string(len - pos - 1, 'a');
            }
            CHECK(Utf8FromWide(wide) == expected);
            CHECK(WideFromUtf8(expected) == wide);
            CHECK(WideFromSjis(SjisFromWide(wide)) == wide);
            if (mode == 0) {
                CHECK(AnsiFromWide(wide) == expected);
                CHECK(WideFromAnsi(expected) == wide);
            }
        }
    }

    // 全て非ASCII(出力が 1024 バイトのスタックバッファを超える)
    {
        std::wstring wide(3000, L'\u3042');
        std::string expected;
        for (int i = 0; i < 3000; ++i)
            expected += UTF8_A;
        CHECK(Utf8FromWide(wide) == expected);
        CHECK(WideFromUtf8(expected) == wide);
    }
}

//////////////////////////////////////////////////////////////////
// Tests: Macros (UNICODE / ANSI)

static void TestMacros() {
    // ASCII only: どちらのビルドでも結果が変わらない
    CHECK(Utf8FromText(TEXT("abc")) == "abc");
    CHECK(TextFromUtf8("abc") == TextFromText(TEXT("abc")));
    CHECK(WideFromText(TEXT("abc")) == L"abc");
    CHECK(AnsiFromText(TEXT("abc")) == "abc");

#ifdef UNICODE
    CHECK(Utf8FromText(TextFromUtf8(std::string(UTF8_A))) == UTF8_A);
    CHECK(TextFromWide(L"\u3042") == L"\u3042");
#else
    CHECK(TextFromWide(L"abc") == "abc");
#endif
}

//////////////////////////////////////////////////////////////////
// Tests: Output (redirected to a file => UTF-8)

static void TestOutput() {
    const std::string path = MakeTempPath();

    // FilePrintfW
    {
        FILE* fp = std::fopen(path.c_str(), "wb");
        CHECK(fp != nullptr);
        if (fp) {
            FilePrintfW(fp, L"%ls=%d\n", L"\u3042", 42);
            std::fclose(fp);
        }
        CHECK(ReadBytes(path) == std::string(UTF8_A) + "=42\n");
    }

    // FilePrintfA (ASCII)
    {
        FILE* fp = std::fopen(path.c_str(), "wb");
        CHECK(fp != nullptr);
        if (fp) {
            FilePrintfA(fp, "hello %s %d", "world", 123);
            std::fclose(fp);
        }
        CHECK(ReadBytes(path) == "hello world 123");
    }

    // FilePutStringW / FilePutStringA
    {
        FILE* fp = std::fopen(path.c_str(), "wb");
        CHECK(fp != nullptr);
        if (fp) {
            FilePutStringW(L"\u3044", fp);
            FilePutStringA("-", fp);
            FilePutStringW(L"\U0001F600", fp);
            std::fclose(fp);
        }
        CHECK(ReadBytes(path) == std::string(UTF8_I) + "-" + UTF8_EMOJI);
    }

    // 1024文字を超える長い文字列が切り詰められないこと
    {
        const std::wstring big(5000, L'x');
        FILE* fp = std::fopen(path.c_str(), "wb");
        CHECK(fp != nullptr);
        if (fp) {
            FilePrintfW(fp, L"%ls", big.c_str());
            std::fclose(fp);
        }
        CHECK(ReadBytes(path) == std::string(5000, 'x'));
    }

    // NULL を渡してもクラッシュしない
    {
        FilePutStringA(nullptr, stdout);
        FilePutStringW(nullptr, stdout);
        CHECK(true);
    }

    // DebugPrintf (クラッシュしないことの確認)
    {
        DebugPrintfA("conuni_test: %s %d\n", "debug", 1);
        DebugPrintfW(L"conuni_test: %ls %d\n", L"debug \u3042", 2);
        CHECK(true);
    }

    std::remove(path.c_str());
}

//////////////////////////////////////////////////////////////////
// Tests: Input (redirected from a file => UTF-8)

static void TestInput() {
    const std::string path = MakeTempPath();

    // UTF-8 BOM, CRLF, LF, 末尾に改行なし
    {
        const std::string content =
            std::string("\xEF\xBB\xBF") + "line1\r\n" + UTF8_A + "\n" + "last";
        CHECK(WriteBytes(path, content));

        FILE* fp = std::fopen(path.c_str(), "rb");
        CHECK(fp != nullptr);
        if (fp) {
            std::wstring line;
            CHECK(FileGetLineW(line, fp));
            CHECK(line == L"line1");
            CHECK(FileGetLineW(line, fp));
            CHECK(line == L"\u3042");
            CHECK(FileGetLineW(line, fp));
            CHECK(line == L"last");
            CHECK(!FileGetLineW(line, fp)); // EOF
            CHECK(line.empty());
            std::fclose(fp);
        }
    }

    // 空行
    {
        CHECK(WriteBytes(path, "\n\nx\n"));
        FILE* fp = std::fopen(path.c_str(), "rb");
        CHECK(fp != nullptr);
        if (fp) {
            std::wstring line;
            CHECK(FileGetLineW(line, fp));
            CHECK(line.empty());
            CHECK(FileGetLineW(line, fp));
            CHECK(line.empty());
            CHECK(FileGetLineW(line, fp));
            CHECK(line == L"x");
            CHECK(!FileGetLineW(line, fp));
            std::fclose(fp);
        }
    }

    // 内部バッファ(512バイト)より長い行
    {
        const std::string longLine(1500, 'y');
        CHECK(WriteBytes(path, longLine + "\r\n" + "next\n"));
        FILE* fp = std::fopen(path.c_str(), "rb");
        CHECK(fp != nullptr);
        if (fp) {
            std::string line;
            CHECK(FileGetLineA(line, fp));
            CHECK(line == longLine);
            CHECK(FileGetLineA(line, fp));
            CHECK(line == "next");
            CHECK(!FileGetLineA(line, fp));
            std::fclose(fp);
        }
    }

    // 内部バッファ(4096バイト)の境界前後の長さの行
    {
        const size_t sizes[] = { 4093, 4094, 4095, 4096, 4097 };
        for (size_t size : sizes) {
            const std::string longLine(size, 'q');
            CHECK(WriteBytes(path, longLine + "\r\n" + "next\n"));
            FILE* fp = std::fopen(path.c_str(), "rb");
            CHECK(fp != nullptr);
            if (fp) {
                std::string line;
                CHECK(FileGetLineA(line, fp));
                CHECK(line == longLine);
                CHECK(FileGetLineA(line, fp));
                CHECK(line == "next");
                CHECK(!FileGetLineA(line, fp));
                std::fclose(fp);
            }
        }
    }

    // 4096バイトを超える非ASCIIの行
    {
        std::string longUtf8;
        std::wstring longWide;
        for (int i = 0; i < 3000; ++i) {
            longUtf8 += UTF8_A;
            longWide += L'\u3042';
        }
        CHECK(WriteBytes(path, longUtf8 + "\r\n" + "z\n"));
        FILE* fp = std::fopen(path.c_str(), "rb");
        CHECK(fp != nullptr);
        if (fp) {
            std::wstring line;
            CHECK(FileGetLineW(line, fp));
            CHECK(line == longWide);
            CHECK(FileGetLineW(line, fp));
            CHECK(line == L"z");
            CHECK(!FileGetLineW(line, fp));
            std::fclose(fp);
        }
    }

    // サロゲートペア
    {
        CHECK(WriteBytes(path, std::string(UTF8_EMOJI) + "\n"));
        FILE* fp = std::fopen(path.c_str(), "rb");
        CHECK(fp != nullptr);
        if (fp) {
            std::wstring line;
            CHECK(FileGetLineW(line, fp));
            CHECK(line == L"\U0001F600");
            std::fclose(fp);
        }
    }

    // 空ファイル
    {
        CHECK(WriteBytes(path, ""));
        FILE* fp = std::fopen(path.c_str(), "rb");
        CHECK(fp != nullptr);
        if (fp) {
            std::wstring line;
            CHECK(!FileGetLineW(line, fp));
            std::fclose(fp);
        }
    }

    // NULL
    {
        std::wstring line;
        CHECK(!FileGetLineW(line, nullptr));
    }

    std::remove(path.c_str());
}

//////////////////////////////////////////////////////////////////

int main() {
    TestConversion();
    TestConversionBoundaries();
    TestMacros();
    TestOutput();
    TestInput();

    std::printf("%d / %d checks passed\n", g_nTotal - g_nFailed, g_nTotal);
    return (g_nFailed == 0) ? 0 : 1;
}
