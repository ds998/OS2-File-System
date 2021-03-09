#include "fs.h"
#include "part.h"
#include "KernelFS.h"
#include <iostream>
using namespace std;
KernelFS* FS::myImpl = new KernelFS();
char FS::mount(Partition* partition) {
	return myImpl->mount(partition);
}

char FS::unmount() {
	return myImpl->unmount();
}

char FS::format() {
	return myImpl->format();
}

FileCnt FS::readRootDir() {
	return (FileCnt)myImpl->readRootDir();
}

File* FS::open(char* fname, char mode) {
	return myImpl->open(fname, mode);
}

char FS::doesExist(char* fname) {
	return myImpl->doesExist(fname);
}

char FS::deleteFile(char* fname) {
	return myImpl->deleteFile(fname);
}

FS::FS() {
	myImpl = new KernelFS();
}

FS::~FS() {
	delete myImpl;
}

KernelFS* FS::get_impl() {
	return myImpl;
}