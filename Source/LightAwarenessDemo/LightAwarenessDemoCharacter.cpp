// Copyright Epic Games, Inc. All Rights Reserved.

#include "LightAwarenessDemoCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "LightAwarenessConfig.h"


DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ALightAwarenessDemoCharacter

ALightAwarenessDemoCharacter::ALightAwarenessDemoCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(FollowCamera);  // 카메라 앞에 붙이기
	Flashlight->Intensity = 5000.f;
	Flashlight->AttenuationRadius = 1000.f;
	Flashlight->InnerConeAngle = 20.f;
	Flashlight->OuterConeAngle = 35.f;
	Flashlight->SetVisibility(false, true);
}

void ALightAwarenessDemoCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
	float Interval = (Config ? Config->SampleInterval : 0.05f);

	GetWorldTimerManager().SetTimer(
		LightSampleTimer,
		this,
		&ALightAwarenessDemoCharacter::UpdateLightLevel,
		Interval,
		true
	);
}

//////////////////////////////////////////////////////////////////////////
// Input

void ALightAwarenessDemoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALightAwarenessDemoCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALightAwarenessDemoCharacter::Look);

		EnhancedInputComponent->BindAction(
			FlashlightAction,
			ETriggerEvent::Started,
			this,
			&ALightAwarenessDemoCharacter::ToggleFlashlight
		);

	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ALightAwarenessDemoCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ALightAwarenessDemoCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

bool ALightAwarenessDemoCharacter::IsLightOccluded(const FVector& From, const AActor* LightActor) const
{
	if (!LightActor) return true;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LightOcclusion), false, this);

	GetWorld()->LineTraceSingleByChannel(
		Hit,
		From,
		LightActor->GetActorLocation(),
		ECC_Visibility,
		Params
	);

	if (bDrawLightDebug)
	{
		FColor C = Hit.bBlockingHit ? FColor::Red : FColor::Green;
		DrawDebugLine(GetWorld(), From, LightActor->GetActorLocation(), C, false, 0.05f, 0, 1.f);
	}

	// 빛에 닿기 전에 무언가 맞으면 가려진 것
	return (Hit.bBlockingHit && Hit.GetActor() != LightActor);
}

//////////////////////////////////////////////////////////////////////////
// Light Level Calculation
float ALightAwarenessDemoCharacter::CalculateLightLevel()
{
	UWorld* World = GetWorld();

	// --- PIE World 강제 적용 (필요한 경우) ---
	for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
	{
		if (Ctx.WorldType == EWorldType::PIE)
		{
			World = Ctx.World();
			break;
		}
	}

	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[LightSense] World is NULL!"));
		return 0.f;
	}

	// ---- Config fallback ----
	const float DistFalloffK = (Config ? Config->DistFalloffK : 10.f);
	const float MaxPointWeight = (Config ? Config->MaxPointWeight : 1.f);
	const float OccludedFactor = (Config ? Config->OccludedFactor : 0.2f);
	const float LightNorm = (Config ? Config->LightNorm : 500.f);

	FVector PlayerLoc = GetActorLocation();

	float Sum = 0.f;
	int32 Count = 0;

	// DEBUG: 월드 안의 LightComponent 개수 세기
	int32 DebugPoint = 0;
	int32 DebugSpot = 0;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* A = *It;
		if (!A) continue;

		if (A->FindComponentByClass<UPointLightComponent>())
			DebugPoint++;

		if (A->FindComponentByClass<USpotLightComponent>())
			DebugSpot++;
	}

	UE_LOG(LogTemp, Warning, TEXT("DEBUG: Found PointLights=%d SpotLights=%d"), DebugPoint, DebugSpot);

	// ----- 모든 Actor를 돌며 LightComponent 탐색 -----
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor) continue;

		// ========== POINT LIGHT ==========
		if (UPointLightComponent* PLC = Actor->FindComponentByClass<UPointLightComponent>())
		{
			if (!PLC->IsRegistered()) continue;

			FVector LPos = PLC->GetComponentLocation();
			float Dist = FVector::Distance(PlayerLoc, LPos);

			if (Dist <= PLC->AttenuationRadius)
			{
				float Raw = PLC->Intensity / FMath::Max(Dist * DistFalloffK, 1.f);
				float Val = FMath::Clamp(Raw / LightNorm, 0.f, MaxPointWeight);

				if (IsLightOccluded(PlayerLoc, Actor))
					Val *= OccludedFactor;

				UE_LOG(LogTemp, Warning,
					TEXT("[POINT] %s | Intens=%.1f Dist=%.1f Raw=%.2f Val=%.2f"),
					*Actor->GetName(), PLC->Intensity, Dist, Raw, Val);

				Sum += Val;
				Count++;
			}
		}

		// ========== SPOT LIGHT ==========
		if (USpotLightComponent* SLC = Actor->FindComponentByClass<USpotLightComponent>())
		{
			if (!SLC->IsRegistered()) continue;

			FVector LPos = SLC->GetComponentLocation();
			FVector ToPlayer = PlayerLoc - LPos;
			float Dist = ToPlayer.Length();

			if (Dist <= SLC->AttenuationRadius)
			{
				FVector Forward = SLC->GetComponentRotation().Vector();
				float CosTheta = FVector::DotProduct(Forward, ToPlayer.GetSafeNormal());
				float Angle = FMath::RadiansToDegrees(acosf(FMath::Clamp(CosTheta, -1.f, 1.f)));

				if (Angle <= SLC->OuterConeAngle)
				{
					float Raw = SLC->Intensity / FMath::Max(Dist * DistFalloffK, 1.f);
					float Val = FMath::Clamp(Raw / LightNorm, 0.f, MaxPointWeight);

					if (IsLightOccluded(PlayerLoc, Actor))
						Val *= OccludedFactor;

					UE_LOG(LogTemp, Warning,
						TEXT("[SPOT] %s | Intens=%.1f Dist=%.1f Angle=%.1f Raw=%.2f Val=%.2f"),
						*Actor->GetName(), SLC->Intensity, Dist, Angle, Raw, Val);

					Sum += Val;
					Count++;
				}
			}
		}
	}

	float Final = (Count > 0 ? Sum / Count : 0.f);

	UE_LOG(LogTemp, Warning,
		TEXT("LightLevel FINAL = %.3f (Count %d)"), Final, Count);

	return Final;
}

//////////////////////////////////////////////////////////////////////////
// Timer Update

void ALightAwarenessDemoCharacter::UpdateLightLevel()
{
	LightLevel = CalculateLightLevel();

	if (bDrawLightDebug)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			0.f,
			FColor::Yellow,
			FString::Printf(TEXT("LightLevel: %.3f"), LightLevel)
		);
	}
}

void ALightAwarenessDemoCharacter::ToggleFlashlight()
{
	bFlashlightOn = !bFlashlightOn;
	Flashlight->SetVisibility(bFlashlightOn, true);
}