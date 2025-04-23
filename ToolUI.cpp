#include "ToolUI.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "XmlFile.h"
#include "XmlParser.h"
#include "XmlNode.h"
#include "MyMesh.h"
#define _USE_MATH_DEFINES
#include <cmath>

#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "EntitySystem/MovieSceneEntityManager.h"
#include "Runtime/CrashReportCore/Public/Android/AndroidErrorReport.h"
#include "Widgets/Input/SSlider.h"

void ToolUI::Construct(const FArguments& args)
{
    ChildSlot
    [
        SNew(SVerticalBox)

        // File Path and Browse Button Section
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SNew(SHorizontalBox)

            // Editable Text Box for File Path Display
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(5)
            [
                SAssignNew(FilePathTextBox, SEditableTextBox)
                .HintText(FText::FromString("File path will appear here"))
                .IsReadOnly(true) // text box read-only
            ]

            // Browse Button
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5)
            [
                SNew(SButton)
                .Text(FText::FromString("Browse for files"))
                .OnClicked(this, &ToolUI::OnBrowseButtonClicked)
                .ToolTipText(FText::FromString("Only 'rect', 'circle', and 'polygon' elements are supported."))
            ]
        ]
        // convert to text button
        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SNew(SButton)
            .Text(FText::FromString("Convert SVG to Text"))
            .OnClicked(this, &ToolUI::OnConvertSVGButtonClicked)
        ]

        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SAssignNew(ExtractedSVGTextBox, SEditableTextBox)
            .HintText(FText::FromString("Extracted svg text will appear here"))
            .ToolTipText(FText::FromString("Paste or view SVG data here.\nOnly 'rect', 'circle', and 'polygon' elements are supported."))
            
        ]
        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SNew(SButton)
            .Text(FText::FromString("Parse Custom SVG"))
            .OnClicked(this, &ToolUI::OnParseCustomSVGButtonClicked)
            .ToolTipText(FText::FromString("Use this to parse SVG for non file input."))
        ]

        +SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SNew(SButton)
            .Text(FText::FromString("Extract SVG Data"))
            .OnClicked(this, &ToolUI::OnExtractSVGButtonClicked)
            .ToolTipText(FText::FromString("Use this to parse SVG for file input."))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(5)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Extrusion Depth:"))
            ]
            + SHorizontalBox::Slot()
            .FillWidth(0.3f)
            .Padding(5)
            [
                SAssignNew(ExtrusionDepthTextBox, SEditableTextBox)
                .Text(this, &ToolUI::GetExtrusionDepthText)
                .OnTextCommitted(this, &ToolUI::OnExtrusionDepthTextCommitted)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(0.7f)
            .Padding(5)
            [
                SNew(SSlider)
                .Value(this, &ToolUI::GetExtrusionDepthSliderValue)
                .OnValueChanged(this, &ToolUI::OnExtrusionDepthSliderChanged)
                .MinValue(0.0f)
                .MaxValue(500.0f)
            ]
        ]

        // Generate Button Section
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SNew(SButton)
            .Text(FText::FromString("Generate"))
            .OnClicked(this, &ToolUI::OnGenerateButtonClicked)
            .ToolTipText(FText::FromString("Button to generate 3D mesh."))
        ]
    ];
}

FReply ToolUI::OnGenerateButtonClicked()
{
    for (FSVGElements& Elements : ParsedSVGElements)
    {
        if (Elements.ElementType.Equals(TEXT("rect"), ESearchCase::IgnoreCase))
        {
            // Compute the flat vertices (2D) for the rectangle.
            Trinangulation(Elements);

            // Expecting Elements.Vertices to hold your 4 2D corner points.
            // Create vertices for the top face (z = 0) and bottom face (z = -ExtrusionDepth).
            TArray<FVector> Vertices;
            for (const FVector2D& Vec2D : Elements.Vertices)
            {
                // Top face vertex
                Vertices.Add(FVector(Vec2D.X, Vec2D.Y, 0.f));
            }
            for (const FVector2D& Vec2D : Elements.Vertices)
            {
                // Bottom face vertex (offset in negative z)
                Vertices.Add(FVector(Vec2D.X, Vec2D.Y, -ExtrusionDepth));
            }

            // Define triangle indices.
            // Assuming Elements.Vertices originally has 4 points in order:
            // Top face: indices 0,1,2,3; Bottom face: indices 4,5,6,7.

            TArray<int32> Triangles;
            // Top face 
            Triangles.Append({ 0, 2, 1, 0, 3, 2 });
            // Bottom face (reverse order so the normals face the opposite way):
            Triangles.Append({ 4, 5, 6, 4, 6, 7 });

            // Side faces:
            
            Triangles.Append({ 0, 1, 5, 0, 5, 4 }); //side 1, 0-1 edge
            
            Triangles.Append({ 1, 2, 6, 1, 6, 5 });  // Side 2, 1-2 edge
           
            Triangles.Append({ 2, 3, 7, 2, 7, 6 });  // Side 3, 2-3 edge
          
            Triangles.Append({ 3, 0, 4, 3, 4, 7 });   // Side 4, edge 3-4

            FActorSpawnParameters SpawnParameters;
            UWorld* World = GWorld;
            if (!World)
            {
                UE_LOG(LogTemp, Error, TEXT("World not found."));
                return FReply::Handled();
            }
            AMyMeshActor* MeshActor = World->SpawnActor<AMyMeshActor>(AMyMeshActor::StaticClass(), FTransform::Identity, SpawnParameters);
            if (MeshActor)
            {
                MeshActor->CreateMesh(Vertices, Triangles);
            }
        }
        // for a circle
        else if (Elements.ElementType.Equals(TEXT("circle"), ESearchCase::IgnoreCase))
        {
            Trinangulation(Elements); //approcimate circle with 32 segments
           const int32 NumPoints = Elements.Vertices.Num();
            if (NumPoints < 3)
            {
                UE_LOG(LogTemp, Error, TEXT("Not enough vertices for circle"));
                return FReply::Handled();
            }
            
            float Extrusion = ExtrusionDepth;
            float cx = Elements.Parameters[0];              // Generate top/bottom faces and connect with side triangles
                                                            // Central vertex for top/bottom faces improves triangulation
            float cy = Elements.Parameters[1];

            TArray<FVector2D> TopFace;
            TopFace.Add(FVector2d(cx, cy));
            TopFace.Append(Elements.Vertices);

            int32 TopCount = TopFace.Num();
            TArray<FVector> Vertices3D;
            
            for (const FVector2D& Vec2D : TopFace)
            {
                Vertices3D.Add(FVector(Vec2D.X, Vec2D.Y, 0.f));
            }
            
            //bottom face
            const int32 BottomOffset = TopCount;
            for (const FVector2D& Vec2D : TopFace)
            {
                Vertices3D.Add(FVector(Vec2D.X, Vec2D.Y, -Extrusion));
            }
            TArray<int32> Triangles;
           
            for (int32 i = 1; i < TopCount - 1; i++)
            {
                //top
                Triangles.Append({0, i, i+1});
                
            }
            Triangles.Append({ 0, TopCount - 1, 1});
          
            for (int32 i = 1; i < TopCount - 1; i++)
            {
                Triangles.Append({BottomOffset, BottomOffset + i + 1, BottomOffset + i});
            }
            Triangles.Append({BottomOffset, BottomOffset + 1 , BottomOffset + TopCount - 1});

            for (int32 i = 1; i < TopCount; i++)
            {
                int32 nextIndex = (i == TopCount - 1) ? 1 : i + 1;
                int32 TopA = i;
                int32 TopB = nextIndex;
                int32 bottomA = BottomOffset + i;
                int32 bottomB = BottomOffset + nextIndex;

                Triangles.Append({TopA, bottomA, TopB});
                Triangles.Append({TopB, bottomA, bottomB});
            }
            UWorld* World = GWorld;
            if (!World)
            {
                UE_LOG(LogTemp, Error, TEXT("World not found."));
                return FReply::Handled();
            }
            FActorSpawnParameters SpawnParameters;
            AMyMeshActor* MeshActor = World ->SpawnActor<AMyMeshActor>(AMyMeshActor::StaticClass(), FTransform::Identity, SpawnParameters);
            if (MeshActor)
            {
                MeshActor->CreateMesh(Vertices3D, Triangles);
            }
        }
        // for polygons
        else if (Elements.ElementType.Equals(TEXT("polygon"), ESearchCase::IgnoreCase))
        {
            Trinangulation(Elements);
            const int32 NumVertices = Elements.Vertices.Num();
            if (NumVertices < 3)
            {
                UE_LOG(LogTemp, Error, TEXT("Not enough vertices for Polygon"));
                return FReply::Handled();
            }
            TArray<FVector> Vert3D;
            // top fave z = 0
            for (const FVector2d& Vec2D : Elements.Vertices)
            {
                Vert3D.Add(FVector(Vec2D.X, Vec2D.Y, 0.f));
            }

            const int32 BottomOffset = NumVertices;
            for (const FVector2D& Vec2D : Elements.Vertices)
            {
                Vert3D.Add(FVector(Vec2D.X, Vec2D.Y, -ExtrusionDepth));
            }
            TArray<int32> Triangles;
            for (int32 i = 0; i < Elements.Triangles.Num(); i+= 3)
            {
                Triangles.Append({Elements.Triangles[i], Elements.Triangles[i + 1], Elements.Triangles[i + 2]});
            }

            for (int32 i = 0; i < Elements.Triangles.Num(); i+= 3)
            {
                Triangles.Append({BottomOffset + Elements.Triangles[i], BottomOffset + Elements.Triangles[i+2], BottomOffset + Elements.Triangles[i + 1]});
            }
            for (int32 i = 0; i < NumVertices; i++)
            {
                int32 nextIndex = (i + 1) % NumVertices; // Wrap around to the first vertex
                int32 topA = i;
                int32 topB = nextIndex;
                int32 bottomA = BottomOffset + i;
                int32 bottomB = BottomOffset + nextIndex;

                Triangles.Append({ topA, bottomA, topB }); // Side triangle 1
                Triangles.Append({ topB, bottomA, bottomB }); // Side triangle 2
            }
            UWorld* World = GWorld;
            if (!World)
            {
                UE_LOG(LogTemp, Error, TEXT("World not found."));
                return FReply::Handled();
            }
            FActorSpawnParameters SpawnParameters;
            AMyMeshActor* MeshActor = World->SpawnActor<AMyMeshActor>(
                AMyMeshActor::StaticClass(), FTransform::Identity, SpawnParameters);
            if (MeshActor)
            {
                MeshActor->CreateMesh(Vert3D, Triangles);
            }
            
        }
        
    }
    
    UE_LOG(LogTemp, Log, TEXT("Generate Button Clicked, extruded mesh created."));
    return FReply::Handled();
}

FReply ToolUI::OnBrowseButtonClicked()
{
    FString SelectedFile;
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        TArray<FString> UserFiles;
        DesktopPlatform->OpenFileDialog(
            nullptr,
            TEXT("Select an SVG file"),
            FPaths::ProjectContentDir(),
            TEXT(""),
            TEXT("SVG Files (*.svg)|*.svg"),
            EFileDialogFlags::None,
            UserFiles
        );

        if (UserFiles.Num() > 0)
        {
            SelectedFile = UserFiles[0];
            FilePathTextBox->SetText(FText::FromString(SelectedFile)); // Update the text box
            CurrentFilePath = SelectedFile; // Store the selected file path
            UE_LOG(LogTemp, Log, TEXT("File Selected: %s"), *SelectedFile);

            // Load the file content to check its validity
            FString FileContent;
            if (FFileHelper::LoadFileToString(FileContent, *SelectedFile))
            {
                UE_LOG(LogTemp, Log, TEXT("Loaded SVG Content: %s"), *FileContent);

                // Check for unsupported SVG elements
                FXmlFile XmlFile(FileContent, EConstructMethod::ConstructFromBuffer);
                if (!XmlFile.IsValid())
                {
                    FString ErrorMsg = FString::Printf(TEXT("Invalid SVG file: %s"), *XmlFile.GetLastError());
                    UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMsg);
                    FilePathTextBox->SetText(FText::FromString("Invalid SVG file."));
                    return FReply::Handled();
                }

                FXmlNode* RootNode = XmlFile.GetRootNode();
                if (!RootNode)
                {
                    UE_LOG(LogTemp, Error, TEXT("No root node found in the SVG file."));
                    FilePathTextBox->SetText(FText::FromString("No root node found."));
                    return FReply::Handled();
                }

                //  Ignore non-essential nodes such as <title> and <desc>
                bool bHasSupportedElement = false;
                const TArray<FXmlNode*>& ChildNodes = RootNode->GetChildrenNodes();
                for (FXmlNode* Node : ChildNodes)
                {
                    FString NodeTag = Node->GetTag();

                    // Skip non-essential elements.
                    if (NodeTag.Equals(TEXT("title"), ESearchCase::IgnoreCase) ||
                        NodeTag.Equals(TEXT("desc"), ESearchCase::IgnoreCase))
                    {
                        continue;
                    }

                    if (NodeTag.Equals(TEXT("rect"), ESearchCase::IgnoreCase) ||
                        NodeTag.Equals(TEXT("circle"), ESearchCase::IgnoreCase) ||
                        NodeTag.Equals(TEXT("polygon"), ESearchCase::IgnoreCase))
                    {
                        bHasSupportedElement = true;
                        break; // Found a supported element, so the file is valid for our purposes.
                    }
                    else
                    {
                        // Log unsupported elements but continue processing.
                        UE_LOG(LogTemp, Warning, TEXT("Unsupported SVG element encountered (ignored): %s"), *NodeTag);
                    }
                }

                if (!bHasSupportedElement)
                {
                    UE_LOG(LogTemp, Warning, TEXT("No supported SVG elements found in the file."));
                    FilePathTextBox->SetText(FText::FromString("No supported SVG elements found."));
                    return FReply::Handled();
                }

                UE_LOG(LogTemp, Log, TEXT("SVG file contains supported elements."));
                FilePathTextBox->SetText(FText::FromString("SVG file is valid."));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to load the SVG file."));
                FilePathTextBox->SetText(FText::FromString("Failed to load file."));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No file selected."));
            FilePathTextBox->SetText(FText::FromString("No file selected."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Desktop Platform is not available."));
        FilePathTextBox->SetText(FText::FromString("Platform unavailable."));
    }

    return FReply::Handled();
}


FReply ToolUI::OnParseCustomSVGButtonClicked()
{
    FString CustomSVG = ExtractedSVGTextBox->GetText().ToString();
    UE_LOG(LogTemp, Log, TEXT("Custom SVG: %s"), *CustomSVG);
    ProcessSVGData(CustomSVG);
    return FReply::Handled();
}


FReply ToolUI::OnConvertSVGButtonClicked()
{
    if (CurrentFilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("No file selected."));
        return FReply::Handled();
    }

    FString FileContent;
    if (FFileHelper::LoadFileToString(FileContent, *CurrentFilePath))
    {
        UE_LOG(LogTemp, Log, TEXT("File Loaded"));
        ExtractedSVGTextBox->SetText(FText::FromString(FileContent));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("File Not Loaded"));
    }
    
    return FReply::Handled();
}

FReply ToolUI::OnExtractSVGButtonClicked()
{
    if (CurrentFilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("No file selected."));
        return FReply::Handled();
    }

    FString FileContent;
    if (FFileHelper::LoadFileToString(FileContent, *CurrentFilePath))
    {
        UE_LOG(LogTemp, Log, TEXT("File Loaded"));
        ProcessSVGData(FileContent);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("File Not Loaded"));
    }
    return FReply::Handled();
}

void ToolUI::ProcessSVGData(const FString& SVGData)
{
    FXmlFile XmlFile(SVGData, EConstructMethod::ConstructFromBuffer);
    if (!XmlFile.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid XML file: %s, Please input 'circle', 'rect' and/or 'polygon' element svg"), *XmlFile.GetLastError());
        return;
    }

    FXmlNode* RootNode = XmlFile.GetRootNode();
    if (!RootNode)
    {
        UE_LOG(LogTemp, Error, TEXT("No Root Node found"));
        return;
    }

    const TArray<FXmlNode*>& ChildNodes = RootNode->GetChildrenNodes();
    for (FXmlNode* Node : ChildNodes)
    {
        FString NodeTag = Node->GetTag();
        if (NodeTag.Equals(TEXT("rect"), ESearchCase::IgnoreCase) ||
            NodeTag.Equals(TEXT("circle"), ESearchCase::IgnoreCase) ||
            NodeTag.Equals(TEXT("polygon"), ESearchCase::IgnoreCase))
        {
            ProcessSVGNode(Node);
        }
    }
}


void ToolUI::ProcessSVGNode(FXmlNode* Node)
{
    FString NodeTag = Node->GetTag();
    
    if (NodeTag.Equals(TEXT("rect"), ESearchCase::IgnoreCase))
    {
        FString Xaxis = Node->GetAttribute(TEXT("x"));
        FString Yaxis = Node->GetAttribute(TEXT("y"));
        FString Width = Node->GetAttribute(TEXT("width"));
        FString Height = Node->GetAttribute(TEXT("height"));

        float X = FCString::Atof(*Xaxis);
        float Y = FCString::Atof(*Yaxis);
        float WidthNum = FCString::Atof(*Width);
        float HeightNum = FCString::Atof(*Height);

        UE_LOG(LogTemp, Log, TEXT("Rectangle Found: X=%.2f, Y=%.2f, Width=%.2f, Height=%.2f"), X, Y, WidthNum, HeightNum);

        FSVGElements RectElement = FSVGElements(TEXT("rect"));
        RectElement.Parameters.Add(X);
        RectElement.Parameters.Add(Y);
        RectElement.Parameters.Add(WidthNum);
        RectElement.Parameters.Add(HeightNum);

        ParsedSVGElements.Add(RectElement);
    }
    else if (NodeTag.Equals(TEXT("circle"), ESearchCase::IgnoreCase))
    {
        FString CX = Node->GetAttribute(TEXT("cx"));
        FString CY = Node->GetAttribute(TEXT("cy"));
        FString R = Node->GetAttribute(TEXT("r"));

        float cx = FCString::Atof(*CX);
        float cy = FCString::Atof(*CY);
        float radius = FCString::Atof(*R);

        UE_LOG(LogTemp, Log, TEXT("Circle Found: cx=%.2f, cy=%.2f, r=%.2f"), cx, cy, radius);

        FSVGElements CircleElement = FSVGElements(TEXT("circle"));
        CircleElement.Parameters.Add(cx);
        CircleElement.Parameters.Add(cy);   
        CircleElement.Parameters.Add(radius);

        ParsedSVGElements.Add(CircleElement);
    }
    else if (NodeTag.Equals(TEXT("polygon"), ESearchCase::IgnoreCase))
    {
        FString Points = Node->GetAttribute(TEXT("points"));
        TArray<FString> PointPairs;
        Points.ParseIntoArray(PointPairs, TEXT(" "), true);

        FSVGElements PolygonElement = FSVGElements(TEXT("polygon"));
        for (const FString& Pair : PointPairs)
        {
            TArray<FString> Coordinates;
            Pair.ParseIntoArray(Coordinates, TEXT(","), true);
            if (Coordinates.Num() == 2)
            {
                float x = FCString::Atof(*Coordinates[0]);
                float y = FCString::Atof(*Coordinates[1]);
                PolygonElement.Vertices.Add(FVector2D(x, y));
            }
        }

        UE_LOG(LogTemp, Log, TEXT("Polygon Found with %d vertices"), PolygonElement.Vertices.Num());
        ParsedSVGElements.Add(PolygonElement);
    }
}

void ToolUI::Trinangulation(FSVGElements& Elements)
{
    if (Elements.ElementType.Equals(TEXT("rect"), ESearchCase::IgnoreCase))
    {
        if (Elements.Parameters.Num() < 4)
        {
            UE_LOG(LogTemp, Error, TEXT("Not enough parameters for rectangle triangulation"));
            return;
        }

        float xRect = Elements.Parameters[0];
        float yRect = Elements.Parameters[1];
        float width = Elements.Parameters[2];
        float height = Elements.Parameters[3];

        // Calculate the four corners of the rectangle.
        FVector2D Point0(xRect, yRect);
        FVector2D Point1(xRect + width, yRect);
        FVector2D Point2(xRect + width, yRect + height);
        FVector2D Point3(xRect, yRect + height);

        Elements.Vertices.Empty();
        Elements.Vertices.Add(Point0);
        Elements.Vertices.Add(Point1);
        Elements.Vertices.Add(Point2);
        Elements.Vertices.Add(Point3);

        UE_LOG(LogTemp, Log, TEXT("Triangulated Rectangle Vertices:"));
        UE_LOG(LogTemp, Log, TEXT("Point0: %s"), *Point0.ToString());
        UE_LOG(LogTemp, Log, TEXT("Point1: %s"), *Point1.ToString());
        UE_LOG(LogTemp, Log, TEXT("Point2: %s"), *Point2.ToString());
        UE_LOG(LogTemp, Log, TEXT("Point3: %s"), *Point3.ToString());
    }
    // for a circle
    else if (Elements.ElementType.Equals(TEXT("circle"), ESearchCase::IgnoreCase))
    {
        if (Elements.Parameters.Num() < 3)
        {
            UE_LOG(LogTemp, Error, TEXT("Not enough parameters for circle triangulation"));
            return;
        }

        // Extract circle parameters.
        float cx = Elements.Parameters[0];
        float cy = Elements.Parameters[1];
        float radius = Elements.Parameters[2];

        const int32 Segments = 32; // Number of segments for approximating the circle.
        Elements.Vertices.Empty();

        // Generate the perimeter vertices for the circle.
        // Same structure as rectangle: "compute, add to Vertices, then log"
        for (int32 i = 0; i < Segments; i++)
        {
            float theta = 2.0f * PI * i / Segments;
            float px = radius * FMath::Cos(theta);
            float py = radius * FMath::Sin(theta);
            FVector2D Vertex(cx + px, cy + py);
            Elements.Vertices.Add(Vertex);
        }

        UE_LOG(LogTemp, Log, TEXT("Triangulated Circle Vertices:"));
        for (int32 i = 0; i < Elements.Vertices.Num(); i++)
        {
            UE_LOG(LogTemp, Log, TEXT("Point%d: %s"), i, *Elements.Vertices[i].ToString());
        }
    }
    // for polygons
    else if (Elements.ElementType.Equals(TEXT("polygon"), ESearchCase::IgnoreCase))
    {
        const int32 NumVertices = Elements.Vertices.Num();
        if (NumVertices < 3)
        {
            UE_LOG(LogTemp, Error, TEXT("Polygon must have at least 3 vertices"));
            return;
        }
        // Assuming the vertices are ordered counterclockwise.
        Elements.Triangles.Empty();
        for (int32 i = 1; i < NumVertices - 1; i++)
        {
            Elements.Triangles.Append({ 0, i, i + 1 }); // Create triangles relative to the first vertex
        }

        UE_LOG(LogTemp, Log, TEXT("Triangulated Polygon with %d triangles"), Elements.Triangles.Num() / 3);
    }
}

FText ToolUI::GetExtrusionDepthText() const
{
    return FText::AsNumber(ExtrusionDepth);
}

void ToolUI::OnExtrusionDepthTextCommitted(const FText& InText, ETextCommit::Type CommitInfo)
{
    FString Text = InText.ToString();
    if (Text.IsNumeric())
    {
        ExtrusionDepth = FCString::Atof(*Text);
        // Clamp value to slider range
        ExtrusionDepth = FMath::Clamp(ExtrusionDepth, 0.0f, 250.0f);
    }
}

float ToolUI::GetExtrusionDepthSliderValue() const
{
    return ExtrusionDepth;
}

void ToolUI::OnExtrusionDepthSliderChanged(float NewValue)
{
    ExtrusionDepth = NewValue;
    // Update text box to reflect slider value
    ExtrusionDepthTextBox->SetText(FText::AsNumber(ExtrusionDepth));
}
















