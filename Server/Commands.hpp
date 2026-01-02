namespace Commands
{
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
        // else if (Cmd == L"snipermod")
        // {
        //     static UFortWorldItemDefinition* Snipers[] = {
        //         UObject::FindObject<UFortWorldItemDefinition>("FortWeaponRangedItemDefinition WID_Sniper_Paprika_Athena_UC.WID_Sniper_Paprika_Athena_UC"),
        //         UObject::FindObject<UFortWorldItemDefinition>("FortWeaponRangedItemDefinition WID_Sniper_Paprika_Athena_R.WID_Sniper_Paprika_Athena_R"),
        //         UObject::FindObject<UFortWorldItemDefinition>("FortWeaponRangedItemDefinition WID_Sniper_Paprika_Athena_VR.WID_Sniper_Paprika_Athena_VR"),
        //         UObject::FindObject<UFortWorldItemDefinition>("FortWeaponRangedItemDefinition WID_Sniper_Paprika_Athena_SR.WID_Sniper_Paprika_Athena_SR")
        //     };
        //     static auto ModSetData = UObject::FindObject<UFortWeaponModSetData>("FortWeaponModSetData WMSet_CovertOps.WMSet_CovertOps");
    
        //     for (int i = 0; i < 4; i++)
        //     {
        //         Inventory::GiveItem(PlayerController, Snipers[i], 1, ModSetData);
        //     }
        //     Inventory::Update(PlayerController);
        // }
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
        else if (Cmd == L"givesniper")
        {
            static auto ItemDef = UObject::FindObject<UFortWorldItemDefinition>("FortWeaponRangedItemDefinition WID_Sniper_Paprika_Athena_SR.WID_Sniper_Paprika_Athena_SR");
            Inventory::GiveItem(PlayerController, ItemDef, 1);
            Inventory::Update(PlayerController);
        }
        else if (Cmd == L"crash")
        {
            ((AFortPlayerControllerAthena*)UWorld::GetWorld())->WorldInventory->HandleInventoryLocalUpdate();
        }
    }
}
