#ifndef BULB_ASSETREADER_HH_
#define BULB_ASSETREADER_HH_

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>

#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#else
#include <dirent.h>
#endif
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "bulb/ut.hh"

namespace bulb
{
   class AssetReader
   //================
   {
   public:
      AssetReader(AssetReader const&) = delete;
      AssetReader(AssetReader&&) = delete;
      AssetReader& operator=(AssetReader const&) = delete;
      AssetReader& operator=(AssetReader &&) = delete;
      static AssetReader& instance()
      //--------------------------------
      {
         static AssetReader the_instance;
         return the_instance;
      }
#ifdef __ANDROID__
      void set_manager(AAssetManager *pManager) { androidAssetManager = pManager; }

      bool is_asset_dir(const char* assetdir)
      //-------------------------------------
      {
         if (androidAssetManager == nullptr)
            return false;
         std::string assetDir(assetdir);
         if (assetDir.find("assets/") == 0)
            assetDir = "/" + assetDir.substr(7);
         struct D { void operator()(AAssetDir* p) const { if (p) AAssetDir_close(p); }; };
         D d;
         std::unique_ptr<AAssetDir, D> dir(AAssetManager_openDir(androidAssetManager, assetDir.c_str()), d);
         if (dir)
            return true;
         return false;
      }

      bool exists(const char *assetname)
      //--------------------------------
      {
         if (androidAssetManager == nullptr)
         return false;
         std::string assetName(assetname);
         if (assetName.find("assets/") == 0)
            assetName = "/" + assetName.substr(7);
         AAsset* ass = AAssetManager_open(androidAssetManager, assetName.c_str(), AASSET_MODE_BUFFER);
         if (ass == nullptr)
            return false;
         AAsset_close(ass);
         return true;
      }

   template<typename T>
   bool read_asset_vector(const char* assetname, std::vector<T>& v)
//-----------------------------------------------------------------------------------
   {
      if (androidAssetManager == nullptr)
         return false;
      std::string assetName(assetname);
      if (assetName.find("assets/") == 0)
         assetName = "/" + assetName.substr(7);
      struct D { void operator()(AAsset* p) const { if (p) AAsset_close(p); }; };
      D d;
      AAsset* ass = AAssetManager_open(androidAssetManager, assetName.c_str(), AASSET_MODE_BUFFER);
      if (ass == nullptr)
      {
         v.clear();
         return false;
      }
      std::unique_ptr<AAsset, D> asset_p(ass, d);
//   std::unique_ptr<AAsset, D> asset_p(AAssetManager_open(asset_manager, assetName.c_str(), AASSET_MODE_BUFFER), d);
      if (! asset_p)
      {
         v.clear();
         return false;
      }
      size_t len = AAsset_getLength(asset_p.get());
      v.resize(len);
      int rlen = AAsset_read(asset_p.get(), v.data(), len);
      if (rlen < len)
      {
         v.clear();
         return false;
      }
      return true;
   }

   bool read_asset_string(const char* assetname, std::string& s)
   //-----------------------------------------------------------
   {
      if (androidAssetManager == nullptr)
      {
         s.clear();
         return false;
      }
      std::string assetName(assetname);
      if (assetName.find("assets/") == 0)
         assetName = "/" + assetName.substr(7);
      struct D { void operator()(AAsset* p) const { if (p) AAsset_close(p); }; };
      D d;
      std::unique_ptr<AAsset, D> asset_p(AAssetManager_open(androidAssetManager, assetName.c_str(), AASSET_MODE_BUFFER), d);
      if (! asset_p)
      {
         s.clear();
         return false;
      }
      size_t len = AAsset_getLength(asset_p.get());
      std::unique_ptr<char[]> pch(new char[len+1]);
      int rlen = AAsset_read(asset_p.get(), pch.get(), len);
      if (rlen < len)
      {
         s.clear();
         return false;
      }
      pch[len] = 0;
      s = static_cast<const char *>(pch.get());
      return true;
   }

   size_t asset_length(const char* assetname)
   //----------------------------------------
   {
      if (androidAssetManager == nullptr)
         return false;
      std::string assetName(assetname);
      if (assetName.find("assets/") == 0)
         assetName = "/" + assetName.substr(7);
      struct D { void operator()(AAsset* p) const { if (p) AAsset_close(p); }; };
      D d;
      std::unique_ptr<AAsset, D> asset_p(AAssetManager_open(androidAssetManager, assetName.c_str(), AASSET_MODE_BUFFER), d);
      if (! asset_p)
         return false;
      return AAsset_getLength(asset_p.get());
   }

   bool copy_assets_to(const char *assetdir, const char *sdcardDir, bool isRecurse=false)
   //-------------------------------------------------------------------------------------
   {
      if (androidAssetManager == nullptr)
         return false;
      std::string assetDir(assetdir);
      if (assetDir.find("assets/") == 0)
         assetDir = "/" + assetDir.substr(7);
      struct D { void operator()(AAssetDir* p) const { if (p) AAssetDir_close(p); }; };
      D d;
      std::unique_ptr<AAssetDir, D> dir(AAssetManager_openDir(androidAssetManager, assetDir.c_str()), d);
      if (! dir)
         return false;
      struct stat sb;
      if (stat(sdcardDir, &sb))
         return false;
      if (! S_ISDIR(sb.st_mode))
         return false;
      const char* assetName = AAssetDir_getNextFileName(dir.get());
      unsigned char buf[8192];
      while (assetName != nullptr)
      {
         struct DD { void operator()(AAsset* p) const { if (p) AAsset_close(p); }; };
         DD dd;
         std::unique_ptr<AAsset, DD> asset(AAssetManager_open(androidAssetManager, assetName, AASSET_MODE_STREAMING), dd);
         int count = AAsset_read(asset.get(), buf, 8192);
         std::string filename(assetName);
         auto psl = filename.rfind('/');
         if (psl != std::string::npos)
            filename = filename.substr(psl+1);
         std::string filepath = std::string(sdcardDir) + "/" + filename;
         std::ofstream ofs(filepath);
         if ( (count > 0) && (ofs) )
         {
            while (count > 0)
            {
               ofs.write(reinterpret_cast<const char*>(buf), count);
               count = AAsset_read(asset.get(), buf, 8192);
            }
         }
         assetName = AAssetDir_getNextFileName(dir.get());
      }
      return true;
   }

   private:
      AAssetManager *androidAssetManager;
#else
      bool is_asset_dir(const char* assetdir) { return is_dir(assetdir); }

      bool exists(const char *assetname) { std::ifstream ifs(assetname); return ifs.good(); }

      template<typename T>
      bool read_asset_vector(const char* assetname, std::vector<T>& v)
      //-----------------------------------------------------------------------------------
      {
         v.clear();
         std::ifstream ifs(assetname);
         if (!ifs.good())
            return false;
         ifs.seekg(0, ifs.end);
         size_t len = ifs.tellg();
         ifs.seekg(0, ifs.beg);
         std::unique_ptr<char[]> contents(new char[len+1]);
         ifs.read(contents.get(), len);
         if (!ifs.good())
            return false;
         for (int i=0; i<len; i++)
         {
            T x = static_cast<T>(contents[i]);
            v.emplace_back(x);
         }
         return true;
      }

      bool read_asset_string(const char* assetname, std::string& s)
      //-----------------------------------------------------------
      {
         std::ifstream ifs(assetname);
         if (!ifs.good())
            return false;
         std::string ss((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
         if (!ifs.good())
            return false;
         s = std::move(ss);
         return true;
      }

      size_t asset_length(const char* assetname)
      //----------------------------------------
      {
         std::ifstream in(assetname, std::ifstream::ate | std::ifstream::binary);
         return in.tellg();
      }

      bool copy_assets_to(const char *assetdir, const char *sdcardDir, bool isRecurse=false)
      //-------------------------------------------------------------------------------------
      {
         std::string assetDir(assetdir);
         std::string toDir(sdcardDir);
         if ( (! is_asset_dir(assetdir)) || (! is_asset_dir(sdcardDir)) ) return false;
         struct D { void operator()(DIR* p) const { if (p) closedir(p); }; };
         D d;
         std::unique_ptr<DIR, D> dir(opendir(assetdir), d);
         if (! dir)
            return false;

         char buf[8192];
         struct dirent* dp;
         while ((dp = readdir(dir.get())) != nullptr)
         {
            std::string filename(dp->d_name);
            if ( (filename == ".") || (filename == "..") )
               continue;
            std::ifstream in(assetDir + "/" + filename, std::ios_base::in | std::ios_base::binary);
            std::ofstream out(toDir + "/" + filename, std::ios_base::out | std::ios_base::binary);
            do
            {
               in.read(&buf[0], 8192);
               out.write(&buf[0], in.gcount());
            } while (in.gcount() > 0);
            in.close(); out.close();
         }
         return true;
      }
#endif
   private:
      AssetReader() = default;
   };
}
#endif
