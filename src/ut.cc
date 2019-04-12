#include <fstream>
#include <memory>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#ifndef WIN32
#include <pwd.h>
#endif

#ifdef HAVE_LIBZIP
#include <zip.h>
#endif

#include "filament/Box.h"
#include "math/quat.h"
#include "math/mat4.h"

#include "Log.hh"

namespace bulb
{
   filament::math::mat4f scale_to_unitcube(const filament::Aabb& bounds)
   //--------------------------------------------------------------------
   {
      auto minpt = bounds.min;
      auto maxpt = bounds.max;
      float maxExtent = 0;
      maxExtent = std::max(maxpt.x - minpt.x, maxpt.y - minpt.y);
      maxExtent = std::max(maxExtent, maxpt.z - minpt.z);
      float scaleFactor = 2.0f / maxExtent;
      filament::math::float3 center = (minpt + maxpt) / 2.0f;
//      center.z += 4.0f / scaleFactor;
      filament::math::mat4f S = filament::math::mat4f::scaling(scaleFactor) * filament::math::mat4f::translation(-center);
      return S;
   }

   std::string temp_dir()
   //---------------------------
   {
      std::string tmpDir;
#ifdef __ANDROID__
      tmpDir = "/sdcard/Documents/bulb";
#else
#if defined (WIN32)
      const char* pch = getenv("USERPROFILE");
      if (pch == nullptr)
      {
         pch = getenv("SystemRoot");
         if (pch != nullptr)
            tmpDir = std::string(pch) + "\\Temp\\Documents\\bulb";
         else
         {
            pch = getenv("SystemDrive");
            if (pch != nullptr)
               tmpDir = std::string(pch) + "\\Temp\\Documents\\bulb";
            else
               tmpDir = "C:\\Temp\\Documents\\bulb";
         }
      }
      else
         tmpDir = std::string(pch) + "\\Documents\\bulb";
#else
      const char* pch = getenv("~");
      if (pch == nullptr)
      {
         pch = getenv("HOME");
         if (pch == nullptr)
         {
            struct passwd *uid;
            if ((uid = getpwuid(getuid())) != nullptr)
               pch = uid->pw_dir;
            else
               tmpDir = "/tmp/Documents/bulb";
         }
      }
      tmpDir = std::string(pch) + "/Documents/bulb";
#endif
#endif
      return tmpDir;
   }

   bool is_dir(const char* dir)
   //-----------------------------
   {
#if defined(_WIN32)
         DWORD result = GetFileAttributesW(dir);
         if (result == INVALID_FILE_ATTRIBUTES) return false;
         return (result & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
         struct stat sb;
         if (stat(dir, &sb))
            return false;
         return S_ISDIR(sb.st_mode);
#endif
      }

   bool mkdirs(const char* dirp)
   //--------------------------
   {
      std::string dir(dirp), dirSoFar;
      auto p = dir.find('/');
 #if defined (WIN32)
      if (p == std::string::npos)
         p = dir.find('\\');
 #endif
      while (p != std::string::npos)
      {
         dirSoFar.append(dir.substr(0, p)).append("/");
         if (! is_dir(dirSoFar.c_str()))
            mkdir(dirSoFar.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
         dir = dir.substr(p+1);
         p = dir.find('/');
// #if defined (WIN32)
         if (p == std::string::npos)
            p = dir.find('\\');
// #endif
      }
      dirSoFar.append(dir).append("/");
      if (! is_dir(dirSoFar.c_str()))
         mkdir(dirSoFar.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    return true;
   }

   void deldirs(const char* dirpath, int maxDepth)
   //---------------------------------------------
   {
      if ( (maxDepth < 0) || (! is_dir(dirpath)) ) return;
      struct D { void operator()(DIR* p) const { if (p) closedir(p); }; };
      D d;
      std::unique_ptr<DIR, D> dir(opendir(dirpath), d);
      if (! dir)
         return;
      struct dirent* dp;
      while ((dp = readdir(dir.get())) != nullptr)
      {
         std::string name(dp->d_name), path = std::string(dirpath) + "/" + name;
         if ( (name == ".") || (name == "..") )
            continue;
         bool isDir;
         if ( (dp->d_type == DT_UNKNOWN) || (dp->d_type == DT_LNK) )
         {
            struct stat stbuf;
            if (stat(path.c_str(), &stbuf) < 0)
               continue;
            isDir = S_ISDIR(stbuf.st_mode);
         }
         else
            isDir = (dp->d_type == DT_DIR);
         if (isDir)
            deldirs(path.c_str(), maxDepth-1);
         else
            unlink(path.c_str());
      }
      rmdir(dirpath);
   }

#ifdef HAVE_LIBZIP
   bool unzip(const char* zipfile, std::string dir)
   //----------------------------------------------
   {
      Log logger("ut::unzip");
      int err = 0;
      struct Dz { void operator()(zip* p) const { if (p) zip_close(p); }; };
      Dz dz;
      std::unique_ptr<zip, Dz> z(zip_open("foo.zip", ZIP_RDONLY, &err), dz);
      if (! z)
      {
         logger.error("Error opening zip {0}", zipfile);
         return false;
      }
      zip_int64_t no = zip_get_num_entries(z.get(), 0);
      if (no <= 0)
      {
         logger.error("Error or no entries enumerating zip {0}", zipfile);
         return false;
      }
      if ( (! is_dir(dir.c_str())) && (! mkdirs(dir.c_str())) )
      {
         logger.error("Directory {0} does not exist and could not be created.", dir.c_str());
         return false;
      }

      char buf[4096];
      struct zip_stat stat;
      for (zip_int64_t i = 0; i < no; i++)
      {
         if (zip_stat_index(z.get(), (zip_uint64_t) i, 0, &stat) < 0)
         {
            logger.error("Error getting entry {0} from zip file {1}", i, zipfile);
            return false;
         }
         std::string name = std::string(stat.name);
         std::string dest = dir;
         dest.append("/").append(name);
         if (name[name.length() - 1] == '/')
            mkdirs(dest.c_str());
         else
         {
            struct Dzf { void operator()(zip_file* p) const { if (p) zip_fclose(p); }; };
            Dzf dzf;
            std::unique_ptr<zip_file, Dzf> zf(zip_fopen_index(z.get(), (zip_uint64_t) i, 0), dzf);
            if (!zf)
            {
               logger.error("Error opening zipped file {0}/{1}", zipfile, name.c_str());
               return false;
            }
            std::ofstream ofs(dest.c_str(), std::ios::binary | std::ios::out);
            if (! ofs)
            {
               logger.error("Error opening zip extraction file {0}", dest.c_str());
               return false;
            }
            zip_int64_t sum = 0;
            while (sum < stat.size)
            {
               zip_int64_t len = zip_fread(zf.get(), buf, 4096);
               if (len < 0)
               {
                  logger.error("Error: Reading zipped file {0}/{1}", zipfile, name.c_str());
                  ofs.close();
                  unlink(dest.c_str());
                  return false;
               }
               ofs.write(buf, len);
               sum += len;
            }
         }
      }
      return true;
   }
#endif


}