"""Exercise real host converters with both the current and legacy asset setting."""
from pathlib import Path
import shutil
import subprocess
import sys
import zipfile

prefix, work = map(Path, sys.argv[1:3])
source = work / "source/assets"
outputs = {"screen.dmv", "icon.dmvi", "body.dmvf", "title.dmvf"}
for old in (False, True):
    build = work / ("assets-old" if old else "assets-new")
    shutil.copytree(work / "assets-tools", build)
    subprocess.run(["cmake", "-S", str(source), "-B", str(build),
                    f"-DOLD_ASSET_NAME={'ON' if old else 'OFF'}", *sys.argv[3:]], check=True)
    subprocess.run(["cmake", "--build", str(build), "--parallel", "2"], check=True)
    for name in outputs:
        assert (build / "views" / name).stat().st_size > 0, name
    with zipfile.ZipFile(build / "packages/sdk_assets.zip") as package:
        for name in outputs:
            assert package.read("views/" + name) == (build / "views" / name).read_bytes()
    # Make sure incremental builds do not rerun converters unnecessarily.
    stamps = {name: (build / "views" / name).stat().st_mtime_ns for name in outputs}
    subprocess.run(["cmake", "--build", str(build), "--parallel", "2"], check=True)
    assert stamps == {name: (build / "views" / name).stat().st_mtime_ns for name in outputs}
    if old:
        for name in outputs:
            assert (build / "views" / name).read_bytes() == (work / "assets-new/views" / name).read_bytes()
print("Real view/image/font converters, tracking, packaging and legacy asset alias passed")
