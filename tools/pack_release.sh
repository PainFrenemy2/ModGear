#!/usr/bin/env bash
# pack_release.sh <папка с артефактами сборки> <папка для релизов>
# Для каждой редакции (ModGear, ModGearAlfa) - своя папка <релизы>/<имя>, в именах версия из Version.h:
#   <имя>-<версия>-SAMP.dll / .so          плагин SA-MP
#   <имя>-<версия>-openmp.dll / .so        компонент open.mp
#   <имя>-<версия>-includes.zip            include + документация
#   <имя>-<версия>-TestServer-SAMP.zip     готовый тестовый сервер SA-MP с плагином
set -euo pipefail

ART=$(cd "$1" && pwd)
mkdir -p "$2"
OUT=$(cd "$2" && pwd)
ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=$(mktemp -d)

ver() { # ver <префикс define>
	local v=""
	for k in MAJOR MINOR PATCH; do v="$v.$(sed -nE "s/^#define ${1}_$k ([0-9]+).*/\1/p" "$ROOT/server/Version.h")"; done
	echo "${v#.}"
}

zipdir() { # zipdir <архив> <папка> <что внутри>
	if command -v zip >/dev/null; then (cd "$2" && zip -qr "$1" "$3")
	elif command -v 7z >/dev/null; then (cd "$2" && 7z a -tzip -bso0 -bsp0 "$1" "$3")
	else (cd "$2" && python -c 'import sys,os,zipfile
z=zipfile.ZipFile(sys.argv[1],"w",zipfile.ZIP_DEFLATED)
for d,_,fs in os.walk(sys.argv[2]):
    for f in fs: p=os.path.join(d,f); z.write(p,os.path.normpath(p))
z.close()' "$1" "$3")
	fi
}

YTDLP="$WORK/yt-dlp.exe"
curl -fsSL -o "$YTDLP" https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe || rm -f "$YTDLP"

for name in ModGear ModGearAlfa; do
	edition=full; v=$(ver MODGEAR_VERSION)
	[ "$name" = ModGearAlfa ] && edition=alfa && v=$(ver MODGEAR_ALFA_VERSION)
	dst="$OUT/$name"
	mkdir -p "$dst"

	cp "$ART/$edition-samp-windows/$name.dll" "$dst/$name-$v-SAMP.dll"
	cp "$ART/$edition-samp-linux/$name.so"    "$dst/$name-$v-SAMP.so"
	cp "$ART/$edition-omp-windows/$name.dll"  "$dst/$name-$v-openmp.dll"
	cp "$ART/$edition-omp-linux/$name.so"     "$dst/$name-$v-openmp.so"

	inc="$WORK/inc-$name"
	mkdir -p "$inc"
	if [ "$name" = ModGear ]; then cp "$ROOT"/include/ModGear.inc "$ROOT"/include/ModGear_*.inc "$inc/"
	else cp "$ROOT/include/ModGearAlfa.inc" "$inc/"
	fi
	cp "$ROOT/docs/index.html" "$inc/ModGear.html"
	zipdir "$dst/$name-$v-includes.zip" "$inc" .

	title="$name Test Server"
	srv="$WORK/server-$name/$title"
	mkdir -p "$srv"
	cp -a "$ROOT/testserver/." "$srv/"
	rm -f "$srv/filterscripts/.gitkeep"
	cp "$ART/$edition-samp-windows/$name.dll" "$ART/$edition-samp-linux/$name.so" "$srv/plugins/"
	cp "$inc"/*.inc "$srv/pawno/include/"
	if [ "$name" = ModGearAlfa ]; then rm -f "$srv/gamemodes/ModGear.pwn"; else rm -f "$srv/gamemodes/ModGearAlfa.pwn"; fi
	sed -i -E "s/^(gamemode0[[:space:]]+).*/\1$name/; s/^(hostname[[:space:]]+).*/\1$title/; /^plugins /s/\bModGear(Alfa)?\b/$name/" "$srv/server.cfg"
	if [ "${OS:-}" = "Windows_NT" ]; then
		(cd "$srv/gamemodes" && ../pawno/pawncc.exe "$name.pwn" -i../pawno/include -d3 "-o$name.amx")
	fi
	[ -f "$YTDLP" ] && cp "$YTDLP" "$srv/"
	zipdir "$dst/$name-$v-TestServer-SAMP.zip" "$WORK/server-$name" "$title"
done

rm -rf "$WORK"
ls -laR "$OUT"
