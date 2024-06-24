// PluginManager.h
#pragma once

#include <vector>
#include <string>
#include <memory>
#include "Plugin.h"

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #define DYNLIB_HANDLE HINSTANCE
    #define DYNLIB_LOAD LoadLibraryA
    #define DYNLIB_GETSYM GetProcAddress
    #define DYNLIB_UNLOAD FreeLibrary
#else
    #include <dlfcn.h>
    #define DYNLIB_HANDLE void*
    #define DYNLIB_LOAD(path) dlopen(path, RTLD_LAZY)
    #define DYNLIB_GETSYM dlsym
    #define DYNLIB_UNLOAD dlclose
#endif

class PluginManager {
public:
    static PluginManager& getInstance() {
        static PluginManager instance;
        return instance;
    }

    void LoadPlugin(const std::string& path);
    void UnloadPlugin(const std::string& path);
    void InitializePlugins();
    void UpdatePlugins(float TimeSinceLastFrame);
    void ConstantUpdatePlugins(float TimeSinceLastFrame);
    void ShutdownPlugins();

private:
    PluginManager() = default;
    ~PluginManager() = default;
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    std::vector<std::unique_ptr<Plugin>> plugins;
    std::vector<DYNLIB_HANDLE> pluginHandles;
};

