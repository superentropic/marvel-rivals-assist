#pragma once

namespace Variables
{
	UEngine* Engine = nullptr;
	UWorld* World = nullptr;
	APawn* AcknowledgedPawn = nullptr;
	ULocalPlayer* LocalPlayer = nullptr;
	UGameInstance* GameInstance = nullptr;
	APlayerController* PlayerController = nullptr;
	UGameViewportClient* ViewportClient = nullptr;
	APlayerCameraManager* PlayerCameraManager = nullptr;
	AMarvelBaseCharacter* TargetPlayerPTR = nullptr;
	FVector CameraLocation = FVector();
	FRotator CameraRotation = FRotator();
	float FieldOfView = 0.f;

	FVector2D ScreenSize = FVector2D(
		static_cast<double>(GetSystemMetrics(SM_CXSCREEN)),
		static_cast<double>(GetSystemMetrics(SM_CYSCREEN)));
	FVector2D ScreenCenter = FVector2D(
		static_cast<double>(GetSystemMetrics(SM_CXSCREEN)) / 2.0,
		static_cast<double>(GetSystemMetrics(SM_CYSCREEN)) / 2.0);
}