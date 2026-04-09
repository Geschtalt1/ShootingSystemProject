

#include "Components/HitImpactComponent.h"
#include "Components/DecalComponent.h"

#include "GameFramework/SurfacePhysicalMaterial.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"

#include "NiagaraFunctionLibrary.h"

UHitImpactComponent::UHitImpactComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UHitImpactComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UHitImpactComponent::HandleImpact(const FHitImpactParams& InHitImpactParams)
{
	if (!InHitImpactParams.HitInfo.bBlockingHit)
	{
		return false;
	}

	// Сохраняем информацию о последнем попадании для последующего использования.
	LastHitImpact = InHitImpactParams;

	// Пытаемся получить физический материал поверхности, на которую произошло попадание.
	USurfacePhysicalMaterial* SurfaceMaterial = Cast<USurfacePhysicalMaterial>(LastHitImpact.HitInfo.PhysMaterial);
	if (!SurfaceMaterial) 
	{
		return false;
	}

	// Ищем данные об эффектах попадания, соответствующие этому тегу поверхности.
	// Если данные не найдены (FindImpactData возвращает false), выходим.
	const FGameplayTag& SurfaceTag = SurfaceMaterial->SurfaceTag;
	if (!FindImpactDataByTag(SurfaceTag, ImpactData))
	{
		// Пробуем найти дефолтные данные.
		if (!FindImpactDataByName(DefaultRowName, ImpactData))
		{
			return false;
		}
	}

	// Спавним визуальные эффекты и звуки.
	SpawnDecalImpact();
	SpawnEffectImpact();
	PlaySoundImpact();

	OnHandledImpact.Broadcast(SurfaceTag, LastHitImpact.HitInfo);
	return true;
}

bool UHitImpactComponent::FindImpactDataByName(const FName SurfaceName, FImpactData& FoundData) const
{
	if (!ImpactTable || !SurfaceName.IsValid())
	{
		return false;
	}

	// Получение структуры таблицы и проверка ее соответствия FImpactData.
	const UScriptStruct* Struct = ImpactTable->GetRowStruct();
	if (Struct->IsChildOf<FImpactData>() != true)
	{
		return false;
	}

	// Поиск строки данных в таблице по тегу поверхности.
	FImpactData* SlotData = ImpactTable->FindRow<FImpactData>(
		SurfaceName,
		FString(), // FString для логирования ошибок, здесь пустой.
		true // bAllowNotFound - true, чтобы FindRow возвращал nullptr, а не падал.
	);

	if (SlotData)
	{
		// Если данные найдены, копируем их в FoundData.
		FoundData = *SlotData;
	}

	return SlotData != nullptr;
}

bool UHitImpactComponent::FindImpactDataByTag(const FGameplayTag SurfaceTag, FImpactData& FoundData) const
{
	return FindImpactDataByName(SurfaceTag.GetTagName(), FoundData);
}

FTransform UHitImpactComponent::GetSpawnImpactTransform(const FHitResult& HitInfo) const
{
	return FTransform(
		UKismetMathLibrary::MakeRotFromX(HitInfo.Normal),
		HitInfo.ImpactPoint
	);
}

void UHitImpactComponent::SpawnDecalImpact()
{
	if (ImpactData.DecalMaterial)
	{
		// Определяем рандомный размер декали.
		const float X = UKismetMathLibrary::RandomFloatInRange(ImpactData.MinScale.X, ImpactData.MaxScale.X);
		const float Y = UKismetMathLibrary::RandomFloatInRange(ImpactData.MinScale.Y, ImpactData.MaxScale.Y);
		const float Z = UKismetMathLibrary::RandomFloatInRange(ImpactData.MinScale.Z, ImpactData.MaxScale.Z);

		// Определяем рандомный поворот декали.
		const float RandomDeltaRoll = UKismetMathLibrary::RandomFloatInRange(0.0f, 360.0f);

		const FTransform SpawnTransform = GetSpawnImpactTransform(LastHitImpact.HitInfo);

		// Спавн декали.
		UDecalComponent* DecalComponent = UGameplayStatics::SpawnDecalAtLocation(
			this,
			ImpactData.DecalMaterial,
			FVector(X, Y, Z),
			SpawnTransform.GetLocation(),
			SpawnTransform.Rotator(),
			ImpactData.DecalLifeSpan
		);

		DecalComponent->AddRelativeRotation(FRotator(0.0f, 0.0f, RandomDeltaRoll));
		DecalComponent->SetFadeScreenSize(0.0f);
		DecalComponent->FadeInStartDelay = 20.f;
		DecalComponent->FadeDuration = 0.5f;

		// Прикрепляем декаль в попавший компонент.
		AActor* OwnerDecalComp = DecalComponent->GetOwner();
		FAttachmentTransformRules NewAttachmentRules = FAttachmentTransformRules(EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, true);
		OwnerDecalComp->AttachToComponent(LastHitImpact.HitInfo.GetComponent(), NewAttachmentRules);
	}
}

void UHitImpactComponent::SpawnEffectImpact()
{
	if (ImpactData.ImpactEffect)
	{
		// Получаем трансформацию (позицию и вращение) для спавна эффекта.
		// Используем GetSpawnImpactTransform для определения правильного места и ориентации.

		const FTransform SpawnTransform = GetSpawnImpactTransform(LastHitImpact.HitInfo);

		// Спавним систему частиц Niagara в указанной локации с нужным вращением и масштабом.
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			ImpactData.ImpactEffect,
			SpawnTransform.GetLocation(),
			SpawnTransform.Rotator(),
			ImpactData.EffectScale
		);
	}
}

void UHitImpactComponent::PlaySoundImpact()
{
	if (ImpactData.ImpactSound)
	{
		// Воспроизводим звук в локации попадания с заданным множителем громкости.
		UGameplayStatics::PlaySoundAtLocation(
			this,
			ImpactData.ImpactSound,
			LastHitImpact.HitInfo.ImpactPoint,
			ImpactData.VolumeMultiplier * LastHitImpact.VolumeMultiplier,
			LastHitImpact.PitchMultiplier
		);
	}
}

void UHitImpactComponent::SetImpactTable(UDataTable* NewDataTable)
{
	ImpactTable = NewDataTable;
}