# 🌐 Как открыть порты для сервера с ModGear

Чтобы игроки могли зайти на сервер и у них работали музыка, видео и вебкамера, на сервере должны быть открыты порты.

| Порт | Протокол | Зачем | Когда нужен |
|---|---|---|---|
| **7777** | UDP | игровой сервер SA-MP / open.mp | всегда |
| **7780** | TCP | медиасервер ModGear: музыка с YouTube, видео, Twitch | если в моде вызывается `Media_Init()` |
| **7781** | UDP | вебкамера (AZRTC) | если в моде вызывается `Webcam_Init()` |

> [!NOTE]
> Если в `server.cfg` / `config.json` стоит другой порт сервера или вы передаёте свои порты в `Media_Init(port)` / `Webcam_Init(port)` -
> открывайте их, а не те, что в таблице.

Порт нужно открыть **во всех местах**, через которые идёт трафик:

1. брандмауэр самой системы (Windows или Linux);
2. фаервол хостинга, если он есть (панель VPS/VDS);
3. роутер, если сервер стоит дома.

---

## Содержание

- [Windows](#-windows)
- [Linux](#-linux)
- [Фаервол хостинга (VPS/VDS)](#-фаервол-хостинга-vpsvds)
- [Сервер на домашнем ПК (роутер)](#-сервер-на-домашнем-пк-роутер)
- [bind и адрес для игроков](#-bind-и-адрес-для-игроков)
- [Как проверить, что порт открыт](#-как-проверить-что-порт-открыт)
- [Частые проблемы](#-частые-проблемы)

---

## 🪟 Windows

### Через PowerShell (быстрее всего)

Откройте PowerShell **от имени администратора** (Пуск → правой кнопкой по «Windows PowerShell» → «Запуск от имени администратора») и выполните:

```powershell
New-NetFirewallRule -DisplayName "SA-MP 7777 UDP"   -Direction Inbound -Protocol UDP -LocalPort 7777 -Action Allow
New-NetFirewallRule -DisplayName "ModGear media"    -Direction Inbound -Protocol TCP -LocalPort 7780 -Action Allow
New-NetFirewallRule -DisplayName "ModGear webcam"   -Direction Inbound -Protocol UDP -LocalPort 7781 -Action Allow
```

Проверить, что правила появились:

```powershell
Get-NetFirewallRule -DisplayName "SA-MP*","ModGear*" | Format-Table DisplayName, Enabled, Direction, Action
```

Удалить, если нужно:

```powershell
Remove-NetFirewallRule -DisplayName "SA-MP 7777 UDP","ModGear media","ModGear webcam"
```

### Через командную строку (netsh)

Командная строка **от имени администратора**:

```bat
netsh advfirewall firewall add rule name="SA-MP 7777 UDP" dir=in action=allow protocol=UDP localport=7777
netsh advfirewall firewall add rule name="ModGear media"  dir=in action=allow protocol=TCP localport=7780
netsh advfirewall firewall add rule name="ModGear webcam" dir=in action=allow protocol=UDP localport=7781
```

### Мышкой

1. **Пуск** → введите `wf.msc` → Enter (откроется «Монитор брандмауэра Защитника Windows»).
2. Слева **Правила для входящих подключений** → справа **Создать правило...**
3. Тип правила: **Для порта** → Далее.
4. Выберите **UDP**, «Определённые локальные порты»: `7777` → Далее.
5. **Разрешить подключение** → Далее → отметьте все профили (доменный, частный, публичный) → Далее.
6. Имя: `SA-MP 7777 UDP` → Готово.
7. Повторите для `7780` (**TCP**) и `7781` (**UDP**).

> [!TIP]
> При первом запуске `samp-server.exe` / `omp-server.exe` Windows сама может спросить «Разрешить доступ?» - нажмите **Разрешить**
> и отметьте **обе** галочки (частные и общественные сети). Но правило по порту надёжнее: оно не зависит от пути к exe.

### Антивирус

Если стоит сторонний антивирус со своим фаерволом (Kaspersky, ESET, Avast и т.д.), порты нужно разрешить и в нём - либо добавить
`samp-server.exe` / `omp-server.exe` в доверенные программы.

---

## 🐧 Linux

Сначала узнайте, какой фаервол стоит. Обычно на Ubuntu/Debian - **ufw**, на CentOS/AlmaLinux/Rocky - **firewalld**.
Если ни одного нет, правила задаются через **iptables** (или nftables).

```bash
sudo ufw status            # ufw
sudo firewall-cmd --state  # firewalld
sudo iptables -L -n        # iptables
```

### ufw (Ubuntu, Debian)

```bash
sudo ufw allow 7777/udp
sudo ufw allow 7780/tcp
sudo ufw allow 7781/udp
sudo ufw status
```

> [!WARNING]
> Если ufw ещё не включён, перед `sudo ufw enable` обязательно разрешите SSH, иначе потеряете доступ к серверу:
> ```bash
> sudo ufw allow OpenSSH
> sudo ufw enable
> ```

### firewalld (CentOS, AlmaLinux, Rocky, Fedora)

```bash
sudo firewall-cmd --permanent --add-port=7777/udp
sudo firewall-cmd --permanent --add-port=7780/tcp
sudo firewall-cmd --permanent --add-port=7781/udp
sudo firewall-cmd --reload
sudo firewall-cmd --list-ports
```

### iptables

```bash
sudo iptables -A INPUT -p udp --dport 7777 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 7780 -j ACCEPT
sudo iptables -A INPUT -p udp --dport 7781 -j ACCEPT
```

Правила iptables сбрасываются после перезагрузки. Чтобы сохранить:

```bash
# Ubuntu / Debian
sudo apt install iptables-persistent
sudo netfilter-persistent save

# CentOS / RHEL
sudo service iptables save
```

### nftables

```bash
sudo nft add rule inet filter input udp dport 7777 accept
sudo nft add rule inet filter input tcp dport 7780 accept
sudo nft add rule inet filter input udp dport 7781 accept
sudo sh -c 'nft list ruleset > /etc/nftables.conf'
```

### Запуск сервера на Linux

Плагин и серверы 32-битные. На 64-битной системе нужны 32-битные библиотеки:

```bash
# Ubuntu / Debian
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install libc6:i386 libstdc++6:i386
```

Чтобы сервер не закрывался вместе с SSH, запускайте его в `screen` или `tmux`:

```bash
sudo apt install screen
screen -S samp
./samp03svr        # SA-MP  (open.mp: ./omp-server)
# выйти, не останавливая сервер: Ctrl+A, затем D
# вернуться: screen -r samp
```

---

## ☁️ Фаервол хостинга (VPS/VDS)

У многих хостингов есть свой фаервол **поверх** системы: «Security Groups», «Firewall», «Сетевые правила» в панели управления.
Если он включён, порты нужно добавить и туда - иначе трафик не дойдёт до сервера, даже если в системе всё открыто.

Добавьте входящие правила:

| Протокол | Порт | Источник |
|---|---|---|
| UDP | 7777 | `0.0.0.0/0` (все) |
| TCP | 7780 | `0.0.0.0/0` |
| UDP | 7781 | `0.0.0.0/0` |

Где это обычно находится:
- **Hetzner** - Cloud Console → Firewalls;
- **AWS** - EC2 → Security Groups → Inbound rules;
- **Google Cloud** - VPC network → Firewall;
- **Oracle Cloud** - Networking → Virtual Cloud Networks → Security Lists (+ ещё нужно открыть порты в самой системе, там по умолчанию закрыто всё);
- **Timeweb, Selectel, Aeza, FirstVDS** и другие - раздел «Firewall» / «Сеть» в панели, если он включён.

---

## 🏠 Сервер на домашнем ПК (роутер)

Если сервер запущен дома, а играть будут из интернета, на роутере нужно пробросить порты на ваш компьютер.

1. Узнайте локальный IP компьютера: `ipconfig` (Windows, строка «IPv4-адрес») или `ip a` (Linux). Обычно это `192.168.0.x` или `192.168.1.x`.
2. Зайдите в настройки роутера - чаще всего `192.168.0.1` или `192.168.1.1` в браузере (логин и пароль - на наклейке роутера).
3. Найдите раздел **Port Forwarding / Переадресация портов / Виртуальный сервер / NAT**.
4. Добавьте правила на IP вашего ПК:

   | Внешний порт | Внутренний порт | Протокол | IP компьютера |
   |---|---|---|---|
   | 7777 | 7777 | UDP | 192.168.x.x |
   | 7780 | 7780 | TCP | 192.168.x.x |
   | 7781 | 7781 | UDP | 192.168.x.x |

5. Чтобы IP компьютера не менялся, закрепите его в роутере (DHCP → «Резервирование адреса») или задайте статический в Windows.

> [!IMPORTANT]
> Если провайдер выдаёт «серый» IP (адрес в роутере на странице WAN не совпадает с тем, что показывает [2ip.ru](https://2ip.ru)),
> проброс не сработает - снаружи к вам не достучаться. Нужно заказать у провайдера «белый» (публичный) IP или использовать VPS.

---

## 📍 bind и адрес для игроков

`bind` в `server.cfg` (SA-MP) или `"network": { "bind": "..." }` в `config.json` (open.mp) - на каком адресе сервер слушает:

| bind | Кто может зайти |
|---|---|
| `127.0.0.1` | только с этого же компьютера (так настроены тестовые серверы из релиза) |
| `192.168.x.x` | компьютеры в локальной сети |
| не задан или `0.0.0.0` | все, у кого есть доступ к открытому порту |

В SA-MP в `bind` пишется **только IP**, порт - отдельной строкой `port 7777`.

Медиасервер ModGear отдаёт игрокам ссылки на музыку и видео с адресом из `bind`. Если `bind` не задан, плагин сам узнаёт
внешний IP. Если сервер за NAT или у него несколько адресов - укажите внешний IP явно:

```pawn
Media_Init(7780, "1.2.3.4");
Webcam_Init(7781, "1.2.3.4");
```

---

## ✅ Как проверить, что порт открыт

**Слушает ли сервер порт** (на самом сервере):

```powershell
# Windows
netstat -ano | findstr ":7777 :7780 :7781"
```
```bash
# Linux
sudo ss -tulpn | grep -E ':7777|:7780|:7781'
```

Если строки нет - сервер не запущен или слушает другой порт / адрес (смотрите `bind`).

**Доступен ли TCP-порт медиасервера снаружи** (с другого компьютера):

```powershell
# Windows
Test-NetConnection 1.2.3.4 -Port 7780
```
```bash
# Linux
nc -zv 1.2.3.4 7780
```

`TcpTestSucceeded : True` / `succeeded` - порт открыт. Можно также открыть в браузере `http://1.2.3.4:7780/` - если медиасервер
доступен, страница ответит (пусть даже ошибкой 404).

**UDP-порт игры** проверяется проще всего самим клиентом: добавьте сервер в SA-MP и посмотрите, отвечает ли он (пинг и онлайн).
Онлайн-проверки портов обычно умеют только TCP, для UDP они показывают «закрыт» даже когда всё работает.

---

## ❓ Частые проблемы

| Проблема | Что проверить |
|---|---|
| Сервер виден в локальной сети, но не из интернета | проброс портов на роутере, «серый» IP, фаервол хостинга |
| Сервер не виден вообще | брандмауэр системы, `bind`, запущен ли сервер (`netstat` / `ss`) |
| Зайти можно, но музыка и видео не играют у других игроков | открыт ли 7780/**TCP**, правильный ли адрес в `Media_Init` |
| У вас музыка играет, у игроков из интернета - нет | медиасервер отдаёт ссылку на локальный `bind` - задайте внешний IP в `Media_Init(7780, "внешний IP")` |
| Вебкамера не подключается | открыт ли 7781/**UDP**, вызван ли `Webcam_Init()` |
| После перезагрузки Linux порты снова закрыты | правила iptables не сохранены (см. `netfilter-persistent save`) |
