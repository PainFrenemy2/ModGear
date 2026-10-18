main(){}
// qawno (open.mp): режим совместимости с SA-MP - без строгих тегов и предупреждений обёртки a_samp
#define SAMP_COMPAT
#include <a_samp>
#include <Pawn.CMD>
#include <sscanf2>
#include <ModGear>
#include <ModGear_Interface>
#include <ModGear_Player>
#include <ModGear_Vehicle>
#include <ModGear_World>
#include <ModGear_Media>
#include <ModGear_Voice>
#include <ModGear_NPC>
#include <streamer>
#if defined _INC_open_mp
	// в open.mp SendClientMessage сам форматирует, плагин fmt не нужен
	#define SendClientMessagef SendClientMessage
	#define SendRconCommandf SendRconCommand
	#define RCON_SERVER_NAME "name"
#else
	#define RCON_SERVER_NAME "hostname"
	#include <fmt>
#endif
#include <Pawn.RakNet>
#pragma dynamic 16384
// ======================================================================
//  Тестовый сервер ModGear: /menu - все функции плагина по разделам
// ======================================================================
#define MODGEAR_VERSION_MAJOR ((ModGear_IncludeFileVersion >> 8) & 0xF)
#define MODGEAR_VERSION_MINOR ((ModGear_IncludeFileVersion >> 4) & 0xF)
#define MODGEAR_VERSION_PATCH (ModGear_IncludeFileVersion & 0xF)

#define SCM                             SendClientMessage
#define SPD                             ShowPlayerDialog
#define SCMf                            SendClientMessagef
#define SPDf                            ShowPlayerDialogf

#define C_MAIN                          0xFAAC58FF
#define C_OK                            0x9ACD32FF
#define C_ERR                           0xBE2D2DFF

#define DLG_MAIN                        (1)
#define DLG_LIST                        (2)
#define DLG_INPUT                       (3)
#define DLG_INFO                        (4)
#define DLG_HANDLING                    (5)
#define DLG_HANDLING_VALUE              (6)
#define DIALOG_MUSIC                    (50)
#define DIALOG_MUSIC_CAR                (51)
#define DIALOG_MUSIC_SPEAKER            (52)
#define DIALOG_MUSIC_OBJECT             (53)
#define DIALOG_MUSIC_OBJECT_STOP        (54)
#define DIALOG_MUSIC_SELF               (55)
#define DIALOG_MUSIC_BG                 (56)
#define DIALOG_VIDEO                    (60)
#define DIALOG_VIDEO_PLAY               (61)
#define DIALOG_VIDEO_SEEK               (62)
#define DIALOG_VIDEO_PHONE              (63)
#define DIALOG_VOICE                    (70)
#define DIALOG_VOICE_INPUT              (71)
#define DIALOG_VOICE_INFO               (72)

#define TEST_TAB                        (10)    // своя вкладка чата
#define TEST_BROWSER                    (1)     // своё окно CEF

// ---- пункты меню ----
#define K_TOGGLE                        (0)     // вкл / выкл
#define K_INPUT                         (1)     // ввод значения
#define K_ACTION                        (2)     // действие

enum
{
	// интерфейс и худ
	I_HUD, I_RADAR, I_SPEEDO, I_HINTS, I_HUNGER, I_WANTED, I_FUEL, I_MILAGE, I_FUELTYPE, I_DIALOGTYPE,
	I_MAP, I_DISCORD, I_DIALOGSAVE, I_CHATALPHA, I_RADIUS, I_SNOW, I_CHATBOARD, I_WALLHACK, I_AIM, I_GEO,
	I_CINEMA, I_COMPASS, I_HOTKEY, I_HUDINFO, I_DISCORDTEXT, I_CEFEVENT, I_BROWSER, I_MOUSE, I_WAYPOINT, I_NAVI,
	// чат и ник
	I_CHATMODE, I_CHATLOCK, I_CMDHELP, I_DLGSTYLE, I_DLGREMEMBER, I_CHATTAB, I_OWNTAB, I_TABMSG, I_BADGE,
	I_REPORT, I_TAGSTYLE, I_TAGTITLE, I_TAGSTATUS, I_TAGPLATE, I_TAGGRAD, I_TAGICONS, I_BUBBLE, I_DMGTEXT, I_TAGRESET,
	// игрок
	I_SKIN, I_WEAPON, I_HEAL, I_JETPACK, I_JETVIS, I_COLLISION, I_TAGICON, I_BONE, I_GREENZONE, I_SPEEDLIMIT,
I_ANTIPAUSE, I_AIRBRAKE, I_TARGET, I_WHITELIST, I_TRACER,
	I_ACCADD, I_ACCDEL, I_ACCEDIT, I_ACCDIRT, I_ACCTEXT,
	I_WALKSTYLE, I_REMWEAPON, I_TPFADE, I_FADE, I_FIRSTPERSON, I_FLIGHT, I_BIKEFALL, I_REDIRECT,
	// машина
	I_CAR, I_REPAIR, I_PART, I_PARTDEL, I_VINYL, I_NEON, I_SMOKE, I_HEADLIGHT, I_WHEELCOLOR, I_TURNCOLOR,
	I_PLATE, I_TONER, I_CAMBER, I_WHEELSIZE, I_CALIPERS, I_HYDRA, I_EXHAUST, I_STROBE, I_SIREN, I_HORN,
	I_DRIFT, I_DIRT, I_TOWCAR, I_HANDLING, I_HANDMULTI,
	// мир
	I_PARTICLE, I_SKYLOGO, I_OBJDIRT, I_OBJSCALE, I_ZONEWORLD, I_CLIMB, I_WEATHER, I_TIME, I_WATER, I_EFFECTS,
	// медиа и голос
	I_MUSIC, I_VIDEO, I_VOICE, I_WEBCAM,
	// NPC
	I_GUARD, I_GUARDCAR, I_GUARDVEH, I_ENEMY, I_PEDGRAD, I_PEDCLEAR,
	// Arizona: новое
	I_SKINTEX, I_SKINRESET, I_STAMINA, I_BREATH, I_WATERDRIVE, I_FLYCAR, I_BIKECRUISE, I_FPAIM, I_CJANIMS, I_HEADMOVE,
	I_LIGHTMODE, I_NOISE, I_LAUNCH, I_ROUTE, I_LOADSCREEN, I_CANSWIM, I_COMPASSTGT, I_NITRO, I_NITROCOLOR, I_SILENT,
	I_HEADTEX, I_PORTAL, I_PORTALDEL, I_LINE, I_VISOBJ, I_LOADMAP,
	// NPC: действия и параметры (над охранником или первым врагом)
	I_PEDGO, I_PEDSTOP, I_PEDSHOOTPOS, I_PEDSHOOTME, I_PEDSHOOTPED, I_PEDANIM, I_PEDANGLE, I_PEDSKIN, I_PEDTP, I_PEDWORLD,
	I_PEDCOL, I_PEDGOD, I_PEDWALK, I_PEDHAT, I_PEDBUBBLE, I_PEDSTREAM, I_PEDSYNC, I_PEDRESPAWN, I_PEDINFO,
	// проверка остальных функций
	I_CARINFO, I_PLAYERINFO, I_RESETTUNE, I_COPYHAND, I_REMSKINTEX, I_WTRACER, I_ACCMAT, I_BUBBLEDIST, I_TABFORMAT, I_DLGTEXT,
	I_REPORTDEL, I_TDMOVE, I_PARTCLEAR, I_CEFCURSOR, I_BROWSERTEST, I_CEFLOG, I_VISMOVE, I_MEDIATITLE, I_CARAUDIO, I_KINOTITLE,
	// хендлинг через диалог
	I_HANDDLG,
	I_COUNT
};

enum E_ITEM { itId, itMenu, itKind, itName[48], itHint[200], bool:itCar };
static const gItems[][E_ITEM] =
{
	{I_HUD, 0, K_INPUT, "Тип худа", "0 выкл, 1 PUBG, 2 Vice City, 3 San Andreas, 4-6 мобильные", false},
	{I_RADAR, 0, K_INPUT, "Тип радара", "0 выкл, 1-2 свой, 3 Vice City", false},
	{I_SPEEDO, 0, K_INPUT, "Тип спидометра", "0 выкл, 1-2 мобильный, 3 продвинутый, 4 упрощённый", false},
	{I_HINTS, 0, K_TOGGLE, "Подсказки клавиш", "", false},
	{I_HUNGER, 0, K_INPUT, "Сытость на худе", "0-100", false},
	{I_WANTED, 0, K_INPUT, "Уровень розыска", "0-7", false},
	{I_FUEL, 0, K_INPUT, "Топливо", "0-300 литров", false},
	{I_MILAGE, 0, K_INPUT, "Пробег", "0-999999", false},
	{I_FUELTYPE, 0, K_INPUT, "Тип топлива", "0 бензин, 1 электро, 2 скрыть значок", false},
	{I_DIALOGTYPE, 1, K_INPUT, "Тип диалогов", "0 полный, 1 упрощённый, 2 простой", false},
	{I_MAP, 0, K_TOGGLE, "Зеркальная карта", "", false},
	{I_DISCORD, 1, K_TOGGLE, "Позиция в Discord", "", false},
	{I_DIALOGSAVE, 1, K_TOGGLE, "Сохранять текст в диалогах", "", false},
	{I_CHATALPHA, 2, K_TOGGLE, "Прозрачный чат", "", false},
	{I_RADIUS, 12, K_INPUT, "Дальность 3D-метки навигатора", "0-500 м, 0 - выключить", false},
	{I_SNOW, 0, K_TOGGLE, "Снегопад", "", false},
	{I_CHATBOARD, 0, K_TOGGLE, "Текст над игроками", "", false},
	{I_WALLHACK, 0, K_TOGGLE, "Игроки сквозь стены", "", false},
	{I_AIM, 0, K_INPUT, "Прицел", "0 обычный, 1-10 свои", false},
	{I_GEO, 0, K_TOGGLE, "Местоположение на худе", "", false},
	{I_CINEMA, 0, K_TOGGLE, "Кинорежим (скрыть худ)", "", false},
	{I_COMPASS, 0, K_TOGGLE, "Компас над радаром", "", false},
	{I_HOTKEY, 0, K_INPUT, "Подсказка клавиши", "Тип (0 рация, 1 рация фракции, 2 рация семьи, 3 взаимодействие, 4 телефон) и клавиша\nПример: 4 P", false},
	{I_HUDINFO, 0, K_ACTION, "Данные сервера на худе", "", false},
	{I_DISCORDTEXT, 1, K_INPUT, "Свой текст в Discord", "Текст (пусто - убрать)", false},
	{I_CEFEVENT, 1, K_INPUT, "JS в интерфейсе Arizona", "JS-код, например:\nwindow.executeEvent('event.setActiveView', '[\"Inventory\"]');", false},
	{I_BROWSER, 1, K_TOGGLE, "Своё окно CEF (example.com)", "", false},
	{I_MOUSE, 1, K_TOGGLE, "Слежение за курсором", "", false},
	{I_WAYPOINT, 12, K_TOGGLE, "Метка на карте (кинотеатр)", "", false},
	{I_NAVI, 12, K_ACTION, "Маршрут навигатора (3 точки)", "", false},

	{I_CHATMODE, 2, K_INPUT, "Режим чата", "0 скрыт, 1 лёгкий, 2 полный", false},
	{I_CHATLOCK, 2, K_TOGGLE, "Запретить менять режим чата", "", false},
	{I_CMDHELP, 2, K_TOGGLE, "Подсказки команд", "", false},
	{I_DLGSTYLE, 1, K_INPUT, "Оформление диалогов", "1-5", false},
	{I_DLGREMEMBER, 1, K_TOGGLE, "Диалоги помнят пункт", "", false},
	{I_CHATTAB, 2, K_INPUT, "Вкладка чата Arizona", "Номер вкладки (вкл / выкл):\n0 VIP, 1 орг, 2 орг нонРП, 3 семья, 4 нонРП, 5 крик, 6 шёпот, 7 работа", false},
	{I_OWNTAB, 2, K_TOGGLE, "Своя вкладка \"Тест\" (/testtab)", "", false},
	{I_TABMSG, 2, K_INPUT, "Сообщение во вкладку \"Тест\"", "Текст", false},
	{I_BADGE, 2, K_TOGGLE, "Значок в шапке чата", "", false},
	{I_REPORT, 2, K_ACTION, "Тестовая жалоба себе (админ)", "", false},
	{I_TAGSTYLE, 3, K_TOGGLE, "Ники Arizona (для всех)", "", false},
	{I_TAGTITLE, 3, K_INPUT, "Строка над ником", "Текст (пусто - убрать)", false},
	{I_TAGSTATUS, 3, K_INPUT, "Описание под ником", "Текст (пусто - убрать)", false},
	{I_TAGPLATE, 3, K_INPUT, "Табличка над ником", "Текст (пусто - убрать)", false},
	{I_TAGGRAD, 3, K_INPUT, "Переливающийся ник", "Оттенок1 насыщенность1 оттенок2 насыщенность2\n(оттенок 0-359, насыщенность 0-100)\nПример: 0 100 60 100", false},
	{I_TAGICONS, 3, K_TOGGLE, "Значки у ника", "", false},
	{I_BUBBLE, 3, K_INPUT, "Облачко над собой", "Текст", false},
	{I_DMGTEXT, 3, K_TOGGLE, "Цифры урона при попадании", "", false},
	{I_TAGRESET, 3, K_ACTION, "Сбросить ник", "", false},

	{I_SKIN, 4, K_INPUT, "Скин", "ID скина", false},
	{I_WEAPON, 6, K_INPUT, "Оружие", "ID оружия", false},
	{I_HEAL, 6, K_ACTION, "Вылечиться (100 HP + 100 брони)", "", false},
	{I_JETPACK, 7, K_ACTION, "Джетпак", "", false},
	{I_JETVIS, 4, K_INPUT, "Видимость джетпаков", "0 скрыты все, 1 видны все, 2 скрыт свой", false},
	{I_COLLISION, 4, K_TOGGLE, "Столкновения с игроками", "", false},
	{I_TAGICON, 3, K_INPUT, "Значок над ником (icons.txd)", "Имя текстуры из /models/icons.txd (пусто - убрать)", false},
	{I_BONE, 4, K_INPUT, "Размер кости", "Кость и размер, -1 - сброс\nПример: 2 1.5 (голова)", false},
	{I_GREENZONE, 6, K_TOGGLE, "Зелёная зона", "", false},
	{I_SPEEDLIMIT, 7, K_INPUT, "Ограничитель скорости", "Скорость (0 - снять) и модель (0 - все машины)\nПример: 60 0", false},
	{I_ANTIPAUSE, 7, K_TOGGLE, "Без паузы при сворачивании", "", false},
	{I_AIRBRAKE, 7, K_TOGGLE, "Полёт (админ)", "", false},
	{I_TARGET, 6, K_INPUT, "Режим цели", "0-2", false},
	{I_WHITELIST, 6, K_TOGGLE, "Только кулак / Deagle / M4", "", false},
	{I_TRACER, 6, K_TOGGLE, "Красные лазерные трассеры", "", false},
	{I_ACCADD, 5, K_INPUT, "Аксессуар: надеть", "Слот (0-9), модель, кость\nПример: 0 19078 1", false},
	{I_ACCDEL, 5, K_INPUT, "Аксессуар: снять", "Слот (0-9)", false},
	{I_ACCEDIT, 5, K_INPUT, "Аксессуар: редактор", "Слот (0-9)", false},
	{I_ACCDIRT, 5, K_INPUT, "Аксессуар: грязь", "Слот, грязь (0-15), текстура (0 грязь, 1 потёртости)\nПример: 0 15 0", false},
	{I_ACCTEXT, 5, K_INPUT, "Аксессуар: надпись", "Слот и текст\nПример: 0 ModGear", false},
	{I_WALKSTYLE, 4, K_INPUT, "Походка", "Группа анимаций GTA, пусто - как у скина\nПример: fatman, sexywoman, oldman, gang1, drunkman", false},
	{I_REMWEAPON, 6, K_INPUT, "Забрать одно оружие", "ID оружия 1-46", false},
	{I_TPFADE, 7, K_ACTION, "Телепорт на 30 м вперёд с проявлением", "", false},
	{I_FADE, 7, K_TOGGLE, "Затемнить экран", "", false},
	{I_FIRSTPERSON, 6, K_TOGGLE, "Вид от первого лица", "", false},
	{I_FLIGHT, 7, K_TOGGLE, "Полёт без потолка высоты", "", false},
	{I_BIKEFALL, 7, K_TOGGLE, "Не слетать с мотоцикла", "", false},
	{I_REDIRECT, 7, K_INPUT, "Переподключить на сервер", "Адрес и порт\nПример: 127.0.0.1 7777", false},

	{I_CAR, 8, K_INPUT, "Выдать машину", "Модель (400-611, свои 612+)", false},
	{I_REPAIR, 8, K_ACTION, "Починить и перевернуть", "", true},
	{I_PART, 8, K_INPUT, "Деталь тюнинга", "Слот (1 капот, 2 спойлер, 3 глушитель, 4 диски) и модель\nПример: 4 6753", true},
	{I_PARTDEL, 8, K_INPUT, "Снять деталь", "Модель детали", true},
	{I_VINYL, 8, K_INPUT, "Винил", "Винил (0-17) и цвет (0-255)\nПример: 2 1", true},
	{I_NEON, 8, K_INPUT, "Неон", "Цвет 1-255, 0 - выкл", true},
	{I_SMOKE, 8, K_INPUT, "Цвет дыма", "0-255", true},
	{I_HEADLIGHT, 8, K_INPUT, "Цвет фар", "0-255", true},
	{I_WHEELCOLOR, 8, K_INPUT, "Цвет дисков", "0-255 (только диски лаунчера)", true},
	{I_TURNCOLOR, 8, K_INPUT, "Подсветка поворотников", "Цвет RRGGBBAA (AA - яркость)\nПример: FF0000FF", true},
	{I_PLATE, 8, K_INPUT, "Номер", "Тип (0-51), номер, регион\nПример: 3 7777BK AC", true},
	{I_TONER, 8, K_INPUT, "Тонировка", "Часть (0-6), R G B, яркость\nПример: 0 0 0 0 200", true},
	{I_CAMBER, 8, K_INPUT, "Развал колёс", "Ось (0 перед, 1 зад), угол (-15..15), вынос (-0.1..0.1)\nПример: 0 10 0.05", true},
	{I_WHEELSIZE, 8, K_INPUT, "Размер колёс", "0.0-1.05\nПример: 0.95", true},
	{I_CALIPERS, 8, K_INPUT, "Тормозные суппорты", "Модель (0 - убрать)", true},
	{I_HYDRA, 9, K_INPUT, "Гидравлика", "0 выкл, 1 поднять, 2 опустить", true},
	{I_EXHAUST, 9, K_TOGGLE, "Хлопки из выхлопа", "", true},
	{I_STROBE, 9, K_INPUT, "Стробоскопы", "Режим (0 - выкл), скорость, луч (0/1)\nПример: 1 1.0 1", true},
	{I_SIREN, 9, K_TOGGLE, "Мигалка / сирена", "", true},
	{I_HORN, 9, K_INPUT, "Звук клаксона", "ID звука, -1 - стандартный", true},
	{I_DRIFT, 9, K_TOGGLE, "Дрифт-машина", "", true},
	{I_DIRT, 9, K_INPUT, "Грязь и износ", "Грязь (0-100), скорость набора (0 - не пачкается), износ (0-17)\nПример: 80 1.0 10", true},
	{I_TOWCAR, 9, K_TOGGLE, "Машина на крыше (сцепка)", "", true},
	{I_HANDLING, 10, K_INPUT, "Хендлинг: параметр", "Поле и значение\nmaxspeed, acceleration, inertia, brake, traction, suspension...\nПример: maxspeed 300", true},
	{I_HANDDLG, 10, K_ACTION, "Хендлинг: список параметров", "", true},
	{I_HANDMULTI, 10, K_INPUT, "Хендлинг: два параметра сразу", "Поле значение поле значение\nПример: maxspeed 300 acceleration 25", true},

	{I_PARTICLE, 11, K_INPUT, "Эффект частиц (10 с)", "Название эффекта GTA\nПример: explosion_large, fire_large, prt_spark, smoke50lit", false},
	{I_SKYLOGO, 11, K_INPUT, "Надпись в небе", "Текст \"строка1;строка2\" (пусто - убрать)", false},
	{I_OBJDIRT, 11, K_INPUT, "Грязь экрана кинотеатра", "0-15", false},
	{I_OBJSCALE, 11, K_INPUT, "Масштаб экрана кинотеатра", "Например 1.5 (1.0 - обычный)", false},
	{I_ZONEWORLD, 11, K_INPUT, "Названия зон мира", "Виртуальный мир", false},
	{I_CLIMB, 11, K_TOGGLE, "Запрет лазать (интерьер 0)", "", false},
	{I_WEATHER, 11, K_INPUT, "Погода", "ID погоды", false},
	{I_TIME, 11, K_INPUT, "Время", "Час 0-23", false},
	{I_WATER, 11, K_INPUT, "Уровень воды (потоп)", "Высота над морем, 0 - обычная вода\nПример: 30", false},
	{I_EFFECTS, 11, K_TOGGLE, "Отключить эффекты мира", "", false},

	{I_MUSIC, 13, K_ACTION, "Музыка (/music)", "", false},
	{I_VIDEO, 13, K_ACTION, "Видео и кинотеатр (/kinomenu)", "", false},
	{I_VOICE, 13, K_ACTION, "Голосовой чат (/voice)", "", false},
	{I_WEBCAM, 13, K_TOGGLE, "Вебкамера (видят игроки рядом)", "", false},

	{I_GUARD, 14, K_TOGGLE, "Охранник", "", false},
	{I_GUARDCAR, 14, K_ACTION, "Охранник: сесть ко мне в машину", "", true},
	{I_GUARDVEH, 14, K_INPUT, "Охранник: садиться за мной в машину", "0 - нет, 1 - с анимацией (по умолчанию), 2 - сразу на место", false},
	{I_ENEMY, 14, K_ACTION, "Враг: пед атакует меня", "", false},
	{I_PEDGRAD, 14, K_INPUT, "Переливающееся имя охранника", "Оттенок1 насыщенность1 оттенок2 насыщенность2\nПример: 200 80 260 80", false},
	{I_PEDCLEAR, 14, K_ACTION, "Убрать врагов", "", false},
	{I_SKINTEX, 4, K_INPUT, "Цвет текстуры скина", "Текстура и цвет RRGGBBAA\nПример: ballas1 FF3030FF", false},
	{I_SKINRESET, 4, K_ACTION, "Сбросить текстуры скина", "", false},
	{I_STAMINA, 7, K_TOGGLE, "Бесконечная выносливость", "", false},
	{I_BREATH, 7, K_INPUT, "Запас воздуха под водой", "0-1150", false},
	{I_WATERDRIVE, 9, K_TOGGLE, "Машины ездят по воде", "", false},
	{I_FLYCAR, 9, K_TOGGLE, "Летающая машина", "", false},
	{I_BIKECRUISE, 7, K_TOGGLE, "Круиз на мотоцикле (W)", "", false},
	{I_FPAIM, 6, K_TOGGLE, "Прицел от первого лица", "", false},
	{I_CJANIMS, 4, K_TOGGLE, "Походка CJ у себя", "", false},
	{I_HEADMOVE, 4, K_TOGGLE, "Голова за камерой", "", false},
	{I_LIGHTMODE, 4, K_INPUT, "Свет от игрока", "0 выкл, 1 пятно у игрока, 2 пятно впереди", false},
	{I_NOISE, 0, K_TOGGLE, "Помехи на экране", "", false},
	{I_LAUNCH, 9, K_TOGGLE, "Launch control (W+S)", "", false},
	{I_ROUTE, 12, K_TOGGLE, "Маршрут на радаре (чекпоинт 300 м впереди)", "", false},
	{I_LOADSCREEN, 0, K_INPUT, "Экран загрузки", "Время в мс, 0 - скрыть\nПример: 3000", false},
	{I_CANSWIM, 7, K_TOGGLE, "Плавание в режиме Vice City", "", false},
	{I_COMPASSTGT, 0, K_ACTION, "Компас на точку спавна", "", false},
	{I_NITRO, 9, K_TOGGLE, "Нитро Arizona", "", true},
	{I_NITROCOLOR, 9, K_INPUT, "Цвет пламени нитро", "0 обычное, 1 красное, 2 оранжевое, 3 розовое", true},
	{I_SILENT, 9, K_TOGGLE, "Двигатель без звука", "", true},
	{I_HEADTEX, 8, K_INPUT, "Текстура фар", "Имя текстуры, пусто - стандартная", true},
	{I_PORTAL, 12, K_ACTION, "Портал: вход передо мной, выход в 15 м", "", false},
	{I_PORTALDEL, 12, K_ACTION, "Убрать портал", "", false},
	{I_LINE, 11, K_TOGGLE, "3D-линия в небо от игрока", "", false},
	{I_VISOBJ, 11, K_TOGGLE, "Объект только для отрисовки", "", false},
	{I_LOADMAP, 11, K_INPUT, "Загрузить карту клиента", "Имя файла из data\\maps\nПример: streamer_map_mirage.bin", false},
	{I_PEDGO, 14, K_ACTION, "Пед: идти ко мне", "", false},
	{I_PEDSTOP, 14, K_ACTION, "Пед: стоп", "", false},
	{I_PEDSHOOTPOS, 14, K_ACTION, "Пед: выстрелить в точку передо мной", "", false},
	{I_PEDSHOOTME, 14, K_ACTION, "Пед: выстрелить в меня", "", false},
	{I_PEDSHOOTPED, 14, K_ACTION, "Охранник: выстрелить во врага", "", false},
	{I_PEDANIM, 14, K_INPUT, "Пед: анимация", "Библиотека и имя\nПример: DANCING dance_loop", false},
	{I_PEDANGLE, 14, K_INPUT, "Пед: угол", "0-360", false},
	{I_PEDSKIN, 14, K_INPUT, "Пед: скин", "ID скина", false},
	{I_PEDTP, 14, K_ACTION, "Пед: телепорт ко мне", "", false},
	{I_PEDWORLD, 14, K_ACTION, "Пед: мой мир и интерьер", "", false},
	{I_PEDCOL, 14, K_TOGGLE, "Пед: столкновения", "", false},
	{I_PEDGOD, 14, K_TOGGLE, "Пед: бессмертие", "", false},
	{I_PEDWALK, 14, K_INPUT, "Пед: походка", "Группа анимаций\nПример: fatman, gang1, woman", false},
	{I_PEDHAT, 14, K_TOGGLE, "Пед: каска на голове", "", false},
	{I_PEDBUBBLE, 14, K_INPUT, "Пед: облачко над головой", "Текст", false},
	{I_PEDSTREAM, 14, K_INPUT, "Пед: дальность прорисовки", "Метры, 0 - общая", false},
	{I_PEDSYNC, 14, K_INPUT, "Педы: частота синхронизации", "50-1000 мс", false},
	{I_PEDRESPAWN, 14, K_ACTION, "Пед: возродить", "", false},
	{I_PEDINFO, 14, K_ACTION, "Пед: все параметры", "", false},
	{I_CARINFO, 8, K_ACTION, "Всё, что поставлено машине", "", true},
	{I_PLAYERINFO, 7, K_ACTION, "Всё, что поставлено мне", "", false},
	{I_RESETTUNE, 8, K_ACTION, "Сбросить весь тюнинг машины", "", true},
	{I_COPYHAND, 10, K_INPUT, "Скопировать хендлинг с машины", "ID машины-источника", true},
	{I_REMSKINTEX, 4, K_INPUT, "Убрать одну текстуру скина", "Имя текстуры\nПример: ballas1", false},
	{I_WTRACER, 6, K_TOGGLE, "Трассер только для Desert Eagle", "", false},
	{I_ACCMAT, 5, K_INPUT, "Материал аксессуара", "Слот\nПример: 0", false},
	{I_BUBBLEDIST, 3, K_TOGGLE, "Облачка на любом расстоянии", "", false},
	{I_TABFORMAT, 2, K_INPUT, "Формат своей вкладки чата", "Формат с {NICK} и {TEXT}\nПример: [{NICK}] {TEXT}", false},
	{I_DLGTEXT, 1, K_ACTION, "Заменить текст открытого диалога", "", false},
	{I_REPORTDEL, 2, K_ACTION, "Убрать свою жалобу", "", false},
	{I_TDMOVE, 0, K_ACTION, "Сдвинуть текстдрав", "", false},
	{I_PARTCLEAR, 11, K_ACTION, "Убрать все эффекты частиц", "", false},
	{I_CEFCURSOR, 1, K_TOGGLE, "Курсор в интерфейсе Arizona", "", false},
	{I_BROWSERTEST, 1, K_ACTION, "Своё окно CEF: сдвиг, ссылка, JS, сообщение", "", false},
	{I_CEFLOG, 1, K_TOGGLE, "Показывать ответы интерфейса в чате", "", false},
	{I_VISMOVE, 11, K_ACTION, "Объект отрисовки: сдвиг, поворот, масштаб", "", false},
	{I_MEDIATITLE, 13, K_INPUT, "Название трека по ссылке", "Ссылка YouTube", false},
	{I_CARAUDIO, 13, K_INPUT, "Радио машины: управление", "play, time N или title", true},
	{I_KINOTITLE, 13, K_ACTION, "Название видео в кинотеатре", "", false}
};
static const gMenuNames[][] = {"Худ и экран", "Диалоги, CEF и мышь", "Чат и жалобы", "Ник и облачка", "Игрок: внешний вид", "Игрок: аксессуары", "Игрок: оружие и бой", "Игрок: режимы и перемещение", "Машина: выдать и тюнинг", "Машина: эффекты и режимы", "Машина: хендлинг", "Мир: объекты, эффекты, погода", "Мир: навигация и порталы", "Медиа, голос и вебкамера", "NPC (педы)"};

new gVal[MAX_PLAYERS][I_COUNT];             // значение / вкл-выкл пункта
new bool:gSet[MAX_PLAYERS][I_COUNT];        // значение задавали
new gMenu[MAX_PLAYERS], gItem[MAX_PLAYERS], gListMap[MAX_PLAYERS][64];
new gPlayerCar[MAX_PLAYERS], bool:gMobile[MAX_PLAYERS];
new gGuard[MAX_PLAYERS] = {-1, ...};
#define MAX_ENEMIES                     (8)
new gEnemy[MAX_ENEMIES] = {-1, ...};
new bool:gDmgText[MAX_PLAYERS];
new bool:gCefLog[MAX_PLAYERS], PlayerText:gTestTD[MAX_PLAYERS] = {PlayerText:INVALID_TEXT_DRAW, ...};

// ---- урон на сервере ----
new Float:gHealth[MAX_PLAYERS], Float:gArmour[MAX_PLAYERS], bool:gAlive[MAX_PLAYERS], gLastPedHit[MAX_PLAYERS] = {-1, ...}, gLastPedHitTime[MAX_PLAYERS];

// ---- мир ----
new kinema, kinoVideo, kinoUrl[200], kinemaLS, kinoVideoLS; // экран у аэропорта ЛВ и экран Legislative Square (ЛС)
new gVoiceLocal, gVoiceRadio;
new dialogTitle[64];

// ---- голос ----
#define MAX_VOICE_CH                    (32)
new gVoiceName[MAX_VOICE_CH][32], bool:gVoiceUsed[MAX_VOICE_CH], bool:gVoiceEcho, bool:gVoiceRelay = true, bool:gVoiceTalk[MAX_PLAYERS], gVoiceAction[MAX_PLAYERS];
// ---- музыка ----
#define MAX_SPEAKERS                    (20)
#define SPEAKER_MODEL                   (2229)
#define SPEAKER_DISTANCE                (40.0)
enum enmSpeaker { bool:spActive, spObject, bool:spDynamic, bool:spOwnObject, Float:spPos[3], spAudio, spUrl[160] };
new SpeakerInfo[MAX_SPEAKERS][enmSpeaker];
enum enminfVehicle { vAudioID, vAudioUrl[160] };
new VI[MAX_VEHICLES][enminfVehicle];

enum infcreate { icVehID, icModel, Float:icPos[4], icHood, icSpoiler, icExhaust, icWheels, icColor[2], icPlateType, icPlate[10], icRegion[5] };
new InfoCreateVehicle[][infcreate] =
{
	{0, 612, {1325.7351,1279.0178,10.5827,359.3188}, 3123, 3128, 3124, 6753, {6, 1}, 1, "777AAA", "77"},
	{0, 613, {1312.5683,1279.0178,10.5837,359.3188}, 3115, 3121, 3117, 6754, {6, 1}, 2, "A777AA", "77"},
	{0, 614, {1319.1517,1279.0178,10.5837,359.3188}, 3144, 0, 0, 6753, {6, 1}, 3, "7777BK", "AC"},
	{0, 3201, {1305.9849,1279.0178,10.5837,359.3188}, 0, 0, 0, 6753, {6, 1}, 4, "7777KA", "11"}
};

// ======================================================================
//  Игроки
// ======================================================================
public OnPlayerConnect(playerid)
{
	for(new i; i < I_COUNT; i++) gVal[playerid][i] = 0, gSet[playerid][i] = false;
	gVal[playerid][I_COLLISION] = 1;
	gPlayerCar[playerid] = 0;
	gDmgText[playerid] = false;
	gMobile[playerid] = false;

	new version[24];
	GetPlayerVersion(playerid, version, sizeof(version));
	if(!strcmp(version, "Arizona PC")) SetPlayerLauncherStatus(playerid, true);
	else if(!strcmp(version, "arizona-mobile"))
	{
		SetPlayerLauncherStatus(playerid, true);
		gMobile[playerid] = true;
		SetTimerEx("AuthMobile", 2000, false, "d", playerid);
	}
	else SetPlayerLauncherStatus(playerid, false);

	SetPlayerInterface(playerid, Custom_Hud, 5);
	SetPlayerInterface(playerid, Custom_Radar, 2);
	SetPlayerInterface(playerid, Custom_Hints, 1);
	gVal[playerid][I_HUD] = 5, gVal[playerid][I_RADAR] = 2, gVal[playerid][I_HINTS] = 1;
	gSet[playerid][I_HUD] = gSet[playerid][I_RADAR] = true;

	SCMf(playerid, C_MAIN, "Тестовый сервер плагина {FFFFFF}ModGear %d.%d.%dv", MODGEAR_VERSION_MAJOR, MODGEAR_VERSION_MINOR, MODGEAR_VERSION_PATCH);
	SCM(playerid, C_MAIN, "Все функции: {FFFFFF}/menu{FAAC58}, команды: {FFFFFF}/cmd");
	for(new s = WEAPONSKILL_PISTOL; s <= WEAPONSKILL_SNIPERRIFLE; s++) SetPlayerSkillLevel(playerid, s, 1000);
	return 1;
}
forward AuthMobile(playerid);
public AuthMobile(playerid)
{
	// вход мобильного лаунчера без окна авторизации
	SendCustom(playerid, 9, 8, 0);
	SendPlayerCefEventMobile(playerid, 9, 3, "success");
	SendCustom(playerid, 62, 9, 0);
	return 1;
}
stock SendCustom(playerid, ...)
{
	new BitStream:bs = BS_New();
	BS_WriteValue(bs, PR_UINT8, 220);
	for(new i = 1; i < numargs(); i++) BS_WriteValue(bs, PR_UINT8, getarg(i));
	PR_SendPacket(bs, playerid);
	BS_Delete(bs);
	return 1;
}
public OnPlayerDisconnect(playerid, reason)
{
	gTestTD[playerid] = PlayerText:INVALID_TEXT_DRAW;
	gCefLog[playerid] = false;
	if(gPlayerCar[playerid]) DestroyVehicle(gPlayerCar[playerid]), gPlayerCar[playerid] = 0;
	FireGuard(playerid);
	return 1;
}
#define SPAWN_X (1319.2843)
#define SPAWN_Y (1261.8027)
#define SPAWN_Z (10.8203)
#define SPAWN_A (358.2892)
public OnPlayerRequestClass(playerid, classid)
{
	SetSpawnInfo(playerid, NO_TEAM, 102, SPAWN_X, SPAWN_Y, SPAWN_Z, SPAWN_A, 0, 0, 0, 0, 0, 0);
	SpawnPlayer(playerid);
	return 1;
}
public OnPlayerSpawn(playerid)
{
	SetPlayerSkin(playerid, gSet[playerid][I_SKIN] ? gVal[playerid][I_SKIN] : 102);
	GivePlayerWeapon(playerid, 31, 5000);
	GivePlayerWeapon(playerid, 24, 5000);
	SetPlayerVirtualWorld(playerid, 0);
	SetPlayerInterior(playerid, 0);
	SetHP(playerid, 100.0, 0.0);
	if(gVoiceLocal) Voice_AddPlayer(gVoiceLocal, playerid);
	return 1;
}
public OnPlayerDeath(playerid, killerid, reason)
{
	gAlive[playerid] = false;
	// килфид: убил пед - его имя (SendPedDeathMessage), иначе игрок или "сам"
	if(killerid == INVALID_PLAYER_ID && gLastPedHit[playerid] != -1 && gettime() - gLastPedHitTime[playerid] <= 10 && IsValidPed(gLastPedHit[playerid])) SendPedDeathMessage(gLastPedHit[playerid], playerid, reason);
	else SendDeathMessage(killerid, playerid, reason);
	gLastPedHit[playerid] = -1;
	// смерть самой игры (взрыв, вода): спавн на точке спавна, а не в 0,0,0
	SetSpawnInfo(playerid, NO_TEAM, GetPlayerSkin(playerid), SPAWN_X, SPAWN_Y, SPAWN_Z, SPAWN_A, 0, 0, 0, 0, 0, 0);
	// все педы, которые атаковали погибшего (бандиты, охранники), прекращают атаку
	new bool:isped;
	for(new p; p < GetPedPoolSize() + 1; p++)
		if(IsValidPed(p) && GetPedAttackTarget(p, isped) == playerid && !isped) PedStopAttack(p);
	return 1;
}
public OnPlayerCommandPerformed(playerid, cmd[], params[], result, flags)
{
	if(result == -1) return SCM(playerid, C_MAIN, "[Инфо] {FFFFFF}Нет такой команды. Все команды - {FAAC58}/cmd");
	return 1;
}
public OnPlayerKeyStateChange(playerid, newkeys, oldkeys)
{
	// фары на ALT в машине
	if((newkeys & KEY_ACTION) && !(oldkeys & KEY_ACTION))
	{
		new carid = GetPlayerVehicleID(playerid);
		if(carid)
		{
			new engine, lights, alarm, doors, bonnet, boot, objective;
			GetVehicleParamsEx(carid, engine, lights, alarm, doors, bonnet, boot, objective);
			SetVehicleParamsEx(carid, engine, lights == VEHICLE_PARAMS_ON ? VEHICLE_PARAMS_OFF : VEHICLE_PARAMS_ON, alarm, doors, bonnet, boot, objective);
		}
	}
	return 1;
}
stock Float:GetPointZPos(const Float:fX, const Float:fY, &Float:fZ = 0.0)
{
	if(!((-3000.0 < fX < 3000.0) && (-3000.0 < fY < 3000.0))) return 0.0;
	static File:s_hMap;
	if(!s_hMap)
	{
		s_hMap = fopen("SAfull.hmap", io_read);
		if(!s_hMap) return 0.0;
	}
	new afZ[1];
	fseek(s_hMap, ((6000 * (-floatround(fY, floatround_tozero) + 3000) + (floatround(fX, floatround_tozero) + 3000)) << 1));
	fblockread(s_hMap, afZ);
	return (fZ = ((afZ[0] >>> 16) * 0.01));
}
public OnPlayerClickMap(playerid, Float:fX, Float:fY, Float:fZ)
{
	GetPointZPos(fX, fY, fZ);
	if(GetPlayerState(playerid) == PLAYER_STATE_DRIVER) SetVehiclePos(GetPlayerVehicleID(playerid), fX, fY, fZ + 1.0);
	else SetPlayerPos(playerid, fX, fY, fZ + 1.0);
	SetPlayerInterior(playerid, 0);
	SetPlayerVirtualWorld(playerid, 0);
	return 1;
}

// ======================================================================
//  Урон на сервере
//  Здоровье хранит сервер и ставит игроку итоговое значение: клиент Arizona сам урон не отнимает.
//  Урон от игроков - из OnPlayerGiveDamage (сообщает стрелок), остальной - из OnPlayerTakeDamage,
//  чтобы одно попадание не считалось дважды.
// ======================================================================
stock SetHP(playerid, Float:health, Float:armour)
{
	gHealth[playerid] = health;
	gArmour[playerid] = armour;
	gAlive[playerid] = health > 0.0;
	SetPlayerHealth(playerid, health);
	SetPlayerArmour(playerid, armour);
	return 1;
}
stock DamagePlayer(playerid, issuerid, Float:amount, weaponid)
{
	if(!gAlive[playerid] || amount <= 0.0) return 0;
	new Float:shown = amount;
	// падение, утопление и огонь броня не держит
	if(gArmour[playerid] > 0.0 && weaponid != 54 && weaponid != 53 && weaponid != 37)
	{
		if(amount <= gArmour[playerid]) gArmour[playerid] -= amount, amount = 0.0;
		else amount -= gArmour[playerid], gArmour[playerid] = 0.0;
		SetPlayerArmour(playerid, gArmour[playerid]);
	}
	gHealth[playerid] -= amount;
	if(issuerid != INVALID_PLAYER_ID && gDmgText[issuerid])
	{
		new Float:x, Float:y, Float:z;
		GetPlayerPos(playerid, x, y, z);
		ShowDamageText(issuerid, shown, x, y, z + 1.0);
	}
	if(gHealth[playerid] <= 0.0) return InstantDeath(playerid, issuerid, weaponid);
	SetPlayerHealth(playerid, gHealth[playerid]);
	return 1;
}
// смерть без экрана "Wasted": сразу на точку спавна с полным здоровьем
stock InstantDeath(playerid, killerid, weaponid)
{
	OnPlayerDeath(playerid, killerid, weaponid); // там же килфид
	SetPlayerPos(playerid, SPAWN_X, SPAWN_Y, SPAWN_Z); // из машины тоже высаживает
	SetPlayerFacingAngle(playerid, SPAWN_A);
	SetCameraBehindPlayer(playerid);
	ResetPlayerWeapons(playerid);
	OnPlayerSpawn(playerid); // мир, интерьер, оружие, 100 HP
	return 1;
}
public OnPlayerGiveDamage(playerid, damagedid, Float:amount, weaponid, bodypart)
{
	if(damagedid != INVALID_PLAYER_ID && IsPlayerConnected(damagedid)) DamagePlayer(damagedid, playerid, amount, weaponid);
	return 1;
}
public OnPlayerTakeDamage(playerid, issuerid, Float:amount, weaponid, bodypart)
{
	if(issuerid == INVALID_PLAYER_ID)
	{
		DamagePlayer(playerid, INVALID_PLAYER_ID, amount, weaponid);
		return 1;
	}
	// хозяина бьют - охранник атакует обидчика
	new ped = gGuard[playerid];
	if(ped != -1 && !IsPedDead(ped)) GuardAttack(ped, playerid, issuerid);
	return 1;
}
public OnPlayerDamageByPed(playerid, pedid, Float:amount, weaponid, bodypart)
{
	gLastPedHit[playerid] = pedid; // для килфида, если от этого урона умрёт
	gLastPedHitTime[playerid] = gettime();
	DamagePlayer(playerid, INVALID_PLAYER_ID, amount, weaponid);
	// хозяина бьёт NPC (бандит) - охранник атакует его
	new guard = gGuard[playerid];
	if(guard != -1 && guard != pedid && !IsPedDead(guard)) GuardAttackPed(guard, pedid);
	return 1;
}

// ======================================================================
//  Меню
// ======================================================================
CMD:menu(playerid) { return ShowMainMenu(playerid); }

stock ShowMainMenu(playerid)
{
	new s[1024];
	for(new m; m < sizeof(gMenuNames); m++) format(s, sizeof(s), "%s{FAAC58}%d. {FFFFFF}%s\n", s, m + 1, gMenuNames[m]);
	return SPD(playerid, DLG_MAIN, DIALOG_STYLE_LIST, dialogTitle, s, "Выбрать", "Закрыть");
}
stock ItemStatus(playerid, idx, dest[], size = sizeof(dest))
{
	new id = gItems[idx][itId];
	switch(gItems[idx][itKind])
	{
		case K_TOGGLE: format(dest, size, "%s", gVal[playerid][id] ? ("{9ACD32}вкл") : ("{B83434}выкл"));
		case K_INPUT: if(gSet[playerid][id]) format(dest, size, "{FFFFFF}%d", gVal[playerid][id]); else format(dest, size, "{969696}ввод");
		default: format(dest, size, "{969696}>>");
	}
	return 1;
}
stock ShowItemMenu(playerid, menu)
{
	new s[4096], st[24], n;
	gMenu[playerid] = menu;
	s = "{CCCCCC}Функция\t{CCCCCC}Значение\n";
	for(new i; i < sizeof(gItems); i++)
	{
		if(gItems[i][itMenu] != menu) continue;
		ItemStatus(playerid, i, st);
		format(s, sizeof(s), "%s%s%s\t%s\n", s, gItems[i][itCar] ? ("{FAAC58}[авто] {FFFFFF}") : ("{FFFFFF}"), gItems[i][itName], st);
		gListMap[playerid][n++] = i;
	}
	new title[64];
	format(title, sizeof(title), "{FAAC58}%s", gMenuNames[menu]);
	return SPD(playerid, DLG_LIST, DIALOG_STYLE_TABLIST_HEADERS, title, s, "Выбрать", "Назад");
}
stock AskInput(playerid, idx, const error[] = "")
{
	new s[400];
	format(s, sizeof(s), "%s%s{FFFFFF}%s", error, error[0] ? ("\n\n") : (""), gItems[idx][itHint]);
	return SPD(playerid, DLG_INPUT, DIALOG_STYLE_INPUT, gItems[idx][itName], s, "Ок", "Назад");
}
// ---- хендлинг через диалог: список параметров, выбрал - ввёл значение ----
// только параметры, которые доходят до клиента Arizona (RPC 161)
static const gHandField[][] = {"maxspeed", "acceleration", "inertia", "brake", "brakebias", "steering", "traction", "tractionloss", "tractionbias", "suspforce", "suspdamping", "susphighspd", "suspupper", "susplower", "suspbias", "antidive"};
static const gHandName[][] = {"Максимальная скорость", "Ускорение", "Инерция двигателя", "Сила торможения", "Баланс тормозов (перед/зад)", "Угол поворота руля", "Сцепление с дорогой", "Потеря сцепления", "Баланс сцепления (перед/зад)", "Жёсткость подвески", "Демпфирование подвески", "Подвеска на скорости", "Верхний предел подвески", "Нижний предел подвески (высота)", "Баланс подвески (перед/зад)", "Против клевка при торможении"};
new gHandPick[MAX_PLAYERS];

stock ShowHandlingDialog(playerid)
{
	new carid = GetPlayerVehicleID(playerid);
	if(!carid)
	{
		SCM(playerid, C_ERR, "[Ошибка] {FFFFFF}Сядьте в машину.");
		return 0;
	}
	new s[2048], Float:v;
	s = "{CCCCCC}Параметр\t{CCCCCC}Поле\t{CCCCCC}Значение\n";
	for(new i; i < sizeof(gHandField); i++)
	{
		if(GetVehicleHandling(carid, gHandField[i], v)) format(s, sizeof(s), "%s{FFFFFF}%s\t{969696}%s\t{9ACD32}%.3f\n", s, gHandName[i], gHandField[i], v);
		else format(s, sizeof(s), "%s{FFFFFF}%s\t{969696}%s\t{969696}как в handling.cfg\n", s, gHandName[i], gHandField[i]);
	}
	strcat(s, "{B83434}Сбросить весь хендлинг машины\t\t");
	return SPD(playerid, DLG_HANDLING, DIALOG_STYLE_TABLIST_HEADERS, "{FAAC58}Хендлинг машины", s, "Изменить", "Назад");
}

stock OnHandlingDialog(playerid, dialogid, response, listitem, const inputtext[])
{
	new carid = GetPlayerVehicleID(playerid);
	if(dialogid == DLG_HANDLING)
	{
		if(!response) return ShowItemMenu(playerid, gMenu[playerid]);
		if(!carid) return SCM(playerid, C_ERR, "[Ошибка] {FFFFFF}Сядьте в машину.");
		if(listitem == sizeof(gHandField))
		{
			ResetVehicleHandling(carid);
			SCM(playerid, C_OK, "[Хендлинг] {FFFFFF}Весь хендлинг машины сброшен.");
			return ShowHandlingDialog(playerid);
		}
		if(listitem < 0 || listitem >= sizeof(gHandField)) return ShowHandlingDialog(playerid);
		gHandPick[playerid] = listitem;
		new text[160], Float:v;
		if(GetVehicleHandling(carid, gHandField[listitem], v)) format(text, sizeof(text), "{FFFFFF}%s (%s)\nСейчас: {9ACD32}%.3f\n\n{FFFFFF}Новое значение:", gHandName[listitem], gHandField[listitem], v);
		else format(text, sizeof(text), "{FFFFFF}%s (%s)\nСейчас: как в handling.cfg\n\nНовое значение:", gHandName[listitem], gHandField[listitem]);
		return SPD(playerid, DLG_HANDLING_VALUE, DIALOG_STYLE_INPUT, "{FAAC58}Хендлинг", text, "Применить", "Назад");
	}
	if(!response || !carid) return ShowHandlingDialog(playerid);
	new Float:value;
	if(sscanf(inputtext, "f", value))
	{
		SCM(playerid, C_ERR, "[Ошибка] {FFFFFF}Нужно число, например 250 или 0.85");
		return ShowHandlingDialog(playerid);
	}
	SetVehicleHandling(carid, gHandField[gHandPick[playerid]], value);
	SCMf(playerid, C_OK, "[Хендлинг] {FFFFFF}%s = %.3f", gHandName[gHandPick[playerid]], value);
	return ShowHandlingDialog(playerid);
}
public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
	if(dialogid == DLG_HANDLING || dialogid == DLG_HANDLING_VALUE) return OnHandlingDialog(playerid, dialogid, response, listitem, inputtext);
	if(dialogid >= DIALOG_MUSIC && dialogid <= DIALOG_MUSIC_BG) return Music_OnDialog(playerid, dialogid, response, listitem, inputtext);
	if(dialogid >= DIALOG_VIDEO && dialogid <= DIALOG_VIDEO_PHONE) return Video_OnDialog(playerid, dialogid, response, listitem, inputtext);
	if(dialogid >= DIALOG_VOICE && dialogid <= DIALOG_VOICE_INFO) return Voice_OnDialog(playerid, dialogid, response, listitem, inputtext);
	switch(dialogid)
	{
		case DLG_MAIN:
		{
			if(!response || listitem < 0 || listitem >= sizeof(gMenuNames)) return 1;
			return ShowItemMenu(playerid, listitem);
		}
		case DLG_LIST:
		{
			if(!response) return ShowMainMenu(playerid);
			new idx = gListMap[playerid][listitem], id = gItems[idx][itId];
			gItem[playerid] = idx;
			if(gItems[idx][itCar] && !GetPlayerVehicleID(playerid))
			{
				SCM(playerid, C_ERR, "[Ошибка] {FFFFFF}Сядьте в машину.");
				return ShowItemMenu(playerid, gMenu[playerid]);
			}
			if(gItems[idx][itKind] == K_INPUT) return AskInput(playerid, idx);
			if(gItems[idx][itKind] == K_TOGGLE) gVal[playerid][id] = !gVal[playerid][id];
			if(!OnItem(playerid, id, "")) return 1; // пункт открыл своё окно
			return ShowItemMenu(playerid, gMenu[playerid]);
		}
		case DLG_INPUT:
		{
			if(!response) return ShowItemMenu(playerid, gMenu[playerid]);
			new idx = gItem[playerid];
			if(gItems[idx][itCar] && !GetPlayerVehicleID(playerid))
			{
				SCM(playerid, C_ERR, "[Ошибка] {FFFFFF}Сядьте в машину.");
				return ShowItemMenu(playerid, gMenu[playerid]);
			}
			if(!OnItem(playerid, gItems[idx][itId], inputtext)) return 1;
			return ShowItemMenu(playerid, gMenu[playerid]);
		}
	}
	return 1;
}
// значение 0..max или ошибка
stock InputRange(playerid, id, const text[], min, max)
{
	new v;
	if(sscanf(text, "d", v) || v < min || v > max)
	{
		new e[80];
		format(e, sizeof(e), "{B83434}Нужно число от %d до %d", min, max);
		AskInput(playerid, gItem[playerid], e);
		return cellmin;
	}
	gVal[playerid][id] = v;
	gSet[playerid][id] = true;
	return v;
}
stock InputError(playerid, const error[] = "Неверный ввод")
{
	new e[96];
	format(e, sizeof(e), "{B83434}%s", error);
	AskInput(playerid, gItem[playerid], e);
	return 0;
}
// 1 - вернуть в список пунктов, 0 - пункт сам показал окно
stock OnItem(playerid, id, const text[])
{
	new v, carid = GetPlayerVehicleID(playerid), bool:on = bool:gVal[playerid][id];
	switch(id)
	{
		// ---- интерфейс и худ ----
		case I_HUD: { if((v = InputRange(playerid, id, text, 0, 6)) == cellmin) return 0; SetPlayerInterface(playerid, Custom_Hud, v); }
		case I_RADAR: { if((v = InputRange(playerid, id, text, 0, 3)) == cellmin) return 0; SetPlayerInterface(playerid, Custom_Radar, v); }
		case I_SPEEDO: { if((v = InputRange(playerid, id, text, 0, 4)) == cellmin) return 0; SetPlayerInterface(playerid, Custom_Speedometer, v); }
		case I_HINTS: SetPlayerInterface(playerid, Custom_Hints, on);
		case I_HUNGER: { if((v = InputRange(playerid, id, text, 0, 100)) == cellmin) return 0; SetPlayerInterface(playerid, Custom_Hunger, v); }
		case I_WANTED: { if((v = InputRange(playerid, id, text, 0, 7)) == cellmin) return 0; SetPlayerInterface(playerid, Custom_Wanted, v); }
		case I_FUEL: { if((v = InputRange(playerid, id, text, 0, 300)) == cellmin) return 0; SetPlayerInterface(playerid, Custom_Fuel, v); }
		case I_MILAGE: { if((v = InputRange(playerid, id, text, 0, 999999)) == cellmin) return 0; SetPlayerInterface(playerid, Custom_Milage, v); }
		case I_FUELTYPE: { if((v = InputRange(playerid, id, text, 0, 2)) == cellmin) return 0; SetPlayerInterface(playerid, Custom_TypeFuel, v); }
		case I_DIALOGTYPE: { if((v = InputRange(playerid, id, text, 0, 2)) == cellmin) return 0; SetPlayerInterface(playerid, Custom_TypeDialog, v); }
		case I_MAP: SetPlayerInterface(playerid, Custom_Map, on);
		case I_DISCORD: SetPlayerInterface(playerid, Custom_Discord, on);
		case I_DIALOGSAVE: SetPlayerInterface(playerid, Custom_Dialog, on);
		case I_CHATALPHA: SetPlayerInterface(playerid, Custom_Chat, on);
		case I_RADIUS: { if((v = InputRange(playerid, id, text, 0, 500)) == cellmin) return 0; SetPlayerInterface(playerid, Custom_SetRadius, v); }
		case I_SNOW: SetPlayerInterface(playerid, Custom_Show, on);
		case I_CHATBOARD: SetPlayerInterface(playerid, Custom_ChatBoard, on);
		case I_WALLHACK: SetPlayerInterface(playerid, Custom_WallHack, on);
		case I_AIM: { if((v = InputRange(playerid, id, text, 0, 10)) == cellmin) return 0; SetPlayerInterface(playerid, Custom_TypeAim, v); }
		case I_GEO: SetPlayerInterface(playerid, Custom_GeoPosition, on);
		case I_CINEMA: SetPlayerInterface(playerid, Custom_CinemaMode, on);
		case I_COMPASS: SetPlayerCompass(playerid, on);
		case I_HOTKEY:
		{
			new key[24];
			if(sscanf(text, "ds[24]", v, key) || v < 0 || v > 4) return InputError(playerid);
			SetPlayerHotKeyHint(playerid, v, key);
		}
		case I_HUDINFO:
		{
			SetServerHudInfo(1, "Test Server", "ModGear", "RolePlay", 0, 0, 2, playerid);
			SCM(playerid, C_OK, "[Худ] {FFFFFF}Данные сервера отправлены.");
		}
		case I_DISCORDTEXT: SetPlayerDiscordStatus(playerid, text);
		case I_CEFEVENT:
		{
			if(!text[0]) return InputError(playerid, "Введите JS-код");
			SendPlayerCefEvent(playerid, text);
		}
		case I_BROWSER:
		{
			if(on)
			{
				Cef_CreateBrowser(playerid, TEST_BROWSER, "https://example.com", 200, 150, 900, 600);
				SCM(playerid, C_OK, "[CEF] {FFFFFF}Окно открыто. Курсор - /cefcursor, закрыть - снова этот пункт.");
			}
			else Cef_DestroyBrowser(playerid, TEST_BROWSER);
		}
		case I_MOUSE:
		{
			SetPlayerMouseTracking(playerid, on ? MOUSE_TRACK_DELTA : MOUSE_TRACK_OFF, 20.0);
			if(on) SCM(playerid, C_OK, "[Мышь] {FFFFFF}Откройте курсор (F6 / диалог) - координаты пойдут в OnPlayerMouseMove.");
		}
		case I_WAYPOINT:
		{
			if(on) SetPlayerWaypoint(playerid, 1278.93, 1300.0, 10.82);
			else RemovePlayerWaypoint(playerid);
		}
		case I_NAVI:
		{
			new Float:x, Float:y, Float:z;
			GetPlayerPos(playerid, x, y, z);
			new Float:route[12];
			route[0] = x + 15.0, route[1] = y, route[2] = z, route[3] = 3.0;
			route[4] = x + 15.0, route[5] = y + 15.0, route[6] = z, route[7] = 3.0;
			route[8] = x, route[9] = y + 15.0, route[10] = z, route[11] = 3.0;
			SetPlayerNavigation(playerid, route, 3);
			SCM(playerid, C_OK, "[Навигатор] {FFFFFF}Маршрут из 3 точек - идите по стрелке.");
		}

		// ---- чат и ник ----
		case I_CHATMODE: { if((v = InputRange(playerid, id, text, 0, 2)) == cellmin) return 0; SetPlayerChatMode(playerid, v); }
		case I_CHATLOCK: LockPlayerChatMode(playerid, on);
		case I_CMDHELP: SetPlayerCommandHelper(playerid, on);
		case I_DLGSTYLE: { if((v = InputRange(playerid, id, text, 1, 5)) == cellmin) return 0; SetPlayerDialogStyle(playerid, v); }
		case I_DLGREMEMBER: SetPlayerDialogRemember(playerid, on);
		case I_CHATTAB:
		{
			if(sscanf(text, "d", v) || v < 0 || v > 7) return InputError(playerid, "Номер вкладки 0-7");
			// бит v - вкладка включена
			gVal[playerid][id] ^= (1 << v);
			SetPlayerChatTab(playerid, v, bool:(gVal[playerid][id] & (1 << v)));
			SCMf(playerid, C_OK, "[Чат] {FFFFFF}Вкладка %d: %s", v, (gVal[playerid][id] & (1 << v)) ? ("показана") : ("убрана"));
		}
		case I_OWNTAB:
		{
			if(on)
			{
				CreatePlayerChatTab(playerid, TEST_TAB, "Тест", "testtab", 0x66CCFFFF, false);
				SCM(playerid, C_OK, "[Чат] {FFFFFF}Вкладка \"Тест\": текст в ней уходит командой /testtab.");
			}
			else RemovePlayerChatTab(playerid, TEST_TAB);
		}
		case I_TABMSG:
		{
			if(!text[0]) return InputError(playerid, "Введите текст");
			SendClientMessageToTab(playerid, TEST_TAB, 0x66CCFFFF, text);
		}
		case I_BADGE: SetPlayerChatBadge(playerid, 1, on ? ("Тестовый значок") : (""), 0xF013);
		case I_REPORT:
		{
			AddAdminReport(playerid, playerid, "Тестовая жалоба: помогите, застрял в текстурах");
			SCM(playerid, C_OK, "[Жалобы] {FFFFFF}Жалоба добавлена в список (кнопки /ot и /re).");
		}
		case I_TAGSTYLE: SetNameTagStyle(on);
		case I_TAGTITLE: SetPlayerNameTagTitle(playerid, text, 0x66CCFFFF);
		case I_TAGSTATUS: SetPlayerNameTagStatus(playerid, text);
		case I_TAGPLATE: SetPlayerNameTagPlate(playerid, text, 0xFFD700CC, 0x000000FF);
		case I_TAGGRAD:
		{
			new h1, s1, h2, s2;
			if(sscanf(text, "dddd", h1, s1, h2, s2)) return InputError(playerid);
			SetPlayerNameGradient(playerid, h1, s1, h2, s2);
		}
		case I_TAGICONS:
		{
			SetPlayerNameTagParam(playerid, NAMETAG_ICON_SWORDS, on);
			SetPlayerNameTagParam(playerid, NAMETAG_ICON_PILL, on);
		}
		case I_BUBBLE:
		{
			if(!text[0]) return InputError(playerid, "Введите текст");
			SetPlayerChatBubbleEx(playerid, text, 0xFFFFFFFF, 25.0, 6000);
		}
		case I_DMGTEXT: gDmgText[playerid] = on;
		case I_TAGRESET:
		{
			ResetPlayerNameTag(playerid);
			gVal[playerid][I_TAGICONS] = 0;
		}

		// ---- игрок ----
		case I_SKIN: { if((v = InputRange(playerid, id, text, 0, 20000)) == cellmin) return 0; SetPlayerSkin(playerid, v); }
		case I_WEAPON: { if((v = InputRange(playerid, id, text, 1, 46)) == cellmin) return 0; GivePlayerWeapon(playerid, v, 5000); }
		case I_HEAL: SetHP(playerid, 100.0, 100.0);
		case I_JETPACK: SetPlayerSpecialAction(playerid, SPECIAL_ACTION_USEJETPACK);
		case I_JETVIS: { if((v = InputRange(playerid, id, text, 0, 2)) == cellmin) return 0; SetPlayerJetpackVisibility(playerid, v); }
		case I_COLLISION: SetPlayerCollision(playerid, on);
		case I_TAGICON: SetPlayerNameTagIcon(playerid, text, text[0] != 0);
		case I_BONE:
		{
			new Float:sc;
			if(sscanf(text, "dF(1.0)", v, sc)) return InputError(playerid);
			SetPlayerBoneScale(playerid, v, sc);
		}
		case I_GREENZONE: SetPlayerGreenZone(playerid, on);
		case I_SPEEDLIMIT:
		{
			new Float:sp, model;
			if(sscanf(text, "fD(0)", sp, model)) return InputError(playerid);
			SetPlayerSpeedLimit(playerid, sp, model);
			gVal[playerid][id] = floatround(sp), gSet[playerid][id] = true;
		}
		case I_ANTIPAUSE: SetPlayerAntiPause(playerid, on);
		case I_AIRBRAKE: SetPlayerAirBrake(playerid, on, 100.0);
		case I_TARGET: { if((v = InputRange(playerid, id, text, 0, 2)) == cellmin) return 0; SetPlayerTargetMode(playerid, v); }
		case I_WHITELIST:
		{
			new const weapons[] = {0, 24, 31};
			SetPlayerWeaponWhitelist(playerid, on, weapons, sizeof(weapons));
			if(on) GivePlayerWeapon(playerid, 30, 200); // АК - на него переключиться нельзя
		}
		case I_TRACER:
		{
			if(on) SetPlayerBulletTracer(playerid, 0xFFFF0000, 0.5, true);
			else ResetPlayerBulletTracer(playerid);
		}
		case I_WALKSTYLE: SetPlayerWalkStyle(playerid, text);
		case I_REMWEAPON: { if((v = InputRange(playerid, id, text, 1, 46)) == cellmin) return 0; RemovePlayerWeapon(playerid, v); }
		case I_TPFADE:
		{
			new Float:x, Float:y, Float:z, Float:a;
			GetPlayerPos(playerid, x, y, z);
			GetPlayerFacingAngle(playerid, a);
			TeleportPlayerWithFade(playerid, x + 30.0 * floatsin(-a, degrees), y + 30.0 * floatcos(-a, degrees), z + 1.0, a, -1, 1500);
		}
		case I_FADE: FadePlayerScreen(playerid, on, 1000);
		case I_FIRSTPERSON: SetPlayerFirstPerson(playerid, on);
		case I_FLIGHT: SetPlayerFlightLimits(playerid, !on);
		case I_BIKEFALL: SetPlayerBikeFall(playerid, !on);
		case I_REDIRECT:
		{
			new host[64], port;
			if(sscanf(text, "s[64]d", host, port)) return InputError(playerid);
			RedirectPlayer(playerid, host, port);
		}
		case I_ACCADD:
		{
			new slot, model, bone;
			if(sscanf(text, "ddD(1)", slot, model, bone) || slot < 0 || slot > 9) return InputError(playerid);
			SetPlayerAttachedObjectEx(playerid, slot, true, model, bone);
		}
		case I_ACCDEL:
		{
			if(sscanf(text, "d", v) || v < 0 || v > 9) return InputError(playerid, "Слот 0-9");
			RemovePlayerAttachedObjectEx(playerid, v);
		}
		case I_ACCEDIT:
		{
			if(sscanf(text, "d", v) || v < 0 || v > 9) return InputError(playerid, "Слот 0-9");
			if(!IsPlayerAttachedObjectSlotEx(playerid, v)) return InputError(playerid, "Слот пустой");
			EditAttachedObjectEx(playerid, v);
			return 0;
		}
		case I_ACCDIRT:
		{
			new slot, level, tex;
			if(sscanf(text, "ddD(0)", slot, level, tex)) return InputError(playerid);
			SetPlayerAttachedObjectDirt(playerid, slot, level, tex);
		}
		case I_ACCTEXT:
		{
			new slot, t[64];
			if(sscanf(text, "ds[64]", slot, t)) return InputError(playerid);
			SetPlayerAttachedObjectMatText(playerid, slot, 0, t, 90, 24, 1, true, "Arial", 0xFFFFFFFF, 0xFF000000);
		}

		// ---- машина ----
		case I_CAR:
		{
			if((v = InputRange(playerid, id, text, 400, 20000)) == cellmin) return 0;
			if(gPlayerCar[playerid]) DestroyVehicle(gPlayerCar[playerid]);
			new Float:x, Float:y, Float:z, Float:a;
			GetPlayerPos(playerid, x, y, z);
			GetPlayerFacingAngle(playerid, a);
			gPlayerCar[playerid] = CreateVehicle(v, x, y, z + 0.5, a, random(100), random(100), -1);
			if(!gPlayerCar[playerid]) return InputError(playerid, "Такой модели нет");
			AddVehicleComponent(gPlayerCar[playerid], 1010); // нитро для теста (без него индикатор нитро красный)
			PutPlayerInVehicle(playerid, gPlayerCar[playerid], 0);
		}
		case I_REPAIR:
		{
			new Float:a;
			GetVehicleZAngle(carid, a);
			SetVehicleZAngle(carid, a);
			RepairVehicle(carid);
		}
		case I_PART:
		{
			new slot, model;
			if(sscanf(text, "dd", slot, model) || slot < 1 || slot > 4) return InputError(playerid);
			SetVehicleTuningPart(carid, slot, model);
		}
		case I_PARTDEL:
		{
			if(sscanf(text, "d", v)) return InputError(playerid);
			if(!GetVehicleTuningPartSlot(carid, v)) return InputError(playerid, "Такой детали на машине нет");
			RemoveVehicleTuningPart(carid, v);
		}
		case I_VINYL:
		{
			new vinyl, color;
			if(sscanf(text, "dd", vinyl, color)) return InputError(playerid);
			SetVehicleVinyl(carid, vinyl, color);
		}
		case I_NEON: { if((v = InputRange(playerid, id, text, 0, 255)) == cellmin) return 0; SetVehicleNeonColor(carid, v); }
		case I_SMOKE: { if((v = InputRange(playerid, id, text, 0, 255)) == cellmin) return 0; SetVehicleSmokeColor(carid, v); }
		case I_HEADLIGHT: { if((v = InputRange(playerid, id, text, 0, 255)) == cellmin) return 0; SetVehicleHeadlightColor(carid, v); }
		case I_WHEELCOLOR: { if((v = InputRange(playerid, id, text, 0, 255)) == cellmin) return 0; SetVehicleWheelColor(carid, v); }
		case I_TURNCOLOR:
		{
			if(sscanf(text, "h", v)) return InputError(playerid, "Цвет в HEX, например FF0000FF");
			SetVehicleTurnLightColor(carid, v);
		}
		case I_PLATE:
		{
			new type, number[11], region[6];
			if(sscanf(text, "ds[11]s[6]", type, number, region)) return InputError(playerid);
			SetVehiclePlate(carid, type, number, region);
		}
		case I_TONER:
		{
			new part, r, g, b, br;
			if(sscanf(text, "ddddd", part, r, g, b, br)) return InputError(playerid);
			SetVehicleToner(carid, part, r, g, b, br);
		}
		case I_CAMBER:
		{
			new axle, Float:angle, Float:offset;
			if(sscanf(text, "dfF(0.0)", axle, angle, offset)) return InputError(playerid);
			SetVehicleWheelCamber(carid, axle, angle, offset);
		}
		case I_WHEELSIZE:
		{
			new Float:sz;
			if(sscanf(text, "f", sz)) return InputError(playerid);
			SetVehicleWheelSize(carid, sz);
		}
		case I_CALIPERS: { if(sscanf(text, "d", v)) return InputError(playerid); SetVehicleBrakeCalipers(carid, v); }
		case I_HYDRA: { if((v = InputRange(playerid, id, text, 0, 2)) == cellmin) return 0; SetVehicleHydraulics(carid, v); }
		case I_EXHAUST: SetVehicleExhaustShots(carid, on);
		case I_STROBE:
		{
			new mode, Float:speed, beam;
			if(sscanf(text, "dF(1.0)D(0)", mode, speed, beam)) return InputError(playerid);
			SetVehicleStrobeLights(carid, mode, speed, bool:beam);
		}
		case I_SIREN: SetVehicleSiren(carid, on);
		case I_HORN: { if(sscanf(text, "d", v)) return InputError(playerid); SetVehicleHornSound(carid, v); }
		case I_DRIFT:
		{
			SetVehicleDrift(carid, on);
		}
		case I_DIRT:
		{
			new Float:dirt, Float:rate, wear;
			if(sscanf(text, "fF(1.0)D(1)", dirt, rate, wear)) return InputError(playerid);
			SetVehicleDirt(carid, dirt, rate, wear);
		}
		case I_TOWCAR:
		{
			if(on) AttachVehicleToVehicle(carid, 0, 411, 0.0, -0.5, 1.4, 0.0, 0.0, 0.0, 3, 3);
			else DetachVehicleFromVehicle(carid, 0);
		}
		case I_HANDLING:
		{
			new field[32], Float:value;
			if(sscanf(text, "s[32]f", field, value)) return InputError(playerid);
			if(!SetVehicleHandling(carid, field, value)) return InputError(playerid, "Такого параметра нет");
		}
		case I_HANDMULTI:
		{
			new f1[32], f2[32], Float:v1, Float:v2;
			if(sscanf(text, "s[32]fs[32]f", f1, v1, f2, v2)) return InputError(playerid);
			if(!SetVehicleHandling(carid, f1, v1, f2, v2)) return InputError(playerid, "Таких параметров нет");
		}

		// ---- NPC: действия ----
		case I_PEDGO, I_PEDSTOP, I_PEDSHOOTPOS, I_PEDSHOOTME, I_PEDSHOOTPED, I_PEDANIM, I_PEDANGLE, I_PEDSKIN, I_PEDTP, I_PEDWORLD,
			I_PEDCOL, I_PEDGOD, I_PEDWALK, I_PEDHAT, I_PEDBUBBLE, I_PEDSTREAM, I_PEDRESPAWN, I_PEDINFO:
		{
			new ped = TestPed(playerid);
			if(ped == -1) return InputError(playerid, "Сначала наймите охранника или вызовите врага");
			new Float:x, Float:y, Float:z, Float:a;
			GetPlayerPos(playerid, x, y, z);
			GetPlayerFacingAngle(playerid, a);
			switch(id)
			{
				case I_PEDGO: { SetPedFollowPlayer(ped, -1); PedGoTo(ped, x, y, z, PED_MOVE_RUN); }
				case I_PEDSTOP: PedStop(ped);
				case I_PEDSHOOTPOS: PedShootAt(ped, x + 8.0 * floatsin(-a, degrees), y + 8.0 * floatcos(-a, degrees), z);
				case I_PEDSHOOTME: PedShootAtPlayer(ped, playerid);
				case I_PEDSHOOTPED:
				{
					new enemy = -1;
					for(new i; i < MAX_ENEMIES; i++) if(gEnemy[i] != -1) { enemy = gEnemy[i]; break; }
					if(gGuard[playerid] == -1 || enemy == -1) return InputError(playerid, "Нужны и охранник, и враг");
					PedShootAtPed(gGuard[playerid], enemy);
				}
				case I_PEDANIM:
				{
					new lib[32], name[32];
					if(sscanf(text, "s[32]s[32]", lib, name)) return InputError(playerid);
					PedApplyAnimation(ped, lib, name, 4.1, 1);
				}
				case I_PEDANGLE: { if((v = InputRange(playerid, id, text, 0, 360)) == cellmin) return 0; SetPedAngle(ped, float(v)); }
				case I_PEDSKIN: { if((v = InputRange(playerid, id, text, 0, 20000)) == cellmin) return 0; SetPedSkin(ped, v); }
				case I_PEDTP: SetPedPos(ped, x + 1.5, y, z);
				case I_PEDWORLD: { SetPedVirtualWorld(ped, GetPlayerVirtualWorld(playerid)); SetPedInterior(ped, GetPlayerInterior(playerid)); }
				case I_PEDCOL: SetPedCollision(ped, on);
				case I_PEDGOD: SetPedInvulnerable(ped, on);
				case I_PEDWALK: SetPedAnimGroup(ped, text);
				case I_PEDHAT:
				{
					if(on) SetPedAttachedObject(ped, 0, 18638, 2, 0.15, 0.0, 0.0);
					else RemovePedAttachedObject(ped, 0);
				}
				case I_PEDBUBBLE: SetPedChatBubble(ped, text, 0xFFFFFFFF, 20.0, 5000);
				case I_PEDSTREAM: { if((v = InputRange(playerid, id, text, 0, 1000)) == cellmin) return 0; SetPedStreamDistance(ped, float(v)); }
				case I_PEDRESPAWN: RespawnPed(ped, 100.0);
				case I_PEDINFO: { ShowPedInfo(playerid, ped); return 0; }
			}
		}
		case I_PEDSYNC: { if((v = InputRange(playerid, id, text, 50, 1000)) == cellmin) return 0; SetPedSyncRate(v); }

		// ---- проверка остальных функций ----
		case I_CARINFO: { ShowCarInfo(playerid, carid); return 0; }
		case I_PLAYERINFO: { ShowMyInfo(playerid); return 0; }
		case I_RESETTUNE: ResetVehicleTuning(carid);
		case I_COPYHAND:
		{
			if((v = InputRange(playerid, id, text, 1, MAX_VEHICLES - 1)) == cellmin) return 0;
			if(!CopyVehicleHandling(v, carid)) return InputError(playerid, "У той машины хендлинг не задан");
		}
		case I_REMSKINTEX: { if(!RemovePlayerSkinTexture(playerid, text)) return InputError(playerid, "Такой текстуры не задано"); }
		case I_WTRACER: SetPlayerWeaponTracer(playerid, 24, on ? 0xFF00FF00 : 0, on ? 1.0 : 0.0, false);
		case I_ACCMAT:
		{
			if((v = InputRange(playerid, id, text, 0, 9)) == cellmin) return 0;
			SetPlayerAttachedObjectMaterial(playerid, v, 0, "metalbarrier", "sam_camo", 0xFFFFFFFF);
		}
		case I_BUBBLEDIST: SetChatBubbleNoDistance(on);
		case I_TABFORMAT: SetPlayerChatTabFormat(playerid, "Тест", text);
		case I_DLGTEXT:
		{
			SPD(playerid, DLG_INFO, DIALOG_STYLE_MSGBOX, "Диалог", "Старый текст - через 2 секунды заменится", "Ок", "");
			SetTimerEx("SwapDialogText", 2000, false, "d", playerid);
			return 0;
		}
		case I_REPORTDEL: RemoveAdminReport(playerid, playerid);
		case I_TDMOVE:
		{
			if(gTestTD[playerid] == PlayerText:INVALID_TEXT_DRAW)
			{
				gTestTD[playerid] = CreatePlayerTextDraw(playerid, 100.0, 300.0, "ModGear: TextDraw");
				PlayerTextDrawShow(playerid, gTestTD[playerid]);
			}
			MoveTextDrawForPlayer(playerid, _:gTestTD[playerid] + 2048, 100.0 + float(random(300)), 200.0 + float(random(150)));
		}
		case I_PARTCLEAR: DestroyAllParticles(playerid);
		case I_CEFCURSOR: SetPlayerCefCursor(playerid, 0, on);
		case I_BROWSERTEST:
		{
			if(!gVal[playerid][I_BROWSER]) return InputError(playerid, "Сначала откройте своё окно CEF (раздел интерфейса)");
			Cef_SetBrowserPos(playerid, TEST_BROWSER, 300, 200);
			Cef_SetBrowserUrl(playerid, TEST_BROWSER, "https://example.org");
			Cef_ExecuteJS(playerid, TEST_BROWSER, "document.body.style.background = '#223'");
			Cef_SendMessage(playerid, 1, "ModGear test");
			Cef_ToggleBrowser(playerid, TEST_BROWSER);
			SCM(playerid, C_OK, "[CEF] {FFFFFF}Окно сдвинуто, ссылка и фон сменены, окно скрыто (пункт ещё раз - показать).");
		}
		case I_CEFLOG: gCefLog[playerid] = on;
		case I_VISMOVE:
		{
			if(!gVal[playerid][I_VISOBJ]) return InputError(playerid, "Сначала включите объект отрисовки (Arizona: новое)");
			new Float:x, Float:y, Float:z;
			GetPlayerPos(playerid, x, y, z);
			SetVisualObjectPos(playerid, 1, x + 3.0, y + 3.0, z);
			SetVisualObjectRot(playerid, 1, 0.0, 0.0, float(random(360)));
			SetVisualObjectScale(playerid, 1, 0.5, 0.5, 3.0);
		}
		case I_MEDIATITLE:
		{
			new title[128];
			if(!text[0]) return InputError(playerid);
			if(Media_GetTitle(text, title)) SCMf(playerid, C_OK, "[Медиа] {FFFFFF}%s", title);
			else SCM(playerid, C_MAIN, "[Медиа] {FFFFFF}Название ещё узнаём - повторите через пару секунд.");
		}
		case I_CARAUDIO:
		{
			new stream = Audio_GetVehicleStreamID(carid), cmd[8], arg;
			if(stream == -1) return InputError(playerid, "В машине не играет радио (/music)");
			if(sscanf(text, "s[8]D(0)", cmd, arg)) return InputError(playerid);
			if(!strcmp(cmd, "play", true)) Audio_PlayStream(stream);
			else if(!strcmp(cmd, "time", true)) Audio_SetStreamTime(stream, arg);
			else
			{
				new title[128];
				if(Audio_GetTitle(stream, title)) SCMf(playerid, C_OK, "[Радио] {FFFFFF}%s", title);
				else SCM(playerid, C_MAIN, "[Радио] {FFFFFF}Название ещё узнаём.");
			}
		}
		case I_KINOTITLE:
		{
			new title[128];
			if(!kinoVideo) return InputError(playerid, "В кинотеатре ничего не включено (/kinomenu)");
			if(Video_GetTitle(kinoVideo, title)) SCMf(playerid, C_OK, "[Кино] {FFFFFF}%s", title);
			else SCM(playerid, C_MAIN, "[Кино] {FFFFFF}Название ещё узнаём.");
		}

		// ---- Arizona: новое ----
		case I_SKINTEX:
		{
			new tex[64], color;
			if(sscanf(text, "s[64]x", tex, color)) return InputError(playerid);
			SetPlayerSkinTexture(playerid, tex, color);
		}
		case I_SKINRESET: ResetPlayerSkinTextures(playerid);
		case I_STAMINA: SetPlayerInfiniteStamina(playerid, on);
		case I_BREATH: { if((v = InputRange(playerid, id, text, 0, 1150)) == cellmin) return 0; SetPlayerBreath(playerid, float(v)); }
		case I_WATERDRIVE: SetPlayerDriveOnWater(playerid, on);
		case I_FLYCAR: SetPlayerFlyingCar(playerid, on);
		case I_BIKECRUISE: SetPlayerBikeCruise(playerid, on);
		case I_FPAIM: SetPlayerFirstPersonAim(playerid, on);
		case I_CJANIMS: SetPlayerCJAnims(playerid, on);
		case I_HEADMOVE: SetPlayerHeadMove(playerid, on);
		case I_LIGHTMODE: { if((v = InputRange(playerid, id, text, 0, 2)) == cellmin) return 0; SetPlayerLightMode(playerid, v); }
		case I_NOISE: SetPlayerScreenNoise(playerid, on);
		case I_LAUNCH: SetPlayerLaunchControl(playerid, on);
		case I_ROUTE:
		{
			if(on)
			{
				new Float:x, Float:y, Float:z, Float:a;
				GetPlayerPos(playerid, x, y, z);
				GetPlayerFacingAngle(playerid, a);
				SetPlayerRaceCheckpoint(playerid, 1, x + 300.0 * floatsin(-a, degrees), y + 300.0 * floatcos(-a, degrees), z, 0.0, 0.0, 0.0, 8.0);
			}
			else DisablePlayerRaceCheckpoint(playerid);
			SetPlayerRadarRoute(playerid, on);
		}
		case I_LOADSCREEN: { if((v = InputRange(playerid, id, text, 0, 60000)) == cellmin) return 0; ShowPlayerLoadScreen(playerid, v); }
		case I_CANSWIM: SetPlayerCanSwim(playerid, on);
		case I_COMPASSTGT: { SetPlayerCompass(playerid, true); SetPlayerCompassTarget(playerid, 1319.28, 1261.80); }
		case I_NITRO: SetVehicleNitro(carid, on);
		case I_NITROCOLOR: { if((v = InputRange(playerid, id, text, 0, 3)) == cellmin) return 0; SetVehicleNitroColor(carid, v); }
		case I_SILENT: SetVehicleSilentEngine(carid, on);
		case I_HEADTEX: SetVehicleHeadlightTexture(carid, text);
		case I_PORTAL:
		{
			new Float:x, Float:y, Float:z, Float:a;
			GetPlayerPos(playerid, x, y, z);
			GetPlayerFacingAngle(playerid, a);
			new Float:dx = floatsin(-a, degrees), Float:dy = floatcos(-a, degrees);
			SetPortal(0, 1, x + 3.0 * dx, y + 3.0 * dy, z, -dx, -dy, 0.0);
			SetPortal(0, 2, x + 15.0 * dy, y - 15.0 * dx, z, dx, dy, 0.0);
			SetPlayerPortals(-1, true);
		}
		case I_PORTALDEL: RemovePortal(0);
		case I_LINE:
		{
			if(on) CreateLine(playerid, 0, LINE_POINT_PLAYER, playerid, 0.0, 0.0, 0.5, LINE_POINT_PLAYER, playerid, 0.0, 0.0, 30.0, 0xFF3030FF, 0.1);
			else DestroyLine(playerid, 0);
		}
		case I_VISOBJ:
		{
			if(on)
			{
				new Float:x, Float:y, Float:z, Float:a;
				GetPlayerPos(playerid, x, y, z);
				GetPlayerFacingAngle(playerid, a);
				CreateVisualObject(playerid, 1, 3524, x + 4.0 * floatsin(-a, degrees), y + 4.0 * floatcos(-a, degrees), z - 1.0, 0.0, 0.0, a, 1.5, 1.5, 1.5);
			}
			else DestroyVisualObject(playerid, 1);
		}
		case I_LOADMAP: { if(!text[0]) return InputError(playerid); LoadPlayerMap(playerid, text); }

		// ---- мир ----
		case I_PARTICLE:
		{
			if(!text[0]) return InputError(playerid, "Введите название эффекта");
			new Float:x, Float:y, Float:z, Float:a, name[32];
			format(name, sizeof(name), "%s", text);
			GetPlayerPos(playerid, x, y, z);
			GetPlayerFacingAngle(playerid, a);
			x += 3.0 * floatsin(-a, degrees);
			y += 3.0 * floatcos(-a, degrees);
			CreateParticle(-1, name, x, y, z, GetPlayerInterior(playerid), 1000);
			SetTimerEx("StopTestParticle", 10000, false, "sfffd", name, x, y, z, GetPlayerInterior(playerid));
		}
		case I_SKYLOGO: SetPlayerSkyLogo(playerid, text, text[0] != 0);
		case I_OBJDIRT: { if((v = InputRange(playerid, id, text, 0, 15)) == cellmin) return 0; SetObjectDirt(kinema, v); }
		case I_OBJSCALE:
		{
			new Float:sc;
			if(sscanf(text, "f", sc) || sc <= 0.0) return InputError(playerid);
			SetObjectScale(kinema, sc);
		}
		case I_ZONEWORLD: { if((v = InputRange(playerid, id, text, 0, 100000)) == cellmin) return 0; SetPlayerZoneNamesWorld(playerid, v); }
		case I_CLIMB: SetInteriorClimbing(playerid, 0, on);
		case I_WEATHER: { if((v = InputRange(playerid, id, text, 0, 255)) == cellmin) return 0; SetPlayerWeather(playerid, v); }
		case I_TIME: { if((v = InputRange(playerid, id, text, 0, 23)) == cellmin) return 0; SetPlayerTime(playerid, v, 0); }
		case I_WATER:
		{
			new Float:level;
			if(sscanf(text, "f", level)) return InputError(playerid);
			if(level > 0.0) SetPlayerWaterLevel(playerid, level, 0.0, 5000); else SetPlayerWaterLevel(playerid, 0.0);
		}
		case I_EFFECTS: SetPlayerWorldEffects(playerid, !on);

		// ---- медиа и голос ----
		case I_MUSIC: { Music_ShowMenu(playerid); return 0; }
		case I_HANDDLG: { ShowHandlingDialog(playerid); return 0; }
		case I_VIDEO: { Video_ShowMenu(playerid); return 0; }
		case I_VOICE: { Voice_ShowMenu(playerid); return 0; }
		case I_WEBCAM:
		{
			if(!on) Webcam_Stop(playerid);
			else
			{
				Webcam_Start(playerid);
				if(Webcam_IsLinked(playerid)) SCM(playerid, C_MAIN, "[Вебкамера] {FFFFFF}Запрос отправлен, ждём ответ клиента...");
				else SCM(playerid, C_MAIN, "[Вебкамера] {FFFFFF}Ждём подключения AZRTC к медиасерверу...");
			}
		}

		// ---- NPC ----
		case I_GUARD:
		{
			if(on) HireGuard(playerid);
			else FireGuard(playerid);
		}
		case I_GUARDCAR:
		{
			if(gGuard[playerid] == -1) { SCM(playerid, C_ERR, "[NPC] {FFFFFF}Сначала наймите охранника."); return 1; }
			// -1 - первое свободное пассажирское место; с анимацией (тронетесь - сядет сразу)
			if(!PedEnterVehicle(gGuard[playerid], carid, -1, false)) { SCM(playerid, C_ERR, "[NPC] {FFFFFF}Свободных мест нет."); return 1; }
		}
		case I_GUARDVEH:
		{
			if(gGuard[playerid] == -1) { SCM(playerid, C_ERR, "[NPC] {FFFFFF}Сначала наймите охранника."); return 1; }
			if((v = InputRange(playerid, id, text, 0, 2)) == cellmin) return 0;
			SetPedFollowVehicle(gGuard[playerid], v);
		}
		case I_ENEMY: SpawnEnemy(playerid);
		case I_PEDGRAD:
		{
			new h1, s1, h2, s2;
			if(gGuard[playerid] == -1) return InputError(playerid, "Сначала наймите охранника");
			if(sscanf(text, "dddd", h1, s1, h2, s2)) return InputError(playerid);
			SetPedNameGradient(gGuard[playerid], h1, s1, h2, s2);
		}
		case I_PEDCLEAR:
		{
			for(new i; i < MAX_ENEMIES; i++) if(gEnemy[i] != -1) DestroyPed(gEnemy[i]), gEnemy[i] = -1;
		}
	}
	return 1;
}

forward StopTestParticle(const name[], Float:x, Float:y, Float:z, interior);
public StopTestParticle(const name[], Float:x, Float:y, Float:z, interior)
{
	DestroyParticle(-1, name, x, y, z, interior, 1000);
	return 1;
}
public OnPlayerMouseMove(playerid, Float:x, Float:y)
{
	new t[48];
	format(t, sizeof(t), "~w~mouse %.0f %.0f", x, y);
	GameTextForPlayer(playerid, t, 800, 4);
	return 1;
}
public OnPlayerNavigationPoint(playerid, index)
{
	SCMf(playerid, C_OK, "[Навигатор] {FFFFFF}Точка %d пройдена.", index + 1);
	return 1;
}
public OnPlayerCefEvent(playerid, const event[], uid)
{
	// /ceflog (или пункт меню "ответы интерфейса") - все события интерфейса: в чат (начало) и в лог сервера (целиком).
	// Без него - только свои окна (uid != 0)
	if(gCefLog[playerid])
	{
		printf("[CEF] игрок %d, uid %d: %s", playerid, uid, event);
		new part[100];
		strmid(part, event, 0, sizeof(part) - 1);
		SCMf(playerid, 0xAAAAAAFF, "[CEF] uid %d: %s%s", uid, part, strlen(event) >= sizeof(part) - 1 ? ("...") : (""));
	}
	else if(uid) SCMf(playerid, C_MAIN, "[CEF] {FFFFFF}uid %d: %s", uid, event);
	return 1;
}
// разрешение экрана: плагин спрашивает его сам при входе, /screen - спросить ещё раз
public OnPlayerScreenSize(playerid, width, height)
{
	SCMf(playerid, C_MAIN, "[Экран] {FFFFFF}Разрешение: %dx%d", width, height);
	return 1;
}
CMD:screen(playerid)
{
	new w, h;
	if(GetPlayerScreenSize(playerid, w, h)) SCMf(playerid, C_MAIN, "[Экран] {FFFFFF}Последнее известное: %dx%d, спрашиваю ещё раз...", w, h);
	else SCM(playerid, C_MAIN, "[Экран] {FFFFFF}Ещё не знаем, спрашиваю у клиента...");
	RequestPlayerScreenSize(playerid);
	return 1;
}
CMD:ceflog(playerid)
{
	gCefLog[playerid] = !gCefLog[playerid];
	return SCMf(playerid, C_OK, "[CEF] {FFFFFF}Все события интерфейса в чат и лог сервера: %s", gCefLog[playerid] ? ("вкл") : ("выкл"));
}
CMD:testtab(playerid, params[])
{
	// текст из своей вкладки "Тест"
	new name[MAX_PLAYER_NAME];
	GetPlayerName(playerid, name, sizeof(name));
	if(isnull(params)) return 1;
	new msg[144];
	format(msg, sizeof(msg), "[Тест] %s: %s", name, params);
	for(new i; i < MAX_PLAYERS; i++)
		if(IsPlayerConnected(i) && gVal[i][I_OWNTAB]) SendClientMessageToTab(i, TEST_TAB, 0x66CCFFFF, msg, true);
	return 1;
}
CMD:cefcursor(playerid, params[])
{
	new st;
	if(sscanf(params, "d", st)) return SCM(playerid, C_MAIN, "[CEF] {FFFFFF}/cefcursor [0/1] - курсор в своём окне");
	Cef_SetBrowserFocus(playerid, TEST_BROWSER, bool:st);
	return 1;
}

// ======================================================================
//  NPC: охранник и враги
// ======================================================================
// пед для пунктов NPC: свой охранник, иначе первый враг
stock TestPed(playerid)
{
	if(gGuard[playerid] != -1 && IsValidPed(gGuard[playerid])) return gGuard[playerid];
	for(new i; i < MAX_ENEMIES; i++) if(gEnemy[i] != -1 && IsValidPed(gEnemy[i])) return gEnemy[i];
	return -1;
}
stock ShowPedInfo(playerid, ped)
{
	new s[700], Float:x, Float:y, Float:z, bool:isped;
	GetPedPos(ped, x, y, z);
	format(s, sizeof s, "Пед %d\nПозиция: %.1f, %.1f, %.1f, угол %.0f\nСкин %d, оружие %d\nЗдоровье %.0f, броня %.0f, бессмертие %d, мёртв %d\nМир %d, интерьер %d, машина %d\nИдёт за игроком %d, атакует %d\nВедёт синхронизацию игрок %d, у меня в прорисовке %d",
		ped, x, y, z, GetPedAngle(ped), GetPedSkin(ped), GetPedWeapon(ped), GetPedHealth(ped), GetPedArmour(ped), IsPedInvulnerable(ped), IsPedDead(ped),
		GetPedVirtualWorld(ped), GetPedInterior(ped), GetPedVehicle(ped), GetPedFollowPlayer(ped), GetPedAttackTarget(ped, isped), GetPedSyncOwner(ped), IsPedStreamedIn(ped, playerid));
	return SPD(playerid, DLG_INFO, DIALOG_STYLE_MSGBOX, "Пед", s, "Ок", "");
}
stock ShowCarInfo(playerid, carid)
{
	new s[1600], color, number[16], region[8], r, g, b, br, bool:beam, bool:simple, Float:speed, Float:ang, Float:off, tex[64], Float:dirt = GetVehicleDirt(carid);
	new vinyl = GetVehicleVinyl(carid, color), plate = GetVehiclePlate(carid, number, region);
	new strobe = GetVehicleStrobeLights(carid, speed, beam), calipers = GetVehicleBrakeCalipers(carid, simple), toner = GetVehicleToner(carid, VEHICLE_TONER_frontal, r, g, b, br);
	new camber = GetVehicleWheelCamber(carid, VEHICLE_WHEELS_FRONT, ang, off);
	GetVehicleHeadlightTexture(carid, tex);
	format(s, sizeof s, "Машина %d\nДетали: капот %d, спойлер %d, выхлоп %d, диски %d\nВинил %d, цвет %d, неон %d, дым %d, фары %d, диски цвет %d\nРазмер колёс %.2f, развал перед %d (%.1f, %.2f)\nНомер тип %d: %s %s\nТонировка лобового %d (%d %d %d %d)\n",
		carid, GetVehicleTuningPart(carid, VEHICLE_PART_HOOD), GetVehicleTuningPart(carid, VEHICLE_PART_SPOILER), GetVehicleTuningPart(carid, VEHICLE_PART_EXHAUST), GetVehicleTuningPart(carid, VEHICLE_PART_WHEELS),
		vinyl, color, GetVehicleNeonColor(carid), GetVehicleSmokeColor(carid), GetVehicleHeadlightColor(carid), GetVehicleWheelColor(carid),
		GetVehicleWheelSize(carid), camber, ang, off, plate, number, region, toner, r, g, b, br);
	format(s, sizeof s, "%sГидравлика %d, стробоскопы %d (%.1f, луч %d), суппорты %d (simple %d)\nХлопки %d, мигалка %d, дрифт %d, нитро %d (цвет %d), тихий двигатель %d\nТекстура фар: %s, поворотники %d, грязь %.0f\nХендлинг задан: %d (maxspeed %d)",
		s, GetVehicleHydraulics(carid), strobe, speed, beam, calipers, simple, IsVehicleExhaustShots(carid), IsVehicleSiren(carid), IsVehicleDrift(carid),
		IsVehicleNitro(carid), GetVehicleNitroColor(carid), IsVehicleSilentEngine(carid), tex[0] ? tex : ("-"), GetVehicleTurnSignal(carid), dirt,
		IsVehicleHandlingSet(carid), IsVehicleHandlingSet(carid, "maxspeed"));
	return SPD(playerid, DLG_INFO, DIALOG_STYLE_MSGBOX, "Машина", s, "Ок", "");
}
stock ShowMyInfo(playerid)
{
	new s[400], walk[32];
	GetPlayerWalkStyle(playerid, walk);
	format(s, sizeof s, "Лаунчер: %d\nПоходка: %s\nСвет: %d\nТекстур скина задано: %d\nВебкамера: %d (0 выкл, 1 ждём, 2 идёт), связь %d",
		GetPlayerLauncherStatus(playerid), walk[0] ? walk : ("-"), GetPlayerLightMode(playerid), GetPlayerSkinTextureCount(playerid), Webcam_GetState(playerid), Webcam_IsLinked(playerid));
	return SPD(playerid, DLG_INFO, DIALOG_STYLE_MSGBOX, "Мои параметры", s, "Ок", "");
}
forward SwapDialogText(playerid);
public SwapDialogText(playerid)
{
	SetPlayerDialogText(playerid, "Новый текст - заменён через SetPlayerDialogText");
	return 1;
}
// ответы интерфейса Arizona разобранными параметрами (пункт "Показывать ответы интерфейса в чате")
public OnPlayerCefResponse(playerid, const event[], argc)
{
	if(!gCefLog[playerid]) return 1;
	new arg[96], raw[144], msg[200];
	Cef_GetRaw(raw);
	format(msg, sizeof msg, "[CEF] %s (%d): %s", event, Cef_GetArgCount(), raw);
	SCM(playerid, 0xAAAAAAFF, msg);
	for(new i; i < argc && i < 4; i++)
	{
		Cef_GetArg(i, arg);
		SCMf(playerid, 0xAAAAAAFF, "  %d: %s (int %d, float %.2f)", i, arg, Cef_GetArgInt(i), Cef_GetArgFloat(i));
	}
	// поля объекта JSON по имени (если интерфейс прислал {"id": .., "value": ..})
	if(Cef_GetArgByName("id", arg)) SCMf(playerid, 0xAAAAAAFF, "  id = %s / %d / %.2f", arg, Cef_GetArgIntByName("id"), Cef_GetArgFloatByName("id"));
	return 1;
}
stock HireGuard(playerid)
{
	if(gGuard[playerid] != -1) return 1;
	new Float:x, Float:y, Float:z, Float:a, name[MAX_PLAYER_NAME], text[64];
	GetPlayerPos(playerid, x, y, z);
	GetPlayerFacingAngle(playerid, a);
	x -= 2.0 * floatsin(-a, degrees);
	y -= 2.0 * floatcos(-a, degrees);
	new ped = CreatePed(165, x, y, z, a, GetPlayerVirtualWorld(playerid), GetPlayerInterior(playerid));
	if(ped == -1) return SCM(playerid, C_ERR, "[Ошибка] {FFFFFF}Нет свободных педов"), gVal[playerid][I_GUARD] = 0;
	GetPlayerName(playerid, name, sizeof(name));
	format(text, sizeof(text), "Охранник %s", name);
	SetPedName(ped, text);
	SetPedHealth(ped, 200.0, 200.0);
	SetPedArmour(ped, 100.0);
	SetPedWeapon(ped, 24);
	SetPedFollowPlayer(ped, playerid, 2.5);
	gGuard[playerid] = ped;
	// в машину хозяина садится сам: плагин (SetPedFollowVehicle, по умолчанию с анимацией)
	SCM(playerid, C_OK, "[Охранник] {FFFFFF}Нанят: ходит за вами, защищает, садится в вашу машину.");
	return 1;
}
stock FireGuard(playerid)
{
	if(gGuard[playerid] == -1) return 1;
	DestroyPed(gGuard[playerid]);
	gGuard[playerid] = -1;
	gVal[playerid][I_GUARD] = 0;
	return 1;
}
stock GuardOwner(pedid)
{
	for(new i; i < MAX_PLAYERS; i++) if(gGuard[i] == pedid) return i;
	return -1;
}
// охранник атакует обидчика хозяина. Хозяин остаётся (SetPedFollowPlayer): когда атака кончится
// (обидчик умер / вышел, PedStopAttack), охранник сам вернётся к нему
stock GuardAttack(ped, ownerid, targetid)
{
	#pragma unused ownerid
	if(GetPedVehicle(ped) || targetid == INVALID_PLAYER_ID) return 1; // из машины не стреляет
	new bool:isped;
	if(GetPedAttackTarget(ped, isped) == targetid && !isped) return 1; // уже атакует его
	PedAttackPlayer(ped, targetid, -1);
	return 1;
}
stock GuardAttackPed(ped, targetped)
{
	if(GetPedVehicle(ped) || ped == targetped) return 1;
	new bool:isped;
	if(GetPedAttackTarget(ped, isped) == targetped && isped) return 1;
	PedAttackPed(ped, targetped, -1);
	return 1;
}
stock SpawnEnemy(playerid)
{
	new slot = -1;
	for(new i; i < MAX_ENEMIES; i++) if(gEnemy[i] == -1) { slot = i; break; }
	if(slot == -1) return SCM(playerid, C_ERR, "[NPC] {FFFFFF}Врагов уже много - уберите их.");
	new Float:x, Float:y, Float:z, Float:a;
	GetPlayerPos(playerid, x, y, z);
	GetPlayerFacingAngle(playerid, a);
	x += 12.0 * floatsin(-a, degrees);
	y += 12.0 * floatcos(-a, degrees);
	new ped = CreatePed(28, x, y, z, a + 180.0, GetPlayerVirtualWorld(playerid), GetPlayerInterior(playerid));
	if(ped == -1) return SCM(playerid, C_ERR, "[NPC] {FFFFFF}Нет свободных педов.");
	SetPedName(ped, "Бандит");
	SetPedColor(ped, 0xFF4040FF);
	SetPedWeapon(ped, 22);
	SetPedHealth(ped, 100.0);
	PedAttackPlayer(ped, playerid, -1);
	gEnemy[slot] = ped;
	return SCM(playerid, C_OK, "[NPC] {FFFFFF}Бандит атакует вас. Урон считает сервер.");
}
public OnPedDamage(pedid, issuerid, bool:issuer_is_ped, Float:amount, weaponid, bodypart)
{
	new owner = GuardOwner(pedid);
	if(owner != -1 && !issuer_is_ped && issuerid == owner) return 0; // хозяин своего охранника не ранит
	if(!issuer_is_ped && gDmgText[issuerid])
	{
		new Float:x, Float:y, Float:z;
		GetPedPos(pedid, x, y, z);
		ShowDamageText(issuerid, amount, x, y, z + 1.0);
	}
	if(owner != -1 && !issuer_is_ped) GuardAttack(pedid, owner, issuerid);
	return 1;
}
public OnPedDeath(pedid, killerid, bool:killer_is_ped, weaponid)
{
	new owner = GuardOwner(pedid);
	if(owner != -1)
	{
		SCM(owner, C_ERR, "[Охранник] {FFFFFF}Ваш охранник убит - через 3 секунды встанет рядом с вами.");
		SetTimerEx("GuardRespawn", 3000, false, "dd", owner, pedid);
		return 1;
	}
	for(new i; i < MAX_ENEMIES; i++) if(gEnemy[i] == pedid) gEnemy[i] = -1;
	SetTimerEx("RemoveDeadPed", 5000, false, "d", pedid);
	return 1;
}
forward GuardRespawn(playerid, pedid);
public GuardRespawn(playerid, pedid)
{
	if(gGuard[playerid] != pedid || !IsValidPed(pedid) || !IsPedDead(pedid)) return 1;
	new Float:x, Float:y, Float:z, Float:a;
	GetPlayerPos(playerid, x, y, z);
	GetPlayerFacingAngle(playerid, a);
	SetPedPos(pedid, x - 2.0 * floatsin(-a, degrees), y - 2.0 * floatcos(-a, degrees), z);
	SetPedVirtualWorld(pedid, GetPlayerVirtualWorld(playerid));
	SetPedInterior(pedid, GetPlayerInterior(playerid));
	RespawnPed(pedid, 200.0);
	SetPedHealth(pedid, 200.0, 200.0);
	SetPedArmour(pedid, 100.0);
	SetPedFollowPlayer(pedid, playerid, 2.5);
	return 1;
}
forward RemoveDeadPed(pedid);
public RemoveDeadPed(pedid)
{
	if(IsValidPed(pedid) && IsPedDead(pedid)) DestroyPed(pedid);
	return 1;
}

// ======================================================================
//  Вебкамера
// ======================================================================
public OnPlayerWebcamLink(playerid, linked)
{
	if(linked) SCM(playerid, C_OK, "[Вебкамера] {FFFFFF}AZRTC подключён. Показ: /menu -> Медиа и голос -> Вебкамера.");
	return 1;
}
public OnPlayerWebcam(playerid, status)
{
	if(status != 1) gVal[playerid][I_WEBCAM] = 0;
	switch(status)
	{
		case 1: SCM(playerid, C_OK, "[Вебкамера] {FFFFFF}Показ идёт: камеру видят игроки рядом.");
		case 0: SCM(playerid, C_MAIN, "[Вебкамера] {FFFFFF}Показ остановлен.");
		default: SCM(playerid, C_ERR, "[Вебкамера] {FFFFFF}Камера не открылась: включите её в настройках AZRTC (F11).");
	}
	return 1;
}

// ======================================================================
//  Сервер
// ======================================================================
public OnGameModeInit()
{
	new d, mo, y, h, mi;
	getdate(y, mo, d);
	gettime(h, mi);
	SendRconCommandf("%s ModGear | Тестовый сервер | %d.%d.%dv", RCON_SERVER_NAME, MODGEAR_VERSION_MAJOR, MODGEAR_VERSION_MINOR, MODGEAR_VERSION_PATCH);
	format(dialogTitle, sizeof(dialogTitle), "{FAAC58}ModGear {FFFFFF}%d.%d.%dv", MODGEAR_VERSION_MAJOR, MODGEAR_VERSION_MINOR, MODGEAR_VERSION_PATCH);
	new build[40];
	format(build, sizeof(build), "Build: %02d.%02d.%d %02d:%02d", d, mo, y, h, mi);
	SetGameModeText(build);
	SetWorldTime(23); // всегда ночь 23:00 (сменить - /st)
	UsePlayerPedAnims();

	Media_Init();
	Webcam_Init(); // UDP-порт MODGEAR_WEBCAM_PORT (7781) должен быть открыт
	gVoiceLocal = Voice_Create();
	gVoiceRadio = Voice_Create("Рация", 0.0);
	Voice_Remember(gVoiceLocal, "Локальный");
	Voice_Remember(gVoiceRadio, "Рация");

	CreateDynamicMapIcon(1319.2843, 1261.8027, 10.8203, 87, 0, -1, -1, -1);
	for(new i; i < sizeof(InfoCreateVehicle); i++)
	{
		new car = AddStaticVehicleEx(InfoCreateVehicle[i][icModel], InfoCreateVehicle[i][icPos][0], InfoCreateVehicle[i][icPos][1], InfoCreateVehicle[i][icPos][2], InfoCreateVehicle[i][icPos][3], InfoCreateVehicle[i][icColor][0], InfoCreateVehicle[i][icColor][1], 300);
		InfoCreateVehicle[i][icVehID] = car;
		AddVehicleComponent(car, 1010); // нитро
		SetVehiclePlate(car, InfoCreateVehicle[i][icPlateType], InfoCreateVehicle[i][icPlate], InfoCreateVehicle[i][icRegion]);
		if(InfoCreateVehicle[i][icHood]) SetVehicleTuningPart(car, VEHICLE_PART_HOOD, InfoCreateVehicle[i][icHood]);
		if(InfoCreateVehicle[i][icSpoiler]) SetVehicleTuningPart(car, VEHICLE_PART_SPOILER, InfoCreateVehicle[i][icSpoiler]);
		if(InfoCreateVehicle[i][icExhaust]) SetVehicleTuningPart(car, VEHICLE_PART_EXHAUST, InfoCreateVehicle[i][icExhaust]);
		if(InfoCreateVehicle[i][icWheels]) SetVehicleTuningPart(car, VEHICLE_PART_WHEELS, InfoCreateVehicle[i][icWheels]);
	}
	// кинотеатр у аэропорта ЛВ: экран, фонари, диван
	kinema = CreateObject(7914, 1278.931274, 1311.162231, 13.150308, 0.0, 0.0, -0.2);
	SetObjectMaterial(kinema, 0, 10765, "airportgnd_sfse", "black64", 0);
	CreateDynamicObject(1294, 1272.001953, 1310.312011, 13.472962, 0.0, 0.0, 90.100128);
	CreateDynamicObject(1294, 1278.674194, 1310.321533, 13.472962, 0.0, 0.0, 90.100128);
	CreateDynamicObject(1294, 1285.575805, 1310.331665, 13.472962, 0.0, 0.0, 90.100128);
	CreateDynamicObject(1728, 1267.234375, 1307.696289, 9.753808, 0.0, 0.0, 31.900003);
	// второй экран - Legislative Square (ЛС, модель 12049, текстура legisq_screen): полотно изогнутое,
	// 30.4 x 11 м, смотрит на юг - перед ним билборд 7914 с масштабом 1.8
	kinemaLS = CreateObject(7914, 1477.20, -1695.25, 19.96, 0.0, 0.0, 0.0, 300.0);
	SetObjectScale(kinemaLS, 1.8, false);
	SetObjectMaterial(kinemaLS, 0, 10765, "airportgnd_sfse", "black64", 0);
	ApplyDynamicActorAnimation(CreateDynamicActor(102, 1269.4082, 1308.1591, 10.8203, 211.0356), "JST_BUISNESS", "GIRL_02", 4.1, 1, 1, 1, 1, 1);
	ApplyDynamicActorAnimation(CreateDynamicActor(312, 1332.4679, 1262.8011, 10.8203, 89.2910), "BD_FIRE", "M_SMKLEAN_LOOP", 4.1, 1, 1, 1, 1, 1);
	printf("[Server] Тестовый сервер загружен (%s)", build);
	return 1;
}

// ======================================================================
//  Команды
// ======================================================================
CMD:cmd(playerid)
{
	new m[1400];
	strcat(m, "{FAAC58}/menu{FFFFFF} - все функции плагина по разделам\n");
	strcat(m, "{FAAC58}/music /kinomenu /voice{FFFFFF} - музыка, видео, голос\n");
	strcat(m, "{FAAC58}/car [модель] /rep /flip /sp /hp{FFFFFF} - машина, починка, перевернуть машину, респавн, лечение\n");
	strcat(m, "{FAAC58}/st [час] /sw [погода] /tp [x y z]{FFFFFF} - время, погода, телепорт (или метка на карте)\n");
	strcat(m, "{FAAC58}/hand [поле] [значение] /handget [поле] /handreset{FFFFFF} - хендлинг машины\n");
	strcat(m, "{FAAC58}/testtab [текст]{FFFFFF} - написать во вкладку \"Тест\"\n");
	strcat(m, "{FAAC58}/cefcursor [0/1]{FFFFFF} - курсор в своём окне CEF\n");
	strcat(m, "{FAAC58}/ceflog{FFFFFF} - все события интерфейса в чат (OnPlayerCefEvent / OnPlayerCefResponse)\n");
	strcat(m, "{FAAC58}/keys [текст]{FFFFFF} - StringToKeys / KeysToString");
	return SPD(playerid, 0, DIALOG_STYLE_MSGBOX, "{FAAC58}Команды", m, "Ок", "");
}
CMD:car(playerid, params[])
{
	new model;
	if(sscanf(params, "d", model)) return SCM(playerid, C_MAIN, "[Инфо] {FFFFFF}/car [модель]");
	gItem[playerid] = 0;
	for(new i; i < sizeof(gItems); i++) if(gItems[i][itId] == I_CAR) gItem[playerid] = i;
	OnItem(playerid, I_CAR, params);
	return 1;
}
CMD:rep(playerid)
{
	new carid = GetPlayerVehicleID(playerid);
	if(!carid) return SCM(playerid, C_MAIN, "[Инфо] {FFFFFF}Вы не в машине.");
	new Float:a;
	GetVehicleZAngle(carid, a);
	SetVehicleZAngle(carid, a);
	RepairVehicle(carid);
	return SCM(playerid, C_OK, "[Инфо] {FFFFFF}Машина починена.");
}
// перевернуть машину на колёса (обычные функции SA-MP): SetVehicleZAngle сбрасывает крен и тангаж
CMD:flip(playerid)
{
	new carid = GetPlayerVehicleID(playerid);
	if(!carid) return SCM(playerid, C_MAIN, "[Инфо] {FFFFFF}Вы не в машине.");
	new Float:x, Float:y, Float:z, Float:a;
	GetVehiclePos(carid, x, y, z);
	GetVehicleZAngle(carid, a);
	SetVehiclePos(carid, x, y, z + 1.0);
	SetVehicleZAngle(carid, a);
	SetVehicleVelocity(carid, 0.0, 0.0, 0.0);
	RepairVehicle(carid);
	return SCM(playerid, C_OK, "[Инфо] {FFFFFF}Машина перевёрнута на колёса и починена.");
}
// только починка (без переворота)
CMD:rem(playerid)
{
	new carid = GetPlayerVehicleID(playerid);
	if(!carid) return SCM(playerid, C_MAIN, "[Инфо] {FFFFFF}Вы не в машине.");
	RepairVehicle(carid);
	return SCM(playerid, C_OK, "[Инфо] {FFFFFF}Машина починена.");
}
CMD:sp(playerid) { SpawnPlayer(playerid); return 1; }
CMD:hp(playerid) { SetHP(playerid, 100.0, 100.0); return SCM(playerid, C_OK, "[Инфо] {FFFFFF}100 HP и 100 брони."); }
CMD:sw(playerid, params[])
{
	new w;
	if(sscanf(params, "d", w)) return SCM(playerid, C_MAIN, "[Инфо] {FFFFFF}/sw [погода]");
	SetWeather(w);
	return SCMf(playerid, C_OK, "[Инфо] {FFFFFF}Погода %d.", w);
}
CMD:st(playerid, params[])
{
	new hh;
	if(sscanf(params, "d", hh)) return SCM(playerid, C_MAIN, "[Инфо] {FFFFFF}/st [час]");
	SetWorldTime(hh);
	return SCMf(playerid, C_OK, "[Инфо] {FFFFFF}Время %02d:00.", hh);
}
CMD:tp(playerid, params[])
{
	new Float:x, Float:y, Float:z;
	if(sscanf(params, "p<,>fff", x, y, z) && sscanf(params, "fff", x, y, z)) return SCM(playerid, C_MAIN, "[Инфо] {FFFFFF}/tp [x y z]");
	return SetPlayerPos(playerid, x, y, z);
}
CMD:keys(playerid, params[])
{
	if(isnull(params)) return SCM(playerid, C_MAIN, "[Инфо] {FFFFFF}/keys [текст до 19 символов]");
	new k1, k2, k3, back[32];
	StringToKeys(k1, k2, k3, params);
	KeysToString(k1, k2, k3, back);
	return SCMf(playerid, -1, "[Тест] '%s' -> %d, %d, %d -> '%s'", params, k1, k2, k3, back);
}
CMD:hand(playerid, params[])
{
	new field[32], Float:value, carid = GetPlayerVehicleID(playerid);
	if(!carid) return SCM(playerid, C_MAIN, "[Хендлинг] {FFFFFF}Сядьте в машину.");
	if(sscanf(params, "s[32]f", field, value)) return SCM(playerid, C_MAIN, "[Хендлинг] {FFFFFF}/hand [maxspeed|acceleration|inertia|brake|traction|suspension|...] [значение]");
	if(!SetVehicleHandling(carid, field, value)) return SCM(playerid, C_ERR, "[Хендлинг] {FFFFFF}Такого параметра нет.");
	return SCMf(playerid, C_OK, "[Хендлинг] {FFFFFF}%s = %.3f", field, value);
}
CMD:handget(playerid, params[])
{
	new field[32], Float:value, carid = GetPlayerVehicleID(playerid);
	if(!carid || sscanf(params, "s[32]", field)) return SCM(playerid, C_MAIN, "[Хендлинг] {FFFFFF}/handget [поле] (в машине)");
	if(!GetVehicleHandling(carid, field, value)) return SCM(playerid, -1, "[Хендлинг] не задано");
	return SCMf(playerid, -1, "[Хендлинг] %s = %.3f", field, value);
}
CMD:handreset(playerid)
{
	new carid = GetPlayerVehicleID(playerid);
	if(!carid) return SCM(playerid, C_MAIN, "[Хендлинг] {FFFFFF}Сядьте в машину.");
	ResetVehicleHandling(carid);
	return SCM(playerid, C_OK, "[Хендлинг] {FFFFFF}Сброшено (вернётся после пересоздания машины у игроков).");
}
CMD:music(playerid) { return Music_ShowMenu(playerid); }
CMD:kinomenu(playerid) { return Video_ShowMenu(playerid); }
CMD:kino(playerid) // к экрану кинотеатра (Legislative Square)
{
	SetPlayerInterior(playerid, 0);
	SetPlayerVirtualWorld(playerid, 0);
	SetPlayerPos(playerid, 1477.2, -1718.0, 13.55);
	SetPlayerFacingAngle(playerid, 0.0);
	SetCameraBehindPlayer(playerid);
	return SCM(playerid, C_OK, "[Кино] {FFFFFF}Экран перед вами. Видео: /kinomenu");
}
CMD:voice(playerid) { return Voice_ShowMenu(playerid); }
public OnVideoFinished(videoid)
{
	if(videoid == kinoVideo) SendClientMessageToAll(-1, "[Кино] Фильм закончился.");
	return 1;
}
public OnAudioStreamFinished(audioID) { return 1; }

// ======================================================================
//  Голос
// ======================================================================
public OnPlayerFirstPersonAim(playerid, bool:aiming)
{
	SCMf(playerid, -1, "[Тест] прицел от первого лица: %s", aiming ? ("вошёл") : ("вышел"));
	return 1;
}
public OnPlayerVoice(playerid, channelid, bool:talking)
{
	if(gVoiceTalk[playerid]) SCMf(playerid, 0xFAAC58FF, "[Голос] {FFFFFF}канал %d: %s", channelid, talking ? ("говорю") : ("молчу"));
	// рация: кто говорит, AZVoice показывает сам (у глобальной зоны канала есть название)
	return 1;
}
stock Voice_Remember(id, const name[])
{
	if(id <= 0 || id >= MAX_VOICE_CH) return 0;
	gVoiceUsed[id] = true;
	format(gVoiceName[id], 32, "%s", name);
	return 1;
}
stock Voice_Toggle(playerid, id)
{
	if(!id) return SCM(playerid, 0xFAAC58FF, "[Голос] {FFFFFF}Этот канал удалён.");
	if(Voice_IsPlayerInChannel(id, playerid)) Voice_RemovePlayer(id, playerid);
	else Voice_AddPlayer(id, playerid);
	return 1;
}
stock Voice_ShowMenu(playerid)
{
	new s[1400];
	format(s, sizeof(s), "{FAAC58}[Канал] {FFFFFF}Локальный (рядом): %s\n{FAAC58}[Канал] {FFFFFF}Общая рация: %s\n{FAAC58}[Комната] {FFFFFF}Создать комнату рации\n{FAAC58}[Комната] {FFFFFF}Войти в канал по ID\n{FAAC58}[Комната] {FFFFFF}Выйти из канала по ID\n{FAAC58}[Канал] {FFFFFF}Переименовать канал\n{FAAC58}[Канал] {FFFFFF}Радиус канала (0 - рация)\n{FAAC58}[Канал] {FFFFFF}Удалить канал\n{FAAC58}[Мут] {FFFFFF}Замутить / размутить игрока\n{FAAC58}[Тест] {FFFFFF}Слышать себя (эхо): %s\n{FAAC58}[Тест] {FFFFFF}Пересылка голоса: %s\n{FAAC58}[Тест] {FFFFFF}Писать в чат, когда я говорю: %s\n{FAAC58}[Инфо] {FFFFFF}Каналы и проверки",
		(gVoiceLocal && Voice_IsPlayerInChannel(gVoiceLocal, playerid)) ? ("{9ACD32}ВКЛ") : ("{B83434}ВЫКЛ"),
		(gVoiceRadio && Voice_IsPlayerInChannel(gVoiceRadio, playerid)) ? ("{9ACD32}ВКЛ") : ("{B83434}ВЫКЛ"),
		gVoiceEcho ? ("{9ACD32}ВКЛ") : ("{B83434}ВЫКЛ"),
		gVoiceRelay ? ("{9ACD32}ВКЛ") : ("{B83434}ВЫКЛ"),
		gVoiceTalk[playerid] ? ("{9ACD32}ВКЛ") : ("{B83434}ВЫКЛ"));
	return SPD(playerid, DIALOG_VOICE, DIALOG_STYLE_LIST, "{FAAC58}Голосовой чат", s, "Выбрать", "Закрыть");
}
stock Voice_AskInput(playerid)
{
	new const ask[][] =
	{
		"", "",
		"{FFFFFF}Название комнаты рации\n{CCCCCC}Пример: Рация ПД",
		"{FFFFFF}ID канала, в который войти:",
		"{FFFFFF}ID канала, из которого выйти:",
		"{FFFFFF}ID канала и новое название через пробел\n{CCCCCC}Пример: 3 Рация ФБР",
		"{FFFFFF}ID канала и радиус в метрах\n0 - рация (слышно на любом расстоянии)\n{CCCCCC}Пример: 1 30",
		"{FFFFFF}ID канала, который удалить:",
		"{FFFFFF}ID игрока (мут включается / выключается):"
	};
	return SPD(playerid, DIALOG_VOICE_INPUT, DIALOG_STYLE_INPUT, "{FAAC58}Голосовой чат", ask[gVoiceAction[playerid]], "Ок", "Назад");
}
stock Voice_Err(playerid, const text[])
{
	SCMf(playerid, 0xFAAC58FF, "[Голос] {FFFFFF}%s", text);
	return Voice_ShowMenu(playerid);
}
stock Voice_ShowInfo(playerid)
{
	new s[1800], line[160];
	format(s, sizeof(s), "{FFFFFF}Ваш мут: %s{FFFFFF}\n\n{FAAC58}ID  Канал  |  Вы в канале  |  Рядом есть кого слышать\n", Voice_IsMuted(playerid) ? ("{B83434}да") : ("{9ACD32}нет"));
	for(new i = 1; i < MAX_VOICE_CH; i++)
	{
		if(!gVoiceUsed[i]) continue;
		format(line, sizeof(line), "{FFFFFF}%d  %s  |  %s  |  %s\n", i, gVoiceName[i], Voice_IsPlayerInChannel(i, playerid) ? ("да") : ("нет"), Voice_IsPlayerInZone(i, playerid) ? ("да") : ("нет"));
		strcat(s, line);
	}
	return SPD(playerid, DIALOG_VOICE_INFO, DIALOG_STYLE_MSGBOX, "{FAAC58}Голосовой чат - каналы", s, "Назад", "");
}
stock Voice_OnDialog(playerid, dialogid, response, listitem, const inputtext[])
{
	if(dialogid == DIALOG_VOICE)
	{
		if(!response) return 1;
		switch(listitem)
		{
			case 0: Voice_Toggle(playerid, gVoiceLocal);
			case 1: Voice_Toggle(playerid, gVoiceRadio);
			case 9: gVoiceEcho = !gVoiceEcho, Voice_SetEcho(gVoiceEcho);
			case 10: gVoiceRelay = !gVoiceRelay, Voice_SetFormat(gVoiceRelay ? 0 : -1);
			case 11: gVoiceTalk[playerid] = !gVoiceTalk[playerid];
			case 12: return Voice_ShowInfo(playerid);
			default:
			{
				gVoiceAction[playerid] = listitem;
				return Voice_AskInput(playerid);
			}
		}
		return Voice_ShowMenu(playerid);
	}
	if(dialogid == DIALOG_VOICE_INFO || !response) return Voice_ShowMenu(playerid);
	new id, Float:r, name[32];
	switch(gVoiceAction[playerid])
	{
		case 2:
		{
			if(strlen(inputtext) < 1 || strlen(inputtext) > 30) return Voice_AskInput(playerid);
			id = Voice_Create(inputtext, 0.0);
			if(!id) return Voice_Err(playerid, "Не удалось создать комнату.");
			Voice_Remember(id, inputtext);
			Voice_AddPlayer(id, playerid);
			SCMf(playerid, 0xFAAC58FF, "[Голос] {FFFFFF}Комната \"%s\" создана, ID %d. Вы в ней.", inputtext, id);
		}
		case 3:
		{
			if(sscanf(inputtext, "d", id) || !Voice_AddPlayer(id, playerid)) return Voice_Err(playerid, "Нет такого канала или вы уже в нём.");
			SCMf(playerid, 0xFAAC58FF, "[Голос] {FFFFFF}Вы вошли в канал %d.", id);
		}
		case 4:
		{
			if(sscanf(inputtext, "d", id) || !Voice_RemovePlayer(id, playerid)) return Voice_Err(playerid, "Нет такого канала или вас в нём нет.");
			SCMf(playerid, 0xFAAC58FF, "[Голос] {FFFFFF}Вы вышли из канала %d.", id);
		}
		case 5:
		{
			if(sscanf(inputtext, "ds[32]", id, name) || !Voice_SetName(id, name)) return Voice_Err(playerid, "Нет такого канала.");
			Voice_Remember(id, name);
			SCMf(playerid, 0xFAAC58FF, "[Голос] {FFFFFF}Канал %d теперь называется \"%s\".", id, name);
		}
		case 6:
		{
			if(sscanf(inputtext, "df", id, r) || !Voice_SetRadius(id, r)) return Voice_Err(playerid, "Нет такого канала.");
			if(r <= 0.0) SCMf(playerid, 0xFAAC58FF, "[Голос] {FFFFFF}Канал %d теперь рация: слышно на любом расстоянии.", id);
			else SCMf(playerid, 0xFAAC58FF, "[Голос] {FFFFFF}Канал %d: слышно в радиусе %.1f м.", id, r);
		}
		case 7:
		{
			if(sscanf(inputtext, "d", id) || !Voice_Destroy(id)) return Voice_Err(playerid, "Нет такого канала.");
			if(id > 0 && id < MAX_VOICE_CH) gVoiceUsed[id] = false;
			if(id == gVoiceLocal) gVoiceLocal = 0;
			if(id == gVoiceRadio) gVoiceRadio = 0;
			SCMf(playerid, 0xFAAC58FF, "[Голос] {FFFFFF}Канал %d удалён.", id);
		}
		case 8:
		{
			if(sscanf(inputtext, "u", id) || !IsPlayerConnected(id)) return Voice_Err(playerid, "Нет такого игрока.");
			Voice_Mute(id, !Voice_IsMuted(id));
			SCMf(playerid, 0xFAAC58FF, "[Голос] {FFFFFF}Мут игрока %d: %s", id, Voice_IsMuted(id) ? ("ВКЛ") : ("ВЫКЛ"));
		}
	}
	return Voice_ShowMenu(playerid);
}

// ======================================================================
//  Видео
// ======================================================================
stock Video_ShowMenu(playerid)
{
	return SPD(playerid, DIALOG_VIDEO, DIALOG_STYLE_LIST, "{FAAC58}Видео (кинотеатр)",
		"{FAAC58}[Экран] {FFFFFF}Включить видео (ссылка)\n{FAAC58}[Экран] {FFFFFF}Остановить видео\n{FAAC58}[Экран] {FFFFFF}Перемотать (секунда)\n{FAAC58}[Экран] {FFFFFF}Удалить экран\n{FAAC58}[Инфо] {FFFFFF}Что сейчас идёт\n{FAAC58}[Телефон] {FFFFFF}Показать YouTube в телефоне\n{FAAC58}[Телефон] {FFFFFF}Скрыть видео в телефоне",
		"Выбрать", "Закрыть");
}
stock Video_OnDialog(playerid, dialogid, response, listitem, const inputtext[])
{
	if(dialogid == DIALOG_VIDEO)
	{
		if(!response) return 1;
		switch(listitem)
		{
			case 0: return SPD(playerid, DIALOG_VIDEO_PLAY, DIALOG_STYLE_INPUT, "{FAAC58}Видео на экране", "{FFFFFF}Вставьте ссылку (YouTube, Twitch или страница):", "Включить", "Назад");
			case 1:
			{
				Video_Stop(kinoVideoLS);
				if(!Video_Stop(kinoVideo)) SCM(playerid, -1, "[Кино] видео не включено");
				else SCM(playerid, -1, "[Кино] видео остановлено");
			}
			case 2: return SPD(playerid, DIALOG_VIDEO_SEEK, DIALOG_STYLE_INPUT, "{FAAC58}Перемотка", "{FFFFFF}С какой секунды продолжить:", "Перемотать", "Назад");
			case 3:
			{
				if(Video_Destroy(kinoVideoLS)) kinoVideoLS = 0;
				if(Video_Destroy(kinoVideo)) { kinoVideo = 0; kinoUrl[0] = 0; SCM(playerid, -1, "[Кино] экран удалён (создастся заново при включении видео)"); }
				else SCM(playerid, -1, "[Кино] экран не создан");
			}
			case 4: return Video_ShowInfo(playerid);
			case 5: return SPD(playerid, DIALOG_VIDEO_PHONE, DIALOG_STYLE_INPUT, "{FAAC58}Видео в телефоне", "{FFFFFF}Вставьте ссылку YouTube или id видео:", "Показать", "Назад");
			case 6:
			{
				HidePlayerPhoneVideo(playerid);
				SCM(playerid, -1, "[Кино] видео в телефоне скрыто");
			}
		}
		return Video_ShowMenu(playerid);
	}
	if(!response) return Video_ShowMenu(playerid);
	switch(dialogid)
	{
		case DIALOG_VIDEO_PLAY:
		{
			if(!IsUrl(inputtext)) return UrlError(playerid), SPD(playerid, DIALOG_VIDEO_PLAY, DIALOG_STYLE_INPUT, "{FAAC58}Видео на экране", "{FFFFFF}Вставьте ссылку (YouTube, Twitch или страница):", "Включить", "Назад");
			if(!kinoVideo) kinoVideo = Video_Create(kinema, 7914, 40.0);
			if(!kinoVideoLS) kinoVideoLS = Video_Create(kinemaLS, 7914, 120.0);
			if(!Video_Play(kinoVideo, inputtext)) return SCM(playerid, -1, "[Кино] не удалось включить видео");
			Video_Play(kinoVideoLS, inputtext);
			strpack(kinoUrl, inputtext);
			SCMf(playerid, -1, "[Кино] видео %d включено. Идёт на обоих экранах: у аэропорта ЛВ и в ЛС (/kino).", kinoVideo);
		}
		case DIALOG_VIDEO_PHONE:
		{
			if(strlen(inputtext) < 5) return SPD(playerid, DIALOG_VIDEO_PHONE, DIALOG_STYLE_INPUT, "{FAAC58}Видео в телефоне", "{FFFFFF}Вставьте ссылку YouTube или id видео:", "Показать", "Назад");
			new link[128];
			format(link, sizeof(link), "%s", inputtext);
			ShowPlayerPhoneVideo(playerid, link);
			SCMf(playerid, -1, "[Кино] видео в телефоне: %s", inputtext);
		}
		case DIALOG_VIDEO_SEEK:
		{
			new sec;
			if(sscanf(inputtext, "d", sec)) return SPD(playerid, DIALOG_VIDEO_SEEK, DIALOG_STYLE_INPUT, "{FAAC58}Перемотка", "{FFFFFF}С какой секунды продолжить:", "Перемотать", "Назад");
			Video_SetTime(kinoVideoLS, sec);
			if(!Video_SetTime(kinoVideo, sec)) SCM(playerid, -1, "[Кино] видео не идёт");
			else SCMf(playerid, -1, "[Кино] перемотано на %d сек.", sec);
		}
	}
	return 1;
}

stock Video_ShowInfo(playerid)
{
	new info[700], url[200], tc[16], tl[16], playing = Video_IsPlaying(kinoVideo);
	MakeUrl(url, kinoUrl);
	MakeTime(tc, playing ? Video_GetTime(kinoVideo) : -1);
	new len = Video_GetDuration(kinoVideo);
	MakeTime(tl, (playing || len >= 0) ? len : -1);
	format(info, sizeof(info),
		"{FAAC58}Экран: {FFFFFF}объект %d (модель 7914)\n{FAAC58}ID видео: {FFFFFF}%d\n{FAAC58}Статус: %s\n{FAAC58}Время: {FFFFFF}%s / %s\n{FAAC58}Ссылка:\n{CCCCCC}%s",
		kinema, kinoVideo, (playing ? "{9ACD32}идёт" : "{B83434}не идёт"), tc, tl, url);
	return SPD(playerid, DIALOG_VIDEO, DIALOG_STYLE_MSGBOX, "{FAAC58}Видео - что идёт", info, "Назад", "");
}


// ======================================================================
//  Музыка
// ======================================================================
// ссылка, а не мусор: http(s)://, без пробелов, кавычек и управляющих символов (Esc/Enter и т.п.)
stock IsUrl(const s[])
{
	new len = strlen(s);
	if(len < 10 || len > 250) return 0;
	if(strcmp(s, "http://", true, 7) && strcmp(s, "https://", true, 8)) return 0;
	for(new i; i < len; i++) if(s[i] <= ' ' || s[i] == '"' || s[i] == '\'' || s[i] == '\\' || s[i] == '<' || s[i] == '>' || s[i] == '{' || s[i] == '}') return 0;
	return 1;
}
stock UrlError(playerid) return SCM(playerid, 0xB83434FF, "[Ошибка] {FFFFFF}Это не ссылка. Нужна ссылка вида https://... (прямой mp3, YouTube или Twitch).");
stock Music_ShowMenu(playerid)
{
	return SPD(playerid, DIALOG_MUSIC, DIALOG_STYLE_LIST, "{FAAC58}Музыка", "{FAAC58}[Машина] {FFFFFF}Включить трек в машине\n{FAAC58}[Машина] {FFFFFF}Остановить трек в машине\n{FAAC58}[Колонка] {FFFFFF}Поставить колонку с треком\n{FAAC58}[Колонка] {FFFFFF}Убрать ближайшую колонку\n{FAAC58}[Объект] {FFFFFF}Включить трек на объекте (по ID)\n{FAAC58}[Объект] {FFFFFF}Остановить трек на объекте\n{FAAC58}[Инфо] {FFFFFF}Что сейчас играет\n{FAAC58}[На себе] {FFFFFF}Включить трек на себе\n{FAAC58}[На себе] {FFFFFF}Остановить трек на себе\n{FAAC58}[На себе] {FFFFFF}ID моего потока\n{FAAC58}[Фон] {FFFFFF}Включить фоновую музыку (слышу только я)\n{FAAC58}[Фон] {FFFFFF}Выключить фоновую музыку\n{FAAC58}[Фон] {FFFFFF}Проверки потока (тип, владелец, повтор, время)\n{FAAC58}[Фон] {FFFFFF}Переключить повтор", "Выбрать", "Закрыть");
}
stock Music_OnDialog(playerid, dialogid, response, listitem, const inputtext[])
{
	if(dialogid == DIALOG_MUSIC)
	{
		if(!response) return 1;
		switch(listitem)
		{
			case 0: return SPD(playerid, DIALOG_MUSIC_CAR, DIALOG_STYLE_INPUT, "{FAAC58}Трек в машине", "{FFFFFF}Вставьте ссылку: прямой mp3 или YouTube:", "Включить", "Назад");
			case 1: Music_CarStop(playerid);
			case 2: return SPD(playerid, DIALOG_MUSIC_SPEAKER, DIALOG_STYLE_INPUT, "{FAAC58}Колонка", "{FFFFFF}Колонка встанет перед вами.\nВставьте ссылку: прямой mp3 или YouTube:", "Поставить", "Назад");
			case 3: Music_SpeakerRemove(playerid);
			case 4: return SPD(playerid, DIALOG_MUSIC_OBJECT, DIALOG_STYLE_INPUT, "{FAAC58}Трек на объекте", "{FFFFFF}Введите ID объекта и ссылку на трек через пробел\n{CCCCCC}Пример: 15 https://youtu.be/VIDEOID  (или прямой .mp3)", "Включить", "Назад");
			case 5: return SPD(playerid, DIALOG_MUSIC_OBJECT_STOP, DIALOG_STYLE_INPUT, "{FAAC58}Трек на объекте", "{FFFFFF}Введите ID объекта:", "Остановить", "Назад");
			case 6: return Music_ShowInfo(playerid);
			case 7: return SPD(playerid, DIALOG_MUSIC_SELF, DIALOG_STYLE_INPUT, "{FAAC58}Трек на себе", "{FFFFFF}Звук будет привязан к вам.\nВставьте ссылку: прямой mp3 или YouTube:", "Включить", "Назад");
			case 8:
			{
				new aid = Audio_GetPlayerStreamID(playerid);
				if(aid == -1) SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}На вас ничего не играет.");
				else Audio_DestroyStream(aid), SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Трек на вас остановлен.");
			}
			case 9: SCMf(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Audio_GetPlayerStreamID(%d) = %d", playerid, Audio_GetPlayerStreamID(playerid));
			case 10: return SPD(playerid, DIALOG_MUSIC_BG, DIALOG_STYLE_INPUT, "{FAAC58}Фоновая музыка", "{FFFFFF}Играет только у вас, без 3D (не из машины и не от персонажа).\nВставьте ссылку: прямой mp3 или YouTube:", "Включить", "Назад");
			case 11:
			{
				if(Audio_StopForPlayer(playerid)) SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Фоновая музыка выключена.");
				else SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Фоновой музыки нет.");
			}
			case 12:
			{
				new aid = Audio_GetPlayerBackgroundID(playerid), tc[16], tl[16];
				if(aid == -1) SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Фоновой музыки нет (Audio_GetPlayerBackgroundID = -1).");
				else
				{
					MakeTime(tc, Audio_GetStreamTime(aid));
					MakeTime(tl, Audio_GetDuration(aid));
					SCMf(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}поток %d: valid %d, тип %d (фон = %d), владелец %d, играет %d, повтор %d, %s / %s",
						aid, Audio_IsValidStream(aid), Audio_GetStreamType(aid), AUDIO_TYPE_BACKGROUND, Audio_GetStreamOwner(aid), Audio_IsStreamPlaying(aid), Audio_IsStreamLooped(aid), tc, tl);
				}
			}
			case 13:
			{
				new aid = Audio_GetPlayerBackgroundID(playerid);
				if(aid == -1) SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Фоновой музыки нет.");
				else
				{
					Audio_SetStreamLoop(aid, !Audio_IsStreamLooped(aid));
					SCMf(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Повтор: %s", Audio_IsStreamLooped(aid) ? ("вкл") : ("выкл"));
				}
			}
		}
		return Music_ShowMenu(playerid);
	}
	if(!response) return Music_ShowMenu(playerid);
	switch(dialogid)
	{
		case DIALOG_MUSIC_CAR:
		{
			if(!IsUrl(inputtext)) return UrlError(playerid), SPD(playerid, DIALOG_MUSIC_CAR, DIALOG_STYLE_INPUT, "{FAAC58}Трек в машине", "{FFFFFF}Вставьте ссылку: прямой mp3 или YouTube:", "Включить", "Назад");
			Music_CarPlay(playerid, inputtext);
		}
		case DIALOG_MUSIC_SPEAKER:
		{
			if(!IsUrl(inputtext)) return UrlError(playerid), SPD(playerid, DIALOG_MUSIC_SPEAKER, DIALOG_STYLE_INPUT, "{FAAC58}Колонка", "{FFFFFF}Колонка встанет перед вами.\nВставьте ссылку: прямой mp3 или YouTube:", "Поставить", "Назад");
			Music_SpeakerCreate(playerid, inputtext);
		}
		case DIALOG_MUSIC_BG:
		{
			if(!IsUrl(inputtext)) return UrlError(playerid), SPD(playerid, DIALOG_MUSIC_BG, DIALOG_STYLE_INPUT, "{FAAC58}Фоновая музыка", "{FFFFFF}Играет только у вас, без 3D (не из машины и не от персонажа).\nВставьте ссылку: прямой mp3 или YouTube:", "Включить", "Назад");
			new aid = Audio_PlayForPlayer(playerid, inputtext, 0, false);
			if(aid == -1) return SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Не удалось включить (для YouTube нужен Media_Init + yt-dlp/ffmpeg).");
			SCMf(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Фоновая музыка включена, поток %d (без повтора).", aid);
		}
		case DIALOG_MUSIC_SELF:
		{
			if(!IsUrl(inputtext)) return UrlError(playerid), SPD(playerid, DIALOG_MUSIC_SELF, DIALOG_STYLE_INPUT, "{FAAC58}Трек на себе", "{FFFFFF}Звук будет привязан к вам.\nВставьте ссылку: прямой mp3 или YouTube:", "Включить", "Назад");
			new aid = Audio_GetPlayerStreamID(playerid);
			if(aid == -1) aid = Audio_AttachStreamToPlayer(playerid);
			if(aid == -1) return SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Не удалось создать поток.");
			if(!Audio_SetStreamUrl(aid, inputtext, 0)) return SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Не удалось включить (для YouTube нужен Media_Init + yt-dlp/ffmpeg).");
			SCMf(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Трек привязан к вам, поток %d.", aid);
		}
		case DIALOG_MUSIC_OBJECT:
		{
			new objectid, url[200];
			if(sscanf(inputtext, "ds[200]", objectid, url)) return SPD(playerid, DIALOG_MUSIC_OBJECT, DIALOG_STYLE_INPUT, "{FAAC58}Трек на объекте", "{FFFFFF}Введите ID объекта и ссылку на трек через пробел\n{CCCCCC}Пример: 15 https://youtu.be/VIDEOID  (или прямой .mp3)", "Включить", "Назад");
			Music_ObjectPlay(playerid, objectid, url);
		}
		case DIALOG_MUSIC_OBJECT_STOP:
		{
			new objectid;
			if(sscanf(inputtext, "d", objectid)) return SPD(playerid, DIALOG_MUSIC_OBJECT_STOP, DIALOG_STYLE_INPUT, "{FAAC58}Трек на объекте", "{FFFFFF}Введите ID объекта:", "Остановить", "Назад");
			Music_ObjectStop(playerid, objectid);
		}
	}
	return 1;
}

stock MakeTime(dest[], sec, size = sizeof(dest))
{
	if(sec < 0) format(dest, size, "?");
	else format(dest, size, "%d:%02d", sec / 60, sec % 60);
}

stock MakeUrl(dest[], const packed[], size = sizeof(dest))
{
	strunpack(dest, packed, size);
	if(!dest[0]) format(dest, size, "-");
}
stock Music_ShowInfo(playerid)
{
	new info[1400], line[400], tc[16], tl[16], u[160], carid = GetPlayerVehicleID(playerid);
	format(info, sizeof(info), "{FAAC58}Машина:\n");
	new aid = carid ? Audio_GetVehicleStreamID(carid) : -1;
	if(aid != -1)
	{
		MakeTime(tc, Audio_GetStreamTime(aid));
		MakeTime(tl, Audio_GetDuration(aid));
		MakeUrl(u, VI[carid][vAudioUrl]);
		format(line, sizeof(line), "{FFFFFF}машина ID %d, поток %d, %s\n%s / %s\nссылка: {CCCCCC}%s\n",
			carid, aid, (Audio_IsStreamPlaying(aid) ? "{9ACD32}играет" : "{B83434}пауза"), tc, tl, u);
		strcat(info, line);
	}
	else strcat(info, "{969696}вы не в машине или трек не играет\n");
	strcat(info, "\n{FAAC58}Колонки и объекты:\n");
	new speakers = 0;
	for(new i; i < MAX_SPEAKERS; i++)
	{
		if(!SpeakerInfo[i][spActive]) continue;
		speakers++;
		MakeTime(tc, Audio_GetStreamTime(SpeakerInfo[i][spAudio]));
		MakeTime(tl, Audio_GetDuration(SpeakerInfo[i][spAudio]));
		MakeUrl(u, SpeakerInfo[i][spUrl]);
		format(line, sizeof(line), "{FFFFFF}#%d объект %d (%s), %s / %s\n{CCCCCC}%s\n",
			i, SpeakerInfo[i][spObject], (SpeakerInfo[i][spOwnObject] ? "колонка" : "чужой объект"), tc, tl, u);
		strcat(info, line);
	}
	if(!speakers) strcat(info, "{969696}нет активных колонок/объектов\n");
	return SPD(playerid, DIALOG_MUSIC, DIALOG_STYLE_MSGBOX, "{FAAC58}Музыка - что играет", info, "Назад", "");
}

stock Music_CarPlay(playerid, const url[])
{
	new carid = GetPlayerVehicleID(playerid);
	if(!carid) return SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Вы не в транспорте!");
	if(!IsUrl(url)) return UrlError(playerid);
	VI[carid][vAudioID] = Audio_AttachStreamToVehicle(carid);
	if(VI[carid][vAudioID] == -1) return SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Не удалось создать поток для машины.");
	Audio_SetStreamUrl(VI[carid][vAudioID], url, 0);
	strpack(VI[carid][vAudioUrl], url);
	return SCMf(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Трек включён в машине ID %d.", carid);
}
stock Music_CarStop(playerid)
{
	new carid = GetPlayerVehicleID(playerid);
	if(!carid) return SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Вы не в транспорте!");
	new audioid = Audio_GetVehicleStreamID(carid);
	if(audioid == -1 || !Audio_IsStreamPlaying(audioid)) return SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}В этой машине трек не играет.");
	Audio_StopStream(audioid);
	return SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Трек в машине остановлен.");
}

stock Music_FreeSlot()
{
	for(new i; i < MAX_SPEAKERS; i++) if(!SpeakerInfo[i][spActive]) return i;
	return -1;
}
stock Music_FindObject(objectid, bool:dynamic)
{
	for(new i; i < MAX_SPEAKERS; i++)
		if(SpeakerInfo[i][spActive] && SpeakerInfo[i][spObject] == objectid && SpeakerInfo[i][spDynamic] == dynamic) return i;
	return -1;
}
stock Music_Start(slot, objectid, bool:dynamic, bool:own, Float:x, Float:y, Float:z, const url[])
{
	if(!SpeakerInfo[slot][spActive])
	{
		new audioid = Audio_CreateStreamAtPos(x, y, z, SPEAKER_DISTANCE);
		if(audioid == -1) return 0;
		SpeakerInfo[slot][spAudio] = audioid;
		SpeakerInfo[slot][spActive] = true;
		SpeakerInfo[slot][spObject] = objectid;
		SpeakerInfo[slot][spDynamic] = dynamic;
		SpeakerInfo[slot][spOwnObject] = own;
		SpeakerInfo[slot][spPos][0] = x;
		SpeakerInfo[slot][spPos][1] = y;
		SpeakerInfo[slot][spPos][2] = z;
	}
	Audio_SetStreamUrl(SpeakerInfo[slot][spAudio], url, 0);
	strpack(SpeakerInfo[slot][spUrl], url);
	return 1;
}
stock Music_Stop(slot)
{
	if(!SpeakerInfo[slot][spActive]) return;
	Audio_DestroyStream(SpeakerInfo[slot][spAudio]);
	if(SpeakerInfo[slot][spOwnObject]) DestroyDynamicObject(STREAMER_TAG_OBJECT:SpeakerInfo[slot][spObject]);
	SpeakerInfo[slot][spActive] = false;
}
stock Music_SpeakerCreate(playerid, const url[])
{
	if(!IsUrl(url)) return UrlError(playerid);
	new slot = Music_FreeSlot();
	if(slot == -1) return SCMf(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Больше %d колонок поставить нельзя.", MAX_SPEAKERS);
	new Float:x, Float:y, Float:z, Float:a;
	GetPlayerPos(playerid, x, y, z);
	GetPlayerFacingAngle(playerid, a);
	x += 1.5 * floatsin(-a, degrees);
	y += 1.5 * floatcos(-a, degrees);
	new objectid = _:CreateDynamicObject(SPEAKER_MODEL, x, y, z - 1.0, 0.0, 0.0, a, GetPlayerVirtualWorld(playerid), GetPlayerInterior(playerid));
	if(!Music_Start(slot, objectid, true, true, x, y, z, url))
	{
		DestroyDynamicObject(STREAMER_TAG_OBJECT:objectid);
		return SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Не удалось создать звук.");
	}
	return SCMf(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Колонка #%d поставлена (объект %d), слышно в радиусе %.0f м.", slot, objectid, SPEAKER_DISTANCE);
}
stock Music_SpeakerRemove(playerid)
{
	new best = -1, Float:bestDist = 10.0, Float:d;
	for(new i; i < MAX_SPEAKERS; i++)
	{
		if(!SpeakerInfo[i][spActive] || !SpeakerInfo[i][spOwnObject]) continue;
		d = GetPlayerDistanceFromPoint(playerid, SpeakerInfo[i][spPos][0], SpeakerInfo[i][spPos][1], SpeakerInfo[i][spPos][2]);
		if(d < bestDist) bestDist = d, best = i;
	}
	if(best == -1) return SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Рядом (10 м) нет колонки.");
	Music_Stop(best);
	return SCMf(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Колонка #%d убрана.", best);
}
stock Music_ObjectPlay(playerid, objectid, const url[])
{
	if(!IsUrl(url)) return UrlError(playerid);
	new Float:x, Float:y, Float:z, bool:dynamic;
	if(IsValidObject(objectid)) GetObjectPos(objectid, x, y, z);
	else if(IsValidDynamicObject(STREAMER_TAG_OBJECT:objectid)) GetDynamicObjectPos(STREAMER_TAG_OBJECT:objectid, x, y, z), dynamic = true;
	else return SCMf(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Объект %d не найден.", objectid);
	new slot = Music_FindObject(objectid, dynamic);
	if(slot == -1) slot = Music_FreeSlot();
	if(slot == -1) return SCMf(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Больше %d источников музыки нельзя.", MAX_SPEAKERS);
	if(!Music_Start(slot, objectid, dynamic, false, x, y, z, url)) return SCM(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Не удалось создать звук.");
	return SCMf(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Трек включён на объекте %d (%s), радиус %.0f м.", objectid, (dynamic ? "streamer" : "обычный"), SPEAKER_DISTANCE);
}
stock Music_ObjectStop(playerid, objectid)
{
	new slot = Music_FindObject(objectid, false);
	if(slot == -1) slot = Music_FindObject(objectid, true);
	if(slot == -1) return SCMf(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}На объекте %d трек не играет.", objectid);
	Music_Stop(slot);
	return SCMf(playerid, 0xFAAC58FF, "[Музыка] {FFFFFF}Трек на объекте %d остановлен.", objectid);
}

