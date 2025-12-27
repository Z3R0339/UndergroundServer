namespace Inventory
{
    void GiveItem(AFortPlayerControllerAthena* PlayerController, UFortWorldItemDefinition* ItemDef, int32 Count = 1)
    {
        auto Item = (UFortWorldItem*)ItemDef->CreateTemporaryItemInstanceBP(Count, 1);
        PlayerController->WorldInventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
        PlayerController->WorldInventory->Inventory.ItemInstances.Add(Item);
    }

    void Update(AFortPlayerControllerAthena* PlayerController)
    {
        PlayerController->WorldInventory->HandleInventoryLocalUpdate();
        Utils::MarkArrayDirty(&PlayerController->WorldInventory->Inventory);
    }
    
    void ServerExecuteInventoryItemHook(AFortPlayerControllerAthena* PlayerController, const FGuid& ItemGuid)
    {
        auto Pawn = (AFortPlayerPawnAthena*)PlayerController->Pawn;

        for (int i = 0; i < PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
        {
            auto Entry = PlayerController->WorldInventory->Inventory.ReplicatedEntries[i];
            if (UKismetGuidLibrary::EqualEqual_GuidGuid(Entry.ItemGuid, ItemGuid))
            {
                auto Weapon = Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry.ItemDefinition, ItemGuid, {}, false);
                if (Entry.ItemDefinition->IsA(UFortWeaponItemDefinition::StaticClass()))
                {
                    auto WeaponDef = (UFortWeaponItemDefinition*)Entry.ItemDefinition;
                    Weapon->WeaponModSlots = WeaponDef->WeaponModSlots;
                }
            }
        }
    }
}
