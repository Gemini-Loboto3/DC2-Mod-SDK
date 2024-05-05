#pragma once

#include <stdafx.h>
#include <string>
#include <vector>
#include <algorithm>
#include "xxhash.h"

class X64Hash
{
public:
	class Entry
	{
	public:
		Entry(XXH64_hash_t _h, std::string _n) :
			h(_h), n(_n) {}

		bool operator==(const Entry& h2) const
		{
			if (h == h2.h)
				return true;

			return false;
		}

		XXH64_hash_t h;
		std::string n;
	};

	int PushHash(const void* data, size_t size, LPCSTR name)
	{
		XXH64_hash_t hash = XXH64(data, size, 0);
		Entry h = { hash, name };

		auto f = std::find(hashes.begin(), hashes.end(), h);
		if (f != hashes.end())
			return f - hashes.begin();
		//for (size_t i = 0, si = hashes.size(); i < si; i++)
		//	if (hash == hashes[i])
		//		return;

		hashes.push_back({ hash, name });
		SortByHash();
		return -1;
	}

	//void Write(LPCSTR out_name)
	//{
	//	XMLDocument xml;
	//	xml.SetBOM(true);

	//	auto r = xml.NewElement("Hashes");
	//	for (auto it = hashes.begin(); it != hashes.end(); it++)
	//	{
	//		char str[32];
	//		auto e = xml.NewElement("Hash");
	//		sprintf_s(str, 32, "%016llX", it->h);
	//		e->SetAttribute("h", str);
	//		e->SetAttribute("n", it->n.c_str());

	//		for (int i = 0; i < 3; i++)
	//		{
	//			auto el = xml.NewElement("Alias");
	//			el->SetText(i);
	//			e->InsertEndChild(el);
	//		}

	//		r->InsertEndChild(e);
	//	}
	//	xml.InsertFirstChild(r);
	//	xml.SaveFile(out_name);
	//}

	void SortByHash()
	{
		std::sort(hashes.begin(), hashes.end(), [](Entry& a, Entry& b)
		{
			return a.h < b.h;
		});
	}

	void SortByName()
	{
		std::sort(hashes.begin(), hashes.end(), [](Entry& a, Entry& b)
		{
			return a.n < b.n;
		});
	}

	std::vector<Entry> hashes;
};