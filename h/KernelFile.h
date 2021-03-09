#pragma once
#include <vector>
#include "KernelFS.h"
#include "file.h"
using namespace std;
class Partition;
class KernelFile {
	struct block {
		BytesCnt block;
		BytesCnt pos;
		BytesCnt origin_block;
	};
	Partition* part = nullptr;
	char shift_buffer[2048];
	KernelFS::f* file;
	BytesCnt pos = 0;
	BytesCnt end = 0;
	vector<block*> sec_indexes;
	vector<block*> file_blocks;
	bool unread;
	KernelFile();
	~KernelFile();
	char write(BytesCnt, char* buffer);
	BytesCnt read(BytesCnt, char* buffer);
	char seek(BytesCnt);
	BytesCnt filePos();
	char eof();
	BytesCnt getFileSize();
	char truncate();
	friend class File;
};
