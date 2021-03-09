#include "file.h"
#include "KernelFS.h"
#include "KernelFile.h"


File::~File() { delete myImpl; myImpl = nullptr; }
File::File() { myImpl = new KernelFile(); }
char File::write(BytesCnt cnt, char* buffer) { return myImpl->write(cnt, buffer); }
BytesCnt File::read(BytesCnt cnt, char* buffer) { return myImpl->read(cnt, buffer); }
char File::seek(BytesCnt address) { return myImpl->seek(address); }
BytesCnt File::filePos() { return myImpl->filePos(); }
char File::eof() { return myImpl->eof(); }
BytesCnt File::getFileSize() { return myImpl->getFileSize(); }
char File::truncate() { return myImpl->truncate(); }