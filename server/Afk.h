#pragma once

// Игрок свернул игру (AFK): клиент в это время не шлёт вообще ничего, а пришедшие ему пакеты
// Arizona (CEF, худ, голос...) применяет криво или теряет. Пакеты плагина такому игроку копим

class BitStream;

namespace Afk
{
	void Joined(int playerid);   // подключился: ждём первой активности
	void Left(int playerid);     // вышел: очередь выбросить
	void Activity(int playerid); // игрок точно играет (см. IsActivityPacket / IsActivityRPC)
	// Свёрнутая игра сразу после входа ещё шлёт вход, класс, спавн, войс и события CEF - это не
	// активность. Активность - синхронизация (её SA-MP не шлёт, пока окно свёрнуто) и RPC от действий
	bool IsActivityPacket(int packetid);
	bool IsActivityRPC(int rpcid);
	bool Away(int playerid);     // свернул игру (или ещё не развернул после входа)

	// Отправить пакет (rpc = false) или RPC игроку (-1 - всем): отошедшим - в очередь
	bool Send(int playerid, bool rpc, int rpcid, BitStream* bs);
	void Flush();                // каждый тик: вернувшимся - всё накопленное
}
