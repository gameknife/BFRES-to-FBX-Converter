#include "FBXWriter.h"
#include <iostream>
#include "Primitives.h"


FBXWriter::FBXWriter()
{
}

FBXWriter::~FBXWriter()
{
}

void FBXWriter::CreateFBX(FbxScene* pScene, const BFRESStructs::BFRES& bfres)
{
    WriteModel(pScene, bfres.fmdl[1]);
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::WriteModel(FbxScene* pScene, const BFRESStructs::FMDL& fmdl)
{
    WriteSkeleton(pScene, fmdl.fskl);

    for (uint32 i = 0; i < fmdl.fshps.size(); i++)
    {
        WriteShape(pScene, fmdl.fshps[i]);
    }
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::WriteSkeleton(FbxScene* pScene, const BFRESStructs::FSKL& fskl)
{
    const uint32 uiTotalBones = fskl.bones.size();
    std::vector<FbxNode*> boneNodes(uiTotalBones);
    for (int32 i = 0; i < uiTotalBones; i++)
        CreateBone(pScene, fskl.bones[i], boneNodes[i]);

    for (int32 i = 0; i < uiTotalBones; i++)
    {
        const BFRESStructs::Bone& bone = fskl.bones[i];
        if (bone.parentIndex >= 0 && bone.parentIndex < uiTotalBones)
            boneNodes[bone.parentIndex]->AddChild(boneNodes[i]);
        else
            pScene->GetRootNode()->AddChild(boneNodes[i]);
    }
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::CreateBone(FbxScene* pScene, const BFRESStructs::Bone& bone, FbxNode*& lBoneNode)
{
    // Create a node for our mesh in the scene.
    lBoneNode = FbxNode::Create(pScene, bone.name.c_str());

    // Create a bone.
    FbxSkeleton* lBone = FbxSkeleton::Create(pScene, bone.name.c_str());

    // Set the node attribute of the bone node.
    lBoneNode->SetNodeAttribute(lBone);
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::WriteShape(FbxScene* pScene, const BFRESStructs::FSHP& fshp)
{
    WriteMesh(pScene, fshp, fshp.lodMeshes[0], fshp.vertices);
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::WriteMesh(FbxScene* pScene, const BFRESStructs::FSHP& fshp, const BFRESStructs::LODMesh& lodMesh, const std::vector<BFRESStructs::FVTX>& vertices)
{

    // Create a node for our mesh in the scene.
    FbxNode* lMeshNode = FbxNode::Create(pScene, fshp.name.c_str());

    // Create a mesh.
    FbxMesh* lMesh = FbxMesh::Create(pScene, "");

    // Set the node attribute of the mesh node.
    lMeshNode->SetNodeAttribute(lMesh);

    // Add the mesh node to the root node in the scene.
    FbxNode* lRootNode = pScene->GetRootNode();
    lRootNode->AddChild(lMeshNode);

    // Initialize the control point array of the mesh.
    uint32 uiNumControlPoints(vertices.size());
    lMesh->InitControlPoints(uiNumControlPoints);
    FbxVector4* lControlPoints = lMesh->GetControlPoints();


    // Create a normal layer.
    FbxLayerElementNormal* lLayerElementNormal = FbxLayerElementNormal::Create(lMesh, "");

    // Set its mapping mode to map each normal vector to each control point.
    lLayerElementNormal->SetMappingMode(FbxLayerElement::eByControlPoint);

    // Set the reference mode of so that the n'th element of the normal array maps to the n'th
    // element of the control point array.
    lLayerElementNormal->SetReferenceMode(FbxLayerElement::eDirect);


    FbxVector4 normal(0, 0, 0, 1.0f);
    for (uint32 i = 0; i < uiNumControlPoints; i++)
    {
        // TODO make this iterative
        const Math::vector3F& posVec = vertices[i].position0;
        lControlPoints[i] = FbxVector4(posVec.X, posVec.Y, posVec.Z);

        normal.mData[0] = vertices[i].normal.X;
        normal.mData[1] = vertices[i].normal.Y;
        normal.mData[2] = vertices[i].normal.Z;
        normal.mData[3] = 1.0f;

        lLayerElementNormal->GetDirectArray().Add(normal);
    }
    // Create layer 0 for the mesh if it does not already exist.
    // This is where we will define our normals.
    FbxLayer* lLayer = lMesh->GetLayer(0);
    if (lLayer == NULL) {
        lMesh->CreateLayer();
        lLayer = lMesh->GetLayer(0);
    }

    lLayer->SetNormals(lLayerElementNormal);

    // Define which control points belong to a poly
    uint32 uiPolySize(3);
    // TODO make this iterative
    for (uint32 i = 0; i < lodMesh.faceVertices.size(); ++i)
    {
        // first index
        if ((i % uiPolySize) == 0)
            lMesh->BeginPolygon();

        // TODO make this iterative
        lMesh->AddPolygon(lodMesh.faceVertices[i]);

        // last index
        if ((i + 1) % uiPolySize == 0)
            lMesh->EndPolygon();
    }
}
