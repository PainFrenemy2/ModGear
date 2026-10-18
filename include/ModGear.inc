/*
 * ============================================================================
 *   __  __           _  ____
 *  |  \/  | ___   __| |/ ___| ___  __ _ _ __       Функции лаунчера Arizona RP
 *  | |\/| |/ _ \ / _` | |  _ / _ \/ _` | '__|      для SA-MP 0.3.7 и open.mp
 *  | |  | | (_) | (_| | |_| |  __/ (_| | |
 *  |_|  |_|\___/ \__,_|\____|\___|\__,_|_|         Автор: PainFrenemy2
 *
 * ============================================================================
 *  Это основной файл: лаунчер, сервер, утилиты, замена своих моделей.
 *  Остальное подключай в моде по необходимости:
 *
 *      <ModGear_Interface>   худ, CEF, чат, диалоги, навигатор, мышь
 *      <ModGear_Player>      игрок: ник, аксессуары, оружие, режимы, камера
 *      <ModGear_Vehicle>     машины: тюнинг, грязь, сцепка, хендлинг
 *      <ModGear_World>       мир: объекты, частицы, надписи в небе, зоны
 *      <ModGear_Media>       медиасервер, музыка, видео на экранах
 *      <ModGear_Voice>       голосовые каналы, рации и вебкамера
 *      <ModGear_NPC>         педы и охранники (libPED)
 *
 *  Каждый модуль сам подключает <ModGear>.
 *
 *  GitHub:        https://github.com/PainFrenemy2/ModGear
 *  Подключение:   #include <ModGear>
 *  Документация:  https://painfrenemy2.github.io/ModGear/
 *  Описание есть и в архиве релиза: ModGear.html
 * ============================================================================
 */

#if defined _ModGear_included
	#endinput
#endif
#define _ModGear_included

public ModGear_IncludeFileVersion = 0x06B;
#pragma unused ModGear_IncludeFileVersion

// ----------------------------------------------------------------------------
//  ЛАУНЧЕР
// ----------------------------------------------------------------------------
native SetPlayerLauncherStatus(playerid, bool:status);          // отметить, что игрок зашёл с лаунчера. Вызывать первым
native GetPlayerLauncherStatus(playerid);                       // 1 - игрок с лаунчером

// ----------------------------------------------------------------------------
//  СЕРВЕР
// ----------------------------------------------------------------------------
// данные сервера на худе: номер, название, проект, тип, флаг, логотип, множитель PayDay; playerid -1 - всем
native SetServerHudInfo(number, const name[], const project[], const type[], flag, logo, payday, playerid = -1);

// ----------------------------------------------------------------------------
//  УТИЛИТЫ
// ----------------------------------------------------------------------------
native StringToKeys(&key_1, &key_2, &key_3, const text[]);      // строка -> 3 числа (для БД)
native KeysToString(key_1, key_2, key_3, const text[]);         // 3 числа -> строка

// ----------------------------------------------------------------------------
//  ЗАМЕНА СВОИХ МОДЕЛЕЙ НА СТАНДАРТНЫЕ ДЛЯ ИГРОКОВ БЕЗ ЛАУНЧЕРА.
//  Верни -1, если замены нет.
// ----------------------------------------------------------------------------
forward GetCustomIcon(icon);
public GetCustomIcon(icon)
{
	new defaulticon = -1;
	switch(icon)
	{
		case 87: defaulticon = 6;
		case 88: defaulticon = 7;
	}
	return defaulticon;
}
forward GetCustomVehicle(veh);
public GetCustomVehicle(veh)
{
	new defaultveh = -1;
	switch(veh)
	{
		case 612: defaultveh = 412;
		case 613: defaultveh = 411;
		case 614: defaultveh = 400;
		case 3201: defaultveh = 411;
	}
	return defaultveh;
}
forward GetCustomWeapon(weaponid);
public GetCustomWeapon(weaponid) // кастомное оружие Arizona (core/custom_weapon.txt: id, модель, базовое, иконка)
{
	new defaultweapon = -1;
	switch(weaponid)
	{
		case 104, 106, 108: defaultweapon = 4;
		case 99, 100, 110: defaultweapon = 5;
		case 88, 107: defaultweapon = 8;
		case 90, 91: defaultweapon = 16;
		case 73, 89, 93, 109: defaultweapon = 22;
		case 71, 72, 74, 75, 76, 87, 101, 105, 123, 124, 125, 137, 138, 139, 140, 153: defaultweapon = 24;
		case 94, 103, 149, 150, 151, 152: defaultweapon = 25;
		case 119, 120, 121, 122, 157: defaultweapon = 26;
		case 80, 86, 97: defaultweapon = 27;
		case 98, 111, 112, 113, 114, 154: defaultweapon = 28;
		case 81, 84: defaultweapon = 29;
		case 85, 102, 126, 127, 128, 133, 134, 135, 136, 155: defaultweapon = 30;
		case 79, 82, 96, 129, 130, 131, 132, 141, 142, 143, 144, 156: defaultweapon = 31;
		case 77, 78, 83, 115, 116, 117, 118, 158: defaultweapon = 32;
		case 95: defaultweapon = 33;
		case 92, 145, 146, 147, 148: defaultweapon = 34;
	}
	return defaultweapon;
}
forward GetCustomSkin(skin);
public GetCustomSkin(skin)
{
	new defaultskin = -1;
	switch(skin)
	{
		case 312: defaultskin = 102;
		case 313: defaultskin = 103;
	}
	return defaultskin;
}

