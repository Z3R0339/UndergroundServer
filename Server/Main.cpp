#include <Windows.h>
#include <iostream>
#include <fstream>
#include <print>

#include <Utils.hpp>
#include <Hook.hpp>

#include "Inventory.hpp"
#include "Abilities.hpp"
#include "Net.hpp"

#include <SDK/Event_CentralPicnic_Thumper_classes.hpp>
#include <SDK/CentralPicnic_MasterEventController_classes.hpp>
#include <SDK/B_MMObj_RiftPoiGameStateComponent_classes.hpp>
#include <SDK/Valet_BasicCar_Vehicle_classes.hpp>
#include <SDK/AscenderCodeRuntime_classes.hpp>
#include <SDK/WeaponModsCodeRuntime_classes.hpp>
#include <SDK/GameplayTags_classes.hpp>

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
    static auto AbilitySet = UObject::FindObject<UFortAbilitySet>("FortAbilitySet GAS_AthenaPlayer.GAS_AthenaPlayer");
    Abilities::GiveAbilitySet(PlayerState->AbilitySystemComponent, AbilitySet);

    auto AssetManager = Utils::GetAssetManager();
    Inventory::GiveItem(PlayerController, AssetManager->GameDataCosmetics->FallbackPickaxe->WeaponDefinition);
    Inventory::GiveItem(PlayerController, UFortKismetLibrary::K2_GetResourceItemDefinition(EFortResourceType::Wood));
    Inventory::GiveItem(PlayerController, UFortKismetLibrary::K2_GetResourceItemDefinition(EFortResourceType::Stone));
    Inventory::GiveItem(PlayerController, UFortKismetLibrary::K2_GetResourceItemDefinition(EFortResourceType::Metal));
    Inventory::GiveItem(PlayerController, UObject::FindObject<UFortResourceItemDefinition>("FortResourceItemDefinition Athena_WadsItemData.Athena_WadsItemData"));
    static auto ShockGrenade = UObject::FindObject<UFortWorldItemDefinition>("FortWeaponRangedItemDefinition Athena_ShockGrenade.Athena_ShockGrenade");
    Inventory::GiveItem(PlayerController, ShockGrenade);
    Inventory::Update(PlayerController);

    auto translivesmatter = StartSpot->GetTransform();
    translivesmatter.Translation = { 0, 0, 10000 };
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

        TSoftClassPtr<class UClass> SoftPtr;
        SoftPtr.ObjectID.AssetPath.PackageName = UKismetStringLibrary::Conv_StringToName(L"/WeaponModStation/Gameplay/Actors/BP_WeaponModStation_v2");
        SoftPtr.ObjectID.AssetPath.AssetName = UKismetStringLibrary::Conv_StringToName(L"BP_WeaponModStation_v2_C");
        static auto Class = UKismetSystemLibrary::LoadClassAsset_Blocking(SoftPtr);

        // auto Manager = UWorldPartitionBlueprintLibrary::GetDataLayerManager(UWorld::GetWorld());
        // auto Instances = Manager->GetDataLayerInstances();
        // for (auto Instance : Instances)
        // {
        //     Manager->SetDataLayerInstanceRuntimeState(Instance, EDataLayerRuntimeState::Activated, false);
        // }
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
    else if (Cmd == L"snipermod")
    {
        static UFortWorldItemDefinition* Snipers[] = {
            UObject::FindObject<UFortWorldItemDefinition>("FortWeaponRangedItemDefinition WID_Sniper_Paprika_Athena_UC.WID_Sniper_Paprika_Athena_UC"),
            UObject::FindObject<UFortWorldItemDefinition>("FortWeaponRangedItemDefinition WID_Sniper_Paprika_Athena_R.WID_Sniper_Paprika_Athena_R"),
            UObject::FindObject<UFortWorldItemDefinition>("FortWeaponRangedItemDefinition WID_Sniper_Paprika_Athena_VR.WID_Sniper_Paprika_Athena_VR"),
            UObject::FindObject<UFortWorldItemDefinition>("FortWeaponRangedItemDefinition WID_Sniper_Paprika_Athena_SR.WID_Sniper_Paprika_Athena_SR")
        };
        static auto ModSetData = UObject::FindObject<UFortWeaponModSetData>("FortWeaponModSetData WMSet_CovertOps.WMSet_CovertOps");

        for (int i = 0; i < 4; i++)
        {
            Inventory::GiveItem(PlayerController, Snipers[i], 1, ModSetData);
        }
        Inventory::Update(PlayerController);
    }
    else if (Cmd == L"car")
    {
        auto Loc = PlayerController->Pawn->K2_GetActorLocation();
        Loc.X += 5000;
        Loc.Y += 5000;
        Loc.Z += 5000;
        auto Car = Utils::SpawnActor<AValet_BasicCar_Vehicle_C>(Loc);

    }
    else if (Cmd == L"modstation")
    {
        static TSoftClassPtr<class UClass> SoftPtr;
        static bool uwu = true;
        if (uwu)
        {
            uwu = false;

            SoftPtr.ObjectID.AssetPath.PackageName = UKismetStringLibrary::Conv_StringToName(L"/WeaponModStation/Gameplay/Actors/BP_WeaponModStation_v2");
            SoftPtr.ObjectID.AssetPath.AssetName = UKismetStringLibrary::Conv_StringToName(L"BP_WeaponModStation_v2_C");
        }
        static auto Class = UKismetSystemLibrary::LoadClassAsset_Blocking(SoftPtr);
        Utils::SpawnActor(Class, PlayerController->Pawn->K2_GetActorLocation() + (PlayerController->Pawn->GetActorForwardVector() * 1000));
    }
    else if (Cmd == L"crash")
    {
        ((AFortPlayerControllerAthena*)UWorld::GetWorld())->WorldInventory->HandleInventoryLocalUpdate();
    }
}

void ServerMoveHook(AFortPhysicsPawn* Pawn, const FReplicatedPhysicsPawnState& InState)
{
    auto Component = (UPrimitiveComponent*)Pawn->GetComponentByClass(UPrimitiveComponent::StaticClass());
    Component->K2_SetWorldLocationAndRotation((FVector)InState.Translation, UKismetMathLibrary::Quat_Rotator(InState.Rotation), false, nullptr, true);
    Component->SetAllPhysicsLinearVelocity(InState.LinearVelocity, false);
    Component->SetAllPhysicsAngularVelocityInDegrees(InState.AngularVelocity, false);
}

void ServerPurchaseWeaponModForWeaponHook(UFortWeaponModStationComponent* Component, UFortWeaponModItemDefinition* WeaponMod, AFortWeapon* Weapon)
{
    auto PlayerController = (AFortPlayerControllerAthena*)Component->GetOwner();

    if (!Component->CanPlayerAffordModForWeapon(WeaponMod, Weapon, PlayerController))
        return;

    bool added = false;
    for (auto& Slot : Weapon->WeaponModSlots)
    {
        if (UBlueprintGameplayTagLibrary::EqualEqual_GameplayTag(WeaponMod->ModSlot, Slot.WeaponMod->ModSlot))
        {
            Slot.WeaponMod = WeaponMod;
            added = true;
        }
    }

    if (!added)
    {
        Weapon->WeaponModSlots.Add({ WeaponMod, true });
    }

    // TODO Add player name to weapon name

    static auto GoldItemDef = UObject::FindObject<UFortResourceItemDefinition>("FortResourceItemDefinition Athena_WadsItemData.Athena_WadsItemData");

    // TODO Get real mod cost
    Inventory::RemoveItem(PlayerController, GoldItemDef, 75);
}

void (*ServerStopInteractWithWorkbenchActorOriginal)(UFortWeaponModStationComponent* Component, AFortWeaponModStationBase* NewInteractingWeaponModStation);
void ServerStopInteractWithWorkbenchActorHook(UFortWeaponModStationComponent* Component, AFortWeaponModStationBase* NewInteractingWeaponModStation)
{
    auto PlayerController = (AFortPlayerControllerAthena*)Component->GetOwner();
    auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;

    for (auto Ability : PlayerState->AbilitySystemComponent->ActivatableAbilities.Items)
    {
        static UClass* AbilityClass = UObject::FindClass("BlueprintGeneratedClass GA_WeaponModStation_Camera_v2.GA_WeaponModStation_Camera_v2_C");
        if (Ability.Ability->IsA(AbilityClass))
        {
            PlayerState->AbilitySystemComponent->ClearAbility(Ability.Handle);
            break;
        }
    }

    ServerStopInteractWithWorkbenchActorOriginal(Component, NewInteractingWeaponModStation);
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
    Utils::ExecuteConsoleCommand(L"log LogWeaponModStation VeryVerbose");

    Hook::Function(InSDKUtils::GetImageBase() + 0x3641180, ReturnHook); // RequestExit
    Hook::Function(InSDKUtils::GetImageBase() + 0x38930E0, ReturnHook); // GameSession crash
    Hook::Function(InSDKUtils::GetImageBase() + 0x118C5B0, Net::GetNetModeHook);
    Hook::Function(InSDKUtils::GetImageBase() + 0x63AD804, Net::KickPlayerHook);
    Hook::Function(InSDKUtils::GetImageBase() + 0x19C6B78, Net::TickFlushHook, &Net::TickFlushOriginal);

    Hook::VTable<AFortGameModeBR>(2328 / 8, ReadyToStartMatchHook, &ReadyToStartMatchOriginal);
    Hook::VTable<AFortGameModeBR>(1832 / 8, SpawnDefaultPawnForHook);
    Hook::VTable<AFortPlayerControllerAthena>(2416 / 8, ServerAcknowledgePossessionHook);
    Hook::VTable<AFortPlayerControllerAthena>(4000 / 8, ServerCheatHook);
    Hook::VTable<AFortPlayerControllerAthena>(4432 / 8, Inventory::ServerExecuteInventoryItemHook);
    Hook::VTable<AFortPlayerControllerAthena>(4520 / 8, Inventory::ServerAttemptInventoryDrop);
    Hook::VTable<AFortPlayerPawnAthena>(4616 / 8, Inventory::ServerHandlePickup);
    Hook::VTable<UFortWeaponModStationComponent>(1304 / 8, ServerPurchaseWeaponModForWeaponHook);
    Hook::VTable<UFortWeaponModStationComponent>(1320 / 8, ServerStopInteractWithWorkbenchActorHook, &ServerStopInteractWithWorkbenchActorOriginal);

    for (int i = 0; i < UObject::GObjects->Num(); i++)
    {
        auto Object = UObject::GObjects->GetByIndex(i);
        if (!Object || !Object->HasTypeFlag(EClassCastFlags::Class)) continue;

        if (((UClass*)Object)->IsSubclassOf(UAbilitySystemComponent::StaticClass()))
        {
            Hook::VTable((void**)((UClass*)Object)->DefaultObject->VTable, 2240 / 8, Abilities::InternalServerTryActivateAbilityHook);
        }
    }
    // Hook::VTable<UFortAbilitySystemComponentAthena>(2240 / 8, Abilities::InternalServerTryActivateAbilityHook);
    Hook::VTable<AFortDagwoodVehicle>(2192 / 8, ServerMoveHook);

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
