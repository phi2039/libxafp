
#include <iostream>
#include "../include/libxafp.h"

xafp_context_pool_handle pool = NULL;

xafp_client_handle get_context(const char* host, const char* username, const char* pass)
{
  if (pool)
    return xafp_get_context(pool, host, username, pass);
  return xafp_create_context("kennel", username, pass);
}

void free_context(xafp_client_handle ctx)
{
  if (pool)
    xafp_free_context(pool, ctx);
  else
    xafp_destroy_context(ctx);  
}

void cp(const char* path, const char* dest, const char* username, const char* pass)
{
  xafp_client_handle ctx = get_context("kennel", username, pass);
  
  // Authenticate and open the desired volume
  char volume[128];
  if (path[0] == '/')
    path++; //offset start by one to account for leading '/'
  
  char* pVol = strstr(path, "/"); // Find volume name separator
  if (pVol)
  {
    int pos = pVol - path;
    strncpy(volume, path, pos);
    volume[pos] = 0;
  }
  else
    return;

    xafp_file_handle fileHandle = xafp_open_file(ctx, path, xafp_open_flag_read);
    if (fileHandle)
    {
      uint64_t bytesWritten = 0;
      uint64_t lastUpdate = 0;
      FILE* fp = fopen("/Users/chris/test.mp4", "w");
      uint64_t bufLen = 262144;
      void* pBuf = malloc(bufLen);
      time_t lastTime = time(NULL);
      time_t startTime = lastTime;
      while (xafp_read_file(ctx, fileHandle, bytesWritten, pBuf, bufLen) > 0)
      {
        fwrite(pBuf, 1, bufLen, fp);
        bytesWritten += bufLen; 
        uint64_t deltaBytes = bytesWritten - lastUpdate;
        if (deltaBytes > 10485760) // 10MB
        {
          time_t deltaTime = time(NULL) - lastTime;
          printf("%lu: Wrote %llu bytes in %lu seconds (%0.2f KB/sec)\n", clock(), deltaBytes, deltaTime, (double)(deltaBytes/1024) / (double)deltaTime);
          lastTime = time(NULL);
          lastUpdate = bytesWritten;
        }
      }
      time_t deltaTime = time(NULL) - startTime;
      printf("%lu: Wrote %llu bytes in %lu seconds (%0.2f KB/sec)\n", clock(), bytesWritten, deltaTime, (double)(bytesWritten/1024) / (double)deltaTime);
      free(pBuf);
      fclose(fp);
      xafp_close_file(ctx, fileHandle);
    }
  
  // Clean-up the session
  free_context(ctx);
}

void ls(const char* path, const char* username, const char* pass)
{
  xafp_client_handle ctx = get_context("kennel", username, pass);
  
  // Authenticate and open the desired volume
  char volume[128];
  if (path[0] == '/')
    path++; //offset start by one to account for leading '/'
  
  char* pVol = strstr(path, "/"); // Find volume name separator
  if (pVol)
  {
    int pos = pVol - path;
    strncpy(volume, path, pos);
    volume[pos] = 0;
  }
  else
    return;
  
    xafp_node_iterator iter = xafp_get_dir_iter(ctx, path);
    xafp_node_info* pNode = xafp_next(iter);
    while(pNode)
    {
      if (!(pNode->attributes & xafp_node_att_hidden))
      {
        uint32_t afpRights = pNode->unixPrivs.userRights;
        time_t modTime = pNode->modDate;
        char timeString[32];
        strncpy(timeString, ctime(&modTime), 32);
        timeString[strlen(timeString) - 1] = '\0';
        printf ("%s%s%s%s%s%s%s%s%s%s %s %d %d %dK %s %s\n", 
                pNode->isDirectory ? "d" : "-",
                (afpRights & MAKE_XAFP_PERMS_OWNER(xafp_node_perms_read)) ? "r" : "-",
                (afpRights & MAKE_XAFP_PERMS_OWNER(xafp_node_perms_write)) ? "w" : "-",
                (afpRights & MAKE_XAFP_PERMS_OWNER(xafp_node_perms_exec)) ? "x" : "-",
                (afpRights & MAKE_XAFP_PERMS_GROUP(xafp_node_perms_read)) ? "r" : "-",
                (afpRights & MAKE_XAFP_PERMS_GROUP(xafp_node_perms_write)) ? "w" : "-",
                (afpRights & MAKE_XAFP_PERMS_GROUP(xafp_node_perms_exec)) ? "x" : "-",
                (afpRights & MAKE_XAFP_PERMS_OTHER(xafp_node_perms_read)) ? "r" : "-",
                (afpRights & MAKE_XAFP_PERMS_OTHER(xafp_node_perms_write)) ? "w" : "-",
                (afpRights & MAKE_XAFP_PERMS_OTHER(xafp_node_perms_exec)) ? "x" : "-",
                "-",
                pNode->unixPrivs.userId,
                pNode->unixPrivs.groupId,
                pNode->isDirectory ? 0 : (uint32_t)(pNode->fileInfo.dataForkLen/1000),
                timeString,
                pNode->name
                );
      }
      pNode = xafp_next(iter);
    }
    xafp_free_iter(iter);
  
  // Clean-up the session
  free_context(ctx);
}

void mkdir(const char* path, const char* username, const char* pass)
{
  xafp_client_handle ctx = get_context("kennel", username, pass);
  
  // Authenticate and open the desired volume
  char volume[128];
  if (path[0] == '/')
    path++; //offset start by one to account for leading '/'
  
  char* pVol = strstr(path, "/"); // Find volume name separator
  if (pVol)
  {
    int pos = pVol - path;
    strncpy(volume, path, pos);
    volume[pos] = 0;
  }
  else
    return;
  
    xafp_create_dir(ctx, path);
  
  // Clean-up the session
  free_context(ctx);  
}

void rm(const char* path, const char* username, const char* pass)
{
  xafp_client_handle ctx = get_context("kennel", username, pass);
  
  // Authenticate and open the desired volume
  char volume[128];
  if (path[0] == '/')
    path++; //offset start by one to account for leading '/'
  
  char* pVol = strstr(path, "/"); // Find volume name separator
  if (pVol)
  {
    int pos = pVol - path;
    strncpy(volume, path, pos);
    volume[pos] = 0;
  }
  else
    return;
  
  xafp_remove(ctx, path);
  
  // Clean-up the session
  free_context(ctx);  
}

void touch(const char* path, const char* username, const char* pass)
{
  xafp_client_handle ctx = get_context("kennel", username, pass);
  
  // Authenticate and open the desired volume
  char volume[128];
  if (path[0] == '/')
    path++; //offset start by one to account for leading '/'
  
  char* pVol = strstr(path, "/"); // Find volume name separator
  if (pVol)
  {
    int pos = pVol - path;
    strncpy(volume, path, pos);
    volume[pos] = 0;
  }
  else
    return;
  
  xafp_create_file(ctx, path);
  free_context(ctx);  
}

void cat(const char* path, void* pBuf, int len, const char* username, const char* pass, uint32_t offset=0)
{
  xafp_client_handle ctx = get_context("kennel", username, pass);
  
  // Authenticate and open the desired volume
  char volume[128];
  if (path[0] == '/')
    path++; //offset start by one to account for leading '/'
  
  char* pVol = strstr(path, "/"); // Find volume name separator
  if (pVol)
  {
    int pos = pVol - path;
    strncpy(volume, path, pos);
    volume[pos] = 0;
  }
  else
    return;
  
    xafp_file_handle file = xafp_open_file(ctx, path, xafp_open_flag_read | xafp_open_flag_write);
    if (file)
    {
      xafp_write_file(ctx, file, offset, pBuf, len);
      xafp_close_file(ctx, file);
    }
  
  // Clean-up the session
  free_context(ctx);  
}

void mv(const char* path, const char* newPath, const char* username, const char* pass, uint32_t offset=0)
{
  xafp_client_handle ctx = get_context("kennel", username, pass);
  
  // Authenticate and open the desired volume
  char volume[128];
  if (path[0] == '/')
    path++; //offset start by one to account for leading '/'
  
  char* pVol = strstr(path, "/"); // Find volume name separator
  if (pVol)
  {
    int pos = pVol - path;
    strncpy(volume, path, pos);
    volume[pos] = 0;
  }
  else
    return;
  
    xafp_rename_file(ctx, path, newPath);
  
  // Clean-up the session
  free_context(ctx);  
}


int main (int argc, char * const argv[]) 
{
  xafp_set_log_level(XAFP_LOG_LEVEL_INFO | XAFP_LOG_FLAG_DSI_PROTO | XAFP_LOG_FLAG_SESS_MGR);
  char secret[32];
  FILE* fsecret = fopen(argv[1], "r");
  fread(secret, 1, sizeof(secret), fsecret);

  pool = xafp_create_context_pool();
  
  ls("/Media/video/Movies/BRRip","chris",secret);
  
  mkdir("/Media/video/Test/foo","chris",secret);  
  rm("/Media/video/Test/foo","chris",secret);

  rm("/Media/video/Test/bar.txt","chris",secret);
  touch("/Media/video/Test/bar.txt","chris",secret);
  char msg[] = "Hello World!\n";
  cat("/Media/video/Test/bar.txt",msg, strlen(msg), "chris",secret);
  
  mv("/Media/video/Test/bar.txt", "/Media/video/Test/bar2.txt", "chris",secret);

  rm("/Media/video/Test/bar2.txt","chris",secret);
//  cp("/Media/video/Movies/BRRip/Aeon Flux.mp4","/Users/chris/test.mp4","chris",secret);

//  sleep(30);
  
  xafp_destroy_context_pool(pool);
  
  return 0;
}
