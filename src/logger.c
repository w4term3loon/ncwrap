#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static const char file_path[] = "wrap.log";

ncw_err
ncw_log_file(const char *log) {
  FILE *file = fopen(file_path, "a");
  if (file == NULL) {
    // NOTE: errno ignored (?)
    return NCW_FAILED_TO_OPEN;
  }

  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);

  char buffer[26];
  memset(buffer, 0, sizeof buffer);

  strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", timeinfo);
  fprintf(file, "[%s\b]: %s\n", buffer, log);

  if (fclose(file)) {
    return NCW_FAILED_TO_CLOSE;
  }

  return NCW_OK;
}
