#ifndef INFOSERVER_H
#define INFOSERVER_H

#include <cstdint>
#include <vector>
#include <string>
#include <set>
#include "../../Version.h"
#include <lib/SimpleParts.h>

#define MAX_VEHICLES (2000)
#define MAX_PLAYERS (1000)


enum CustomType
{
    Custom_Speedometer,
    Custom_Hud,
    Custom_Radar,
    Custom_Fuel,
    Custom_Milage,
    Custom_TypeFuel,
    Custom_Hunger,
    Custom_Hints,
    Custom_TypeDialog,
    Custom_Map,
    Custom_Discord,
    Custom_Dialog,
    Custom_Chat,
    Custom_SetRadius,
    Custom_Show,
    Custom_Wanted,
    Custom_ChatBoard,
    Custom_WallHack,
    Custom_TypeAim,
    Custom_GeoPosition,
    Custom_CinemaMode
};

struct AudioInfo
{
    int aType; // тип 0: off 1: фон игрока (2D, слышит только он) 2: игрок 3: машина 4: точка на карте (колонка)
    int aTime; // секунд с начала трека (пока играет - обновляет Native::Audio_SyncTime из aZeroMs)
    long long aZeroMs = 0; // пока играет: момент (steady, мс), когда трек был на 0
    int aPause = 1; // пауза
    int aUid; // уникальный номер тачки или игрока
    char aURL[300]; // bytes url музыки
    int aSizeURL; // размер ссылки
    float aPos[3] = { 0.0f, 0.0f, 0.0f }; // тип 4: координаты
    float aRadius = 50.0f;                 // тип 4: радиус слышимости
    std::string aSource;                   // ссылка, как её дал скрипт (YouTube и т.д.)
    int aDurationMs = -1;                  // длина трека в мс: -1 - ещё узнаём
    bool aLoop = true;                     // по кругу: в конце начать заново (false - стоп + public)
    bool aRadioFx = false;                 // эффект "как из рации" (252:18 тип 2) - досылается при каждом запуске трека
    void* aOwner = nullptr;                // скрипт, который создал поток: выгрузили (gmx) - поток удаляется
};

extern std::vector<AudioInfo> aTemp;
extern std::string currentVersion;
extern std::set<int> connectedPlayers;

extern const char* tagsname[];

struct PlayerInfos
{
    int pLauncherStat;
    unsigned char pEkranWindows[4];
    int pSkin;
    int pHud;
    int pSpeed;
    int pHints;
    uint8_t pStatusLoadCef;
    uint8_t pCefStarted;   // новый клиент: CEF (главное меню) клиент открыл сам, без 220:20
    int pChatOpen;
    int pChat;
    int pColision;
    char pHotKey[5][10];
    int pAcs_Model[10];
    int pAcs_bone[10];
    bool pAcs_create[10];
    float pAcs_OffsetX[10];
    float pAcs_OffsetY[10];
    float pAcs_OffsetZ[10];
    float pAcs_fRotX[10];
    float pAcs_fRotY[10];
    float pAcs_fRotZ[10];
    float pAcs_fScaleX[10];
    float pAcs_fScaleY[10];
    float pAcs_fScaleZ[10];
    int pAcs_color_1[10];
    int pAcs_color_2[10];
    bool pLoadRadioCheck;
    int pRadio;
    bool pMobile;
    char pTag[80];
    bool pTagStatus;
};

extern PlayerInfos PI[1000];

struct VehicleInfo
{
    int vModelLuxe = 0;
    int vDefaultModel = 0;
    int vID = 0;
    int vParts[PART_SLOTS] = {};   // simple-детали по слотам (VehiclePartSlot), диски - vDiski
    int vNitroColor = -1;   // цвет пламени нитро 0..3 (220:151), -1 - не задан
    bool vNitro = false;    // нитро-система Arizona (220:156)
    bool vSilentEngine = false; // двигатель без звука (220:150)
    std::string vHeadlightTexture;   // текстура фар (220:193), "" - стандартная
    int vTurnSignal = 0;        // поворотники (220:2 от водителя): 0 выкл, 1 левый, 2 правый, 3 аварийка
    int vDiski = 0;
    int vColor = 0;
    bool vColorCustom = false;
    float vDirt = -1.0f;        // грязь 0..100 (220:26), -1 - не задана
    float vDirtRate = 1.0f;     // множитель набора грязи при езде (0 - не пачкается)
    int vWear = 1;              // износ кузова 0..17 (0-1 - нет)
    uint32_t vTurnColor = 0;    // подсветка поворотников 0xRRGGBBAA (220:133)
    bool vTurnColorSet = false; // цвет кузова задан через плагин (материал Arizona) - только тогда его надо досылать
    int vDiskiColor = -1;                    // -1 - цвет дисков не задан
    int vNeon = 0;
    int vHidravlic = 0;
    int vToner[7] = { 0 };
    int vToner_red[7] = { 0 };
    int vToner_green[7] = { 0 };
    int vToner_blue[7] = { 0 };
    int vToner_size[7] = { 0 };
    int vVinil = 0;
    int vNumberType = 0;
    int vColorShift = 0;
    int vHeaderlamp = 0;
    int vOtstrel = 0;
    int vStrobe = 0;                         // режим стробоскопов (0 - выкл)
    float vStrobeSpeed = 1.0f;
    bool vStrobeBeam = false;
    int vCalipers = 0;                       // модель суппортов (0 - нет)
    bool vCalipersSimple = false;
    bool vDrift = false;
    bool vSiren = false;                     // мигалка (220:152)
    float vDiameteWheel = 0.0f;
    float vWheel_1[2] = { 0.0f, 0.0f };
    float vWheel_2[2] = { 0.0f, 0.0f };
    char vNumber[8] = { '\0' };
    char vRegion[4] = { '\0' };
    char vNameCar[50] = { '\0' };
    char vNameCar2[20] = { '\0' };
    int vUIDradio = -1;
    float vHandling[64] = { 0.0f };          // хендлинг Arizona (RPC 161 kind 0): значение по id параметра
    unsigned long long vHandlingMask = 0;    // какие id заданы
};

extern VehicleInfo vTemp[2000];

struct InfoServer
{
    int iServerFlag = 0;
    int iServerLogo = 11;
    int iXPayDay = 0;
    int iServerNumber = 0;
    char iServerName[50] = "Test";
    char iServerProject[50] = "Arizona";
    char iServerType[50] = "RolePlay";
};

extern InfoServer iServer;

struct ColorRGB
{
    uint8_t R, G, B;
};

extern const ColorRGB ColorRGBArray[];

extern const char* regionNames[];
struct WheelInfo { int model; const char* texture; };
extern const WheelInfo wheelTable[];
extern const int wheelTableSize;
const WheelInfo* FindWheel(int model); // nullptr - это не диск
extern const char* tonerNames[];
extern const char* wheelSettings[];

struct ChatMod
{
    int id;
    int color[3];
    std::string code;
    std::string message;
    std::string description;
};

extern const ChatMod chatMod[];

#endif // INFOSERVER_H
