#pragma once

// Общие обработчики событий сервера. Вызываются из platform/samp и platform/omp

#include <string>
#include <vector>

class BitStream;

namespace Events
{
	void OnPlayerConnect(int playerid, bool mobile);
	void OnPlayerDisconnect(int playerid);

	// viewerid — кто видит, playerid / vehicleid — кто появился / пропал
	void OnPlayerStreamIn(int viewerid, int playerid);
	void OnPlayerStreamOut(int viewerid, int playerid);
	void OnVehicleStreamIn(int vehicleid, int viewerid);
	void OnVehicleStreamOut(int vehicleid, int viewerid);
	// игроку только что включили лаунчер: дослать всё, что он уже видит (машины, игроки, свои аксессуары)
	// стрим у него случился
	void ResyncStreamed(int playerid);

	// Пакет 220 от клиента. bs стоит сразу после ID пакета
	void OnIncomingCustomPacket(int playerid, BitStream& bs);
	// Голос (сырой пакет 252) от клиента: data[0] = 252
	void OnVoicePacket(int playerid, const unsigned char* data, unsigned int length);
	// RPC от клиента. bs — данные RPC
	void OnIncomingRPC(int playerid, int rpcid, BitStream& bs);
	// RPC серверу -> клиенту: можно изменить bs (на месте) до отправки
	void OnOutgoingRPC(int playerid, int rpcid, BitStream& bs);
	// То же RPC уже отправлено
	void OnOutgoingRPCSent(int playerid, int rpcid, BitStream& bs);
	// Открытый у игрока диалог показать заново с другим текстом (тот же ID, стиль, заголовок, кнопки)
	// false - диалог не открыт
	bool ResendDialogText(int playerid, const std::string& text);

	void OnSecondTimer();
	void OnFastTimer();     // раз в 200 мс (педы)
	void OnTick();          // каждый тик сервера (повторы состояния при стриме)
	// Пакет 221 (libPED) от клиента. bs стоит сразу после ID пакета
	void OnPedPacket(int playerid, BitStream& bs);

	// ---- кастомное оружие Arizona (core.asi, модуль CustomWeapon)
	// Клиент Arizona в синхронизации (200 машина, 207 пешком, 211 пассажир) кладёт в поле оружия базовое
	bool IsSyncPacket(int id);
	// синхронизация от игрока (data[0] = ID пакета): запомнить полный ID его оружия
	void OnIncomingSync(int playerid, const unsigned char* data, unsigned int length);
	// синхронизация игроку viewerid (data[0] = ID, bits - длина в битах): байт, который дописать в конец, или -1
	int OutgoingSyncWeapon(int viewerid, const unsigned char* data, unsigned int bits);
	// полный ID оружия игрока (71+ - кастомное Arizona), 0 - нет / неизвестно
	int GetFullWeapon(int playerid);
	// копия пакета с байтом в конце (bits - длина исходного в битах); в out - bits + 8 бит
	void AppendByte(const unsigned char* data, unsigned int bits, unsigned char value, std::vector<unsigned char>& out);
}
