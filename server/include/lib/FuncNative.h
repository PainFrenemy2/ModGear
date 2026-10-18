#pragma once
#include <lib/Native.h>
#include "../../DebugLog.h"
#include "../../Events.h"
#include "../../Platform.h"
#include <lib/Handling.h>
#include <lib/Video.h>
#include <lib/Voice.h>
#include <lib/Webcam.h>
#include <lib/ArzExtra.h>
#include <lib/ArzChat.h>
#include <lib/ArzCore.h>
#include "../../Media.h"

bool CheckNumberOfArguments(AMX* amx, const cell* params, int num_expected)
{
	if (((int)params[0] / (int)sizeof(cell)) >= num_expected) return true;
	else amx_RaiseError(amx, AMX_ERR_PARAMS);
	return false;
}

static cell AMX_NATIVE_CALL n_InterfaceCEF(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		arg_index,
		arg_type,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected)) return 0;
	CustomType index = static_cast<CustomType>(params[arg_index]);
	int type = static_cast<int>(params[arg_type]);
	if (type < 0) return 0;
	Native::b_InterfaceCEF(static_cast<int>(params[arg_playerid]), index, type);
	return 1;
}
static cell AMX_NATIVE_CALL n_RemoveAttachComponent(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		arg_component,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_carid] <= 0 || !params[arg_component] || params[arg_carid] >= MAX_VEHICLES) return 0;
	return Native::b_RemoveVehicleComponent((int)params[arg_carid], (int)params[arg_component]) ? 1 : 0;
}
static cell AMX_NATIVE_CALL n_CustomizeCarWheel(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		arg_float,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected)) return 0;
	float float_value = amx_ctof(params[arg_float]);
	if (params[arg_carid] <= 0 || float_value > 1.05f || float_value < 0.0f || params[arg_carid] >= MAX_VEHICLES) return 0;
	vTemp[params[arg_carid]].vDiameteWheel = float_value;
	Native::b_CustomizeCarWheel(params[arg_carid], float_value);
	return 1;
}
static cell AMX_NATIVE_CALL n_GetVehicleInSlotComponent(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		arg_component,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_carid] <= 0 || !params[arg_component] || params[arg_carid] >= MAX_VEHICLES) return 0;
	return Native::GetVehicleComponentSlot((int)params[arg_carid], (int)params[arg_component]);
}
static cell AMX_NATIVE_CALL n_GetVehicleInComponentSlot(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		arg_slot,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_carid] <= 0 || !params[arg_slot] || params[arg_slot] >= PART_SLOTS || params[arg_slot] < 0 || params[arg_carid] >= MAX_VEHICLES) return 0;
	if (params[arg_slot] == PART_WHEELS) return vTemp[params[arg_carid]].vDiski;
	return vTemp[params[arg_carid]].vParts[params[arg_slot]];
}
static cell AMX_NATIVE_CALL n_InstallVehicleComponent(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		arg_type,
		arg_component,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || !params[arg_carid] || params[arg_type] < 0 || params[arg_type] >= PART_SLOTS) return 0;
	return Native::b_InstallVehicleComponent(params[arg_carid], params[arg_type], params[arg_component]) ? 1 : 0;
}
static bool HideYouTube(int playerid)
{
	BitStream bs;
	bs.Write((uint8_t)220);
	bs.Write((uint8_t)16);
	bs.Write((uint32_t)2);
	Native::WriteCefString(bs, "about:blank");
	Native::MBSendPacket(playerid, &bs, 1, 9);
	bs.Reset();
	return 1;
}
static cell AMX_NATIVE_CALL n_HideYouTubeVideoPhone(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected)) return 0;
	HideYouTube(params[arg_playerid]);
	return 1;
}
void ProcessYouTubeLink(char* text) {
	const char* youtubePrefix = "https://www.youtube.com/watch?v=";
	size_t prefixLength = strlen(youtubePrefix);
	if (strncmp(text, youtubePrefix, prefixLength) == 0) {
		memmove(text, text + prefixLength, strlen(text) - prefixLength + 1);
	}
}
static cell AMX_NATIVE_CALL n_ShowYouTubeVideoPhone(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		arg_url,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected)) return 0;
	HideYouTube(params[arg_playerid]);
	char utf8text[200];
	char text[401];
	cell* url;
	BitStream bs;
	amx_GetAddr(amx, params[arg_url], &url);
	amx_GetString(utf8text, url, 0, sizeof(utf8text));
	ProcessYouTubeLink(utf8text);
	snprintf(text, sizeof(text), "https://bagwell.dev/youtube_play_phone.php?video_id=%s", utf8text);
	bs.Write((uint8_t)220);
	bs.Write((uint8_t)16);
	bs.Write((uint32_t)2);
	Native::WriteCefString(bs, text);
	Native::MBSendPacket(params[arg_playerid], &bs, 1, 9);
	return 1;
}
static cell AMX_NATIVE_CALL n_VehicleToneCustomizer(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		arg_type,
		arg_red,
		arg_green,
		arg_blue,
		arg_size,
		arg_status,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_carid] <= 0 || params[arg_type] < 0 || params[arg_type] > 6 || params[arg_carid] >= MAX_VEHICLES) return 0;
	if (params[arg_red] > 255 || params[arg_red] < 0 || params[arg_green] > 255 || params[arg_green] < 0 || params[arg_blue] > 255 || params[arg_blue] < 0) return 0;
	vTemp[params[arg_carid]].vToner[params[arg_type]] = params[arg_status];
	vTemp[params[arg_carid]].vToner_red[params[arg_type]] = params[arg_red];
	vTemp[params[arg_carid]].vToner_green[params[arg_type]] = params[arg_green];
	vTemp[params[arg_carid]].vToner_blue[params[arg_type]] = params[arg_blue];
	vTemp[params[arg_carid]].vToner_size[params[arg_type]] = params[arg_size];
	Native::b_VehicleToneCustomizer(params[arg_carid], params[arg_type], params[arg_red], params[arg_green], params[arg_blue], params[arg_size]);
	return 1;
}
static cell AMX_NATIVE_CALL n_SetVinylCustomization(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		arg_type,
		arg_color,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_carid] <= 0 || params[arg_color] > 255 || params[arg_color] < 0 || params[arg_carid] >= MAX_VEHICLES) return 0;
	if (params[arg_type] > 17 || params[arg_type] < 0) return 0;
	vTemp[params[arg_carid]].vVinil = params[arg_type];
	vTemp[params[arg_carid]].vColor = params[arg_color];
	vTemp[params[arg_carid]].vColorCustom = true;
	Native::b_SetVinylCustomization(params[arg_carid], params[arg_type], params[arg_color]);
	return 1;
}
static cell AMX_NATIVE_CALL n_GetPlayerLauncherStatus(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected)) return 0;
	return PI[params[arg_playerid]].pLauncherStat;
}
int AddElement(std::vector<AudioInfo>& vec)
{
	int index = 0;
	while (index < vec.size() && vec[index].aType != 0) ++index;
	if (index == vec.size()) vec.emplace_back(AudioInfo{});
	else vec[index] = AudioInfo{};
	return index;
}
int FindElement(std::vector<AudioInfo>& vec, int uid, int type)
{
	for (int i = 0; i < (int)vec.size(); i++) if (uid >= 0 && uid == vec[i].aUid && type == vec[i].aType && type > 0) return i;
	return -1;
}
// строка идёт внутрь JS '...' и JSON "...": убираем кавычки, \ и управляющие символы (иначе CEF-событие ломается)
static void CleanJs(char* s)
{
	char* out = s;
	for (; *s; s++)
		if ((unsigned char)*s >= 0x20 && *s != '\'' && *s != '"' && *s != '\\') *out++ = *s;
	*out = 0;
}
// существующий поток: 0 <= id < размер и поток не удалён
static bool IsAudioId(cell id)
{
	return id >= 0 && id < (cell)aTemp.size() && aTemp[id].aType > 0;
}
static cell AMX_NATIVE_CALL n_Audio_AttachStreamToPlayer(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected)) return -1;
	if (params[arg_playerid] >= MAX_PLAYERS || params[arg_playerid] < 0) return -1;
	if (PI[params[arg_playerid]].pRadio != -1) return PI[params[arg_playerid]].pRadio;
	int index = AddElement(aTemp);
	aTemp[index].aType = 2;
	aTemp[index].aOwner = amx;
	aTemp[index].aUid = params[arg_playerid];
	aTemp[index].aPause = 1;
	aTemp[index].aTime = 0;
	PI[params[arg_playerid]].pRadio = index;
	strcpy(aTemp[index].aURL, "");
	Native::Audio_SendZone(index); // самому игроку и тем, у кого он в прорисовке
	return index;
}
static cell AMX_NATIVE_CALL n_Audio_AttachStreamToVehicle(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected)) return -1;
	int carid = params[arg_carid];
	if (carid >= MAX_VEHICLES || carid < 1 || !Platform::IsVehicleValid(carid)) return -1;
	if (vTemp[carid].vUIDradio != -1) return vTemp[carid].vUIDradio;
	int index = AddElement(aTemp);
	aTemp[index].aType = 3;
	aTemp[index].aOwner = amx;
	aTemp[index].aUid = carid;
	aTemp[index].aPause = 1;
	aTemp[index].aTime = 0;
	vTemp[carid].vUIDradio = index;
	strcpy(aTemp[index].aURL, "");
	Native::Audio_SendZone(index); // всем, у кого машина в прорисовке
	return index;
}
// звук в точке на карте (тип 4) - так у Arizona сделаны колонки и музыка на площадях
// Возвращает ID потока: трек включается как обычно - Audio_UpdateAudioStreamID(ID, ссылка)
static cell AMX_NATIVE_CALL n_Audio_CreateStreamAtPos(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_x,
		arg_y,
		arg_z,
		arg_radius,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected)) return -1;
	const float radius = amx_ctof(params[arg_radius]);
	if (radius <= 0.0f || radius > 1000.0f) return -1;
	int index = AddElement(aTemp);
	aTemp[index].aType = 4;
	aTemp[index].aOwner = amx;
	aTemp[index].aUid = -1;
	aTemp[index].aPause = 1;
	aTemp[index].aTime = 0;
	aTemp[index].aPos[0] = amx_ctof(params[arg_x]);
	aTemp[index].aPos[1] = amx_ctof(params[arg_y]);
	aTemp[index].aPos[2] = amx_ctof(params[arg_z]);
	aTemp[index].aRadius = radius;
	strcpy(aTemp[index].aURL, "");
	Native::Audio_SendZone(index); // зона всем игрокам с лаунчером
	return index;
}
static cell AMX_NATIVE_CALL n_SetPlayerLauncherStatus(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		arg_status,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected)) return 0;
	if (params[arg_status] > 1 || params[arg_status] < 0) return 0;
	if (params[arg_playerid] < 0 || params[arg_playerid] >= MAX_PLAYERS || !Platform::IsPlayerConnected(params[arg_playerid])) return 0;
	if (params[arg_status] && PI[params[arg_playerid]].pLoadRadioCheck == false)
	{
		Voice::Connect(params[arg_playerid]); // как Arizona: 03, 09, канал "Локальный"
		PI[params[arg_playerid]].pLoadRadioCheck = true;
	}
	const bool enabled = params[arg_status] && !PI[params[arg_playerid]].pLauncherStat;
	PI[params[arg_playerid]].pLauncherStat = params[arg_status];
	if (enabled) Native::Audio_SendPointZones(params[arg_playerid]); // колонки уже стоят - показать их
	if (enabled) { ArzExtra::OnLauncherEnabled(params[arg_playerid]); ArzChat::OnLauncherEnabled(params[arg_playerid]); } // грязь объектов, ники, вкладки
	if (enabled) ArzCore::OnLauncherEnabled(params[arg_playerid]); // своя походка
	if (enabled) Events::ResyncStreamed(params[arg_playerid]); // машины и игроки, которые он уже видит
	return true;
}
// включить трек потока uid всем слушателям (если уже играл - перезапуск с новой ссылкой/временем)
// aTime должен быть уже задан (с какой секунды): на время отправки поток считается на паузе
// чтобы Audio_SyncTime не перезаписал его старой позицией
void Audio_Start(int uid)
{
	if (!IsAudioId(uid)) return;
	if (!aTemp[uid].aPause) Native::Audio_SendStop(uid);
	aTemp[uid].aPause = 1;
	Native::Audio_SendPlay(uid);
	aTemp[uid].aPause = 0;
	Native::Audio_SetStarted(uid);
}
static cell AMX_NATIVE_CALL n_Audio_GetVehicleStreamID(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_carid] >= MAX_VEHICLES || params[arg_carid] < 1) return -1;
	return FindElement(aTemp, params[arg_carid], 3);
}
static cell AMX_NATIVE_CALL n_Audio_IsStreamPlaying(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_audioID,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || !IsAudioId(params[arg_audioID])) return 0;
	int uid = params[arg_audioID];
	return aTemp[uid].aPause ? 0 : 1;
}
static cell AMX_NATIVE_CALL n_Audio_GetPlayerStreamID(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_playerid] >= MAX_PLAYERS || params[arg_playerid] < 0) return -1;
	return FindElement(aTemp, params[arg_playerid], 2);
}
static cell AMX_NATIVE_CALL n_Audio_GetStreamTime(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_audioID,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || !IsAudioId(params[arg_audioID])) return 0;
	int uid = params[arg_audioID];
	Native::Audio_SyncTime(uid);
	return aTemp[uid].aTime;
}
static cell AMX_NATIVE_CALL n_Audio_SetStreamTime(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_audioID,
		arg_time,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || !IsAudioId(params[arg_audioID]) || params[arg_time] > 100000 || params[arg_time] < 0) return 0;
	int uid = params[arg_audioID];
	aTemp[params[arg_audioID]].aTime = params[arg_time];
	Audio_Start(uid);
	return 1;
}
static cell AMX_NATIVE_CALL n_Audio_DestroyStream(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_audioID,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || !IsAudioId(params[arg_audioID])) return 0;
	int uid = params[arg_audioID];
	Native::Audio_Free(uid); // зону удаляем у всех слушателей, затем освобождаем поток
	return 1;
}
static cell AMX_NATIVE_CALL n_Audio_StopStream(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_audioID,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || !IsAudioId(params[arg_audioID]) || aTemp[params[arg_audioID]].aPause) return 0;
	int uid = params[arg_audioID];
	Native::Audio_SyncTime(uid); // пауза: время замирает здесь
	aTemp[uid].aPause = 1;
	Native::Audio_SendStop(uid);
	return 1;
}
static cell AMX_NATIVE_CALL n_Audio_PlayStream(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_audioID,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || !IsAudioId(params[arg_audioID]) || !aTemp[params[arg_audioID]].aPause) return 0;
	int uid = params[arg_audioID];
	Audio_Start(uid);
	return 1;
}
// включить трек url с секунды time в потоке uid (Audio_SetStreamUrl, Audio_PlayForPlayer)
static int Audio_SetUrl(int uid, const char* url, int time, bool loop)
{
	if (!IsAudioId(uid) || time < 0 || time > 100000) return 0;
	// мусор вместо ссылки (пробелы, кавычки, Esc/Enter, не http) - не включаем
	if (strlen(url) >= sizeof(AudioInfo::aURL) - 10 || !Media::IsSafeUrl(url)) return 0;
	// YouTube -> mp3 с медиасервера плагина (Media_Init), прямые ссылки как есть
	const std::string playUrl = Media::AudioUrl(url);
	if (playUrl.empty())
	{
		Platform::Log(Platform::LogLevel::Warning, "[ModGear]: Ссылка YouTube в Audio_ работает после Media_Init (медиасервер не запущен или ещё узнаёт IP).");
		return 0;
	}
	AudioInfo& a = aTemp[uid];
	a.aTime = time;
	a.aLoop = loop;
	a.aSource = url;
	a.aDurationMs = Media::DurationMs(url);
	Media::Prefetch(Media::YouTubeId(url));
	snprintf(a.aURL, sizeof(a.aURL), "%s", playUrl.c_str());
	Audio_Start(uid);
	return 1;
}
static cell AMX_NATIVE_CALL n_Audio_UpdateAudioStreamID(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_audioID,
		arg_url,
		arg_time,
		arg_loop,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_time] > 100000 || params[arg_time] < 0 || !IsAudioId(params[arg_audioID])) return 0;
	char url[300];
	cell* url_ptr;
	amx_GetAddr(amx, params[arg_url], &url_ptr);
	amx_GetString(url, url_ptr, 0, sizeof(url));
	return Audio_SetUrl(params[arg_audioID], url, params[arg_time], params[arg_loop] != 0);
}
static cell AMX_NATIVE_CALL n_SetPlayerCollisionStatus(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		arg_status,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || !PI[params[arg_playerid]].pLauncherStat || params[arg_status] > 1 || params[arg_status] < 0) return false;
	PI[params[arg_playerid]].pColision = params[arg_status];
	for (int i : connectedPlayers)
	{
		if (PI[i].pLauncherStat && (Platform::IsPlayerStreamedIn(params[arg_playerid], i) || i == params[arg_playerid])) Native::b_SetPlayerCollisionStatus(params[arg_playerid], i, PI[params[arg_playerid]].pColision);
	}
	return true;
}
static cell AMX_NATIVE_CALL n_UpdateInfoHotKey(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		arg_type,
		arg_text,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_type] > 4 || params[arg_type] < 0) return 0;
	if (params[arg_playerid] < 0 || params[arg_playerid] >= MAX_PLAYERS) return 0;
	const int playerid = (int)params[arg_playerid], type = (int)params[arg_type];
	char keys[10];
	cell* keys_ptr;
	amx_GetAddr(amx, params[arg_text], &keys_ptr);
	amx_GetString(keys, keys_ptr, 0, sizeof(keys));
	CleanJs(keys);
	snprintf(PI[playerid].pHotKey[type], sizeof(PI[playerid].pHotKey[type]), "%s", keys);
	// подсказки уже на экране - обновить сразу
	if (PI[playerid].pHints == 1) Native::SendHotKey(playerid, type);
	return 1;
}
static cell AMX_NATIVE_CALL n_UpdateInfoHudGtaSa(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_ServerNumber,
		arg_ServerName,
		arg_ServerProject,
		arg_ServerType,
		arg_ServerFlag,
		arg_ServerLogo,
		arg_XPayDay,
		arg_playerid,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected)) return 0;
	char number[50], number1[50], number2[50];
	cell* number_ptr;
	amx_GetAddr(amx, params[arg_ServerName], &number_ptr);
	amx_GetString(number, number_ptr, 0, sizeof(number));
	amx_GetAddr(amx, params[arg_ServerProject], &number_ptr);
	amx_GetString(number1, number_ptr, 0, sizeof(number1));
	amx_GetAddr(amx, params[arg_ServerType], &number_ptr);
	amx_GetString(number2, number_ptr, 0, sizeof(number2));
	CleanJs(number);
	CleanJs(number1);
	CleanJs(number2);
	auto updateinfohud = [&](int i)
		{
			if (PI[i].pHud == 4) Native::ShowCef(i, "window.executeEvent('event.hudViceCity.updateDonateMultiplier', '[%d]');", params[arg_XPayDay]);
			// arizonahud - на любом худе, кроме Vice City и мобильного
			// онлайн - настоящий
			else if (PI[i].pHud == 6) Native::ShowCefMobile(i, 8, 0, "%d", params[arg_XPayDay]);
			else Native::ShowCef(i, "window.executeEvent('event.arizonahud.serverInfo', '[{\"id\":%d,\"title\":\"%s\",\"project\":\"%s\",\"type\":\"%s\",\"onLine\":%d,\"flag\":%d,\"logo\":%d,\"multiplier\":%d}]');", params[arg_ServerNumber], number, number1, number2, (int)connectedPlayers.size(), params[arg_ServerFlag], params[arg_ServerLogo], params[arg_XPayDay]);
		};
	if (params[arg_playerid] == -1)
	{
		iServer.iServerFlag = params[arg_ServerFlag];
		iServer.iServerLogo = params[arg_ServerLogo];
		iServer.iXPayDay = params[arg_XPayDay];
		iServer.iServerNumber = params[arg_ServerNumber];
		snprintf(iServer.iServerName, sizeof(iServer.iServerName), number);
		snprintf(iServer.iServerProject, sizeof(iServer.iServerProject), number1);
		snprintf(iServer.iServerType, sizeof(iServer.iServerType), number2);
	}
	if (params[arg_playerid] == -1) for (int i : connectedPlayers) updateinfohud(i);
	else updateinfohud(params[arg_playerid]);
	return 1;
}
static cell AMX_NATIVE_CALL n_InstallNumberVehicle(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		arg_type,
		arg_number,
		arg_region,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_carid] <= 0 || params[arg_type] > 51 || params[arg_carid] >= MAX_VEHICLES) return 0;
	char number[10], region[20];
	cell* number_ptr;
	amx_GetAddr(amx, params[arg_number], &number_ptr);
	amx_GetString(number, number_ptr, 0, sizeof(number));
	amx_GetAddr(amx, params[arg_region], &number_ptr);
	amx_GetString(region, number_ptr, 0, sizeof(region));
	snprintf(vTemp[params[arg_carid]].vNumber, sizeof(vTemp[params[arg_carid]].vNumber), "%.*s", (int)sizeof(vTemp[params[arg_carid]].vNumber) - 1, number);
	snprintf(vTemp[params[arg_carid]].vRegion, sizeof(vTemp[params[arg_carid]].vRegion), "%.*s", (int)sizeof(vTemp[params[arg_carid]].vRegion) - 1, region);
	vTemp[params[arg_carid]].vNumberType = params[arg_type];
	Native::b_InstallNumberVehicle(params[arg_carid], params[arg_type], number, region);
	return 1;
}
static cell AMX_NATIVE_CALL n_CustomizeWheelSettings(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		arg_type,
		arg_flot,
		arg_flot2,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_carid] <= 0 || params[arg_type] < 0 || params[arg_type] > 1 || params[arg_carid] >= MAX_VEHICLES) return 0;
	if (params[arg_type] == 0)
	{
		float flot = amx_ctof(params[arg_flot]);
		float flot2 = amx_ctof(params[arg_flot2]);
		if (flot > 15.0f || flot < -15.0f || flot2 > 0.1f || flot2 < -0.1f) return 0;
		vTemp[params[arg_carid]].vWheel_1[0] = flot;
		vTemp[params[arg_carid]].vWheel_1[1] = flot2;
		Native::b_WheelSettingsCustom(params[arg_carid], params[arg_type], flot, flot2);
	}
	else if (params[arg_type] == 1)
	{
		float flot = amx_ctof(params[arg_flot]);
		float flot2 = amx_ctof(params[arg_flot2]);
		if (flot > 15.0f || flot < -15.0f || flot2 > 0.1f || flot2 < -0.1f) return 0;
		vTemp[params[arg_carid]].vWheel_2[0] = flot;
		vTemp[params[arg_carid]].vWheel_2[1] = flot2;
		Native::b_WheelSettingsCustom(params[arg_carid], params[arg_type], flot, flot2);
	}
	return 1;
}
static cell AMX_NATIVE_CALL n_SmokeColorShift(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		arg_color,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_carid] <= 0 || params[arg_color] < 0 || params[arg_color] > 255 || params[arg_carid] >= MAX_VEHICLES) return 0;
	vTemp[params[arg_carid]].vColorShift = params[arg_color];
	Native::b_SmokeColorShift(params[arg_carid], params[arg_color]);
	return 1;
}
static cell AMX_NATIVE_CALL n_SetColorHeadlamp(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		arg_color,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_carid] <= 0 || params[arg_color] < 0 || params[arg_color] > 255 || params[arg_carid] >= MAX_VEHICLES) return 0;
	vTemp[params[arg_carid]].vHeaderlamp = params[arg_color];
	Native::b_SetColorHeadlamp(params[arg_carid], params[arg_color]);
	return 1;
}
static cell AMX_NATIVE_CALL n_NeonColorChange(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		arg_color,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_carid] <= 0 || params[arg_color] < 0 || params[arg_color] > 255 || params[arg_carid] >= MAX_VEHICLES) return 0;
	vTemp[params[arg_carid]].vNeon = params[arg_color];
	Native::b_NeonColorChange(params[arg_carid], params[arg_color]);
	return 1;
}
static cell AMX_NATIVE_CALL n_SetShotsExhaust(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		arg_status,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_carid] <= 0 || params[arg_status] < 0 || params[arg_status] > 1) return 0;
	vTemp[params[arg_carid]].vOtstrel = params[arg_status];
	Native::b_SetShotsExhaust(params[arg_carid], params[arg_status]);
	return 1;
}
static cell AMX_NATIVE_CALL n_SetVehicleColorWheel(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		arg_color,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_carid] <= 0 || params[arg_color] > 255 || params[arg_color] < 0 || params[arg_carid] >= MAX_VEHICLES) return 0;
	// красить можно только диск со своей текстурой обода (см. wheelTable)
	const WheelInfo* wheel = FindWheel(vTemp[params[arg_carid]].vDiski);
	if (!wheel || !wheel->texture[0]) return 0;
	vTemp[params[arg_carid]].vDiskiColor = params[arg_color];
	Native::b_SetVehicleColorWheel(params[arg_carid], params[arg_color]);
	return 1;
}
std::pair<long long, std::pair<long long, long long>> stringToInt(const std::string& inputString)
{
	size_t length = inputString.length();
	size_t oneThirdLength = length / 3;
	size_t twoThirdsLength = 2 * oneThirdLength;

	std::ostringstream oss;
	long long firstNumber, secondNumber, thirdNumber;

	for (size_t i = 0; i < length; i++)
	{
		oss << static_cast<int>(inputString[i]);
		if (i == oneThirdLength - 1) {
			firstNumber = std::stoll(oss.str());
			oss.str("");
		}
		else if (i == twoThirdsLength - 1) {
			secondNumber = std::stoll(oss.str());
			oss.str("");
		}
	}
	thirdNumber = std::stoll(oss.str());
	return std::make_pair(firstNumber, std::make_pair(secondNumber, thirdNumber));
}
std::string intToString(long long firstNumber, long long secondNumber, long long thirdNumber)
{
	std::ostringstream oss;
	oss << firstNumber << secondNumber << thirdNumber;
	std::string result = oss.str();
	std::string decodedString;
	for (size_t i = 0; i < result.length(); i += 2)
	{
		std::string digit = result.substr(i, 2);
		char decodedChar = static_cast<char>(std::stoi(digit));
		decodedString.push_back(decodedChar);
	}
	return decodedString;
}

static cell AMX_NATIVE_CALL n_ConvertedStringInt(AMX* amx, cell* params)
{
	enum
	{
		arg_size,
		arg_int_1,
		arg_int_2,
		arg_int_3,
		arg_name,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1

	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected)) return 0;
	cell* cptr;
	char name[20];
	amx_GetAddr(amx, params[arg_name], &cptr);
	amx_GetString(name, cptr, 0, 20);
	auto result = stringToInt(name);
	amx_GetAddr(amx, params[arg_int_1], &cptr);
	*cptr = static_cast<int>(result.first);
	amx_GetAddr(amx, params[arg_int_2], &cptr);
	*cptr = static_cast<int>(result.second.first);
	amx_GetAddr(amx, params[arg_int_3], &cptr);
	*cptr = static_cast<int>(result.second.second);
	return 1;
}
static cell AMX_NATIVE_CALL n_ConvertedIntString(AMX* amx, cell* params)
{
	enum
	{
		arg_size,
		arg_int_1,
		arg_int_2,
		arg_int_3,
		arg_name,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected)) return 0;
	cell* cptr;
	amx_GetAddr(amx, params[arg_name], &cptr);
	std::string outputString = intToString(params[arg_int_1], params[arg_int_2], params[arg_int_3]);
	amx_SetString(cptr, outputString.c_str(), 0, 0, strlen(outputString.c_str()) + 1);
	return 1;
}
static cell AMX_NATIVE_CALL n_RemovePlayerAttachedObjectEx(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		arg_index,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_index] < 0 || params[arg_index] > 9) return 0;
	Native::ResetAcs(params[arg_playerid], params[arg_index], -1);
	return 1;
}
static cell AMX_NATIVE_CALL n_IsPlayerAttachedObjectSlotEx(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		arg_index,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_index] < 0 || params[arg_index] > 9) return 0;
	return (PI[params[arg_playerid]].pAcs_bone[params[arg_index]] > 0 && PI[params[arg_playerid]].pAcs_Model[params[arg_index]] != -1 ? 1 : 0);
}
static cell AMX_NATIVE_CALL n_SetPlayerAttachedObjectEx(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		arg_index,
		arg_create,
		arg_modelid,
		arg_bone,
		arg_fOffsetX,
		arg_fOffsetY,
		arg_fOffsetZ,
		arg_fRotX,
		arg_fRotY,
		arg_fRotZ,
		arg_fScaleX,
		arg_fScaleY,
		arg_fScaleZ,
		arg_color_1,
		arg_color_2,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_index] < 0 || params[arg_index] > 9 || params[arg_bone] < 1 || params[arg_bone] > 18) return false;
	int playerid = params[arg_playerid], type = params[arg_index];
	PI[playerid].pAcs_create[type] = params[arg_create];
	PI[playerid].pAcs_bone[type] = params[arg_bone];
	PI[playerid].pAcs_Model[type] = params[arg_modelid];

	PI[playerid].pAcs_OffsetX[type] = amx_ctof(params[arg_fOffsetX]);
	PI[playerid].pAcs_OffsetY[type] = amx_ctof(params[arg_fOffsetY]);
	PI[playerid].pAcs_OffsetZ[type] = amx_ctof(params[arg_fOffsetZ]);

	PI[playerid].pAcs_fRotX[type] = amx_ctof(params[arg_fRotX]);
	PI[playerid].pAcs_fRotY[type] = amx_ctof(params[arg_fRotY]);
	PI[playerid].pAcs_fRotZ[type] = amx_ctof(params[arg_fRotZ]);

	PI[playerid].pAcs_fScaleX[type] = amx_ctof(params[arg_fScaleX]);
	PI[playerid].pAcs_fScaleY[type] = amx_ctof(params[arg_fScaleY]);
	PI[playerid].pAcs_fScaleZ[type] = amx_ctof(params[arg_fScaleZ]);

	PI[playerid].pAcs_color_1[type] = params[arg_color_1];
	PI[playerid].pAcs_color_2[type] = params[arg_color_2];
	Native::AttachPlyerAcs(-1, playerid, type, PI[playerid].pAcs_create[type], PI[playerid].pAcs_bone[type], PI[playerid].pAcs_Model[type], PI[playerid].pAcs_OffsetX[type], PI[playerid].pAcs_OffsetY[type], PI[playerid].pAcs_OffsetZ[type], PI[playerid].pAcs_fRotX[type], PI[playerid].pAcs_fRotY[type], PI[playerid].pAcs_fRotZ[type], PI[playerid].pAcs_fScaleX[type], PI[playerid].pAcs_fScaleY[type], PI[playerid].pAcs_fScaleZ[type], PI[playerid].pAcs_color_1[type], PI[playerid].pAcs_color_2[type]);
	return true;
}
static cell AMX_NATIVE_CALL n_ToggleChatMod(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		arg_type,
		arg_status,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_playerid] < 0 || params[arg_playerid] >= MAX_PLAYERS || params[arg_status] < 0 || params[arg_status] > 1 || params[arg_type] > 7 || params[arg_type] < 0) return 0;
	// _chat.asi: 220:36 [u8 вкладка][str8 команда][u32 цвет RGBA][str8 название][u8 только во вкладке], 220:37 [u8 вкладка]
	// вкладки CHAT_TAB_* - номера 1..8 (свои вкладки CreatePlayerChatTab - любые 1..31)
	const ChatMod& m = chatMod[params[arg_type]];
	ArzChat::Tab& t = ArzChat::tabs[params[arg_playerid]][params[arg_type] + 1];
	t = ArzChat::Tab();
	if (params[arg_status])
	{
		t.used = true;
		t.command = m.code;
		t.name = m.description;
		t.color = (uint32_t(m.color[0]) << 24) | (uint32_t(m.color[1]) << 16) | (uint32_t(m.color[2]) << 8) | 0xFF;
	}
	ArzChat::SendTab((int)params[arg_playerid], (int)params[arg_type] + 1);
	return 1;
}
static cell AMX_NATIVE_CALL n_SetVehicleHidraulics(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_carid,
		arg_type,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_carid] <= 0 || params[arg_type] < 0 || params[arg_type] > 2 || params[arg_carid] >= MAX_VEHICLES) return 0;
	vTemp[params[arg_carid]].vHidravlic = params[arg_type];
	Native::b_SetVehicleHidraulics(params[arg_carid], params[arg_type]);
	return 1;
}
static cell AMX_NATIVE_CALL n_ShowCefMobile(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		arg_arg_1,
		arg_arg_2,
		arg_url,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected)) return 0;
	char utf8text[4096];
	cell* url;
	amx_GetAddr(amx, params[arg_url], &url);
	amx_GetString(utf8text, url, 0, sizeof(utf8text));
	Native::ShowCefMobile(params[arg_playerid], params[arg_arg_1], params[arg_arg_2], "%s", utf8text);
	return 1;
}
static cell AMX_NATIVE_CALL n_ShowSelectCef(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		arg_browzer,
		arg_status,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected)) return 0;
	BitStream bs;
	bs.Write((uint8_t)220);
	bs.Write((uint8_t)(PI[params[arg_playerid]].pMobile ? 62 : 25));
	if (PI[params[arg_playerid]].pMobile) bs.Write((uint8_t)(params[arg_browzer]));
	else bs.Write((uint32_t)0);
	bs.Write((uint8_t)(params[arg_status] ? 128 : 0));
	Native::MBSendPacket(params[arg_playerid], &bs, 1, 9);
	bs.Reset();
	return 1;
}
static cell AMX_NATIVE_CALL n_ShowCef(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		arg_url,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected)) return 0;
	char utf8text[4096];
	cell* url;
	BitStream bs;
	amx_GetAddr(amx, params[arg_url], &url);
	amx_GetString(utf8text, url, 0, sizeof(utf8text));
	Native::ShowCef(params[arg_playerid], "%s", utf8text);
	return 1;
}
static cell AMX_NATIVE_CALL n_EditAttachedObjectEx(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		arg_index,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_index] < 0 || params[arg_index] > 10) return 0;
	BitStream bs;
	bs.Write((uint32_t)(-params[arg_index] - 1));
	Native::MBSendRPC(params[arg_playerid], 116, &bs, 1, 9);
	bs.Reset();
	return 1;
}
static cell AMX_NATIVE_CALL n_ShowPlayerTag(AMX* amx, cell* params)
{
	enum
	{
		args_size,
		arg_playerid,
		arg_name,
		arg_status,
		__dummy_elem_, num_args_expected = __dummy_elem_ - 1
	};
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || params[arg_playerid] >= MAX_PLAYERS || params[arg_playerid] < 0) return 0;
	cell* cptr;
	amx_GetAddr(amx, params[arg_name], &cptr);
	amx_GetString(PI[params[arg_playerid]].pTag, cptr, 0, 80);
	Native::b_ShowPlayerTag(params[arg_playerid], -1, params[arg_status]);
	return 1;
}
// ---- хендлинг (RPC 161 kind 0, как у Arizona)
static void HandlingString(AMX* amx, cell param, char* out, int size)
{
	cell* ptr = nullptr;
	out[0] = 0;
	if (amx_GetAddr(amx, param, &ptr) == AMX_ERR_NONE && ptr) amx_GetString(out, ptr, 0, size);
}
static bool HandlingCar(cell carid) { return carid > 0 && carid < MAX_VEHICLES && Platform::IsVehicleValid((int)carid); }
// GetVehicleHandling(carid, const field[], &Float:value) - то, что выставлено машине через ModGear
static cell AMX_NATIVE_CALL n_GetVehicleHandling(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 3) || params[1] <= 0 || params[1] >= MAX_VEHICLES) return 0;
	char field[48];
	HandlingString(amx, params[2], field, sizeof(field));
	int id = Handling::ArzId(field);
	cell* out = nullptr;
	if (id == -1 || !(vTemp[params[1]].vHandlingMask >> id & 1) || amx_GetAddr(amx, params[3], &out) != AMX_ERR_NONE || !out) return 0;
	float v = vTemp[params[1]].vHandling[id];
	*out = amx_ftoc(v);
	return 1;
}
// SetVehicleHandling(carid, const field[], Float:value, ...) - один или несколько параметров одним пакетом
// SetVehicleHandling(car, "maxspeed", 300.0) или SetVehicleHandling(car, "maxspeed", 300.0, "acceleration", 20.0)
// Поле - имя ("fMaxVelocity", "maxspeed", "suspension") или id ("0x0A"). Возвращает, сколько параметров принято
static cell AMX_NATIVE_CALL n_SetVehicleHandling(AMX* amx, cell* params)
{
	const int args = (int)(params[0] / (cell)sizeof(cell));
	if (args < 3 || (args - 1) % 2 != 0 || !HandlingCar(params[1])) return 0;
	std::vector<std::pair<int, float>> list;
	for (int i = 2; i + 1 <= args; i += 2)
	{
		char field[48];
		HandlingString(amx, params[i], field, sizeof(field));
		// первая пара - обычные аргументы (значение), остальные - переменные (по ссылке)
		float v;
		if (i == 2) v = amx_ctof(params[3]);
		else
		{
			cell* value = nullptr;
			if (amx_GetAddr(amx, params[i + 1], &value) != AMX_ERR_NONE || !value) continue;
			v = amx_ctof(*value);
		}
		const int id = Handling::ArzId(field);
		if (id == -1) continue;
		bool replaced = false;
		for (auto& p : list) if (p.first == id) { p.second = v; replaced = true; }
		if (!replaced) list.push_back({ id, v });
	}
	if (list.empty()) return 0;
	Handling::Set((int)params[1], list);
	return (cell)list.size();
}
// CopyVehicleHandling(fromcarid, tocarid) - скопировать выставленный хендлинг с одной машины на другую
static cell AMX_NATIVE_CALL n_CopyVehicleHandling(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 2) || params[1] <= 0 || params[1] >= MAX_VEHICLES || !HandlingCar(params[2])) return 0;
	auto list = Handling::Stored((int)params[1]);
	if (list.empty()) return 0;
	Handling::Set((int)params[2], list);
	return 1;
}
// ResetVehicleHandling(carid) - забыть выставленный хендлинг (у тех, кто уже видит машину, он останется до перезагрузки машины)
static cell AMX_NATIVE_CALL n_ResetVehicleHandling(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1) || params[1] <= 0 || params[1] >= MAX_VEHICLES) return 0;
	vTemp[params[1]].vHandlingMask = 0;
	return 1;
}

// ---- видео на экране-объекте (как кинотеатр Arizona)
// Video_Create(objectid, modelid, Float:distance) - экран на объекте, вернёт id видео или 0
static cell AMX_NATIVE_CALL n_Video_Create(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 3) || params[1] < 0 || params[1] > 65535 || params[2] <= 0 || params[2] > 65535) return 0;
	Video::Screen s;
	s.used = true;
	s.objectid = (int)params[1];
	s.model = (int)params[2];
	s.dist = amx_ctof(params[3]);
	if (s.dist <= 0.0f || s.dist > 500.0f) return 0;
	s.hasPos = Platform::GetObjectPos(s.objectid, s.x, s.y, s.z);
	s.owner = amx;
	for (size_t i = 0; i < Video::screens.size(); i++)
		if (!Video::screens[i].used) { Video::screens[i] = s; return (cell)i + 1; }
	Video::screens.push_back(s);
	return (cell)Video::screens.size();
}
// Video_Play(videoid, const url[], start) - включить ссылку (YouTube, Twitch или любую страницу)
static cell AMX_NATIVE_CALL n_Video_Play(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 4) || params[3] < 0 || !Video::Get((int)params[1])) return 0;
	char url[512];
	HandlingString(amx, params[2], url, sizeof(url));
	if (!Media::IsSafeUrl(url)) return 0; // мусор вместо ссылки не показываем
	Video::Play((int)params[1], url, (int)params[3], params[4] != 0);
	Video::Update();
	return 1;
}
// Video_Stop(videoid) - остановить у всех
static cell AMX_NATIVE_CALL n_Video_Stop(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1) || !Video::Get((int)params[1])) return 0;
	Video::StopAll((int)params[1]);
	return 1;
}
// Video_GetTime(videoid) - сколько секунд идёт видео, -1 - не играет
static cell AMX_NATIVE_CALL n_Video_GetTime(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1)) return -1;
	Video::Screen* s = Video::Get((int)params[1]);
	return s && s->playing ? Video::Elapsed(*s) : -1;
}
// Video_Destroy(videoid) - остановить и удалить экран
static cell AMX_NATIVE_CALL n_Video_Destroy(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1) || !Video::Get((int)params[1])) return 0;
	Video::StopAll((int)params[1]);
	Video::screens[params[1] - 1] = Video::Screen();
	return 1;
}

// Video_GetDuration(videoid) - длина видео в секундах: -1 - ещё узнаём или это стрим/страница
static cell AMX_NATIVE_CALL n_Video_GetDuration(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1)) return -1;
	Video::Screen* s = Video::Get((int)params[1]);
	return s && s->duration > 0 ? s->duration : -1;
}
// Video_GetTitle(videoid, dest[], size = 128) - title of the playing video; 1 - ready, 0 - not yet
static cell AMX_NATIVE_CALL n_Video_GetTitle(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 3)) return 0;
	Video::Screen* s = Video::Get((int)params[1]);
	const std::string title = s ? Media::Title(s->source) : "";
	cell* dest;
	amx_GetAddr(amx, params[2], &dest);
	amx_SetString(dest, title.c_str(), 0, 0, (int)params[3]);
	return title.empty() ? 0 : 1;
}
// Video_IsPlaying(videoid)
static cell AMX_NATIVE_CALL n_Video_IsPlaying(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1)) return 0;
	Video::Screen* s = Video::Get((int)params[1]);
	return s && s->playing ? 1 : 0;
}
// Video_SetTime(videoid, seconds) - перемотать
static cell AMX_NATIVE_CALL n_Video_SetTime(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 2) || params[2] < 0) return 0;
	Video::Screen* s = Video::Get((int)params[1]);
	if (!s || !s->playing) return 0;
	Video::Seek((int)params[1], (int)params[2]);
	return 1;
}

// Audio_GetDuration(audioID) - длина трека в секундах, -1 - ещё узнаём или неизвестна
static cell AMX_NATIVE_CALL n_Audio_GetDuration(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1) || !IsAudioId(params[1])) return -1;
	AudioInfo& a = aTemp[params[1]];
	if (a.aDurationMs < 0 && !a.aSource.empty()) a.aDurationMs = Media::DurationMs(a.aSource);
	return a.aDurationMs > 0 ? (a.aDurationMs + 999) / 1000 : -1;
}
// Media_GetTitle(const url[], dest[], size = 128) - name/title of a link; 1 - ready, 0 - not yet
static cell AMX_NATIVE_CALL n_Media_GetTitle(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 3)) return 0;
	char url[512];
	HandlingString(amx, params[1], url, sizeof(url));
	const std::string title = Media::Title(url);
	cell* dest;
	amx_GetAddr(amx, params[2], &dest);
	amx_SetString(dest, title.c_str(), 0, 0, (int)params[3]);
	return title.empty() ? 0 : 1;
}
// Audio_GetTitle(audioID, dest[], size = 128) - title of the playing track; 1 - ready, 0 - not yet
static cell AMX_NATIVE_CALL n_Audio_GetTitle(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 3) || !IsAudioId(params[1])) return 0;
	const std::string title = Media::Title(aTemp[params[1]].aSource);
	cell* dest;
	amx_GetAddr(amx, params[2], &dest);
	amx_SetString(dest, title.c_str(), 0, 0, (int)params[3]);
	return title.empty() ? 0 : 1;
}
// ---- фоновая музыка игрока (тип 1): клиент играет её без 3D, слышит только сам игрок
static bool AudioPlayerOk(cell playerid)
{
	return playerid >= 0 && playerid < MAX_PLAYERS && Platform::IsPlayerConnected((int)playerid);
}
static int Audio_CreateBackground(AMX* amx, int playerid)
{
	const int existing = FindElement(aTemp, playerid, 1);
	if (existing != -1) return existing;
	const int index = AddElement(aTemp);
	aTemp[index].aType = 1;
	aTemp[index].aOwner = amx;
	aTemp[index].aUid = playerid;
	aTemp[index].aPause = 1;
	aTemp[index].aTime = 0;
	strcpy(aTemp[index].aURL, "");
	Native::Audio_SendZone(index); // только самому игроку
	return index;
}
// Audio_CreatePlayerBackground(playerid) - audioID фона игрока (уже есть - тот же); трек - Audio_SetStreamUrl
static cell AMX_NATIVE_CALL n_Audio_CreatePlayerBackground(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1) || !AudioPlayerOk(params[1])) return -1;
	return Audio_CreateBackground(amx, (int)params[1]);
}
// Audio_PlayForPlayer(playerid, const url[], playtime = 0, bool:loop = true) - включить фон игроку; audioID или -1
static cell AMX_NATIVE_CALL n_Audio_PlayForPlayer(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 4) || !AudioPlayerOk(params[1])) return -1;
	char url[300];
	HandlingString(amx, params[2], url, sizeof(url));
	if (!Media::IsSafeUrl(url)) return -1; // проверка до создания фона - чтобы мусор не оставлял пустой поток
	const int index = Audio_CreateBackground(amx, (int)params[1]);
	return Audio_SetUrl(index, url, (int)params[3], params[4] != 0) ? index : -1;
}
// Audio_StopForPlayer(playerid) - выключить и удалить фон игрока; 0 - фона не было
static cell AMX_NATIVE_CALL n_Audio_StopForPlayer(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1) || params[1] < 0 || params[1] >= MAX_PLAYERS) return 0;
	const int index = FindElement(aTemp, (int)params[1], 1);
	if (index == -1) return 0;
	Native::Audio_Free(index);
	return 1;
}
// Audio_GetPlayerBackgroundID(playerid) - audioID фона игрока или -1
static cell AMX_NATIVE_CALL n_Audio_GetPlayerBackgroundID(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1) || params[1] < 0 || params[1] >= MAX_PLAYERS) return -1;
	return FindElement(aTemp, (int)params[1], 1);
}
// ---- проверки потока
static cell AMX_NATIVE_CALL n_Audio_IsValidStream(AMX* amx, cell* params)
{
	return CheckNumberOfArguments(amx, params, 1) && IsAudioId(params[1]) ? 1 : 0;
}
// 1 фон игрока, 2 на игроке, 3 машина, 4 точка; 0 - нет такого потока
static cell AMX_NATIVE_CALL n_Audio_GetStreamType(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1) || !IsAudioId(params[1])) return 0;
	return aTemp[params[1]].aType;
}
// игрок (фон / на игроке) или машина; -1 - точка или нет такого потока
static cell AMX_NATIVE_CALL n_Audio_GetStreamOwner(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1) || !IsAudioId(params[1]) || aTemp[params[1]].aType == 4) return -1;
	return aTemp[params[1]].aUid;
}
static cell AMX_NATIVE_CALL n_Audio_IsStreamLooped(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1) || !IsAudioId(params[1])) return 0;
	return aTemp[params[1]].aLoop ? 1 : 0;
}
// Audio_SetStreamLoop(audioID, bool:loop) - повтор; если трек играет - перезапуск с того же места
static cell AMX_NATIVE_CALL n_Audio_SetStreamLoop(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 2) || !IsAudioId(params[1])) return 0;
	const int uid = (int)params[1];
	const bool loop = params[2] != 0;
	if (aTemp[uid].aLoop == loop) return 1;
	aTemp[uid].aLoop = loop;
	// флаг повтора клиент берёт только при запуске трека
	if (!aTemp[uid].aPause && aTemp[uid].aURL[0]) { Native::Audio_SyncTime(uid); Audio_Start(uid); }
	return 1;
}
// Audio_SetStreamRadioEffect(audioID, bool:enable) - звук потока как из рации/телефона (фильтры Arizona)
static cell AMX_NATIVE_CALL n_Audio_SetStreamRadioEffect(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 2) || !IsAudioId(params[1])) return 0;
	const int uid = (int)params[1];
	aTemp[uid].aRadioFx = params[2] != 0;
	Native::Audio_SendEffect(uid);
	return 1;
}
// Media_Init(port, const host[]) - медиасервер: YouTube в Audio_ и длина треков/видео
static cell AMX_NATIVE_CALL n_Media_Init(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 2) || params[1] <= 0 || params[1] > 65535) return 0;
	char host[64];
	HandlingString(amx, params[2], host, sizeof(host));
	const int quality = (params[0] / (cell)sizeof(cell)) >= 3 ? (int)params[3] : 720; // необязательный 3-й арг
	return Media::Start((int)params[1], host, quality) ? 1 : 0;
}

// ---- вебкамера Arizona (AZRTC, свой UDP-порт)
// Webcam_Init(port, const host[] = "") - медиасервер вебкамеры; host - IP, который увидят игроки ("" - определить самому)
static cell AMX_NATIVE_CALL n_Webcam_Init(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 2) || params[1] <= 0 || params[1] > 65535) return 0;
	char host[64];
	HandlingString(amx, params[2], host, sizeof(host));
	return Webcam::Init((int)params[1], host) ? 1 : 0;
}
// Webcam_Start(playerid) - включить показ камеры игрока (ответ - OnPlayerWebcam)
static cell AMX_NATIVE_CALL n_Webcam_Start(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1) || !Webcam::Valid((int)params[1]) || !Platform::IsPlayerConnected((int)params[1])) return 0;
	Webcam::Relink((int)params[1]); // AZRTC не подключён - адрес уйдёт заново, VideoInit - сразу после подключения
	return AzRtc::StartCam((int)params[1]) ? 1 : 0;
}
static cell AMX_NATIVE_CALL n_Webcam_Stop(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1) || !Webcam::Valid((int)params[1])) return 0;
	return AzRtc::StopCam((int)params[1]) ? 1 : 0;
}
// Webcam_GetState(playerid) - 0 выкл, 1 ждём ответа клиента, 2 показ идёт
static cell AMX_NATIVE_CALL n_Webcam_GetState(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1) || !Webcam::Valid((int)params[1])) return 0;
	return (cell)AzRtc::GetCam((int)params[1]);
}
// Webcam_IsLinked(playerid) - 1, если AZRTC игрока подключился к медиасерверу
static cell AMX_NATIVE_CALL n_Webcam_IsLinked(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1) || !Webcam::Valid((int)params[1])) return 0;
	return AzRtc::IsLinked((int)params[1]) ? 1 : 0;
}

// ---- голосовые каналы (RPC 252)
// Voice_Create(const name[] = "Локальный", Float:radius = 20.0) - id канала; radius 0 - общий (рация)
static cell AMX_NATIVE_CALL n_Voice_Create(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 2)) return 0;
	char name[64];
	HandlingString(amx, params[1], name, sizeof(name));
	return Voice::Create(name, amx_ctof(params[2]), amx);
}
static cell AMX_NATIVE_CALL n_Voice_AddPlayer(AMX* amx, cell* params)
{
	return CheckNumberOfArguments(amx, params, 2) && Voice::AddPlayer((int)params[1], (int)params[2]) ? 1 : 0;
}
static cell AMX_NATIVE_CALL n_Voice_RemovePlayer(AMX* amx, cell* params)
{
	return CheckNumberOfArguments(amx, params, 2) && Voice::RemovePlayer((int)params[1], (int)params[2]) ? 1 : 0;
}
static cell AMX_NATIVE_CALL n_Voice_SetRadius(AMX* amx, cell* params)
{
	return CheckNumberOfArguments(amx, params, 2) && Voice::SetRadius((int)params[1], amx_ctof(params[2])) ? 1 : 0;
}
static cell AMX_NATIVE_CALL n_Voice_SetName(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 2)) return 0;
	char name[64];
	HandlingString(amx, params[2], name, sizeof(name));
	return Voice::SetName((int)params[1], name) ? 1 : 0;
}
static cell AMX_NATIVE_CALL n_Voice_Destroy(AMX* amx, cell* params)
{
	return CheckNumberOfArguments(amx, params, 1) && Voice::Destroy((int)params[1]) ? 1 : 0;
}
static cell AMX_NATIVE_CALL n_Voice_IsPlayerInChannel(AMX* amx, cell* params)
{
	return CheckNumberOfArguments(amx, params, 2) && Voice::IsInChannel((int)params[1], (int)params[2]) ? 1 : 0;
}
static cell AMX_NATIVE_CALL n_Voice_IsPlayerInZone(AMX* amx, cell* params)
{
	return CheckNumberOfArguments(amx, params, 2) && Voice::IsInZone((int)params[1], (int)params[2]) ? 1 : 0;
}
static cell AMX_NATIVE_CALL n_Voice_Mute(AMX* amx, cell* params)
{
	return CheckNumberOfArguments(amx, params, 2) && Voice::Mute((int)params[1], params[2] != 0) ? 1 : 0;
}
// Voice_SetFormat(0 | -1) - пересылать голос / нет, Voice_SetEcho(bool) - слышать себя
static cell AMX_NATIVE_CALL n_Voice_SetFormat(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1) || params[1] < -1 || params[1] > 0) return 0;
	Voice::format = (int)params[1];
	return 1;
}
static cell AMX_NATIVE_CALL n_Voice_SetEcho(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1)) return 0;
	Voice::echo = params[1] != 0;
	return 1;
}
static cell AMX_NATIVE_CALL n_Voice_IsMuted(AMX* amx, cell* params)
{
	if (!CheckNumberOfArguments(amx, params, 1) || params[1] < 0 || params[1] >= MAX_PLAYERS) return 0;
	return Voice::muted.test((size_t)params[1]) ? 1 : 0;
}

// ---- функции Arizona, которые включает сервер
static bool ValidCar(cell carid) { return carid > 0 && carid < MAX_VEHICLES; }
static bool ValidPlayer(cell playerid) { return playerid >= 0 && playerid < MAX_PLAYERS; }

static cell AMX_NATIVE_CALL n_SetVehicleStrobeLights(AMX* amx, cell* params)
{
	enum { args_size, arg_carid, arg_mode, arg_speed, arg_beam, __dummy_elem_, num_args_expected = __dummy_elem_ - 1 };
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || !ValidCar(params[arg_carid]) || params[arg_mode] < 0 || params[arg_mode] > 255) return 0;
	VehicleInfo& v = vTemp[params[arg_carid]];
	v.vStrobe = params[arg_mode];
	v.vStrobeSpeed = amx_ctof(params[arg_speed]);
	v.vStrobeBeam = params[arg_beam] != 0;
	Native::b_SetVehicleStrobeLights(params[arg_carid], v.vStrobe, v.vStrobeSpeed, v.vStrobeBeam);
	return 1;
}
static cell AMX_NATIVE_CALL n_SetVehicleBrakeCalipers(AMX* amx, cell* params)
{
	enum { args_size, arg_carid, arg_model, arg_simple, __dummy_elem_, num_args_expected = __dummy_elem_ - 1 };
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || !ValidCar(params[arg_carid]) || params[arg_model] < 0 || params[arg_model] > 0xFFFF) return 0;
	VehicleInfo& v = vTemp[params[arg_carid]];
	v.vCalipers = params[arg_model];
	v.vCalipersSimple = params[arg_simple] != 0;
	Native::b_SetVehicleBrakeCalipers(params[arg_carid], v.vCalipers, v.vCalipersSimple);
	return 1;
}
static cell AMX_NATIVE_CALL n_SetVehicleDrift(AMX* amx, cell* params)
{
	enum { args_size, arg_carid, arg_status, __dummy_elem_, num_args_expected = __dummy_elem_ - 1 };
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || !ValidCar(params[arg_carid])) return 0;
	vTemp[params[arg_carid]].vDrift = params[arg_status] != 0;
	Native::b_SetVehicleDrift(params[arg_carid], vTemp[params[arg_carid]].vDrift);
	return 1;
}
static cell AMX_NATIVE_CALL n_SetVehicleSiren(AMX* amx, cell* params)
{
	enum { args_size, arg_carid, arg_status, __dummy_elem_, num_args_expected = __dummy_elem_ - 1 };
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || !ValidCar(params[arg_carid])) return 0;
	vTemp[params[arg_carid]].vSiren = params[arg_status] != 0;
	Native::b_SetVehicleSiren(params[arg_carid], vTemp[params[arg_carid]].vSiren);
	return 1;
}
static cell AMX_NATIVE_CALL n_SetPlayerSpeedLimit(AMX* amx, cell* params)
{
	enum { args_size, arg_playerid, arg_speed, arg_modelid, __dummy_elem_, num_args_expected = __dummy_elem_ - 1 };
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || !ValidPlayer(params[arg_playerid]) || !PI[params[arg_playerid]].pLauncherStat) return 0;
	if (params[arg_modelid] > 0xFFFF) return 0;
	Native::b_SetPlayerSpeedLimit(params[arg_playerid], amx_ctof(params[arg_speed]), params[arg_modelid]);
	return 1;
}
static cell AMX_NATIVE_CALL n_SetPlayerGreenZone(AMX* amx, cell* params)
{
	enum { args_size, arg_playerid, arg_status, __dummy_elem_, num_args_expected = __dummy_elem_ - 1 };
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || !ValidPlayer(params[arg_playerid]) || !PI[params[arg_playerid]].pLauncherStat) return 0;
	// 220:110: u8 вкл, u8 число разрешённых видов оружия (0 - только кулак). Тот же пакет у
	// SetPlayerWeaponWhitelist: core.asi показывает зелёную зону, vorbisFile ограничивает оружие
	Native::ShowCustom(params[arg_playerid], { 110, params[arg_status] ? 1 : 0, 0 });
	return 1;
}
static cell AMX_NATIVE_CALL n_SetPlayerTargetMode(AMX* amx, cell* params)
{
	enum { args_size, arg_playerid, arg_mode, __dummy_elem_, num_args_expected = __dummy_elem_ - 1 };
	if (!CheckNumberOfArguments(amx, params, num_args_expected) || !ValidPlayer(params[arg_playerid]) || !PI[params[arg_playerid]].pLauncherStat) return 0;
	if (params[arg_mode] < 0 || params[arg_mode] > 2) return 0; // клиент принимает 0-2
	Native::ShowCustom(params[arg_playerid], { 228, params[arg_mode] }); // 220:228 u8
	return 1;
}
