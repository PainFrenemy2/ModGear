#pragma once

// Перехват RakNet в samp-server (SA-MP 0.3.7 / 0.3.DL)

class BitStream;

namespace SampHooks
{
	// logprintf — адрес функции логирования сервера (для поиска сигнатуры)
	bool Init(void* logprintf);
	bool SendPacket(int playerid, BitStream* bs);
	bool SendRPC(int playerid, int rpcid, BitStream* bs);
}
