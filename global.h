#pragma once
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <deque>

namespace mods {
    enum ESPBoxType {
        ESP_BOX_2D,
        ESP_BOX_3D,
        ESP_BOX_CORNERED
    };
    enum TracerStartPosition {
        TRACER_TOP,
        TRACER_CENTER,
        TRACER_BOTTOM
    };
    enum AimbotMode {
        REGULAR,
        SILENT
    };
    enum CrosshairType {
        NONE,
        DOT,
        CROSS,
        CIRCLE
    };
    enum ThemeType {
        DARK,
        LIGHT,
        CUSTOM
    };
    enum TargetingPriority {
        LEAST_HP,
        LEAST_FOV,
        LEAST_DISTANCE,
        CLOSEST_TO_CROSSHAIR
    };
    enum ESPFeaturePosition {
        LEFT,
        RIGHT,
        TOP,
        BOTTOM
    };
    enum RadarDesign {
        CIRCULAR,
        SQUARE
    };

    namespace FUCKED_UP_SHIT {
        bool shit1 = false;
        bool shit2 = false;
        bool shit3 = false;
        bool shit4 = false;


        float shit1float = 0;
        float shit2float = 0;
        float shit3float = 0;
        float shit4float = 0;
    };


    bool SelfCustomTimeDilationBool = false;
    float SelfCustomTimeDilationFloat = 0.150;

    bool CustomTimeDilationBool = false;
    float CustomTimeDilationFloat = 0.150;

    bool TriggerBot = false;
    float TriggerBotDistance = 999999999;


    bool aimbot = false;
    float smoothing = 8;
    float speed = 2;
    int fov = 8;
    float actualfovcircle = 0.0f;
    bool aimbotFovCircle = false;
    bool VisCheck = false;
    bool esp = false;
    bool fov_changer = false;
    int fov_changer_amount = 115;
    bool LocalCheck = false;
    bool bAimbotTeamCheck = false;
    bool bESPTeamCheck = false;
    bool bHealthBar = false;
    bool bUltimatePercentage = false;
    bool bGlow = false;
    bool bGlowThroughWalls = false;
    bool bGlowIgnoreTeammates = false;
    float aimSmoothing = 8.0f;
    bool isSettingAimbotKey = false;
    int aimbotKey = VK_LBUTTON;
    std::string aimbotKeyName = "LMB";
    bool bShowHeroNames = false;

    bool bESPBox = false;
    ImColor visibleColor = ImColor(0, 255, 0);
    ImColor nonVisibleColor = ImColor(255, 0, 0);
    std::string aimHitbox = "Head";
    // PUBG-style individual bone selection (body part selector)
    bool bBoneHead = true;
    bool bBoneNeck = false;
    bool bBoneChest = false;
    bool bBoneStomach = false;
    bool bBoneLeftShoulder = false;
    bool bBoneRightShoulder = false;
    bool bBoneLeftElbow = false;
    bool bBoneRightElbow = false;
    bool bBoneLeftHand = false;
    bool bBoneRightHand = false;
    bool bBonePelvis = false;
    bool bBoneLeftKnee = false;
    bool bBoneRightKnee = false;
    bool bBoneLeftFoot = false;
    bool bBoneRightFoot = false;
    float aimOffset = 0.0f;
    bool bulletTP = false;
    bool bAimPrediction = false;
    bool bFilterByLowHealth = false;
    bool bSkeletonESP = false;
    bool bOutOfFOVArrows = false;
    bool bRadar = false;

    ESPBoxType espBoxType = ESP_BOX_2D;
    bool bTracerLines = false;
    TracerStartPosition tracerStartPos = TRACER_CENTER;
    bool bShowDistance = false;
    bool bShowHealthText = false;
    bool bShowUltPercentageText = false;
    bool bAimHumanizer = false;
    float humanizerLevel = 5.0f;
    float projectileSpeed = 10000.0f;
    bool isSettingMenuKey = false;
    int menuToggleKey = VK_INSERT;
    std::string menuToggleKeyName = "Insert";
    int safeExitKey = VK_DELETE;
    AimbotMode aimbotMode = REGULAR;

    CrosshairType crosshairType = NONE;
    ImColor crosshairColor = ImColor(255, 255, 255);
    float crosshairSize = 5.0f;
    float crosshairThickness = 1.0f;
    bool bSpinbot = false;
    bool bSpinbotX = false;
    bool bSpinbotY = false;
    bool bSpinbotZ = false;
    float SpiningSpeedX = 10;
    float SpiningSpeedY = 10;
    float SpiningSpeedZ = 10;
    ThemeType currentTheme = DARK;
    ImColor customWindowBg = ImColor(0.1f, 0.1f, 0.1f, 1.0f);
    ImColor customText = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
    std::string settingHotkeyFor = "";
    int espHotkey = 0;
    std::string espHotkeyName = "None";
    int glowHotkey = 0;
    std::string glowHotkeyName = "None";
    int bulletTPHotkey = 0;
    std::string bulletTPHotkeyName = "None";
    int spinbotHotkey = 0;
    std::string spinbotHotkeyName = "None";
    int SelfTimeHotkey = 0;
    std::string SelfTimekeyName = "None";
    bool bRapidFire = false;
    float rapidFireRate = 0.1f;
    int rapidFireHotkey = 0;
    std::string rapidFireHotkeyName = "None";

    // Existing color customization
    ImColor aimbotFovCircleColor = ImColor(255, 255, 255);
    ImColor heroNameTextColor = ImColor(255, 255, 255);
    ImColor heroNameBgColor = ImColor(0, 0, 0, 150);
    ImColor skeletonESPColor = ImColor(255, 255, 255);
    ImColor outOfFOVArrowsColor = ImColor(255, 0, 0);
    ImColor radarBgColor = ImColor(0, 0, 0, 100);
    ImColor radarLocalColor = ImColor(0, 255, 0);
    ImColor radarEnemyColor = ImColor(255, 0, 0);
    ImColor tracerColor = ImColor(255, 255, 0);

    // New color customization
    ImColor distanceTextColor = ImColor(255, 255, 255);
    ImColor distanceTextOutlineColor = ImColor(0, 0, 0);
    ImColor heroNameTextOutlineColor = ImColor(0, 0, 0);
    ImColor healthTextColor = ImColor(255, 255, 255);
    ImColor healthTextOutlineColor = ImColor(0, 0, 0);
    ImColor ultTextColor = ImColor(255, 255, 255);
    ImColor ultTextOutlineColor = ImColor(0, 0, 0);
    ImColor healthHighColor = ImColor(0, 255, 0, 250);
    ImColor healthMidColor = ImColor(255, 255, 0, 250);
    ImColor healthLowColor = ImColor(255, 0, 0, 250);
    ImColor ultBarColor = ImColor(255, 255, 0, 250);
    ImColor healthBarOutlineColor = ImColor(0, 0, 0);
    ImColor ultBarOutlineColor = ImColor(0, 0, 0);
    ImColor barBackgroundColor = ImColor(0, 0, 0, 200);

    namespace Experimental {
        bool SmallPerson = false;
        float SmallPersonScale = 0.5f;
        bool HideLocalPlayer = false;
        bool StreamProof = false;
    }

    float aimbotMaxDistance = 500.0f;
    float espMaxDistance = 500.0f;


    static const std::unordered_map<int, std::string> heroIDToName = {
        {1011, "Hulk"}, {1014, "Punisher"}, {1015, "Storm"}, {1016, "Loki"}, {1017, "Human Torch"},
        {1018, "Doctor Strange"}, {1020, "Mantis"}, {1021, "Hawkeye"}, {1022, "Captain America"},
        {1023, "Rocket Raccoon"}, {1024, "Hela"}, {1025, "Dagger"}, {1026, "Black Panther"},
        {1027, "Groot"}, {1029, "Magik"}, {1030, "Moon Knight"}, {1031, "Luna Snow"},
        {1032, "Squirrel Girl"}, {1033, "Black Widow"}, {1034, "Iron Man"}, {1035, "Venom"},
        {1036, "Spider Man"}, {1037, "Magneto"}, {1038, "Scarlet Witch"}, {1039, "Thor"},
        {1040, "Mister Fantastic"}, {1041, "Winter Soldier"}, {1042, "Peni Parker"},
        {1043, "Star Lord"}, {1045, "Namor"}, {1046, "Adam Warlock"}, {1047, "Jeff"},
        {1048, "Psylocke"}, {1049, "Wolverine"}, {1050, "Invisible Woman"}, {1051, "The Thing"},
        {1052, "Iron Fist"}, {4016, "Galacta Bot"}, {4018, "Galacta Bot Plus"}
    };

    TargetingPriority aimbotPriority = LEAST_HP;
    bool bHardLock      = false;
    bool bAimbotSnapLine = false;
    ImColor snapLineColor = ImColor(255, 0, 0);
    float snapLineThickness = 1.0f;

    // Updated font rendering customizations
    std::vector<std::string> fontNames;
    std::vector<ImFont*> availableFonts;
    int selectedESPFontIndex = 4;  // For ESP renders (4 = Verdana)
    int selectedMenuFontIndex = 0; // For menu UI
    float textScale = 1.0f;
    float baseFontSize = 12.0f;

    bool bTextBackground = false;

    // ==================== ESP EXTENDED INFO LABELS ====================
    bool bShowHeroNameESP = false;
    ImColor heroNameESPColor = ImColor(255, 255, 255);
    ImColor heroNameESPOutline = ImColor(0, 0, 0);

    bool bShowHeroIcons = false;
    float heroIconSize = 28.0f;

    bool bShowKillsESP = false;
    ImColor killsESPColor = ImColor(255, 220, 50);
    ImColor killsESPOutline = ImColor(0, 0, 0);

    bool bShowKDRESP = false;
    ImColor kdrESPColor = ImColor(255, 200, 100);
    ImColor kdrESPOutline = ImColor(0, 0, 0);

    bool bShowHealingESP = false;
    ImColor healingESPColor = ImColor(80, 220, 80);
    ImColor healingESPOutline = ImColor(0, 0, 0);

    bool bShowKillStreakESP = false;
    ImColor killStreakESPColor = ImColor(255, 140, 0);
    ImColor killStreakESPOutline = ImColor(0, 0, 0);

    bool bShowPlatformESP = false;
    // Platform: Windows = blue, Console = green (fixed per spec)

    bool bESPBoxOutline = false;
    ImColor espBoxOutlineColor = ImColor(0, 0, 0);
    float espBoxThickness = 1.0f;

    ESPFeaturePosition healthBarPosition = LEFT;
    ESPFeaturePosition ultBarPosition = RIGHT;
    ESPFeaturePosition distancePosition = BOTTOM;
    ESPFeaturePosition heroNamePosition = TOP;

    bool bDynamicFOV = false;
    float minFovMultiplier = 0.5f;
    float maxFovMultiplier = 1.5f;

    bool bShowEnemyOverlay = false;
    struct EnemyInfoOverlay {
        int heroID;
        float ultPercentage;
        float CurrentHP;
    };

    std::vector<EnemyInfoOverlay> enemyOverlayData;

    std::map<std::wstring, ImVec2> capturedBoneRelativePositions;
    std::vector<ImVec2> capturedBoxCornerRelativePositions;
    bool skeletonCaptured = false;
    bool bShowESPPreview = false;

    // New radar design variable
    RadarDesign radarDesign = CIRCULAR;
    float closestDistance = FLT_MAX;  // Initialized to maximum float value

    // Radar settings
    float radarSize = 150.0f;
    float radarRange = 5000.0f;
    float radarZoom = 1.0f;
    ImVec2 radarPos = ImVec2(20, 200);
    bool bRadarDraggable = false;

    // Crosshair additional settings
    bool bCrosshairOutline = false;
    float crosshairGap = 3.0f;
    int crosshairSegments = 4;

    // Damage log
    bool bDamageLog = false;
    ImVec2 damageLogPos = ImVec2(20, 400);
    int damageLogMaxEntries = 8;
    float damageLogFadeTime = 5.0f;

    struct DamageLogEntry {
        std::string text;
        float timestamp;
        ImColor color;
    };
    std::deque<DamageLogEntry> damageLogEntries;

    // Info widget
    bool bInfoWidget = false;
    ImVec2 infoWidgetPos = ImVec2(20, 20);

    // Keybind widget
    bool bKeybindWidget = false;
    ImVec2 keybindWidgetPos = ImVec2(20, 120);

    // Config system
    std::string configPath = "";
    std::string currentConfigName = "default";
    std::vector<std::string> configFiles;

    // Watermark
    bool bWatermark = false;

    // Match Status Widget (queue / connecting banner)
    bool bMatchStatusWidget = false;

    // Language Toggle (0 = Chinese, 1 = English)
    int currentLanguage = 1;

    // License System
    inline bool bLicenseSaved = false;
    inline char licenseUser[64] = "";
    inline char licensePass[64] = "";

    // Skin Changer
    bool bSkinChanger = false;
    int skinChangerSelectedHero = 0;
    std::unordered_map<int32_t, int32_t> skinOverrides;
    std::unordered_map<int32_t, bool> skinEnabled;

    // ==================== NEW FEATURES ====================

    // Ultimate Tracker Widget
    bool bUltTracker = false;
    ImVec2 ultTrackerPos = ImVec2(20, 500);
    struct UltTrackerEntry {
        int heroID;
        float ultPercent;
        bool isReady;
    };
    std::vector<UltTrackerEntry> ultTrackerEntries;

    // Active Features Widget
    bool bActiveFeaturesWidget = false;
    ImVec2 activeFeaturesPos = ImVec2(20, 300);

    // Ban Phase Overlay
    bool bBanPhaseOverlay = false;

    // Filters & Style
    float distanceFilter = 500.0f;
    float espFontSize = 14.0f;
    enum DrawStyle { DRAW_DEFAULT, DRAW_MINIMALIST };
    DrawStyle drawStyle = DRAW_DEFAULT;
    enum TextPosition { TEXT_DOWN, TEXT_MIDDLE, TEXT_UP };
    TextPosition textPosition = TEXT_DOWN;

    // Adaptive FOV
    bool bAdaptiveFov = false;
    float adaptiveCloseDistance = 50.0f;
    float adaptiveFarDistance = 300.0f;
    float adaptiveFovScaleClose = 2.0f;
    float adaptiveFovScaleFar = 0.5f;
    bool bShowAdaptiveFov = false;

    // Auto Skill
    bool bAutoSkill = false;
    bool bAutoMelee = false;
    int autoSkillSelectedHero = 0;
    bool bAutoDetectHero = false;
    struct HeroSkillConfig {
        bool enabled = false;
        bool useQ = false;
        bool useE = false;
        bool useShift = false;
        bool useRMB = false;
        float hpThreshold = 50.0f;
        float distThreshold = 20.0f;
    };
    std::unordered_map<int, HeroSkillConfig> heroSkillConfigs;

    // Health Pack & Elsa Trap ESP
    bool bHealthPackESP = false;
    bool bShowOnlyNoCDHealthPack = false;
    bool bHealthPackDistance = false;
    bool bHealthPackSnaplines = false;
    float healthPackMaxDistance = 150.0f;
    float healthPackFontSize = 12.0f;

    // Auto Heal (hero-specific HP thresholds)
    bool bAutoHeal = false;
    struct AutoHealConfig {
        bool enabled = false;
        float hpThreshold = 100.0f;
    };
    std::unordered_map<std::string, AutoHealConfig> autoHealConfigs = {
        {"Adam Warlock", {false, 249.0f}},
        {"Loki",         {false, 274.0f}},
        {"Ultron",       {false, 249.0f}},
        {"Jeff",         {false, 249.0f}},
        {"Cloak & Dagger", {false, 274.0f}}
    };

    // Rage Mode
    bool bRageMode = false;
    bool bRageConfirmation = false;
    bool bSeparateRageKey = false;
    int rageKey = 0;
    std::string rageKeyName = "None";
    bool bIsSettingRageKey = false;
    bool bShowFovSilent = false;
    bool bSilentAim = false;
    bool bSilentHealingOnly = false;
    bool bSilentAdaptiveFov = false;
    float silentMagicFov = 15.0f;

    // Hitbox Editor
    bool bHitboxEditor = false;
    float hitboxHead = 1.0f;
    float hitboxBody = 1.0f;
    float hitboxArmsL = 1.0f;
    float hitboxArmsR = 1.0f;
    float hitboxLegsL = 1.0f;
    float hitboxLegsR = 1.0f;

    // ==================== CHAMS ====================
    bool bChams = false;
    bool bChamsThroughWalls = false;
    bool bChamsTeamCheck = false;
    enum ChamsStyle { CHAMS_FLAT, CHAMS_WIREFRAME, CHAMS_GLOW, CHAMS_PULSE };
    ChamsStyle chamsStyle = CHAMS_FLAT;
    ImColor chamsVisibleColor = ImColor(0, 255, 0, 180);
    ImColor chamsNotVisibleColor = ImColor(255, 0, 0, 180);
    float chamsOpacity = 0.7f;
    bool bChamsOutline = false;
    ImColor chamsOutlineColor = ImColor(255, 255, 255);
    float chamsOutlineThickness = 2.0f;
    bool bChamsHealthBased = false;
    float chamsGlowIntensity = 8.0f;

    // Self Chams
    bool bSelfChams = false;
    enum SelfChamsStyle { SELF_CHAMS_FLAT, SELF_CHAMS_WIREFRAME, SELF_CHAMS_GLOW };
    SelfChamsStyle selfChamsStyle = SELF_CHAMS_GLOW;
    ImColor selfChamsColor = ImColor(0, 150, 255, 200);
    float selfChamsOpacity = 0.6f;
    float selfChamsGlowIntensity = 6.0f;
    bool bSelfWireframe = false;


    // ==================== HEALER AREA ====================
    bool bHealerMode = false;
    bool bAutoHealTeammates = false;
    bool bHealPriorityLowest = false;
    float healThresholdPercent = 70.0f;
    bool bTeammateESP = false;
    bool bTeammateHealthBars = false;
    bool bHealRangeIndicator = false;
    float healRange = 30.0f;
    bool bSmartHealTarget = false;
    bool bHealerLOSCheck = false;
    bool bAutoUltHeal = false;
    float autoUltHealThreshold = 30.0f;
    bool bHealerAutoShield = false;
    float autoShieldThreshold = 50.0f;
    bool bHealerNotifyLowHP = false;
    float healerNotifyThreshold = 25.0f;
    ImColor teammateColor = ImColor(0, 150, 255);
    ImColor teammateHealableColor = ImColor(0, 255, 100);
    ImColor teammateCriticalColor = ImColor(255, 50, 50);
    bool bHealerDashboard = false;
    ImVec2 healerDashboardPos = ImVec2(20, 350);

    // ==================== SKIN CHANGER SEARCH ====================
    char skinSearchBuf[64] = "";

    // ==================== CUSTOM SKINS (NEXUS MODS) ====================
    std::unordered_map<int32_t, std::vector<std::string>> customSkinFiles; // multiple files per hero
    char customSkinPathBuf[260] = "";
    bool bCustomSkinEnabled = false;

    // ==================== VERTS PRANK ====================
    bool bVertsPrank = false;

    // ==================== ANTI-DETECTION ====================
    bool bAntiDetection = false;
    bool bACWatchdog = false;              // persistent AC thread killer
    int  acWatchdogIntervalMs = 2500;     // ms between scans (lower = faster catch, higher = less CPU)
    bool bInputJitter = false;
    float inputJitterMs = 15.0f;        // random delay 0-N ms before inputs
    bool bTimingRandomization = false;
    float timingVariance = 0.3f;        // 0-1, how much to randomize action timing
    bool bMemoryCloaking = false;         // hide cheat memory pages
    bool bThreadHiding = false;           // hide cheat threads from queries
    bool bAntiScreenshot = false;        // blank overlay during screenshots
    bool bSpreadActions = false;          // spread automated actions over time
    float actionSpreadMs = 50.0f;
    bool bHumanizedMouse = false;         // bezier curve mouse movement
    float mouseHumanizeStrength = 0.5f;  // 0-1

    // ==================== HEALING DASHBOARD STATS ====================
    float totalHealingDone = 0.0f;
    float healingPerSecond = 0.0f;
    int teammatesHealed = 0;
    int revivesPerformed = 0;
    int shieldsApplied = 0;
    float lastHealTickTime = 0.0f;
    float healingWindow[60] = {};       // rolling 60-second window
    int healingWindowIdx = 0;
    std::string currentHealTarget = "None";
    float currentTargetHP = 0.0f;
    float currentTargetMaxHP = 0.0f;

    // ==================== HEAL AIM ====================
    bool  bHealAim           = false;
    bool  bIsSettingHealAimKey = false;
    int   healAimKey         = VK_MENU;   // default: Alt
    float healAimFov         = 150.0f;    // pixel radius
    float healAimSmoothing   = 8.0f;
    bool  bHealAimFovCircle  = false;
    int   healAimPriority    = 0;         // 0=LowestHP 1=ClosestCrosshair 2=ClosestDistance
    static const char* healAimPriorityNames[] = { "Lowest HP", "Closest Crosshair", "Closest Distance" };
    ImU32 healAimFovColor    = IM_COL32(0, 200, 100, 120); // green tint circle

    // ==================== HEALER ABILITY KEY MAP ====================
    // Maps healer names to their heal ability key (VK code)
    static const std::unordered_map<std::string, int> healerAbilityKeys = {
        {"Adam Warlock",   'E'},
        {"Loki",           'E'},
        {"Ultron",         'E'},
        {"Jeff",           'Q'},
        {"Cloak & Dagger", 'E'},
        {"Luna Snow",      'E'},
        {"Mantis",         'E'},
        {"Rocket Raccoon", 'E'},
        {"Invisible Woman",'E'},
    };

    // Auto Heal expanded (shield heroes)
    struct AutoShieldConfig {
        bool enabled = false;
        float hpThreshold = 200.0f;
    };
    std::unordered_map<std::string, AutoShieldConfig> autoShieldConfigs = {
        {"Invisible Woman", {false, 276.0f}},
        {"Venom",           {false, 799.0f}},
        {"Namor",           {false, 274.0f}},
        {"Scarlet Witch",   {false, 249.0f}},
        {"Mister Fantastic", {false, 374.0f}},
        {"Hulk",            {false, 749.0f}},
    };

    // ==================== EXPLOITS ====================

    // 1. No Cooldown
    bool bNoCooldown = false;

    // 2. No Ability Cost (Infinite Ammo/Energy)
    bool bNoCost = false;

    // 3. Cooldown Reset On Demand
    bool bCooldownReset = false;
    int cooldownResetHotkey = 0;
    std::string cooldownResetHotkeyName = "None";
    bool bIsSettingCDResetKey = false;

    // 5. Projectile Homing
    bool bProjectileHoming = false;
    float homingAcceleration = 50000.0f;
    float homingMaxSeconds = 5.0f;

    // 6. Projectile Speed Override
    bool bProjectileSpeedOverride = false;
    float projectileSpeedMultiplier = 10.0f;

    // 7. Instant Ability Activation
    bool bInstantAbility = false;

    // 8. Auto Combo
    bool bAutoCombo = false;
    int autoComboDelayMs = 50;

    // 9. Speed Hack
    bool bSpeedHack = false;
    float speedHackMultiplier = 1.5f;
    bool bSpeedPulseMode = false;
    int speedPulseOnMs = 200;
    int speedPulseOffMs = 300;
    bool bAntiCorrection = false;

    // 10. Super Jump
    bool bSuperJump = false;
    float superJumpMultiplier = 2.0f;

    // 11. Fly Hack / Anti-Gravity
    bool bFlyHack = false;
    float flySpeed = 600.0f;
    float gravityScaleOverride = 0.0f;
    bool bFlyUseMovementMode = false;

    // 12. Infinite Dash
    bool bInfiniteDash = false;

    // 13. Wall Climb Anywhere
    bool bWallClimbAnywhere = false;

    // 14. No Fall Damage
    bool bNoFallDamage = false;

    // 15. (removed)

    // 16. (removed)

    // 17. (removed)

    // 18. Enemy Ability Cooldown ESP
    bool bEnemyCooldownESP = false;

    // 19. Enemy Ult Charge Reader (Exact %)
    bool bExactUltTracker = false;

    // ==================== REGULAR FEATURES ====================

    // --- Visual Features ---
    bool bThreatIndicator = false;
    float threatHighThreshold = 70.0f;
    float threatMedThreshold = 40.0f;
    ImColor threatHighColor = ImColor(255, 0, 0);
    ImColor threatMedColor = ImColor(255, 165, 0);
    ImColor threatLowColor = ImColor(0, 200, 0);

    bool bKillPrediction = false;
    float killPredictDamage = 250.0f;

    bool bTeamCompAnalyzer = false;
    ImVec2 teamCompPos = ImVec2(20, 600);

    bool bSoundESP = false;
    float soundESPRange = 3000.0f;

    bool bDeathHeatmap = false;
    struct DeathPoint { float x; float y; float z; float timestamp; };
    std::vector<DeathPoint> deathPoints;

    bool bDamageNumbers = false;
    float damageNumberScale = 1.0f;
    float damageNumberDuration = 1.5f;
    struct DamagePopup { float x; float y; float value; float timestamp; bool isHeadshot; };
    std::vector<DamagePopup> damagePopups;

    bool bLOSIndicator = false;

    // --- Combat Assistance ---
    bool bAutoReload = false;
    int autoReloadDelayMs = 100;

    bool bAutoMeleeRange = false;
    float autoMeleeDistance = 5.0f;

    bool bPeekAssist = false;

    bool bRecoilDisplay = false;
    float recoilDisplayScale = 1.0f;

    bool bHitSound = false;
    int hitSoundType = 0; // 0=click 1=bell 2=quake
    float hitSoundVolume = 1.0f;

    bool bAutoCrouchSpam = false;
    int crouchSpamIntervalMs = 80;

    bool bTargetCycling = false;
    int targetCycleNextKey = 0;
    int targetCyclePrevKey = 0;
    std::string targetCycleNextKeyName = "None";
    std::string targetCyclePrevKeyName = "None";
    int currentTargetIndex = 0;

    // --- Healer / Support Features ---
    bool bSmartHealQueue = false;
    struct HealQueueEntry { int heroID; float hp; float maxHp; float distance; bool los; int priority; };
    std::vector<HealQueueEntry> healQueue;

    bool bHealPrediction = false;

    bool bAutoAbilityRotation = false;
    int abilityRotationDelayMs = 200;

    bool bTeamHPDashboard = false;
    ImVec2 teamHPDashboardPos = ImVec2(20, 450);
    struct TeammateDashEntry { int heroID; float hp; float maxHp; float distance; bool hasLOS; };
    std::vector<TeammateDashEntry> teamDashEntries;

    bool bHealSnipeAlert = false;
    float healSnipeAlertThreshold = 100.0f;

    // --- Config / QoL Features ---
    bool bStreamSafeMode = false;

    bool bSessionStats = false;
    ImVec2 sessionStatsPos = ImVec2(20, 500);
    int sessionKills = 0;
    int sessionDeaths = 0;
    int sessionAssists = 0;
    float sessionDamageDealt = 0.0f;
    float sessionHealingDone = 0.0f;
    int sessionHeadshots = 0;
    int sessionShots = 0;
    float sessionStartTime = 0.0f;

    bool bMatchTimer = false;
    ImVec2 matchTimerPos = ImVec2(20, 50);
    float matchStartTime = 0.0f;

    bool bHotkeyCheatSheet = false;
    ImVec2 hotkeySheetPos = ImVec2(20, 250);

    // --- Map / Game Awareness ---
    bool bObjectiveTimer = false;
    ImVec2 objectiveTimerPos = ImVec2(20, 70);
    float objectiveStartTime = 0.0f;

    bool bSpawnTimer = false;
    struct SpawnTimerEntry { std::string heroName; float deathTime; float respawnDuration; };
    std::vector<SpawnTimerEntry> spawnTimers;

    bool bFlankAlert = false;
    float flankAlertAngle = 90.0f;
    float flankAlertRange = 3000.0f;
    bool bFlankAlertSound = false;

    bool bHealthPackTimer = false;
    struct HealthPackInfo { float x; float y; float z; float lastPickupTime; float respawnTime; bool isAvailable; };
    std::vector<HealthPackInfo> healthPackTimers;

    bool bEnhancedMinimap = false;
    float minimapZoom = 1.0f;
    float minimapSize = 200.0f;
    ImVec2 minimapPos = ImVec2(20, 200);

    // --- Rubberband File Logger ---
    bool bRubberbandLog = false;
    std::string rubberbandLogPath = "";
    int rubberbandLogCount = 0;

    // ==================== COMBOS ====================
    bool bCombosEnabled = false;
    bool bComboHoldKey = false;
    bool bComboDisableTriggerbot = false;
    int comboKey1 = 0;
    std::string comboKey1Name = "None";
    bool bIsSettingComboKey1 = false;
    int comboKey2 = 0;
    std::string comboKey2Name = "None";
    bool bIsSettingComboKey2 = false;
    bool bComboRunning = false;

    // Combo 1 keybind heroes
    bool bComboRogue = false;
    bool bComboAngela = false;
    bool bComboGambit = false;
    bool bComboDaredevil = false;
    bool bComboBlackPanther = false;
    bool bComboBlackPantherAutoMark = false;
    bool bComboDoctorStrange = false;
    bool bComboDoctorStrangeHoldShield = false;
    bool bComboMagic1_1 = false;
    bool bComboMagic1_1Aimbot = false;
    bool bComboMagic1_2 = false;
    float comboMagic1_2HitboxScale = 1.0f;
    bool bComboGroot = false;
    bool bComboVenom = false;
    bool bComboJeff = false;
    bool bComboBlackWidow = false;
    bool bComboCaptainAmerica = false;
    bool bComboPsylocke1 = false;
    bool bComboPsylocke1_2 = false;
    bool bComboMagneto = false;
    bool bComboSpiderMan1_1 = false;
    bool bComboSpiderMan1_2 = false;
    bool bComboSpiderMan1_3 = false;
    bool bComboMisterFantastic = false;
    bool bComboWinterSoldier1_1 = false;
    bool bComboWinterSoldier1_2 = false;
    bool bComboWolverine = false;
    bool bComboPhoenix = false;
    bool bComboThor = false;
    bool bComboEmmaFrost = false;
    bool bComboHawkeye = false;

    // Combo 2 keybind heroes
    bool bComboSpiderMan2_1 = false;
    bool bComboNamor = false;
    bool bComboMantisAutoHeal = false;
    bool bComboPhoenixBeyblade = false;
    bool bComboHumanTorch = false;

    // ==================== AUTO KEY ====================
    bool bAutoKeyEnabled = false;

    // Auto Melee
    bool bAutoMeleeKey = false;
    float autoMeleeRange = 3.0f;
    int autoMeleeVK = 'V';
    std::string autoMeleeVKName = "V";
    bool bIsSettingAutoMeleeKey = false;

    // Auto Shield (generic - press shield key when HP% low)
    bool bAutoShieldAbility = false;
    float autoShieldAbilityHPPct = 30.0f;
    int autoShieldAbilityVK = 'E';
    std::string autoShieldAbilityVKName = "E";
    bool bIsSettingAutoShieldAbKey = false;

    // Auto Immune
    bool bAutoImmune = false;
    float autoImmuneHPPct = 20.0f;
    int autoImmuneVK = 'E';
    std::string autoImmuneVKName = "E";
    bool bIsSettingAutoImmuneKey = false;

    // Auto Kill Ability
    bool bAutoKillAbility = false;
    float autoKillAbilityEnemyHP = 50.0f;
    int autoKillAbilityVK = 'Q';
    std::string autoKillAbilityVKName = "Q";
    bool bAutoKillAbTrigger = false;
    bool bIsSettingAutoKillAbKey = false;

    // Animation Cancel
    bool bAnimCancel = false;
    int animCancelDelayMs = 100;
    int animCancelVK = VK_LSHIFT;
    std::string animCancelVKName = "LShift";
    bool bIsSettingAnimCancelKey = false;

    // Auto Buff
    bool bAutoBuff = false;
    int autoBuffIntervalMs = 5000;
    int autoBuffVK = 'Q';
    std::string autoBuffVKName = "Q";
    bool bIsSettingAutoBuffKey = false;

    // Auto Heal (AutoKey version)
    bool bAutoKeyHeal = false;
    float autoKeyHealHPPct = 40.0f;
    int autoKeyHealVK = 'E';
    std::string autoKeyHealVKName = "E";
    bool bIsSettingAutoKeyHealKey = false;

    // Auto Shift Ability
    bool bAutoShiftAbility = false;
    int autoShiftIntervalMs = 3000;

    // Auto Shield - Invisible Woman (heroID 1050)
    bool bAutoShieldIW = false;
    float autoShieldIWHP = 150.0f;

    // Auto Shield - Cloak & Dagger (heroID 1025)
    bool bAutoShieldCD = false;
    float autoShieldCDHP = 100.0f;

    // ==================== DODGE / ULT CANCEL ====================
    bool bDodgeEnabled = false;
    int dodgeUltCancelKey = 'E';
    std::string dodgeUltCancelKeyName = "E";
    bool bIsSettingDodgeKey = false;
    float dodgeMaxRange = 50.0f;

    // Per-hero ult blocking toggles
    bool bDodgeInvisibleWoman = false;
    bool bDodgeRogue = false;
    bool bDodgeLuna = false;
    bool bDodgeHulk = false;
    bool bDodgePeniParker = false;
    bool bDodgeSpiderMan = false;
    bool bDodgeMantis = false;

    // ==================== PSILENT (ProcessEvent-based Silent Aim) ====================
    bool bPSilentEnabled = false;
    float pSilentFov = 15.0f;
    float pSilentMaxDistance = 200.0f;
    bool bPSilentUseAimbotKey = false;
    int pSilentKey = VK_LBUTTON;
    std::string pSilentKeyName = "LMB";
    bool bIsSettingPSilentKey = false;
    bool bPSilentShowFov = false;
    ImColor pSilentFovColor = ImColor(0, 200, 255, 180);
    bool bPSilentTeamCheck = false;
    // Fire params struct offsets — dump HandleFireWithParams after each patch
    int pSilentOffsetUseCustomTarget = 0x00;
    int pSilentOffsetViewLocation    = 0x08;
    int pSilentOffsetViewRotation    = 0x20;
    int pSilentOffsetTargetLocation  = 0x128;
}

namespace Keys {
    SDK::FKey Insert;
    SDK::FKey LeftMouseButton;
}