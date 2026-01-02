#include <SDK/FortniteGame_classes.hpp>
using namespace SDK;

namespace Utils
{
    template <typename T>
    T* SpawnObject(UObject* Outer)
    {
        return (T*)UGameplayStatics::SpawnObject(T::StaticClass(), Outer);
    }

    template <typename T = AActor>
    T* SpawnActor(UClass* ActorClass, FVector Pos = {}, AActor* Owner = nullptr, FRotator Rot = {}, FVector Scale = { 1, 1, 1 })
    {
        auto translivesmatter = UKismetMathLibrary::MakeTransform(Pos, Rot, Scale);

        auto ret = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), ActorClass, translivesmatter, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner, ESpawnActorScaleMethod::MultiplyWithRoot);

        if (ret)
            ret = UGameplayStatics::FinishSpawningActor(ret, translivesmatter, ESpawnActorScaleMethod::MultiplyWithRoot);

        return (T*)ret;
    }

    template <typename T>
    T* SpawnActor(FVector Pos = {}, AActor* Owner = nullptr, FRotator Rot = {}, FVector Scale = { 1, 1, 1 })
    {
        return SpawnActor<T>(T::StaticClass(), Pos, Owner, Rot, Scale);
    }

    void ExecuteConsoleCommand(const wchar_t* Cmd)
    {
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), Cmd, nullptr);
    }

    void MarkArrayDirty(void* Arr)
    {
        static void (*NativeFunc)(void*) = decltype(NativeFunc)(InSDKUtils::GetImageBase() + 0x168F3EC);
        NativeFunc(Arr);
    }

    void MarkItemDirty(void* Arr, void* Item)
    {
        static void (*NativeFunc)(void*, void*) = decltype(NativeFunc)(InSDKUtils::GetImageBase() + 0x168F3C8);
        NativeFunc(Arr, Item);
    }

    UFortAssetManager* GetAssetManager()
    {
        return (UFortAssetManager*)UEngine::GetEngine()->AssetManager;
    }

    template <typename T>
    TArray<T*> GetAllActorsOfClass()
    {
        TArray<AActor*> ret;
        UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), T::StaticClass(), &ret);
        return *(TArray<T*>*)(&ret);
    }

    template <typename T = AActor>
    TArray<T*> GetAllActorsOfClass(UClass* ActorClass)
    {
        TArray<AActor*> ret;
        UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), ActorClass, &ret);
        return *(TArray<T*>*)(&ret);
    }

    AFortGameModeBR* GetGameMode()
    {
        return (AFortGameModeBR*)UWorld::GetWorld()->AuthorityGameMode;
    }

    template <typename T = UObject>
    T* LoadAsset(const wchar_t* PackageName, const wchar_t* AssetName)
    {
        TSoftObjectPtr<UObject> Softie;
        Softie.ObjectID.AssetPath.PackageName = UKismetStringLibrary::Conv_StringToName(PackageName);
        Softie.ObjectID.AssetPath.AssetName = UKismetStringLibrary::Conv_StringToName(AssetName);
        return (T*)UKismetSystemLibrary::LoadAsset_Blocking(Softie);
    }

    UClass* LoadClass(const wchar_t* PackageName, const wchar_t* AssetName)
    {
        TSoftClassPtr<UClass> Softie;
        Softie.ObjectID.AssetPath.PackageName = UKismetStringLibrary::Conv_StringToName(PackageName);
        Softie.ObjectID.AssetPath.AssetName = UKismetStringLibrary::Conv_StringToName(AssetName);
        return UKismetSystemLibrary::LoadClassAsset_Blocking(Softie);
    }

    template <typename T>
    T* GetSoftPtr(TSoftObjectPtr<T> SoftPtr)
    {
        auto ret = SoftPtr.Get();

        if (!ret)
            ret = (T*)UKismetSystemLibrary::LoadAsset_Blocking((TSoftObjectPtr<UObject>)SoftPtr);

        return (T*)ret;
    }

    UClass* GetSoftPtr(TSoftClassPtr<UClass>& SoftPtr)
    {
        auto ret = SoftPtr.Get();

        if (!ret)
            ret = UKismetSystemLibrary::LoadClassAsset_Blocking(SoftPtr);

        return ret;
    }

    template <typename T = UObject>
    T* FindObjectFast(const std::string& Name, EClassCastFlags RequiredType = EClassCastFlags::None, EClassCastFlags ExcludeType = EClassCastFlags::Package)
    {
        for (int i = 0; i < UObject::GObjects->Num(); ++i)
        {
            UObject* Object = UObject::GObjects->GetByIndex(i);
        
            if (!Object || Object->HasTypeFlag(ExcludeType))
                continue;
            
            if (Object->HasTypeFlag(RequiredType) && Object->GetName() == Name)
                return (T*)Object;
        }

        return nullptr;
    }
}
