/*
 * ============================================================================
 *  ModGear_Voice  -  голос и вебкамера
 * ----------------------------------------------------------------------------
 *  Голосовые каналы Arizona (локальный голос, рации) и вебкамера AZRTC.
 *
 *  Подключение:   #include <ModGear_Voice>      (сам подключает <ModGear>)
 *  Документация:  https://painfrenemy2.github.io/ModGear/
 * ============================================================================
 */
#if defined _ModGear_Voice_included
	#endinput
#endif
#define _ModGear_Voice_included

#include <ModGear>

// ---- голосовые каналы ----
// radius > 0 - локальный канал: участники слышат друг друга только рядом (в радиусе)
// radius = 0 - общий канал (рация): все участники слышат друг друга на любом расстоянии
// id канала
native Voice_Create(const name[] = "Локальный", Float:radius = 20.0);
native Voice_AddPlayer(channelid, playerid);
native Voice_RemovePlayer(channelid, playerid);
native Voice_SetRadius(channelid, Float:radius);                // 0 - сделать общим (рация)
native Voice_SetName(channelid, const name[]);
native Voice_Destroy(channelid);
native Voice_IsPlayerInChannel(channelid, playerid);            // 1 - игрок в канале
native Voice_IsPlayerInZone(channelid, playerid);               // 1 - в канале и рядом есть кого слышать (общий - всегда)
native Voice_Mute(playerid, bool:toggle);                       // его не слышит никто ни в одном канале
native Voice_IsMuted(playerid);
forward OnPlayerVoice(playerid, channelid, bool:talking);       // нажал / отпустил кнопку голоса в канале
native Voice_SetFormat(format);                                 // пересылка голоса: 0 - вкл (по умолчанию), -1 - выкл; эхо - слышать самого себя
native Voice_SetEcho(bool:toggle);

// ---- Вебкамера Arizona (AZRTC) ----
// Отдельный медиасервер на своём UDP-порту (открой его для игроков). Игрокам с лаунчером (SetPlayerLauncherStatus)
// плагин сам шлёт адрес и одноразовый билет, azrtc.asi подключается -> OnPlayerWebcamLink(playerid, 1).
// Webcam_Start - показ камеры игрока; картинку видят игроки, у которых он в зоне стрима.
#if !defined MODGEAR_WEBCAM_PORT
	#define MODGEAR_WEBCAM_PORT 7781
#endif
// host - IP для клиентов, пусто - как у Media_Init
native Webcam_Init(port = MODGEAR_WEBCAM_PORT, const host[] = "");
native Webcam_Start(playerid);                                  // включить показ (ответ - OnPlayerWebcam)
native Webcam_Stop(playerid);
native Webcam_GetState(playerid);                               // 0 - выкл, 1 - ждём ответ клиента, 2 - показ идёт
native Webcam_IsLinked(playerid);                               // 1 - AZRTC игрока подключён к медиасерверу
forward OnPlayerWebcamLink(playerid, linked);                   // AZRTC подключился (1) / отключился (0)
forward OnPlayerWebcam(playerid, status);                       // 1 - показ идёт, 0 - остановлен, -1 - отклонён (камера выключена в F11 / не открылась)
