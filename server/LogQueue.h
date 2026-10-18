#pragma once

// Лог из любого потока. logprintf SA-MP (и логгер open.mp) не рассчитаны на вызовы из нескольких потоков
// сразу: текст мог перемешаться, а сервер - упасть. Основной поток пишет сразу, остальные (медиасервер
// вебка, фоновая проверка доступа) кладут строку в очередь, а основной поток сбрасывает её из тика

#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace LogQueue
{
	struct Entry { int level; std::string text; };

	constexpr size_t MAX_PENDING = 4096; // больше - лишнее отбрасываем (и пишем, сколько), память не растёт

	inline std::thread::id mainThread;    // задаётся один раз при загрузке, до запуска фоновых потоков
	inline std::mutex mx;
	inline std::vector<Entry> pending;
	inline size_t dropped = 0;

	inline void SetMainThread() { mainThread = std::this_thread::get_id(); }

	// до SetMainThread фоновых потоков ещё нет - значит, это основной
	inline bool OnMainThread() { return mainThread == std::thread::id() || std::this_thread::get_id() == mainThread; }

	inline void Push(int level, std::string text)
	{
		std::lock_guard<std::mutex> lock(mx);
		if (pending.size() < MAX_PENDING) pending.push_back({ level, std::move(text) });
		else ++dropped;
	}

	// только из основного потока: write(level, text) - запись в лог сервера
	template <class Write>
	void Flush(Write write)
	{
		std::vector<Entry> batch;
		size_t lost = 0;
		{
			std::lock_guard<std::mutex> lock(mx);
			if (pending.empty() && !dropped) return;
			batch.swap(pending);
			lost = dropped;
			dropped = 0;
		}
		for (const Entry& e : batch) write(e.level, e.text.c_str());
		if (lost) write(1, ("[ModGear]: пропущено сообщений лога: " + std::to_string(lost)).c_str());
	}
}
