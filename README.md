# ModGear

[![Release](https://img.shields.io/github/v/release/PainFrenemy2/ModGear?label=release)](https://github.com/PainFrenemy2/ModGear/releases/latest)
[![Build](https://github.com/PainFrenemy2/ModGear/actions/workflows/build.yml/badge.svg)](https://github.com/PainFrenemy2/ModGear/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

Плагин для **SA-MP 0.3.7** и **open.mp**, который даёт серверу функции клиента **Arizona RP**: интерфейс и HUD,
кастомные модели машин, скинов и оружия, тюнинг, педы (боты), музыка и видео на экранах, голосовые каналы,
вебкамера и многое другое. Игроки без лаунчера Arizona видят обычные модели GTA - для них плагин сам
подменяет всё кастомное на стандартное.

Полный список функций с примерами - в [документации](https://htmlpreview.github.io/?https://raw.githubusercontent.com/PainFrenemy2/ModGear/HEAD/docs/index.html)
(она же лежит в каждом архиве релиза как `ModGear.html`).

---

## Содержание

- [Что умеет](#что-умеет)
- [Установка](#установка)
- [Порты](#порты)
- [Сборка из исходников](#сборка-из-исходников)
- [Автосборка на GitHub](#автосборка-на-github)
- [Тестовый мод](#тестовый-мод)
- [Структура репозитория](#структура-репозитория)
- [Частые проблемы](#частые-проблемы)
- [Лицензия](#лицензия)

---

## Что умеет

| Модуль | Include | Что внутри |
|---|---|---|
| Основа | `ModGear` | статус лаунчера, информация о сервере, замена моделей/скинов/иконок/оружия для игроков без лаунчера |
| Интерфейс | `ModGear_Interface` | HUD, CEF-окна, чат и вкладки чата, диалоги, навигатор, курсор, таблица игроков |
| Игрок | `ModGear_Player` | ник и аксессуары, оружие (обойма, точность, кастомное оружие 71+), камера, затемнение экрана, 3D-иконки в мире |
| Машины | `ModGear_Vehicle` | тюнинг, винилы, грязь и износ, суппорты, стробоскопы, нитро, сцепка машин, хендлинг |
| Мир | `ModGear_World` | объекты, частицы, надписи в небе, зоны, вода |
| Медиа | `ModGear_Media` | музыка (mp3 и YouTube) в машинах, колонках и фоном, видео на экранах (YouTube, Twitch) |
| Голос | `ModGear_Voice` | голосовые каналы, рации, вебкамера (AZRTC) |
| NPC | `ModGear_NPC` | педы и охранники: ходьба, оружие, урон по игрокам, посадка в машины |

Есть две редакции:

- **ModGear** - полная;
- **ModGearAlfa** - облегчённая: замена моделей, скинов и иконок, цвет машин, статус лаунчера.
  Подключается вместо `ModGear`, не вместе с ним.

---

## Установка

Скачай архив из [Releases](https://github.com/PainFrenemy2/ModGear/releases/latest):

| Архив | Для чего |
|---|---|
| `ModGear-SAMP.zip` | SA-MP 0.3.7 (Windows и Linux) |
| `ModGear-openmp.zip` | open.mp (Windows и Linux) |

### SA-MP 0.3.7

1. Из архива скопируй `plugins/ModGear.dll` (Windows) или `plugins/ModGear.so` (Linux) в папку `plugins` сервера.
2. Скопируй всё из `pawno/include` в `pawno/include` сервера.
3. Допиши плагин в `server.cfg`:
   ```
   plugins ModGear          (Windows)
   plugins ModGear.so       (Linux)
   ```

### open.mp

1. `components/ModGear.dll` или `components/ModGear.so` - в папку `components` сервера. В `config.json` ничего дописывать не нужно, компоненты грузятся сами.
2. Всё из `qawno/include` - в `qawno/include` сервера.

### В моде

```pawn
#include <a_samp>
#include <ModGear>
#include <ModGear_Player>   // и другие модули по необходимости

public OnPlayerConnect(playerid)
{
    // клиент Arizona представляется версией "Arizona PC" (или "arizona-mobile") -
    // отмечаем игрока, после этого ему работают функции плагина
    new version[24];
    GetPlayerVersion(playerid, version, sizeof version);
    SetPlayerLauncherStatus(playerid, !strcmp(version, "Arizona PC") || !strcmp(version, "arizona-mobile"));
    return 1;
}
```

Подробные примеры для каждой функции - в документации.

---

## Порты

| Порт | Протокол | Зачем | Когда нужен |
|---|---|---|---|
| **7777** | UDP | сам игровой сервер | всегда |
| **7780** | TCP | медиасервер плагина: музыка с YouTube, видео, Twitch | если в моде есть `Media_Init()` |
| **7781** | UDP | вебкамера (AZRTC) | если в моде есть `Webcam_Init()` |

Порты медиасервера и вебкамеры можно поменять: `Media_Init(port)`, `Webcam_Init(port)` или
`#define MODGEAR_MEDIA_PORT` / `#define MODGEAR_WEBCAM_PORT` перед `#include`.

### Windows (брандмауэр)

PowerShell от имени администратора:

```powershell
New-NetFirewallRule -DisplayName "SA-MP 7777"      -Direction Inbound -Protocol UDP -LocalPort 7777 -Action Allow
New-NetFirewallRule -DisplayName "ModGear media"   -Direction Inbound -Protocol TCP -LocalPort 7780 -Action Allow
New-NetFirewallRule -DisplayName "ModGear webcam"  -Direction Inbound -Protocol UDP -LocalPort 7781 -Action Allow
```

### Linux

```bash
# ufw
sudo ufw allow 7777/udp
sudo ufw allow 7780/tcp
sudo ufw allow 7781/udp

# или iptables
sudo iptables -A INPUT -p udp --dport 7777 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 7780 -j ACCEPT
sudo iptables -A INPUT -p udp --dport 7781 -j ACCEPT
```

### Роутер (сервер дома)

Если сервер стоит на домашнем ПК, а играть будут из интернета, в настройках роутера пробрось те же порты
(Port Forwarding / Виртуальный сервер) на локальный IP компьютера (`192.168.x.x`). Узнать его: `ipconfig` (Windows)
или `ip a` (Linux).

### bind

- `bind 127.0.0.1` - сервер виден только с этого же компьютера (удобно для теста).
- `bind 192.168.x.x` - виден в локальной сети.
- без `bind` (или `0.0.0.0`) - виден отовсюду, где открыт порт.

В `server.cfg` в `bind` пишется только IP, порт - отдельной строкой `port 7777`.

Медиасервер даёт игрокам ссылки на тот же адрес, что и `bind`. Если `bind` не задан, плагин сам узнаёт внешний IP
(через api.ipify.org). Адрес можно указать явно: `Media_Init(7780, "1.2.3.4")`.

### Музыка и видео с YouTube

Рядом с сервером должны лежать:

- **Windows:** [yt-dlp.exe](https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe),
  `ffmpeg.exe` и `ffprobe.exe` из [FFmpeg](https://github.com/BtbN/FFmpeg-Builds/releases/latest/download/ffmpeg-master-latest-win64-gpl.zip) (папка `bin`);
- **Linux:** [yt-dlp_linux](https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp_linux) (переименовать в `yt-dlp`),
  `ffmpeg` и `ffprobe` из [FFmpeg](https://github.com/BtbN/FFmpeg-Builds/releases/latest/download/ffmpeg-master-latest-linux64-gpl.tar.xz),
  затем `chmod +x yt-dlp ffmpeg ffprobe`.

Прямые ссылки на mp3 работают и без них.

---

## Сборка из исходников

### Windows - одной кнопкой

```bat
git clone --recursive https://github.com/PainFrenemy2/ModGear.git
cd ModGear
build.bat
```

`build.bat` сам ставит через winget всё, чего не хватает (Git, CMake, Visual Studio 2022 Build Tools с C++),
подтягивает библиотеки open.mp и собирает четыре версии. Готовые файлы - в папке `out`:

```
out/ModGear-SAMP/plugins/ModGear.dll
out/ModGear-openmp/components/ModGear.dll
out/ModGearAlfa-SAMP/...
out/ModGearAlfa-openmp/...
```

Можно собрать только нужное: `build.bat full`, `build.bat alfa`, `build.bat samp`, `build.bat omp`
(и сочетания, например `build.bat full samp`).

### Windows - вручную

Нужны Visual Studio 2022 (компонент «Разработка классических приложений на C++») и CMake 3.19+.
Плагин 32-битный, поэтому `-A Win32`.

```bat
git submodule update --init --recursive
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -DMODGEAR_PLATFORM=samp -DMODGEAR_EDITION=full
cmake --build build --config Release
```

Результат: `build/server/Release/ModGear.dll`.

### Linux

Нужен 32-битный компилятор:

```bash
sudo apt install build-essential cmake git g++-multilib
git clone --recursive https://github.com/PainFrenemy2/ModGear.git
cd ModGear
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 \
      -DMODGEAR_PLATFORM=samp -DMODGEAR_EDITION=full
cmake --build build --parallel
```

Результат: `build/server/ModGear.so`.

### Параметры CMake

| Параметр | Значения | По умолчанию |
|---|---|---|
| `MODGEAR_PLATFORM` | `samp` - плагин SA-MP, `omp` - компонент open.mp | `omp` |
| `MODGEAR_EDITION` | `full` - ModGear, `alfa` - ModGearAlfa | `full` |

Для `omp` нужны сабмодули `lib/omp-sdk` и `lib/omp-net` (`git submodule update --init --recursive`).
mbedTLS скачивается автоматически при первой настройке CMake. С CMake 4.x добавь `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`.

Версия задаётся в `server/Version.h`.

---

## Автосборка на GitHub

Workflow `.github/workflows/build.yml` запускается на каждый push и pull request и собирает все
восемь вариантов (2 редакции x SA-MP/open.mp x Windows/Linux). Готовые файлы - во вкладке
**Actions -> запуск -> Artifacts** (`ModGear-release` - уже упакованные архивы).

Релиз выпускается вручную - пушем тега:

```bash
git tag v0.7.0
git push origin v0.7.0
```

Workflow соберёт всё и создаст релиз с архивами. Чтобы релиз публиковался от твоего аккаунта, а не от
`github-actions`, добавь в настройках репозитория секрет `RELEASE_TOKEN` (личный токен с правом `Contents: write`).

---

## Тестовый мод

В `examples` лежат тестовые моды с меню на все функции плагина (`/menu`):

```
examples/samp/gamemodes/ModGear.pwn       - полная редакция, SA-MP
examples/samp/gamemodes/ModGearAlfa.pwn   - Alfa, SA-MP
examples/samp/server.cfg
examples/openmp/gamemodes/ModGear.pwn     - то же для open.mp
examples/openmp/config.json
```

Для компиляции нужны include: [Pawn.CMD](https://github.com/katursis/Pawn.CMD), [sscanf](https://github.com/Y-Less/sscanf),
[Streamer](https://github.com/samp-incognito/samp-streamer-plugin), [Pawn.RakNet](https://github.com/katursis/Pawn.RakNet),
[fmt](https://github.com/IllidanS4/pawn-fmt) (только SA-MP) и сами `ModGear*.inc` из папки `include`.

```bat
pawncc ModGear.pwn -i<папка include> -d3 -oModGear.amx
```

---

## Структура репозитория

```
server/             исходники плагина
  Plugin.cpp        регистрация нативов, перехват моделей
  Events.cpp        события сервера, пакеты клиента, синхронизация
  Media.cpp         медиасервер (YouTube, Twitch, видео)
  AzRtc.cpp         сервер вебкамеры
  include/lib/      модули: нативы, педы, голос, видео, CEF, пакеты Arizona
  platform/samp/    вход для SA-MP (хуки RakServer)
  platform/omp/     вход для open.mp (компонент)
include/            ModGear*.inc для Pawn
lib/                RakNet (вебкамера), sampgdk, open.mp SDK (сабмодули)
docs/index.html     документация по всем функциям
examples/           тестовые моды и конфиги
tools/              упаковка релиза
build.bat           сборка под Windows
```

---

## Частые проблемы

| Проблема | Что сделать |
|---|---|
| Сервер пишет, что не нашёл плагин | на Linux в `server.cfg` нужно `ModGear.so`; проверь, что файл 32-битный (`file ModGear.so`) |
| Функции не работают у игрока | у игрока должен быть клиент Arizona; мод должен вызвать `SetPlayerLauncherStatus(playerid, true)` (см. пример выше) |
| Музыка с YouTube не играет | рядом с сервером нет `yt-dlp`/`ffmpeg`, или закрыт порт 7780/TCP, или не вызван `Media_Init()` |
| Видео/музыка у игроков из интернета не грузятся, а у тебя да | медиасервер отдаёт ссылки на адрес из `bind` - для игроков из интернета задай внешний IP: `Media_Init(7780, "внешний IP")` |
| Вебкамера не подключается | открой 7781/UDP и вызови `Webcam_Init()` |
| open.mp: `sampgdk: Native function not found` | обнови ModGear до 0.7.0+ |

---

## Лицензия

[MIT](LICENSE) - можно использовать, менять и распространять, в том числе на своих серверах и в своих
сборках, при сохранении копирайта.

Автор: [PainFrenemy2](https://github.com/PainFrenemy2)
