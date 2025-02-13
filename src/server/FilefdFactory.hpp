#pragma once

#include <cstddef>
#include <map>
#include <sys/types.h>

typedef struct s_fdoffset {
  int filefd;
  off_t offset;
  size_t fileSize;
  bool done;
  s_fdoffset(int filefd_, off_t offset_, size_t fileSize_, bool done_)
      : filefd(filefd_), offset(offset_), fileSize(fileSize_), done(done_) {};
} t_fdoffset;

class FilefdFactory {
private:
  std::map<int, t_fdoffset *> fdoffsets;
  int chunkSize;

public:
  bool ifExists(int fd);
  void addFdoffset(int event_fd, int filefd, size_t fileSize, off_t offset);
  void sendFdoffset(int fd);
  void removeFdoffset(int fd);
  int getChunkSize();
  FilefdFactory(int chunkSize_) : chunkSize(chunkSize_) {};
  ~FilefdFactory() {
    for (std::map<int, t_fdoffset *>::iterator it = fdoffsets.begin();
         it != fdoffsets.end(); ++it) {
      delete it->second;
    }
  };
};
