"""Run host regression scripts; this does not connect to a hardware device."""
from pathlib import Path
import argparse
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--python-only', action='store_true')
parser.add_argument('patterns', nargs='*', help='Optional test filename globs')
args = parser.parse_args()
patterns = args.patterns or (['test_*.py'] if args.python_only else ['test_*.py', 'test_*.mjs'])
tests = sorted({p for pattern in patterns for p in (ROOT / '_debug/tests').glob(pattern)})
if not tests:
    parser.error('No tests matched')
failed = []
for test in tests:
    command = [sys.executable] if test.suffix == '.py' else ['node']
    result = subprocess.run([*command, str(test)], cwd=ROOT / 'platformio')
    print(f"{'PASS' if result.returncode == 0 else 'FAIL'}: {test.name}", flush=True)
    if result.returncode:
        failed.append(test.name)
print(f'{len(tests)-len(failed)}/{len(tests)} passed')
sys.exit(bool(failed))
