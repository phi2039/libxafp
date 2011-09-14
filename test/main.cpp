#include <iostream>

#include "../src/AFPClient.h"
#include "../src/AFPClient.h"

void ls(const char* path, const char* username, const char* pass)
{
  // Obtain an AFP Session
  // TODO: Should we implement a session manager to minimize setup/teardown?
  CAFPSession session;
  //  if(!session.Open("127.0.0.1", 548, 3000))
  if(!session.Open("kennel", 548, 3000))
    return;
  
  // Authenticate the session
  session.Login(username,pass);
  
  uint32_t volID = session.OpenVolume("Media");
  if (volID)
  {
    int dirID = session.GetDirectory(volID, path);
    //session.List(volID, dirID);
    int fileHandle = session.OpenFile(volID, dirID, "Aeon Flux.mp4");
    if (fileHandle)
    {
      uint64_t bytesWritten = 0;
      uint64_t lastUpdate = 0;
      FILE* fp = fopen("/Users/chris/test.mp4", "w");
      uint64_t bufLen = 262144;
      void* pBuf = malloc(bufLen);
      time_t lastTime = time(NULL);
      time_t startTime = lastTime;
      while (session.ReadFile(fileHandle, bytesWritten, pBuf, bufLen) > 0)
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
      session.CloseFile(fileHandle);
    }
    session.CloseVolume(volID);
  }
  
  // Clean-up the session
  session.Logout();
  session.Close();
}

int main (int argc, char * const argv[]) 
{
  XafpSetLogLevel(XAFP_LOG_LEVEL_INFO);
  char secret[32];
  FILE* fsecret = fopen(argv[1], "r");
  fread(secret, 1, sizeof(secret), fsecret);

  ls("video/Movies/BRRip","chris",secret);
  
  return 0;
}
