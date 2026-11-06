// conuni.cpp --- Console Unicode Input/Output for C++/Win32
// Author: katahiromz
// License: MIT

#include "conuni.h"
#include <io.h>
#include <cstdlib>
#include <cstring>

//////////////////////////////////////////////////////////////////
// Conversion

std::wstring WideFromAnsi(const std::string& ansi, UINT nCodePage) {
    if (ansi.empty())
        return std::wstring();

    INT cch = ::MultiByteToWideChar(nCodePage, 0,
                                    ansi.data(), static_cast<int>(ansi.size()),
                                    nullptr, 0);
    if (cch <= 0)
        return std::wstring();

    std::wstring wide(size_t(cch), UNICODE_NULL);
    int result = ::MultiByteToWideChar(nCodePage, 0,
                                       ansi.data(), static_cast<int>(ansi.size()),
                                       &wide[0], cch);
    if (result <= 0)
        return std::wstring();

    return wide;
}

std::string AnsiFromWide(const std::wstring& wide, UINT nCodePage) {
    if (wide.empty())
        return std::string();

    INT cb = ::WideCharToMultiByte(nCodePage, 0,
                                   wide.data(), static_cast<int>(wide.size()),
                                   nullptr, 0, nullptr, nullptr);
    if (cb <= 0)
        return std::string();

    std::string ansi(size_t(cb), ANSI_NULL);
    int result = ::WideCharToMultiByte(nCodePage, 0,
                                       wide.data(), static_cast<int>(wide.size()),
                                       &ansi[0], cb, nullptr, nullptr);
    if (result <= 0)
        return std::string();

    return ansi;
}

//////////////////////////////////////////////////////////////////
// Internal helpers

// FILE* がコンソールに接続されていれば、そのハンドルを返す。
// (_isatty は NUL デバイスも true にするため、GetConsoleMode で判定する)
static bool GetConsoleHandleOf(FILE* fp, HANDLE& hConsole) {
    if (!fp)
        return false;

    int no = _fileno(fp);
    if (no < 0)
        return false;

    intptr_t h = _get_osfhandle(no);
    if (h == -1)
        return false;

    DWORD dwMode;
    if (!::GetConsoleMode(reinterpret_cast<HANDLE>(h), &dwMode))
        return false;

    hConsole = reinterpret_cast<HANDLE>(h);
    return true;
}

// ワイド文字列を出力する。コンソールなら WriteConsoleW、それ以外は UTF-8。
static void PutWide(const std::wstring& wide, FILE* fout) {
    if (!fout || wide.empty())
        return;

    HANDLE hConsole;
    if (GetConsoleHandleOf(fout, hConsole)) {
        fflush(fout); // stdio のバッファとの順序を保つ
        const size_t chunk = 8192;
        for (size_t ich = 0; ich < wide.size(); ) {
            size_t n = wide.size() - ich;
            if (n > chunk)
                n = chunk;
            // サロゲートペアを分断しない
            if (n < wide.size() - ich && wide[ich + n - 1] >= 0xD800 && wide[ich + n - 1] <= 0xDBFF)
                --n;
            DWORD cchWritten = 0;
            if (!::WriteConsoleW(hConsole, wide.data() + ich, static_cast<DWORD>(n),
                                 &cchWritten, nullptr) || cchWritten == 0)
                break;
            ich += cchWritten;
        }
    } else {
        std::string utf8 = Utf8FromWide(wide);
        fwrite(utf8.data(), 1, utf8.size(), fout);
    }
}

static std::string FormatV(LPCSTR pszFormat, va_list va) {
    if (!pszFormat)
        return std::string();

    va_list va2;
    va_copy(va2, va);
    int len = _vscprintf(pszFormat, va2);
    va_end(va2);
    if (len < 0)
        return std::string();

    std::string text(size_t(len) + 1, '\0');
    StringCchVPrintfA(&text[0], text.size(), pszFormat, va);
    text.resize(strlen(text.c_str()));
    return text;
}

static std::wstring FormatV(LPCWSTR pszFormat, va_list va) {
    if (!pszFormat)
        return std::wstring();

    va_list va2;
    va_copy(va2, va);
    int len = _vscwprintf(pszFormat, va2);
    va_end(va2);
    if (len < 0)
        return std::wstring();

    std::wstring text(size_t(len) + 1, L'\0');
    StringCchVPrintfW(&text[0], text.size(), pszFormat, va);
    text.resize(wcslen(text.c_str()));
    return text;
}

//////////////////////////////////////////////////////////////////
// Output

void FilePutStringA(LPCSTR pszString, FILE* fout) {
    if (!pszString)
        return;
    PutWide(WideFromAnsi(pszString), fout);
}

void FilePutStringW(LPCWSTR pszString, FILE* fout) {
    if (!pszString)
        return;
    PutWide(std::wstring(pszString), fout);
}

void FileVPrintfA(FILE* fout, LPCSTR pszFormat, va_list va) {
    FilePutStringA(FormatV(pszFormat, va).c_str(), fout);
}

void FileVPrintfW(FILE* fout, LPCWSTR pszFormat, va_list va) {
    FilePutStringW(FormatV(pszFormat, va).c_str(), fout);
}

void FilePrintfA(FILE* fout, LPCSTR pszFormat, ...) {
    va_list va;
    va_start(va, pszFormat);
    FileVPrintfA(fout, pszFormat, va);
    va_end(va);
}

void FilePrintfW(FILE* fout, LPCWSTR pszFormat, ...) {
    va_list va;
    va_start(va, pszFormat);
    FileVPrintfW(fout, pszFormat, va);
    va_end(va);
}

void PrintfA(LPCSTR pszFormat, ...) {
    va_list va;
    va_start(va, pszFormat);
    FileVPrintfA(stdout, pszFormat, va);
    va_end(va);
}

void PrintfW(LPCWSTR pszFormat, ...) {
    va_list va;
    va_start(va, pszFormat);
    FileVPrintfW(stdout, pszFormat, va);
    va_end(va);
}

void DebugVPrintfA(LPCSTR pszFormat, va_list va) {
    ::OutputDebugStringA(FormatV(pszFormat, va).c_str());
}

void DebugVPrintfW(LPCWSTR pszFormat, va_list va) {
    ::OutputDebugStringW(FormatV(pszFormat, va).c_str());
}

void DebugPrintfA(LPCSTR pszFormat, ...) {
    va_list va;
    va_start(va, pszFormat);
    DebugVPrintfA(pszFormat, va);
    va_end(va);
}

void DebugPrintfW(LPCWSTR pszFormat, ...) {
    va_list va;
    va_start(va, pszFormat);
    DebugVPrintfW(pszFormat, va);
    va_end(va);
}

//////////////////////////////////////////////////////////////////
// Input

template <typename T_STR>
static void StripNewLine(T_STR& str) {
    while (!str.empty() && (str.back() == '\n' || str.back() == '\r'))
        str.pop_back();
}

bool FileGetLineW(std::wstring& line, FILE* fin) {
    line.clear();
    if (!fin)
        return false;

    HANDLE hConsole;
    if (GetConsoleHandleOf(fin, hConsole)) { // コンソール入力
        bool bGot = false;
        WCHAR szBuf[256];
        for (;;) {
            DWORD cchRead = 0;
            if (!::ReadConsoleW(hConsole, szBuf, _countof(szBuf), &cchRead, nullptr) ||
                cchRead == 0)
            {
                break;
            }
            bGot = true;
            line.append(szBuf, cchRead);
            if (line.back() == L'\n')
                break;
        }
        if (!bGot)
            return false;

        if (line[0] == 0x1A) { // Ctrl+Z (EOF)
            line.clear();
            return false;
        }
        StripNewLine(line);
        return true;
    }

    // リダイレクト/ファイル入力 (UTF-8)
    std::string bytes;
    bool bGot = false;
    char szBuf[512];
    while (fgets(szBuf, _countof(szBuf), fin)) {
        bGot = true;
        bytes += szBuf;
        if (bytes.back() == '\n')
            break;
    }
    if (!bGot)
        return false;

    StripNewLine(bytes);
    if (bytes.size() >= 3 && memcmp(bytes.data(), "\xEF\xBB\xBF", 3) == 0) // UTF-8 BOM
        bytes.erase(0, 3);

    line = WideFromUtf8(bytes);
    return true;
}

bool FileGetLineA(std::string& line, FILE* fin) {
    std::wstring wide;
    bool ret = FileGetLineW(wide, fin);
    line = AnsiFromWide(wide);
    return ret;
}

bool GetLineA(std::string& line) {
    return FileGetLineA(line, stdin);
}

bool GetLineW(std::wstring& line) {
    return FileGetLineW(line, stdin);
}
