
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ObjectPoolComponent.generated.h"

class UPoolableComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSpawnedObjectFromPool, UPoolableComponent*, Object, const FTransform&, SpawnTransform, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReturnedObjectToPool, UPoolableComponent*, Object);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPoolDepleted);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SHOOTINGSYSTEM_API UObjectPoolComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UObjectPoolComponent();

public:
    /**
     * @brief Инициализирует пул, создавая начальное количество объектов.
     * 
     * Очищает существующие объекты пула, если они есть, а затем создает
     * `InitialPoolSize` новых объектов, используя `CreateReusableObject()`.
     */
    void InitializePool();

    /**
     * @brief Создает новый объект, который может быть использован пулом, или добавляет существующий, если он еще не в пуле.
     *
     * Эта функция предназначена для первоначального наполнения пула или для расширения пула при необходимости.
     * Она спавнит актера указанного класса, находит или добавляет к нему UPoolableComponent,
     * устанавливает его как неиспользуемый и добавляет в общий список объектов пула.
     *
     * @return Указатель на созданный UPoolableComponent, готовый к использованию из пула.
     */
    UPoolableComponent* CreateReusableObject();

public:
    /** Получение объекта из пула. Если доступных нет, может создавать новый (если лимит позволяет) или возвращать nullptr. */
    UFUNCTION(BlueprintCallable, Category = "Utility")
    bool SpawnPooledObject(const FTransform& SpawnTransform, AActor* Instigator, UPoolableComponent*& PooledObject);

    /** @brief Возвращение объекта обратно в пул для дальнейшего переиспользования. */
    UFUNCTION(BlueprintCallable, Category = "Utility")
    bool ReleaseObject(UPoolableComponent* PoolableObject);

    /** @brief Очистка всего пула. */
    UFUNCTION(BlueprintCallable, Category = "Utility")
    void ClearPool();

    /** @brief Обновляет пулл, удаляет старые объекты и создает новые. */
    UFUNCTION(BlueprintCallable, Category = "Utility")
    void RefreshObjectPool();

    /**
     * @brief Ищет первый доступный (неиспользуемый) объект в пуле.
     * 
     * @param[out] PoolableObject Ссылка на переменную, куда будет записан найденный доступный объект.
     *                          Если объект не найден, останется nullptr.
     * 
     * @return True, если доступный объект был найден и присвоен PoolableObject, false в противном случае.
     */
    UFUNCTION(BlueprintCallable, Category = "Utility")
    bool FindAvailableObject(UPoolableComponent*& PoolableObject) const;

    /** @brief Возвращает количество доступных объектов в данный момент. */
    UFUNCTION(BlueprintCallable, Category = "Utility")
    int32 GetAvailableObjectCount() const;

    /** @brief Возвращает количество объектов, которые в данный момент используются. */
    UFUNCTION(BlueprintCallable, Category = "Utility")
    int32 GetActiveObjectCount() const;

public:
    /**
     * @brief Устанавливает флаг готовности пула объектов.
     *
     * @param bReady Флаг, указывающий, готов ли пул к работе.
     */
    UFUNCTION(BlueprintCallable, Category = "Utility")
    void SetPoolReady(bool bReady);

    /**
     * @brief Устанавливает класс актеров, которые будут создаваться и управляться пулом объектов.
     *
     * @param NewPooledClass Новый класс актеров для пула.
     * @param bRefreshPool Флаг, указывающий, нужно ли немедленно обновить пул.
     */
    UFUNCTION(BlueprintCallable, Category = "Utility")
    void SetPooledActorClass(TSubclassOf<AActor> NewPooledClass, bool bRefreshPool = true);

    /**
     * @brief Устанавливает начальный размер пула объектов.
     * Это определяет, сколько объектов будет создано и добавлено в пул при его первоначальной инициализации.
     *
     * @param NewInitialSize Новый размер пула при инициализации.
     */
    UFUNCTION(BlueprintCallable, Category = "Utility")
    void SetInitialPoolSize(int32 NewInitialSize);

    /**
     * @brief Устанавливает максимальный размер пула объектов.
     *
     * @param NewMaxPoolSize Новый максимальный размер пула.
     */
    UFUNCTION(BlueprintCallable, Category = "Utility")
    void SetMaxPoolSize(int32 NewMaxPoolSize);

public:
    /**
     * @brief Возвращает массив всех объектов, находящихся в этом пуле (как активных, так и доступных).
     * @return Массив указателей на UObjectPoolComponent.
     */
    UFUNCTION(BlueprintCallable, Category = "Utility")
    FORCEINLINE TArray<UPoolableComponent*> GetObjectPool() const { return ObjectPool; };

    /**
     * @brief Проверяет, есть ли в пуле место для добавления новых объектов.
     * 
     * Если MaxPoolSize установлен в 0, считается, что ограничений нет.
     * 
     * @return True, если есть место для новых объектов или нет лимита, false в противном случае.
     */
    UFUNCTION(BlueprintCallable, Category = "Utility")
    FORCEINLINE bool HasCapacity() const { return (MaxPoolSize <= 0) || (ObjectPool.Num() < MaxPoolSize); }

    /**
     * @brief Возвращает текущее количество объектов, находящихся в этом пуле (как активных, так и доступных).
     * @return Текущее количество объектов в пуле.
     */
    UFUNCTION(BlueprintCallable, Category = "Utility")
    FORCEINLINE int32 GetObjectPoolAmount() const { return ObjectPool.Num(); };

    /**
     * @brief Возвращает true, если пулл готов к использованию.
     */
    UFUNCTION(BlueprintCallable, Category = "Utility")
    FORCEINLINE bool IsPoolReady() const { return bIsPoolReady; }

public:
    /**
     * @brief Делегат, вызываемый, когда объект успешно извлечен из пула и установлен.
     */
    UPROPERTY(BlueprintAssignable)
    FOnSpawnedObjectFromPool OnSpawnedObjectFromPool;

    /**
     * @brief Делегат, вызываемый, когда объект успешно возвращен обратно в пул.
     */
    UPROPERTY(BlueprintAssignable)
    FOnReturnedObjectToPool OnReturnedObjectToPool;
   
    /**
     * @brief Делегат, вызываемый, когда попытка получить объект из пула завершилась неудачно.
     */
    UPROPERTY(BlueprintAssignable)
    FOnPoolDepleted OnPoolDepleted;

protected:
	virtual void BeginPlay() override;

private:
    /**
     * @brief Свидетельствует о том, что пул готов к использованию, если значение true, пулл объектов будет создан при старте.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Object Pool")
    bool bIsPoolReady = true;

    /**
     * @brief Класс актера, который будет создаваться для пула.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Object Pool")
    TSubclassOf<AActor> PooledActorClass = nullptr;

    /**
     * @brief Количество объектов, которые нужно создать при старте.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Object Pool")
    int32 InitialPoolSize = 10;

    /**
     * @brief Максимальное количество объектов, которое может быть в пуле.
     * Если 0, то пул может расширяться бесконечно (не рекомендуется).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", ClampMin = "0"), Category = "Object Pool")
    int32 MaxPoolSize = 50;

private:
    /**
     * @brief Массив всех UPoolableComponent, которые были созданы для этого пула.
     * 
     * Хранит как доступные (неиспользуемые), так и активные (используемые) объекты.
     * Используется для управления жизненным циклом всех объектов пула и для поиска доступных экземпляров.
     */
    UPROPERTY(Transient)
    TArray<UPoolableComponent*> ObjectPool;
};
