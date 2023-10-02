#pragma once

#include <cstdint>
#include <memory>
#include "fwd.hpp"

namespace zls
{
	class File {
	public:
		typedef std::int64_t int_type;
		typedef std::uint64_t uint_type;
		enum { Set, Cur, End };
		virtual uint_type read(void* buf, uint_type size) = 0;
		virtual int_type tell() = 0;
		virtual bool seek(int_type offset, int_type origin) = 0;
	};


	class FileView {
	public:
		typedef File::int_type int_type;
		typedef File::uint_type uint_type;
		FileView(File* file, uint_type offset);
		uint_type read(uint_type fake_pos, void* pBuf, uint_type n);
		File* file() noexcept;
		uint_type offset() const noexcept;
	private:
		constexpr uint_type pos_limit() noexcept
		{
			return static_cast<uint_type>(std::numeric_limits<int_type>::max());
		}
	private:
		File* _file;
		uint_type _offset;
	};

	class Archive {
	public:
		Archive();
		~Archive();		
		bool is_open() const noexcept;
		void close();
		bool open(File* file, uint32_t flags);
		bool extract(int index, void* buf, size_t bufsize);
		int nfiles();
		bool stat(int index, FileStat* stat);
		bool is_dir(int index);
	private:
		std::unique_ptr<FileView> _view;
		std::unique_ptr<mz_zip_archive> _zip;
	};
}

