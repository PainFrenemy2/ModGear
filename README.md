<div align="center">

# ⚙️ ModGear

**Плагин для SA-MP и open.mp: функции клиента Arizona RP на вашем сервере**

[![Release](https://img.shields.io/github/v/release/PainFrenemy2/ModGear?filter=ModGear-v*&style=for-the-badge&logo=github&label=ModGear&color=2ea44f)](https://github.com/PainFrenemy2/ModGear/releases/latest)
[![Build](https://img.shields.io/github/actions/workflow/status/PainFrenemy2/ModGear/build.yml?branch=main&style=for-the-badge&logo=githubactions&logoColor=white&label=Build)](https://github.com/PainFrenemy2/ModGear/actions/workflows/build.yml)
[![Downloads](https://img.shields.io/github/downloads/PainFrenemy2/ModGear/total?style=for-the-badge&color=blue)](https://github.com/PainFrenemy2/ModGear/releases)
[![License](https://img.shields.io/github/license/PainFrenemy2/ModGear?style=for-the-badge&color=lightgrey)](LICENSE)

![SA-MP](https://img.shields.io/badge/SA--MP-0.3.7-orange?style=flat-square)
![open.mp](https://img.shields.io/badge/open.mp-component-5b3cc4?style=flat-square)
![Windows](https://img.shields.io/badge/Windows-x86-0078D6?style=flat-square&logo=windows&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-x86-FCC624?style=flat-square&logo=linux&logoColor=black)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=cplusplus&logoColor=white)

[**📥 Скачать**](https://github.com/PainFrenemy2/ModGear/releases/latest) •
[**📖 Документация**](https://painfrenemy2.github.io/ModGear/) •
[**🛠 Сборка**](#-сборка-из-исходников) •
[**🌐 Порты**](#-порты)

</div>

---

## ✨ Возможности

- 🚗 **Машины**: винилы, диски, неон, фары, дым, выхлоп, суппорты, стробоскопы, нитро, грязь и износ, сцепка машин, хендлинг, кастомные модели
- 🖥 **Интерфейс**: HUD Arizona, спидометр, радар, карта, чат с вкладками, диалоги, подсказки, таблица игроков, 3D-иконки в мире
- 🌐 **CEF**: свои окна и события, YouTube на телефоне
- 🔫 **Оружие**: кастомное оружие Arizona (id 71+) с синхронизацией между игроками, обойма и точность
- 🧍 **Педы**: боты и охранники - ходьба, оружие, урон по игрокам, посадка в машины, килфид
- 🔊 **Звук и видео**: музыка (mp3 и YouTube) в машинах, колонках и фоном, эффект рации, видео на экранах (YouTube, Twitch)
- 🎙 **Голос**: голосовые каналы, рации, вебкамера (AZRTC)
- 🎒 **Аттачи**: расширенные `SetPlayerAttachedObjectEx` / `EditAttachedObjectEx`, материалы и текст на объектах
- 🔁 Игроки без лаунчера видят обычные модели GTA - кастомные машины, скины, иконки и оружие подменяются автоматически
- 📱 Поддержка мобильного клиента

Все функции с примерами - в [**документации**](https://painfrenemy2.github.io/ModGear/).

## 📦 Редакции

| | **ModGear** | **ModGearAlfa** |
|---|:---:|:---:|
| Замена моделей / скинов / иконок / оружия | ✅ | ✅ |
| Цвет машин, статус лаунчера | ✅ | ✅ |
| Тюнинг (диски, неон, винилы, тонировка…) | ✅ | ❌ |
| HUD, спидометр, CEF-интерфейсы | ✅ | ❌ |
| Музыка, видео, голос, вебкамера | ✅ | ❌ |
| Педы, аттачи, теги, чат-комнаты | ✅ | ❌ |

ModGearAlfa подключается **вместо** ModGear (`#include <ModGearAlfa>`), не вместе с ним.

## 📥 Что скачивать

В каждом [релизе](https://github.com/PainFrenemy2/ModGear/releases):

| Файл | Для чего |
|---|---|
| `ModGear-SAMP.zip` | сервер **SA-MP**: `plugins/ModGear.dll` / `.so` + `pawno/include/*.inc` |
| `ModGear-openmp.zip` | сервер **open.mp**: `components/ModGear.dll` / `.so` + `qawno/include/*.inc` |
| `ModGearAlfa-*.zip` | то же для облегчённой редакции |

## 🚀 Установка

**SA-MP**
1. Распакуйте `ModGear-SAMP.zip` в папку сервера.
2. В `server.cfg` допишите плагин: `plugins ... ModGear` (Linux: `ModGear.so`).

**open.mp**
1. Распакуйте `ModGear-openmp.zip` в папку сервера (`components/`). В `config.json` ничего дописывать не нужно.

**В моде**

```pawn
#include <a_samp>
#include <ModGear>
#include <ModGear_Player>   // и другие модули по необходимости

public OnPlayerConnect(playerid)
{
    // клиент Arizona представляется версией "Arizona PC" или "arizona-mobile"
    new version[24];
    GetPlayerVersion(playerid, version, sizeof version);
    SetPlayerLauncherStatus(playerid, !strcmp(version, "Arizona PC") || !strcmp(version, "arizona-mobile"));
    return 1;
}
```

| Include | Что внутри |
|---|---|
| `ModGear` | основа: лаунчер, сервер, утилиты, замена моделей |
| `ModGear_Interface` | HUD, CEF, чат, диалоги, навигатор, мышь |
| `ModGear_Player` | игрок: ник, аксессуары, оружие, режимы, камера |
| `ModGear_Vehicle` | машины: тюнинг, грязь, сцепка, хендлинг |
| `ModGear_World` | мир: объекты, частицы, надписи в небе, зоны |
| `ModGear_Media` | медиасервер, музыка, видео на экранах |
| `ModGear_Voice` | голосовые каналы, рации и вебкамера |
| `ModGear_NPC` | педы и охранники |

## 🌐 Порты

| Порт | Протокол | Зачем | Когда нужен |
|---|---|---|---|
| **7777** | UDP | игровой сервер | всегда |
| **7780** | TCP | медиасервер: музыка с YouTube, видео, Twitch | если в моде есть `Media_Init()` |
| **7781** | UDP | вебкамера (AZRTC) | если в моде есть `Webcam_Init()` |

Порты можно поменять: `Media_Init(port)`, `Webcam_Init(port)` или `#define MODGEAR_MEDIA_PORT` / `MODGEAR_WEBCAM_PORT` перед `#include`.

<details>
<summary><b>🪟 Открыть порты в Windows</b></summary>

PowerShell от имени администратора:

```powershell
New-NetFirewallRule -DisplayName "SA-MP 7777"     -Direction Inbound -Protocol UDP -LocalPort 7777 -Action Allow
New-NetFirewallRule -DisplayName "ModGear media"  -Direction Inbound -Protocol TCP -LocalPort 7780 -Action Allow
New-NetFirewallRule -DisplayName "ModGear webcam" -Direction Inbound -Protocol UDP -LocalPort 7781 -Action Allow
```
</details>

<details>
<summary><b>🐧 Открыть порты в Linux</b></summary>

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
</details>

<details>
<summary><b>🏠 Сервер на домашнем ПК (роутер, bind)</b></summary>

Чтобы на сервер заходили из интернета, в роутере пробросьте те же порты (Port Forwarding / Виртуальный сервер)
на локальный IP компьютера (`192.168.x.x`, смотреть через `ipconfig` или `ip a`).

`bind` в `server.cfg` / `config.json`:
- `127.0.0.1` - сервер виден только с этого компьютера (для теста);
- `192.168.x.x` - виден в локальной сети;
- не задан (или `0.0.0.0`) - виден отовсюду, где открыт порт.

В `server.cfg` в `bind` пишется только IP, порт - отдельной строкой `port 7777`.

Медиасервер отдаёт игрокам ссылки на тот же адрес, что и `bind`. Если `bind` не задан, плагин сам узнаёт
внешний IP. Адрес можно указать явно: `Media_Init(7780, "1.2.3.4")`.
</details>

### 🎵 Музыка и видео с YouTube

Чтобы `Audio_` и видео играли ссылки YouTube, положите рядом с сервером бесплатные программы.
Для всего остального они **не нужны**, прямые mp3-ссылки работают и без них.

**Windows** (в папку сервера):
- [yt-dlp.exe](https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe)
- [ffmpeg](https://github.com/BtbN/FFmpeg-Builds/releases/latest/download/ffmpeg-master-latest-win64-gpl.zip) - из архива `bin/ffmpeg.exe` и `bin/ffprobe.exe`

**Linux** (рядом с сервером, затем `chmod +x`):
- [yt-dlp_linux](https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp_linux) - переименуйте в `yt-dlp`
- [ffmpeg](https://github.com/BtbN/FFmpeg-Builds/releases/latest/download/ffmpeg-master-latest-linux64-gpl.tar.xz) - из архива `bin/ffmpeg` и `bin/ffprobe`

Треки с YouTube кешируются в `scriptfiles/modgear_media`, кеш сам чистится при превышении 500 МБ.

## 🛠 Сборка из исходников

**Windows - одной кнопкой**

```bat
git clone --recursive https://github.com/PainFrenemy2/ModGear.git
cd ModGear
build.bat
```

`build.bat` сам поставит через winget всё, чего не хватает (Git, CMake, Visual Studio 2022 Build Tools с C++),
подтянет библиотеки open.mp и соберёт все версии в папку `out`. Только нужное: `build.bat full`, `alfa`, `samp`, `omp`
(можно сочетать: `build.bat full samp`).

<details>
<summary><b>Windows - вручную (CMake)</b></summary>

Нужны Visual Studio 2022 (компонент «Разработка классических приложений на C++») и CMake 3.19+. Плагин 32-битный.

```bat
git submodule update --init --recursive
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32 -DMODGEAR_PLATFORM=samp -DMODGEAR_EDITION=full
cmake --build build --config Release
```

Результат: `build/server/Release/ModGear.dll`.
</details>

<details>
<summary><b>Linux</b></summary>

```bash
sudo apt install build-essential cmake git g++-multilib
git clone --recursive https://github.com/PainFrenemy2/ModGear.git
cd ModGear
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 \
      -DMODGEAR_PLATFORM=samp -DMODGEAR_EDITION=full
cmake --build build --parallel
```

Результат: `build/server/ModGear.so`.
</details>

| Параметр CMake | Значения | По умолчанию |
|---|---|---|
| `MODGEAR_PLATFORM` | `samp` - плагин SA-MP, `omp` - компонент open.mp | `omp` |
| `MODGEAR_EDITION` | `full` - ModGear, `alfa` - ModGearAlfa | `full` |

Для `omp` нужны сабмодули (`git submodule update --init --recursive`). mbedTLS скачивается сам.
С CMake 4.x добавьте `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`. Версия - в `server/Version.h`.

### ⚡ Автосборка

GitHub Actions собирает все 8 вариантов (2 редакции × SA-MP/open.mp × Windows/Linux) на каждый push и pull request.
Готовые файлы - во вкладке **Actions → запуск → Artifacts**.

Релиз выходит по тегу:

```bash
git tag ModGear-v0.7.1
git push origin ModGear-v0.7.1
```

Чтобы релиз публиковался от вашего аккаунта, а не от `github-actions`, добавьте секрет `RELEASE_TOKEN`
(личный токен с правом `Contents: write`).

## 🧪 Тестовый мод

В [`examples`](examples) - тестовые моды с меню на все функции плагина (`/menu`) и примеры конфигов
для SA-MP и open.mp. Для компиляции нужны include
[Pawn.CMD](https://github.com/katursis/Pawn.CMD), [sscanf](https://github.com/Y-Less/sscanf),
[Streamer](https://github.com/samp-incognito/samp-streamer-plugin), [Pawn.RakNet](https://github.com/katursis/Pawn.RakNet),
[fmt](https://github.com/IllidanS4/pawn-fmt) (SA-MP) и `ModGear*.inc` из [`include`](include).

<details>
<summary><b>📁 Структура репозитория</b></summary>

```
server/             исходники плагина
  Plugin.cpp        регистрация нативов, перехват моделей
  Events.cpp        события сервера, пакеты клиента, синхронизация
  Media.cpp         медиасервер (YouTube, Twitch, видео)
  AzRtc.cpp         сервер вебкамеры
  include/lib/      модули: нативы, педы, голос, видео, CEF, пакеты Arizona
  platform/samp/    вход для SA-MP
  platform/omp/     вход для open.mp
include/            ModGear*.inc для Pawn
lib/                RakNet, sampgdk, open.mp SDK (сабмодули)
docs/               документация (GitHub Pages)
examples/           тестовые моды и конфиги
tools/              упаковка релиза
build.bat           сборка под Windows
```
</details>

## ❓ Частые проблемы

| Проблема | Что сделать |
|---|---|
| Сервер не видит плагин | на Linux в `server.cfg` - `ModGear.so`; файл должен быть 32-битный |
| Функции не работают у игрока | нужен клиент Arizona и вызов `SetPlayerLauncherStatus(playerid, true)` (пример выше) |
| Музыка с YouTube не играет | нет `yt-dlp`/`ffmpeg`, закрыт порт 7780/TCP или не вызван `Media_Init()` |
| Музыка/видео не грузятся у игроков из интернета | задайте внешний IP: `Media_Init(7780, "внешний IP")` |
| Вебкамера не подключается | откройте 7781/UDP и вызовите `Webcam_Init()` |

---

<div align="center">

Распространяется по лицензии [MIT](LICENSE)

© **PainFrenemy2** — [github.com/PainFrenemy2](https://github.com/PainFrenemy2)

</div>
