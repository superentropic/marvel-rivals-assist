#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include "../../ThirdParty/nlohmann/json.hpp"
#include "../../global.h"

namespace ConfigSystemFS = std::filesystem;

namespace ConfigSystem {

    inline std::string GetConfigDir() {
        char path[MAX_PATH];
        if (GetModuleFileNameA(NULL, path, MAX_PATH)) {
            std::string dir = ConfigSystemFS::path(path).parent_path().string();
            std::string configDir = dir + "\\configs";
            if (!ConfigSystemFS::exists(configDir)) ConfigSystemFS::create_directories(configDir);
            return configDir;
        }
        return ".\\configs";
    }

    inline ImColor JsonToImColor(const json& j) {
        return ImColor(j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j.size() > 3 ? j[3].get<float>() : 1.0f);
    }

    inline json ImColorToJson(const ImColor& c) {
        return json::array({ c.Value.x, c.Value.y, c.Value.z, c.Value.w });
    }

    inline json ImVec2ToJson(const ImVec2& v) {
        return json::array({ v.x, v.y });
    }

    inline ImVec2 JsonToImVec2(const json& j) {
        return ImVec2(j[0].get<float>(), j[1].get<float>());
    }

    inline json SerializeConfig() {
        json cfg;

        // Aimbot
        cfg["aimbot"]["enabled"] = mods::aimbot;
        cfg["aimbot"]["smoothing"] = mods::smoothing;
        cfg["aimbot"]["fov"] = mods::fov;
        cfg["aimbot"]["fovCircle"] = mods::aimbotFovCircle;
        cfg["aimbot"]["visCheck"] = mods::VisCheck;
        cfg["aimbot"]["teamCheck"] = mods::bAimbotTeamCheck;
        cfg["aimbot"]["hitbox"] = mods::aimHitbox;
        cfg["aimbot"]["boneHead"] = mods::bBoneHead;
        cfg["aimbot"]["boneNeck"] = mods::bBoneNeck;
        cfg["aimbot"]["boneChest"] = mods::bBoneChest;
        cfg["aimbot"]["bonePelvis"] = mods::bBonePelvis;
        cfg["aimbot"]["boneLeftHand"] = mods::bBoneLeftHand;
        cfg["aimbot"]["boneRightHand"] = mods::bBoneRightHand;
        cfg["aimbot"]["aimOffset"] = mods::aimOffset;
        cfg["aimbot"]["prediction"] = mods::bAimPrediction;
        cfg["aimbot"]["projectileSpeed"] = mods::projectileSpeed;
        cfg["aimbot"]["humanizer"] = mods::bAimHumanizer;
        cfg["aimbot"]["humanizerLevel"] = mods::humanizerLevel;
        cfg["aimbot"]["snapLine"] = mods::bAimbotSnapLine;
        cfg["aimbot"]["snapLineColor"] = ImColorToJson(mods::snapLineColor);
        cfg["aimbot"]["snapLineThickness"] = mods::snapLineThickness;
        cfg["aimbot"]["maxDistance"] = mods::aimbotMaxDistance;
        cfg["aimbot"]["priority"] = (int)mods::aimbotPriority;
        cfg["aimbot"]["bulletTP"] = mods::bulletTP;
        cfg["aimbot"]["key"] = mods::aimbotKey;
        cfg["aimbot"]["keyName"] = mods::aimbotKeyName;
        cfg["aimbot"]["fovCircleColor"] = ImColorToJson(mods::aimbotFovCircleColor);
        cfg["aimbot"]["dynamicFOV"] = mods::bDynamicFOV;
        cfg["aimbot"]["minFovMul"] = mods::minFovMultiplier;
        cfg["aimbot"]["maxFovMul"] = mods::maxFovMultiplier;
        cfg["aimbot"]["customTimeDilation"] = mods::CustomTimeDilationBool;
        cfg["aimbot"]["customTimeDilationVal"] = mods::CustomTimeDilationFloat;

        // ESP
        cfg["esp"]["enabled"] = mods::esp;
        cfg["esp"]["box"] = mods::bESPBox;
        cfg["esp"]["boxType"] = (int)mods::espBoxType;
        cfg["esp"]["boxThickness"] = mods::espBoxThickness;
        cfg["esp"]["boxOutline"] = mods::bESPBoxOutline;
        cfg["esp"]["boxOutlineColor"] = ImColorToJson(mods::espBoxOutlineColor);
        cfg["esp"]["healthBar"] = mods::bHealthBar;
        cfg["esp"]["ultPercentage"] = mods::bUltimatePercentage;
        cfg["esp"]["skeleton"] = mods::bSkeletonESP;
        cfg["esp"]["tracerLines"] = mods::bTracerLines;
        cfg["esp"]["tracerStartPos"] = (int)mods::tracerStartPos;
        cfg["esp"]["showHeroNames"] = mods::bShowHeroNames;
        cfg["esp"]["showDistance"] = mods::bShowDistance;
        cfg["esp"]["showHealthText"] = mods::bShowHealthText;
        cfg["esp"]["showUltText"] = mods::bShowUltPercentageText;
        cfg["esp"]["teamCheck"] = mods::bESPTeamCheck;
        cfg["esp"]["glow"] = mods::bGlow;
        cfg["esp"]["maxDistance"] = mods::espMaxDistance;
        cfg["esp"]["healthBarPos"] = (int)mods::healthBarPosition;
        cfg["esp"]["ultBarPos"] = (int)mods::ultBarPosition;
        cfg["esp"]["distancePos"] = (int)mods::distancePosition;
        cfg["esp"]["heroNamePos"] = (int)mods::heroNamePosition;

        // Colors
        cfg["colors"]["visible"] = ImColorToJson(mods::visibleColor);
        cfg["colors"]["nonVisible"] = ImColorToJson(mods::nonVisibleColor);
        cfg["colors"]["skeleton"] = ImColorToJson(mods::skeletonESPColor);
        cfg["colors"]["tracer"] = ImColorToJson(mods::tracerColor);
        cfg["colors"]["healthHigh"] = ImColorToJson(mods::healthHighColor);
        cfg["colors"]["healthMid"] = ImColorToJson(mods::healthMidColor);
        cfg["colors"]["healthLow"] = ImColorToJson(mods::healthLowColor);
        cfg["colors"]["ultBar"] = ImColorToJson(mods::ultBarColor);
        cfg["colors"]["healthBarOutline"] = ImColorToJson(mods::healthBarOutlineColor);
        cfg["colors"]["ultBarOutline"] = ImColorToJson(mods::ultBarOutlineColor);
        cfg["colors"]["barBg"] = ImColorToJson(mods::barBackgroundColor);
        cfg["colors"]["distanceText"] = ImColorToJson(mods::distanceTextColor);
        cfg["colors"]["distanceOutline"] = ImColorToJson(mods::distanceTextOutlineColor);
        cfg["colors"]["heroNameText"] = ImColorToJson(mods::heroNameTextColor);
        cfg["colors"]["heroNameOutline"] = ImColorToJson(mods::heroNameTextOutlineColor);
        cfg["colors"]["heroNameBg"] = ImColorToJson(mods::heroNameBgColor);
        cfg["colors"]["healthText"] = ImColorToJson(mods::healthTextColor);
        cfg["colors"]["healthTextOutline"] = ImColorToJson(mods::healthTextOutlineColor);
        cfg["colors"]["ultText"] = ImColorToJson(mods::ultTextColor);
        cfg["colors"]["ultTextOutline"] = ImColorToJson(mods::ultTextOutlineColor);
        cfg["colors"]["crosshair"] = ImColorToJson(mods::crosshairColor);

        // Crosshair
        cfg["crosshair"]["type"] = (int)mods::crosshairType;
        cfg["crosshair"]["size"] = mods::crosshairSize;
        cfg["crosshair"]["thickness"] = mods::crosshairThickness;
        cfg["crosshair"]["gap"] = mods::crosshairGap;
        cfg["crosshair"]["outline"] = mods::bCrosshairOutline;

        // Misc
        cfg["misc"]["fovChanger"] = mods::fov_changer;
        cfg["misc"]["fovAmount"] = mods::fov_changer_amount;
        cfg["misc"]["spinbot"] = mods::bSpinbot;
        cfg["misc"]["spinbotX"] = mods::bSpinbotX;
        cfg["misc"]["spinbotY"] = mods::bSpinbotY;
        cfg["misc"]["spinbotZ"] = mods::bSpinbotZ;
        cfg["misc"]["spinSpeedX"] = mods::SpiningSpeedX;
        cfg["misc"]["spinSpeedY"] = mods::SpiningSpeedY;
        cfg["misc"]["spinSpeedZ"] = mods::SpiningSpeedZ;
        cfg["misc"]["smallPerson"] = mods::Experimental::SmallPerson;
        cfg["misc"]["smallScale"] = mods::Experimental::SmallPersonScale;
        cfg["misc"]["hideLocal"] = mods::Experimental::HideLocalPlayer;
        cfg["misc"]["selfTimeDilation"] = mods::SelfCustomTimeDilationBool;
        cfg["misc"]["selfTimeDilationVal"] = mods::SelfCustomTimeDilationFloat;
        cfg["misc"]["rapidFire"] = mods::bRapidFire;
        cfg["misc"]["rapidFireRate"] = mods::rapidFireRate;
        cfg["misc"]["enemyOverlay"] = mods::bShowEnemyOverlay;
        cfg["misc"]["localCheck"] = mods::LocalCheck;

        // Radar
        cfg["radar"]["enabled"] = mods::bRadar;
        cfg["radar"]["design"] = (int)mods::radarDesign;
        cfg["radar"]["size"] = mods::radarSize;
        cfg["radar"]["range"] = mods::radarRange;
        cfg["radar"]["zoom"] = mods::radarZoom;
        cfg["radar"]["pos"] = ImVec2ToJson(mods::radarPos);

        // Widgets
        cfg["widgets"]["info"] = mods::bInfoWidget;
        cfg["widgets"]["infoPos"] = ImVec2ToJson(mods::infoWidgetPos);
        cfg["widgets"]["keybind"] = mods::bKeybindWidget;
        cfg["widgets"]["keybindPos"] = ImVec2ToJson(mods::keybindWidgetPos);
        cfg["widgets"]["watermark"] = mods::bWatermark;
        cfg["widgets"]["damageLog"] = mods::bDamageLog;
        cfg["widgets"]["damageLogPos"] = ImVec2ToJson(mods::damageLogPos);

        // Hotkeys
        cfg["hotkeys"]["menu"] = mods::menuToggleKey;
        cfg["hotkeys"]["menuName"] = mods::menuToggleKeyName;
        cfg["hotkeys"]["esp"] = mods::espHotkey;
        cfg["hotkeys"]["espName"] = mods::espHotkeyName;
        cfg["hotkeys"]["glow"] = mods::glowHotkey;
        cfg["hotkeys"]["glowName"] = mods::glowHotkeyName;
        cfg["hotkeys"]["bulletTP"] = mods::bulletTPHotkey;
        cfg["hotkeys"]["bulletTPName"] = mods::bulletTPHotkeyName;
        cfg["hotkeys"]["spinbot"] = mods::spinbotHotkey;
        cfg["hotkeys"]["spinbotName"] = mods::spinbotHotkeyName;
        cfg["hotkeys"]["selfTime"] = mods::SelfTimeHotkey;
        cfg["hotkeys"]["selfTimeName"] = mods::SelfTimekeyName;
        cfg["hotkeys"]["rageKey"] = mods::rageKey;
        cfg["hotkeys"]["rageKeyName"] = mods::rageKeyName;

        // Adaptive FOV
        cfg["adaptiveFov"]["enabled"] = mods::bAdaptiveFov;
        cfg["adaptiveFov"]["closeDist"] = mods::adaptiveCloseDistance;
        cfg["adaptiveFov"]["farDist"] = mods::adaptiveFarDistance;
        cfg["adaptiveFov"]["scaleClose"] = mods::adaptiveFovScaleClose;
        cfg["adaptiveFov"]["scaleFar"] = mods::adaptiveFovScaleFar;
        cfg["adaptiveFov"]["showCircle"] = mods::bShowAdaptiveFov;

        // Rage Mode
        cfg["rage"]["enabled"] = mods::bRageMode;
        cfg["rage"]["confirmation"] = mods::bRageConfirmation;
        cfg["rage"]["separateKey"] = mods::bSeparateRageKey;
        cfg["rage"]["showFovSilent"] = mods::bShowFovSilent;
        cfg["rage"]["silentAim"] = mods::bSilentAim;
        cfg["rage"]["silentHealOnly"] = mods::bSilentHealingOnly;
        cfg["rage"]["silentAdaptive"] = mods::bSilentAdaptiveFov;
        cfg["rage"]["silentMagicFov"] = mods::silentMagicFov;

        // Hitbox Editor
        cfg["hitbox"]["enabled"] = mods::bHitboxEditor;
        cfg["hitbox"]["head"] = mods::hitboxHead;
        cfg["hitbox"]["body"] = mods::hitboxBody;
        cfg["hitbox"]["armsL"] = mods::hitboxArmsL;
        cfg["hitbox"]["armsR"] = mods::hitboxArmsR;
        cfg["hitbox"]["legsL"] = mods::hitboxLegsL;
        cfg["hitbox"]["legsR"] = mods::hitboxLegsR;

        // Filters & Style
        cfg["filters"]["distanceFilter"] = mods::distanceFilter;
        cfg["filters"]["espFontSize"] = mods::espFontSize;
        cfg["filters"]["drawStyle"] = (int)mods::drawStyle;
        cfg["filters"]["textPosition"] = (int)mods::textPosition;
        cfg["filters"]["banPhase"] = mods::bBanPhaseOverlay;

        // Auto Skill
        cfg["autoSkill"]["enabled"] = mods::bAutoSkill;
        cfg["autoSkill"]["autoMelee"] = mods::bAutoMelee;
        cfg["autoSkill"]["autoDetect"] = mods::bAutoDetectHero;
        for (auto& [heroId, skCfg] : mods::heroSkillConfigs) {
            std::string key = std::to_string(heroId);
            cfg["autoSkill"]["heroes"][key]["enabled"] = skCfg.enabled;
            cfg["autoSkill"]["heroes"][key]["useQ"] = skCfg.useQ;
            cfg["autoSkill"]["heroes"][key]["useE"] = skCfg.useE;
            cfg["autoSkill"]["heroes"][key]["useShift"] = skCfg.useShift;
            cfg["autoSkill"]["heroes"][key]["useRMB"] = skCfg.useRMB;
            cfg["autoSkill"]["heroes"][key]["hpThreshold"] = skCfg.hpThreshold;
            cfg["autoSkill"]["heroes"][key]["distThreshold"] = skCfg.distThreshold;
        }

        // Health Pack ESP
        cfg["healthPack"]["enabled"] = mods::bHealthPackESP;
        cfg["healthPack"]["onlyNoCD"] = mods::bShowOnlyNoCDHealthPack;
        cfg["healthPack"]["distance"] = mods::bHealthPackDistance;
        cfg["healthPack"]["snaplines"] = mods::bHealthPackSnaplines;
        cfg["healthPack"]["maxDist"] = mods::healthPackMaxDistance;
        cfg["healthPack"]["fontSize"] = mods::healthPackFontSize;

        // Auto Heal
        cfg["autoHeal"]["enabled"] = mods::bAutoHeal;
        for (auto& [name, ahCfg] : mods::autoHealConfigs) {
            cfg["autoHeal"]["heroes"][name]["enabled"] = ahCfg.enabled;
            cfg["autoHeal"]["heroes"][name]["threshold"] = ahCfg.hpThreshold;
        }

        // New Widgets
        cfg["widgets"]["ultTracker"] = mods::bUltTracker;
        cfg["widgets"]["ultTrackerPos"] = ImVec2ToJson(mods::ultTrackerPos);
        cfg["widgets"]["activeFeatures"] = mods::bActiveFeaturesWidget;
        cfg["widgets"]["activeFeaturesPos"] = ImVec2ToJson(mods::activeFeaturesPos);

        // Skin Changer
        cfg["skinChanger"]["enabled"] = mods::bSkinChanger;
        cfg["skinChanger"]["customEnabled"] = mods::bCustomSkinEnabled;
        for (auto& [hid, skinId] : mods::skinOverrides) {
            cfg["skinChanger"]["overrides"][std::to_string(hid)] = skinId;
        }
        for (auto& [hid, enabled] : mods::skinEnabled) {
            cfg["skinChanger"]["heroEnabled"][std::to_string(hid)] = enabled;
        }
        for (auto& [hid, files] : mods::customSkinFiles) {
            cfg["skinChanger"]["customFiles"][std::to_string(hid)] = files;
        }

        // Chams
        cfg["chams"]["enabled"] = mods::bChams;
        cfg["chams"]["throughWalls"] = mods::bChamsThroughWalls;
        cfg["chams"]["teamCheck"] = mods::bChamsTeamCheck;
        cfg["chams"]["style"] = (int)mods::chamsStyle;
        cfg["chams"]["visibleColor"] = ImColorToJson(mods::chamsVisibleColor);
        cfg["chams"]["notVisibleColor"] = ImColorToJson(mods::chamsNotVisibleColor);
        cfg["chams"]["opacity"] = mods::chamsOpacity;
        cfg["chams"]["outline"] = mods::bChamsOutline;
        cfg["chams"]["outlineColor"] = ImColorToJson(mods::chamsOutlineColor);
        cfg["chams"]["outlineThickness"] = mods::chamsOutlineThickness;
        cfg["chams"]["healthBased"] = mods::bChamsHealthBased;
        cfg["chams"]["glowIntensity"] = mods::chamsGlowIntensity;
        cfg["chams"]["selfEnabled"] = mods::bSelfChams;
        cfg["chams"]["selfStyle"] = (int)mods::selfChamsStyle;
        cfg["chams"]["selfColor"] = ImColorToJson(mods::selfChamsColor);
        cfg["chams"]["selfOpacity"] = mods::selfChamsOpacity;
        cfg["chams"]["selfGlowIntensity"] = mods::selfChamsGlowIntensity;
        cfg["chams"]["selfWireframe"] = mods::bSelfWireframe;

        // Healer Area
        cfg["healer"]["mode"] = mods::bHealerMode;
        cfg["healer"]["autoHealTeammates"] = mods::bAutoHealTeammates;
        cfg["healer"]["priorityLowest"] = mods::bHealPriorityLowest;
        cfg["healer"]["threshold"] = mods::healThresholdPercent;
        cfg["healer"]["teammateESP"] = mods::bTeammateESP;
        cfg["healer"]["teammateHP"] = mods::bTeammateHealthBars;
        cfg["healer"]["rangeIndicator"] = mods::bHealRangeIndicator;
        cfg["healer"]["range"] = mods::healRange;
        cfg["healer"]["smartTarget"] = mods::bSmartHealTarget;
        cfg["healer"]["losCheck"] = mods::bHealerLOSCheck;
        cfg["healer"]["autoUltHeal"] = mods::bAutoUltHeal;
        cfg["healer"]["ultHealThreshold"] = mods::autoUltHealThreshold;
        cfg["healer"]["autoShield"] = mods::bHealerAutoShield;
        cfg["healer"]["shieldThreshold"] = mods::autoShieldThreshold;
        cfg["healer"]["notifyLowHP"] = mods::bHealerNotifyLowHP;
        cfg["healer"]["notifyThreshold"] = mods::healerNotifyThreshold;
        cfg["healer"]["dashboard"] = mods::bHealerDashboard;
        cfg["healer"]["dashboardPos"] = ImVec2ToJson(mods::healerDashboardPos);
        cfg["healer"]["teammateColor"] = ImColorToJson(mods::teammateColor);
        cfg["healer"]["healableColor"] = ImColorToJson(mods::teammateHealableColor);
        cfg["healer"]["criticalColor"] = ImColorToJson(mods::teammateCriticalColor);

        // Auto Shield
        for (auto& [name, asCfg] : mods::autoShieldConfigs) {
            cfg["autoShield"][name]["enabled"] = asCfg.enabled;
            cfg["autoShield"][name]["threshold"] = asCfg.hpThreshold;
        }

        // Anti-Detection
        cfg["antiDetection"]["enabled"] = mods::bAntiDetection;
        cfg["antiDetection"]["inputJitter"] = mods::bInputJitter;
        cfg["antiDetection"]["inputJitterMs"] = mods::inputJitterMs;
        cfg["antiDetection"]["timingRandomization"] = mods::bTimingRandomization;
        cfg["antiDetection"]["timingVariance"] = mods::timingVariance;
        cfg["antiDetection"]["memoryCloaking"] = mods::bMemoryCloaking;
        cfg["antiDetection"]["threadHiding"] = mods::bThreadHiding;
        cfg["antiDetection"]["antiScreenshot"] = mods::bAntiScreenshot;
        cfg["antiDetection"]["spreadActions"] = mods::bSpreadActions;
        cfg["antiDetection"]["actionSpreadMs"] = mods::actionSpreadMs;
        cfg["antiDetection"]["humanizedMouse"] = mods::bHumanizedMouse;
        cfg["antiDetection"]["mouseHumanizeStrength"] = mods::mouseHumanizeStrength;

        return cfg;
    }

    inline void DeserializeConfig(const json& cfg) {
        auto get = [](const json& j, const std::string& key, auto& target) {
            try { if (j.contains(key)) target = j[key].get<std::remove_reference_t<decltype(target)>>(); }
            catch (...) {}
        };
        auto getColor = [](const json& j, const std::string& key, ImColor& target) {
            try { if (j.contains(key)) target = JsonToImColor(j[key]); }
            catch (...) {}
        };
        auto getVec2 = [](const json& j, const std::string& key, ImVec2& target) {
            try { if (j.contains(key)) target = JsonToImVec2(j[key]); }
            catch (...) {}
        };

        if (cfg.contains("aimbot")) {
            auto& a = cfg["aimbot"];
            get(a, "enabled", mods::aimbot);
            get(a, "smoothing", mods::smoothing);
            get(a, "fov", mods::fov);
            get(a, "fovCircle", mods::aimbotFovCircle);
            get(a, "visCheck", mods::VisCheck);
            get(a, "teamCheck", mods::bAimbotTeamCheck);
            get(a, "hitbox", mods::aimHitbox);
            get(a, "boneHead", mods::bBoneHead);
            get(a, "boneNeck", mods::bBoneNeck);
            get(a, "boneChest", mods::bBoneChest);
            get(a, "bonePelvis", mods::bBonePelvis);
            get(a, "boneLeftHand", mods::bBoneLeftHand);
            get(a, "boneRightHand", mods::bBoneRightHand);
            get(a, "aimOffset", mods::aimOffset);
            get(a, "prediction", mods::bAimPrediction);
            get(a, "projectileSpeed", mods::projectileSpeed);
            get(a, "humanizer", mods::bAimHumanizer);
            get(a, "humanizerLevel", mods::humanizerLevel);
            get(a, "snapLine", mods::bAimbotSnapLine);
            getColor(a, "snapLineColor", mods::snapLineColor);
            get(a, "snapLineThickness", mods::snapLineThickness);
            get(a, "maxDistance", mods::aimbotMaxDistance);
            if (a.contains("priority")) mods::aimbotPriority = (mods::TargetingPriority)a["priority"].get<int>();
            get(a, "bulletTP", mods::bulletTP);
            get(a, "key", mods::aimbotKey);
            get(a, "keyName", mods::aimbotKeyName);
            getColor(a, "fovCircleColor", mods::aimbotFovCircleColor);
            get(a, "dynamicFOV", mods::bDynamicFOV);
            get(a, "minFovMul", mods::minFovMultiplier);
            get(a, "maxFovMul", mods::maxFovMultiplier);
            get(a, "customTimeDilation", mods::CustomTimeDilationBool);
            get(a, "customTimeDilationVal", mods::CustomTimeDilationFloat);
        }
        if (cfg.contains("esp")) {
            auto& e = cfg["esp"];
            get(e, "enabled", mods::esp);
            get(e, "box", mods::bESPBox);
            if (e.contains("boxType")) mods::espBoxType = (mods::ESPBoxType)e["boxType"].get<int>();
            get(e, "boxThickness", mods::espBoxThickness);
            get(e, "boxOutline", mods::bESPBoxOutline);
            getColor(e, "boxOutlineColor", mods::espBoxOutlineColor);
            get(e, "healthBar", mods::bHealthBar);
            get(e, "ultPercentage", mods::bUltimatePercentage);
            get(e, "skeleton", mods::bSkeletonESP);
            get(e, "tracerLines", mods::bTracerLines);
            if (e.contains("tracerStartPos")) mods::tracerStartPos = (mods::TracerStartPosition)e["tracerStartPos"].get<int>();
            get(e, "showHeroNames", mods::bShowHeroNames);
            get(e, "showDistance", mods::bShowDistance);
            get(e, "showHealthText", mods::bShowHealthText);
            get(e, "showUltText", mods::bShowUltPercentageText);
            get(e, "teamCheck", mods::bESPTeamCheck);
            get(e, "glow", mods::bGlow);
            get(e, "maxDistance", mods::espMaxDistance);
            if (e.contains("healthBarPos")) mods::healthBarPosition = (mods::ESPFeaturePosition)e["healthBarPos"].get<int>();
            if (e.contains("ultBarPos")) mods::ultBarPosition = (mods::ESPFeaturePosition)e["ultBarPos"].get<int>();
            if (e.contains("distancePos")) mods::distancePosition = (mods::ESPFeaturePosition)e["distancePos"].get<int>();
            if (e.contains("heroNamePos")) mods::heroNamePosition = (mods::ESPFeaturePosition)e["heroNamePos"].get<int>();
        }
        if (cfg.contains("colors")) {
            auto& c = cfg["colors"];
            getColor(c, "visible", mods::visibleColor);
            getColor(c, "nonVisible", mods::nonVisibleColor);
            getColor(c, "skeleton", mods::skeletonESPColor);
            getColor(c, "tracer", mods::tracerColor);
            getColor(c, "healthHigh", mods::healthHighColor);
            getColor(c, "healthMid", mods::healthMidColor);
            getColor(c, "healthLow", mods::healthLowColor);
            getColor(c, "ultBar", mods::ultBarColor);
            getColor(c, "healthBarOutline", mods::healthBarOutlineColor);
            getColor(c, "ultBarOutline", mods::ultBarOutlineColor);
            getColor(c, "barBg", mods::barBackgroundColor);
            getColor(c, "distanceText", mods::distanceTextColor);
            getColor(c, "distanceOutline", mods::distanceTextOutlineColor);
            getColor(c, "heroNameText", mods::heroNameTextColor);
            getColor(c, "heroNameOutline", mods::heroNameTextOutlineColor);
            getColor(c, "heroNameBg", mods::heroNameBgColor);
            getColor(c, "healthText", mods::healthTextColor);
            getColor(c, "healthTextOutline", mods::healthTextOutlineColor);
            getColor(c, "ultText", mods::ultTextColor);
            getColor(c, "ultTextOutline", mods::ultTextOutlineColor);
            getColor(c, "crosshair", mods::crosshairColor);
        }
        if (cfg.contains("crosshair")) {
            auto& x = cfg["crosshair"];
            if (x.contains("type")) mods::crosshairType = (mods::CrosshairType)x["type"].get<int>();
            get(x, "size", mods::crosshairSize);
            get(x, "thickness", mods::crosshairThickness);
            get(x, "gap", mods::crosshairGap);
            get(x, "outline", mods::bCrosshairOutline);
        }
        if (cfg.contains("misc")) {
            auto& m = cfg["misc"];
            get(m, "fovChanger", mods::fov_changer);
            get(m, "fovAmount", mods::fov_changer_amount);
            get(m, "spinbot", mods::bSpinbot);
            get(m, "spinbotX", mods::bSpinbotX);
            get(m, "spinbotY", mods::bSpinbotY);
            get(m, "spinbotZ", mods::bSpinbotZ);
            get(m, "spinSpeedX", mods::SpiningSpeedX);
            get(m, "spinSpeedY", mods::SpiningSpeedY);
            get(m, "spinSpeedZ", mods::SpiningSpeedZ);
            get(m, "smallPerson", mods::Experimental::SmallPerson);
            get(m, "smallScale", mods::Experimental::SmallPersonScale);
            get(m, "hideLocal", mods::Experimental::HideLocalPlayer);
            get(m, "selfTimeDilation", mods::SelfCustomTimeDilationBool);
            get(m, "selfTimeDilationVal", mods::SelfCustomTimeDilationFloat);
            get(m, "rapidFire", mods::bRapidFire);
            get(m, "rapidFireRate", mods::rapidFireRate);
            get(m, "enemyOverlay", mods::bShowEnemyOverlay);
            get(m, "localCheck", mods::LocalCheck);
        }
        if (cfg.contains("radar")) {
            auto& r = cfg["radar"];
            get(r, "enabled", mods::bRadar);
            if (r.contains("design")) mods::radarDesign = (mods::RadarDesign)r["design"].get<int>();
            get(r, "size", mods::radarSize);
            get(r, "range", mods::radarRange);
            get(r, "zoom", mods::radarZoom);
            getVec2(r, "pos", mods::radarPos);
        }
        if (cfg.contains("widgets")) {
            auto& w = cfg["widgets"];
            get(w, "info", mods::bInfoWidget);
            getVec2(w, "infoPos", mods::infoWidgetPos);
            get(w, "keybind", mods::bKeybindWidget);
            getVec2(w, "keybindPos", mods::keybindWidgetPos);
            get(w, "watermark", mods::bWatermark);
            get(w, "damageLog", mods::bDamageLog);
            getVec2(w, "damageLogPos", mods::damageLogPos);
            get(w, "ultTracker", mods::bUltTracker);
            getVec2(w, "ultTrackerPos", mods::ultTrackerPos);
            get(w, "activeFeatures", mods::bActiveFeaturesWidget);
            getVec2(w, "activeFeaturesPos", mods::activeFeaturesPos);
        }
        if (cfg.contains("hotkeys")) {
            auto& h = cfg["hotkeys"];
            get(h, "menu", mods::menuToggleKey);
            get(h, "menuName", mods::menuToggleKeyName);
            get(h, "esp", mods::espHotkey);
            get(h, "espName", mods::espHotkeyName);
            get(h, "glow", mods::glowHotkey);
            get(h, "glowName", mods::glowHotkeyName);
            get(h, "bulletTP", mods::bulletTPHotkey);
            get(h, "bulletTPName", mods::bulletTPHotkeyName);
            get(h, "spinbot", mods::spinbotHotkey);
            get(h, "spinbotName", mods::spinbotHotkeyName);
            get(h, "selfTime", mods::SelfTimeHotkey);
            get(h, "selfTimeName", mods::SelfTimekeyName);
            get(h, "rageKey", mods::rageKey);
            get(h, "rageKeyName", mods::rageKeyName);
        }
        if (cfg.contains("adaptiveFov")) {
            auto& af = cfg["adaptiveFov"];
            get(af, "enabled", mods::bAdaptiveFov);
            get(af, "closeDist", mods::adaptiveCloseDistance);
            get(af, "farDist", mods::adaptiveFarDistance);
            get(af, "scaleClose", mods::adaptiveFovScaleClose);
            get(af, "scaleFar", mods::adaptiveFovScaleFar);
            get(af, "showCircle", mods::bShowAdaptiveFov);
        }
        if (cfg.contains("rage")) {
            auto& rg = cfg["rage"];
            get(rg, "enabled", mods::bRageMode);
            get(rg, "confirmation", mods::bRageConfirmation);
            get(rg, "separateKey", mods::bSeparateRageKey);
            get(rg, "showFovSilent", mods::bShowFovSilent);
            get(rg, "silentAim", mods::bSilentAim);
            get(rg, "silentHealOnly", mods::bSilentHealingOnly);
            get(rg, "silentAdaptive", mods::bSilentAdaptiveFov);
            get(rg, "silentMagicFov", mods::silentMagicFov);
        }
        if (cfg.contains("hitbox")) {
            auto& hb = cfg["hitbox"];
            get(hb, "enabled", mods::bHitboxEditor);
            get(hb, "head", mods::hitboxHead);
            get(hb, "body", mods::hitboxBody);
            get(hb, "armsL", mods::hitboxArmsL);
            get(hb, "armsR", mods::hitboxArmsR);
            get(hb, "legsL", mods::hitboxLegsL);
            get(hb, "legsR", mods::hitboxLegsR);
        }
        if (cfg.contains("filters")) {
            auto& f = cfg["filters"];
            get(f, "distanceFilter", mods::distanceFilter);
            get(f, "espFontSize", mods::espFontSize);
            if (f.contains("drawStyle")) mods::drawStyle = (mods::DrawStyle)f["drawStyle"].get<int>();
            if (f.contains("textPosition")) mods::textPosition = (mods::TextPosition)f["textPosition"].get<int>();
            get(f, "banPhase", mods::bBanPhaseOverlay);
        }
        if (cfg.contains("autoSkill")) {
            auto& as = cfg["autoSkill"];
            get(as, "enabled", mods::bAutoSkill);
            get(as, "autoMelee", mods::bAutoMelee);
            get(as, "autoDetect", mods::bAutoDetectHero);
            if (as.contains("heroes")) {
                for (auto& [key, val] : as["heroes"].items()) {
                    try {
                        int heroId = std::stoi(key);
                        auto& skCfg = mods::heroSkillConfigs[heroId];
                        get(val, "enabled", skCfg.enabled);
                        get(val, "useQ", skCfg.useQ);
                        get(val, "useE", skCfg.useE);
                        get(val, "useShift", skCfg.useShift);
                        get(val, "useRMB", skCfg.useRMB);
                        get(val, "hpThreshold", skCfg.hpThreshold);
                        get(val, "distThreshold", skCfg.distThreshold);
                    } catch (...) {}
                }
            }
        }
        if (cfg.contains("healthPack")) {
            auto& hp = cfg["healthPack"];
            get(hp, "enabled", mods::bHealthPackESP);
            get(hp, "onlyNoCD", mods::bShowOnlyNoCDHealthPack);
            get(hp, "distance", mods::bHealthPackDistance);
            get(hp, "snaplines", mods::bHealthPackSnaplines);
            get(hp, "maxDist", mods::healthPackMaxDistance);
            get(hp, "fontSize", mods::healthPackFontSize);
        }
        if (cfg.contains("autoHeal")) {
            auto& ah = cfg["autoHeal"];
            get(ah, "enabled", mods::bAutoHeal);
            if (ah.contains("heroes")) {
                for (auto& [name, ahCfg] : mods::autoHealConfigs) {
                    if (ah["heroes"].contains(name)) {
                        auto& hj = ah["heroes"][name];
                        get(hj, "enabled", ahCfg.enabled);
                        get(hj, "threshold", ahCfg.hpThreshold);
                    }
                }
            }
        }
        if (cfg.contains("skinChanger")) {
            auto& sc = cfg["skinChanger"];
            get(sc, "enabled", mods::bSkinChanger);
            get(sc, "customEnabled", mods::bCustomSkinEnabled);
            if (sc.contains("overrides")) {
                for (auto& [key, val] : sc["overrides"].items()) {
                    try {
                        int32_t hid = std::stoi(key);
                        mods::skinOverrides[hid] = val.get<int32_t>();
                    } catch (...) {}
                }
            }
            if (sc.contains("heroEnabled")) {
                for (auto& [key, val] : sc["heroEnabled"].items()) {
                    try {
                        int32_t hid = std::stoi(key);
                        mods::skinEnabled[hid] = val.get<bool>();
                    } catch (...) {}
                }
            }
            if (sc.contains("customFiles")) {
                for (auto& [key, val] : sc["customFiles"].items()) {
                    try {
                        int32_t hid = std::stoi(key);
                        mods::customSkinFiles[hid] = val.get<std::vector<std::string>>();
                    } catch (...) {}
                }
            }
        }
        if (cfg.contains("chams")) {
            auto& ch = cfg["chams"];
            get(ch, "enabled", mods::bChams);
            get(ch, "throughWalls", mods::bChamsThroughWalls);
            get(ch, "teamCheck", mods::bChamsTeamCheck);
            if (ch.contains("style")) mods::chamsStyle = (mods::ChamsStyle)ch["style"].get<int>();
            getColor(ch, "visibleColor", mods::chamsVisibleColor);
            getColor(ch, "notVisibleColor", mods::chamsNotVisibleColor);
            get(ch, "opacity", mods::chamsOpacity);
            get(ch, "outline", mods::bChamsOutline);
            getColor(ch, "outlineColor", mods::chamsOutlineColor);
            get(ch, "outlineThickness", mods::chamsOutlineThickness);
            get(ch, "healthBased", mods::bChamsHealthBased);
            get(ch, "glowIntensity", mods::chamsGlowIntensity);
            get(ch, "selfEnabled", mods::bSelfChams);
            if (ch.contains("selfStyle")) mods::selfChamsStyle = (mods::SelfChamsStyle)ch["selfStyle"].get<int>();
            getColor(ch, "selfColor", mods::selfChamsColor);
            get(ch, "selfOpacity", mods::selfChamsOpacity);
            get(ch, "selfGlowIntensity", mods::selfChamsGlowIntensity);
            get(ch, "selfWireframe", mods::bSelfWireframe);
        }
        if (cfg.contains("healer")) {
            auto& hr = cfg["healer"];
            get(hr, "mode", mods::bHealerMode);
            get(hr, "autoHealTeammates", mods::bAutoHealTeammates);
            get(hr, "priorityLowest", mods::bHealPriorityLowest);
            get(hr, "threshold", mods::healThresholdPercent);
            get(hr, "teammateESP", mods::bTeammateESP);
            get(hr, "teammateHP", mods::bTeammateHealthBars);
            get(hr, "rangeIndicator", mods::bHealRangeIndicator);
            get(hr, "range", mods::healRange);
            get(hr, "smartTarget", mods::bSmartHealTarget);
            get(hr, "losCheck", mods::bHealerLOSCheck);
            get(hr, "autoUltHeal", mods::bAutoUltHeal);
            get(hr, "ultHealThreshold", mods::autoUltHealThreshold);
            get(hr, "autoShield", mods::bHealerAutoShield);
            get(hr, "shieldThreshold", mods::autoShieldThreshold);
            get(hr, "notifyLowHP", mods::bHealerNotifyLowHP);
            get(hr, "notifyThreshold", mods::healerNotifyThreshold);
            get(hr, "dashboard", mods::bHealerDashboard);
            getVec2(hr, "dashboardPos", mods::healerDashboardPos);
            getColor(hr, "teammateColor", mods::teammateColor);
            getColor(hr, "healableColor", mods::teammateHealableColor);
            getColor(hr, "criticalColor", mods::teammateCriticalColor);
        }
        if (cfg.contains("autoShield")) {
            auto& as = cfg["autoShield"];
            for (auto& [name, asCfg] : mods::autoShieldConfigs) {
                if (as.contains(name)) {
                    auto& aj = as[name];
                    get(aj, "enabled", asCfg.enabled);
                    get(aj, "threshold", asCfg.hpThreshold);
                }
            }
        }
        if (cfg.contains("antiDetection")) {
            auto& ad = cfg["antiDetection"];
            get(ad, "enabled", mods::bAntiDetection);
            get(ad, "inputJitter", mods::bInputJitter);
            get(ad, "inputJitterMs", mods::inputJitterMs);
            get(ad, "timingRandomization", mods::bTimingRandomization);
            get(ad, "timingVariance", mods::timingVariance);
            get(ad, "memoryCloaking", mods::bMemoryCloaking);
            get(ad, "threadHiding", mods::bThreadHiding);
            get(ad, "antiScreenshot", mods::bAntiScreenshot);
            get(ad, "spreadActions", mods::bSpreadActions);
            get(ad, "actionSpreadMs", mods::actionSpreadMs);
            get(ad, "humanizedMouse", mods::bHumanizedMouse);
            get(ad, "mouseHumanizeStrength", mods::mouseHumanizeStrength);
        }
    }

    inline bool SaveCategoryFile(const std::string& dir, const std::string& category, const json& data) {
        try {
            std::string filepath = dir + "\\" + category + ".bak";
            std::ofstream file(filepath);
            if (!file.is_open()) return false;
            file << data.dump(2);
            file.close();
            return true;
        } catch (...) { return false; }
    }

    inline json LoadCategoryFile(const std::string& dir, const std::string& category) {
        try {
            std::string filepath = dir + "\\" + category + ".bak";
            std::ifstream file(filepath);
            if (!file.is_open()) return json();
            json data = json::parse(file);
            file.close();
            return data;
        } catch (...) { return json(); }
    }

    inline bool SaveConfig(const std::string& name) {
        try {
            std::string baseDir = GetConfigDir();
            std::string cfgDir = baseDir + "\\" + name;
            if (!ConfigSystemFS::exists(cfgDir)) ConfigSystemFS::create_directories(cfgDir);

            json fullCfg = SerializeConfig();

            // Save each top-level category as a separate .bak file
            std::vector<std::string> categories = {
                "aimbot", "esp", "colors", "crosshair", "misc", "radar",
                "widgets", "hotkeys", "adaptiveFov", "rage", "hitbox",
                "filters", "autoSkill", "healthPack", "autoHeal",
                "skinChanger", "chams", "healer", "autoShield", "antiDetection"
            };

            for (const auto& cat : categories) {
                if (fullCfg.contains(cat)) {
                    SaveCategoryFile(cfgDir, cat, fullCfg[cat]);
                }
            }

            mods::currentConfigName = name;
            return true;
        }
        catch (...) { return false; }
    }

    inline bool LoadConfig(const std::string& name) {
        try {
            std::string baseDir = GetConfigDir();
            std::string cfgDir = baseDir + "\\" + name;

            // Try new .bak folder format first
            if (ConfigSystemFS::exists(cfgDir) && ConfigSystemFS::is_directory(cfgDir)) {
                json fullCfg;
                std::vector<std::string> categories = {
                    "aimbot", "esp", "colors", "crosshair", "misc", "radar",
                    "widgets", "hotkeys", "adaptiveFov", "rage", "hitbox",
                    "filters", "autoSkill", "healthPack", "autoHeal",
                    "skinChanger", "chams", "healer", "autoShield", "antiDetection"
                };

                for (const auto& cat : categories) {
                    json catData = LoadCategoryFile(cfgDir, cat);
                    if (!catData.is_null()) {
                        fullCfg[cat] = catData;
                    }
                }

                DeserializeConfig(fullCfg);
                mods::currentConfigName = name;
                return true;
            }

            // Fallback: try legacy single .json file
            std::string legacyPath = baseDir + "\\" + name + ".json";
            if (ConfigSystemFS::exists(legacyPath)) {
                std::ifstream file(legacyPath);
                if (!file.is_open()) return false;
                json cfg = json::parse(file);
                file.close();
                DeserializeConfig(cfg);
                mods::currentConfigName = name;
                return true;
            }

            return false;
        }
        catch (...) { return false; }
    }

    inline bool DeleteConfig(const std::string& name) {
        try {
            std::string baseDir = GetConfigDir();
            std::string cfgDir = baseDir + "\\" + name;

            // Delete new folder format
            if (ConfigSystemFS::exists(cfgDir) && ConfigSystemFS::is_directory(cfgDir)) {
                ConfigSystemFS::remove_all(cfgDir);
                return true;
            }

            // Delete legacy .json
            std::string legacyPath = baseDir + "\\" + name + ".json";
            return ConfigSystemFS::remove(legacyPath);
        }
        catch (...) { return false; }
    }

    inline std::vector<std::string> GetConfigList() {
        std::vector<std::string> configs;
        try {
            std::string dir = GetConfigDir();
            for (const auto& entry : ConfigSystemFS::directory_iterator(dir)) {
                // New format: subdirectories containing .bak files
                if (entry.is_directory()) {
                    configs.push_back(entry.path().filename().string());
                }
                // Legacy format: .json files
                else if (entry.path().extension() == ".json") {
                    configs.push_back(entry.path().stem().string());
                }
            }
        }
        catch (...) {}
        return configs;
    }
}
