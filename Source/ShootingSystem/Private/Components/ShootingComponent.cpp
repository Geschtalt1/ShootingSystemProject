


#include "Components/ShootingComponent.h"
#include "Components/ObjectPoolComponent.h"
#include "Components/PoolableComponent.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"

#include "GameFramework/ProjectileBase.h"

#include "NiagaraFunctionLibrary.h"

#include "Curves/CurveFloat.h"

UShootingComponent::UShootingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UShootingComponent::BeginPlay()
{
	Super::BeginPlay();	

	SetupFireMode();
	SetSpreadScale(SpreadConfig.BaseValue);
	InitRecoverySpreadTimeline();
}

void UShootingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RecoverySpreadTimeline.TickTimeline(DeltaTime);
}

void UShootingComponent::StartFire()
{
	if (!bIsFiring && CanFire())
	{
		ReceiveStartFire();
		OnStartFire.Broadcast();

		// Очищаем и останавливаем таймеры и Timeline, связанные с предыдущими состояниями стрельбы или восстановления.
		// Это гарантирует, что никакие предыдущие процессы не будут мешать новому началу стрельбы.
		GetTimerManager().ClearTimer(TimerFiring);
		GetTimerManager().ClearTimer(TimerResetFire);
		GetTimerManager().ClearTimer(TimerStartRecoverySpread);
		RecoverySpreadTimeline.Stop();

		bIsFiring = true;
	
		// В зависимости от режима стрельбы, вызываем соответствующую функцию для создания выстрела(ов).
		if (FireMode == EFireMode::FM_Single)
		{
			CreateSingleFire();
		}
		else if (FireMode == EFireMode::FM_Auto)
		{
			CreateAutoFire();
		}
		else if (FireMode == EFireMode::FM_Burst)
		{
			CreateBurstFire();
		}
	}
}

void UShootingComponent::FinishFire()
{
	if ((bIsFiring) && (!GetTimerManager().IsTimerActive(TimerResetFire)))
	{
		// Очищаем таймер, который непосредственно управляет выстрелами.
		// Если он активен, он больше не нужен, так как мы переходим к завершению.
		GetTimerManager().ClearTimer(TimerFiring);

		// Сбрасываем состояние стрельбы через время.
		GetTimerManager().SetTimer(
			TimerResetFire,
			this,
			&UShootingComponent::HandleFinishedFiring,
			GetFireRate(),
			false
		);
	}
}


bool UShootingComponent::CanFire() const
{
	return CanFireInternal();
}

bool UShootingComponent::CanFireInternal_Implementation() const
{
	return HasAmmoInMagazine() && FireMode != EFireMode::FM_None;
}

void UShootingComponent::HandleFire()
{	
	FireCount++;

	SpawnShootLineTrace();
	SpawnMuzzleFlash();
	PlaySoundFire();
	PlayMontageFire();

	if (!Ammo.bInfinite)
	{
		ConsumeAmmo(1);
	}
	
	ReceiveFire();
	OnFire.Broadcast();
}

void UShootingComponent::CreateSingleFire()
{
	HandleFire();
	FinishFire();
}

void UShootingComponent::CreateAutoFire()
{
	// Создаем таймер на автоматическую стрельбу.
	GetTimerManager().SetTimer(
		TimerFiring,
		this,
		&UShootingComponent::AutoFireTimerCallback,
		GetFireRate(),
		true,
		0.0f
	);
}

void UShootingComponent::CreateBurstFire()
{
	// Создаем таймер на burst стрельбу.
	GetTimerManager().SetTimer(
		TimerFiring,
		this,
		&UShootingComponent::BurstFireTimerCallback,
		GetFireRate(),
		true,
		0.0f
	);
}

void UShootingComponent::AutoFireTimerCallback()
{
	HandleFire();

	if (!CanFire())
	{
		FinishFire();
	}
}

void UShootingComponent::BurstFireTimerCallback()
{
	HandleFire();

	if (!CanFire() || FireCount >= 3)
	{
		FinishFire();
	}
}

void UShootingComponent::HandleFinishedFiring()
{
	bIsFiring = false;
	FireCount = 0;

	// Очищаем таймеры, которые могли быть активны во время стрельбы.
	GetTimerManager().ClearTimer(TimerResetFire);
	GetTimerManager().ClearTimer(TimerStartRecoverySpread);
	
	// Устанавливаем новый таймер для запуска восстановления разброса.
	// Это позволяет сделать паузу между окончанием стрельбы и началом восстановления разброса.
	GetTimerManager().SetTimer(
		TimerStartRecoverySpread, 
		this,                   
		&UShootingComponent::StartRecoverySpread, 
		SpreadConfig.RecoveryDelay, 
		false                     
	);

	ReceiveFinishFire();
	OnFinishFire.Broadcast();
}

void UShootingComponent::SpawnShootLineTrace()
{
	for (int32 i = 0; i < BulletsPerShot; i++)
	{
		// Получаем позицию, откуда будет произведен выстрел.
		const FVector& FireLocation = GetFireLocation();
		const FVector& FireDirection = GetFireDirection() * HitscanRange;

		// Рассчитываем конечную точку выстрела, применяя разброс.
		// `CalculateSpread` добавляет случайное отклонение к вектору, определяя реальную цель трассировки.
		// `FireLocation + FireDirection` - это точка на максимальной дальности, если бы стрельба была идеально прямой.
		const FVector& End = CalculateSpread(FireLocation + FireDirection);

		// Увеличиваем текущий масштаб разброса.
		// При каждом выстреле разброс будет немного увеличиваться, пока не достигнет своего максимума.
		// `SpreadConfig.BaseThreshold` определяет, насколько увеличивается разброс за один выстрел.
		SetSpreadScale(CurrentSpreadScale + SpreadConfig.BaseThreshold);

		// Спавним лайнтрейс.
		UKismetSystemLibrary::LineTraceSingle(
			this,
			FireLocation,
			End,
			HitscanTraceType,
			false,
			{ GetOwner() },
			bDrawDebugTrace ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
			LastHitResult,
			true
		);

		PostSpawnShootLineTrace(LastHitResult);
	}
}

void UShootingComponent::PostSpawnShootLineTrace(const FHitResult& Hit)
{
	ReceivePostSpawnLineTrace(Hit);
	OnPostSpawnLineTrace.Broadcast(Hit);

	if (FireType == EFireType::FT_LineTrace)
	{
		// Обрабатываем попадание моментально.
		HandleHit(Hit);
	}
	else
	{
		// Спавним проджектайл.
		SpawnProjectile(Hit);
	}
}

void UShootingComponent::HandleHit(const FHitResult& Hit)
{
	if (!Hit.bBlockingHit)
	{
		return;
	}

	// Обрабатываем урон и применяем его к цели.
	const float HandledDamage = HandleCalculateDamage(Hit);
	HandleApplyDamageInternal(HandledDamage, Hit);
	
	if (bDrawDebugDamage)
	{
		DrawDebugApplyDamage(HandledDamage);
	}

	// Спавним эффекты попадания.

	OnTargetHit.Broadcast(Hit.GetActor(), HandledDamage, Hit, FGameplayTag());
}

float UShootingComponent::HandleCalculateDamage(const FHitResult& HitInfo)
{
	return HandleCalculateDamageInternal(HitInfo);
}

float UShootingComponent::HandleCalculateDamageInternal_Implementation(const FHitResult& HitInfo)
{
	float DamageValue = DamageConfig.BaseDamage;

	// Проверяем, нужно ли делить урон на количество "снарядов" за выстрел.
	// Это актуально для оружия типа дробовика, где каждая пуля наносит часть общего урона.
	if ((BulletsPerShot > 1) && (DamageConfig.bDivideDamageByBulletsPerShot))
	{
		// Делим базовый урон на количество пуль, чтобы получить урон ОДНОЙ пули.
		// Если BulletsPerShot = 5, а BaseDamage = 100, то DamageValue станет 20.
		DamageValue = DamageValue / BulletsPerShot;
	}

	// Рассчитываем итоговый урон, учитывая затухание (falloff) от расстояния.
	return CalculateDamageByDistance(DamageValue, HitInfo.Distance);
}

void UShootingComponent::HandleApplyDamageInternal_Implementation(float InDamage, const FHitResult& HitInfo)
{
	const FVector& OwnerOriginLocation = GetOwner()->GetActorLocation();
	const FVector& HitFromFirection = HitInfo.ImpactPoint - OwnerOriginLocation;
	AController* InstigatorController = GetOwner()->GetInstigatorController();

	UGameplayStatics::ApplyPointDamage(
		HitInfo.GetActor(),
		InDamage,
		HitFromFirection,
		HitInfo,
		InstigatorController,
		GetOwner(),
		DamageConfig.DamageType
	);
}

FVector UShootingComponent::GetFireLocation() const
{
	return GetFireLocationInternal();
}

FVector UShootingComponent::GetFireLocationInternal_Implementation() const
{
	return GetMuzzleSocketLocation();
}

FVector UShootingComponent::GetFireDirection() const
{
	return GetFireDirectionInternal();
}

FVector UShootingComponent::GetFireDirectionInternal_Implementation() const
{
	return Mesh ? Mesh->GetForwardVector() : FVector();
}

FVector UShootingComponent::GetMuzzleSocketLocation() const
{
	return Mesh ? Mesh->GetSocketLocation(MuzzleSocketName) : FVector();
}

void UShootingComponent::SpawnProjectile(const FHitResult& Hit)
{
	if (!ProjectilePoolManager)
	{
		return;
	}

	// Определяем начальную точку для спавна снаряда - это позиция сокета "Muzzle".
	const FVector& Start = GetMuzzleSocketLocation();

	// Определяем целевую точку, куда будет направлен снаряд.
	// Если Line Trace был блокирующим (т.е. попал во что-то), используем точку попадания (ImpactPoint).
	// В противном случае (если Line Trace прошел насквозь или не попал ни во что), используем конечную точку трассировки (TraceEnd).
	const FVector& Target = Hit.bBlockingHit ? Hit.ImpactPoint : Hit.TraceEnd;

	// Рассчитываем вращение, которое смотрит от начальной точки (Start) к целевой точке (Target).
	// Это вращение будет применено к снаряду, чтобы он летел в нужном направлении.
	const FRotator LookRotator = UKismetMathLibrary::FindLookAtRotation(Start, Target);

	UPoolableComponent* Object;
	FTransform Transform = FTransform(LookRotator, Start);

	// Запрашиваем снаряд из пула.
	ProjectilePoolManager->SpawnPooledObject(
		Transform,
		GetOwner(),
		Object
	);
}

void UShootingComponent::SpawnMuzzleFlash()
{
	if (Mesh && MuzzleFlash)
	{
		// Проверяем, существует ли на меш-компоненте сокет с именем, указанным в MuzzleSocketName.
		// Это важно, чтобы эффект появлялся в правильной точке (например, на конце ствола).
		if (!Mesh->DoesSocketExist(MuzzleSocketName))
		{
			// Если сокет не найден, прерываем выполнение функции, чтобы избежать ошибок.
			return;
		}

		// Используем UNiagaraFunctionLibrary для спауна Niagara-системы (эффекта вспышки).
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			MuzzleFlash,
			Mesh,
			MuzzleSocketName,
			FVector(),
			FRotator(),
			EAttachLocation::SnapToTarget,
			true
		);
	}
}

void UShootingComponent::PlaySoundFire()
{
	if (Mesh && FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			FireSound,
			Mesh->GetComponentLocation()
		);
	}
}

void UShootingComponent::PlayMontageFire()
{
	if (Mesh && FireMontage)
	{
		UAnimInstance* AnimInst = Mesh->GetAnimInstance();
		if (!AnimInst)
		{
			return;
		}

		AnimInst->Montage_Play(
			FireMontage,
			1.0f,
			EMontagePlayReturnType::MontageLength,
			0.0f,
			bStoAllMontagesWhenPlayedAnimFire
		);
	}
}

void UShootingComponent::RefreshMagazine()
{
	FAmmoInfo NewAmmo = Ammo;

	// Определяем, сколько места свободно в магазине
	int32 SpaceInClip = Ammo.Max - Ammo.Current;

	// Если в магазине нет места или в запасе нет патронов, ничего не делаем
	if (SpaceInClip <= 0 || Ammo.Total <= 0)
	{
		return; // Выходим, т.к. ничего не перенесено
	}

	// Определяем, сколько патронов реально можем перенести
	// Это минимум из свободного места и доступных в запасе патронов
	int32 AmmoToTransfer = FMath::Min(SpaceInClip, Ammo.Total);

	// Обновляем количество патронов
	NewAmmo.Current += AmmoToTransfer;
	NewAmmo.Total -= AmmoToTransfer;

	// Убеждаемся, что количество патронов не превышает максимум
	NewAmmo.Current = FMath::Min(NewAmmo.Current, NewAmmo.Max);

	// Обновляем патроны.
	SetAmmo(NewAmmo);
}

void UShootingComponent::NextFireMode()
{
	if (AvailableFireModes.IsEmpty())
	{
		return;
	}

	const int32 Index = AvailableFireModes.Find(FireMode);
	const EFireMode NewFireMode = AvailableFireModes.IsValidIndex(Index + 1) ? AvailableFireModes[Index + 1] : AvailableFireModes[0];

	SetFireMode(NewFireMode);
}

FVector UShootingComponent::CalculateSpread(const FVector& InDirection)
{
	return CalculateSpreadInternal(InDirection);
}

FVector UShootingComponent::CalculateSpreadInternal_Implementation(const FVector& InDirection)
{
	// Рассчитываем случайное отклонение для каждой оси (X, Y, Z).
	// Диапазон отклонения от -GetSpreadScale() до +GetSpreadScale().
	// Чем больше GetSpreadScale(), тем больше будет случайное отклонение.
	const float X = UKismetMathLibrary::RandomFloatInRange(GetSpreadScale() * (-1.f), GetSpreadScale());
	const float Y = UKismetMathLibrary::RandomFloatInRange(GetSpreadScale() * (-1.f), GetSpreadScale());
	const float Z = UKismetMathLibrary::RandomFloatInRange(GetSpreadScale() * (-1.f), GetSpreadScale());

	return InDirection + FVector(X, Y, Z);
}

float UShootingComponent::CalculateDamageByDistance(float InDamage, float InDistance) const
{
	return UKismetMathLibrary::MapRangeClamped(
		InDistance,
		DamageConfig.DamageFalloffStartDistance,
		DamageConfig.DamageFalloffEndDistance,
		InDamage,
		InDamage * (DamageConfig.DamageFalloffMaxPercentage / 100.f)
	);
}

void UShootingComponent::InitRecoverySpreadTimeline()
{
	// Создаем временный объект FTimelineFloatTrack.
	// Привязываем функцию "UpdateRecoverySpread" к событию обновления float-дорожки.
	FTimelineFloatTrack ProgressUpdate;
	ProgressUpdate.InterpFunc.BindUFunction(this, "UpdateRecoverySpread");

	// Создаем кривую.
	RecoverySpreadCurve = NewObject<UCurveFloat>(this, "RecoverySpread");

	// Сбрасываем существующие ключи кривой, чтобы начать с чистого листа.
	RecoverySpreadCurve->FloatCurve.Reset();

	// Добавляем первую точку в кривую: время 0.0, значение 0.0.
	// Добавляем вторую точку в кривую: время 1.0, значение 1.0.
	RecoverySpreadCurve->FloatCurve.AddKey(0.0f, 0.0f);
	RecoverySpreadCurve->FloatCurve.AddKey(1.0f, 1.0f);

	RecoverySpreadTimeline.AddInterpFloat(RecoverySpreadCurve, ProgressUpdate.InterpFunc);
}

void UShootingComponent::StartRecoverySpread()
{
	CashSpreadScale = CurrentSpreadScale;

	GetTimerManager().ClearTimer(TimerStartRecoverySpread);
	RecoverySpreadTimeline.SetPlayRate(1.0f / SpreadConfig.RecoveryRate);
	RecoverySpreadTimeline.PlayFromStart();
}

void UShootingComponent::UpdateRecoverySpread(float Alpha)
{
	const float NewSpreadScale = UKismetMathLibrary::Lerp(
		CashSpreadScale,
		SpreadConfig.BaseValue,
		Alpha
	);

	SetSpreadScale(NewSpreadScale);
}

void UShootingComponent::Notify_HandleHit(const FHitResult& InHit)
{
	HandleHit(InHit);
}

void UShootingComponent::SetupFireMode()
{
	if (AvailableFireModes.IsEmpty())
	{
		FireMode = EFireMode::FM_None;
	}
	else
	{
		if (!AvailableFireModes.Contains(FireMode))
		{
			SetFireMode(AvailableFireModes[0]);
		}
	}
}

void UShootingComponent::ConsumeAmmo(int32 Amount)
{
	if (Ammo.Current > 0)
	{
		Ammo.Current--;
		SetAmmo(Ammo);
	}
}

void UShootingComponent::SetAmmo(FAmmoInfo NewAmmo)
{
	Ammo = NewAmmo;

	ReceiveUpdateAmmo(Ammo);
	OnAmmoUpdated.Broadcast(Ammo);
}

void UShootingComponent::SetFireType(EFireType NewFireType)
{
	FireType = NewFireType;
}

bool UShootingComponent::SetFireMode(EFireMode NewFireMode, bool bForce)
{
	if ((FireMode != NewFireMode) && (AvailableFireModes.Contains(NewFireMode)) || (bForce))
	{
		FireMode = NewFireMode;
		OnFireModeChanged.Broadcast(FireMode);
		return true;
	}
	return false;
}

void UShootingComponent::SetAvailableFireModes(const TArray<EFireMode>& NewAvailableFireModes)
{
	AvailableFireModes = NewAvailableFireModes;
}

void UShootingComponent::SetFireRate(float NewFireRate)
{
	FireRate = NewFireRate;
}

void UShootingComponent::SetSpreadConfig(FSpreadConfig NewSpreadConfig)
{
	SpreadConfig = NewSpreadConfig;
	SetSpreadScale(SpreadConfig.BaseValue);
}

void UShootingComponent::SetSpreadScale(float NewScale)
{
	CurrentSpreadScale = FMath::Clamp(
		NewScale,
		0.0f,
		SpreadConfig.MaxValue
	);
}

void UShootingComponent::SetDamageConfig(FDamageConfig NewDamageConfig)
{
	DamageConfig = NewDamageConfig;
}

void UShootingComponent::SetHitscanRange(float NewRange)
{
	HitscanRange = NewRange;
}

void UShootingComponent::SetHitscanTraceType(TEnumAsByte<ETraceTypeQuery> NewHitscanTraceType)
{
	HitscanTraceType = NewHitscanTraceType;
}

void UShootingComponent::SetBulletPerShot(int32 NewBulletPerShot)
{
	BulletsPerShot = NewBulletPerShot;
}

void UShootingComponent::SetProjectilePoolManager(UObjectPoolComponent* NewManager, bool bRefresh)
{
	ProjectilePoolManager = NewManager;

	if (bRefresh && NewManager && ProjectileActorClass)
	{
		NewManager->SetPooledActorClass(ProjectileActorClass, true);
		NewManager->SetPoolReady(true);
	}
}

void UShootingComponent::SetMesh(USkeletalMeshComponent* NewMesh)
{
	Mesh = NewMesh;
}

void UShootingComponent::SetFireSound(USoundBase* NewFireSound)
{
	FireSound = NewFireSound;
}

void UShootingComponent::SetMuzzleFlash(UNiagaraSystem* NewMuzzleFlash)
{
	MuzzleFlash = NewMuzzleFlash;
}

void UShootingComponent::SetFireMontage(UAnimMontage* NewFireMontage)
{
	FireMontage = NewFireMontage;
}

void UShootingComponent::DrawDebugApplyDamage(float InDamage)
{
	UKismetSystemLibrary::DrawDebugString(
		this,
		LastHitResult.ImpactPoint,
		FString::Printf(TEXT("Damage: %.2f"), InDamage),
		nullptr,
		FLinearColor::White,
		2.0f
	);
}