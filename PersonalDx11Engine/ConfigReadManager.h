#pragma once
#include <unordered_map>
#include <string>
#include "TypeCast.h"
#include "Debug.h"
class UConfigReadManager
{
private:
    // 복사 및 이동 방지
    UConfigReadManager(const UConfigReadManager&) = delete;
    UConfigReadManager& operator=(const UConfigReadManager&) = delete;
    UConfigReadManager(UConfigReadManager&&) = delete;
    UConfigReadManager& operator=(UConfigReadManager&&) = delete;

    // 생성자/소멸자
    UConfigReadManager() = default;
    ~UConfigReadManager() = default;

public:
    static UConfigReadManager* Get()
    {
        // 포인터를 static으로 선언
        static UConfigReadManager* instance = []() {
            UConfigReadManager* manager = new UConfigReadManager();
            manager->Initialize();
            return manager;
            }();

        return instance;
    }

public:
    template<typename T>
    void GetValue(const char* key, T& OutValue)
    {
        if (auto it = ConfigKeys.find(key); it != ConfigKeys.end())
        {
            if (Engine::CastFromString(it->second, OutValue))
            {
                LOG_INFO("Config Loaded [%s] = %s ", key, it->second.c_str());
            }
            else
            {
                LOG_WARNING("Cannot cast [%s] to the requested type", key);
            }
        }
        else
        {
            LOG_WARNING("Cannot find [%s] in Configs", key);
        }
        return;
    }


    void LoadConfigFromIni();

private:
    void Initialize();
    

private:
    std::unordered_map<std::string, std::string> ConfigKeys;
};