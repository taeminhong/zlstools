#pragma once

#include <vector>
#include <string>
#include "fwd.hpp"

namespace zls {
	typedef std::pair<std::uint64_t, std::uint64_t> Uuid;

	struct Draping_item {
		Uuid uuid;
		std::vector<uint8_t> data;
		int png_offset = -1;
	};

	struct Draping_board {
		Uuid main;
		std::vector<Uuid> uuids;
	};

	void read_bytes(void* dest, const uint8_t** source, std::size_t n);
	int get_png_offset(const uint8_t* data, int len);
	bool read_draping_item(Archive& zls, int entry, const FileStat& status, const Uuid& uuid, Draping_item* item);
	bool read_draping_board(Archive& zls, int entry, const FileStat& status, Draping_board* board);
	bool parseull(const std::string& s, unsigned long long* value);
	bool read_thumbnail_image(Archive& zls, Image* thumbnail, Uuid* uuid);
}