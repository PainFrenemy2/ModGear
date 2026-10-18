#pragma once
// Сжатие строк RakNet (StringCompressor::EncodeString): дерево Хаффмана по englishCharacterFrequencies
// Формат: WriteCompressed(u16 число бит), затем биты (добиты до целого байта кодом длиннее остатка)
// Так Arizona шлёт строки в 220:12 (видео) и сжатые CEF-события
#include <raknet/bitstream.h>
#include <string>
#include <vector>
#include <list>
#include <memory>

namespace StrCompress
{
	inline const unsigned frequencies[256] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 722, 0, 0, 2, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		11084, 58, 63, 1, 0, 31, 0, 317, 64, 64, 44, 0, 695, 62, 980, 266,
		69, 67, 56, 7, 73, 3, 14, 2, 69, 1, 167, 9, 1, 2, 25, 94,
		0, 195, 139, 34, 96, 48, 103, 56, 125, 653, 21, 5, 23, 64, 85, 44,
		34, 7, 92, 76, 147, 12, 14, 57, 15, 39, 15, 1, 1, 1, 2, 3,
		0, 3611, 845, 1077, 1884, 5870, 841, 1057, 2501, 3212, 164, 531, 2019, 1330, 3056, 4037,
		848, 47, 2586, 2919, 4771, 1707, 535, 1106, 152, 1243, 100, 0, 2, 0, 10, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	struct Node { unsigned weight; Node* left; Node* right; Node* parent; };

	// коды символов: биты от корня к листу
	inline const std::vector<std::vector<bool>>& Codes()
	{
		static std::vector<std::vector<bool>> codes = []
		{
			std::vector<std::unique_ptr<Node>> pool;
			Node* leaf[256];
			std::list<Node*> list;
			auto insert = [&](Node* n)
			{
				for (auto it = list.begin(); it != list.end(); ++it)
					if ((*it)->weight >= n->weight) { list.insert(it, n); return; }
				list.push_back(n);
			};
			for (int i = 0; i < 256; i++)
			{
				pool.push_back(std::make_unique<Node>(Node{ frequencies[i] ? frequencies[i] : 1, nullptr, nullptr, nullptr }));
				leaf[i] = pool.back().get();
				insert(leaf[i]);
			}
			Node* root = nullptr;
			while (true)
			{
				Node* lesser = list.front(); list.pop_front();
				Node* greater = list.front(); list.pop_front();
				pool.push_back(std::make_unique<Node>(Node{ lesser->weight + greater->weight, lesser, greater, nullptr }));
				Node* n = pool.back().get();
				lesser->parent = greater->parent = n;
				if (list.empty()) { root = n; break; }
				insert(n);
			}
			std::vector<std::vector<bool>> out(256);
			for (int i = 0; i < 256; i++)
			{
				std::vector<bool> path;
				for (Node* n = leaf[i]; n != root; n = n->parent) path.push_back(n->parent->left != n);
				out[i].assign(path.rbegin(), path.rend());
			}
			return out;
		}();
		return codes;
	}

	inline void Write(BitStream& bs, const std::string& text)
	{
		const auto& codes = Codes();
		std::vector<bool> bits;
		for (unsigned char c : text) bits.insert(bits.end(), codes[c].begin(), codes[c].end());
		if (bits.size() % 8)
		{
			size_t remaining = 8 - bits.size() % 8;
			for (int i = 0; i < 256; i++)
				if (codes[i].size() > remaining) { bits.insert(bits.end(), codes[i].begin(), codes[i].begin() + remaining); break; }
		}
		bs.WriteCompressed((unsigned short)bits.size());
		for (bool b : bits) bs.Write(b);
	}
}
