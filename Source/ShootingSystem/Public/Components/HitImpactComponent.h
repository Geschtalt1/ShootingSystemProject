
#pragma once

#include "GameplayTagsManager.h"
#include "GameplayTagContainer.h"
#include "NiagaraSystem.h"

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HitImpactComponent.generated.h"

USTRUCT(BlueprintType)
struct FImpactData : public FTableRowBase
{
	GENERATED_BODY()

public:
	/**
	 * @brief Базовый звук, который воспроизводится при попадании.
	 * Используется для звуковых эффектов попадания (например, "пистолетный выстрел", "удар по металлу").
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Sound)
	TObjectPtr<USoundBase> ImpactSound = nullptr;

	/**
	 * @brief Множитель громкости для звука попадания.
	 * Значение 1.0f означает нормальную громкость. Значения больше 1.0f делают звук громче,
	 * меньше 1.0f - тише.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Sound)
	float VolumeMultiplier = 1.0f;

	/**
	 * @brief Материал декали, который будет размещен в точке попадания.
	 * Используется для создания видимых следов (например, пробоин от пуль, пятен).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Decal)
	TObjectPtr<UMaterialInstance> DecalMaterial = nullptr;

	/**
	 * @brief Продолжительность жизни декали в секундах.
	 * Определяет, как долго декаль будет оставаться видимой на поверхности.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Decal)
	float DecalLifeSpan = 10.0f;

	/**
	 * @brief Минимальный масштаб для декали.
	 * Определяет наименьший размер декали, которая может быть размещена.
	 * Случайное значение масштаба будет выбираться между MinScale и MaxScale.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Decal)
	FVector MinScale = FVector(0.5f, 0.5f, 0.5f);

	/**
	 * @brief Максимальный масштаб для декали.
	 * Определяет наибольший размер декали, которая может быть размещена.
	 * Случайное значение масштаба будет выбираться между MinScale и MaxScale.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Decal)
	FVector MaxScale = FVector(1.0f, 1.0f, 1.0f);

	/**
	 * @brief Система частиц (Niagara System), которая будет воспроизведена в точке попадания.
	 * Используется для визуальных эффектов, таких как искры, дым, брызги.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = VFX)
	TObjectPtr<UNiagaraSystem> ImpactEffect = nullptr;

	/**
	 * @brief Масштаб для эффекта (Niagara System).
	 * Определяет размер воспроизводимого визуального эффекта.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = VFX)
	FVector EffectScale = FVector(1.0f, 1.0f, 1.0f);
};

USTRUCT(BlueprintType)
struct FHitImpactParams
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHitResult HitInfo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PitchMultiplier = 1.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHandledImpact, FGameplayTag, SurfaceTag, const FHitResult&, HitInfo);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SHOOTINGSYSTEM_API UHitImpactComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UHitImpactComponent();

public:
	/**
	 * @brief Обрабатывает событие попадания, определяя и применяя соответствующие эффекты.
	 *
	 * @param HitImpactParams Информация о месте попадания, содержащая данные о поверхности,
	 *                актере, месте и т.д.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	bool HandleImpact(const FHitImpactParams& InHitImpactParams);

	/**
	 * @brief Ищет данные об эффектах попадания для определенной поверхности по FName.
	 *
	 * @param SurfaceName Тег, идентифицирующий тип поверхности.
	 * @param FoundData Выходной параметр, в который будут скопированы найденные данные об эффектах.
	 * @return True, если данные были успешно найдены и скопированы, false в противном случае.
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Find Impact Data (By Name)"), Category = "Utility")
	bool FindImpactDataByName(const FName SurfaceName, FImpactData& FoundData) const;

	/**
	 * @brief Ищет данные об эффектах попадания для определенного тега поверхности.
	 *
	 * @param SurfaceTag Тег, идентифицирующий тип поверхности (например, "Surface.Wood", "Surface.Metal").
	 * @param FoundData Выходной параметр, в который будут скопированы найденные данные об эффектах.
	 * @return True, если данные были успешно найдены и скопированы, false в противном случае.
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Find Impact Data (By Tag)"), Category = "Utility")
	bool FindImpactDataByTag(const FGameplayTag SurfaceTag, FImpactData& FoundData) const;

	/**
	 * @brief Определяет трансформацию для спавна эффектов попадания.
	 *
	 * @param HitInfo Информация о месте попадания.
	 * @return Трансформация, подходящая для спавна эффектов.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	FTransform GetSpawnImpactTransform(const FHitResult& HitInfo) const;

public:
	/**
	 * @brief Устанавливает новую таблицу данных для эффектов попадания.
	 *
	 * @param NewDataTable Новый указатель на UDataTable, который будет использоваться.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	void SetImpactTable(UDataTable* NewDataTable);

public:
	/**
	 * @brief Возвращает информацию о последнем обработанном попадании.
	 * @return Последняя сохраненная структура FHitImpactParams.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	FORCEINLINE FHitImpactParams GetLastHitImpactParams() const { return LastHitImpact; }

	/**
	 * @brief Возвращает текущую таблицу данных для эффектов попадания.
	 *
	 * @return Указатель на UDataTable, используемый для эффектов попадания. Возвращает nullptr,
	 *         если таблица не была установлена.
	 */
	UFUNCTION(BlueprintCallable, Category = "Utility")
	FORCEINLINE UDataTable* GetImpactTable() const { return ImpactTable; }

public:
	/**
	 * @brief Название строки в таблице по дефолту, которая будет проигрыватся, когда в таблице не будет найдена указаная строка.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core")
	FName DefaultRowName = FName("Default");

public:
	/**
	 * @brief Делегат, который транслируется после успешной обработки попадания.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnHandledImpact OnHandledImpact;

protected:
	virtual void BeginPlay() override;

private:
	void SpawnDecalImpact();
	void SpawnEffectImpact();
	void PlaySoundImpact();

private:
	/**
	 * @brief Таблица данных, содержащая информацию об эффектах попадания для различных типов поверхностей.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "Data Table (FImpactData)"), Category = "Core")
	TObjectPtr<UDataTable> ImpactTable = nullptr;

private:
	/**
	 * @brief Хранит данные об эффектах, соответствующих последнему найденному типу поверхности.
	 */
	UPROPERTY()
	FImpactData ImpactData;

	/**
	 * @brief Сохраняет информацию о последнем обработанном попадании.
	 */
	FHitImpactParams LastHitImpact = FHitImpactParams();
};
