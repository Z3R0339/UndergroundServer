namespace ModStation
{
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
}
