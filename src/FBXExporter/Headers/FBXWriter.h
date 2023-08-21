#pragma once
#include <fbxsdk.h>
#include "BFRES.h"
#include <map>
#include <string>

using namespace BFRESStructs;

class FBXWriter
{
public:
    FBXWriter();
    ~FBXWriter();

    static bool g_bWriteTextures;
	static std::map<std::string, FbxSurfacePhong*> g_MaterialMap;
	static std::map<std::string, FbxFileTexture*> g_TextureMap;

    struct SkinCluster 
    {
        std::string         m_szName;
        std::vector<uint32> m_vControlPointIndices;
        std::vector<float>  m_vControlPointWeights;
    };

	enum class SkinningType
	{
		eSmooth,
		eRigid
	};

	struct BoneMetadata
	{
		uint32       uiBoneIndex;
		std::string  szName; // I know this is wasted space, but bite me. It'll make debugging simpler.
		SkinningType eSkinningType;
	};

    enum class AnimTrackType
    {
        eTranslation,
        eRotation,
        eScale,
        NUM_ANIM_TRACK_TYPES
    };

    void CreateFBX(FbxScene*& pScene, const BFRES& bfres);

    // Animation shit
    void WriteAnimations( FbxScene*& pScene, const Anim& anim );
    void CreateTranslationAnimCurveNode( FbxAnimLayer*& pAnimLayer, FbxNode*& pBone, const BoneAnim& boneAnim );
    void CreateScaleAnimCurveNode( FbxAnimLayer*& pAnimLayer, FbxNode*& pBone, const BoneAnim& boneAnim );
    void CreateRotationAnimCurveNode( FbxAnimLayer*& pAnimLayer, FbxNode*& pBone, const BoneAnim& boneAnim );
    void AddKeyFramesToAnimCurve( FbxAnimCurve*& pAnimCurve, const AnimTrack& animTrack, AnimTrackType animTrackType );

    // Model shit
    void WriteModel( FbxScene*& pScene, const FMDL& fmdl, uint32 fmdlIndex, bool onlySkeleton );
    void WriteSkeleton(FbxScene*& pScene, const FSKL& fskl, std::vector<BoneMetadata>& boneListInfos);
    void WriteShape(FbxScene*& pScene, const FMDL& mdl,  const FSHP& fshp, std::vector<BoneMetadata>& boneListInfos, uint32 fmdlIndex);
    void WriteMesh(FbxSurfacePhong* lMaterial, FbxScene*& pScene, FbxNode*& pLodGroup, const FSHP& fshp, const LODMesh& lodMesh, std::vector<BoneMetadata>& boneListInfos, uint32 fmdlIndex);
    void SetTexturesToMaterial(FbxScene*& pScene, FMAT* fmat, FbxSurfacePhong* lMaterial);

    void MapFacesToVertices( const LODMesh& lodMesh, FbxMesh* lMesh );
    void MapPolygonsToVertices(const LODMesh& lodMesh, FbxMesh* lMesh);

    void WriteSkin(FbxScene*& pScene, FbxMesh*& pMesh, std::map<uint32, SkinCluster>& BoneIndexToSkinClusterMap, uint32 fmdlIndex);
    void WriteBindPose(FbxScene*& pScene, FbxNode*& pMeshNode);

    void CreateSkinClusterData(const FVTX& vert, uint32 uiVertIndex, std::map<uint32, SkinCluster>& BoneIndexToSkinClusterMap, std::vector<BoneMetadata>& boneListInfos, const FSHP& fshp);
    void CreateBone(FbxScene*& pScene, const Bone& bone, FbxNode*& lBoneNode, std::vector<BoneMetadata>& boneListInfos);
};