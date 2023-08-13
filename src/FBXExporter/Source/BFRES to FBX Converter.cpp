#include <iostream>
#include <fbxsdk.h>
#include "MyFBXCube.h"
#include "FBXWriter.h"
#include "XmlParser.h"
#include "BFRES.h"
#include <windows.h>
#include "Globals.h"

#ifdef IOS_REF
    #undef  IOS_REF
    #define IOS_REF (*(pManager->GetIOSettings()))
#endif


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// Export document, the format is ascii by default
bool SaveDocument(FbxManager* pManager, FbxDocument* pDocument, const char* pFilename, int pFileFormat = -1, bool pEmbedMedia = false)
{
    int lMajor, lMinor, lRevision;
    bool lStatus = true;

    // Create an exporter.
    FbxExporter* lExporter = FbxExporter::Create(pManager, "");

    if (pFileFormat < 0 || pFileFormat >= pManager->GetIOPluginRegistry()->GetWriterFormatCount())
    {
        // Write in fall back format if pEmbedMedia is true
        pFileFormat = pManager->GetIOPluginRegistry()->GetNativeWriterFormat();

        // if (!pEmbedMedia)
        // {
        //     //Try to export in ASCII if possible
        //     int lFormatIndex, lFormatCount = pManager->GetIOPluginRegistry()->GetWriterFormatCount();
        //
        //     for (lFormatIndex = 0; lFormatIndex < lFormatCount; lFormatIndex++)
        //     {
        //         if (pManager->GetIOPluginRegistry()->WriterIsFBX(lFormatIndex))
        //         {
        //             FbxString lDesc = pManager->GetIOPluginRegistry()->GetWriterFormatDescription(lFormatIndex);
        //             const char* lASCII = "ascii";
        //             if (lDesc.Find(lASCII) >= 0)
        //             {
        //                 pFileFormat = lFormatIndex;
        //                 break;
        //             }
        //         }
        //     }
        // }
    }

    // Set the export states. By default, the export states are always set to 
    // true except for the option eEXPORT_TEXTURE_AS_EMBEDDED. The code below 
    // shows how to change these states.
    //IOS_REF.SetBoolProp(EXP_FBX_MATERIAL, true);
    //IOS_REF.SetBoolProp(EXP_FBX_TEXTURE, true);
    //IOS_REF.SetBoolProp(EXP_FBX_EMBEDDED, pEmbedMedia);
    //IOS_REF.SetBoolProp(EXP_FBX_ANIMATION, true);
    //IOS_REF.SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);

    // Initialize the exporter by providing a filename.
    if (lExporter->Initialize(pFilename, pFileFormat, pManager->GetIOSettings()) == false)
    {
        FBXSDK_printf("Call to FbxExporter::Initialize() failed.\n");
        FBXSDK_printf("Error returned: %s\n\n", lExporter->GetStatus().GetErrorString());
        return false;
    }

    FbxManager::GetFileFormatVersion(lMajor, lMinor, lRevision);
#if PRINT_DEBUG_INFO
    FBXSDK_printf("FBX version number for this version of the FBX SDK is %d.%d.%d\n\n", lMajor, lMinor, lRevision);
#endif
    // Export the scene.
    lStatus = lExporter->Export(pDocument);

    // Destroy the exporter.
    lExporter->Destroy();
    return lStatus;
}




// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// Parse any flags after the initial mandatory arguments
void ParseArguments( int argc, char**& argv )
{
    FBXWriter::g_bWriteTextures = true;
    medianFilePath.assign( argv[ 1 ] );
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
int main( int argc, char* argv[] )
{
    // If there are no arguments, assume this is debugging and use the debugging filepath
    ParseArguments( argc, argv );

	uint32 lastIndex = medianFilePath.find_last_of(".");
    uint32 lastSlashIndex = medianFilePath.find_last_of("\\");
    std::string fileName = medianFilePath.substr(lastSlashIndex + 1, lastIndex - lastSlashIndex - 1);
    
    BFRESStructs::BFRES* bfres = g_BFRESManager.GetBFRES();
    XML::XmlParser::Parse(medianFilePath.c_str(), *bfres);

    FbxManager* lSdkManager = FbxManager::Create();
    
    if (argc == 1)
    {
		if (!CreateDirectoryA(OUTPUT_FILE_DIR, NULL) && ERROR_ALREADY_EXISTS != GetLastError())
			assert(0 && "Failed to create directory.");

        fbxExportPath = OUTPUT_FILE_DIR;
        fileName = "Name";
    }
    else
    {
        fbxExportPath = argv[ 2 ];
        if (!CreateDirectoryA( fbxExportPath.c_str(), NULL) && ERROR_ALREADY_EXISTS != GetLastError())
            assert(0 && "Failed to create directory.");
    }

    // gameknife, we should export one fbx per model
    
    //fbx->CreateFBX( pScene, *bfres );

    // Convert the scene to meters using the defined options.
    const FbxSystemUnit::ConversionOptions lConversionOptions = {
        false, /* mConvertRrsNodes */
        true, /* mConvertLimits */
        true, /* mConvertClusters */
        true, /* mConvertLightIntensity */
        true, /* mConvertPhotometricLProperties */
        true  /* mConvertCameraClipPlanes */
      };
    
    for (uint32 i = 0; i < bfres->fmdl.size(); i++)
    {
        FbxScene* pScene = FbxScene::Create(lSdkManager, "Scene lame");
        FbxSystemUnit::m.ConvertScene( pScene, lConversionOptions );

        FBXWriter::g_MaterialMap.clear();
        FBXWriter::g_TextureMap.clear();
        
        FBXWriter* fbx = new FBXWriter();
        fbx->WriteModel(pScene, bfres->fmdl[i], i, false);
        
        FbxSystemUnit::cm.ConvertScene( pScene, lConversionOptions );
        string SingleFbxPath = fbxExportPath + bfres->fmdl[i].name + ".fbx";
        SaveDocument(lSdkManager, pScene, SingleFbxPath.c_str());
    }

    if(bfres->fska.anims.size() > 0)
    {
        FbxScene* pScene = FbxScene::Create(lSdkManager, "Scene lame");
        FbxSystemUnit::m.ConvertScene( pScene, lConversionOptions );

        FBXWriter::g_MaterialMap.clear();
        FBXWriter::g_TextureMap.clear();
        
        FBXWriter* fbx = new FBXWriter();

        // skeleton should write
        for (uint32 i = 0; i < bfres->fmdl.size(); i++)
        {
            fbx->WriteModel(pScene, bfres->fmdl[i], i, true);
        }
        
        for (const Anim& anim : bfres->fska.anims)
        {
            fbx->WriteAnimations(pScene, anim);
        }

        FbxSystemUnit::cm.ConvertScene( pScene, lConversionOptions );
        // this name will import as asset name prefix to ue, so we should care about it

        // some animation may comes from mdl file, filename without Animation, add it
        
        if( fileName.find("_Animation") == std::string::npos )
        {
            fileName += "_Mdl_Animation";
        }
        //string SingleFbxPath = fbxExportPath + bfres->fska.anims[0].m_szName + "_Animation";
        string SingleFbxPath = fbxExportPath + fileName;
        SaveDocument(lSdkManager, pScene, SingleFbxPath.c_str());
    }

    return 0;
}

