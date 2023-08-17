import os
import shutil
import re
import multiprocessing
from multiprocessing import Pool, Manager
from multiprocessing.sharedctypes import Value
import time

PARRALEL = True
PROCESS_MAX = multiprocessing.cpu_count()

prefix_filters = [
    # "FldObj_",
    # "TwnObj_",
]

global_start_time = time.time()
global_initial_working_dir = os.getcwd()
global_config_type = "Release"

global_in_dir = os.path.join(global_initial_working_dir, "In\\")
global_out_dir = os.path.join(global_initial_working_dir, "Out\\")
global_pack_dir = os.path.join(global_initial_working_dir, "Pack\\")

global_importer_bin = os.path.join(global_initial_working_dir, "Importer", global_config_type, "BFRESImporter.exe")
global_exporter_bin = os.path.join(global_initial_working_dir, "Exporter", global_config_type, "FBXExporter.exe")

def SingleTask(param) :
    fileGroupSubPath : str = param[0]
    sbfresFile : str = param[1]
    fileNameNoExt : str = param[2]

    share_context_progress : Value = param[3]
    share_context_total : Value = param[4]
    share_context_progress.value = share_context_progress.value + 1
    print("[{}/{}] {}".format( share_context_progress.value, share_context_total.value, sbfresFile) )

    importerCommand = "\"" + global_importer_bin + "\" \"" + \
        os.path.join(global_in_dir, sbfresFile) + "\" \"" + \
        fileGroupSubPath + "/\""
    os.system("\"" + importerCommand + "\"")

    # If it's not a texture Bfres, run the exporter
    inputXMLPath = os.path.join(fileGroupSubPath, fileNameNoExt + ".xml")
    if not sbfresFile.endswith(".Tex1.sbfres"):
        if not sbfresFile.endswith(".Tex2.sbfres"):
            outputFBXPath = fileGroupSubPath + "/"
            exporterCommand = "\"" + global_exporter_bin + "\" \"" + inputXMLPath + \
                "\" \"" + outputFBXPath + "\"" + " -t"
            os.system("\"" + exporterCommand + "\"")

    #os.system("del /q \"" + inputXMLPath + "\"")

# it just divide lines to process, not balanced control
if __name__ == '__main__':   

    mgr = Manager()
    share_context_progress = mgr.Value('i', 0)
    share_context_total = mgr.Value('i', 0)

    if not os.access(global_in_dir, os.F_OK):
        os.mkdir(global_in_dir)
    if not os.access(global_out_dir, os.F_OK):
        os.mkdir(global_out_dir)
    if not os.access(global_pack_dir, os.F_OK):
        os.mkdir(global_pack_dir)

    # step0. remove empty dir in Out
    for root, dir, file in os.walk(global_out_dir):
        if not file and not dir:
            os.rmdir(root)

    # step1. extract pack
    for pack in sorted(os.listdir(global_pack_dir)):
        purename = os.path.splitext(pack)[0]
        if pack.endswith(".pack") and not os.path.exists(os.path.join(global_pack_dir, purename)):
            os.system("sarc extract {}".format( os.path.join(global_pack_dir, pack) ))

    # step2. move sbfres to In
    for root, dir, file in os.walk(global_pack_dir):
        for f in file:
            if f.endswith(".sbfres"):
                print('moving {}...'.format(f))
                shutil.move(os.path.join(root, f), global_in_dir)

    # step3. -xx, sub pack rename, Item_ exception, remove this logic, -xx is partial one, but _xx isnot
    # for sbfresFile in sorted(os.listdir(global_in_dir)):
    #     if( re.match(".*-[0-9][0-9].sbfres", sbfresFile) and not sbfresFile.startswith("Item_") ):
    #         new_text = re.sub("-", "_", sbfresFile)
    #         os.rename(os.path.join(global_in_dir, sbfresFile), os.path.join(global_in_dir, new_text))

    # step4. prepare tasks
    lines = []
    for sbfresFile in sorted(os.listdir(global_in_dir)):
        if sbfresFile.endswith(".sbfres"):		
            fileNameNoExt = sbfresFile[0:len(sbfresFile) - len(".sbfres")]
            # Set file group name
            if sbfresFile.endswith("_Animation.sbfres"):
                fileGroupName = sbfresFile[0:len(
                    sbfresFile) - len("_Animation.sbfres")]
            elif sbfresFile.endswith(".Tex1.sbfres"):
                fileGroupName = sbfresFile[0:len(sbfresFile) - len(".Tex1.sbfres")]
            elif sbfresFile.endswith(".Tex2.sbfres"):
                fileGroupName = sbfresFile[0:len(sbfresFile) - len(".Tex2.sbfres")]
                continue
            elif( re.match(".*-[0-9][0-9].sbfres", sbfresFile) ):
                fileGroupName = sbfresFile[0:len(sbfresFile) - 10]
            else:
                fileGroupName = sbfresFile[0:len(sbfresFile) - len(".sbfres")]

            # If Armor_ or Weapon_, skip
            if fileGroupName.startswith("Demo") or fileGroupName.startswith("Armor_") or fileGroupName.startswith("Weapon_") or fileGroupName.startswith("Player") or fileGroupName.startswith("UMii_"):
                continue

            # If prefix filter, skip
            if len(prefix_filters) > 0:
                included = False
                for prefix_filter in prefix_filters:
                    if fileGroupName.startswith(prefix_filter):
                        included = True
                        break

                if not included:
                    continue

            # Create filegroup subdirectory
            fileGroupSubPath = os.path.join(global_out_dir, fileGroupName)
            
            if not os.path.exists(fileGroupSubPath):
                os.makedirs(fileGroupSubPath)

            task_count = len(lines)
            lines.append((fileGroupSubPath, sbfresFile, fileNameNoExt, share_context_progress, share_context_total))

    share_context_progress.value = 0
    share_context_total.value = len(lines)
    print( "{} tasks use {} cores".format(share_context_total.value, PROCESS_MAX if PARRALEL else 1) )
    
    if PARRALEL:
        pool = Pool(PROCESS_MAX)
        results = pool.map(SingleTask, lines)
        pool.close()
        pool.join()
    else:
        results = map(SingleTask, lines)
        list(results)

    print('all tasks taken: {:.2f} seconds.'.format(time.time() - global_start_time) )