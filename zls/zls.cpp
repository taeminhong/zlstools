#include "zls.hpp"
#include <vector>
#include <miniz/miniz.h>

namespace {
	size_t offset_read(void* pOpaque, mz_uint64 pos, void* pBuf, size_t n)
	{
		zls::FileView* fileView = (zls::FileView*)pOpaque;
		return fileView->read(pos, pBuf, n);
	}

	size_t find_zip_offset(const std::vector<uint8_t>& buffer)
	{
		uint8_t signatures[][4] = {
			{ 0x50, 0x4b, 0x03, 0x04 },
			{ 0x50, 0x4b, 0x05, 0x06 },
			{ 0x50, 0x4b, 0x07, 0x08 }
		};
		size_t sig_size = sizeof(signatures[0]);
		size_t nsig = sizeof(signatures) / sig_size;
		if (buffer.size() < sig_size)
			return -1;

		const uint8_t* end = buffer.data() + (buffer.size() - sig_size);
		for (const uint8_t* p = buffer.data(); p <= end; ++p)
		{
			for (size_t i = 0; i < nsig; ++i)
			{
				if (std::equal(p, p + sig_size, signatures[i]))
					return p - buffer.data();
			}
		}
		return -1;
	}
}
zls::FileView::FileView(File* file, uint_type offset)
	: _file(file),
	_offset(offset)
{
}

zls::FileView::uint_type zls::FileView::read(uint_type fake_pos, void* pBuf, uint_type n)
{
	if (_offset > pos_limit() || fake_pos > pos_limit() - _offset)
		return 0;
		int_type cur_pos = _file->tell();
		int_type real_pos = static_cast<int_type>(_offset + fake_pos);
		if (cur_pos != real_pos && !_file->seek(real_pos, File::Set))
			return 0;
		return _file->read(pBuf, n);
}

zls::File* zls::FileView::file() noexcept
{
	return _file;
}

zls::FileView::uint_type zls::FileView::offset() const noexcept
{
	return _offset;
}

zls::Archive::Archive() = default;

zls::Archive::~Archive()
{
	close();
}

bool zls::Archive::is_open() const noexcept
{
	return !!_view;
}

void zls::Archive::close()
{
	if (!is_open())
		return;

	mz_zip_reader_end(_zip.get());
	_zip.reset();
	_view.reset();
}

bool zls::Archive::open(File* file, uint32_t flags)
{
	if (!file->seek(0, File::End))
		return false;
	File::uint_type real_file_size = static_cast<File::uint_type>(file->tell());
	if (!file->seek(0, File::Set))
		return false;
	std::vector<std::uint8_t> buffer;
	buffer.resize(std::min(File::uint_type(128), real_file_size));
	if (file->read(buffer.data(), buffer.size()) != buffer.size())
		return false;
	File::uint_type zip_offset = find_zip_offset(buffer);
	if (zip_offset == -1)
		return false;
	if (real_file_size <= zip_offset)
		return false;
	File::uint_type file_size = real_file_size - zip_offset;
	{
		close();
		_zip.reset(new mz_zip_archive());
		_view.reset(new FileView(file, zip_offset));
		memset(_zip.get(), 0, sizeof(mz_zip_archive));
	}
	if (!mz_zip_reader_init_internal(_zip.get(), static_cast<mz_uint32>(flags)))
	{
		close();
		return false;
	}
	_zip->m_pRead = offset_read;
	_zip->m_pIO_opaque = _view.get();
	_zip->m_archive_size = file_size;
	if (!mz_zip_reader_read_central_dir(_zip.get(), static_cast<mz_uint32>(flags)))
	{
		close();
		return false;
	}
	return true;
}

bool zls::Archive::extract(int index, void* buf, size_t bufsize)
{
	if (!is_open())
		return false;

	return mz_zip_reader_extract_to_mem(_zip.get(), index, buf, bufsize, 0);
}

int zls::Archive::nfiles()
{
	if (!is_open())
		return 0;
	return mz_zip_reader_get_num_files(_zip.get());
}

bool zls::Archive::stat(int index, FileStat* stat)
{
	if (!is_open())
		return false;
	return mz_zip_reader_file_stat(_zip.get(), index, stat);
}

bool zls::Archive::is_dir(int index)
{
	if (!is_open())
		return false;
	return mz_zip_reader_is_file_a_directory(_zip.get(), index);
}