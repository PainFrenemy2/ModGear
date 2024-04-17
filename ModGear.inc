public ModGear_IncludeFileVersion = 0x055;
#pragma unused ModGear_IncludeFileVersion
enum
{
	Custom_Speedometer,    // 0: Off, 1: Advanced (PC), 2: Simplified (PC), 3: Mobile Speedometer (1), 4: Mobile Speedometer (2)
	Custom_Hud,            // 0: Off, 1: PUBG (PC), 2: Vice City (PC), 3: San Andreas (PC), 4: Custom (1) (Mobile), 5: Custom (2) (Mobile), 6: Custom (3) (Mobile)
	Custom_Radar,          // 0: Off, 1: Custom (1) (PC/Mobile), 2: Custom (2) (PC/Mobile), 3: Vice City (PC)
	Custom_Fuel,           // 0 to 300 (PC/Mobile)
	Custom_Milage,         // 0 to 999999 (PC/Mobile)
	Custom_TypeFuel,       // 0: Petrol, 1: Electric, 2: Hide Icon (PC)
	Custom_Hunger,         // 0 to 100 (PC/Mobile)
	Custom_Hints,          // 0: Off, 1: On (PC)
	Custom_TypeNickName,   // 0: Off, 1: On (PC/Mobile)
	Custom_TypeDialog,     // 0: Full, 1: Simplified, 2: Simple (PC/Mobile)
	Custom_Map,            // 0: Off, 1: On (Mirror View) (PC/Mobile)
	Custom_Discord,        // 0: Off, 1: On (Discord Stream) (PC/Mobile)
	Custom_Dialog,         // 0: Off, 1: On (Save Dialog) (PC/Mobile)
	Custom_Chat,           // 0: Off, 1: On (Transparent Chat) (PC/Mobile)
	Custom_SetRadius,      // 0 to 500 (3D Text Drawing) (PC/Mobile)
	Custom_Show,           // 0: Off, 1: On (Snowfall) (PC/Mobile)
	Custom_Wanted,         // 0 to 7 (Change Wanted Level) (PC/Mobile)
	Custom_ChatBoard,      // 0: Off, 1: On (Player Text Board) (PC/Mobile)
	Custom_WallHack,       // 0: Off, 1: On (Player Wall Hack) (PC/Mobile)
	Custom_TypeAim		   // 0 to 10 (Type of Aim) (PC/Mobile)
};
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
//Server
native UpdateInfoHudGtaSa(ServerNumber, const ServerName[], const ServerProject[], const ServerType[], ServerFlag, ServerLogo, XPayday, playerid = -1);
//Player
native SetPlayerLauncherStatus(playerid, bool:status);
native GetPlayerLauncherStatus(playerid);
native SetPlayerCollisionStatus(playerid, bool:status);

native ShowYouTubeVideoPhone(playerid, params[]);
native HideYouTubeVideoPhone(playerid);

native InterfaceCef(playerid, index, type);
native ShowCef(playerid, const event[]);
native ShowCefMobile(playerid, arg_1, arg_2, const event[]);
native ShowSelectCef(playerid, bool:status);

native SetPlayerAttachedObjectEx(playerid, index, bool:create, modelid, bone, Float:fOffsetX = 0.0, Float:fOffsetY = 0.0, Float:fOffsetZ = 0.0, Float:fRotX = 0.0, Float:fRotY = 0.0, Float:fRotZ = 0.0, Float:fScaleX = 1.0, Float:fScaleY = 1.0, Float:fScaleZ = 1.0, materialcolor1 = 0, materialcolor2 = 0);
native RemovePlayerAttachedObjectEx(playerid, index);
native IsPlayerAttachedObjectSlotEx(playerid, index);

#define CHAT_MOD_VIP                   (0)
#define CHAT_MOD_ORG                   (1)
#define CHAT_MOD_ORG_NONRP             (2)
#define CHAT_MOD_FAMILY                (3)
#define CHAT_MOD_NONRP                 (4) 
#define CHAT_MOD_SHOUT                 (5)
#define CHAT_MOD_WHISPER               (6)
#define CHAT_MOD_WORK                  (7)
native SetPlayerChatRoom(playerid, type, bool:status);

#define HOT_KEY_RADIO                  (0)
#define HOT_KEY_FRACTION_RADIO         (1)
#define HOT_KEY_FAMILY_RADIO           (2)
#define HOT_KEY_INTERACTION            (3)
#define HOT_KEY_PHONE                  (4)
native UpdateInfoHotKey(playerid, type, const nameKey[] = "null");

//Vehicle
#define VEHICLE_HYDRAULICS_OFF         (0)
#define VEHICLE_HYDRAULICS_HIGH        (1)
#define VEHICLE_HYDRAULICS_LOW         (2)
native SetVehicleHidraulics(carid, type);

#define VEHICLE_TONER_frontal		   (0)
#define VEHICLE_TONER_frontal_f		   (1)
#define VEHICLE_TONER_frontal_r		   (2)
#define VEHICLE_TONER_frontal_d		   (3)
#define VEHICLE_TONER_FRONT_LIGHT	   (4)
#define VEHICLE_TONER_REAR_LIGHT	   (5)
#define VEHICLE_TONER_REAR_GLASS	   (6)
native VehicleToneCustomizer(carid, type, red_color, green_color, blue_color, brigh_size, bool:status = true);

#define VEHICLE_COMPONENT_CAPOT		   (1)
#define VEHICLE_COMPONENT_SPOILER	   (2)
#define VEHICLE_COMPONENT_GLUSHITEL	   (3)
#define VEHICLE_COMPONENT_DISKI		   (4)
native GetVehicleInComponentSlot(carid, slot);
native InstallVehicleComponent(carid, slot, component);

native GetVehicleInSlotComponent(carid, component);
native RemoveAttachComponent(carid, component);
native SetVinylCustomization(carid, type, color);
native CustomizeWheelSettings(carid, type, Float:corner_wheel, Float:drop_wheel);
native CustomizeWheelSize(carid, Float:size_wheel = 0.85);
native NeonColorChange(carid, color);
native SmokeColorShift(carid, color);
native SetColorHeadlamp(carid, color);
native InstallNumberVehicle(carid, type, const number[], const region[]);
native SetVehicleColorWheel(carid, color);
native SetShotsExhaust(carid, bool:status);
//Audio
native Audio_AttachStreamToVehicle(vehicleid);
native Audio_AttachStreamToPlayer(playerid); 
native Audio_UpdateAudioStreamID(audioID, const urlmp3[], playtime = 0);
native Audio_GetVehicleStreamID(vehicleid);
native Audio_GetPlayerStreamID(playerid);
native Audio_GetStreamTime(audioID);
native Audio_SetStreamTime(audioID, time);
native Audio_DestroyStream(audioID);
native Audio_IsStreamPlaying(audioID);
native Audio_StopStream(audioID);
native Audio_PlayStream(audioID);
//Other
native ConvertedStringInt(&key_1, &key_2, &key_3, const text[]);
native ConvertedIntString(key_1, key_2, key_3, const text[]);