#include "ProcessInfo.h"
#include "CurrentThread.h"
#include "FileUtil.h"

__thread int t_numOpenedFiles = 0;

int fdDirFilter(const struct dirent* d)
{
  if (::isdigit(d->d_name[0]))
  {
    ++t_numOpenedFiles;
  }

  return 0;
}

__thread std::vector<pid_t>* t_pids = NULL;
int taskDirFilter(const struct dirent* d)
{
  if (::isdigit(d->d_name[0]))
  {
    t_pids->push_back(atoi(d->d_name));
  }
  
  return 0;
}

int scanDir(
    const char *dirpath, 
    int (*filter)(const struct dirent *))
{
  struct dirent** namelist = NULL;
  int result = ::scandir(
          dirpath, 
          &namelist, 
          filter, 
          alphasort);
  assert(namelist == NULL);
  return result;
}

TimeStamp g_startTime = TimeStamp::now();
int g_clockTicks = (int)(::sysconf(_SC_CLK_TCK));
int g_pageSize = (int)(::sysconf(_SC_PAGE_SIZE));

pid_t pid()
{
  return ::getpid();
}

string pidString()
{
  char buf[32];
  snprintf(buf, sizeof buf, "%d", pid());
  return buf;
}

uid_t uid()
{
  return ::getuid();
}

string username()
{
  struct passwd pwd;
  struct passwd* result = NULL;
  char buf[8192];
  const char* name = "unknownuser";
  getpwuid_r(uid(), &pwd, buf, sizeof buf, &result);
  if (result)
  {
    name = pwd.pw_name;
  }
  
  return name;
}

uid_t euid()
{
  return ::geteuid();
}

TimeStamp startTime()
{
  return g_startTime;
}

int clockTicksPerSecond()
{
  return g_clockTicks;
}

int pageSize()
{
  return g_pageSize;
}

bool isDebugBuild()
{
#ifdef NDEBUG
  return false;
#else
  return true;
#endif
}

string hostname()
{
  char buf[256];
  if (::gethostname(buf, sizeof buf) == 0)
  {
    buf[sizeof(buf)-1] = '\0';
    return buf;
  }
  else
  {
    return "unknownhost";
  }
}

string procname()
{
  return procname(procStat()).as_string();
}

StringPiece procname(const string& stat)
{
  StringPiece name;
  size_t lp = stat.find('(');
  size_t rp = stat.rfind(')');
  if (lp != string::npos && rp != string::npos && lp < rp)
  {
    name.set(stat.data()+lp+1, (int)(rp-lp-1));
  }

  return name;
}

string procStatus()
{
  string result;
  readFile("/proc/self/status", 65536, &result);
  return result;
}

string procStat()
{
  string result;
  readFile("/proc/self/stat", 65536, &result);
  return result;
}

string threadStat()
{
  char buf[64];
  snprintf(
    buf, 
    sizeof buf, 
    "/proc/self/task/%d/stat", 
    tid());
  string result;
  readFile(buf, 65536, &result);
  return result;
}

string exePath()
{
  string result;
  char buf[1024];
  ssize_t n = ::readlink(
          "/proc/self/exe", 
          buf, 
          sizeof buf);
  if (n > 0)
  {
    result.assign(buf, n);
  }
  
  return result;
}

int openedFiles()
{
  t_numOpenedFiles = 0;
  scanDir("/proc/self/fd", fdDirFilter);
  return t_numOpenedFiles;
}

int maxOpenFiles()
{
  struct rlimit rl;
  if (::getrlimit(RLIMIT_NOFILE, &rl))
  {
    return openedFiles();
  }
  else
  {
    return (int)(rl.rlim_cur);
  }
}

CpuTime cpuTime()
{
  CpuTime t;
  struct tms tms;
  if (::times(&tms) >= 0)
  {
    const double hz = (double)(clockTicksPerSecond());
    t.userSeconds = (double)(tms.tms_utime) / hz;
    t.systemSeconds = (double)(tms.tms_stime) / hz;
  }

  return t;
}

int numThreads()
{
  int result = 0;
  string status = procStatus();
  size_t pos = status.find("Threads:");
  if (pos != string::npos)
  {
    result = ::atoi(status.c_str() + pos + 8);
  }
  
  return result;
}

std::vector<pid_t> threads()
{
  std::vector<pid_t> result;
  t_pids = &result;
  scanDir("/proc/self/task", taskDirFilter);
  t_pids = NULL;
  std::sort(result.begin(), result.end());
  return result;
}

