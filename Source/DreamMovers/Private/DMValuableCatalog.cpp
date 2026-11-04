#include "DMValuableCatalog.h"

#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace DMValuableCatalog
{
	TArray<FDMValuableSpec> MakeChapterOne()
	{
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		// Glossy master with Roughness / Metallic, made by build_level.py; the engine shape material is the fallback.
		UMaterialInterface* Shape = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Proto/Materials/M_Proto_Shape.M_Proto_Shape"));

		auto Item = [Shape](const TCHAR* Name, UStaticMesh* Mesh, const FVector& Size, float Mass, float Crack, int32 Value,
			const FLinearColor& Tint, float Rough = 0.6f, float Metal = 0.f)
		{
			FDMValuableSpec Spec;
			Spec.Name = FText::FromString(Name);
			Spec.Mesh = Mesh;
			Spec.Material = Shape;
			Spec.Size = Size;
			Spec.MassKg = Mass;
			Spec.CrackSpeed = Crack;
			Spec.Value = Value;
			Spec.Tint = Tint;
			Spec.Roughness = Rough;
			Spec.Metallic = Metal;
			return Spec;
		};

		const FLinearColor Gold(0.95f, 0.75f, 0.3f);
		const FLinearColor Glass(0.7f, 0.85f, 1.f);
		const FLinearColor Red(0.8f, 0.1f, 0.12f);
		const FLinearColor Wood(0.45f, 0.28f, 0.15f);
		const FLinearColor Brass(0.75f, 0.55f, 0.25f);

		return {
			// Small (S): one hand, light, mostly fragile
			Item(TEXT("크리스탈 쿠페 잔"), Sphere, FVector(25.f, 25.f, 25.f), 0.5f, 220.f, 1200, Glass, 0.05f),
			Item(TEXT("도자기 찻잔"), Cube, FVector(22.f, 22.f, 15.f), 0.6f, 260.f, 600, FLinearColor(0.95f, 0.85f, 0.9f)),
			Item(TEXT("토끼 머리 열쇠"), Cube, FVector(10.f, 35.f, 8.f), 0.4f, 600.f, 800, FLinearColor(0.8f, 0.8f, 0.85f)),
			Item(TEXT("회중시계"), Cylinder, FVector(18.f, 18.f, 6.f), 0.5f, 380.f, 700, Brass, 0.25f, 1.f),
			Item(TEXT("향수병"), Cylinder, FVector(14.f, 14.f, 25.f), 0.4f, 200.f, 900, FLinearColor(1.f, 0.6f, 0.75f), 0.05f),
			Item(TEXT("금박 달걀"), Sphere, FVector(20.f, 20.f, 26.f), 0.8f, 350.f, 1500, Gold, 0.2f, 1.f),
			// Medium (M): one player, heavy-ish
			Item(TEXT("황동 오일 램프"), Cylinder, FVector(35.f, 35.f, 70.f), 6.f, 330.f, 1400, Brass, 0.25f, 1.f),
			Item(TEXT("붉은 하트 성물함"), Sphere, FVector(50.f, 45.f, 55.f), 8.f, 300.f, 2500, Red, 0.08f),
			Item(TEXT("유리 랜턴"), Cube, FVector(40.f, 40.f, 70.f), 7.f, 300.f, 1800, Glass, 0.05f),
			Item(TEXT("도자기 광대 인형"), Cube, FVector(35.f, 30.f, 50.f), 5.f, 280.f, 1100, FLinearColor(0.95f, 0.9f, 0.7f)),
			Item(TEXT("별자리 지구본"), Sphere, FVector(55.f, 55.f, 55.f), 9.f, 450.f, 1000, FLinearColor(0.25f, 0.35f, 0.6f)),
			// Large (L): two players
			Item(TEXT("회전목마 말"), Cube, FVector(120.f, 40.f, 90.f), 32.f, 400.f, 3500, FLinearColor(0.95f, 0.9f, 0.95f)),
			Item(TEXT("대형 오르골"), Cube, FVector(100.f, 70.f, 60.f), 35.f, 300.f, 4000, Wood),
			Item(TEXT("축음기"), Cylinder, FVector(80.f, 80.f, 90.f), 30.f, 400.f, 3000, FLinearColor(0.5f, 0.35f, 0.2f)),
			Item(TEXT("금박 액자 그림"), Cube, FVector(10.f, 140.f, 110.f), 25.f, 380.f, 2800, Gold, 0.3f, 1.f),
		};
	}
}
