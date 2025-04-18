#pragma once
#include <memory>
#include <type_traits>
#include <string>
#include <sstream>
#include <stdexcept>

namespace Engine
{
    template<typename To, typename From>
    To* Cast(const From* ptr) 
    {
        //상속관계 확인
        static_assert(std::is_base_of_v<To, From> || std::is_base_of_v<From, To>,
                      "Cast Failed - Only Castable in Inheritance");
        //Upcast
        if constexpr (std::is_base_of_v<To, From>) 
        {
            return static_cast<To*>(const_cast<From*>(ptr));
        }
        else 
        {
            return dynamic_cast<To*>(const_cast<From*>(ptr));
        }
    }

    // shared_ptr 특수화
    template<typename To, typename From>
    std::shared_ptr<To> Cast(const std::shared_ptr<From>& ptr) {
        static_assert(std::is_base_of_v<To, From> || std::is_base_of_v<From, To>,
                      "Cast Failed - Only Castable in Inheritance");

        if constexpr (std::is_base_of_v<To, From>) {
            return std::static_pointer_cast<To>(ptr);
        }
        else {
            return std::dynamic_pointer_cast<To>(ptr);
        }
    }

    // weak_ptr 특수화
    template<typename To, typename From>
    std::weak_ptr<To> Cast(const std::weak_ptr<From>& ptr) {
        static_assert(std::is_base_of_v<To, From> || std::is_base_of_v<From, To>,
                      "Cast Failed - Only Castable in Inheritance");

        if (auto locked = ptr.lock()) {
            if constexpr (std::is_base_of_v<To, From>) {
                return std::static_pointer_cast<To>(locked);
            }
            else {
                return std::dynamic_pointer_cast<To>(locked);
            }
        }
        return std::weak_ptr<To>();
    }

    template<typename T>
    bool CastFromString(const std::string& str, T& OutValue) {
        std::stringstream ss(str);
        ss >> OutValue;

        // 입력이 완전히 변환되지 않았거나 실패한 경우
        if (ss.fail() || !ss.eof()) {
            return false;
        }

        return true;
    }
}
