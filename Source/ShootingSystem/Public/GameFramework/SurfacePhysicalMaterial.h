
#pragma once

#include "GameplayTagContainer.h"

#include "CoreMinimal.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "SurfacePhysicalMaterial.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTINGSYSTEM_API USurfacePhysicalMaterial : public UPhysicalMaterial
{
	GENERATED_BODY()
	

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = PhysicalProperties)
	FGameplayTag SurfaceTag;
};
