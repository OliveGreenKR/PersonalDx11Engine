#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>

//only valuable in this header
namespace
{
    // 문자열에서 공백 제거 (좌우 트림)
    std::string trim(const std::string& str)
    {
        std::string s = str;
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), s.end());
        return s;
    }

}
namespace INI
{

    // INI 파일에서 섹션 데이터를 읽는 범용 함수
    std::unordered_map<std::string, std::string> ReadIniSection(const std::string& filePath, const std::string& sectionName)
    {
        std::unordered_map<std::string, std::string> keyValuePairs;

        std::ifstream file(filePath);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open *.ini file: " + filePath);
        }

        std::string line;
        bool inTargetSection = false;

        while (std::getline(file, line))
        {
            line = trim(line);

            // 빈 줄이나 주석 무시
            if (line.empty() || line[0] == ';' || line[0] == '#')
                continue;

            // 섹션 확인
            if (line.front() == '[' && line.back() == ']')
            {
                std::string currentSection = trim(line.substr(1, line.size() - 2));
                inTargetSection = (currentSection == sectionName);
                continue;
            }

            // 목표 섹션이 아닌 경우 건너뜀
            if (!inTargetSection)
                continue;

            // 키-값 쌍 파싱
            std::istringstream iss(line);
            std::string key, value;
            if (std::getline(iss, key, '=') && std::getline(iss, value))
            {
                key = trim(key);
                value = trim(value);
                keyValuePairs[key] = value;
            }
        }

        file.close();
        return keyValuePairs;
    }
}




