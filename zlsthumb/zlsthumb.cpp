#include <iostream>
#include <iomanip>
#include <zls/image.hpp>
#include <zls/draping.hpp>
#include <zls/zls.hpp>

std::string to_string(const zls::Uuid& uuid, const std::string& sep)
{
	return std::to_string(uuid.first) + sep + std::to_string(uuid.second);
}

class CFile: public zls::File {
public:
	CFile()
		: _file(nullptr)
	{
	}
	~CFile()
	{
		if (_file)
			fclose(_file);
	}
	CFile(const CFile& other) = delete;
	CFile& operator=(const CFile& other) = delete;
	bool open(const char* filename, const char* mode)
	{
		_file = fopen(filename, mode);
		return _file;
	}
	bool is_open() const
	{
		return _file;
	}
	void swap(CFile& other)
	{
		std::swap(_file, other._file);
	}

	virtual uint_type read(void* buf, uint_type size)
	{
		if (!is_open())
			return 0;
		return fread(buf, 1, size, _file);
	}
	virtual int_type tell()
	{
		if (!is_open())
			return -1;
		return ftell(_file);
	}
	virtual bool seek(int_type offset, int_type origin)
	{
		if (!is_open())
			return false;
		
		return fseek(_file, static_cast<long>(offset), cfile_origin(origin)) == 0;
	}
private:
	int cfile_origin(int_type origin)
	{
		switch (origin)
		{
		case Set: return SEEK_SET;
		case Cur: return SEEK_CUR;
		case End: return SEEK_END;
		}
		throw std::invalid_argument(std::to_string(origin) + "is invalid origin value.");
	}
private:
	FILE* _file;
};

int main(int argc, char* argv[])
{
	using namespace std;
	using namespace zls;

	try {
		if (argc < 2) {
			cerr << "usage: zlsthumb [FILE]...\n";
			exit(1);
		}

		for (int i = 1; i < argc; ++i) {
			try {
				const char* filename = argv[i];
				cout << "file name: " << filename << endl;
				CFile file;
				file.open(argv[i], "rb");
				if (!file.is_open()) {
					cerr << "failed to open file: " << argv[i] << "\n";
					continue;
				}
				Archive zls;
				if (!zls.open(&file, 0))
				{
					cerr << "failed to load zip file\n";
					continue;
				}
				Uuid thumbnail_uuid;
				Image thumbnail;
				if (!read_thumbnail_image(zls, &thumbnail, &thumbnail_uuid)) {
					cerr << "Can't find thumbnail image.\n";
					continue;
				}

				cout << "thumbnail image size: " << thumbnail.width() << "x" << thumbnail.height() << "\n"
					<< "channels: " << thumbnail.nchannels() << "\n";

				const string outfilename = to_string(thumbnail_uuid.first) + "_" + to_string(thumbnail_uuid.second) + ".bmp";

				Image small_image = thumbnail.resize(256, 256);

				small_image.write_bmp(outfilename.c_str());
			}
			catch (std::exception& e) {
				cerr << e.what() << endl;
				continue;
			}
			catch (...) {
				cerr << "Something went wrong.\n";
				continue;
			}
		}
		return 0;
	}
	catch (std::exception& e) {
		cerr << e.what() << endl;
		exit(1);
	}
	catch (...) {
		cerr << "Something went wrong.\n";
		exit(1);
	}
}