#include "MyMesh.h"

AMyMeshActor::AMyMeshActor()
{
	
	ProcMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProcMeshComponent"));
	RootComponent = ProcMeshComponent;
}

void AMyMeshActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void AMyMeshActor::CreateMesh(const TArray<FVector>& Vertices, const TArray<int32>& Triangles)
{
	TArray<FVector> Normals;
	Normals.Init(FVector(0.f, 0.f, 1.f), Vertices.Num());

	ProcMeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, {}, {}, {}, true);
}


