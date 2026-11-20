#!/usr/bin/env bash
set -eo pipefail

source /opt/openfoam14/etc/bashrc

cd "$(dirname "$0")/.."

MESH="mesh/tank.msh"
BOUNDARY="case/constant/polyMesh/boundary"

if [[ ! -f "$MESH" ]]; then
    echo "Mesh not found: $MESH"
    echo "Run the C++ mesh generator first."
    exit 1
fi

if [[ -e "$BOUNDARY" ]]; then
    echo "OpenFOAM mesh already exists."
    echo "Remove it manually before re-importing."
    exit 1
fi

echo "Importing Gmsh mesh..."
gmshToFoam "$MESH" -case case

echo "Configuring 2D boundary..."

python3 - <<'PY'
from pathlib import Path
import re

path = Path("case/constant/polyMesh/boundary")
text = path.read_text()

match = re.search(
    r"(frontAndBack\s*\{)(.*?)(\})",
    text,
    re.DOTALL
)

if match is None:
    raise RuntimeError("frontAndBack patch not found")

block = match.group(2)

block, count = re.subn(
    r"\btype\s+patch;",
    "type empty;",
    block
)

if count != 1:
    raise RuntimeError("Cannot change patch type")

block = re.sub(
    r"\bphysicalType\s+patch;",
    "physicalType empty;",
    block
)

text = text[:match.start(2)] + block + text[match.end(2):]
path.write_text(text)
PY

echo "Checking mesh..."
checkMesh -case case

echo "Mesh preparation completed."
