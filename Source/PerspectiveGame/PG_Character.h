// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PG_Character.generated.h"

UCLASS(config=Game)
class APG_Character : public ACharacter
{
	GENERATED_BODY()

public:
	/** Side view camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera)
	class UCameraComponent* SideViewCameraComponent;

	/** Eye view camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera)
	class USceneCaptureComponent2D* EyeCaptureComponent;

	/** Camera boom positioning the camera beside the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Camera)
		UMaterialParameterCollection* MPC_LOSAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Camera)
		float MaxCameraDegOffset = 15.f;
	
	// The animation to play while running around
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
		class UPaperFlipbook* RunningAnimation;

	// The animation to play while idle (standing still)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Animations)
		class UPaperFlipbook* IdleAnimation;

	/** The main skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly)
		class UPaperFlipbookComponent* Sprite;

private:
	UMaterialParameterCollectionInstance* MPC_LOSInstance;

public:
	APG_Character();

	/** Returns SideViewCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetSideViewCameraComponent() const { return SideViewCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	FORCEINLINE FVector GetEyeLocation() const;

	void LookTowards(const FVector& TargetLocation);

protected:
	void MoveRight(float Value);
	void MoveForward(float Value);

	/** Handle touch inputs. */
	void TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location);

	/** Handle touch stop event. */
	void TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// End of APawn interface

	virtual void Tick(float DeltaSeconds) override;

	virtual void BeginPlay() override;

	/** Called to choose the correct animation to play based on the character's movement state */
	void UpdateAnimation();
	void UpdateCharacter();

private:
	float LimitAngle(float Angle) const;

};
