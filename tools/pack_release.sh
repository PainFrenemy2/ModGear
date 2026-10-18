#!/usr/bin/env bash
# pack_release.sh <папка с артефактами сборки> <папка для архивов>
#   <name>-SAMP.zip    plugins/<name>.dll + .so, pawno/include/*.inc
#   <name>-openmp.zip  components/<name>.dll + .so, qawno/include/*.inc
set -euo pipefail

ART=$(realpath "$1")
OUT=$(realpath -m "$2")
ROOT=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$OUT"
WORK=$(mktemp -d)

pack() {
	local edition=$1 name=$2 d

	d="$WORK/$name-SAMP"
	mkdir -p "$d/plugins" "$d/pawno/include"
	cp "$ART/$edition-samp-windows/$name.dll" "$ART/$edition-samp-linux/$name.so" "$d/plugins/"
	cp "$ROOT/include/$name.inc" "$d/pawno/include/"
	if [ "$name" = "ModGear" ]; then cp "$ROOT"/include/ModGear_*.inc "$d/pawno/include/"; fi
	cp "$ROOT/docs/index.html" "$d/ModGear.html"
	(cd "$d" && zip -qr "$OUT/$name-SAMP.zip" .)

	d="$WORK/$name-openmp"
	mkdir -p "$d/components" "$d/qawno/include"
	cp "$ART/$edition-omp-windows/$name.dll" "$ART/$edition-omp-linux/$name.so" "$d/components/"
	cp "$ROOT/include/$name.inc" "$d/qawno/include/"
	if [ "$name" = "ModGear" ]; then cp "$ROOT"/include/ModGear_*.inc "$d/qawno/include/"; fi
	cp "$ROOT/docs/index.html" "$d/ModGear.html"
	(cd "$d" && zip -qr "$OUT/$name-openmp.zip" .)
}

pack full ModGear
pack alfa ModGearAlfa
rm -rf "$WORK"
ls -la "$OUT"
