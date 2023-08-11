#include "FBXWriter.h"
#include <iostream>
#include "ConsoleColor.h"
#include "Primitives.h"
#include "assert.h"
#include "Globals.h"

FBXWriter::FBXWriter()
{
}

FBXWriter::~FBXWriter()
{
}

bool FBXWriter::g_bWriteTextures = false;
std::map<std::string, FbxSurfacePhong*> FBXWriter::g_MaterialMap;
std::map<std::string, FbxFileTexture*> FBXWriter::g_TextureMap;
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::CreateFBX(FbxScene*& pScene, const BFRES& bfres)
{
    for (uint32 i = 0; i < bfres.fmdl.size(); i++)
    {
        WriteModel(pScene, bfres.fmdl[i], i, false);
    }

    for (const Anim& anim : bfres.fska.anims)
    {
        WriteAnimations(pScene, anim);
    }
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::WriteAnimations(FbxScene*& pScene, const Anim& anim)
{
    // One AnimStack per animation
    FbxAnimStack* pAnimStack = FbxAnimStack::Create(pScene, anim.m_szName.c_str());

    FbxString tempString = "Anim Stack: ";
    tempString += anim.m_szName.c_str();
    pAnimStack->Description = tempString;

    FbxTime fbxTime;
    fbxTime.SetFrame(anim.m_cFrames, FbxTime::eFrames30);
    pAnimStack->LocalStop.Set(fbxTime);

    // One AnimLayer per AnimStack
    tempString = "Anim Layer: ";
    tempString += anim.m_szName.c_str();
    FbxAnimLayer* pAnimLayer = FbxAnimLayer::Create(pScene, tempString);
    pAnimStack->AddMember(pAnimLayer);

    for (const BoneAnim& boneAnim : anim.m_vBoneAnims)
    {
        // Get bone that matches boneAnim name
        FbxNode* pBone = pScene->FindNodeByName(boneAnim.m_szName.c_str());
        assert(pBone);
        if (pBone)
        {
            // Curve Node for Bone Translation
            CreateTranslationAnimCurveNode(pAnimLayer, pBone, boneAnim);

            // Curve Node for Bone Rotation
            CreateRotationAnimCurveNode(pAnimLayer, pBone, boneAnim);

            // Curve Node for Bone Scale
            CreateScaleAnimCurveNode(pAnimLayer, pBone, boneAnim);
        }
    }
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::CreateScaleAnimCurveNode(FbxAnimLayer*& pAnimLayer, FbxNode*& pBone, const BoneAnim& boneAnim)
{
    // Add keyframes to X Channel
    FbxAnimCurve* pXAnimCurve = pBone->LclScaling.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
    AddKeyFramesToAnimCurve(pXAnimCurve, boneAnim.m_XSCA, AnimTrackType::eScale);

    // Add keyframes to Y Channel
    FbxAnimCurve* pYAnimCurve = pBone->LclScaling.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
    AddKeyFramesToAnimCurve(pYAnimCurve, boneAnim.m_YSCA, AnimTrackType::eScale);

    // Add keyframes to Z Channel
    FbxAnimCurve* pZAnimCurve = pBone->LclScaling.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
    AddKeyFramesToAnimCurve(pZAnimCurve, boneAnim.m_ZSCA, AnimTrackType::eScale);
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::CreateRotationAnimCurveNode(FbxAnimLayer*& pAnimLayer, FbxNode*& pBone, const BoneAnim& boneAnim)
{
    assert(boneAnim.m_eRotType == BoneAnim::AnimRotationType::EULER);

    // Add keyframes to X Channel
    FbxAnimCurve* pXAnimCurve = pBone->LclRotation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
    AddKeyFramesToAnimCurve(pXAnimCurve, boneAnim.m_XROT, AnimTrackType::eRotation);

    // Add keyframes to Y Channel
    FbxAnimCurve* pYAnimCurve = pBone->LclRotation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
    AddKeyFramesToAnimCurve(pYAnimCurve, boneAnim.m_YROT, AnimTrackType::eRotation);

    // Add keyframes to Z Channel
    FbxAnimCurve* pZAnimCurve = pBone->LclRotation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
    AddKeyFramesToAnimCurve(pZAnimCurve, boneAnim.m_ZROT, AnimTrackType::eRotation);
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::CreateTranslationAnimCurveNode(FbxAnimLayer*& pAnimLayer, FbxNode*& pBone, const BoneAnim& boneAnim)
{
    // Add keyframes to X Channel
    FbxAnimCurve* pXAnimCurve = pBone->LclTranslation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
    AddKeyFramesToAnimCurve(pXAnimCurve, boneAnim.m_XPOS, AnimTrackType::eTranslation);

    // Add keyframes to Y Channel
    FbxAnimCurve* pYAnimCurve = pBone->LclTranslation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
    AddKeyFramesToAnimCurve(pYAnimCurve, boneAnim.m_YPOS, AnimTrackType::eTranslation);

    // Add keyframes to Z Channel
    FbxAnimCurve* pZAnimCurve = pBone->LclTranslation.GetCurve(pAnimLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);
    AddKeyFramesToAnimCurve(pZAnimCurve, boneAnim.m_ZPOS, AnimTrackType::eTranslation);
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::AddKeyFramesToAnimCurve(FbxAnimCurve*& pAnimCurve, const AnimTrack& animTrack, AnimTrackType animTrackType)
{
    if (pAnimCurve && (animTrack.m_cKeys > 0))
    {
        assert(animTrack.m_eInterpolationType == AnimTrack::CurveInterpolationType::HERMITE);
        pAnimCurve->KeyModifyBegin();
        for (uint32 i = 0; i < animTrack.m_cKeys; ++i)
        {
            const KeyFrame& keyFrame = animTrack.m_vKeyFrames[i];
            FbxTime fbxTime;
            fbxTime.SetFrame(keyFrame.m_uiFrame, FbxTime::eFrames30);
            uint32 uiKeyIndex = pAnimCurve->KeyAdd(fbxTime);

            float fValue = keyFrame.m_fValue;
            if (animTrackType == AnimTrackType::eRotation)
                fValue = (float)Math::ConvertRadiansToDegrees(fValue);

            pAnimCurve->KeySet(uiKeyIndex, fbxTime, fValue, FbxAnimCurveDef::eInterpolationCubic, FbxAnimCurveDef::eTangentAuto, keyFrame.m_fSlope1, keyFrame.m_fSlope2);
        }
        pAnimCurve->KeyModifyEnd();
    }
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::WriteModel(FbxScene*& pScene, const FMDL& fmdl, uint32 fmdlIndex, bool onlySkeleton)
{
    // Create an array to store the smooth and rigid bone indices
    std::vector<BoneMetadata> boneInfoList(fmdl.fskl.boneList.size());

    WriteSkeleton(pScene, fmdl.fskl, boneInfoList);

    if( !onlySkeleton )
    {
        for (uint32 i = 0; i < fmdl.fshps.size(); i++)
        {
            WriteShape(pScene, fmdl.fshps[i], boneInfoList, fmdlIndex);
        }
    }
}

bool g_RootBoneCreated = false;
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::WriteSkeleton(FbxScene*& pScene, const FSKL& fskl, std::vector<BoneMetadata>& boneInfoList)
{
    // two root bone, ue cannot handle
    if(fskl.bones.size() == 1 && !fskl.bones[0].useRigidMatrix && !fskl.bones[0].useSmoothMatrix)
    {
        return;
    }

    // gameknife mod, ue only support single root bone, so skip next root bone temp
    // if(g_RootBoneCreated)
    // {
    //     boneInfoList.clear();
    //     return;
    // }
    
    const uint32 uiTotalBones = fskl.bones.size();
    std::vector<FbxNode*> boneNodes(uiTotalBones);

    for (uint32 i = 0; i < uiTotalBones; i++)
        CreateBone(pScene, fskl.bones[i], boneNodes[i], boneInfoList);

    for (uint32 i = 0; i < uiTotalBones; i++)
    {
        const Bone& bone = fskl.bones[i];
        if (bone.parentIndex >= 0)
            boneNodes[bone.parentIndex]->AddChild(boneNodes[i]);
        else
        {
            g_RootBoneCreated = true;
            pScene->GetRootNode()->AddChild(boneNodes[i]);
        }
    }
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::CreateBone(FbxScene*& pScene, const Bone& bone, FbxNode*& lBoneNode, std::vector<BoneMetadata>& boneInfoList)
{
    // Create a node for our mesh in the scene.
    lBoneNode = FbxNode::Create(pScene, bone.name.c_str());

    // Set transform data
    FbxDouble3 fBoneScale = FbxDouble3(bone.scale.X, bone.scale.Y, bone.scale.Z);
    lBoneNode->LclScaling.Set(fBoneScale);
    if (bone.rotationType == Bone::RotationType::EulerXYZ)
    {
        FbxDouble3 fBoneRot = FbxDouble3(
            Math::ConvertRadiansToDegrees(bone.rotation.X),
            Math::ConvertRadiansToDegrees(bone.rotation.Y),
            Math::ConvertRadiansToDegrees(bone.rotation.Z));
        lBoneNode->LclRotation.Set(fBoneRot);
    }
    else
    {
        // This FbxWriter doesn't support bone quaternion rotation yet
        assert(0 && "FbxWriter doesn't support bone quaternion rotation yet");
    }
    FbxDouble3 fBonePos = FbxDouble3(bone.position.X, bone.position.Y, bone.position.Z);
    lBoneNode->LclTranslation.Set(fBonePos);

    // Create a bone.
    FbxSkeleton* lBone = FbxSkeleton::Create(pScene, bone.name.c_str());
    if (bone.parentIndex == -1)
    {
        lBone->SetSkeletonType(FbxSkeleton::eRoot);
    }
    else
    {
        lBone->SetSkeletonType(FbxSkeleton::eLimbNode);
    }

    lBone->Size.Set(0.03);
    // Set the node attribute of the bone node.
    lBoneNode->SetNodeAttribute(lBone);

    // Add bone data to the bone info list
    if (bone.useSmoothMatrix)
    {
        boneInfoList[bone.smoothMatrixIndex].uiBoneIndex = bone.index;
        boneInfoList[bone.smoothMatrixIndex].szName = bone.name;
        boneInfoList[bone.smoothMatrixIndex].eSkinningType = SkinningType::eSmooth;
    }
    if (bone.useRigidMatrix)
    {
        boneInfoList[bone.rigidMatrixIndex].uiBoneIndex = bone.index;
        boneInfoList[bone.rigidMatrixIndex].szName = bone.name;
        boneInfoList[bone.rigidMatrixIndex].eSkinningType = SkinningType::eRigid;
    }
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::WriteShape(FbxScene*& pScene, const FSHP& fshp, std::vector<BoneMetadata>& boneListInfos, uint32 fmdlIndex)
{
    std::string meshName = fshp.name + "_LODGroup";
    FbxNode* lLodGroup = FbxNode::Create(pScene, meshName.c_str());
    FbxLODGroup* lLodGroupAttr = FbxLODGroup::Create(pScene, meshName.c_str());
    // Array lChildNodes contains geometries of all LOD levels

    // create the single material
    FMAT* fmat = g_BFRESManager.GetMaterialByIndex(fshp.modelIndex, fshp.materialIndex);

    std::string matName = fmat->name;

    // add or get from materialmap
    if (g_MaterialMap.find(matName) == g_MaterialMap.end())
    {
        FbxString lMaterialName = fmat->name.c_str();
        FbxSurfacePhong* lMaterial = FbxSurfacePhong::Create(pScene, lMaterialName);

        if (g_bWriteTextures)
        {
            // Get Material used for this mesh
            SetTexturesToMaterial(pScene, fmat, lMaterial);
        }
        g_MaterialMap[matName] = lMaterial;
    }

    FbxSurfacePhong* lMaterial = g_MaterialMap[matName];

    for (int j = 0; j < fshp.lodMeshes.size(); j++)
    {
        WriteMesh(lMaterial, pScene, lLodGroup, fshp, fshp.lodMeshes[j], boneListInfos, fmdlIndex);
        //lLodGroupAttr->AddDisplayLevel( FbxLODGroup::EDisplayLevel::eUseLOD );
        //lLodGroupAttr->AddThreshold( 500 * j );
    }
    lLodGroup->SetNodeAttribute(lLodGroupAttr);
    pScene->GetRootNode()->AddChild(lLodGroup);
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::WriteMesh(FbxSurfacePhong* lMaterial, FbxScene*& pScene, FbxNode*& pLodGroup, const FSHP& fshp, const LODMesh& lodMesh, std::vector<BoneMetadata>& boneListInfos, uint32 fmdlIndex)
{
    bool hasSkeleton = boneListInfos.size() > 0;

    uint32 uiLODIndex = pLodGroup->GetChildCount();
    std::string meshName = fshp.name;
    meshName += "_LOD" + std::to_string(uiLODIndex);

    // Create a node for our mesh in the scene.
    FbxNode* lMeshNode = FbxNode::Create(pScene, meshName.c_str());

    // Create a mesh.
    FbxMesh* lMesh = FbxMesh::Create(pScene, meshName.c_str());

    // Set the node attribute of the mesh node.
    lMeshNode->SetNodeAttribute(lMesh);

    // Add the mesh node to the root node in the scene.
    pLodGroup->AddChild(lMeshNode);

    // Initialize the control point array of the mesh.
    uint32 uiNumControlPoints(fshp.vertices.size());
    lMesh->InitControlPoints(uiNumControlPoints);
    FbxVector4* lControlPoints = lMesh->GetControlPoints();

    //Create layer elements and set mapping modes and reference modes
    FbxLayerElementNormal* lLayerElementNormal = FbxLayerElementNormal::Create(lMesh, "_n0");
    FbxLayerElementUV* lLayerElementUV0 = FbxLayerElementUV::Create(lMesh, "UVChannel_1");
    FbxLayerElementUV* lLayerElementUV1 = FbxLayerElementUV::Create(lMesh, "UVChannel_2");
    FbxLayerElementUV* lLayerElementUV2 = FbxLayerElementUV::Create(lMesh, "UVChannel_3");
    FbxLayerElementTangent* lLayerElementTangent = FbxLayerElementTangent::Create(lMesh, "_t0");
    FbxLayerElementBinormal* lLayerElementBinormal = FbxLayerElementBinormal::Create(lMesh, "_b0");
    FbxLayerElementVertexColor* lLayerElementCol0 = FbxLayerElementVertexColor::Create(lMesh, "_c0");

    lLayerElementNormal->SetMappingMode(FbxLayerElement::eByControlPoint);
    lLayerElementUV0->SetMappingMode(FbxLayerElement::eByControlPoint);
    lLayerElementUV1->SetMappingMode(FbxLayerElement::eByControlPoint);
    lLayerElementUV2->SetMappingMode(FbxLayerElement::eByControlPoint);
    lLayerElementTangent->SetMappingMode(FbxLayerElement::eByControlPoint);
    lLayerElementBinormal->SetMappingMode(FbxLayerElement::eByControlPoint);
    lLayerElementCol0->SetMappingMode(FbxLayerElement::eByControlPoint);

    lLayerElementNormal->SetReferenceMode(FbxLayerElement::eDirect);
    lLayerElementUV0->SetReferenceMode(FbxLayerElement::eDirect);
    lLayerElementUV1->SetReferenceMode(FbxLayerElement::eDirect);
    lLayerElementUV2->SetReferenceMode(FbxLayerElement::eDirect);
    lLayerElementTangent->SetReferenceMode(FbxLayerElement::eDirect);
    lLayerElementBinormal->SetReferenceMode(FbxLayerElement::eDirect);
    lLayerElementCol0->SetReferenceMode(FbxLayerElement::eDirect);

    std::map<uint32, SkinCluster> SkinClusterMap;

    uint32 face = uiNumControlPoints / 3;
    for (uint32 i = 0; i < uiNumControlPoints; i++)
    {
        const Math::vector3F& posVec = fshp.vertices[i].position0;
        lControlPoints[i] = FbxVector4(posVec.X, posVec.Y, posVec.Z);

        const Math::vector3F& normalVec = fshp.vertices[i].normal;
        lLayerElementNormal->GetDirectArray().Add(FbxVector4(normalVec.X, normalVec.Y, normalVec.Z));

#if FLIP_UV_VERTICAL
        const Math::vector2F& uv0Vec = fshp.vertices[i].uv0;
        lLayerElementUV0->GetDirectArray().Add(FbxVector2(uv0Vec.X, 1 - uv0Vec.Y));
        const Math::vector2F& uv1Vec = fshp.vertices[i].uv1;
        lLayerElementUV1->GetDirectArray().Add(FbxVector2(uv1Vec.X, 1 - uv1Vec.Y));
        const Math::vector2F& uv2Vec = fshp.vertices[i].uv2;
        lLayerElementUV2->GetDirectArray().Add(FbxVector2(uv2Vec.X, 1 - uv2Vec.Y));
#else
        const Math::vector2F& uv0Vec = fshp.vertices[ i ].uv0;
        lLayerElementUV0->GetDirectArray().Add( FbxVector2( uv0Vec.X, uv0Vec.Y ) );
        const Math::vector2F& uv1Vec = fshp.vertices[ i ].uv1;
        lLayerElementUV1->GetDirectArray().Add( FbxVector2( uv1Vec.X, uv1Vec.Y ) );
        const Math::vector2F& uv2Vec = fshp.vertices[ i ].uv2;
        lLayerElementUV2->GetDirectArray().Add( FbxVector2( uv2Vec.X, uv2Vec.Y ) );
#endif

        const Math::vector4F& tangentVec = fshp.vertices[i].tangent;
        lLayerElementTangent->GetDirectArray().Add(FbxVector4(tangentVec.X, tangentVec.Y, tangentVec.Z, tangentVec.W));

        const Math::vector4F& binormalVec = fshp.vertices[i].binormal;
        lLayerElementBinormal->GetDirectArray().Add(FbxVector4(binormalVec.X, binormalVec.Y, binormalVec.Z, binormalVec.W));

        const Math::vector4F& col0Vec = fshp.vertices[i].color0;
        lLayerElementCol0->GetDirectArray().Add(FbxVector4(col0Vec.X, col0Vec.Y, col0Vec.Z, col0Vec.W));

        if (hasSkeleton)
            CreateSkinClusterData(fshp.vertices[i], i, SkinClusterMap, boneListInfos, fshp); // Convert the vertex-to-bone mapping to bone-to-vertex so it conforms with fbx cluster data
    }

    if (hasSkeleton)
        WriteSkin(pScene, lMesh, SkinClusterMap, fmdlIndex);

    // Create layer 0 for the mesh if it does not already exist.
    // This is where we will define our normals.
    FbxLayer* lLayer00 = lMesh->GetLayer(0);
    if (lLayer00 == NULL)
    {
        lMesh->CreateLayer();
        lLayer00 = lMesh->GetLayer(0);
    }
    lLayer00->SetNormals(lLayerElementNormal);
    lLayer00->SetUVs(lLayerElementUV0, FbxLayerElement::eTextureDiffuse);
    lLayer00->SetUVs(lLayerElementUV1, FbxLayerElement::eTextureNormalMap);
    lLayer00->SetUVs(lLayerElementUV2, FbxLayerElement::eTextureTransparency); // We dont know what this is used for
    lLayer00->SetTangents(lLayerElementTangent);
    lLayer00->SetBinormals(lLayerElementBinormal);
    lLayer00->SetVertexColors(lLayerElementCol0);

    MapFacesToVertices(lodMesh, lMesh);

    // TODO set materials to an LOD group, not the mesh - fix the hack
    // Set material mapping.
    FbxGeometryElementMaterial* lMaterialElement = lMesh->CreateElementMaterial();
    lMaterialElement->SetMappingMode(FbxGeometryElement::eByPolygon);
    lMaterialElement->SetReferenceMode(FbxGeometryElement::eDirect);

    FbxLayerElementSmoothing* lLayerElementSmoothing = FbxLayerElementSmoothing::Create(lMesh, "Smoothing");

    lLayerElementSmoothing->SetMappingMode(FbxLayerElement::eByPolygon);
    lLayerElementSmoothing->SetReferenceMode(FbxLayerElement::eDirect);    // must be direct
    
    lLayer00->SetSmoothing(lLayerElementSmoothing);
    
    lMeshNode->AddMaterial(lMaterial);
    lMeshNode->SetShadingMode(FbxNode::eTextureShading);

    // TODO move this function call into write animations
    WriteBindPose(pScene, lMeshNode);
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// Currently as far as it will get. Certain things, like AO maps, are not
// supported by FBX's Phong material as far as I can tell.
void FBXWriter::SetTexturesToMaterial(FbxScene*& pScene, FMAT* fmat, FbxSurfacePhong* lMaterial)
{
    for (uint32 i = 0; i < fmat->textureRefs.textureCount; i++)
    {
        TextureRef& tex = fmat->textureRefs.textures[i];
        GX2TextureMapType type = tex.type;
        FbxTexture::ETextureUse textureUse;
        FbxString uvLayerName;
        FbxTexture::EWrapMode wrapModeX;
        FbxTexture::EWrapMode wrapModeY;

        std::string& textureName = g_BFRESManager.GetTextureFromMaterialByType(fmat, type)->name;

        // add or get texture from texturemap
        if (g_TextureMap.find(textureName) == g_TextureMap.end())
        {
            FbxFileTexture* lTexture = FbxFileTexture::Create(pScene, textureName.c_str());
            switch (tex.clampX)
            {
            case BFRESStructs::TextureRef::GX2TexClamp::Wrap:
                wrapModeX = FbxTexture::eRepeat;
                break;
            case BFRESStructs::TextureRef::GX2TexClamp::Mirror:
                break;
            case BFRESStructs::TextureRef::GX2TexClamp::Clamp:
                wrapModeX = FbxTexture::eClamp;
                break;
            case BFRESStructs::TextureRef::GX2TexClamp::MirrorOnce:
                break;
            case BFRESStructs::TextureRef::GX2TexClamp::ClampHalfBorder:
                break;
            case BFRESStructs::TextureRef::GX2TexClamp::MirrorOnceHalfBorder:
                break;
            case BFRESStructs::TextureRef::GX2TexClamp::ClampBorder:
                break;
            case BFRESStructs::TextureRef::GX2TexClamp::MirrorOnceBorder:
                break;
            default:
                break;
            }

            switch (tex.clampY)
            {
            case BFRESStructs::TextureRef::GX2TexClamp::Wrap:
                wrapModeY = FbxTexture::eRepeat;
                break;
            case BFRESStructs::TextureRef::GX2TexClamp::Mirror:
                break;
            case BFRESStructs::TextureRef::GX2TexClamp::Clamp:
                wrapModeY = FbxTexture::eClamp;
                break;
            case BFRESStructs::TextureRef::GX2TexClamp::MirrorOnce:
                break;
            case BFRESStructs::TextureRef::GX2TexClamp::ClampHalfBorder:
                break;
            case BFRESStructs::TextureRef::GX2TexClamp::MirrorOnceHalfBorder:
                break;
            case BFRESStructs::TextureRef::GX2TexClamp::ClampBorder:
                break;
            case BFRESStructs::TextureRef::GX2TexClamp::MirrorOnceBorder:
                break;
            default:
                break;
            }

            std::string filePath = (fbxExportPath + (std::string)"Textures/" + textureName + ".tga");
            lTexture->SetFileName(filePath.c_str());
            lTexture->SetMappingType(FbxTexture::eUV);
            lTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
            lTexture->SetSwapUV(false);
            lTexture->SetTranslation(0.0, 0.0);
            lTexture->SetScale(1.0, 1.0);
            lTexture->SetRotation(0.0, 0.0);
            lTexture->UVSet.Set(uvLayerName); // Connect texture to the proper UV
            lTexture->SetWrapMode(wrapModeX, wrapModeY);

            g_TextureMap[textureName] = lTexture;
        }

        FbxFileTexture* lTexture = g_TextureMap[textureName];

        switch (type)
        {
        case GX2TextureMapType::Albedo:
            lMaterial->Diffuse.ConnectSrcObject(lTexture);
            break;
        case GX2TextureMapType::Normal:
            lMaterial->NormalMap.ConnectSrcObject(lTexture);
            break;
        case GX2TextureMapType::Specular:
            lMaterial->Specular.ConnectSrcObject(lTexture);
            break;
        case GX2TextureMapType::AmbientOcclusion:
            lMaterial->Ambient.ConnectSrcObject(lTexture);
            break;
        case GX2TextureMapType::Emission:
            lMaterial->Emissive.ConnectSrcObject(lTexture);
            break;
        case GX2TextureMapType::Mask:
            lMaterial->TransparentColor.ConnectSrcObject(lTexture);
            break;
        case GX2TextureMapType::Bake:
            lMaterial->Ambient.ConnectSrcObject(lTexture);
            break;
        case GX2TextureMapType::Shadow:
            lMaterial->Ambient.ConnectSrcObject(lTexture);
            break;
        case GX2TextureMapType::Light:
            uvLayerName = "_uv1"; //lLayerElementUV1->GetName();
            break;
        case GX2TextureMapType::MRA:
            uvLayerName = "_uv1"; //lLayerElementUV1->GetName();
            break;
        case GX2TextureMapType::Metalness:
            uvLayerName = "_uv1"; //lLayerElementUV1->GetName();
            break;
        case GX2TextureMapType::Roughness:
            uvLayerName = "_uv1"; //lLayerElementUV1->GetName();
            break;
        case GX2TextureMapType::SubSurfaceScattering:
            uvLayerName = "_uv1"; //lLayerElementUV1->GetName();
            break;
        default:
            break;
        }
    }
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::MapFacesToVertices(const LODMesh& lodMesh, FbxMesh* lMesh)
{
    // Define which control points belong to a poly
    uint32 uiPolySize(3);
    // TODO make this iterative
    for (uint32 i = 0; i < lodMesh.faceVertices.size(); ++i)
    {
        // first index
        if ((i % uiPolySize) == 0)
        {
            lMesh->BeginPolygon();
        }

        // TODO make this iterative
        lMesh->AddPolygon(lodMesh.faceVertices[i]);

        // last index
        if ((i + 1) % uiPolySize == 0)
            lMesh->EndPolygon();
    }
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::WriteSkin(FbxScene*& pScene, FbxMesh*& pMesh, std::map<uint32, SkinCluster>& BoneIndexToSkinClusterMap, uint32 fmdlIndex)
{
    FbxSkin* pSkin = FbxSkin::Create(pScene, pMesh->GetNode()->GetName());
    FbxAMatrix& lXMatrix = pMesh->GetNode()->EvaluateGlobalTransform();
    const FSKL& fskl = *g_BFRESManager.GetSkeletonByModelIndex(fmdlIndex);

    std::map<uint32, SkinCluster>::iterator iter = BoneIndexToSkinClusterMap.begin();
    std::map<uint32, SkinCluster>::iterator end = BoneIndexToSkinClusterMap.end();


    for (; iter != end; ++iter)
    {
        SkinCluster& skinCluster = iter->second;
        if (skinCluster.m_vControlPointIndices.size() == 0)
            std::string boneName = fskl.bones[iter->first].name;

        FbxNode* pBoneNode = pScene->FindNodeByName(FbxString(skinCluster.m_szName.c_str()));
        assert(pBoneNode != NULL);

        FbxCluster* pCluster = FbxCluster::Create(pScene, skinCluster.m_szName.c_str());
        pCluster->SetLink(pBoneNode);
        // eTotalOne means Mode eTotalOne is identical to mode eNormalize except that the sum of the weights assigned to a control point is not normalized and must equal 1.0.
        // https://help.autodesk.com/view/FBX/2017/ENU/?guid=__cpp_ref_class_fbx_cluster_html
        pCluster->SetLinkMode(FbxCluster::eTotalOne);

        for (uint32 uiControlPoint = 0; uiControlPoint < skinCluster.m_vControlPointIndices.size(); ++uiControlPoint)
        {
            pCluster->AddControlPointIndex(skinCluster.m_vControlPointIndices[uiControlPoint],
                                           skinCluster.m_vControlPointWeights[uiControlPoint]);
        }

        // Now we have the mesh and the skeleton correctly positioned,
        // set the Transform and TransformLink matrix accordingly.
        pCluster->SetTransformMatrix(lXMatrix);
        pCluster->SetTransformLinkMatrix(pBoneNode->EvaluateGlobalTransform());

        // Add the clusters to the skin
        pSkin->AddCluster(pCluster);
    }

    pMesh->AddDeformer(pSkin);
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// Add the specified node to the node array. Also, add recursively
// all the parent node of the specified node to the array.
void AddNodeRecursively(FbxArray<FbxNode*>& pNodeArray, FbxNode* pNode)
{
    if (pNode)
    {
        AddNodeRecursively(pNodeArray, pNode->GetParent());

        if (pNodeArray.Find(pNode) == -1)
        {
            // Node not in the list, add it
            pNodeArray.Add(pNode);
        }
    }
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::WriteBindPose(FbxScene*& pScene, FbxNode*& pMeshNode)
{
    // In the bind pose, we must store all the link's global matrix at the time of the bind.
    // Plus, we must store all the parent(s) global matrix of a link, even if they are not
    // themselves deforming any model.

    // In this example, since there is only one model deformed, we don't need walk through 
    // the scene
    //

    // Now list the all the link involve in the patch deformation
    FbxArray<FbxNode*> lClusteredFbxNodes;
    int i, j;

    if (pMeshNode && pMeshNode->GetNodeAttribute())
    {
        int lSkinCount = 0;
        int lClusterCount = 0;


        lSkinCount = ((FbxGeometry*)pMeshNode->GetNodeAttribute())->GetDeformerCount(FbxDeformer::eSkin);
        //Go through all the skins and count them
        //then go through each skin and get their cluster count
        for (i = 0; i < lSkinCount; ++i)
        {
            FbxSkin* lSkin = (FbxSkin*)((FbxGeometry*)pMeshNode->GetNodeAttribute())->GetDeformer(i, FbxDeformer::eSkin);
            lClusterCount += lSkin->GetClusterCount();
        }

        //if we found some clusters we must add the node
        if (lClusterCount)
        {
            //Again, go through all the skins get each cluster link and add them
            for (i = 0; i < lSkinCount; ++i)
            {
                FbxSkin* lSkin = (FbxSkin*)((FbxGeometry*)pMeshNode->GetNodeAttribute())->GetDeformer(i, FbxDeformer::eSkin);
                lClusterCount = lSkin->GetClusterCount();
                for (j = 0; j < lClusterCount; ++j)
                {
                    FbxNode* lClusterNode = lSkin->GetCluster(j)->GetLink();
                    AddNodeRecursively(lClusteredFbxNodes, lClusterNode);
                }
            }

            // Add the patch to the pose
            lClusteredFbxNodes.Add(pMeshNode);
        }
    }

    // Now create a bind pose with the link list
    if (lClusteredFbxNodes.GetCount())
    {
        // A pose must be named. Arbitrarily use the name of the patch node.
        FbxPose* lPose = FbxPose::Create(pScene, pMeshNode->GetName());

        // default pose type is rest pose, so we need to set the type as bind pose
        lPose->SetIsBindPose(true);

        for (i = 0; i < lClusteredFbxNodes.GetCount(); i++)
        {
            FbxNode* lKFbxNode = lClusteredFbxNodes.GetAt(i);
            FbxMatrix lBindMatrix = lKFbxNode->EvaluateGlobalTransform();

            lPose->Add(lKFbxNode, lBindMatrix);
        }

        // Add the pose to the scene
        pScene->AddPose(lPose);
    }
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
void FBXWriter::CreateSkinClusterData(const FVTX& vert, uint32 uiVertIndex, std::map<uint32, SkinCluster>& BoneIndexToSkinClusterMap, std::vector<BoneMetadata>& boneListInfos, const FSHP& fshp)
{
    uint32 uiBlendIndices[4] = {vert.blendIndex.X, vert.blendIndex.Y, vert.blendIndex.Z, vert.blendIndex.W};
    float fBlendWeights[4] = {vert.blendWeights.X, vert.blendWeights.Y, vert.blendWeights.Z, vert.blendWeights.W};

    if (boneListInfos[uiBlendIndices[0]].eSkinningType == SkinningType::eRigid)
    {
        std::map<uint32, SkinCluster>::iterator iter = BoneIndexToSkinClusterMap.find(boneListInfos[uiBlendIndices[0]].uiBoneIndex); // Try to find a skin cluster with the given bone index

        if (iter != BoneIndexToSkinClusterMap.end()) // If found a skin cluster with the given bone index
        {
            iter->second.m_vControlPointIndices.push_back(uiVertIndex);
            iter->second.m_vControlPointWeights.push_back(1);
        }
        else // Create new cluster with data
        {
            SkinCluster skinCluster;
            skinCluster.m_szName = boneListInfos[uiBlendIndices[0]].szName;
            skinCluster.m_vControlPointIndices.push_back(uiVertIndex);
            skinCluster.m_vControlPointWeights.push_back(1);
            BoneIndexToSkinClusterMap.insert(std::pair<uint32, SkinCluster>(boneListInfos[uiBlendIndices[0]].uiBoneIndex, skinCluster));
        }
    }
    else if (boneListInfos[uiBlendIndices[0]].eSkinningType == SkinningType::eSmooth)
    {
        for (uint32 uiBlendEntry = 0; uiBlendEntry < fshp.vertexSkinCount; ++uiBlendEntry)
        {
            if (fBlendWeights[uiBlendEntry] > 0) // Only write blend weights that are non-zero and that have not already had weights written for this bone index
            {
                std::map<uint32, SkinCluster>::iterator iter = BoneIndexToSkinClusterMap.find(boneListInfos[uiBlendIndices[uiBlendEntry]].uiBoneIndex); // Try to find a skin cluster with the given bone index

                if (iter != BoneIndexToSkinClusterMap.end()) // If found a skin cluster with the given bone index
                {
                    iter->second.m_vControlPointIndices.push_back(uiVertIndex);
                    iter->second.m_vControlPointWeights.push_back(fBlendWeights[uiBlendEntry]);
                }
                else // Create new cluster with data
                {
                    SkinCluster skinCluster;
                    skinCluster.m_szName = boneListInfos[uiBlendIndices[uiBlendEntry]].szName;
                    skinCluster.m_vControlPointIndices.push_back(uiVertIndex);
                    skinCluster.m_vControlPointWeights.push_back(fBlendWeights[uiBlendEntry]);
                    BoneIndexToSkinClusterMap.insert(std::pair<uint32, SkinCluster>(boneListInfos[uiBlendIndices[uiBlendEntry]].uiBoneIndex, skinCluster));
                }
            }
        }
    }
}
