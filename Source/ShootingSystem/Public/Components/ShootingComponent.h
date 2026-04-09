
#pragma once

#include "Components/TimelineComponent.h"
#include "GameplayTagContainer.h"
#include "NiagaraSystem.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShootingComponent.generated.h"

class UObjectPoolComponent;
class AProjectileBase;

// Перечисление для определения режима стрельбы.
UENUM(BlueprintType)
enum class EFireMode : uint8
{
	FM_None           UMETA(DisplayName = "None", Hidden),
	FM_Single         UMETA(DisplayName = "Single"),
	FM_Auto           UMETA(DisplayName = "Auto"),
	FM_Burst          UMETA(DisplayName = "Burst")
};

// Перечисление для определения типа стрельбы
UENUM(BlueprintType)
enum class EFireType : uint8
{
	FT_LineTrace         UMETA(DisplayName = "Line Trace"),
	FT_Projectile        UMETA(DisplayName = "Projectile")
};

USTRUCT(BlueprintType)
struct FAmmoInfo
{
	GENERATED_BODY()

public:
	/** Текущее кол-во патронов в магазине. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Current = 0;

	/** Максимальное кол-во патронов в магазине. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Max = 0;

	/** Общее количество патрон у владельца. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Total = 0;

	/** Тег патронов. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Tag = FGameplayTag();

	/** Бесконечное ли кол-во патронов. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bInfinite = false;
};

USTRUCT(BlueprintType)
struct FSpreadConfig
{
	GENERATED_BODY()

public:
	/** Базовое значение разброса, по умолчанию. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseValue = 0.0f;

	/** Увелечение разброса за один выстрел. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseThreshold = 0.0f;

	/** Максимально допустимый разброс. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxValue = 0.0f;

	/** Скорость уменьшения разброса, когда оружие не стреляет. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "s"))
	float RecoveryRate = 1.0f;

	/** Время через которое разброс начнет уменьшаться после последнего выстрела. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Units = "s"))
	float RecoveryDelay = 1.0f;
};

USTRUCT(BlueprintType)
struct FDamageConfig
{
	GENERATED_BODY()

public:
	/** @brief Тип урона. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	TSubclassOf<UDamageType> DamageType = nullptr;

	/**
	 * @brief Базовый урон, наносимый одним выстрелом или попаданием.
	 * Конкретное значение урона может быть модифицировано другими факторами (например, дистанцией, типом врага).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float BaseDamage = 0.0f;

	/**
	 * @brief Урон максимальный до этой дистанции.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float DamageFalloffStartDistance = 1000.0f;

	/**
	 * @brief Урон минимальный на этой или большей дистанции
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float DamageFalloffEndDistance = 5000.0f;

	/**
	 * @brief Параметр определяющий, до какого предела урон может быть срезан в зависимости от расстояния.
	 *        Рассчитывается по формуле = BaseDamage * (DamageFalloffMaxPercentage / 100).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "100.0", Units = "Percent"), Category = "Damage")
	float DamageFalloffMaxPercentage = 80.0f;

	/**
	 * @brief Флаг, определяющий, нужно ли делить BaseDamage на BulletsPerShot
	 *        для каждого отдельного "снаряда", или же BaseDamage является общим для всех снарядов.
	 *        Если true: Общий урон равен BaseDamage, который затем делится на BulletsPerShot.
	 *        Если false: Каждый снаряд наносит BaseDamage.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Damage")
	bool bDivideDamageByBulletsPerShot = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFire);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStartFire);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFinishFire);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmmoUpdated, const FAmmoInfo&, NewAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFireModeChanged, EFireMode, NewFireMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPostSpawnShootingLineTrace, const FHitResult, Hit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnShootingTargetHit, AActor*, HitActor, float, DamageAmount, const FHitResult&, HitInfo, const FGameplayTag, SurfaceTag);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SHOOTINGSYSTEM_API UShootingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UShootingComponent();

public:
	/**
	 * @brief Начинает процесс стрельбы из оружия.
	 */
	UFUNCTION(BlueprintCallable, Category = "Shooting")
	void StartFire();

	/**
	 * @brief Прекращает процесс стрельбы из оружия.
	 */
	UFUNCTION(BlueprintCallable, Category = "Shooting")
	void FinishFire();

	/**
	 * @brief Проверяет, готово ли оружие к выполнению операции стрельбы в данный момент.
	 * @return True, если оружие может стрелять, false в противном случае.
	 */
	UFUNCTION(BlueprintCallable, Category = "Shooting")
	bool CanFire() const;

public:
	/**
	 * @brief Уменьшает боеприпасы.
	 * @param Amount Количество, которое нужно уменьшить.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Ammo")
	void ConsumeAmmo(int32 Amount = 1);

	/**
	 * @brief Устанавливает новую информацию о боеприпасах для оружия.
	 * 
	 * @param NewAmmo Новая информация о боеприпасах (структура FAmmoInfo).
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Ammo")
	void SetAmmo(FAmmoInfo NewAmmo);

	/**
	 * @brief Устанавливает тип стрельбы для оружия.
	 *
	 * @param NewFireType Новый тип стрельбы.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|FireType")
	void SetFireType(EFireType NewFireType);

	/**
	 * @brief Устанавливает новый режим стрельбы для оружия.
	 * 
	 * @param NewFireMode Новый режим стрельбы.
	 * @param bForce Устанавливает новый режим принудительно обходя проверки.
	 * @return Возращает true, если новый режим удалось установить.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|FireMode")
	bool SetFireMode(EFireMode NewFireMode, bool bForce = false);

	/**
	 * @brief Устанавливает новый список достпных режимов стрельбы для оружия.
	 *
	 * @param NewFireMode Новый режим стрельбы.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|FireMode")
	void SetAvailableFireModes(const TArray<EFireMode>& NewAvailableFireModes);

	/**
	 * @brief Устанавливает новую скорость стрельбы для оружия.
	 * 
	 * @param NewFireRate Новая скорость стрельбы (в выстрелах в секунду).
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|FireRate")
	void SetFireRate(float NewFireRate);

	/**
	 * @brief Устанавливает новую базовую информацию о разбросе для оружия.
	 * 
	 * @param NewSpreadConfig Новая базовая информация о разбросе.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Spread")
	void SetSpreadConfig(FSpreadConfig NewSpreadConfig);

	/**
	 * @brief Устанавливает новое значение для масштаба разброса (spread).
	 *
	 * @param NewScale Новое значение масштаба разброса.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Spread")
	void SetSpreadScale(float NewScale);

	/**
	 * @brief Устанавливает конфикг урона, наносимый оружием.
	 * 
	 * @param NewDamageConfig Новый базовый урон.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Damage")
	void SetDamageConfig(FDamageConfig NewDamageConfig);

	/**
	 * @brief Устанавливает дальность действия трассировки луча (hitscan).
	 * 
	 * @param NewRange Новое значение дальности действия трассировки луча.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Hitscan")
	void SetHitscanRange(float NewRange);

	/**
	 * @brief Устанавливает количество "пуль" (трассировок), выпускаемых за один выстрел.
	 * 
	 * @param NewBulletPerShot Новое количество пуль, выпускаемых за один выстрел.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Hitscan")
	void SetBulletPerShot(int32 NewBulletPerShot);

	/**
	 * @brief Устанавливает тип трассировки (trace type), который будет использоваться для hitscan-запросов.
	 *        
	 * @param NewHitscanTraceType Новый тип трассировки для hitscan-запросов.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Hitscan")
	void SetHitscanTraceType(TEnumAsByte<ETraceTypeQuery> NewHitscanTraceType);

	/**
	 * @brief Устанавливает менеджер пула для пуль (снарядов).
	 * 
	 * @param NewManager Указатель на компонент UObjectPoolComponent, который будет управлять пулями.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Projectile")
	void SetProjectilePoolManager(UObjectPoolComponent* NewManager, bool bRefresh = false);

	/**
	 * @brief Устанавливает звук, который будет воспроизводиться во время выстрела.
	 *
	 * @param NewFireSound - Новый звуковой ресурс (USoundBase), который будет использоваться.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|VFX")
	void SetFireSound(USoundBase* NewFireSound);

	/**
	 * @brief Устанавливает визуальный эффект вспышки у дула оружия. 
	 *
	 * @param NewMuzzleFlash - Новый визуальный эффект Niagara System, который будет использоваться.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|VFX")
	void SetMuzzleFlash(UNiagaraSystem* NewMuzzleFlash);

	/**
	 * @brief Устанавливает монтаж, который будет воспроизводиться во время выстрела на Mesh.
	 *
	 * @param NewFireMontage - Новый анимационный монтаж, который будет использоваться.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Animation")
	void SetFireMontage(UAnimMontage* NewFireMontage);

	/**
	 * @brief Устанавливает или обновляет ссылку на компонент меша, используемый для стрельбы.
	 *
	 * @param NewMesh Указатель на новый USkeletalMeshComponent, который должен быть установлен.
	 *                Это может быть USkeletalMeshComponent или UStaticMeshComponent.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Mesh")
	void SetMesh(USkeletalMeshComponent* NewMesh);

public:
	/**
	 * @brief Возвращает конфиг урона.
	 * @return Базовый конфиг.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Damage")
	FORCEINLINE FDamageConfig GetDamageConfig() const { return DamageConfig; }

	/**
	 * @brief Возвращает текущий режим стрельбы оружия.
	 * @return Текущий режим стрельбы.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|FireMode")
	FORCEINLINE EFireMode GetFireMode() const { return FireMode; }

	/**
	 * @brief Возвращает доступные типы стрельбы.
	 * @return Доступные типа стрельбы.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|FireMode")
	FORCEINLINE TArray<EFireMode> GetAvailableFireModes() const { return AvailableFireModes; }

	/**
	 * @brief Возвращает скорострельность оружия.
	 * @return Скорострельность (выстрелов в секунду).
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|FireRate")
	FORCEINLINE float GetFireRate() const { return FireRate; }

	/**
	 * @brief Возвращает текущую информацию о боеприпасах оружия.
	 * @return Структура FAmmoInfo, содержащая данные о патронах.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Ammo")
	FORCEINLINE FAmmoInfo GetAmmo() const { return Ammo; }

	/**
	 * @brief Проверяет, есть ли патроны в текущем магазине (клипе).
	 * @return True, если в текущем магазине есть патроны, False в противном случае.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Ammo")
	FORCEINLINE bool HasAmmoInMagazine() const { return Ammo.Current > 0; }

	/**
	 * @brief Проверяет, полностью ли заполнен магазин.
	 * @return bool True, если Current равен Max, false в противном случае.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Ammo")
	FORCEINLINE bool IsFullAmmoInMagazine() const { return Ammo.Current == Ammo.Max; }

	/**
	 * @brief Проверяет, стреляет ли оружие в данный момент.
	 * @return bool True, если в стреляет, false в противном случае.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	FORCEINLINE bool IsFiring() const { return bIsFiring; }

	/**
	 * @brief Возвращает базовую информацию о разбросе оружия.
	 * @return Структура FSpreadInfo, содержащая базовые параметры разброса.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Spread")
	FORCEINLINE FSpreadConfig GetSpreadConfig() const { return SpreadConfig; }

	/**
	 * @brief Возвращает текущий масштаб разброса.
	 * @return Текущее значение масштаба разброса.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Spread")
	FORCEINLINE float GetSpreadScale() const { return CurrentSpreadScale; }

	/**
	 * @brief Возвращает результат последнего совершенного "хитскан" выстрела.
	 * Содержит информацию о том, куда попал выстрел, какой актер был задет и т.д.
	 * 
	 * @return Структура FHitResult с данными о последнем попадании.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Shooting")
	FORCEINLINE FHitResult GetLastHitResult() const { return LastHitResult; }

	/**
	 * @brief Возвращает текущий менеджер пула для пуль (снарядов).
	 * Позволяет получить доступ к компоненту, который управляет переиспользованием объектов пуль.
	 * 
	 * @return Указатель на компонент UObjectPoolComponent, управляющий пулями.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Projectile")
	FORCEINLINE UObjectPoolComponent* GetProjectilePoolManager() const { return ProjectilePoolManager; }

	/**
	 * @brief Получает ссылку на компонент меша, используемый для стрельбы.
	 *
	 * @return Указатель на USkeletalMeshComponent, если он найден, иначе nullptr.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Mesh")
	FORCEINLINE USkeletalMeshComponent* GetMesh() const { return Mesh; };

public:
	UFUNCTION(BlueprintCallable, Category = "Utility|Hit")
	void Notify_HandleHit(const FHitResult& InHit);

	/**
	 * @brief Обновляет состояние магазина, перенося патроны из общего запаса.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Ammo")
	void RefreshMagazine();

	/**
	 * @brief Переключает на следующий FireMode.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|FireMode")
	void NextFireMode();

	/**
	 * @brief Рассчитывает вектор направления выстрела с учетом разброса.
	 *
	 * Добавляет случайное отклонение к базовому направлению, имитируя разброс оружия.
	 * Реализация может быть переопределена в Blueprint.
	 *
	 * @param InDirection Базовое направление выстрела (без разброса).
	 * @return Вектор направления выстрела с учетом разброса.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Spread")
	FVector CalculateSpread(const FVector& InDirection);

	/**
	 * @brief Рассчитывает величину урона в зависимости от пройденного расстояния.
	 *
	 * @param Distance Расстояние, пройденное выстрелом или снарядом.
	 * @return Величина урона, рассчитанная с учетом расстояния.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Damage")
	float CalculateDamageByDistance(float InDamage, float InDistance) const;

	/**
	 * @brief Возвращает позицию, откуда будет произведен выстрел.
	 *
	 * Реализация может быть переопределена в Blueprint.
	 *
	 * @return Вектор позиции выстрела.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Shooting")
	FVector GetFireLocation() const;

	/**
	 * @brief Возвращает базовое направление, в котором будет произведен выстрел.
	 *
	 * Реализация может быть переопределена в Blueprint.
	 *
	 * @return Вектор направления выстрела.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Shooting")
	FVector GetFireDirection() const;

	/**
	 * @brief Возвращает мировую позицию сокета "Muzzle" (дула) на сетке оружия.
	 *
	 * @return Мировая позиция сокета "Muzzle". Если сокет не найден, может вернуть нулевой вектор или позицию актера.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility|Muzzle")
	FVector GetMuzzleSocketLocation() const;

public:
	/**
	 * @brief Флаг, определяющий, нужно ли отрисовывать отладочные визуализации для трассировки.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "Debug Config")
	bool bDrawDebugTrace = false;

	/**
	 * @brief Флаг, определяющий, нужно ли отрисовывать отладочные визуализации для урона.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug Config")
	bool bDrawDebugDamage = false;

public:
	/**
	 * @brief Делегат, который вызывается, когда стрельба фактически начинается.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnStartFire OnStartFire;

	/**
	 * @brief Делегат, который вызывается, когда стрельба завершается.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnFinishFire OnFinishFire;

	/**
	 * @brief Делегат, который вызывается каждый раз, когда происходит фактический выстрел.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnFire OnFire;

	/**
	 * @brief Делегат, который вызывается после выполнения трассировки луча (line trace) в процессе стрельбы.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnPostSpawnShootingLineTrace OnPostSpawnLineTrace;

	/**
	 * @brief Делегат, который вызывается при попадание в цель.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnShootingTargetHit OnTargetHit;

	/**
	 * @brief Делегат, который вызывается при обновление патрон.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnAmmoUpdated OnAmmoUpdated;

	/**
	 * @brief Делегат, который вызывается при смене режима огня.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnFireModeChanged OnFireModeChanged;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
protected:
	/**
	 * @brief Создает и настраивает логику для режима стрельбы "Single".
	 */
	void CreateSingleFire();

	/**
	 * @brief Создает и настраивает логику для режима стрельбы "Auto".
	 */
	void CreateAutoFire();

	/**
	 * @brief Создает и настраивает логику для режима стрельбы "Burst".
	 */
	void CreateBurstFire();

protected:
	/**
	 * @brief Обрабатывает основную логику производства одного выстрела.
	 */
	virtual void HandleFire();

	/**
	 * @brief Обрабатывает логику когда стрельба завершилась.
	 */
	virtual void HandleFinishedFiring();

	/**
	 * @brief Обрабатывает получение урона актером.
	 *
	 * @param Hit Информация о месте попадания, которое привело к этому урону.
	 * @return True, если урон был успешно обработан, false в случае ошибки.
	 */
	virtual float HandleCalculateDamage(const FHitResult& HitInfo);

	/**
	 * @brief Выполняет трассировку луча для определения точки попадания выстрела.
	 *
	 * Эта функция используется для симуляции траектории выстрела и определения, куда он попал.
	 */
	virtual void SpawnShootLineTrace();

	/**
	 * @brief Выполняет действия после завершения трассировки луча.
	 */
	virtual void PostSpawnShootLineTrace(const FHitResult& Hit);

	/**
	 * @brief Обрабатывает результат попадания после трассировки луча.
	 */
	virtual void HandleHit(const FHitResult& Hit);

protected:
	void SpawnProjectile(const FHitResult& Hit);
	void SpawnMuzzleFlash();
	void PlaySoundFire();
	void PlayMontageFire();

protected:
	FTimerManager& GetTimerManager() const { return GetWorld()->GetTimerManager(); }

protected:
	/**
	 * @brief Пользовательский Event: Проверяет, может ли оружие произвести выстрел.
	 * Эта функция вызывается перед попыткой выстрела для определения, выполнены ли все необходимые условия (например, наличие патронов, отсутствие перезарядки).
	 *
	 * @return True, если выстрел возможен, False в противном случае.
	 */
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "Can Fire"), Category = Firing)
	bool CanFireInternal() const;
	virtual bool CanFireInternal_Implementation() const;

	/**
	 * @brief Пользовательский Event: Вычисляет и возвращает вектор направления с учетом разброса.
	 * Эта функция добавляет случайное отклонение к направлению выстрела, имитируя разброс оружия.
	 *
	 * @return Вектор направления выстрела с учетом разброса.
	 */
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "Calculate Spread"), Category = Firing)
	FVector CalculateSpreadInternal(const FVector& InDirection);
	virtual FVector CalculateSpreadInternal_Implementation(const FVector& InDirection);

	/**
	 * @brief Пользовательский Event: Возвращает позицию, откуда будет произведен выстрел.
	 * Эта функция позволяет определить точную точку, из которой исходит трейс выстрела.
	 *
	 * @return Вектор позиции выстрела.
	 */
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "Get Fire Location"), Category = Firing)
	FVector GetFireLocationInternal() const;
	virtual FVector GetFireLocationInternal_Implementation() const;

	/**
	 * @brief Пользовательский Event: Возвращает базовое направление, в котором будет произведен выстрел.
	 *
	 * @return Нормализированый вектор базового направления выстрела.
	 */
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "Get Fire Direction"), Category = Firing)
	FVector GetFireDirectionInternal() const;
	virtual FVector GetFireDirectionInternal_Implementation() const;

	/**
	 * @brief Пользовательский Event: Рассчитывает окончательный урон с учетом всех модификаторов.
	 *
	 * Функцию можно полностью переопределить логику расчета урона или дополнить ее.
	 *
	 * @param HitInfo Информация о попадании, содержащая детали о цели и точке контакта.
	 * @return Рассчитанное значение урона, которое будет передано в HandleApplyDamageInternal.
	 */
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "Handle Calculate Damage"), Category = Damage)
	float HandleCalculateDamageInternal(const FHitResult& HitInfo);
	virtual float HandleCalculateDamageInternal_Implementation(const FHitResult& HitInfo);

	/**
	 * @brief Пользовательский Event: Применяет рассчитанный урон к цели.
	 * 
	 * Функцию можно полностью переопределить логику нанесения урона или дополнить ее.
	 *
	 * @param InDamage Рассчитанный финальный урон, который будет применен.
	 * @param HitInfo Информация о попадании, связанная с этим уроном.
	 */
	UFUNCTION(BlueprintNativeEvent, meta = (DisplayName = "Handle Apply Damage"), Category = Damage)
	void HandleApplyDamageInternal(float InDamage, const FHitResult& HitInfo);
	virtual void HandleApplyDamageInternal_Implementation(float InDamage, const FHitResult& HitInfo);

protected:
	/**
	 * @brief Пользовательское событие Blueprint: вызывается в начале процесса стрельбы.
	 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Start Fire"), Category = Firing)
	void ReceiveStartFire();

	/**
	 * @brief Пользовательское событие Blueprint: .
	 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Finish Fire"), Category = Firing)
	void ReceiveFinishFire();

	/**
	 * @brief Пользовательское событие Blueprint: вызывается при каждом фактическом выстреле.
	 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Fire"), Category = Firing)
	void ReceiveFire();

	/**
	 * @brief Пользовательское событие Blueprint: вызывается после выполнения трассировки луча.
	 *
	 * @param Hit Результат трассировки луча.
	 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Post Spawn Line Trace"), Category = Firing)
	void ReceivePostSpawnLineTrace(const FHitResult& Hit);

	/**
	 * @brief Пользовательское событие Blueprint: Вызывается при обновлении информации о патронах.
	 *
	 * @param NewAmmo Новая структура FAmmoInfo, содержащая актуальную информацию о патронах.
	 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Update Ammo"), Category = Ammo)
	void ReceiveUpdateAmmo(const FAmmoInfo& NewAmmo);

private:
	void AutoFireTimerCallback();
	void BurstFireTimerCallback();
	void InitRecoverySpreadTimeline();
	void StartRecoverySpread();
	void SetupFireMode();
	void DrawDebugApplyDamage(float InDamage);

private:
	UFUNCTION()
	void UpdateRecoverySpread(float Alpha);

private:
	/*
	 * @brief Доступные режимы стрельбы.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Core")
	TArray<EFireMode> AvailableFireModes = { EFireMode::FM_Single };

	/**
	 * @brief Определяет основной тип стрельбы данного оружия.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Core")
	EFireType FireType = EFireType::FT_LineTrace;

	/**
	 * @brief Текущий режим стрельбы оружия. Определяет, как происходит выстрел (одиночный, автоматический, очередь и т.д.).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Core")
	EFireMode FireMode = EFireMode::FM_Single;

	/**
	 * @brief Скорострельность оружия в выстрелах в секунду.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", Units = "s"), Category = "Core")
	float FireRate = 0.2f;

	/**
	 * @brief Количество снарядов-пуль, выпускаемых за один выстрел.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1", AllowPrivateAccess = "true"), Category = "Core")
	int32 BulletsPerShot = 1;

	/**
	 * @brief Максимальная дистанция, на которую действует выстрел при использовании техники "хитскан".
	 * Определяет, как далеко пуля "летит" мгновенно.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", Units = "cm"), Category = "Core")
	float HitscanRange = 10000.0f;

	/** @brief Тип трейса. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Core")
	TEnumAsByte<ETraceTypeQuery> HitscanTraceType;

	/**
	 * @brief Имя сокета на сетке актера-владельца, откуда будет исходить выстрел.
	 * Используется для определения точки и направления выстрела.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Core")
	FName MuzzleSocketName = "MuzzleSocket";

	/**
	 * @brief Конфигурация, определяющая параметры урона, наносимого этим объектом.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Damage")
	FDamageConfig DamageConfig = FDamageConfig();

	/**
	 * @brief Класс актера, который будет использоваться как снаряд.
	 * Например, класс пули, ракеты, гранаты.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Projectile")
	TSubclassOf<AProjectileBase> ProjectileActorClass = nullptr;

	/**
	 * @brief Информация о боеприпасах оружия, включая текущее количество в магазине и общий запас.
	 * Определяет, сколько выстрелов можно сделать до перезарядки.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Core")
	FAmmoInfo Ammo = FAmmoInfo();

	/**
	 * @brief Информация о разбросе (спреде) пуль. Определяет, насколько пули отклоняются от центральной линии при стрельбе.
	 * Позволяет симулировать реалистичный разброс для оружия.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Category = "Core")
	FSpreadConfig SpreadConfig = FSpreadConfig();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bStoAllMontagesWhenPlayedAnimFire = true;

private:
	/**
	 * @brief Анимация стрельбы.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "Fire (Montage)", AllowPrivateAccess = "true"), Category = "Animation")
	TObjectPtr<UAnimMontage> FireMontage = nullptr;

	/**
	 * @brief Звуковой эффект (SFX), воспроизводимый при выстреле.
	 * Может быть звуком выстрела из огнестрельного оружия, энергетическим разрядом и т.д.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "Fire (SFX)", AllowPrivateAccess = "true"), Category = "Sound")
	TObjectPtr<USoundBase> FireSound = nullptr;

	/**
	 * @brief Визуальный эффект (VFX), воспроизводимый в точке выстрела (дуле оружия).
	 * Обычно это вспышка при выстреле, энергетический импульс и т.п.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "Muzzle Flash (VFX)", AllowPrivateAccess = "true"), Category = "VFX")
	TObjectPtr<UNiagaraSystem> MuzzleFlash = nullptr;

private:
	/**
	 * @brief Компонент меша, представляющий визуальную модель оружия.
	 * Используется для получения информации о сокетах (например, "Muzzle").
	 */
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> Mesh = nullptr;

	/** @brief Компонент, управляющий пулом снарядов. */
	UPROPERTY()
	TObjectPtr<UObjectPoolComponent> ProjectilePoolManager = nullptr;

	/** @brief Кривая, определяющая, как быстро восстанавливается разброс при стрельбе. */
	UPROPERTY()
	TObjectPtr<UCurveFloat> RecoverySpreadCurve = nullptr;

	/** @brief Результат последней трассировки луча. */
	UPROPERTY()
	FHitResult LastHitResult = FHitResult();

	/** @brief Текущее значение масштаба разброса. */
	float CurrentSpreadScale = 0.0f;

	/** @brief Хранит в себе последнее значение разброса перед его сбросом до базового значения. */
	float CashSpreadScale = 0.0f;

	/** @brief Флаг, указывающий, активна ли в данный момент стрельба. */
	bool bIsFiring = false;

	/***/
	int32 FireCount = 0;

	/**
	 * @brief Таймер для сброса состояния стрельбы.
	 * Используется, например, после отпускания кнопки стрельбы в режиме Auto,
	 * чтобы остановить стрельбу и разрешить следующую.
	 */
	FTimerHandle TimerResetFire;

	/**
	 * @brief Таймер, управляющий непосредственно процессом стрельбы.
	 * Используется для реализации задержек между выстрелами (Auto, Burst).
	 */
	FTimerHandle TimerFiring;

	/** Таймер запускающий процесс уменьшение разброса. */
	FTimerHandle TimerStartRecoverySpread;

	/** Таймлайн для восстановления разброса. */
	FTimeline RecoverySpreadTimeline;
};
