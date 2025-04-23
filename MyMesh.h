#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "MyMesh.generated.h"

UCLASS()
class PUGINTOOL_API AMyMeshActor : public AActor
{
	GENERATED_BODY()

public:
	AMyMeshActor();

	// Call this to create/update the mesh.
	void CreateMesh(const TArray<FVector>& Vertices, const TArray<int32>& Triangles);



protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere)
	UProceduralMeshComponent* ProcMeshComponent;
};
