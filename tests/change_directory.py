import time
import random
from pathlib import Path

BASE = Path("tests/test_dir")

def write_file(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w") as f:
        f.write(content)

def append_file(path, content):
    with open(path, "a") as f:
        f.write(content)

def safe_delete(path):
    if path.is_file():
        path.unlink()
    elif path.is_dir():
        for p in path.glob("**/*"):
            if p.is_file():
                p.unlink()
        for p in sorted(path.glob("**/*"), reverse=True):
            if p.is_dir():
                p.rmdir()
        path.rmdir()

def step_create():
    print("STEP: create")
    write_file(BASE / "file1.txt", "hello")
    write_file(BASE / "dir1/file2.txt", "nested")
    write_file(BASE / "dir1/dir2/file3.txt", "deep")

def step_modify():
    print("STEP: modify")
    append_file(BASE / "file1.txt", "\nmodified")
    write_file(BASE / "dir1/file2.txt", "rewritten " + str(random.randint(0,100)))

def step_add_more():
    print("STEP: add more")
    write_file(BASE / "dir3/new.txt", "new file")
    write_file(BASE / "dir1/dir2/extra.txt", "extra")

def step_delete_some():
    print("STEP: delete some")
    safe_delete(BASE / "dir3")
    safe_delete(BASE / "dir1/dir2/file3.txt")

def step_shuffle():
    print("STEP: shuffle")
    src = BASE / "file1.txt"
    dst = BASE / "dir1/moved.txt"
    if src.exists():
        dst.parent.mkdir(parents=True, exist_ok=True)
        src.rename(dst)

steps = [
    step_create,
    step_modify,
    step_add_more,
    step_delete_some,
    step_shuffle,
]

def main():
    BASE.mkdir(exist_ok=True)

    for f in steps:
        f()
        time.sleep(2)

if __name__ == "__main__":
    main()
