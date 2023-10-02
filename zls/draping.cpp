#include <regex>
#include <map>
#include <miniz/miniz.h>
#include "zls.hpp"
#include "image.hpp"
#include "draping.hpp"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

namespace {
	bool str_starts_with(const char* s, const char* t)
	{
		while (*s && *t)
		{
			if (*s != *t)
				return false;
			s++;
			t++;
		}
		return !*t;
	}
}

void zls::read_bytes(void* dest, const uint8_t** source, std::size_t n)
{
	memcpy(dest, *source, n);
	*source += n;
}

int zls::get_png_offset(const uint8_t* data, int len)
{
	// https://www.w3.org/TR/PNG-Structure.html#PNG-file-signature
	// The first eight bytes of a PNG file always contain the following
	// (decimal) values:
	const uint8_t signature[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	const int signatureLength = ARRAY_SIZE(signature);
	if (len < signatureLength)
		return -1;
	const int end = len - signatureLength;
	for (int offset = 0; offset <= end; ++offset) {
		if (std::equal(signature, signature + signatureLength,
			data + offset, data + offset + signatureLength))
		{
			return offset;
		}
	}
	return -1;
}

bool zls::read_draping_item(Archive& zls, int entry, const FileStat& status, const Uuid& uuid, Draping_item* item)
{
	std::vector<uint8_t> buffer;
	buffer.resize(status.m_uncomp_size);
	if (zls.extract(entry, buffer.data(), buffer.size()))
	{
		int png_offset = get_png_offset(buffer.data(), (int)buffer.size());
		if (png_offset < 0)
			return false;

		item->uuid = uuid;
		item->data.swap(buffer);
		item->png_offset = png_offset;
		return true;
	}
	return false;
}
bool zls::read_draping_board(Archive& zls, int entry, const FileStat& status, Draping_board* board)
{
	using namespace std;

	vector<uint8_t> buffer;
	buffer.resize(status.m_uncomp_size);
	zls.extract(entry, buffer.data(), buffer.size());
	if (buffer.size() < 8 + 8 + 4)
		return false;
	const uint8_t* start = buffer.data();
	const uint8_t* end = start + buffer.size();
	const uint8_t* source = start;
	read_bytes(&board->main.first, &source, sizeof(board->main.first));
	read_bytes(&board->main.second, &source, sizeof(board->main.second));
	uint32_t n = 0;
	read_bytes(&n, &source, sizeof(n));
	board->uuids.clear();
	for (uint32_t i = 0; i < n && source + 16 <= end; ++i)
	{
		Uuid uuid;
		read_bytes(&uuid.first, &source, sizeof(uuid.first));
		read_bytes(&uuid.second, &source, sizeof(uuid.second));
		board->uuids.push_back(uuid);
	}
	return true;
}

bool zls::parseull(const std::string& s, unsigned long long* value)
{
	const std::string max = std::to_string(std::numeric_limits<unsigned long long>::max());
	if (s.size() < max.size())
	{
		*value = std::stoull(s);
		return true;
	}
	else if (s.size() > max.size())
	{
		return false;
	}
	else if (s.compare(max) <= 0)
	{
		*value = std::stoull(s);
		return true;
	}
	return false;
}

bool zls::read_thumbnail_image(Archive& zls, Image* thumbnail, Uuid* uuid)
{
	int num_files = zls.nfiles();
	std::cmatch match;
	std::regex regex("drapingBoard/([0-9]+)_([0-9]+)/drapingItemHeader");
	mz_zip_archive_file_stat status;
	Draping_board draping_board;
	std::map<Uuid, int> draping_item_entries;
	Draping_item fallback_draping_item;
	Draping_item main_draping_item;
	for (int entry = 0; entry < num_files; ++entry)
	{
		zls.stat(entry, &status);
		if (!str_starts_with(status.m_filename, "drapingBoard/"))
			continue;
		if (zls.is_dir(entry))
			continue;
		if (strcmp("drapingBoard/drapingBoardHeader", status.m_filename) == 0)
		{
			read_draping_board(zls, entry, status, &draping_board);
		}
		else if (std::regex_match(status.m_filename, match, regex))
		{
			Uuid uuid;
			if (parseull(match[1].str(), &uuid.first) && parseull(match[2].str(), &uuid.second))
			{
				draping_item_entries[uuid] = entry;
				if (fallback_draping_item.data.empty())
				{
					read_draping_item(zls, entry, status, uuid, &fallback_draping_item);
				}
			}
		}
	}

	auto found = draping_item_entries.find(draping_board.main);
	if (draping_board.main != Uuid(0, 0) ||
		found != draping_item_entries.end())
	{
		if (found->first != fallback_draping_item.uuid)
		{
			zls.stat(found->second, &status);
			read_draping_item(zls, found->second, status, found->first, &main_draping_item);
		}
	}

	const Draping_item* target_draping_item = &main_draping_item;
	if (main_draping_item.data.empty())
		target_draping_item = &fallback_draping_item;

	if (target_draping_item->data.empty())
		return false;

	*uuid = target_draping_item->uuid;
	const uint8_t* png = target_draping_item->data.data() + target_draping_item->png_offset;
	const int png_size = (int)target_draping_item->data.size() - target_draping_item->png_offset;
	*thumbnail = Image(png, png_size);
	return true;
}