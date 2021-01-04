#include "../include/FSManager.h"

/* 全局变量声明 */
FSManager FSManager::instance;


FSManager::FSManager()
{

}
FSManager::~FSManager()
{

}

FSManager& FSManager::Instance()
{
    return FSManager::instance;
}

int FSManager::Initialize(bool flag)
{
    int res;

    this->bm = new BufferManager();
    this->disk = new Disk();
    this->filesystem = new FileSystem();
    this->inodetable = new InodeTable();
    this->filemanager = new FileManager();

    this->bm->Initialize();
    if(flag==true)
    {
        this->disk->Initialize();
        res=1;
    }
    else
    {
        res = this->disk->OpenDisk();
    }
    this->filesystem->Initialize();
    this->inodetable->Initialize();
    if(flag==false)
        this->filemanager->Initialize(flag);

    return res;
}

BufferManager& FSManager::GetBufferManager()
{
    return *(this->bm);
}

Disk& FSManager::GetDisk()
{
    return *(this->disk);
}

FileSystem& FSManager::GetFileSystem()
{
    return *(this->filesystem);
}

InodeTable& FSManager::GetInodeTable()
{
    return *(this->inodetable);
}

FileManager& FSManager::GetFileManager()
{
    return *(this->filemanager);
}
