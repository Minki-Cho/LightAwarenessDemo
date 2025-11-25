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

ALightAwarenessDemoCharacter::ALightAwarenessDemoCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(FollowCamera);
	Flashlight->Intensity = 20000.f;
	Flashlight->AttenuationRadius = 1000.f;
	Flashlight->InnerConeAngle = 20.f;
	Flashlight->OuterConeAngle = 35.f;
	Flashlight->SetVisibility(false, true);
}

void ALightAwarenessDemoCharacter::BeginPlay()
{
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

void ALightAwarenessDemoCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	

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

	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{

		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);


		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	

		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);


		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ALightAwarenessDemoCharacter::Look(const FInputActionValue& Value)
{

	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{

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


	return (Hit.bBlockingHit && Hit.GetActor() != LightActor);
}

float ALightAwarenessDemoCharacter::CalculateLightLevel()
{
	FVector P = GetActorLocation();

	if (!Config) return 0.f;

	float Sum = 0.f;
	int32 Count = 0;

	auto Accum = [&](const TCHAR* Label, AActor* LightActor, float Val, float Dist)
		{
			if (Val <= 0.f) return;
			Sum += Val;
			Count++;

			if (bDrawLightDebug && GEngine)
			{
				FString Msg = FString::Printf(
					TEXT("%s: %s  Dist=%.0f  Val=%.2f"),
					Label,
					*LightActor->GetName(),
					Dist,
					Val
				);
				GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Cyan, Msg);
			}
		};

	for (TActorIterator<APointLight> It(GetWorld()); It; ++It)
	{
		APointLight* L = *It;
		UPointLightComponent* LC = L->PointLightComponent;
		if (!LC) continue;

		float Dist = FVector::Distance(P, L->GetActorLocation());
		if (Dist > LC->AttenuationRadius) continue;

		float Raw = LC->Intensity / FMath::Max(Dist * Config->DistFalloffK, 1.f);
		float Val = FMath::Clamp(Raw / Config->LightNorm, 0.f, Config->MaxPointWeight);

		if (IsLightOccluded(P, L))
			Val *= Config->OccludedFactor;

		Accum(TEXT("Point"), L, Val, Dist);
	}

	for (TActorIterator<ASpotLight> It(GetWorld()); It; ++It)
	{
		ASpotLight* L = *It;
		USpotLightComponent* LC = L->SpotLightComponent;
		if (!LC) continue;

		FVector LPos = L->GetActorLocation();
		FVector ToP = P - LPos;
		float Dist = ToP.Length();
		if (Dist > LC->AttenuationRadius) continue;

		FVector Fwd = L->GetActorForwardVector();
		float CosTheta = FVector::DotProduct(Fwd, ToP.GetSafeNormal());
		float Angle = FMath::RadiansToDegrees(
			acosf(FMath::Clamp(CosTheta, -1.f, 1.f))
		);
		if (Angle > LC->OuterConeAngle) continue;

		auto Smooth = [](float A, float B, float T)
			{
				float X = FMath::Clamp((T - A) / (B - A), 0.f, 1.f);
				return X * X * (3.f - 2.f * X);
			};

		float AngFactor =
			1.f - Smooth(LC->InnerConeAngle * (1.f - Config->SpotSmooth),
				LC->OuterConeAngle,
				Angle);

		float Raw = (LC->Intensity * AngFactor) /
			FMath::Max(Dist * Config->DistFalloffK, 1.f);

		float Val = FMath::Clamp(Raw / Config->LightNorm, 0.f, Config->MaxPointWeight);

		if (IsLightOccluded(P, L))
			Val *= Config->OccludedFactor;

		Accum(TEXT("Spot"), L, Val, Dist);
	}

	if (Flashlight && Flashlight->IsVisible())
	{
		FVector LPos = Flashlight->GetComponentLocation();
		FVector ToP = P - LPos;
		float Dist = ToP.Length();

		if (Dist <= Flashlight->AttenuationRadius)
		{

			FVector Fwd = Flashlight->GetForwardVector();
			float CosTheta = FVector::DotProduct(Fwd, ToP.GetSafeNormal());
			float Angle = FMath::RadiansToDegrees(acosf(FMath::Clamp(CosTheta, -1.f, 1.f)));
			if (Angle <= Flashlight->OuterConeAngle)
			{
				// Smooth angle falloff
				auto Smooth = [](float A, float B, float T)
					{
						float X = FMath::Clamp((T - A) / (B - A), 0.f, 1.f);
						return X * X * (3.f - 2.f * X);
					};

				float AngFactor =
					1.f - Smooth(
						Flashlight->InnerConeAngle * (1.f - Config->SpotSmooth),
						Flashlight->OuterConeAngle,
						Angle
					);

				float Raw = (Flashlight->Intensity * AngFactor) /
					FMath::Max(Dist * Config->DistFalloffK, 1.f);

				float Val = FMath::Clamp(
					Raw / Config->LightNorm,
					0.f,
					Config->MaxPointWeight
				);

				Sum += Val;
				Count++;

				if (bDrawLightDebug)
				{
					GEngine->AddOnScreenDebugMessage(
						-1, 0.f, FColor::Blue,
						FString::Printf(TEXT("[Flashlight] Dist=%.0f  Val=%.2f"), Dist, Val)
					);
				}
			}
		}
	}

	float Avg = (Count > 0 ? Sum / Count : 0.f);
	return FMath::Clamp(Avg, 0.f, 1.f);
}


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