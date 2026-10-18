main(){}
// qawno (open.mp): режим совместимости с SA-MP - без строгих тегов и предупреждений обёртки a_samp
#define SAMP_COMPAT
#include <a_samp>
#include <Pawn.CMD>
#include <sscanf2>
#include <ModGearAlfa>
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

#define MODGEAR_VERSION_MAJOR ((ModGear_IncludeFileVersion >> 8) & 0xF)
#define MODGEAR_VERSION_MINOR ((ModGear_IncludeFileVersion >> 4) & 0xF)
#define MODGEAR_VERSION_PATCH (ModGear_IncludeFileVersion & 0xF)

#define SCM                             SendClientMessage
#define SPD                             ShowPlayerDialog
#define SCMf                            SendClientMessagef
#define SPDf                            ShowPlayerDialogf

enum infcreate
{
	icVehID,
	icModel,
	Float:icPos[4],
	icColor[2]
};
new InfoCreateVehicle[][infcreate] =
{
	{0, 612, {1325.7351,1279.0178,10.5827,359.3188}, {6, 1}},
	{0, 613, {1312.5683,1279.0178,10.5837,359.3188}, {6, 1}},
	{0, 614, {1319.1517,1279.0178,10.5837,359.3188}, {6, 1}},
	{0, 3201, {1305.9849,1279.0178,10.5837,359.3188}, {6, 1}}
};
new kinema;
new dialogtextverison[100];
enum enminfPlayer
{
	pLauncherStatus,
	pVehicle
};
enum enminfVehicle
{
	vLight,
	vAudioID
};
new PI[MAX_PLAYERS][enminfPlayer];
new VI[MAX_VEHICLES][enminfVehicle];
public OnPlayerConnect(playerid)
{
    PI[playerid][pVehicle] = 0;
	static versionsamp[24];
	GetPlayerVersion(playerid, versionsamp, 24);
	if(!strcmp(versionsamp, "Arizona PC")) SetPlayerLauncherStatus(playerid, true), PI[playerid][pLauncherStatus] = 1;
	else if(!strcmp(versionsamp, "arizona-mobile"))
	{
		SetPlayerLauncherStatus(playerid, true);
		PI[playerid][pLauncherStatus] = 2;
	}
	else SetPlayerLauncherStatus(playerid, false), PI[playerid][pLauncherStatus] = 0;
	SCMf(playerid, 0xFAAC58FF, "Добро пожаловать на тестовый сервер плагина {FFFFFF}ModGearAlfa %d.%d.%dv!", MODGEAR_VERSION_MAJOR, MODGEAR_VERSION_MINOR, MODGEAR_VERSION_PATCH);
	SCM(playerid, 0xFAAC58FF, "Команды: {FFFFFF}/sp /rep /flip /st /sw");

	SetPlayerSkillLevel(playerid,WEAPONSKILL_PISTOL,1000);
    SetPlayerSkillLevel(playerid,WEAPONSKILL_PISTOL_SILENCED,1000);
    SetPlayerSkillLevel(playerid,WEAPONSKILL_DESERT_EAGLE,1000);
    SetPlayerSkillLevel(playerid,WEAPONSKILL_SHOTGUN,1000);
    SetPlayerSkillLevel(playerid,WEAPONSKILL_MICRO_UZI,1000);
    SetPlayerSkillLevel(playerid,WEAPONSKILL_MP5,1000);
    SetPlayerSkillLevel(playerid,WEAPONSKILL_AK47,1000);
    SetPlayerSkillLevel(playerid,WEAPONSKILL_M4,1000);
    SetPlayerSkillLevel(playerid,WEAPONSKILL_SNIPERRIFLE,1000);
	return 1;
}
public OnPlayerDisconnect(playerid, reason)
{
	if(PI[playerid][pVehicle]) DestroyVehicle(PI[playerid][pVehicle]);
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
	if(GetPlayerState(playerid) == PLAYER_STATE_DRIVER) SetVehiclePos(GetPlayerVehicleID(playerid), fX, fY, fZ+1.0);
	else SetPlayerPos(playerid, fX, fY, fZ+1.0);
	SetPlayerInterior(playerid, 0);
	SetPlayerVirtualWorld(playerid, 0);
    return 1;
}
public OnPlayerSpawn(playerid)
{
	GivePlayerWeapon(playerid, 31, 5000);
	GivePlayerWeapon(playerid, 24, 5000);
	SetPlayerSkin(playerid, 102);
	SetPlayerVirtualWorld(playerid, 0);
	SetPlayerInterior(playerid, 0);
	return 1;
}
public OnPlayerDeath(playerid, killerid, reason)
{
   	return 1;
}
public OnPlayerRequestClass(playerid, classid)
{
	SetSpawnInfo(playerid, 255, 102, 1319.2843,1261.8027,10.8203, 358.2892, 0, 0, 0, 0, 0, 0);
	SpawnPlayer(playerid);
	return 1;
}
public OnPlayerKeyStateChange(playerid, newkeys, oldkeys)
{
    if(((newkeys & (KEY_ACTION)) == (KEY_ACTION)) && ((oldkeys & (KEY_ACTION)) != (KEY_ACTION)))
	{
		new carid = GetPlayerVehicleID(playerid);
		if(carid != 0)
		{
			new engine,lights,alarm,doors,bonnet,boot,objective;
			GetVehicleParamsEx(carid,engine,lights,alarm,doors,bonnet,boot,objective);
			VI[carid][vLight] = !VI[carid][vLight];
			SetVehicleParamsEx(carid,engine,VI[carid][vLight],alarm,doors,bonnet,boot,objective);
		}
	}
	return 1;
}
public OnPlayerCommandPerformed(playerid, cmd[], params[], result, flags)
{
	if(result == -1) return SCM(playerid, 0xFAAC58FF, "[Информация] {FFFFFF}Неизвестная команда! Для посмотра доступных команд введите {FAAC58}/cmd");
    return 1;
}
public OnGameModeInit()
{
	new days, year, month, hour, minutes;
    getdate(year, month, days);
    gettime(hour,minutes);
	SendRconCommandf("%s ModGearAlfa | Тестовый сервер | %d.%d.%dv", RCON_SERVER_NAME, MODGEAR_VERSION_MAJOR, MODGEAR_VERSION_MINOR, MODGEAR_VERSION_PATCH);

	static string[50];
	format(string, sizeof(string), "Build: %02d.%02d.%i %02d:%02d", days, month, year, hour, minutes);

	SetGameModeText(string);
	SetWorldTime(1);
	CreateDynamicMapIcon(1319.2843,1261.8027,10.8203, 87, 0, -1, -1, -1);
    ApplyDynamicActorAnimation(CreateDynamicActor(102, 1269.4082,1308.1591,10.8203,211.0356), "JST_BUISNESS", "GIRL_02", 4.1, 1, 1, 1, 1, 1);
    ApplyDynamicActorAnimation(CreateDynamicActor(312, 1332.4679,1262.8011,10.8203,89.2910), "BD_FIRE", "M_SMKLEAN_LOOP", 4.1, 1, 1, 1, 1, 1);
	for(new i; i < sizeof(InfoCreateVehicle); i++)
	{
		InfoCreateVehicle[i][icVehID] = AddStaticVehicleEx(InfoCreateVehicle[i][icModel], InfoCreateVehicle[i][icPos][0],InfoCreateVehicle[i][icPos][1], InfoCreateVehicle[i][icPos][2], InfoCreateVehicle[i][icPos][3], InfoCreateVehicle[i][icColor][0],InfoCreateVehicle[i][icColor][1], 300);
	}
	kinema = CreateObject(7914, 1278.931274, 1311.162231, 13.150308, 0.000000, 0.000000, -0.200000);
	SetObjectMaterial(kinema, 0, 10765, "airportgnd_sfse", "black64", 0);
	CreateDynamicObject(1294, 1272.001953, 1310.312011, 13.472962, 0.000000, 0.000000, 90.100128, 0, 0, -1, 300.00, 300.00);
	CreateDynamicObject(1294, 1275.063354, 1310.315185, 13.472962, 0.000000, 0.000000, 90.100128, 0, 0, -1, 300.00, 300.00);
	CreateDynamicObject(1294, 1278.674194, 1310.321533, 13.472962, 0.000000, 0.000000, 90.100128, 0, 0, -1, 300.00, 300.00);
	CreateDynamicObject(1294, 1282.084838, 1310.326538, 13.472962, 0.000000, 0.000000, 90.100128, 0, 0, -1, 300.00, 300.00);
	CreateDynamicObject(1294, 1285.575805, 1310.331665, 13.472962, 0.000000, 0.000000, 90.100128, 0, 0, -1, 300.00, 300.00);
	CreateDynamicObject(2232, 1270.074584, 1311.105957, 11.498214, 0.000000, 0.000000, -5.199999, 0, 0, -1, 300.00, 300.00);
	CreateDynamicObject(2232, 1270.074584, 1311.105957, 10.418211, 0.000000, 0.000000, -5.199999, 0, 0, -1, 300.00, 300.00);
	CreateDynamicObject(2232, 1270.074584, 1311.105957, 12.618223, 0.000000, 0.000000, -5.199999, 0, 0, -1, 300.00, 300.00);
	CreateDynamicObject(2232, 1270.074584, 1311.105957, 13.758232, 0.000000, 0.000000, -5.199999, 0, 0, -1, 300.00, 300.00);
	CreateDynamicObject(2232, 1287.801879, 1310.939331, 10.418211, 0.000000, 0.000000, -5.199999, 0, 0, -1, 300.00, 300.00);
	CreateDynamicObject(2232, 1287.801879, 1310.939331, 11.498229, 0.000000, 0.000000, -5.199999, 0, 0, -1, 300.00, 300.00);
	CreateDynamicObject(2232, 1287.801879, 1310.939331, 12.588232, 0.000000, 0.000000, -5.199999, 0, 0, -1, 300.00, 300.00);
	CreateDynamicObject(2232, 1287.801879, 1310.939331, 13.768239, 0.000000, 0.000000, -5.199999, 0, 0, -1, 300.00, 300.00);
	CreateDynamicObject(1728, 1267.234375, 1307.696289, 9.753808, 0.000000, 0.000000, 31.900003, 0, 0, -1, 300.00, 300.00);
	CreateDynamicObject(7415, 1261.762207, 1303.679931, 10.427982, 26.400014, -7.800004, -178.999969, 0, 0, -1, 300.00, 300.00);
	CreateDynamicObject(18750, 1325.764404, 1228.404174, 42.898155, 99.900001, -176.400009, -2.500005, 0, 0, -1, 300.00, 300.00);

	format(dialogtextverison, sizeof(dialogtextverison), "{FAAC58}ModGearAlfa {FFFFFF}%d.%d.%dv", MODGEAR_VERSION_MAJOR, MODGEAR_VERSION_MINOR, MODGEAR_VERSION_PATCH);
	printf("[Server]: Тестовый сервер успешно загружен! | %s", string);
	return 1;
}
public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
	switch(dialogid)
	{
		case 1:
		{
			if(!response) return 1;
			ShowPlayerLaucherSettings(playerid, listitem);
		}
		case 3:
		{
			new type = GetPVarInt(playerid, "TypeLauncherSettings");
			if(!response) return ShowPlayerLaucherSettings(playerid, (type > 3 ? -1 : type));
			new amount;
			sscanf(inputtext, "d", amount);
			switch(type)
			{
				case 0:
				{
					if(PI[playerid][pVehicle]) DestroyVehicle(PI[playerid][pVehicle]);
			        new Float:x, Float:y, Float:z, Float:a;
			        GetPlayerPos(playerid, x, y, z);
			        GetPlayerFacingAngle(playerid, a);
			        PI[playerid][pVehicle] = AddStaticVehicle(amount, x, y, z, a, random(100), random(100));
			        PutPlayerInVehicle(playerid, PI[playerid][pVehicle], 0);
				}
				case 1: SetPlayerSkin(playerid, amount);
				case 2: GivePlayerWeapon(playerid, amount, 5000);
				default: return 0;
			}
			return ShowPlayerLaucherSettings(playerid, (type > 3 ? -1 : type));
		}
	}
	return 1;
}
stock ShowPlayerLaucherSettings(playerid, type = -1)
{
	SetPVarInt(playerid, "TypeLauncherSettings", type);
	switch(type)
	{
		case 0: SPD(playerid, 3, DIALOG_STYLE_INPUT, dialogtextverison, "{FFFFFF}Введите модель авто\n{CCCCCC}Пример: 612", "Выдать", "Назад");
		case 1: SPD(playerid, 3, DIALOG_STYLE_INPUT, dialogtextverison, "{FFFFFF}Введите ид скина\n{CCCCCC}Пример: 312", "Выдать", "Назад");
		case 2: SPD(playerid, 3, DIALOG_STYLE_INPUT, dialogtextverison, "{FFFFFF}Введите ид оружия\n{CCCCCC}Пример: 41", "Выдать", "Назад");
		default: SPD(playerid, 1, DIALOG_STYLE_TABLIST, dialogtextverison, "{FAAC58}[Player]: {FFFFFF}Выдать транспорт\n{FAAC58}[Player]: {FFFFFF}Выдать скин\n{FAAC58}[Player]: {FFFFFF}Выдать оружие", "Выбрать", "Закрыть");
	}
	return 1;
}
CMD:menu(playerid) { return ShowPlayerLaucherSettings(playerid); }
CMD:sw(playerid, params[])
{
    if(sscanf(params,"d",params[0])) return SCM(playerid, 0xFAAC58FF, "[Информация] {FFFFFF}Используйте: /sw [type]");
	SetWeather(params[0]);
	return SCMf(playerid, 0xFAAC58FF, "[Информация] {FFFFFF}Вы успешно сменили погоду на %d", params[0]);
}
CMD:st(playerid, params[])
{
    if(sscanf(params,"d",params[0])) return SCM(playerid, 0xFAAC58FF, "[Информация] {FFFFFF}Используйте: /st [hour]");
	SetWorldTime(params[0]);
	return SCMf(playerid, 0xFAAC58FF,  "[Информация] {FFFFFF}Вы успешно сменили время на %02d:00", params[0]);
}
CMD:rep(playerid)
{
	new id;
	if(GetPlayerState(id) != 2) return SCM(playerid, 0xFAAC58FF, "[Информация] {FFFFFF}Вы не в транспорте!");
	new veh = GetPlayerVehicleID(id);
	RepairVehicle(veh);
    SetVehicleHealth(veh, 1500);
	return SCM(playerid, 0xFAAC58FF, "[Информация] {FFFFFF}Вы успешно починили транспорт.");
}
CMD:sp(playerid)
{
	if(IsPlayerInAnyVehicle(playerid))
	{
	    new Float:X, Float:Y, Float:Z;
	    GetPlayerPos(playerid, X, Y, Z);
		SetPlayerPos(playerid, X ,Y, Z);
	}
	SpawnPlayer(playerid);
	return SCM(playerid, 0xFAAC58FF, "[Информация] {FFFFFF}Вы заспавнили себя.");
}
CMD:flip(playerid)
{
	new id;
    if(GetPlayerState(id) != 2) return SCM(playerid, 0xFAAC58FF, "[Информация] {FFFFFF}Вы не в транспорте!");
	new veh = GetPlayerVehicleID(id);
    new Float:X,Float:Y,Float:Z,Float:A;
    GetVehiclePos(veh,X,Y,Z);
    SetVehiclePos(veh,X,Y,Z);
    GetVehicleZAngle(veh, A);

    SetVehicleZAngle(veh, A);
	RepairVehicle(veh);
    SetVehicleHealth(veh, 1500);
	return SCM(playerid, 0xFAAC58FF, "[Информация] {FFFFFF}Вы успешно флип'нули транспорт.");
}
CMD:tpcor(playerid, params[])
{
    new Float:x, Float:y, Float:z;
    if(sscanf(params, "p<,>fff", x, y, z)) return SCM(playerid, 0xFAAC58FF, "[Информация] {FFFFFF}Используйте: /tpcor [x], [y], [z]");
    return SetPlayerPos(playerid, x, y, z);
}