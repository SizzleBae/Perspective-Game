// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "PG_Character.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Kismet/KismetMathLibrary.h"
#include "PaperFlipbookComponent.h"

APG_Character::APG_Character()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bAbsoluteRotation = true; // Rotation of the character should not affect rotation of boom
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->TargetArmLength = 500.f;
	CameraBoom->SocketOffset = FVector(0.f,0.f,75.f);
	CameraBoom->RelativeRotation = FRotator(0.f,180.f,0.f);

	// Create a camera and attach to boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	SideViewCameraComponent->bUsePawnControlRotation = false; // We don't want the controller rotating the camera

	// Create a camera and attach to head
	EyeCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("EyeSceneCapture"));
	EyeCaptureComponent->SetupAttachment(GetCapsuleComponent());

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Face in the direction we are moving..
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->GravityScale = 2.f;
	GetCharacterMovement()->AirControl = 0.80f;
	GetCharacterMovement()->JumpZVelocity = 1000.f;
	GetCharacterMovement()->GroundFriction = 3.f;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MaxFlySpeed = 300.f;

	// Try to create the sprite component
	Sprite = CreateOptionalDefaultSubobject<UPaperFlipbookComponent>("CharacterSprite");
	if(Sprite)
	{
		Sprite->AlwaysLoadOnClient = true;
		Sprite->AlwaysLoadOnServer = true;
		Sprite->bOwnerNoSee = false;
		Sprite->bAffectDynamicIndirectLighting = true;
		Sprite->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		Sprite->SetupAttachment(GetCapsuleComponent());
		static FName CollisionProfileName(TEXT("CharacterMesh"));
		Sprite->SetCollisionProfileName(CollisionProfileName);
		Sprite->SetGenerateOverlapEvents(false);
	}
}

void APG_Character::LookTowards(const FVector& TargetLocation)
{
	// Point eye render target camera towards target location
	FVector NewEyeForward = TargetLocation - EyeCaptureComponent->GetComponentLocation();
	EyeCaptureComponent->SetWorldRotation(NewEyeForward.Rotation());

	// Pan camera slightly towards location
	FVector NewCameraForward = TargetLocation - SideViewCameraComponent->GetComponentLocation();
	FRotator NewCameraRotation = NewCameraForward.Rotation();
	
	FTransform NewCameraLocalTransform = UKismetMathLibrary::MakeRelativeTransform(SideViewCameraComponent->GetAttachParent()->GetComponentTransform(), FTransform(NewCameraRotation));
	
	// Limit based on MaxCameraDegOffset
	FRotator NewCameraLocalRotation = NewCameraLocalTransform.Rotator();
	NewCameraLocalRotation.Yaw = LimitAngle(NewCameraLocalRotation.Yaw);
	NewCameraLocalRotation.Pitch = LimitAngle(NewCameraLocalRotation.Pitch);
	NewCameraLocalRotation.Roll = 0.f;

	SideViewCameraComponent->SetRelativeRotation(NewCameraLocalRotation.GetInverse());
}

void APG_Character::UpdateCharacter()
{
	// Update animation to match the motion
	UpdateAnimation();

	// Now setup the rotation of the controller based on the direction we are travelling
	const FVector PlayerVelocity = GetVelocity();
	float TravelDirection = PlayerVelocity.X;
	// Set the rotation so that the character faces his direction of travel.
	if(Controller != nullptr)
	{
		if(TravelDirection < 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0, 180.0f, 0.0f));
		}
		else if(TravelDirection > 0.0f)
		{
			Controller->SetControlRotation(FRotator(0.0f, 0.0f, 0.0f));
		}
	}
}

void APG_Character::UpdateAnimation()
{
	const FVector PlayerVelocity = GetVelocity();
	const float PlayerSpeedSqr = PlayerVelocity.SizeSquared();

	// Are we moving or standing still?
	UPaperFlipbook* DesiredAnimation = (PlayerSpeedSqr > 0.0f) ? RunningAnimation : IdleAnimation;
	if(Sprite->GetFlipbook() != DesiredAnimation)
	{
		Sprite->SetFlipbook(DesiredAnimation);
	}
}

FVector APG_Character::GetEyeLocation() const
{
	return EyeCaptureComponent->GetComponentLocation();
}

void APG_Character::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up game play key bindings
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAxis("MoveRight", this, &APG_Character::MoveRight);
	PlayerInputComponent->BindAxis("MoveForward", this, &APG_Character::MoveForward);

	PlayerInputComponent->BindTouch(IE_Pressed, this, &APG_Character::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &APG_Character::TouchStopped);
}

void APG_Character::BeginPlay()
{
	Super::BeginPlay();

	MPC_LOSInstance = GetWorld()->GetParameterCollectionInstance(MPC_LOSAsset);
}

float APG_Character::LimitAngle(float Angle) const
{
	float SmoothFactor = FMath::SmoothStep(0, MaxCameraDegOffset * 3.f, FMath::Abs(Angle));
	UE_LOG(LogTemp, Warning, TEXT("Given: %f, SmoothFactor: %f"), Angle, SmoothFactor);
	return  SmoothFactor * MaxCameraDegOffset * FMath::Sign(Angle);
}

void APG_Character::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Extract view projection from eye camera to use in shaders
	FMinimalViewInfo ViewInfo;
	EyeCaptureComponent->GetCameraView(0, ViewInfo);

	// Matrix used in materials to project world coordinates to eye camera screen space
	FMatrix EyeMatrix = FRotationTranslationMatrix(ViewInfo.Rotation, ViewInfo.Location).Inverse() 
		* FLookAtMatrix(FVector(0, 0, 0), FVector(-1.f, 0, 0), FVector(0, 0, -1.f)) 
		* ViewInfo.CalculateProjectionMatrix();

	// Matrix must be split and sent in to vector parameters
	MPC_LOSInstance->SetVectorParameterValue("PVRow0", FLinearColor(EyeMatrix.M[0][0], EyeMatrix.M[0][1], EyeMatrix.M[0][2], EyeMatrix.M[0][3]));
	MPC_LOSInstance->SetVectorParameterValue("PVRow1", FLinearColor(EyeMatrix.M[1][0], EyeMatrix.M[1][1], EyeMatrix.M[1][2], EyeMatrix.M[1][3]));
	MPC_LOSInstance->SetVectorParameterValue("PVRow2", FLinearColor(EyeMatrix.M[2][0], EyeMatrix.M[2][1], EyeMatrix.M[2][2], EyeMatrix.M[2][3]));
	MPC_LOSInstance->SetVectorParameterValue("PVRow3", FLinearColor(EyeMatrix.M[3][0], EyeMatrix.M[3][1], EyeMatrix.M[3][2], EyeMatrix.M[3][3]));

	UpdateCharacter();
}

void APG_Character::MoveRight(float Value)
{
	// add movement in that direction
	AddMovementInput(FVector(0.f,-1.f,0.f), Value);
}

void APG_Character::MoveForward(float Value)
{
	// add movement in that direction
	AddMovementInput(FVector(-1.f, 0.f, 0.f), Value);
}

void APG_Character::TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// jump on any touch
	Jump();
}

void APG_Character::TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	StopJumping();
}
