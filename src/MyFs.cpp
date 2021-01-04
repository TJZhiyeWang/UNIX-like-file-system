#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "../include/FSManager.h"
#include "../include/fileSys.h"
#include "../include/INode.h"

using namespace std;

#define MAXLINE 100

void Initialize()
{
    FSManager::Instance().Initialize(true);
    FileSystem& fs = FSManager::Instance().GetFileSystem();

    /* 初始化SuperBlock */
    fs.g_spb->s_isize = FileSystem::INODE_ZONE_SIZE;
    fs.g_spb->s_fsize = FileSystem::DATA_ZONE_END_SECTOR+1;

    fs.g_spb->s_nfree = 0;
    fs.g_spb->s_ninode = 0;
    for(int i=0;i<100;i++)
    {
        fs.g_spb->s_free[i] = -1;
        fs.g_spb->s_inode[i] = -1;
    }

    /* 初始化inode节点*/
    for(int i=99;i>=0;i--)
    {
        fs.g_spb->s_inode[fs.g_spb->s_ninode++] = i;
    }
    /* 初始化空闲块管理 */

    for(int i=FileSystem::DATA_ZONE_END_SECTOR;i>=FileSystem::DATA_ZONE_START_SECTOR;i--)
    {
        fs.BFree(i);
    }

    fs.g_spb->s_flock = 0;
    fs.g_spb->s_fmod  = 1;
    fs.g_spb->s_ilock = 0;
    fs.g_spb->s_ronly = 0;
    fs.g_spb->s_time  = time(0);

    /* 分配0#inode给根目录 */
    Inode* pNode = fs.IAlloc();
    pNode->i_flag |= (Inode::IACC | Inode::IUPD);
    pNode->i_mode = Inode::IALLOC | Inode::IFDIR /* Most vital!! */| Inode::IREAD | Inode::IWRITE | Inode::IEXEC | (Inode::IREAD >> 3) | (Inode::IWRITE >> 3) | (Inode::IEXEC >> 3) | (Inode::IREAD >> 6) | (Inode::IWRITE >> 6) | (Inode::IEXEC >> 6);
    pNode->i_nlink = 1;
    pNode->i_uid = -1;
    pNode->i_gid = -1;
    pNode->i_size = 120;
    FSManager::Instance().GetInodeTable().IPut(pNode);

    FSManager::Instance().GetFileManager().Initialize(true);

    int mode = Inode::IALLOC | Inode::IFDIR /* Most vital!! */| Inode::IREAD | Inode::IWRITE | Inode::IEXEC | (Inode::IREAD >> 3) | (Inode::IWRITE >> 3) | (Inode::IEXEC >> 3) | (Inode::IREAD >> 6) | (Inode::IWRITE >> 6) | (Inode::IEXEC >> 6);

    FSManager::Instance().GetFileManager().fcreat("/home",mode);
    FSManager::Instance().GetFileManager().fcreat("/dev",mode);
    FSManager::Instance().GetFileManager().fcreat("/bin",mode);
    FSManager::Instance().GetFileManager().fcreat("/etc",mode);
    fs.Update();
}

void type_prompt()//命令提示行
{
    FileManager& fileMgr = FSManager::Instance().GetFileManager();
    printf("WZY@%s$ ",fileMgr.curdir);
}

int read_command()//读取命令
{
    char buffer[MAXLINE];
    char *argv[100];//指针数组，用于分割命令和参数

    while(fgets(buffer,MAXLINE,stdin)!=NULL)
    {
        if(buffer[0]=='\0')
            return 0;
        if(buffer[strlen(buffer)-1]=='\n')
            buffer[strlen(buffer)-1] = '\0';
        /* 命令处理 */
        int i=0,len,index=0;
        while(buffer[i]==' ')//处理开头空格
            i++;
        if(buffer[i]=='\0')//全是空格
        {
            return 0;
        }
        argv[index++] = buffer+i;
        /* 将空格处理为尾0,便于提取参数 */
        for(len=strlen(buffer);i<len;i++)
        {
            if(buffer[i]==' ')
            {
                buffer[i] = '\0';
            }
            else
            {
                /* 需防止越界，非空格字符的前一个字符为\0，才是参数的起始位置 */
                if((i-1>=0)&&(buffer[i-1]=='\0'))
                    argv[index++] = buffer+i;
            }
        }
        argv[index] = NULL;//最后一个参数为NULL
        /* 处理完毕，开始根据命令调用API */
        if(strcmp(argv[0],"exit")==0)//退出系统
        {
           return -1;
        }
        if(strcmp(argv[0],"cd")==0)//cd,带一个参数
        {
            if(argv[1]!=NULL&&argv[2]==NULL)
            {
                FSManager::Instance().GetFileManager().ChDir(argv[1]);
            }
            else
            {
                printf("Usage: cd -p\np: pathname of the dir\n");
            }
        }
        else if(strcmp(argv[0],"ls")==0)//ls，0参数
        {
            if(argv[1]==NULL)
                FSManager::Instance().GetFileManager().ls();
            else
            {
                printf("Usage: ls\nNone parameter\n");
            }
        }
        else if(strcmp(argv[0],"mkdir")==0)//mkdir -p，创建目录
        {
            if(argv[1]!=NULL&&argv[2]==NULL)
            {
                int mode = Inode::IALLOC | Inode::IFDIR /* Most vital!! */| Inode::IREAD | Inode::IWRITE | Inode::IEXEC | (Inode::IREAD >> 3) | (Inode::IWRITE >> 3) | (Inode::IEXEC >> 3) | (Inode::IREAD >> 6) | (Inode::IWRITE >> 6) | (Inode::IEXEC >> 6);
                FSManager::Instance().GetFileManager().fcreat(argv[1],mode);
                FSManager::Instance().GetFileManager().fclose();
            }
            else
            {
                printf("Usage: mkdir -p\np: path of dir you want to make\n");
            }
        }
        else if(strcmp(argv[0],"read")==0)//cat -p，读取文件
        {
            if(argv[1]!=NULL&&argv[2]==NULL)
            {
                Inode* pInode;
                pInode=FSManager::Instance().GetFileManager().fopen(argv[1]);
                if(pInode==NULL)
                {
                    printf("File may not exist!!!\n");
                }
                else
                {
                    int count;
                    char buf[513];
                    memset(buf,0,sizeof(buf));
                    while((count=FSManager::Instance().GetFileManager().fread(pInode,buf,512))!=0)
                    {
                        printf("%s",buf);
                    }
                    printf("\n");
                    FSManager::Instance().GetFileManager().fclose();
                }
            }
            else
            {
                printf("Usage: read -p\np: path of file\n");
            }
        }
        else if(strcmp(argv[0],"rm")==0)//rm -p，删除文件或文件夹
        {
            if(argv[1]!=NULL&&argv[2]==NULL)
            {
                FSManager::Instance().GetFileManager().fdelete(argv[1]);
                FSManager::Instance().GetFileManager().fclose();
            }
            else
            {
                printf("Usage: rm -p\np: path of file or dir\n");
            }
        }
        else if(strcmp(argv[0],"create")==0)//vi -p，新建文件
        {
            if(argv[1]!=NULL&&argv[2]==NULL)
            {
                int mode = Inode::IALLOC | Inode::IREAD | Inode::IWRITE | Inode::IEXEC | (Inode::IREAD >> 3) | (Inode::IWRITE >> 3) | (Inode::IEXEC >> 3) | (Inode::IREAD >> 6) | (Inode::IWRITE >> 6) | (Inode::IEXEC >> 6);
                FSManager::Instance().GetFileManager().fcreat(argv[1],mode);
                FSManager::Instance().GetFileManager().fclose();
            }
            else
            {
                printf("Usage: create -p\np: pathname of file to create");
            }
        }
        else if(strcmp(argv[0],"write")==0)//write -p，写文件
        {
            if(argv[1]!=NULL&&argv[2]==NULL)
            {
                Inode* pInode;
                pInode = FSManager::Instance().GetFileManager().fopen(argv[1]);
                if(pInode==NULL)
                {
                    printf("File not exist!!!\n");
                }
                else
                {
                    unsigned char filebuf[1025];
                    memset(filebuf,0,sizeof(filebuf));
                    while(fgets((char *)filebuf,1024,stdin)!=NULL)
                    {
                        if(strcmp((char *)filebuf,"^exit\n")==0)
                            break;
                        FSManager::Instance().GetFileManager().fwrite(pInode,(char *)filebuf,sizeof(filebuf));
                        memset(filebuf,0,sizeof(filebuf));
                    }
                    FSManager::Instance().GetInodeTable().IPut(pInode);
                }
                FSManager::Instance().GetFileManager().fclose();
            }
        }
        else if(strcmp(argv[0],"movein")==0)//import -p1 -p2，计算机中文件保存到自己的磁盘
        {
            if(argv[2]!=NULL&&argv[3]==NULL)
            {
                FILE* fstream;
                Inode* pInode;
                fstream = fopen(argv[1],"rb");//只读
                if(fstream==NULL)
                {
                    printf("Failed to open %s\n",argv[1]);
                    return 0;
                }
                int mode = Inode::IALLOC | Inode::IREAD | Inode::IWRITE | Inode::IEXEC | (Inode::IREAD >> 3) | (Inode::IWRITE >> 3) | (Inode::IEXEC >> 3) | (Inode::IREAD >> 6) | (Inode::IWRITE >> 6) | (Inode::IEXEC >> 6);
                FSManager::Instance().GetFileManager().fcreat(argv[2],mode);
                FSManager::Instance().GetFileManager().fclose();
                pInode = FSManager::Instance().GetFileManager().fopen(argv[2]);
                if(pInode==NULL)
                {
                    printf("File may not exist in mydisk.img\n");
                    return 0;
                }
                int count;
                unsigned char filebuf[512];
                while(true)
                {
                    memset(filebuf,0,sizeof(filebuf));
                    count = fread(filebuf,1,512,fstream);
                    if(count>0)
                    {
                        FSManager::Instance().GetFileManager().fwrite(pInode,(char*)filebuf,count);
                    }
                    else
                        break;
                }
                FSManager::Instance().GetInodeTable().IPut(pInode);
                FSManager::Instance().GetFileManager().fclose();
                fclose(fstream);
            }
            else
            {
                printf("Usage: -p1 -p2\np1: pathname of file in your pc\np2: pathname you wan to save\n");
            }
        }
        else if(strcmp(argv[0],"moveout")==0)//export -p1 -p2，磁盘文件保存到计算机
        {
            if(argv[2]!=NULL&&argv[3]==NULL)
            {
                FILE* fstream;
                Inode* pInode;
                fstream = fopen(argv[2],"wb+");
                if(fstream==NULL)
                {
                    printf("Failed to open %s\n",argv[2]);
                    return 0;
                }
                pInode = FSManager::Instance().GetFileManager().fopen(argv[1]);
                if(pInode==NULL)
                {
                    printf("File may not exist in mydisk.img\n");
                    return 0;
                }
                int count;
                unsigned char filebuf[512];
                while(true)
                {
                    memset(filebuf,0,sizeof(filebuf));
                    count = FSManager::Instance().GetFileManager().fread(pInode,(char*)filebuf,512);
                    if(count>0){
                        fwrite(filebuf,1,count,fstream);
                    }
                    else
                        break;
                }
                FSManager::Instance().GetFileManager().fclose();
                fclose(fstream);
            }
            else
            {
                printf("Usage: -p1 -p2\np1: pathname of file in your pc\np2: pathname you wan to load\n");
            }
        }
        else
        {
            printf("Invalid instruction\n");
        }

        FSManager::Instance().GetFileSystem().g_spb->s_fmod=1;
        FSManager::Instance().GetFileSystem().Update();
        return 0;
    }
    return -1;
}

int main()
{
    char buf[2];
    printf("Do you want to format the disk?[y/n]\n");
    while(fgets(buf,2,stdin)!=NULL)
    {
        if(buf[0]=='y'||buf[0]=='Y'||buf[0]=='\n')
        {
            Initialize();
            break;
        }
        else if(buf[0]=='n'||buf[0]=='N')
        {
            if(FSManager::Instance().Initialize(false)==1)
                break;
            else
            {
                Initialize();
                break;
            }
        }
        printf("Invalid input\n");
        printf("Do you want to format the disk?[y/n]\n");
    }
    getchar();//读取回车
    while(true)
    {
        type_prompt();
        if(read_command()==-1)
            break;
    }
    return 0;
}
