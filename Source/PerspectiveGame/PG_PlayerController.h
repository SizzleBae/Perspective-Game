// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PG_PlayerController.generated.h"

/**
 * 
 */
UCLASS()
class PERSPECTIVEGAME_API APG_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
private:
	class APG_Character* Character;

	FVector LookTowardsLocation;

public:
	APG_PlayerController();

	virtual void PlayerTick(float DeltaTime) override;

protected:
	virtual void OnPossess(APawn* aPawn) override;

};
