import os
import subprocess
import sys
from pathlib import Path

VCPKG_REVISION = "b5d1a94fb7f88fd835e360fd23a45a09ceedbf48"
VCPKG_URL = "https://github.com/microsoft/vcpkg.git"
VCPKG_DIR = Path(__file__).parent / "vcpkg"


def run(cmd, **kwargs):
    print(f">> {' '.join(str(c) for c in cmd)}")
    subprocess.run(cmd, check=True, **kwargs)


def setup_vcpkg():
    # 1. Clone vcpkg if not present, otherwise fetch and checkout target revision
    if not (VCPKG_DIR / ".git").exists():
        run(["git", "clone", "--no-checkout", VCPKG_URL, str(VCPKG_DIR)])
    run(["git", "-C", str(VCPKG_DIR), "fetch", "--quiet", "origin"])
    run(["git", "-C", str(VCPKG_DIR), "checkout", VCPKG_REVISION])

    # 2. Bootstrap vcpkg
    bootstrap = VCPKG_DIR / "bootstrap-vcpkg.bat"
    run([str(bootstrap), "-disableMetrics"])

    # 3. Integrate with Visual Studio (registers MSBuild props/targets)
    vcpkg_exe = VCPKG_DIR / "vcpkg.exe"
    run([str(vcpkg_exe), "integrate", "install"])

    print("\nvcpkg setup complete. Dependencies will be installed automatically by MSBuild.")


if __name__ == "__main__":
    setup_vcpkg()
