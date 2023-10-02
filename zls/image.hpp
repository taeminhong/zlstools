#pragma once

#include <cstdint>

namespace zls {
	class Image {
	public:
		Image();
		Image(int w, int h);
		Image(const void* data, int len);
		~Image();
		Image(Image&&) noexcept;
		Image(const Image&) = delete;
		Image& operator=(Image&& other) noexcept;
		Image& operator=(const Image&) = delete;
		bool empty() const noexcept;
		int stride() const noexcept;
		int width() const noexcept;
		int height() const noexcept;
		int nchannels() const noexcept;
		std::uint8_t* scanline(int y);
		const std::uint8_t* scanline(int y) const;
		Image resize(int w, int h);
		bool write_bmp(const char* filename) const;
		void swap(Image& other) noexcept;
	private:
		std::uint8_t* _bitmap;
		int _width;
		int _height;
		int _nchannels;
	};
}