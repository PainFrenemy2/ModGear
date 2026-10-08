<div align="center">

# ⚙️ ModGear

**Плагин для SA-MP и open.mp: тюнинг авто, кастомный HUD, CEF-интерфейсы и аудиостримы**

[![Release](https://img.shields.io/github/v/release/PainFrenemy2/ModGear?filter=ModGear-v*&style=for-the-badge&logo=github&label=ModGear&color=2ea44f)](https://github.com/PainFrenemy2/ModGear/releases/latest)
[![Alfa](https://img.shields.io/github/v/release/PainFrenemy2/ModGear?filter=ModGearAlfa-*&style=for-the-badge&logo=github&label=ModGearAlfa&color=orange)](https://github.com/PainFrenemy2/ModGear/releases)
[![Downloads](https://img.shields.io/github/downloads/PainFrenemy2/ModGear/total?style=for-the-badge&color=blue)](https://github.com/PainFrenemy2/ModGear/releases)

![SA-MP](https://img.shields.io/badge/SA--MP-0.3.7-orange?style=flat-square)
![open.mp](https://img.shields.io/badge/open.mp-component-5b3cc4?style=flat-square)
![Windows](https://img.shields.io/badge/Windows-x86-0078D6?style=flat-square&logo=windows&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-x86-FCC624?style=flat-square&logo=linux&logoColor=black)

[**📥 Скачать**](https://github.com/PainFrenemy2/ModGear/releases/latest) •
[**📖 Документация**](https://htmlpreview.github.io/?https://raw.githubusercontent.com/PainFrenemy2/ModGear/HEAD/docs/index.html) •
[**💬 Купить / связь**](https://vk.com/seym2)

</div>

---

## ✨ Возможности

- 🚗 **Тюнинг авто**: винилы, диски (цвет и размер), неон, фары, дым, выхлоп, гидравлика, номера
- 🖥 **Интерфейс**: кастомный HUD, спидометр, радар, карта, чат, диалоги, подсказки
- 🌐 **CEF**: `ShowCef`, `ShowCefMobile`, YouTube на телефоне
- 🔊 **Аудио**: стримы, привязанные к игроку или машине
- 🎒 **Аттачи**: расширенные `SetPlayerAttachedObjectEx` / `EditAttachedObjectEx`
- 📱 Поддержка мобильного клиента

## 📦 Редакции

| | **ModGear** | **ModGearAlfa** |
|---|:---:|:---:|
| Замена моделей / скинов / иконок | ✅ | ✅ |
| Цвет машин, статус лаунчера | ✅ | ✅ |
| Тюнинг (диски, неон, винилы, тонировка…) | ✅ | ❌ |
| HUD, спидометр, CEF-интерфейсы | ✅ | ❌ |
| Аудиостримы, аттачи, теги, чат-комнаты | ✅ | ❌ |

## 📥 Что скачивать

В каждом [релизе](https://github.com/PainFrenemy2/ModGear/releases):

| Файл | Для чего |
|---|---|
| `ModGear-SAMP.zip` | сервер **SA-MP**: `plugins/ModGear.dll` / `.so` + `pawno/include/ModGear.inc` |
| `ModGear-openmp.zip` | сервер **open.mp**: `components/ModGear.dll` / `.so` + `qawno/include/ModGear.inc` |
| `ModGear-Server.zip` | готовый тестовый сервер SA-MP — распаковать и запустить `start.bat` |

Для альфы — то же самое с именем `ModGearAlfa`.

## 🚀 Установка

**SA-MP**
1. Распакуйте `ModGear-SAMP.zip` в папку сервера.
2. В `server.cfg`: `plugins ... ModGear` (Linux: `ModGear.so`) и `bind <IP сервера>` (можно не указывать — плагин сам узнает внешний IP).

**open.mp**
1. Распакуйте `ModGear-openmp.zip` в папку сервера (`components/`).
2. В `config.json` можно указать `"network": { "bind": "<IP сервера>" }` (не обязательно — плагин сам узнает внешний IP).

В моде: `#include <ModGear>`.

### 🎵 Радио с YouTube в машине (Audio_)

Чтобы `Audio_`-функции играли ссылки YouTube (они конвертируются в mp3 на сервере),
положите рядом с сервером две бесплатные программы. Для всего остального (видео, хендлинг,
интерфейс) они **не нужны**.

**Windows** (`yt-dlp.exe` и `ffmpeg.exe` в папку сервера):
- yt-dlp — https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe
- ffmpeg — https://github.com/BtbN/FFmpeg-Builds/releases/latest/download/ffmpeg-master-latest-win64-gpl.zip
  (из архива `bin/ffmpeg.exe`)

**Linux** (`yt-dlp` и `ffmpeg` рядом с сервером, затем `chmod +x`):
- yt-dlp — https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp_linux (переименуйте в `yt-dlp`)
- ffmpeg — https://github.com/BtbN/FFmpeg-Builds/releases/latest/download/ffmpeg-master-latest-linux64-gpl.tar.xz
  (из архива `bin/ffmpeg`)

Плагин ищет их в папке сервера или в системном PATH. Без них ссылки YouTube в `Audio_` не заиграют
(в логе будет предупреждение), прямые mp3-ссылки работают и так.

> [!IMPORTANT]
> Плагин работает только на серверах с активированным доступом (IP + порт).
> Для подключения: [vk.com/seym2](https://vk.com/seym2)

---

<div align="center">

© **Pain** — [vk.com/seym2](https://vk.com/seym2)

</div>
