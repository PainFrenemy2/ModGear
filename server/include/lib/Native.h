#pragma once
#include <initializer_list>
#include <raknet/bitstream.h>
#include <raknet/networktypes.h>
#include <lib/InfoServer.h>
#include <pawn/amx/amx.h>
#include <pawn/plugincommon.h>

class Native
{
public:
    static bool MBSendPacket(int player_id, BitStream* bs, int priority, int reliabilit);
    // строка для CEF-пакетов ядра Arizona: u16 длина, u8 сжато (0), байты
    static void WriteCefString(BitStream& bs, const char* text);
    // 220:108 - табличка "печатает" над игроком: u32 id игрока, u8 флаг
    static void SendChatTyping(int to, int playerid, int flag);
    static bool MBSendRPC(int player_id, RPCIndex rpc_id, BitStream* bs, int priority, int reliability);
    static bool LoadFileCef(int playerid, int browserid, const char* url, const char* key);
    static bool ShowCefMobile(int playerid, int ar1, int ar2, const char* format, ...);
    static bool ShowCustom(int playerid, std::initializer_list<int> args);
    static bool ShowCef(int playerid, const char* format, ...);
    static bool ShowCustomRPC(int playerid, int idrpc, std::initializer_list<int> args);
    static bool b_InterfaceCEF(int playerid, int index, int type);
    static int OnPlayerEditAttachedObjectEx(int playerid, int response, int index, int modelid, int boneid, float fOffsetX, float fOffsetY, float fOffsetZ, float fRotX, float fRotY, float fRotZ, float fScaleX, float fScaleY, float fScaleZ);
    static int CallingBackIcon(int icon);
    static int CallingBackSkin(int skin);
    static int CallingBackVehicle(int veh);
    static int CallingBackWeapon(int weapon);  // public GetCustomWeapon: кастомное оружие -> базовое (игрокам без лаунчера)
    static bool	SendPackerPlayer(BitStream& amx, int idrpc, int vehicleid = -1, int playerid = -1, int typeidd = 0);
    static bool b_InstallVehicleComponent(int carid, int type, int component, int playerid = -1);
    static bool b_RemoveVehicleComponent(int carid, int component);
    static int GetVehicleComponentSlot(int carid, int component); // 0 - детали на машине нет
    static bool b_SetVehicleColor(int carid, int color, int playerid = -1);
    static void RestoreBodyMaterials(int carid, int playerid = -1);
    static bool b_SetVinylCustomization(int carid, int type, int color, int playerid = -1);
    static bool b_WheelSettingsCustom(int carid, int type, float flot, float flot2, int playerid = -1);
    static bool b_CustomizeCarWheel(int carid, float diamet, int playerid = -1);
    static bool b_NeonColorChange(int carid, int color, int playerid = -1);
    static bool b_SetVehicleHidraulics(int carid, int type, int playerid = -1);
    static bool b_SmokeColorShift(int carid, int color, int playerid = -1);
    static bool b_SetColorHeadlamp(int carid, int color, int playerid = -1);
    static bool b_SetShotsExhaust(int carid, int status, int playerid = -1);
    static bool b_VehicleToneCustomizer(int carid, int type, int red, int green, int blue, int size, int playerid = -1);
    static bool b_InstallNumberVehicle(int carid, int type, char number[10], char region[20], int playerid = -1);
    static bool b_SetVehicleColorWheel(int carid, int color, int playerid = -1, int type = 0);
    // пакет 220, разобран по клиенту Arizona (core.asi)
    static bool b_SetVehicleStrobeLights(int carid, int mode, float speed, bool beam, int playerid = -1); // 220:209
    static bool b_SetVehicleBrakeCalipers(int carid, int model, bool simple, int playerid = -1);         // 220:220
    static bool b_SetVehicleDrift(int carid, bool status, int playerid = -1);                            // 220:187
    static bool b_SetVehicleSiren(int carid, bool status, int playerid = -1);                            // 220:152
    static bool b_SetPlayerSpeedLimit(int playerid, float speed, int modelid);                           // 220:111
    // аудио (RPC 252, как у Arizona). index - номер в aTemp, playerid -1 = всем, у кого источник в прорисовке
    static void Audio_SendZone(int index, int playerid = -1);    // 10: создать зону (2 - игрок, 3 - машина)
    static void Audio_SendPlay(int index, int playerid = -1);    // 17: включить трек
    static void Audio_SendStop(int index, int playerid = -1);    // 13: остановить
    static void Audio_SendEffect(int index, int playerid = -1);  // 18 тип 2: звук как из рации / 16: снять эффекты
    static void Audio_SendDestroy(int index, int playerid = -1); // 11: удалить зону
    static void Audio_Free(int index);                           // удалить зону у всех и освободить поток
    static void Audio_SendPointZones(int playerid);              // зоны-точки (тип 4), свой фон (тип 1) и их треки одному игроку
    static long long SteadyMs();
    static void SendHotKey(int playerid, int type);              // клавиша подсказки HOT_KEY_* (если подсказки показаны - сразу видна)
    static void Audio_SyncTime(int index);                       // aTime из aZeroMs (пока трек играет)
    static void Audio_SetStarted(int index);                     // трек пошёл с aTime: запомнить aZeroMs
    static bool b_ShowPlayerTag(int playerid, int forplayerid, bool status);
    static bool b_SetPlayerCollisionStatus(int playerid, int forplayerid, uint8_t status);
    static bool ResetAcs(int playerid, int uidtype, int forplayerid);
    static bool AttachPlyerAcs(int playerid, int forplayerid, int index, bool create, int bone = 0, int model = 0, float fSetX = 0.0f, float fSetY = 0.0f, float fSetZ = 0.0f, float fRotX = 0.0f, float fRotY = 0.0f, float fRotZ = 0.0f, float fScaleX = 0.0f, float fScaleY = 0.0f, float fScaleZ = 0.0f, int color_1 = 0, int color_2 = 0);

private:
};