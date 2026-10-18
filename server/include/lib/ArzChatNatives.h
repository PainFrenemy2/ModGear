#pragma once
// Нативы чата Arizona (см. ArzChat.h). Подключается в Plugin.cpp после ArzExtraNatives.h

#include <lib/ArzChat.h>
#include <lib/ArzExtraNatives.h>


// CreatePlayerChatTab(playerid, tabid, const name[], const command[], color = 0xFFFFFFFF, bool:exclusive = false)
// вкладка чата 1..31: текст, набранный в ней, уходит как "/command текст"; exclusive - её сообщения не дублируются в общий чат
static cell AMX_NATIVE_CALL n_CreatePlayerChatTab(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 6 || !ArzNatives::Player(params[1]) || params[2] < 1 || params[2] >= ArzChat::MAX_TABS) return 0;
	ArzChat::Tab& t = ArzChat::tabs[params[1]][params[2]];
	t.used = true;
	t.name = ArzNatives::Str(amx, params[3], 255);
	t.command = ArzNatives::Str(amx, params[4], 255);
	if (!t.command.empty() && t.command[0] == '/') t.command.erase(0, 1);
	t.color = (uint32_t)params[5];
	t.exclusive = params[6] != 0;
	ArzChat::SendTab((int)params[1], (int)params[2]);
	return 1;
}
// RemovePlayerChatTab(playerid, tabid)
static cell AMX_NATIVE_CALL n_RemovePlayerChatTab(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Player(params[1]) || params[2] < 1 || params[2] >= ArzChat::MAX_TABS) return 0;
	ArzChat::tabs[params[1]][params[2]] = ArzChat::Tab();
	ArzChat::SendTab((int)params[1], (int)params[2]);
	return 1;
}
// SendClientMessageToTab(playerid, tabid, color, const message[], bool:only = false) - строка в вкладку чата
// (RPC 93 с двумя байтами в конце); only - не показывать в общем чате; playerid -1 - всем с лаунчером
static cell AMX_NATIVE_CALL n_SendClientMessageToTab(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 5 || !ArzNatives::PlayerOrAll(params[1]) || params[2] < 0 || params[2] >= ArzChat::MAX_TABS) return 0;
	const std::string text = ArzNatives::Str(amx, params[4], 1024);
	auto send = [&](int to)
	{
		if (!ArzExtra::Launcher(to)) return;
		BitStream bs;
		bs.Write((uint32_t)params[3]);
		bs.Write((uint32_t)text.size());
		bs.Write(text.data(), (int)text.size());
		bs.Write((uint8_t)params[2]);
		bs.Write((uint8_t)(params[5] ? 1 : 0));
		Native::MBSendRPC(to, 93, &bs, 1, 9);
	};
	if (params[1] >= 0) send((int)params[1]);
	else for (int i = 0; i < MAX_PLAYERS; i++) send(i);
	return 1;
}

// SetNameTagStyle(bool:arizona) - ники в стиле Arizona (полоски здоровья, значки, таблички) для всех
static cell AMX_NATIVE_CALL n_SetNameTagStyle(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1) return 0;
	ArzChat::customTags = params[1] != 0;
	ArzChat::SendFlag(42, ArzChat::customTags);
	return 1;
}
// SetChatBubbleNoDistance(bool:enable) - облачка над игроками видны на любом расстоянии
static cell AMX_NATIVE_CALL n_SetChatBubbleNoDistance(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1) return 0;
	ArzChat::bubbleNoDistance = params[1] != 0;
	ArzChat::SendFlag(43, ArzChat::bubbleNoDistance);
	return 1;
}
// SetPlayerNameTagTitle(playerid, const text[], color = 0xFFFFFFFF, icon = 0) - строка над ником, "" - убрать
static cell AMX_NATIVE_CALL n_SetPlayerNameTagTitle(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 4 || !ArzNatives::Player(params[1])) return 0;
	ArzChat::NameTag& t = ArzChat::tags[params[1]];
	t.titleText = ArzNatives::Str(amx, params[2], 255);
	t.title = !t.titleText.empty();
	t.titleColor = (uint32_t)params[3];
	t.titleIcon = (uint8_t)params[4];
	ArzChat::SendTitle((int)params[1]);
	return 1;
}
// SetPlayerNameTagStatus(playerid, const text[]) - серый текст под ником (описание), "" - убрать
static cell AMX_NATIVE_CALL n_SetPlayerNameTagStatus(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Player(params[1])) return 0;
	ArzChat::NameTag& t = ArzChat::tags[params[1]];
	t.statusText = ArzNatives::Str(amx, params[2], 1024);
	t.status = !t.statusText.empty();
	ArzChat::SendStatus((int)params[1]);
	return 1;
}
// SetPlayerNameTagPlate(playerid, const text[], bgcolor = 0x000000AA, textcolor = 0xFFFFFFFF) - табличка над ником, "" - убрать
static cell AMX_NATIVE_CALL n_SetPlayerNameTagPlate(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 4 || !ArzNatives::Player(params[1])) return 0;
	ArzChat::NameTag& t = ArzChat::tags[params[1]];
	t.plateText = ArzNatives::Str(amx, params[2], 255);
	t.plate = !t.plateText.empty();
	t.plateBg = (uint32_t)params[3];
	t.plateColor = (uint32_t)params[4];
	ArzChat::SendPlate((int)params[1]);
	return 1;
}
// SetPlayerNameTagParam(playerid, param, value) - параметр ника NAMETAG_* (здоровье, значки...)
static cell AMX_NATIVE_CALL n_SetPlayerNameTagParam(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::Player(params[1]) || params[2] < 0 || params[2] >= ArzChat::TAG_PARAMS) return 0;
	ArzChat::NameTag& t = ArzChat::tags[params[1]];
	t.param[params[2]] = (uint16_t)(params[3] < 0 ? 0 : (params[3] > 0xFFFF ? 0xFFFF : params[3]));
	t.paramSet[params[2]] = true;
	ArzChat::SendParam((int)params[1], (int)params[2]);
	return 1;
}
// SetPlayerNameGradient(playerid, hue1, sat1, hue2, sat2) - переливающийся цвет ника: оттенок 0..359, насыщенность 0..100
static cell AMX_NATIVE_CALL n_SetPlayerNameGradient(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 5 || !ArzNatives::Player(params[1])) return 0;
	ArzChat::NameTag& t = ArzChat::tags[params[1]];
	t.param[15] = ArzChat::Hsv((int)params[2], (int)params[3]);
	t.param[16] = ArzChat::Hsv((int)params[4], (int)params[5]);
	t.paramSet[15] = t.paramSet[16] = true;
	ArzChat::SendParam((int)params[1], 15);
	ArzChat::SendParam((int)params[1], 16);
	return 1;
}
// ResetPlayerNameTag(playerid) - убрать всё, что задано нику
static cell AMX_NATIVE_CALL n_ResetPlayerNameTag(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::Player(params[1])) return 0;
	ArzChat::ResetTag((int)params[1]);
	return 1;
}
// SetPedNameGradient(pedid, hue1, sat1, hue2, sat2) - то же для педа (libPED)
static cell AMX_NATIVE_CALL n_SetPedNameGradient(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 5 || params[1] < 0 || params[1] >= ArzChat::MAX_PEDS) return 0;
	ArzChat::pedGrad[params[1]][0] = ArzChat::Hsv((int)params[2], (int)params[3]);
	ArzChat::pedGrad[params[1]][1] = ArzChat::Hsv((int)params[4], (int)params[5]);
	ArzChat::pedGradSet[params[1]] = true;
	ArzChat::SendPedGradient((int)params[1]);
	return 1;
}
// SetPlayerChatBubbleEx(playerid, const text[], color, Float:distance, expiretime, color2 = 0) - облачко над игроком
// в стиле Arizona (видят все); color2 - второй цвет (двухцветное облачко)
static cell AMX_NATIVE_CALL n_SetPlayerChatBubbleEx(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 6 || !ArzNatives::Player(params[1])) return 0;
	BitStream bs;
	ArzChat::TagBegin(bs, (int)params[1], 11);
	bs.Write((uint32_t)params[3]);
	bs.Write((uint32_t)params[6]);
	bs.Write((uint32_t)(params[5] < 0 ? 0 : params[5]));
	bs.Write(amx_ctof(params[4]));
	ArzChat::Write16(bs, ArzNatives::Str(amx, params[2], 1024));
	ArzExtra::SendTo(-1, bs);
	return 1;
}
// ShowDamageText(playerid, Float:amount, Float:x, Float:y, Float:z, color = 0xFF3333FF) - всплывающее число в точке
// (урон), playerid -1 - всем
static cell AMX_NATIVE_CALL n_ShowDamageText(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 6 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	const uint32_t c = (uint32_t)params[6];
	BitStream bs;
	ArzExtra::Begin(bs, 141);
	bs.Write((uint16_t)0);
	bs.Write(amx_ctof(params[2]));
	bs.Write(amx_ctof(params[3]));
	bs.Write(amx_ctof(params[4]));
	bs.Write(amx_ctof(params[5]));
	bs.Write((uint32_t)0);
	bs.Write((uint8_t)(c >> 24));
	bs.Write((uint8_t)(c >> 16));
	bs.Write((uint8_t)(c >> 8));
	bs.Write((uint8_t)c);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
// SetPlayerChatBadge(playerid, badgeid, const text[], icon = 0) - значок в шапке чата; icon - символ шрифта
// иконок (0xF000..0xF0FF) или 0; text "" - убрать
static cell AMX_NATIVE_CALL n_SetPlayerChatBadge(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 4 || !ArzNatives::Player(params[1]) || params[2] < 0 || params[2] > 255) return 0;
	const std::string text = ArzNatives::Str(amx, params[3], 255);
	BitStream bs;
	ArzExtra::Begin(bs, 172);
	bs.Write((uint8_t)params[2]);
	ArzExtra::WriteStr8(bs, text.c_str());
	if (!text.empty())
	{
		// символ в UTF-8, до 3 байт + 0
		uint8_t icon[4] = {};
		const uint32_t cp = (uint32_t)params[4];
		if (cp >= 0x800 && cp <= 0xFFFF) { icon[0] = uint8_t(0xE0 | (cp >> 12)); icon[1] = uint8_t(0x80 | ((cp >> 6) & 0x3F)); icon[2] = uint8_t(0x80 | (cp & 0x3F)); }
		else if (cp >= 0x80 && cp < 0x800) { icon[0] = uint8_t(0xC0 | (cp >> 6)); icon[1] = uint8_t(0x80 | (cp & 0x3F)); }
		else if (cp > 0 && cp < 0x80) icon[0] = (uint8_t)cp;
		bs.Write((const char*)icon, 4);
	}
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
