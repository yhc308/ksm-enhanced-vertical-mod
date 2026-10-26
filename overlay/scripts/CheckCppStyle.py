"""Review branch: check added C++ while preserving original formatting."""
import runpy
from pathlib import Path
runpy.run_path(str(Path(__file__).with_name("CheckReviewCppStyle.py")), run_name="__main__")
