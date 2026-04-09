
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PoolableComponent.generated.h"

class UObjectPoolComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUsedFromPool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReturnedToPool);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SHOOTINGSYSTEM_API UPoolableComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UPoolableComponent();

public:
	/**
	 * @brief Возвращает объект в пул.
	 * 
	 * Должен быть вызван, когда объект больше не нужен и может быть переиспользован.
	 */
	UFUNCTION(BlueprintCallable, Category = "Object Pool")
	void ReturnToPool();

public:
	/**
	 * @brief Вызывается менеджером пула при выдаче объекта.
	 * 
	 * Активирует объект и устанавливает его состояние как "используемый".
	 */
	void UsedFromPool();

	/**
	 * @brief Внутренняя функция, вызываемая менеджером пула при возврате объекта.
	 * 
	 * Деактивирует объект и устанавливает его состояние как "не используемый".
	 */
	void ReturnedToPool();

	/**
	 * @brief Устанавливает состояние использования объекта (активен/неактивен).
	 */
	void SetInUse(bool bNewStateUse);

	/**
	 * @brief Устанавливает новый пулл менеджер для этого объекта.
	 */
	void SetPoolManager(UObjectPoolComponent* NewPoolManager);

public:
	/**
	 * @brief Проверяет, используется ли объект в данный момент (то есть, извлечен из пула).
	 * @return True, если объект используется, false в противном случае.
	 */
	UFUNCTION(BlueprintPure, Category = "Object Pool")
	FORCEINLINE bool IsUsedObject() const { return bIsUsed; }

	/**
	 * @brief Возвращает ссылку на менеджер пула, который управляет этим объектом.
	 * @return Указатель на UObjectPoolComponent.
	 */
	UFUNCTION(BlueprintPure, Category = "Object Pool")
	FORCEINLINE UObjectPoolComponent* GetPoolManager() const { return PoolManager; }

public:
	/**
	 * @brief Время жизни объекта после активации из пула.
	 * 
	 * Если установлено значение больше 0, объект будет автоматически возвращен в пул по истечении этого времени.
	 * Если 0, объект будет жить до тех пор, пока не будет возвращен в пул вручную.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Setting")
	float TimeToLeave = 10.0f;

public:
	/**
	 * @brief Делегат, вызываемый, когда объект извлекается из пула и становится активным.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnUsedFromPool OnUsedFromPool;

	/**
	 * @brief Делегат, вызываемый, когда объект возвращается обратно в пул.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnReturnedToPool OnReturnedToPool;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/**
	 * @brief Флаг, указывающий, используется ли объект в данный момент (извлечен из пула).
	 */
	UPROPERTY()
	bool bIsUsed = false;

	/**
	 * @brief Таймер, управляющий временем жизни объекта.
	 * 
	 * Используется для автоматического возврата объекта в пул по истечении `TimeToLeave`.
	 */
	UPROPERTY()
	FTimerHandle TimerLeave;

	/**
	 * @brief Ссылка на менеджер пула, который управляет этим объектом.
	 * Устанавливается при выдаче объекта из пула.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UObjectPoolComponent> PoolManager = nullptr;
};
