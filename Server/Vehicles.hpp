namespace Vehicles
{
    void ServerMoveHook(AFortPhysicsPawn* Pawn, const FReplicatedPhysicsPawnState& InState)
    {
        auto Component = (UPrimitiveComponent*)Pawn->GetComponentByClass(UPrimitiveComponent::StaticClass());
        Component->K2_SetWorldLocationAndRotation((FVector)InState.Translation, UKismetMathLibrary::Quat_Rotator(InState.Rotation), false, nullptr, true);
        Component->SetAllPhysicsLinearVelocity(InState.LinearVelocity, false);
        Component->SetAllPhysicsAngularVelocityInDegrees(InState.AngularVelocity, false);
    }

    void Spawn()
    {
        auto SportsCar = Utils::LoadClass(L"/Valet/SportsCar/Valet_SportsCar_Vehicle", L"Valet_SportsCar_Vehicle_C");
        if (SportsCar)
        {
            auto SpawnerClass = UObject::FindClass("BlueprintGeneratedClass Valet_SportsCar_LWSpawner.Valet_SportsCar_LWSpawner_C");
            auto Spawners = Utils::GetAllActorsOfClass(SpawnerClass);
            for (auto Spawner : Spawners)
            {
                // TODO Random chance
                Utils::SpawnActor(SportsCar, Spawner->K2_GetActorLocation());
            }
            Spawners.Free();

            SpawnerClass = UObject::FindClass("BlueprintGeneratedClass Valet_SportsCar_AlwaysSpawn_LWSpawner.Valet_SportsCar_AlwaysSpawn_LWSpawner_C");
            Spawners = Utils::GetAllActorsOfClass(SpawnerClass);
            for (auto Spawner : Spawners)
            {
                Utils::SpawnActor(SportsCar, Spawner->K2_GetActorLocation());
            }
            Spawners.Free();
        }

        auto SUV = Utils::LoadClass(L"/BasicSUV/Vehicle/Valet_BasicSUV_Vehicle", L"Valet_BasicSUV_Vehicle_C");
        if (SUV)
        {
            auto SpawnerClass = UObject::FindClass("BlueprintGeneratedClass LW_BasicSUV_Spawner.LW_BasicSUV_Spawner_C");
            auto Spawners = Utils::GetAllActorsOfClass(SpawnerClass);
            for (auto Spawner : Spawners)
            {
                // TODO Random chance
                Utils::SpawnActor(SUV, Spawner->K2_GetActorLocation());
            }
            Spawners.Free();

            SpawnerClass = UObject::FindClass("BlueprintGeneratedClass LW_BasicSUV_AlwaysSpawner.LW_BasicSUV_AlwaysSpawner_C");
            Spawners = Utils::GetAllActorsOfClass(SpawnerClass);
            for (auto Spawner : Spawners)
            {
                Utils::SpawnActor(SUV, Spawner->K2_GetActorLocation());
            }
            Spawners.Free();
        }
    }
}
