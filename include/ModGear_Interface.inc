/*
 * ============================================================================
 *  ModGear_Interface  -  интерфейс
 * ----------------------------------------------------------------------------
 *  Худ, CEF-окна и ответы интерфейса, чат и вкладки, диалоги,
 *  навигатор, курсор мыши.
 *
 *  Подключение:   #include <ModGear_Interface>      (сам подключает <ModGear>)
 *  Документация:  https://htmlpreview.github.io/?https://raw.githubusercontent.com/PainFrenemy2/ModGear/HEAD/docs/index.html
 * ============================================================================
 */
#if defined _ModGear_Interface_included
	#endinput
#endif
#define _ModGear_Interface_included

#include <ModGear>

// ----------------------------------------------------------------------------
//  ХУД И НАСТРОЙКИ ИНТЕРФЕЙСА
// ----------------------------------------------------------------------------
// Настройки для SetPlayerInterface(playerid, setting, value)
enum
{
	Custom_Speedometer,    // 0 выкл, 1-2 мобильный, 3 продвинутый, 4 упрощённый
	Custom_Hud,            // 0 выкл, 1 PUBG, 2 Vice City, 3 San Andreas, 4-6 мобильные
	Custom_Radar,          // 0 выкл, 1-2 свой, 3 Vice City
	Custom_Fuel,           // 0-300 литров
	Custom_Milage,         // 0-999999 пробег
	Custom_TypeFuel,       // 0 бензин, 1 электро, 2 скрыть значок
	Custom_Hunger,         // 0-100 сытость
	Custom_Hints,          // 0/1 подсказки горячих клавиш
	Custom_TypeDialog,     // 0 полный, 1 упрощённый, 2 простой
	Custom_Map,            // 0/1 зеркальная карта
	Custom_Discord,        // 0/1 позиция в Discord
	Custom_Dialog,         // 0/1 сохранять текст в диалогах
	Custom_Chat,           // 0/1 прозрачный чат
	Custom_SetRadius,      // 0-500 дальность 3D-метки навигатора (220:64), 0 - выключить
	Custom_Show,           // 0/1 снегопад
	Custom_Wanted,         // 0-7 уровень розыска
	Custom_ChatBoard,      // 0/1 текст над игроками
	Custom_WallHack,       // 0/1 видеть игроков сквозь стены
	Custom_TypeAim,        // 0-10 прицел
	Custom_GeoPosition,    // 0/1 текущее местоположение на худе
	Custom_CinemaMode      // 0/1 кинорежим (скрыть худ)
};
native SetPlayerInterface(playerid, setting, value);            // setting - Custom_*, value - значение из списка выше

#define HOT_KEY_RADIO                  (0)
#define HOT_KEY_FRACTION_RADIO         (1)
#define HOT_KEY_FAMILY_RADIO           (2)
#define HOT_KEY_INTERACTION            (3)
#define HOT_KEY_PHONE                  (4)
native SetPlayerHotKeyHint(playerid, hotkey, const key[] = "null"); // клавиша в подсказке HOT_KEY_*, "null" - скрыть

// сдвинуть текстдрав (PlayerTextDraw: id + 2048)
native MoveTextDrawForPlayer(playerid, textid, Float:x, Float:y);
native SetPlayerCompass(playerid, bool:enable);                 // компас над радаром; playerid -1 - всем
native SetPlayerCompassTarget(playerid, Float:x, Float:y);      // куда показывает компас

// ----------------------------------------------------------------------------
//  CEF (ОКНА ЛАУНЧЕРА)
// ----------------------------------------------------------------------------
native SendPlayerCefEvent(playerid, const event[]);             // выполнить JS-событие в интерфейсе
native SendPlayerCefEventMobile(playerid, window, action, const data[]); // то же для мобильного лаунчера
native SetPlayerCefCursor(playerid, browserid, bool:enable);    // курсор в окне интерфейса вкл/выкл
native ShowPlayerPhoneVideo(playerid, const video[]);           // видео YouTube в телефоне (ссылка или id)
native HidePlayerPhoneVideo(playerid);

// ----------------------------------------------------------------------------
//  ЧАТ
// ----------------------------------------------------------------------------
#define CHAT_TAB_VIP                   (0)
#define CHAT_TAB_ORG                   (1)
#define CHAT_TAB_ORG_NONRP             (2)
#define CHAT_TAB_FAMILY                (3)
#define CHAT_TAB_NONRP                 (4)
#define CHAT_TAB_SHOUT                 (5)
#define CHAT_TAB_WHISPER               (6)
#define CHAT_TAB_WORK                  (7)

#define CHAT_MODE_HIDDEN               (0)
#define CHAT_MODE_LIGHT                (1)
#define CHAT_MODE_FULL                 (2)

native SetPlayerChatTab(playerid, tab, bool:show);              // вкладка чата CHAT_TAB_*
native SetPlayerChatTabFormat(playerid, const tab[], const format[]); // формат вкладки: {NICK}, {TEXT}
native SetPlayerChatMode(playerid, mode);                       // CHAT_MODE_*: скрыт / лёгкий / полный
native LockPlayerChatMode(playerid, bool:lock);                 // F5/F10 не меняют режим чата
native SetPlayerCommandHelper(playerid, bool:enable);           // подсказки команд в чате

// свои вкладки чата (1..31; CHAT_TAB_* занимают 1..8): текст, набранный во вкладке, уходит как "/command текст"
// exclusive - сообщения вкладки не дублируются в общий чат
native CreatePlayerChatTab(playerid, tabid, const name[], const command[], color = 0xFFFFFFFF, bool:exclusive = false);
native RemovePlayerChatTab(playerid, tabid);
native SendClientMessageToTab(playerid, tabid, color, const message[], bool:only = false); // строка во вкладку; only - не в общий чат; playerid -1 - всем
native SetPlayerChatBadge(playerid, badgeid, const text[], icon = 0); // значок в шапке чата (0..255), icon - символ 0xF000..0xF0FF; "" - убрать

// жалобы в списке у админа (кнопки "Ответить" /ot, "Следить" /re)
native AddAdminReport(adminid, fromid, const text[]);
native RemoveAdminReport(adminid, fromid);

// ----------------------------------------------------------------------------
//  ДИАЛОГИ
// ----------------------------------------------------------------------------
native SetPlayerDialogText(playerid, const text[]);             // заменить текст открытого диалога (показывается заново с тем же ID)
native SetPlayerDialogRemember(playerid, bool:enable);          // списки диалогов запоминают выбранный пункт
native SetPlayerDialogStyle(playerid, style);                   // оформление диалогов 1..5

// ----------------------------------------------------------------------------
//  НАВИГАТОР И КАРТА
// ----------------------------------------------------------------------------
native SetPlayerWaypoint(playerid, Float:x, Float:y, Float:z);  // метка навигатора на карте
native RemovePlayerWaypoint(playerid);
native SetPlayerNavigation(playerid, const Float:points[], count, bool:onfoot = true, bool:tilt = true); // маршрут стрелки: {x,y,z,r,...}
forward OnPlayerNavigationPoint(playerid, index);               // игрок дошёл до точки маршрута

// ----------------------------------------------------------------------------
//  МЫШЬ, DISCORD
// ----------------------------------------------------------------------------
#define MOUSE_TRACK_OFF                (0)
#define MOUSE_TRACK_ALL                (1)
#define MOUSE_TRACK_DELTA              (2)
native SetPlayerMouseTracking(playerid, mode, Float:mindelta = 0.0); // MOUSE_TRACK_*: DELTA - только сдвиг больше mindelta
forward OnPlayerMouseMove(playerid, Float:x, Float:y);

native SetPlayerDiscordStatus(playerid, const text[]);          // свой текст в Discord, "" - убрать

// ----------------------------------------------------------------------------
//  СВОИ ОКНА CEF (БРАУЗЕРЫ ЛАУНЧЕРА): ОКНО 0 - ИНТЕРФЕЙС ARIZONA, СВОИ - 1..7
// ----------------------------------------------------------------------------
// width/height 0 - до края экрана
native Cef_CreateBrowser(playerid, browserid, const url[], x = 0, y = 0, width = 0, height = 0);
native Cef_DestroyBrowser(playerid, browserid);
native Cef_SetBrowserPos(playerid, browserid, x, y);
native Cef_SetBrowserUrl(playerid, browserid, const url[]);
native Cef_ExecuteJS(playerid, browserid, const code[]);        // выполнить JS (0 - в интерфейсе Arizona)
native Cef_SendMessage(playerid, uid, const text[]);            // во все окна: cef.onMessage(uid, text)
native Cef_ToggleBrowser(playerid, browserid);                  // показать / скрыть
native Cef_SetBrowserFocus(playerid, browserid, bool:focus);    // курсор и клавиатура в окне
forward OnPlayerCefEvent(playerid, const event[], uid);         // страница вызвала cef.SendMessage(text, uid) (и события интерфейса Arizona)

// Ответы интерфейса одним событием, параметры уже разобраны ("событие|a|b", "событие|[...]", "событие|{...}").
// Параметры читаются только внутри OnPlayerCefResponse. Строки - в cp1251.
forward OnPlayerCefResponse(playerid, const event[], argc);
native Cef_GetArgCount();                                       // сколько параметров
native Cef_GetArg(index, dest[], size = sizeof dest);           // параметр строкой (с 0), 0 - нет такого
native Cef_GetArgInt(index);                                    // параметр числом
native Float:Cef_GetArgFloat(index);
native Cef_GetArgByName(const name[], dest[], size = sizeof dest); // поле объекта JSON {"name": ...}
native Cef_GetArgIntByName(const name[]);
native Float:Cef_GetArgFloatByName(const name[]);
native Cef_GetRaw(dest[], size = sizeof dest);                  // вся строка ответа как пришла
// разрешение экрана игрока (окно игры в пикселях). Плагин спрашивает его сам, когда интерфейс загрузился
native RequestPlayerScreenSize(playerid);                       // спросить ещё раз, ответ придёт в OnPlayerScreenSize
native GetPlayerScreenSize(playerid, &width, &height);          // последнее известное разрешение; 0 - ещё не знаем
forward OnPlayerScreenSize(playerid, width, height);            // разрешение пришло или изменилось
