#include "ConfigReadManager.h"
#include "LoadTextFile.h"
#include "Debug.h"

void UConfigReadManager::Initialize()
{
	LoadConfigFromIni();
}

void UConfigReadManager::LoadConfigFromIni()
{
	try
	{
		ConfigKeys = TextFile::ReadPairs(".//Config.ini");
	}
	catch (const std::exception& e)
	{
		// 변환 실패 시 기본값 유지 (필요 시 로깅 추가 가능)
		LOG_ERROR("parsing value: %s\n", e.what());
	}
	
}
