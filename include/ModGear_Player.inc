/*
 * ============================================================================
 *  ModGear_Player  -  игрок
 * ----------------------------------------------------------------------------
 *  Внешний вид и ник Arizona, аксессуары, оружие и стрельба,
 *  режимы игрока, камера.
 *
 *  Подключение:   #include <ModGear_Player>      (сам подключает <ModGear>)
 *  Документация:  https://htmlpreview.github.io/?https://raw.githubusercontent.com/PainFrenemy2/ModGear/HEAD/docs/index.html
 * ============================================================================
 */
#if defined _ModGear_Player_included
	#endinput
#endif
#define _ModGear_Player_included

#include <ModGear>

// ----------------------------------------------------------------------------
//  ВНЕШНИЙ ВИД
// ----------------------------------------------------------------------------
native SetPlayerCollision(playerid, bool:enable);               // столкновения с игроками вкл/выкл
native SetPlayerNameTagIcon(playerid, const icon[], bool:show); // значок над ником (имя из /models/icons.txd)
native SetPlayerBoneScale(playerid, bone, Float:scale);         // увеличить кость (> 1.0), bone -1 - сброс всех
native SetPlayerWalkStyle(playerid, const style[]);             // походка: группа анимаций GTA ("man", "fatman", "sexywoman", "oldman", "gang1", "drunkman"...), "" - как у скина; видят все
native GetPlayerWalkStyle(playerid, style[], size = sizeof style);
native SetPlayerSkinTexture(playerid, const texture[], color = 0, const newtexture[] = "", txd = 0); // перекрасить/заменить текстуру скина (имя текстуры в модели скина), color 0xRRGGBBAA, видят все
native ResetPlayerSkinTextures(playerid);                       // вернуть скину его текстуры
native RemovePlayerSkinTexture(playerid, const texture[]);      // вернуть одной текстуре скина оригинал

// ----------------------------------------------------------------------------
//  НИК В СТИЛЕ ARIZONA (ВИДЯТ ВСЕ ИГРОКИ С ЛАУНЧЕРОМ)
// ----------------------------------------------------------------------------
// для всех: ники Arizona (полоски, значки, таблички) или обычные SA-MP
native SetNameTagStyle(bool:arizona);
native SetPlayerNameTagTitle(playerid, const text[], color = 0xFFFFFFFF, icon = 0); // строка над ником (организация, звание), "" - убрать
native SetPlayerNameTagStatus(playerid, const text[]);          // серый текст под ником (описание), "" - убрать
native SetPlayerNameTagPlate(playerid, const text[], bgcolor = 0x000000AA, textcolor = 0xFFFFFFFF); // табличка над ником, "" - убрать
native SetPlayerNameGradient(playerid, hue1, sat1, hue2, sat2); // переливающийся ник: оттенок 0..359, насыщенность 0..100
native SetPlayerNameTagParam(playerid, param, value);           // NAMETAG_*: здоровье на полоске, значки (1 - показать, 0 - убрать)
native ResetPlayerNameTag(playerid);                            // убрать всё заданное нику
#define NAMETAG_HEALTH                 (0)
#define NAMETAG_MAX_HEALTH             (1)
#define NAMETAG_ARMOUR                 (2)
#define NAMETAG_MAX_ARMOUR             (3)
#define NAMETAG_ICON_EYE               (4)                      // перечёркнутый глаз
#define NAMETAG_ICON_SWORDS            (5)                      // скрещённые клинки
#define NAMETAG_ICON_3                 (6)
#define NAMETAG_ICON_SPIRAL            (8)                      // спираль
#define NAMETAG_TYPING                 (14)                     // точки "печатает" (ставится и само)
#define NAMETAG_ICON_PILL              (18)                     // шприц / таблетка
native SetPlayerChatBubbleEx(playerid, const text[], color, Float:distance, expiretime, color2 = 0); // облачко Arizona над игроком
native SetChatBubbleNoDistance(bool:enable);                    // для всех: облачка видны на любом расстоянии
native ShowDamageText(playerid, Float:amount, Float:x, Float:y, Float:z, color = 0xFF3333FF); // всплывающее число (урон) в точке, playerid -1 - всем

// ----------------------------------------------------------------------------
//  АКСЕССУАРЫ (СЛОТЫ 0-9, ВИДЯТ ВСЕ ИГРОКИ С ЛАУНЧЕРОМ РЯДОМ)
// ----------------------------------------------------------------------------
native SetPlayerAttachedObjectEx(playerid, index, bool:create, modelid, bone, Float:fOffsetX = 0.0, Float:fOffsetY = 0.0, Float:fOffsetZ = 0.0, Float:fRotX = 0.0, Float:fRotY = 0.0, Float:fRotZ = 0.0, Float:fScaleX = 1.0, Float:fScaleY = 1.0, Float:fScaleZ = 1.0, materialcolor1 = 0, materialcolor2 = 0);
native RemovePlayerAttachedObjectEx(playerid, index);
native IsPlayerAttachedObjectSlotEx(playerid, index);           // 1 - слот занят
native EditAttachedObjectEx(playerid, index);                   // редактор аксессуара
native SetPlayerAttachedObjectDirt(playerid, index, level, texture = 0); // грязь 0..15, texture 0 грязь / 1 потёртости
native SetPlayerAttachedObjectMaterial(playerid, index, materialindex, const txd[], const texture[], color = 0);
native SetPlayerAttachedObjectMatText(playerid, index, materialindex, const text[], matsize = 90, fontsize = 24, align = 1, bool:bold = false, const font[] = "Arial", fontcolor = 0xFFFFFFFF, backcolor = 0);

// ----------------------------------------------------------------------------
//  ОРУЖИЕ И СТРЕЛЬБА
// ----------------------------------------------------------------------------
// трассеры пуль игрока (видят все), 0xAARRGGBB
native SetPlayerBulletTracer(playerid, color, Float:time, bool:laser = false);
native SetPlayerWeaponTracer(playerid, weaponid, color, Float:time, bool:laser = false); // то же для одного оружия
native ResetPlayerBulletTracer(playerid);
native SetPlayerWeaponWhitelist(playerid, bool:enable, const weapons[] = {0}, count = 0); // переключение только на оружие из списка (значок зелёной зоны тоже включается)
native SetPlayerTargetMode(playerid, mode);                     // режим отображения цели 0-2
#if !defined _INC_open_mp // в open.mp свой RemovePlayerWeapon (omp_player.inc)
native RemovePlayerWeapon(playerid, weaponid);                  // забрать одно оружие (остальные остаются)
#endif

// ----------------------------------------------------------------------------
//  РЕЖИМЫ
// ----------------------------------------------------------------------------
native SetPlayerGreenZone(playerid, bool:enable);               // зелёная зона Arizona
native SetPlayerSpeedLimit(playerid, Float:speed, modelid = 0); // ограничитель скорости: speed <= 0 - снять; modelid 0 - все машины
native SetPlayerAntiPause(playerid, bool:enable);               // игра не на паузе при сворачивании
native SetPlayerAirBrake(playerid, bool:enable, Float:speed = 100.0); // полёт (админ)
native SetPlayerFlightLimits(playerid, bool:enable);            // потолок высоты для самолётов, вертолётов и джетпака (false - без потолка)
native SetPlayerBikeFall(playerid, bool:enable);                // false - игрок не слетает с мотоцикла/велосипеда при ударе

// ----------------------------------------------------------------------------
//  КАМЕРА И УПРАВЛЕНИЕ
// ----------------------------------------------------------------------------
native SetPlayerFirstPerson(playerid, bool:enable);             // вид от первого лица
native FadePlayerScreen(playerid, bool:toblack, time = 1000);   // плавно затемнить экран (true) или проявить из чёрного (false) за time мс
native TeleportPlayerWithFade(playerid, Float:x, Float:y, Float:z, Float:angle = -1.0, interior = -1, fade = 1000); // телепорт с проявлением экрана; angle/interior < 0 - не менять (интерьер SA-MP - SetPlayerInterior)
native RedirectPlayer(playerid, const host[], port, const nickname[] = "", const password[] = "", bool:vicecity = false); // переподключить на другой сервер ("" - тот же ник); vicecity - режим Arizona Vice City
native SetPlayerCanSwim(playerid, bool:enable);                 // в режиме Vice City игрок тонет сразу; true - может плавать
native SetPlayerLightMode(playerid, mode);                      // свет от игрока: 0 выкл, 1 пятно у игрока, 2 пятно впереди; видят все
native GetPlayerLightMode(playerid);
native GetPlayerSkinTextureCount(playerid);                     // сколько текстур скина задано
native SetPlayerHeadMove(playerid, bool:enable);                // голова поворачивается за камерой (/headmove)
native SetPlayerCJAnims(playerid, bool:enable);                 // свой персонаж ходит анимациями CJ (как UsePlayerPedAnims, только у себя)
native ShowPlayerLoadScreen(playerid, time, background = 0);    // экран загрузки Arizona на time мс (0 - скрыть), фон 0-2
native SetPlayerFlyingCar(playerid, bool:enable);               // машина летает (управление в воздухе как у самолёта)
native SetPlayerLaunchControl(playerid, bool:enable);           // launch control: на месте W+S, отпустить S - старт
native SetPlayerRadarRoute(playerid, bool:enable);              // маршрут по дорогам на радаре до гоночного чекпоинта
native SetPlayerScreenNoise(playerid, bool:enable);             // помехи на экране, как в камере дрона
native SetPlayerDriveOnWater(playerid, bool:enable);            // машины ездят по воде; playerid -1 - всем
native CreatePlayerWorldIcon(playerid, iconid, const texture[], Float:x, Float:y, Float:z, Float:distance = 100.0, const label[] = ""); // 3D-иконка в мире (marker.txd)
native DestroyPlayerWorldIcon(playerid, iconid);
native DestroyAllPlayerWorldIcons(playerid);
native ResetPlayerScreenFade(playerid);                       // сразу убрать затемнение FadePlayerScreen
native SetPlayerAnimSpeed(playerid, const anims[], Float:speed, count = sizeof anims); // скорость анимаций GTA у игрока, count 0 - сброс
native SetPlayerTargetMarked(playerid, targetid, bool:marked); // отметить игрока как цель (режим SetPlayerTargetMode)
native ShowPlayerTestDriveHud(playerid, value1, value2, value3); // HUD тест-драйва
native HidePlayerTestDriveHud(playerid);
native TogglePlayerScoreboard(playerid, bool:enable);        // таблица игроков по TAB вкл/выкл
native SetPlayerCameraFollowVehicle(playerid, bool:enable);  // камера наклоняется вместе с машиной
native GivePlayerCustomWeapon(playerid, weaponid, ammo);     // выдать оружие с любым ID (71+ - кастомное Arizona)
native GetPlayerCustomWeapon(playerid);                       // полный ID оружия в руках (71+ - кастомное)
native SetPlayerWeaponClip(playerid, weaponid, clip);          // патронов в обойме у оружия; playerid -1 - всем
native SetPlayerWeaponAccuracy(playerid, weaponid, Float:accuracy); // точность оружия (GTA ~0.5..1.0)
native ResetPlayerWeaponStats(playerid, weaponid);             // обойма и точность как в игре
native SetPlayerRadarIconsStyle(playerid, bool:arizona);       // иконки радара Arizona или как в GTA
native SetPlayerInfiniteStamina(playerid, bool:enable);         // не устаёт при беге и плавании; playerid -1 - всем
native SetPlayerBreath(playerid, Float:value);                  // запас воздуха под водой (у GTA до ~1150)
native SetPlayerBikeCruise(playerid, bool:enable);              // круиз-контроль на мотоцикле (W на ходу)
native SetPlayerFirstPersonAim(playerid, bool:enable);          // прицел от первого лица с любым оружием -> OnPlayerFirstPersonAim
forward OnPlayerFirstPersonAim(playerid, bool:aiming);

#define JETPACK_HIDE_ALL               (0)
#define JETPACK_SHOW_ALL               (1)
#define JETPACK_HIDE_OWN               (2)
native SetPlayerJetpackVisibility(playerid, mode);              // JETPACK_*
