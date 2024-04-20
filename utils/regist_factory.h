#pragma once

#include <functional>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>

template <typename T, typename... Args>
auto shared_creator() {
  auto f = [](Args... args){
    auto p = std::make_shared<T>(args...);
    return p;
  };
  return f;
}

template <typename T, typename... Args>
class PluginRegistry {
  public:
    typedef std::function<std::shared_ptr<T>(Args...)> FactoryFunction;
    typedef std::unordered_map<std::string, FactoryFunction> FactoryMap;

    static bool add(const std::string& name, FactoryFunction fac) {
      auto& map = getFactoryMap();
      if (map.find(name) != map.end()) {
        spdlog::error("Target plugin has exists!");
        return false;
      }

      map[name] = fac;
      spdlog::info("Regist plugin: {}", name);
      return true;
    }

    static std::shared_ptr<T> create(const std::string& name, Args... args) {
      auto& map = getFactoryMap();
      if (map.find(name) == map.end()) {
        return nullptr;
      }

      return map[name](args...);
    }

    static const FactoryMap& getFinalFactoryMap() {
      return getFactoryMap();
    }

  private:
    static FactoryMap& getFactoryMap() {
      static FactoryMap map;
      return map;
    }
};

#define REGISTER_PLUGIN(base_type, derive_type, plugin_name, ...) \
  PluginRegistry<base_type, __VA_ARGS__>::add(#plugin_name, (shared_creator<derive_type, __VA_ARGS__>()));
