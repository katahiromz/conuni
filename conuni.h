// conuni.h --- Console Unicode Input/Output for C++/Win32
// Author: katahiromz
// License: MIT

#pragma once

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <cstdio>
#include <cstdarg>
#include <string>

//////////////////////////////////////////////////////////////////
// Conversion

#ifndef CP_SJIS
    #define CP_SJIS 932
#endif

std::wstring WideFromAnsi(const std::string& ansi , UINT nCodePage = CP_ACP);
std::string  AnsiFromWide(const std::wstring& wide, UINT nCodePage = CP_ACP);
#define Utf8FromWide(wide) AnsiFromWide((wide), CP_UTF8)
#define SJISFromWide(wide) AnsiFromWide((wide), CP_SJIS)
#define WideFromUtf8(utf8) WideFromAnsi((utf8), CP_UTF8)
#define WideFromSJIS(sjis) WideFromAnsi((sjis), CP_SJIS)

#define AnsiFromAnsi(ansi) std::string(ansi)
#define WideFromWide(wide) std::wstring(wide)
#define Utf8FromUtf8(utf8) std::string(utf8)

// ANSI(CP_ACP) <-> UTF-8 (via Wide)
#define AnsiFromUtf8(utf8) AnsiFromWide(WideFromUtf8(utf8))
#define Utf8FromAnsi(ansi) Utf8FromWide(WideFromAnsi(ansi))

#ifdef UNICODE
    #define TextFromAnsi WideFromAnsi
    #define TextFromWide WideFromWide
    #define TextFromUtf8 WideFromUtf8
    #define AnsiFromText AnsiFromWide
    #define WideFromText WideFromWide
    #define Utf8FromText Utf8FromWide
    #define TextFromText WideFromWide
#else
    #define TextFromAnsi AnsiFromAnsi
    #define TextFromWide AnsiFromWide
    #define TextFromUtf8 AnsiFromUtf8
    #define AnsiFromText AnsiFromAnsi
    #define WideFromText WideFromAnsi
    #define Utf8FromText Utf8FromAnsi
    #define TextFromText AnsiFromAnsi
#endif

//////////////////////////////////////////////////////////////////
// Output
//
// - 出力先がコンソールなら WriteConsoleW で出力する(コードページに依存しない)。
// - リダイレクト/ファイル出力の場合は UTF-8 で出力する。

void PrintfA(LPCSTR  pszFormat, ...);
void PrintfW(LPCWSTR pszFormat, ...);

void DebugPrintfA(LPCSTR  pszFormat, ...);
void DebugPrintfW(LPCWSTR pszFormat, ...);

void FilePutStringA(LPCSTR  pszString, FILE* fout);
void FilePutStringW(LPCWSTR pszString, FILE* fout);

void FilePrintfA(FILE* fout, LPCSTR  pszFormat, ...);
void FilePrintfW(FILE* fout, LPCWSTR pszFormat, ...);

void FileVPrintfA(FILE* fout, LPCSTR  pszFormat, va_list va);
void FileVPrintfW(FILE* fout, LPCWSTR pszFormat, va_list va);

void DebugVPrintfA(LPCSTR  pszFormat, va_list va);
void DebugVPrintfW(LPCWSTR pszFormat, va_list va);

#ifdef UNICODE
    #define Printf PrintfW
    #define DebugPrintf DebugPrintfW
    #define DebugVPrintf DebugVPrintfW
    #define FilePutString FilePutStringW
    #define FilePrintf FilePrintfW
    #define FileVPrintf FileVPrintfW
#else
    #define Printf PrintfA
    #define DebugPrintf DebugPrintfA
    #define DebugVPrintf DebugVPrintfA
    #define FilePutString FilePutStringA
    #define FilePrintf FilePrintfA
    #define FileVPrintf FileVPrintfA
#endif

#ifdef NDEBUG
    #define DPRINTA
    #define DPRINTW
#else
    #define DPRINTA DebugPrintfA
    #define DPRINTW DebugPrintfW
#endif

//////////////////////////////////////////////////////////////////
// Input
//
// - 入力元がコンソールなら ReadConsoleW で読み取る。
// - リダイレクト/ファイル入力の場合は UTF-8 として読み取る。
// - 1行を読み取り、末尾の改行(\r\n, \n)は取り除く。
// - EOF(何も読めなかった場合)は false を返す。

bool FileGetLineA(std::string&  line, FILE* fin);
bool FileGetLineW(std::wstring& line, FILE* fin);

bool GetLineA(std::string&  line);
bool GetLineW(std::wstring& line);

#ifdef UNICODE
    #define FileGetLine FileGetLineW
    #define GetLine GetLineW
#else
    #define FileGetLine FileGetLineA
    #define GetLine GetLineA
#endif
