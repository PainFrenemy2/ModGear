/*
 * ============================================================================
 *  ModGearAlfa  -  облегчённая редакция ModGear
 * ----------------------------------------------------------------------------
 *  Отдельная редакция плагина (ModGearAlfa.dll) с базовыми функциями.
 *  Подключай вместо <ModGear>, а не вместе с ним.
 *
 *  Подключение:   #include <ModGearAlfa>
 *  Документация:  https://htmlpreview.github.io/?https://raw.githubusercontent.com/PainFrenemy2/ModGear/HEAD/docs/index.html
 * ============================================================================
 */

public ModGear_IncludeFileVersion = 0x012;
#pragma unused ModGear_IncludeFileVersion

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

//Player
native SetPlayerLauncherStatus(playerid, bool:status);
native GetPlayerLauncherStatus(playerid);
//Other
native StringToKeys(&key_1, &key_2, &key_3, const text[]);
native KeysToString(key_1, key_2, key_3, const text[]);
