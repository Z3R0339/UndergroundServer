namespace Building
{
    void ServerCreateBuildingActorHook(AFortPlayerControllerAthena* PlayerController, const FCreateBuildingActorData& CreateBuildingData)
    {
        static auto GameState = (AFortGameStateAthena*)Utils::GetGameMode()->GameState;
        auto BuildClass = GameState->AllPlayerBuildableClasses[CreateBuildingData.BuildingClassHandle];
        auto Build = Utils::SpawnActor<ABuildingSMActor>(BuildClass, CreateBuildingData.BuildLoc, nullptr, CreateBuildingData.BuildRot);
        Build->InitializeKismetSpawnedBuildingActor(Build, PlayerController, true, nullptr, true);

        // TODO Remove colliding ABuildActors
    }
}
