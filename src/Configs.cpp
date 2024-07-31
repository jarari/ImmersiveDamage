#include "Configs.h"
#include "SimpleIni.h"
#include <fstream>

bool Configs::enableMod = false;
bool Configs::enableCrit = false;
bool Configs::enableSneak = false;
bool Configs::enableVATS = false;
bool Configs::enableLog = false;
float Configs::playerDamageIncomingMult = 1.f;
float Configs::playerDamageOutgoingMult = 1.f;
float Configs::npcDamageMult = 1.f;

void Configs::LoadConfigs()
{
	std::string path = "Data\\MCM\\Config\\ImmersiveDamage\\settings.ini";
	if (std::filesystem::exists("Data\\MCM\\Settings\\ImmersiveDamage.ini")) {
		path = "Data\\MCM\\Settings\\ImmersiveDamage.ini";
	}
	CSimpleIniA ini(true, false, false);
	SI_Error result = ini.LoadFile(path.c_str());
	if (result >= 0) {
		enableMod = std::stoi(ini.GetValue("Main", "bEnableMod", "1")) > 0;
		enableCrit = std::stoi(ini.GetValue("Main", "bEnableCrit", "1")) > 0;
		enableSneak = std::stoi(ini.GetValue("Main", "bEnableSneak", "1")) > 0;
		enableVATS = std::stoi(ini.GetValue("Main", "bEnableVATS", "1")) > 0;
		enableLog = std::stoi(ini.GetValue("Main", "bEnableLog", "0")) > 0;
		playerDamageIncomingMult = std::stof(ini.GetValue("Damage", "fPlayerDamageIncomingMult", "1.0"));
		playerDamageOutgoingMult = std::stof(ini.GetValue("Damage", "fPlayerDamageOutgoingMult", "1.0"));
		npcDamageMult = std::stof(ini.GetValue("Damage", "fNPCDamageMult", "1.0"));
	}
	ini.Reset();
}
