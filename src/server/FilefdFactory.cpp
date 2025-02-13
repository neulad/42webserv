#include "FilefdFactory.hpp"
#include <sys/sendfile.h>
#include <sys/types.h>
#include <unistd.h>

bool FilefdFactory::ifExists(int fd) {
  return fdoffsets.find(fd) == fdoffsets.end();
}
void FilefdFactory::addFdoffset(int event_fd, int filefd, size_t fileSize,
                                off_t offset) {
  fdoffsets[event_fd] = new t_fdoffset(filefd, offset, fileSize, false);
}
void FilefdFactory::sendFdoffset(int event_fd) {
  if (!ifExists(event_fd))
    return;
  t_fdoffset *fdoffset = fdoffsets[event_fd];
  sendfile(event_fd, fdoffset->filefd, &fdoffset->offset, chunkSize);
  if ((unsigned long)fdoffset->offset == fdoffset->fileSize)
    removeFdoffset(event_fd);
}
void FilefdFactory::removeFdoffset(int event_fd) {
  if (!ifExists(event_fd))
    return;
  t_fdoffset *fdoffset = fdoffsets[event_fd];
  close(fdoffset->filefd), fdoffsets.erase(event_fd);
}

int FilefdFactory::getChunkSize() { return chunkSize; }
