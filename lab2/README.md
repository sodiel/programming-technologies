# Lab 2 — C++ Regular Expressions

## Requirements

- C++23 compiler
- CMake 3.23 or newer

## Build

From the repository root:

```bash
cmake -S . -B build
cmake --build build --target lab2
```

## Commands

```bash
build/lab2/lab2 check -f lab2/data/example.md
build/lab2/lab2 check "# Заголовок"
build/lab2/lab2 find -f lab2/data/example.md h1
build/lab2/lab2 find -f lab2/data/example.md paragraph
build/lab2/lab2 find -f lab2/data/example.md list
build/lab2/lab2 form
build/lab2/lab2 generate 10 42
```

## Markdown behavior

The checker recognizes ATX headings (`#` through `######`), paragraphs,
simple ordered and unordered lists, and fenced code blocks. File input is read
line by line. `check -f` counts blocks without retaining their text. `find`
also scans without retaining the full document and writes matching lines as
`line-number: text` directly to `find-result.txt` in the current directory.
The file is overwritten on each `find` run. The terminal reports how many
matching blocks were written.

The checker reports obvious HTML document and AsciiDoc markers. It implements
the lab's Markdown subset rather than the complete CommonMark/GFM specification.
It checks marker characters directly to keep large-file scanning fast; the
questionnaire uses `std::regex` for date, phone, and email formats. Cyrillic FIO
normalization decodes Russian UTF-8 letters directly. No Boost dependency is
required.

## Questionnaire and generator

The questionnaire normalizes Russian FIO, supported date formats, Russian
phone numbers (`+7`, `+8`, or national `8` prefix, with common separators), and
ASCII email addresses. A `+8` phone input is normalized to the canonical `+7`
prefix. The generator reads complete FIO records from
`data/data.csv` using a streaming reservoir sample, so the 338 MB dictionary is
not loaded into memory. `data/SOURCE.txt` records the dictionary source.

Passing a seed to `generate` makes its output repeatable.
