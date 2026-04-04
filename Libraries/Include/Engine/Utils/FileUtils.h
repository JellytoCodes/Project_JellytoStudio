#pragma once

enum FileMode : uint8
{
	Write,
	Read,
};

class FileUtils
{
public:
	FileUtils();
	~FileUtils();

	void Open(std::wstring filePath, FileMode mode);

	template<typename T>
	void Write(const T& data);

	// 템플릿 특수화
	template <>
	void Write<std::string>(const std::string& data)
	{
		return Write(data);
	}

	void Write(void* data, uint32 dataSize);
	void Write(const std::string& data);

	template<typename T>
	void Read(OUT T& data);

	template<typename T>
	T Read();

	void Read(void** data, uint32 dataSize);
	void Read(OUT std::string& data);

private:
	HANDLE _handle = INVALID_HANDLE_VALUE;
};

template <typename T>
void FileUtils::Write(const T& data)
{
	DWORD numOfBytes = 0;
	assert(::WriteFile(_handle, &data, sizeof(T), static_cast<LPDWORD>(&numOfBytes), nullptr));
}



template <typename T>
void FileUtils::Read(T& data)
{
	DWORD numOfBytes = 0;
	assert(::ReadFile(_handle, &data, sizeof(T), static_cast<LPDWORD>(&numOfBytes), nullptr));
}

template <typename T>
T FileUtils::Read()
{
	T data;
	Read(data);
	return data;
}
