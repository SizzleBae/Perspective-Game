// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "PG_Character.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Materials/MaterialParameterCollectionInstance.h"

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
	//EyeCaptureComponent->SetupAttachment(GetMesh(), "head");

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Face in the direction we are moving..
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->GravityScale = 2.f;
	GetCharacterMovement()->AirControl = 0.80f;
	GetCharacterMovement()->JumpZVelocity = 1000.f;
	GetCharacterMovement()->GroundFriction = 3.f;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MaxFlySpeed = 600.f;

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

FVector4 APG_Character::MultiplyVectorMatrix(const FVector4& Vector, const FMatrix& Matrix) const
{

	FVector4 Row0(Matrix.M[0][0], Matrix.M[0][1], Matrix.M[0][2], Matrix.M[0][3]);
	FVector4 Row1(Matrix.M[1][0], Matrix.M[1][1], Matrix.M[1][2], Matrix.M[1][3]);
	FVector4 Row2(Matrix.M[2][0], Matrix.M[2][1], Matrix.M[2][2], Matrix.M[2][3]);
	FVector4 Row3(Matrix.M[3][0], Matrix.M[3][1], Matrix.M[3][2], Matrix.M[3][3]);

	return (Row0 * Vector.X) + (Row1 * Vector.Y) + (Row2 * Vector.Z) + (Row3 * Vector.W);

}

void APG_Character::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Extract view projection from eye camera to use in shaders
	FMinimalViewInfo ViewInfo;
	EyeCaptureComponent->GetCameraView(0, ViewInfo);

	FMatrix P = ViewInfo.CalculateProjectionMatrix();
	FMatrix V = (FTranslationMatrix(ViewInfo.Location) * FRotationMatrix(ViewInfo.Rotation)).Inverse();
	
	FMatrix PV = V * P;

	MPC_LOSInstance->SetVectorParameterValue("PVRow0", FLinearColor(PV.M[0][0], PV.M[0][1], PV.M[0][2], PV.M[0][3]));
	MPC_LOSInstance->SetVectorParameterValue("PVRow1", FLinearColor(PV.M[1][0], PV.M[1][1], PV.M[1][2], PV.M[1][3]));
	MPC_LOSInstance->SetVectorParameterValue("PVRow2", FLinearColor(PV.M[2][0], PV.M[2][1], PV.M[2][2], PV.M[2][3]));
	MPC_LOSInstance->SetVectorParameterValue("PVRow3", FLinearColor(PV.M[3][0], PV.M[3][1], PV.M[3][2], PV.M[3][3]));
	
	FVector TestPos(200.f, 100.f, 50.f);

	UE_LOG(LogTemp, Warning, TEXT("World : %s"), *TestPos.ToString());
	//TestPos = MultiplyVectorMatrix(TestPos, V);
	UE_LOG(LogTemp, Warning, TEXT("Local : %s"), *TestPos.ToString());
	TestPos = MultiplyVectorMatrix(TestPos, V * P);
	TestPos /= TestPos.Z;
	UE_LOG(LogTemp, Warning, TEXT("Screen: %s"), *TestPos.ToString());
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
