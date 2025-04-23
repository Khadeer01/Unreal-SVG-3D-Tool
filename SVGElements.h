#pragma once

#include "CoreMinimal.h"
#include "SVGElements.generated.h"

USTRUCT()
struct FSVGElements
{
	GENERATED_BODY()

	UPROPERTY()
	FString ElementType; // e.g., path, rect, circle

	UPROPERTY()
	TArray<float> Parameters; // Parsed numbers from 'd' or other attrs

	UPROPERTY()
	TArray<FVector2D> Vertices; // for geometry

	UPROPERTY()
	TArray<int32> Triangles; // for triangulating the shape into renderable geometry
	

	FSVGElements() = default; // default constructor

	FSVGElements(const FString& InType) : ElementType(InType) {} // constructor to initialize the element type
};
