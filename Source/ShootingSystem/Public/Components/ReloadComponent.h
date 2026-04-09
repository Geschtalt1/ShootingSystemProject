
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ReloadComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadingStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReloadingFinish, bool, bSuccessfully);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCanReloadChanged, bool, bNewCanReload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReloadAnimNotify, FName, NotifyName);

UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SHOOTINGSYSTEM_API UReloadComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UReloadComponent();

public:
	/**
	 * @brief Инициирует процесс перезарядки оружия.
	 */
	UFUNCTION(BlueprintCallable, Category = "Reload")
	bool StartReload();

	/**
	 * @brief Отменяет процесс перезарядки оружия.
	 */
	UFUNCTION(BlueprintCallable, Category = "Reload")
	void FinishReload(bool bSuccessfully);

	/**
	 * @brief Проверяет, может ли оружие быть перезаряжено в данный момент.
	 *
	 * Реализация может быть переопределена в Blueprint.
	 *
	 * @return True, если оружие может быть перезаряжено, false в противном случае.
	 */
	UFUNCTION(BlueprintCallable, Category = "Reload")
	bool CanReload() const;

public:
	/**
	 * @brief Устанавливает Skeletal Mesh Component, который будет использоваться для проигрывания анимаций.
	 *
	 * @param NewMesh Указатель на новый Skeletal Mesh Component, который будет использоваться.
	 *                Если передан nullptr, текущий Skeletal Mesh Component будет сброшен.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Mesh")
	void SetMesh(USkeletalMeshComponent* NewMesh);

	/**
	 * @brief Устанавливает флаг, определяющий, разрешена ли перезарядка в данный момент.
	 *
	 * @param bNewCanReload Новое значение флага возможности перезарядки.
	 *                      True - разрешена, False - запрещена.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|State")
	void SetCanReload(bool bNewCanReload);

	/**
	 * @brief Устанавливает продолжительность анимации перезарядки в секундах.
	 * 
	 * @param NewReloadDuration Новая продолжительность перезарядки в секундах.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Reload")
	void SetReloadDuration(float NewReloadDuration);

	/**
	 * @brief Устанавливает анимационный монтаж (Anim Montage), который будет проигрываться во время перезарядки.
	 * 
	 * @param NewReloadAnim Указатель на UAnimMontage для анимации перезарядки.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Reload")
	void SetReloadMontage(UAnimMontage* NewReloadMontage);

public:
	/** @brief Проверяет, идет ли сейчас перезарядка. */
	UFUNCTION(BlueprintCallable, Category = "Reload|State")
	FORCEINLINE bool IsReloading() const { return bIsReloading; }

	/**
	 * @brief Возвращает текущую установленную длительность перезарядки.
	 * @return float Текущая длительность перезарядки в секундах.
	 */
	UFUNCTION(BlueprintCallable, Category = "Reload|Montage")
	FORCEINLINE float GetReloadDuration() const { return ReloadDuration; }

	/**
	 * @brief Получает время перезарядки оружия.
	 *
	 * @return float Фактическая продолжительность перезарядки в секундах.
	 *         Возвращает 0.0f, если `ReloadMontage` не назначен.
	 */
	UFUNCTION(BlueprintCallable, Category = "Reload|Montage")
	FORCEINLINE float GetReloadTime() const { return ReloadMontage ? ReloadMontage->GetPlayLength() / ReloadDuration : 0.f; }

	/**
	 * @brief Возвращает текущую анимационный монтаж перезарядки.
	 * @return текущий монтаж перезарядки.
	 */
	UFUNCTION(BlueprintCallable, Category = "Reload|Montage")
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; };

	/**
	 * @brief Возвращает указатель на Skeletal Mesh Component, связанный с этим компонентом.
	 * @return USkeletalMeshComponent* Указатель на Skeletal Mesh Component владельца,
	 */
	UFUNCTION(BlueprintCallable, Category = "Reload|Mesh")
	FORCEINLINE USkeletalMeshComponent* GetMesh() const { return Mesh; }

public:
	/**
	 * @brief Делегат, вызываемый при начале процесса перезарядки.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnReloadingStart OnStartReloading;

	/**
	 * @brief Делегат, вызываемый при завершении процесса перезарядки.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnReloadingFinish OnFinishReloading;

	/**
	 * @brief Делегат, вызываемый при изменении состояния возможности перезарядки.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnCanReloadChanged OnCanReloadChanged;

	/**
	 * @brief Делегат, вызываемый при получении Anim Notify Begin, обозначающего начало перезарядки.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnReloadAnimNotify OnReloadAnimNotifyBegin;

	/**
	 * @brief Делегат, вызываемый при получении Anim Notify End из анимации перезарядки.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnReloadAnimNotify OnReloadAnimNotifyEnd;

protected:
	virtual void BeginPlay() override;	

protected:
	/**
	 * @brief Обрабатывает логику перезарядки оружия.
	 */
	virtual void HandleReload();

protected:
	/**
	 * @brief Пользовательский Event: Обрабатывает логику перезарядки оружия.
	 * Эта функция вызывается, когда принято решение о перезарядке.
	 */
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "Handle Reload"), Category = Reloading)
	void HandleReloadInternal();
	virtual void HandleReloadInternal_Implementation();

	/**
	 * @brief Пользовательский Event: Проверяет, возможно ли начать перезарядку.
	 * Эта функция вызывается перед началом перезарядки для проверки условий.
	 *
	 * @return True, если перезарядка возможна, False в противном случае.
	 */
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "Can Reload"), Category = Reloading)
	bool CanReloadInternal() const;
	virtual bool CanReloadInternal_Implementation() const;

protected:
	/**
	 * @brief Пользовательское событие Blueprint: вызывается при начале процесса перезарядки.
	 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Start Reload"), Category = Reloading)
	void ReceiveStartReload();

	/**
	 * @brief Пользовательское событие Blueprint: Вызывается при завершении перезарядки.
	 * 
	 * @param bSuccessfully True, если перезарядка была успешной, false в противном случае.
	 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Reloading"), Category = Reloading)
	void ReceiveReloading(bool bSuccessfully);

	/**
	 * @brief Пользовательское событие Blueprint: Вызывается при получении Anim Notify, относящегося к началу.
	 *
	 * @param NotifyName Имя (FName) Anim Notify, который был получен.
	 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Begin Notify"), Category = Animation)
	void ReceiveBeginNotify(FName NotifyName);

	/**
	 * @brief Пользовательское событие Blueprint: Вызывается при получении Anim Notify, относящегося к концу.
	 *
	 * @param NotifyName Имя (FName) Anim Notify, который был получен.
	 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "End Notify"), Category = Animation)
	void ReceiveEndNotify(FName NotifyName);

private:
	void HandleReloadTimerComplete();

private:
	UFUNCTION()
	void OnBeginNotify(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);

	UFUNCTION()
	void OnEndNotify(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);

private:
	/**
	 * @brief Указывает, разрешена ли перезарядка по умолчанию.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Core")
	bool bCanReloadDefault = true;

	/**
	 * @brief Базовая длительность процесса перезарядки в секундах.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", Units = "s"), Category = "Core")
	float ReloadDuration = 2.0f;

	/**
	 * @brief Анимационный монтаж, который должен проигрываться во время перезарядки.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Animation")
	TObjectPtr<UAnimMontage> ReloadMontage = nullptr;

private:
	/**
	 * @brief Указатель на SkeletalMeshComponent владельца актора.
	 */
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> Mesh = nullptr;

	/**
	 * @brief Указатель на UAnimInstance, связанный с Skeletal Mesh Component'ом владельца.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UAnimInstance> AnimInstance = nullptr;

	/**
	 * @brief Флаг, указывающий, выполняется ли в данный момент процесс перезарядки.
	 */
	bool bIsReloading = false;

	/**
	 * @brief Дескриптор таймера для управления процессом перезарядки.
	 */
	FTimerHandle ReloadTimer;
};
