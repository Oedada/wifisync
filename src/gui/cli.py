import argparse
import subprocess
from pathlib import Path
import json

GET_PATH_SCRIPT_PATH = "src/gui/get_data_path"
DETECTING_PATH = "data/detecting_units.ws"

result = subprocess.run(
[GET_PATH_SCRIPT_PATH], # команда и аргументы
capture_output=True, # перехват stdout и stderr
text=True, # декодировать в строки (иначе bytes)
check=True, # бросает исключение при коде != 0
cwd=".", # рабочая директория
)
data_path = Path(str(result.stdout))
with open(data_path / "devices.json") as f:
    devices = json.load(f)

parser = argparse.ArgumentParser()

group = parser.add_mutually_exclusive_group(required=True)
group.add_argument("--add")
group.add_argument("--rm")

args = parser.parse_args()

if args.add and Path(args.add).exists():
    path = str(Path(args.add).resolve())
    with open(data_path / DETECTING_PATH, "a") as f:
        f.write(path + "\n")
    for dkey in devices.keys():
        if path not in devices[dkey]["paths"].keys():
            other_path = input("Введите соответсвующий путь для " + devices[dkey]["name"] + f"({dkey}): ")
            if other_path:
                devices[dkey]["paths"][path] = str(Path(other_path))
            else:
                print("-> x")
    with open(data_path / "devices.json", "w") as f:
        f.write(json.dumps(devices))
                
elif args.rm:
    with open(data_path/ DETECTING_PATH, "r") as f:
        paths = list(map(lambda x: Path(x.strip()).resolve(), f.readlines()))
    if Path(args.rm).resolve() in paths:
        with open(data_path/ DETECTING_PATH, "w") as f:
            for path in paths:
                if path != Path(args.rm).resolve():
                    f.write(str(path) + "\n")


