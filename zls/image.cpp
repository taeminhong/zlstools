#include <cstring>
#define STB_IMAGE_IMPLEMENTATION
#include <zls/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <zls/stb_image_write.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <zls/stb_image_resize.h>
#include <stdexcept>
#include "image.hpp"

zls::Image::Image()
	: _bitmap(nullptr),
	_width(0),
	_height(0),
	_nchannels(0)
{}

zls::Image::Image(int width, int height)
{
	if (width <= 0 || height <= 0)
		throw std::invalid_argument("A nagative size is not allowed.");

	int nchannels = 4;
	void* bitmap = stbi__malloc(width * height * nchannels);
	if (bitmap) {
		_bitmap = static_cast<stbi_uc*>(bitmap);
		_width = width;
		_height = height;
		_nchannels = nchannels;
	}
	else
	{
		throw std::runtime_error("memory allocation failed.");
	}
}

zls::Image::Image(Image&& other) noexcept
	: _bitmap(other._bitmap),
	_width(other._width),
	_height(other._height),
	_nchannels(other._nchannels)
{
	other._bitmap = nullptr;
	other._width = 0;
	other._height = 0;
	other._nchannels = 0;
}

zls::Image& zls::Image::Image::operator=(Image&& other) noexcept
{
	swap(other);
	return *this;
}


zls::Image::~Image()
{
	stbi_image_free(_bitmap);
}

bool zls::Image::empty() const noexcept
{
	return !_bitmap;
}

zls::Image::Image(const void* data, int len)
{
	stbi_uc* bitmap = stbi_load_from_memory(static_cast<const stbi_uc*>(data), len, &_width, &_height, nullptr, 4);
	if (bitmap) {
		_bitmap = bitmap;
		_nchannels = 4;
	}
	else
	{
		throw std::runtime_error(stbi_failure_reason());
	}
}

int zls::Image::stride() const noexcept
{
	return _width * _nchannels;
}

int zls::Image::width() const noexcept
{
	return _width;
}

int zls::Image::height() const noexcept
{
	return _height;
}

int zls::Image::nchannels() const noexcept
{
	return _nchannels;
}

std::uint8_t* zls::Image::scanline(int y)
{
	const Image* const_this = this;
	return const_cast<std::uint8_t*>(const_this->scanline(y));
}

const std::uint8_t* zls::Image::scanline(int y) const
{
	if (empty() || y < 0)
		return nullptr;
	return _bitmap + stride() * y;
}

void zls::Image::swap(zls::Image& other) noexcept
{
	using std::swap;
	swap(_bitmap, other._bitmap);
	swap(_width, other._width);
	swap(_height, other._height);
	swap(_nchannels, other._nchannels);
}

zls::Image zls::Image::resize(int w, int h)
{
	Image result(w, h);

	if (empty())
		return result;

	stbir_resize_uint8_srgb(
		_bitmap, _width, _height, stride(),
		result._bitmap, result._width, result._height, result.stride(),
		4, 3, 0);
	return result;
}

bool zls::Image::write_bmp(const char* filename) const
{
	return stbi_write_bmp(filename, _width, _height, _nchannels, _bitmap);
}
