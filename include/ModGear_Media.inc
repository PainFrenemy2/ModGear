/*
 * ============================================================================
 *  ModGear_Media  -  медиа
 * ----------------------------------------------------------------------------
 *  Медиасервер, музыка (машина, игрок, колонка), видео на экранах.
 *
 *  Подключение:   #include <ModGear_Media>      (сам подключает <ModGear>)
 *  Документация:  https://htmlpreview.github.io/?https://raw.githubusercontent.com/PainFrenemy2/ModGear/HEAD/docs/index.html
 * ============================================================================
 */
#if defined _ModGear_Media_included
	#endinput
#endif
#define _ModGear_Media_included

#include <ModGear>

// ----------------------------------------------------------------------------
//  МЕДИАСЕРВЕР
// ----------------------------------------------------------------------------
// Нужен для ссылок YouTube в Audio_ (плагин сам делает из них mp3) и для длины треков.
// Рядом с сервером должны лежать yt-dlp и ffmpeg (Windows: yt-dlp.exe, ffmpeg.exe, ffprobe.exe), порт открыт для игроков.
#if !defined MODGEAR_MEDIA_PORT
	#define MODGEAR_MEDIA_PORT 7780
#endif
// host - IP для ссылок, пусто - bind из server.cfg (как у сервера), иначе внешний IP
native Media_Init(port = MODGEAR_MEDIA_PORT, const host[] = "", quality = 720);
native Media_GetTitle(const url[], dest[], size = 128);         // название по ссылке; 1 - готово, 0 - ещё узнаём

// ----------------------------------------------------------------------------
//  МУЗЫКА
// ----------------------------------------------------------------------------
// Возвращают audioID (или -1). Трек включается через Audio_SetStreamUrl.
// Ссылки YouTube тоже можно (после Media_Init). Конец трека - OnAudioStreamFinished(audioID).
native Audio_AttachStreamToVehicle(vehicleid);
native Audio_AttachStreamToPlayer(playerid);
native Audio_CreateStreamAtPos(Float:x, Float:y, Float:z, Float:radius = 50.0); // звук в точке (колонка)
native Audio_SetStreamUrl(audioID, const url[], playtime = 0, bool:loop = true); // mp3 или YouTube; loop - по кругу
native Audio_GetVehicleStreamID(vehicleid);                     // audioID машины или -1
native Audio_GetPlayerStreamID(playerid);                       // audioID игрока или -1
native Audio_GetStreamTime(audioID);                            // секунд с начала трека
native Audio_SetStreamTime(audioID, time);
native Audio_GetDuration(audioID);                              // длина трека в секундах, -1 - ещё неизвестна
native Audio_GetTitle(audioID, dest[], size = 128);             // название трека; 1 - готово, 0 - ещё узнаём
native Audio_IsStreamPlaying(audioID);                          // 1 - играет
native Audio_PlayStream(audioID);                               // продолжить
native Audio_StopStream(audioID);                               // пауза
native Audio_DestroyStream(audioID);
forward OnAudioStreamFinished(audioID);

// фоновая музыка только для игрока: без 3D (не из машины и не от персонажа), другие игроки её не слышат.
// Удаляется сама, когда игрок выходит. Остальные Audio_ (время, пауза, длина, название) работают и с ней.
native Audio_PlayForPlayer(playerid, const url[], playtime = 0, bool:loop = true); // включить фон; audioID или -1
native Audio_StopForPlayer(playerid);                           // выключить и удалить фон; 0 - фона не было
native Audio_CreatePlayerBackground(playerid);                  // audioID фона (трек - Audio_SetStreamUrl) или -1
native Audio_GetPlayerBackgroundID(playerid);                   // audioID фона игрока или -1

// проверки потока
#define AUDIO_TYPE_NONE       (0)
#define AUDIO_TYPE_BACKGROUND (1)                               // фон игрока (Audio_PlayForPlayer)
#define AUDIO_TYPE_PLAYER     (2)                               // на игроке (Audio_AttachStreamToPlayer)
#define AUDIO_TYPE_VEHICLE    (3)                               // в машине
#define AUDIO_TYPE_POINT      (4)                               // в точке (колонка)
native Audio_IsValidStream(audioID);                            // 1 - поток есть
native Audio_GetStreamType(audioID);                            // AUDIO_TYPE_*
native Audio_GetStreamOwner(audioID);                           // игрок или машина; -1 - точка или нет потока
native Audio_IsStreamLooped(audioID);                           // 1 - по кругу
native Audio_SetStreamRadioEffect(audioID, bool:enable);      // звук как из рации/телефона
native Audio_SetStreamLoop(audioID, bool:loop);                 // по кругу / до конца (играющий трек продолжит с того же места)

// ----------------------------------------------------------------------------
//  ВИДЕО НА ЭКРАНЕ (КИНОТЕАТР)
// ----------------------------------------------------------------------------
// экран на объекте (CreateObject); id видео или 0
native Video_Create(objectid, modelid = 7914, Float:distance = 40.0);
native Video_Play(videoid, const url[], start = 0, bool:loop = true); // ссылка YouTube/Twitch/страница; start - с какой секунды
native Video_Stop(videoid);
native Video_IsPlaying(videoid);                                // 1 - играет
native Video_GetTime(videoid);                                  // сколько секунд идёт, -1 - не играет
native Video_SetTime(videoid, seconds);                         // перемотать
native Video_GetDuration(videoid);                              // длина в секундах, -1 - неизвестна (стрим) или ещё узнаём
native Video_GetTitle(videoid, dest[], size = 128);             // название; 1 - готово, 0 - ещё узнаём
native Video_Destroy(videoid);
forward OnVideoFinished(videoid);                               // видео дошло до конца
