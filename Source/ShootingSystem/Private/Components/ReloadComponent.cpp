

#include "Components/ReloadComponent.h"

UReloadComponent::UReloadComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UReloadComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UReloadComponent::StartReload()
{
	if (CanReload())
	{
		ReceiveStartReload();
		OnStartReloading.Broadcast();

		bIsReloading = true;

		// Обрабатываем перезарядку.
		HandleReload();
		return true;
	}
	return false;
}

bool UReloadComponent::CanReload() const
{
	return CanReloadInternal();
}

bool UReloadComponent::CanReloadInternal_Implementation() const
{
	return bCanReloadDefault && !IsReloading();
}

void UReloadComponent::HandleReload()
{
	HandleReloadInternal();
}

void UReloadComponent::HandleReloadInternal_Implementation()
{
	if (AnimInstance && ReloadMontage)
	{
		// Рассчитываем скорость проигрывания (PlayRate), чтобы анимация завершилась ровно за ReloadDuration.
		// PlayRate = Длительность_Монтажа / Желаемая_Длительность
		const float PlayRate = GetReloadTime();

		// Привязываем делегаты Notify Begin/End монтажа.
		AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(this, &ThisClass::OnBeginNotify);
		AnimInstance->OnPlayMontageNotifyEnd.AddDynamic(this, &ThisClass::OnEndNotify);

		AnimInstance->Montage_Play(ReloadMontage, PlayRate);
	}

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();

	// Запускаем новый таймер, который будет сигнализировать об окончании процесса перезарядки
	TimerManager.SetTimer(
		ReloadTimer,
		this, 
		&ThisClass::HandleReloadTimerComplete, 
		ReloadDuration, 
		false
	);
}

void UReloadComponent::HandleReloadTimerComplete()
{
	FinishReload(true);
}

void UReloadComponent::OnBeginNotify(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	ReceiveBeginNotify(NotifyName);
	OnReloadAnimNotifyBegin.Broadcast(NotifyName);
}

void UReloadComponent::OnEndNotify(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	ReceiveEndNotify(NotifyName);
	OnReloadAnimNotifyEnd.Broadcast(NotifyName);
}

void UReloadComponent::FinishReload(bool bSuccessfully)
{
	if (IsReloading())
	{
		// Очищаем таймер перезарядки, чтобы предотвратить его дальнейшее срабатывание.
		FTimerManager& TimerManager = GetWorld()->GetTimerManager();
		TimerManager.ClearTimer(ReloadTimer);

		if (AnimInstance && ReloadMontage)
		{
			// Плавно останавливаем проигрывание анимационного монтажа перезарядки.
			AnimInstance->Montage_Stop(0.15f, ReloadMontage);

			// Отвязываем делегаты.
			AnimInstance->OnPlayMontageNotifyBegin.Remove(this, "OnBeginNotify");
			AnimInstance->OnPlayMontageNotifyEnd.Remove(this, "OnEndNotify");
		}

		bIsReloading = false;

		ReceiveReloading(bSuccessfully);
		OnFinishReloading.Broadcast(bSuccessfully);
	}
}

void UReloadComponent::SetMesh(USkeletalMeshComponent* NewMesh)
{
	Mesh = NewMesh;
	if (Mesh)
	{
		AnimInstance = Mesh->GetAnimInstance();
	}
}

void UReloadComponent::SetCanReload(bool bNewCanReload)
{
	if (bCanReloadDefault != bNewCanReload)
	{
		bCanReloadDefault = bNewCanReload;
		OnCanReloadChanged.Broadcast(bNewCanReload);
	}
}

void UReloadComponent::SetReloadDuration(float NewReloadDuration)
{
	ReloadDuration = NewReloadDuration;
}

void UReloadComponent::SetReloadMontage(UAnimMontage* NewReloadMontage)
{
	ReloadMontage = NewReloadMontage;
}