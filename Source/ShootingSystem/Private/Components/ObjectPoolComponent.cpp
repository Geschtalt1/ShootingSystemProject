

#include "Components/ObjectPoolComponent.h"
#include "Components/PoolableComponent.h"

UObjectPoolComponent::UObjectPoolComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UObjectPoolComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (bIsPoolReady)
	{
		InitializePool();
	}
}

void UObjectPoolComponent::InitializePool()
{
	RefreshObjectPool();
}

bool UObjectPoolComponent::SpawnPooledObject(const FTransform& SpawnTransform, AActor* Instigator, UPoolableComponent*& PooledObject)
{
	// Находим доступный объект.
	const bool bFound = FindAvailableObject(PooledObject);
	
	if (!bFound)
	{
		// Если объект не найден, проверяем что в пулле еще есть место.
		if (HasCapacity())
		{
			// Создаем новый объект.
			PooledObject = CreateReusableObject();
		}
	}

	if (!PooledObject)
	{
		OnPoolDepleted.Broadcast();
		return false;
	}

	AActor* OwnerLoc = PooledObject->GetOwner();

	// Устанавливаем transform владельцу компонента PoolableComp.
	OwnerLoc->SetActorTransform(SpawnTransform);
	PooledObject->UsedFromPool();

	OnSpawnedObjectFromPool.Broadcast(PooledObject, SpawnTransform, Instigator);
	return true;
}

bool UObjectPoolComponent::ReleaseObject(UPoolableComponent* PoolableObject)
{
	if (ObjectPool.Contains(PoolableObject))
	{
		PoolableObject->ReturnedToPool();
		OnReturnedObjectToPool.Broadcast(PoolableObject);

		return true;
	}
	return false;
}

void UObjectPoolComponent::ClearPool()
{ 
	for (auto& Object : ObjectPool)
	{
		if (Object)
		{
			Object->GetOwner()->Destroy();
		}
	}

	ObjectPool.Empty();
}

void UObjectPoolComponent::RefreshObjectPool()
{
	ClearPool();

	for (int32 i = 0; i < InitialPoolSize; i++)
	{
		CreateReusableObject();
	}
}

bool UObjectPoolComponent::FindAvailableObject(UPoolableComponent*& PoolableComp) const
{
	for (auto& Object : ObjectPool)
	{
		if ((Object != nullptr) && (Object->IsUsedObject() != true))
		{
			PoolableComp = Object;
			return true;
		}
	}
	PoolableComp = nullptr;
	return false;
}

UPoolableComponent* UObjectPoolComponent::CreateReusableObject()
{
	if (!PooledActorClass)
	{
		return nullptr;
	}

	// Спавним новый актор.
	AActor* NewActor = GetWorld()->SpawnActor<AActor>(PooledActorClass, FTransform());

	// Пытаемся найти PoolableComponent в акторе.
	UPoolableComponent* NewReusableObj = NewActor->FindComponentByClass<UPoolableComponent>();
	if (!NewReusableObj)
	{
		// Добавляем компонент, если его не было по умолчанию.
		UActorComponent* NewComponent = NewActor->AddComponentByClass(
			UPoolableComponent::StaticClass(),
			true,
			FTransform(),
			false
		);

		NewReusableObj = Cast<UPoolableComponent>(NewComponent);
	}

	// Инициализиурем новый объект.
	NewReusableObj->SetInUse(false);
	NewReusableObj->SetPoolManager(this);

	ObjectPool.AddUnique(NewReusableObj);

	return NewReusableObj;
}


int32 UObjectPoolComponent::GetAvailableObjectCount() const
{
	int32 Count = 0;
	for (auto& Object : ObjectPool)
	{
		if ((Object != nullptr) && (Object->IsUsedObject() != true))
		{
			Count++;
		}
	}
	return Count;
}

int32 UObjectPoolComponent::GetActiveObjectCount() const
{
	int32 Count = 0;
	for (auto& Object : ObjectPool)
	{
		if ((Object != nullptr) && (Object->IsUsedObject() != false))
		{
			Count++;
		}
	}
	return Count;
}

void UObjectPoolComponent::SetPooledActorClass(TSubclassOf<AActor> NewPooledClass, bool bRefreshPool)
{
	if (NewPooledClass != nullptr)
	{
		PooledActorClass = NewPooledClass;
		if (bRefreshPool)
		{
			RefreshObjectPool();
		}
	}
}

void UObjectPoolComponent::SetPoolReady(bool bReady)
{
	bIsPoolReady = bReady;
}

void UObjectPoolComponent::SetInitialPoolSize(int32 NewInitialSize)
{
	InitialPoolSize = NewInitialSize;
}

void UObjectPoolComponent::SetMaxPoolSize(int32 NewMaxPoolSize)
{
	MaxPoolSize = NewMaxPoolSize;
}