#include <Windows.h>
#include <iostream>
#include <fstream>
#include <print>

#include <Utils.hpp>
#include <Hook.hpp>

#include <SDK/Event_CentralPicnic_Thumper_classes.hpp>
#include <SDK/CentralPicnic_MasterEventController_classes.hpp>
#include <SDK/B_MMObj_RiftPoiGameStateComponent_classes.hpp>
#include <SDK/Valet_BasicCar_Vehicle_classes.hpp>
#include <SDK/AscenderCodeRuntime_classes.hpp>
#include <SDK/WeaponModsCodeRuntime_classes.hpp>
#include <SDK/GameplayTags_classes.hpp>

#define MessageBox(...) MessageBoxA(NULL, std::format(__VA_ARGS__).c_str(), "UndergroundServer", MB_OK)

#define DisableMME
// #define SkipAircraft

// DataTable ModSetBucketsBaseTable.ModSetBucketsBaseTable
// FortWeaponModSetRow

#include "Inventory.hpp"
#include "Abilities.hpp"
#include "GameLogic.hpp"
#include "Net.hpp"
#include "Vehicles.hpp"
#include "ModStation.hpp"
#include "Building.hpp"
#include "Commands.hpp"

void ReturnHook()
{
    return;
}

bool (*ReadyToStartMatchOriginal)(AFortGameModeBR* GameMode);
bool ReadyToStartMatchHook(AFortGameModeBR* GameMode)
{
    static bool Started = false;
    if (!Started)
    {
        Started = true;

        auto GameState = (AFortGameStateBR*)GameMode->GameState;
        auto Playlist = UObject::FindObject<UFortPlaylistAthena>("FortPlaylistAthena Playlist_DefaultSolo.Playlist_DefaultSolo");
        Playlist->GarbageCollectionFrequency = FLT_MAX;
        GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
        GameState->OnRep_CurrentPlaylistInfo();

        GameState->bIsUsingDownloadOnDemand = false;
        GameState->OnRep_IsUsingDownloadOnDemand();

        auto Logic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());
#ifdef SkipAircraft
        Logic->GamePhase = EAthenaGamePhase::None;
#else
        Logic->GamePhase = EAthenaGamePhase::Warmup;
#endif
        Logic->OnRep_GamePhase(EAthenaGamePhase::Setup);

        // TODO Figure out why CreateNetDriver doesn't work
        // FWorldContext* (*GetWorldContext)(UEngine*, UWorld*) = decltype(GetWorldContext)(InSDKUtils::GetImageBase() + 0x12C8A10);
        // UNetDriver* (*CreateNetDriver)(UEngine*, FWorldContext*, FName, FName) = decltype(CreateNetDriver)(InSDKUtils::GetImageBase() + 0x1FD12D0);
        bool (*InitHost)(AOnlineBeaconHost*) = decltype(InitHost)(InSDKUtils::GetImageBase() + 0x6897A00);
        bool (*PauseBeaconRequests)(AOnlineBeaconHost*, bool) = decltype(PauseBeaconRequests)(InSDKUtils::GetImageBase() + 0x92D6F90);
        bool (*InitListen)(UNetDriver*, UWorld*, FURL&, bool, FString&) = decltype(InitListen)(InSDKUtils::GetImageBase() + 0x6898440);
        void (*SetWorld)(UNetDriver*, UWorld*) = decltype(SetWorld)(InSDKUtils::GetImageBase() + 0x242FAFC);

        auto Beacon = Utils::SpawnActor<AFortOnlineBeaconHost>();
        Beacon->ListenPort = 7777;

        InitHost(Beacon);
        PauseBeaconRequests(Beacon, false);

        auto NetDriver = Beacon->NetDriver;
        auto World = NetDriver->World;
        World->NetDriver = NetDriver;
        *(bool*)(int64(NetDriver) + 0x751) = true;
        NetDriver->NetDriverName = UKismetStringLibrary::Conv_StringToName(L"GameNetDriver");

        FURL Url = {};
        Url.Port = 7776;
        FString Error;

        InitListen(NetDriver, NetDriver->World, Url, false, Error);
        SetWorld(NetDriver, NetDriver->World);

        World->LevelCollections[0].NetDriver = NetDriver;
        World->LevelCollections[1].NetDriver = NetDriver;

        GameMode->bWorldIsReady = true;

        World->ServerStreamingLevelsVisibility = Utils::SpawnActor<AServerStreamingLevelsVisibility>();
    }

    return ReadyToStartMatchOriginal(GameMode);
}

APawn* SpawnDefaultPawnForHook(AFortGameModeBR* GameMode, AFortPlayerControllerAthena* PlayerController, AActor* StartSpot)
{
    PlayerController->WorldInventory = Utils::SpawnActor<AFortInventory>({}, PlayerController);

    auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;
    static auto AbilitySet = UObject::FindObject<UFortAbilitySet>("FortAbilitySet GAS_AthenaPlayer.GAS_AthenaPlayer");
    Abilities::GiveAbilitySet(PlayerState->AbilitySystemComponent, AbilitySet);

    static auto TacSprint = UObject::FindClassFast("GA_Athena_TacticalSprint_C");
    static auto HillScramble = Utils::LoadClass(L"/HillScramble/Gameplay/Abilities/GA_Athena_Player_HillScramble", L"GA_Athena_Player_HillScramble_C");
    PlayerState->AbilitySystemComponent->K2_GiveAbility(TacSprint, 1, 1);
    PlayerState->AbilitySystemComponent->K2_GiveAbility(HillScramble, 1, 1);

    auto AssetManager = Utils::GetAssetManager();
    Inventory::GiveItem(PlayerController, Utils::GetSoftPtr(AssetManager->GameDataCommon->EditToolItem));

    Inventory::GiveItem(PlayerController, Utils::GetSoftPtr(AssetManager->GameDataCosmetics->FallbackPickaxe)->WeaponDefinition);
    Inventory::GiveItem(PlayerController, Utils::GetSoftPtr(AssetManager->GameDataBR->DefaultGlobalCurrencyItemDefinition));

    Inventory::GiveItem(PlayerController, UFortKismetLibrary::K2_GetResourceItemDefinition(EFortResourceType::Wood));
    Inventory::GiveItem(PlayerController, UFortKismetLibrary::K2_GetResourceItemDefinition(EFortResourceType::Stone));
    Inventory::GiveItem(PlayerController, UFortKismetLibrary::K2_GetResourceItemDefinition(EFortResourceType::Metal));

    Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWorldItemDefinition>("AthenaAmmoDataShells"));
    Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWorldItemDefinition>("AthenaAmmoDataBulletsLight"));
    Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWorldItemDefinition>("AthenaAmmoDataBulletsMedium"));
    Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWorldItemDefinition>("AthenaAmmoDataBulletsHeavy"));
    Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWorldItemDefinition>("AmmoDataRockets"));

    Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWorldItemDefinition>("WID_Assault_Paprika_Infantry_Athena_HS_UR_Boss"));
    Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWorldItemDefinition>("WID_Shotgun_Auto_Paprika_Athena_UR_Boss"));
    Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWorldItemDefinition>("WID_SMG_Paprika_Burst_Athena_HS_SR"));
    // Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWorldItemDefinition>("WID_Sniper_Paprika_Athena_SR"));
    Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWorldItemDefinition>("Athena_ShockGrenade"));
    Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWorldItemDefinition>("WID_Paprika_TeamSpray_LowGrav"));

    for (int i = 0; i < 4; i++)
    {
        auto thing = GameMode->StartingItems[i];
        Inventory::GiveItem(PlayerController, (UFortWorldItemDefinition*)thing.Item, thing.Count);
    }

    static auto ShockGrenade = UObject::FindObject<UFortWorldItemDefinition>("FortWeaponRangedItemDefinition Athena_ShockGrenade.Athena_ShockGrenade");
    Inventory::GiveItem(PlayerController, ShockGrenade);
    Inventory::Update(PlayerController);

    auto translivesmatter = StartSpot->GetTransform();
#ifdef SkipAircraft
    translivesmatter.Translation = { 0, 0, 3000 };
#endif
    auto ret = GameMode->SpawnDefaultPawnAtTransform(PlayerController, translivesmatter);
    ret->bCanBeDamaged = false;

    return ret;
}

static inline bool FullyInited = false;

void ServerAcknowledgePossessionHook(AFortPlayerControllerAthena* PlayerController, APawn* P)
{
    PlayerController->AcknowledgedPawn = P;

    if (!FullyInited)
    {
        FullyInited = true;

        auto EventControllers = Utils::GetAllActorsOfClass<ACentralPicnic_MasterEventController_C>();
        if (EventControllers.Num() > 0)
        {
            auto EventController = EventControllers[0];
            EventController->CentralPicnic_Layer_Pre();
        }
        EventControllers.Free();

        Utils::LoadClass(L"/WeaponModStation/Gameplay/Actors/BP_WeaponModStation_v2", L"BP_WeaponModStation_v2_C");
        Vehicles::Spawn();

        GameLogic::InitWarmupTimer();
    }
}

// This function gets called when you shoot and probably is the reason shooting most ch5 guns do nothing
// I reversed all the args thanks to it creating a FFortLightweightProjectileRequest 
// but after messing around a bit idk what to do with it rn so for now using hitscan is good!
//
// InSDKUtils::GetImageBase() + 0x2788BEC
// void RequestProjectile(
//     AFortLightweightProjectileManager* Manager, 
//     TWeakObjectPtr<AActor> Requester, 
//     TWeakObjectPtr<class AActor> FiringWeapon, 
//     TSubclassOf<AFortLightweightProjectileConfig>& ProjectileConfigClass,
//     const FVector& StartPosition,
//     const FVector& StartDirection,
//     ELightweightProjectileRequestType RequestType,
//     float TimeStamp,
//     float TimeBetweenShots
// )

DWORD MainThread(HMODULE Module)
{
    AllocConsole();
    FILE* Dummy;
    freopen_s(&Dummy, "CONOUT$", "w", stdout);
    freopen_s(&Dummy, "CONIN$", "r", stdin);

    FMemory::Init((void*)(InSDKUtils::GetImageBase() + 0x43DC21C));

    MH_Initialize();

    Utils::ExecuteConsoleCommand(L"log LogFortUIDirector None");
    Utils::ExecuteConsoleCommand(L"log LogWeaponModStation VeryVerbose");

    Hook::Function(InSDKUtils::GetImageBase() + 0x3641180, ReturnHook); // RequestExit
    Hook::Function(InSDKUtils::GetImageBase() + 0x38930E0, ReturnHook); // GameSession crash
    Hook::Function(InSDKUtils::GetImageBase() + 0x118C5B0, Net::GetNetModeHook);
    Hook::Function(InSDKUtils::GetImageBase() + 0x118905C, Net::GetNetModeHook);
    Hook::Function(InSDKUtils::GetImageBase() + 0x63AD804, Net::KickPlayerHook);
    Hook::Function(InSDKUtils::GetImageBase() + 0x19C6B78, Net::TickFlushHook, &Net::TickFlushOriginal);

    Hook::Function(InSDKUtils::GetImageBase() + 0x967392C, Inventory::RemoveInventoryItem);

    Hook::VTable<AFortGameModeBR>(2328 / 8, ReadyToStartMatchHook, &ReadyToStartMatchOriginal);
    Hook::VTable<AFortGameModeBR>(1832 / 8, SpawnDefaultPawnForHook);

    Hook::VTable<AFortPlayerControllerAthena>(2416 / 8, ServerAcknowledgePossessionHook);
    Hook::VTable<AFortPlayerControllerAthena>(4000 / 8, Commands::ServerCheatHook);
    Hook::VTable<AFortPlayerControllerAthena>(4432 / 8, Inventory::ServerExecuteInventoryItemHook);
    Hook::VTable<AFortPlayerControllerAthena>(4520 / 8, Inventory::ServerAttemptInventoryDrop);
    Hook::VTable<AFortPlayerControllerAthena>(4688 / 8, Building::ServerCreateBuildingActorHook);
    Hook::VTable<AFortPlayerControllerAthena>(4744 / 8, Building::ServerBeginEditingBuildingActor);
    Hook::VTable<AFortPlayerControllerAthena>(4728 / 8, Building::ServerEndEditingBuildingActor);
    Hook::VTable<AFortPlayerControllerAthena>(4704 / 8, Building::ServerEditBuildingActor);
    Hook::VTable<AFortPlayerControllerAthena>(4656 / 8, Building::ServerRepairBuildingActor);

    Hook::VTable<AFortPlayerPawnAthena>(4616 / 8, Inventory::ServerHandlePickup);

    Hook::VTable<UFortWeaponModStationComponent>(1304 / 8, ModStation::ServerPurchaseWeaponModForWeaponHook);
    Hook::VTable<UFortWeaponModStationComponent>(1320 / 8, ModStation::ServerStopInteractWithWorkbenchActorHook, &ModStation::ServerStopInteractWithWorkbenchActorOriginal);

    Hook::AllVTables<UAbilitySystemComponent>(2240 / 8, Abilities::InternalServerTryActivateAbilityHook);
    Hook::AllVTables<AFortPhysicsPawn>(2192 / 8, Vehicles::ServerMoveHook);

    *(bool*)(InSDKUtils::GetImageBase() + 0x1164007B) = false; // GIsClient
    *(bool*)(InSDKUtils::GetImageBase() + 0x1164000D) = true; // GIsServer
    UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);

    Utils::ExecuteConsoleCommand(L"net.AllowEncryption 0");
    *(bool*)(InSDKUtils::GetImageBase() + 0x117E1128) = true;

#ifdef DisableMME
    *(bool*)(InSDKUtils::GetImageBase() + 0x115845E3) = false; // Fort.MME.Clambering
    Utils::ExecuteConsoleCommand(L"Fort.MME.TacticalSprint 0");
#endif

    Utils::ExecuteConsoleCommand(L"open Helios_Terrain");

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
        break;
    }

    return TRUE;
}
