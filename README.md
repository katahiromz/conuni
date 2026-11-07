# conuni

**Console Unicode Input/Output for C++/Win32**

Windows のコンソールで、コードページ(`chcp`)を気にせずに Unicode 文字列を入出力するための小さな C++ ライブラリです。

- 出力先が**コンソール**なら `WriteConsoleW` で出力(日本語・絵文字もそのまま表示)
- 出力先が**リダイレクト/ファイル/パイプ**なら **UTF-8** で出力
- 入力元が**コンソール**なら `ReadConsoleW` で読み取り
- 入力元が**リダイレクト/ファイル/パイプ**なら **UTF-8** として読み取り
- `printf` 風の書式付き出力(長さ制限なし)
- ANSI / Wide / UTF-8 / Shift_JIS の変換関数
- `UNICODE` の有無に応じて切り替わる `TCHAR` 風マクロ

ファイルは `conuni.h` と `conuni.cpp` の 2 つだけなので、プロジェクトにコピーするだけで使えます。

## 動作環境

- Windows
- C++11 以降
- MSVC / MinGW-w64

## ビルド

CMake を使う場合:

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

テストを無効にするには `-DCONUNI_BUILD_TESTS=OFF` を指定してください。

自分のプロジェクトから使う場合:

```cmake
add_subdirectory(conuni)
target_link_libraries(your_app PRIVATE conuni)
```

CMake を使わない場合は、`conuni.cpp` をプロジェクトに追加して `conuni.h` をインクルードしてください。

## 使い方

```cpp
#include "conuni.h"

int main() {
    PrintfW(L"名前を入力: ");

    std::wstring name;
    if (GetLineW(name))
        PrintfW(L"こんにちは、%ls さん!\n", name.c_str());

    FilePrintfW(stderr, L"エラー: %ls\n", L"何か問題が起きました");
    return 0;
}
```

リダイレクトしても文字化けしません(出力は UTF-8、入力も UTF-8 として扱われます)。

```sh
app.exe > out.txt          # out.txt は UTF-8
app.exe < in.txt           # in.txt は UTF-8 として読み取り
```

## API

### 変換

| 関数 / マクロ | 説明 |
| --- | --- |
| `WideFromAnsi(str, codePage = CP_ACP)` | マルチバイト → UTF-16 |
| `AnsiFromWide(wstr, codePage = CP_ACP)` | UTF-16 → マルチバイト |
| `Utf8FromWide(wstr)` / `WideFromUtf8(str)` | UTF-8 との相互変換 |
| `SjisFromWide(wstr)` / `WideFromSjis(str)` | Shift_JIS(CP932)との相互変換 |
| `AnsiFromUtf8(str)` / `Utf8FromAnsi(str)` | ANSI(CP_ACP)と UTF-8 の相互変換 |

変換に失敗した場合、または入力が空の場合は空文字列を返します。

### 出力

| 関数 | 説明 |
| --- | --- |
| `PrintfA` / `PrintfW` | 標準出力へ書式付き出力 |
| `FilePrintfA` / `FilePrintfW` | 指定した `FILE*` へ書式付き出力 |
| `FileVPrintfA` / `FileVPrintfW` | `va_list` 版 |
| `FilePutStringA` / `FilePutStringW` | 指定した `FILE*` へ文字列を出力(改行は付きません) |
| `DebugPrintfA` / `DebugPrintfW` | `OutputDebugString` へ書式付き出力 |
| `DebugVPrintfA` / `DebugVPrintfW` | `va_list` 版 |

`A` 版の引数は ANSI(`CP_ACP`)の文字列、`W` 版の引数は UTF-16 の文字列として扱われます。

### 入力

| 関数 | 説明 |
| --- | --- |
| `GetLineA(std::string&)` / `GetLineW(std::wstring&)` | 標準入力から 1 行読み取る |
| `FileGetLineA(std::string&, FILE*)` / `FileGetLineW(std::wstring&, FILE*)` | 指定した `FILE*` から 1 行読み取る |

- 末尾の改行(`\r\n` / `\n`)は取り除かれます。
- 何も読み取れなかった場合(EOF、コンソールでの Ctrl+Z)は `false` を返します。
- リダイレクト入力の先頭に UTF-8 BOM があれば取り除きます。
- `A` 版は読み取った文字列を `CP_ACP` に変換して返します。

### TCHAR 風マクロ

`UNICODE` が定義されていれば `W` 版、そうでなければ `A` 版に展開されます。

`Printf`, `FilePrintf`, `FileVPrintf`, `FilePutString`, `DebugPrintf`, `DebugVPrintf`, `GetLine`, `FileGetLine`,
`TextFromAnsi`, `TextFromWide`, `TextFromUtf8`, `AnsiFromText`, `WideFromText`, `Utf8FromText`, `TextFromText`

## 注意事項

- `PrintfW` などのフォーマット文字列で、ワイド文字列を渡すときは MSVC でも MinGW でも動く `%ls` を使ってください(`%s` は `PrintfA` では ANSI、`PrintfW` では環境により解釈が異なります)。
- コンソール出力の前に `fflush` を行い、`stdio` のバッファとの順序を保ちます。ただし、同じストリームに対して `printf` / `std::cout` と混在させるのは避けてください。
- ファイルへ出力するときに UTF-8 の BOM は付きません。
- コンソールかどうかは `GetConsoleMode` で判定しているため、`mintty` などの疑似端末ではパイプ扱い(UTF-8 出力)になります。

## テスト

`conuni_test.cpp` に、変換・出力(ファイル)・入力(ファイル)・マクロのテストがあります。
ANSI ビルド(`conuni_test_ansi`)と UNICODE ビルド(`conuni_test_unicode`)の両方が `ctest` で実行されます。

コンソールへの直接入出力は自動テストできないため、手動で確認してください。

## ライセンス

MIT License

Copyright (c) katahiromz
