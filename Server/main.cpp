#include <Windows.h>
#include <iostream>
#include <fstream>

#include <Utils.hpp>
#include <Hook.hpp>

#include "Inventory.hpp"

#include <SDK/Event_CentralPicnic_Thumper_classes.hpp>
#include <SDK/CentralPicnic_MasterEventController_classes.hpp>

void ReturnHook()
{
    return;
}

int64 GetNetModeHook(int64 a1)
{
    return 1;
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
        GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
        GameState->OnRep_CurrentPlaylistInfo();

        GameState->bIsUsingDownloadOnDemand = false;
        GameState->OnRep_IsUsingDownloadOnDemand();

        auto Logic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());
        Logic->GamePhase = EAthenaGamePhase::None;
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
    auto AbilitySet = UObject::FindObject<UFortAbilitySet>("FortAbilitySet GAS_AthenaPlayer.GAS_AthenaPlayer");
    for (int i = 0; i < AbilitySet->GameplayAbilities.Num(); i++)
    {
        PlayerState->AbilitySystemComponent->K2_GiveAbility(AbilitySet->GameplayAbilities[i], 1, 1);
    }

    auto AssetManager = Utils::GetAssetManager();
    Inventory::GiveItem(PlayerController, AssetManager->GameDataCosmetics->FallbackPickaxe->WeaponDefinition);
    Inventory::GiveItem(PlayerController, UFortKismetLibrary::K2_GetResourceItemDefinition(EFortResourceType::Wood), 500);
    Inventory::GiveItem(PlayerController, UFortKismetLibrary::K2_GetResourceItemDefinition(EFortResourceType::Stone), 500);
    Inventory::GiveItem(PlayerController, UFortKismetLibrary::K2_GetResourceItemDefinition(EFortResourceType::Metal), 500);
    Inventory::Update(PlayerController);

    auto translivesmatter = StartSpot->GetTransform();
    translivesmatter.Translation = { 0, 0, 10000 };
    auto ret = GameMode->SpawnDefaultPawnAtTransform(PlayerController, translivesmatter);
    ret->bCanBeDamaged = false;
    return ret;
}

void ServerAcknowledgePossessionHook(AFortPlayerControllerAthena* PlayerController, APawn* P)
{
    PlayerController->AcknowledgedPawn = P;

    static bool InitedStuff = false;
    if (!InitedStuff)
    {
        InitedStuff = true;

        auto EventControllers = Utils::GetAllActorsOfClass<ACentralPicnic_MasterEventController_C>();
        if (EventControllers.Num() > 0)
        {
            auto EventController = EventControllers[0];
            EventController->CentralPicnic_Layer_Pre();
        }
        EventControllers.Free();
    }
}

void SendClientMoveAdjustments(UNetDriver* NetDriver)
{
    for (UNetConnection* Connection : NetDriver->ClientConnections)
    {
        if (Connection == nullptr || Connection->ViewTarget == nullptr)
        {
            continue;
        }

        static void (*SendClientAdjustment)(APlayerController*) = decltype(SendClientAdjustment)(InSDKUtils::GetImageBase() + 0x663CE28);

        if (APlayerController* PC = Connection->PlayerController)
        {
            SendClientAdjustment(PC);
        }

        for (UNetConnection* ChildConnection : Connection->Children)
        {
            if (ChildConnection == nullptr)
            {
                continue;
            }

            if (APlayerController* PC = ChildConnection->PlayerController)
            {
                SendClientAdjustment(PC);
            }
        }
    }
}

enum class EReplicationSystemSendPass : unsigned
{
    Invalid,
    PostTickDispatch,
    TickFlush,
};

struct FSendUpdateParams
{
    EReplicationSystemSendPass SendPadd;
    float DeltaSeconds;
};

void (*TickFlushOriginal)(UNetDriver* NetDriver, float DeltaSeconds);
void TickFlushHook(UNetDriver* NetDriver, float DeltaSeconds)
{
    auto ReplicationSystem = *(UReplicationSystem**)(int64(NetDriver) + 0x748);

    if (NetDriver->ClientConnections.Num() > 0 && ReplicationSystem)
    {
        static void (*UpdateIrisReplicationViews)(UNetDriver*) = decltype(UpdateIrisReplicationViews)(InSDKUtils::GetImageBase() + 0x6577FB4);
        static void (*PreSendUpdate)(UReplicationSystem*, const FSendUpdateParams&) = decltype(PreSendUpdate)(InSDKUtils::GetImageBase() + 0x58675D8);

        UpdateIrisReplicationViews(NetDriver);
        SendClientMoveAdjustments(NetDriver);
        PreSendUpdate(ReplicationSystem, { EReplicationSystemSendPass::TickFlush, DeltaSeconds });
    }

    TickFlushOriginal(NetDriver, DeltaSeconds);
}

bool KickPlayerHook(int64 a1, int64 a2, int64 a3)
{
    return false;
}

void InternalServerTryActivateAbilityHook(UAbilitySystemComponent* Component, FGameplayAbilitySpecHandle Handle, 
        bool InputPressed, const FPredictionKey& PredictionKey, const FGameplayEventData* TriggerEventData)
{
    static FGameplayAbilitySpec* (*FindAbilitySpecFromHandle)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle)
        = decltype(FindAbilitySpecFromHandle)(InSDKUtils::GetImageBase() + 0x1EA3DB8);

    FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Component, Handle);
    if (!Spec)
    {
        Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
        return;
    }

    const UGameplayAbility* AbilityToActivate = Spec->Ability;

    if (!AbilityToActivate)
    {
        Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
        return;
    }

    if (AbilityToActivate->NetSecurityPolicy == EGameplayAbilityNetSecurityPolicy::ServerOnlyExecution ||
        AbilityToActivate->NetSecurityPolicy == EGameplayAbilityNetSecurityPolicy::ServerOnly)
    {
        Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
        return;
    }

    UGameplayAbility* InstancedAbility = nullptr;
    Spec->InputPressed = true;

    static bool (*InternalTryActivateAbility)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle Handle, FPredictionKey InPredictionKey, 
        UGameplayAbility** OutInstancedAbility, void* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData)
        = decltype(InternalTryActivateAbility)(InSDKUtils::GetImageBase() + 0x724A168);

    if (InternalTryActivateAbility(Component, Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
    {
    }
    else
    {
        Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
        Spec->InputPressed = false;
        Utils::MarkArrayDirty(&Component->ActivatableAbilities);
    }
}

void ServerCheatHook(AFortPlayerControllerAthena* PlayerController, const FString& FMsg)
{
    auto Cmd = FMsg.ToWString();
    auto CmdA = FMsg.ToString();
    if (Cmd.starts_with(L"server "))
    {
        Utils::ExecuteConsoleCommand(Cmd.substr(7).c_str());
    }
    else if (Cmd == L"dumpobjects")
    {
        std::ofstream outfile("objects.txt");
        for (int i = 0; i < UObject::GObjects->Num(); i++)
        {
            auto Object = UObject::GObjects->GetByIndex(i);
            if (!Object) continue;
            outfile << Object->GetFullName() << '\n';
        }
        outfile.close();
    }
    else if (Cmd.starts_with(L"giveitem "))
    {
        auto ItemDef = UObject::FindObject<UFortWorldItemDefinition>(CmdA.substr(9));
        Inventory::GiveItem(PlayerController, ItemDef);
        Inventory::Update(PlayerController);
    }
    else if (Cmd == L"event")
    {
        static int EventState = 0;
        if (EventState == 0)
        {
            auto Thumpers = Utils::GetAllActorsOfClass<AEvent_CentralPicnic_Thumper_C>();
            if (Thumpers.Num() > 0)
            {
                auto Thumper = Thumpers[0];
                Thumper->MulticastFistpump();
            }
            Thumpers.Free();
        }
        else if (EventState == 1)
        {
            auto ChainClass = UObject::FindClass("BlueprintGeneratedClass B_CentralPicnic_Chains.B_CentralPicnic_Chains_C");
            TArray<AActor*> Chains;
            UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), ChainClass, &Chains);
            if (Chains.Num() > 0)
            {
                auto Chain = Chains[0];
                auto DropBoxFunc = ChainClass->GetFunction("B_CentralPicnic_Chains_C", "DropTheBox");
                Chain->ProcessEvent(DropBoxFunc, nullptr);
            }
            Chains.Free();
        }

        EventState++;
    }
}

DWORD MainThread(HMODULE Module)
{
    AllocConsole();
    FILE* Dummy;
    freopen_s(&Dummy, "CONOUT$", "w", stdout);
    freopen_s(&Dummy, "CONIN$", "r", stdin);

    FMemory::Init((void*)(InSDKUtils::GetImageBase() + 0x43DC21C));

    MH_Initialize();

    Utils::ExecuteConsoleCommand(L"log LogFortUIDirector None");

    Hook::Function(InSDKUtils::GetImageBase() + 0x3641180, ReturnHook); // RequestExit
    Hook::Function(InSDKUtils::GetImageBase() + 0x38930E0, ReturnHook); // GameSession crash
    Hook::Function(InSDKUtils::GetImageBase() + 0x118C5B0, GetNetModeHook);
    Hook::Function(InSDKUtils::GetImageBase() + 0x63AD804, KickPlayerHook);
    Hook::Function(InSDKUtils::GetImageBase() + 0x19C6B78, TickFlushHook, &TickFlushOriginal);

    Hook::VTable<AFortGameModeBR>(2328 / 8, ReadyToStartMatchHook, &ReadyToStartMatchOriginal);
    Hook::VTable<AFortGameModeBR>(1832 / 8, SpawnDefaultPawnForHook);
    Hook::VTable<AFortPlayerControllerAthena>(2416 / 8, ServerAcknowledgePossessionHook);
    Hook::VTable<AFortPlayerControllerAthena>(4432 / 8, Inventory::ServerExecuteInventoryItemHook);
    Hook::VTable<AFortPlayerControllerAthena>(4000 / 8, ServerCheatHook);

    Hook::VTable<UFortAbilitySystemComponentAthena>(2240 / 8, InternalServerTryActivateAbilityHook);

    *(bool*)(InSDKUtils::GetImageBase() + 0x1164007B) = false; // GIsClient
    *(bool*)(InSDKUtils::GetImageBase() + 0x1164000D) = true; // GIsServer
    UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);

    Utils::ExecuteConsoleCommand(L"net.AllowEncryption 0");
    *(bool*)(InSDKUtils::GetImageBase() + 0x117E1128) = true;

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
