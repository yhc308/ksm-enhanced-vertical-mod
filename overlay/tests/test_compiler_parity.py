from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
import unittest
from compiler_parity import restore, compare_files, tokens, signature


class CompilerParityTests(unittest.TestCase):
    def test_whole_function_restore(self):
        old = b'int value() { return 16; }\n'
        new = b'int value()\n{\n\treturn 16;\n}\n'
        actual, report = restore(old, new)
        self.assertEqual(actual, old)
        self.assertEqual(report['mode'], 'whole-file')

    def test_retains_changed_function(self):
        old = b'int a() { return 1; }\nint b() {return 2;}'
        new = b'int a()\n{\n return 1;\n}\nint b() {return 3;}'
        result, report = restore(old, new)
        self.assertIn(b'return 3;', result)
        self.assertEqual(report['bodies'], 1)
        self.assertEqual(compare_files(result, new), 'compiler-token-identical')

    def test_changes_are_detected(self):
        for before, after in [(b'1', b'2'), (b'"hello"', b'"world"'), (b'a+b', b'a-b'),
                              (b'f(); g();', b'g(); f();'), (b'0x10', b'16'),
                              (b'//copyright\nint a;', b'//other\nint a;')]:
            with self.subTest(before=before):
                with self.assertRaises(AssertionError):
                    compare_files(before, after)

    def test_macros_preserve_meaningful_whitespace(self):
        a = b'#define F(x) x\nint x;'
        b = b'#define F (x) x\nint x;'
        self.assertEqual(signature(tokens(a)[1]), signature(tokens(b)[1]))
        with self.assertRaises(AssertionError):
            compare_files(a, b)
        self.assertEqual(restore(a, b)[0], b)

    def test_continuation_lines(self):
        a = b'#define F(x) \\\n ((x)+1)\nint x;'
        b = b'#define F(x) \\\n((x)+1)\nint x;'
        with self.assertRaises(AssertionError):
            compare_files(a, b)

    def test_inactive_preprocessor_tokens(self):
        with self.assertRaises(AssertionError):
            compare_files(b'#if 0\nint x=1;\n#endif', b'#if 0\nint x=2;\n#endif')

    def test_raw_string_and_comments_are_not_stripped(self):
        for a, b in [(b'auto s=R"(a b)";', b'auto s=R"(ab)";'),
                     (b'auto s="// hi";', b'auto s="//hi";'),
                     (b'/* a b */ int a;', b'/* ab */ int a;')]:
            with self.assertRaises(AssertionError):
                compare_files(a, b)

    def test_line_sensitive_file_is_retained(self):
        a = b'int a=__LINE__;\nint f(){return 1;}'
        b = b'int a=__LINE__;\n\nint f() { return 1; }'
        self.assertEqual(restore(a, b)[0], b)

    def test_translation_phase_line_endings(self):
        self.assertEqual(signature(tokens(b'/* a\r\nb */ int x;')[1]),
                         signature(tokens(b'/* a\nb */ int x;')[1]))

    def test_bom_and_unicode(self):
        a = '\ufeffint f(){return U"판정";}\n'.encode()
        b = '\ufeffint f()\n{\n return U"판정";\n}\n'.encode()
        result, _ = restore(a, b)
        self.assertEqual(result, a)

    def test_lambda_and_control_bodies_are_not_candidates(self):
        from compiler_parity import function_bodies
        for code in (b'if(x){foo();}', b'while(x){foo();}', b'auto x=[](){foo();};'):
            self.assertEqual(function_bodies(tokens(code)[1]), {})


if __name__ == '__main__':
    unittest.main()
