#pragma once
// Ответы интерфейса (CEF) серверу: одно общее событие с уже разобранными параметрами
// Клиент присылает 220:18 со строкой (cef.SendMessage / события интерфейса Arizona). Строку разбираем

#include <nlohmann/json.hpp>
#include "../../Plugin.h"
#include <string>
#include <vector>
#include <utility>

namespace CefResponse
{
	struct Args
	{
		std::string event, raw;
		std::vector<std::pair<std::string, std::string>> items; // имя (у объекта JSON) и значение
	};

	inline Args current;
	inline bool active = false;

	// UTF-8 -> cp1251 (кириллица); строка не в UTF-8 возвращается как есть
	inline std::string ToCp1251(const std::string& s)
	{
		std::string out;
		out.reserve(s.size());
		for (size_t i = 0; i < s.size();)
		{
			const unsigned char c = (unsigned char)s[i];
			if (c < 0x80) { out += (char)c; i++; continue; }
			if ((c & 0xE0) == 0xC0 && i + 1 < s.size() && ((unsigned char)s[i + 1] & 0xC0) == 0x80)
			{
				const unsigned cp = ((c & 0x1F) << 6) | ((unsigned char)s[i + 1] & 0x3F);
				if (cp >= 0x410 && cp <= 0x44F) out += (char)(cp - 0x410 + 0xC0);
				else if (cp == 0x401) out += (char)0xA8;
				else if (cp == 0x451) out += (char)0xB8;
				else if (cp == 0x2116) out += (char)0xB9;
				else out += '?';
				i += 2;
				continue;
			}
			if ((c & 0xF0) == 0xE0 && i + 2 < s.size())
			{
				const unsigned cp = ((c & 0x0F) << 12) | (((unsigned char)s[i + 1] & 0x3F) << 6) | ((unsigned char)s[i + 2] & 0x3F);
				out += cp == 0x2116 ? (char)0xB9 : cp == 0x2014 ? (char)0x97 : cp == 0x2013 ? (char)0x96 : '?';
				i += 3;
				continue;
			}
			if ((c & 0xF8) == 0xF0 && i + 3 < s.size()) { out += '?'; i += 4; continue; }
			return s; // не UTF-8 (уже cp1251)
		}
		return out;
	}

	inline std::string JsonValue(const nlohmann::json& v)
	{
		if (v.is_string()) return v.get<std::string>();
		if (v.is_boolean()) return v.get<bool>() ? "1" : "0";
		if (v.is_null()) return "";
		return v.dump(); // числа как есть, вложенные массивы/объекты - текстом JSON
	}

	inline Args Parse(const std::string& text)
	{
		Args a;
		a.raw = text;
		size_t split = text.find('|');
		size_t payload = std::string::npos;
		if (split != std::string::npos) payload = split + 1;
		else
		{
			split = text.find_first_of("[{");
			payload = split;
		}
		a.event = text.substr(0, split);
		while (!a.event.empty() && (a.event.back() == ' ' || a.event.back() == ',')) a.event.pop_back();
		if (payload == std::string::npos || payload >= text.size()) return a;
		const std::string rest = text.substr(payload);
		const size_t first = rest.find_first_not_of(" \t");
		if (first != std::string::npos && (rest[first] == '[' || rest[first] == '{'))
		{
			nlohmann::json j = nlohmann::json::parse(rest, nullptr, false);
			if (!j.is_discarded())
			{
				if (j.is_array()) for (const auto& v : j) a.items.emplace_back("", JsonValue(v));
				else if (j.is_object()) for (auto it = j.begin(); it != j.end(); ++it) a.items.emplace_back(it.key(), JsonValue(it.value()));
				return a;
			}
		}
		// "a|b|c"
		size_t pos = 0;
		while (true)
		{
			const size_t bar = rest.find('|', pos);
			a.items.emplace_back("", rest.substr(pos, bar == std::string::npos ? std::string::npos : bar - pos));
			if (bar == std::string::npos) break;
			pos = bar + 1;
		}
		return a;
	}

	// из Events: 220:18 от игрока
	inline void Dispatch(int playerid, const std::string& text)
	{
		Args a = Parse(text);
		a.event = ToCp1251(a.event);
		for (auto& item : a.items) item.second = ToCp1251(item.second);
		// вложенный вызов (паблик сам что-то шлёт и т.п.) не должен затереть параметры внешнего
		Args saved = std::move(current);
		const bool wasActive = active;
		current = std::move(a);
		active = true;
		Plugin::CallPublicStr("OnPlayerCefResponse", playerid, current.event.c_str(), (int)current.items.size());
		current = std::move(saved);
		active = wasActive;
	}

	inline const std::string* Arg(cell index)
	{
		if (!active || index < 0 || index >= (cell)current.items.size()) return nullptr;
		return &current.items[index].second;
	}

	inline const std::string* Named(const std::string& key)
	{
		if (!active) return nullptr;
		for (const auto& item : current.items) if (item.first == key) return &item.second;
		return nullptr;
	}
}
