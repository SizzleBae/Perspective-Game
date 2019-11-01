// Fill out your copyright notice in the Description page of Project Settings.

#include "PG_PlayerController.h"
#include "PG_Character.h"

APG_PlayerController::APG_PlayerController()
{
	bShowMouseCursor = true;
}

void APG_PlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if(IsValid(Character)) 
	{

		FVector MouseWorldLocation, MouseWorldDirection;
		DeprojectMousePositionToWorld(MouseWorldLocation, MouseWorldDirection);

		FHitResult Result;
		GetWorld()->LineTraceSingleByChannel(Result, MouseWorldLocation, MouseWorldLocation + (MouseWorldDirection * 9999.f), ECC_WorldDynamic);
	
		if(Result.bBlockingHit) 
		{
			LookTowardsLocation = (0.9f * LookTowardsLocation) + (0.1f * Result.ImpactPoint);
			Character->LookTowards(LookTowardsLocation);
		}
	}
}

void APG_PlayerController::OnPossess(APawn* aPawn)
{
	Super::OnPossess(aPawn);

	this->Character = Cast <APG_Character>(aPawn);
}


