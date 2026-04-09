

#include "Components/PoolableComponent.h"
#include "Components/ObjectPoolComponent.h"

UPoolableComponent::UPoolableComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UPoolableComponent::BeginPlay()
{
	Super::BeginPlay();
	
	SetInUse(false);
}

void UPoolableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UPoolableComponent::UsedFromPool()
{
	if ((bIsUsed != true) && (GetPoolManager() != nullptr))
	{
		SetInUse(true);

		// Устанавливаем таймер, через сколько объект вернется в пулл.
		GetWorld()->GetTimerManager().SetTimer(
			TimerLeave,
			this,
			&UPoolableComponent::ReturnToPool,
			TimeToLeave,
			false
		);

		OnUsedFromPool.Broadcast();
	}
}

void UPoolableComponent::ReturnToPool()
{
	if ((bIsUsed != false) && (GetPoolManager() != nullptr))
	{
		GetPoolManager()->ReleaseObject(this);
	}
}

void UPoolableComponent::ReturnedToPool()
{
	SetInUse(false);

	// Очищаем таймер.
	GetWorld()->GetTimerManager().ClearTimer(TimerLeave);

	OnReturnedToPool.Broadcast();
}

void UPoolableComponent::SetInUse(bool bNewStateUse)
{
	bIsUsed = bNewStateUse;

	GetOwner()->SetActorEnableCollision(bNewStateUse);
	GetOwner()->SetActorHiddenInGame(!bNewStateUse);
	GetOwner()->SetActorTickEnabled(bNewStateUse);
}

void UPoolableComponent::SetPoolManager(UObjectPoolComponent* NewPoolManager)
{
	PoolManager = NewPoolManager;
}