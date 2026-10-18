#!/usr/bin/env bash
# pack_release.sh <папка с артефактами сборки> <папка для релиза>
#   ModGear-SAMP.dll / .so        плагин SA-MP
#   ModGear-openmp.dll / .so      компонент open.mp
#   ModGearAlfa-*.dll / .so       то же для Alfa
#   ModGear-includes.zip          все .inc + документация
#   ModGear-TestServer.zip        готовый тестовый сервер SA-MP с плагином
set -euo pipefail

ART=$(cd "$1" && pwd)
mkdir -p "$2"
OUT=$(cd "$2" && pwd)
ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=$(mktemp -d)

zipdir() { # zipdir <архив> <папка> <что внутри>
	if command -v zip >/dev/null; then (cd "$2" && zip -qr "$1" "$3")
	else (cd "$2" && 7z a -tzip -bso0 -bsp0 "$1" "$3")
	fi
}

for name in ModGear ModGearAlfa; do
	edition=full; [ "$name" = ModGearAlfa ] && edition=alfa
	cp "$ART/$edition-samp-windows/$name.dll" "$OUT/$name-SAMP.dll"
	cp "$ART/$edition-samp-linux/$name.so"    "$OUT/$name-SAMP.so"
	cp "$ART/$edition-omp-windows/$name.dll"  "$OUT/$name-openmp.dll"
	cp "$ART/$edition-omp-linux/$name.so"     "$OUT/$name-openmp.so"
done

mkdir -p "$WORK/inc"
cp "$ROOT"/include/*.inc "$WORK/inc/"
cp "$ROOT/docs/index.html" "$WORK/inc/ModGear.html"
zipdir "$OUT/ModGear-includes.zip" "$WORK/inc" .

srv="$WORK/server/ModGear Test Server"
mkdir -p "$srv"
cp -a "$ROOT/testserver/." "$srv/"
rm -f "$srv/filterscripts/.gitkeep"
for name in ModGear ModGearAlfa; do
	edition=full; [ "$name" = ModGearAlfa ] && edition=alfa
	cp "$ART/$edition-samp-windows/$name.dll" "$ART/$edition-samp-linux/$name.so" "$srv/plugins/"
done
cp "$ROOT"/include/*.inc "$srv/pawno/include/"
if [ "${OS:-}" = "Windows_NT" ]; then
	(cd "$srv/gamemodes" && for g in ModGear ModGearAlfa; do ../pawno/pawncc.exe "$g.pwn" -i../pawno/include -d3 "-o$g.amx"; done)
fi
curl -fsSL -o "$srv/yt-dlp.exe" https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe || true
zipdir "$OUT/ModGear-TestServer.zip" "$WORK/server" "ModGear Test Server"

rm -rf "$WORK"
ls -la "$OUT"
