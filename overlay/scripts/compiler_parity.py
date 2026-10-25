"""Conservative formatting restoration with the Clang 18 compiler lexer.

Comments are retained. Directives must remain byte-identical (apart from CRLF).
Files using location-sensitive builtins are never rewritten. This deliberately
does not claim an AST equivalence proof: every resulting full file is re-lexed,
and builds and executable regression tests are separate required gates.
"""
from pathlib import Path
import hashlib
import json
import os
import sys
sys.path.insert(0, os.environ.get("REVIEW_LIBCLANG", str(Path(__file__).resolve().parents[1] / "tools/review-libclang-18.1.1")))
from clang.cindex import Index, TranslationUnit

INDEX = Index.create()
CPP = {".cpp", ".hpp", ".h"}


def tokens(data):
    text = data.decode("utf-8-sig").replace("\r\n", "\n")
    # An unsaved translation unit avoids opening project headers or SDKs.
    unit = INDEX.parse("review-input.cpp", args=["-x", "c++", "-std=c++20", "--target=x86_64-pc-windows-msvc",
                       "-fms-extensions", "-fms-compatibility", "-nostdinc", "-nostdinc++"],
                       unsaved_files=[("review-input.cpp", text)],
                       options=TranslationUnit.PARSE_INCOMPLETE | TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
    result = [(token.kind.value, token.spelling, token.extent.start.offset, token.extent.end.offset)
              for token in unit.get_tokens(extent=unit.cursor.extent)]
    return text.encode("utf-8"), result


def signature(items):
    return [(item[0], item[1]) for item in items]


def directives(data):
    lines = data.replace(b"\r\n", b"\n").splitlines(keepends=True)
    result = []
    active = False
    for line in lines:
        if active or line.lstrip().startswith(b"#"):
            result.append(line)
            active = line.rstrip(b"\n").endswith(b"\\")
    return b"".join(result)


def function_bodies(items):
    """Only balanced complete bodies with an unambiguous function-like head.

    Constructors with initializer lists, operators, macros and lambdas are
    intentionally skipped rather than guessed. Matching includes the header.
    """
    stack = []
    pairs = {}
    for i, token in enumerate(items):
        if token[1] in ("{", "(", "["):
            stack.append((token[1], i))
        elif token[1] in ("}", ")", "]"):
            expected = {"}": "{", ")": "(", "]": "["}[token[1]]
            if stack and stack[-1][0] == expected:
                _, start = stack.pop()
                pairs[i] = start
                pairs[start] = i
    bodies = {}
    for opening, closing in pairs.items():
        if opening >= closing or items[opening][1] != "{":
            continue
        j = opening - 1
        while j >= 0 and items[j][1] in ("const", "noexcept", "override", "final", "&", "&&"):
            j -= 1
        if j < 0 or items[j][1] != ")" or j not in pairs:
            continue
        parameter_start = pairs[j]
        if parameter_start < 1:
            continue
        name = items[parameter_start - 1][1]
        if not name.isidentifier() or name in {"if", "while", "for", "switch", "catch", "decltype", "sizeof", "alignof", "requires"}:
            continue
        head = parameter_start - 1
        while head > 0 and items[head - 1][1] not in (";", "{", "}"):
            head -= 1
        header = items[head:opening]
        if any(t[1] in ("#", "[", "]", "define", "operator") for t in header):
            continue
        key = tuple(signature(items[head:closing + 1]))
        bodies.setdefault(key, []).append((items[opening][2], items[closing][3]))
    return bodies


def restore(original, current):
    old_raw, old = tokens(original)
    new_raw, new = tokens(current)
    if any(t[1] in ("__LINE__", "__FILE__", "__builtin_LINE", "__builtin_FILE") for t in old + new):
        return current, {"mode": "retained-location-sensitive"}
    if signature(old) == signature(new) and directives(old_raw) == directives(new_raw):
        return original, {"mode": "whole-file", "bodies": 0}
    old_bodies = function_bodies(old)
    new_bodies = function_bodies(new)
    replacements = []
    for key, spans in new_bodies.items():
        previous = old_bodies.get(key, [])
        if len(spans) != 1 or len(previous) != 1:
            continue
        start, end = spans[0]
        a, b = previous[0]
        target = old_raw[a:b]
        if target != new_raw[start:end] and directives(target) == directives(new_raw[start:end]):
            replacements.append((start, end, target))
    # Prefer complete outer function bodies; never apply overlapping rewrites.
    selected = []
    for item in sorted(replacements, key=lambda x: (x[0], -x[1])):
        if not selected or item[0] >= selected[-1][1]:
            selected.append(item)
    result = new_raw
    for start, end, target in reversed(selected):
        result = result[:start] + target + result[end:]
    _, actual = tokens(result)
    assert signature(actual) == signature(new), "Compiler token mismatch after restoration"
    assert directives(result) == directives(new_raw), "Preprocessor directive changed"
    if current.startswith(b"\xef\xbb\xbf"):
        result = b"\xef\xbb\xbf" + result
    return result, {"mode": "functions" if selected else "retained", "bodies": len(selected)}


def compare_files(reference, candidate):
    if reference == candidate:
        return "byte-identical"
    a, ta = tokens(reference)
    b, tb = tokens(candidate)
    assert signature(ta) == signature(tb), "Compiler token mismatch"
    assert directives(a) == directives(b), "Preprocessor directive mismatch"
    return "compiler-token-identical"
