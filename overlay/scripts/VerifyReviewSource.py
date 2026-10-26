"""Verify full review source against frozen d7fa0f0 compiler-token expectations."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import sys
from compiler_parity import tokens, signature, directives, CPP

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--report', type=Path, default=root / 'build/review-parity.json')
args = parser.parse_args()
reference = json.loads((root / 'docs/review/D7_SOURCE_REFERENCE.json').read_text())
target = json.loads((root / 'docs/review/CORE_TARGET.json').read_text())

def git(*parts):
    return subprocess.check_output(['git', '-c', 'safe.directory=' + root.as_posix(), '-C', str(root), *parts], text=True).strip()

assert git('merge-base', '--is-ancestor', target['coreCommit'], 'HEAD') == ''
assert not git('diff', target['coreCommit'], 'HEAD', '--', 'kshootmania', 'ksmaudio', 'kson', 'ksmaxis', 'patches/dependencies'), 'Test branch changes game files'
actual = {p.relative_to(root).as_posix() for p in (root / 'kshootmania/src').rglob('*') if p.suffix in CPP}
assert actual == set(reference['gameCpp']), 'Game C++ inventory mismatch'
assert len(actual) == 367
for group in ('gameCpp', 'dependencyCpp'):
    for name, expected in reference[group].items():
        data, parsed = tokens((root / name).read_bytes())
        digest = hashlib.sha256(json.dumps(signature(parsed), ensure_ascii=True, separators=(',', ':')).encode()).hexdigest()
        assert digest == expected['tokens'], 'Compiler token mismatch: ' + name
        assert hashlib.sha256(directives(data)).hexdigest() == expected['directives'], 'Directive mismatch: ' + name
for name, expected in reference['runtimeAssets'].items():
    data = (root / name).read_bytes()
    if expected['mode'] == 'text-lf':
        data = data.replace(b'\r\n', b'\n')
    assert hashlib.sha256(data).hexdigest() == expected['sha256'], 'Runtime resource mismatch: ' + name
deleted = git('diff', '--diff-filter=D', '--name-only', reference['upstreamCommit'], 'HEAD', '--', 'kshootmania/src')
assert not deleted, 'Original source deletion: ' + deleted
report = {'status': 'PASS', 'testedCommit': git('rev-parse', 'HEAD'), 'coreCommit': target['coreCommit'],
          'referenceModCommit': reference['referenceModCommit'], 'gameCppFiles': len(actual),
          'dependencyCppFiles': len(reference['dependencyCpp']), 'runtimeResources': len(reference['runtimeAssets']),
          'binaryResourceComparison': 'Exact SHA256', 'textResourceComparison': 'Only CRLF/LF normalized',
          'originalGameSourceDeletions': 0, 'compilerLexer': reference['compilerLexer']}
args.report.parent.mkdir(parents=True, exist_ok=True)
args.report.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
