#include <stdio.h>
#include <string.h>
#include "file.h"
#include "../system/memory.h"
#include "../simulator/mctypes.h"
#include "loader.h"
#include "kind.h"
#include "tyskel.h"
#include "const.h"

#define LINKCODE_EXT ".bc"
#define BYTECODE_EXT ".lp"

//Values needing only during loading.
//TwoBytes LD_LOADER_numLKinds;

//TwoBytes LD_LOADER_numTySkels;

//TwoBytes LD_LOADER_numHConsts;

int LD_LOADER_LoadLinkcodeVer();

int LD_LOADER_LinkOpen(char* modname);
int LD_LOADER_LoadModuleName(char* modname);
int LD_LOADER_LoadDependencies(char* modname);
MEM_GmtEnt* LD_LOADER_GetNewGMTEnt();
void* LD_LOADER_ExtendModSpace(MEM_GmtEnt* ent, int size);
int LD_LOADER_SetName(MEM_GmtEnt* ent,char* modname);
void LD_CODE_LoadCodeSize(MEM_GmtEnt* ent);



//Defines the primary procedure of the loader: the load function

//Loads the module into returns it's index in the global module table
//Returns -1 on failure.
int LD_LOADER_Load(char* modname)
{
	printf("Opening...");
	if(-1==LD_LOADER_LinkOpen(modname))
		return -1;
	printf("\nChecking Version...");
	if(-1==LD_LOADER_LoadLinkcodeVer())
		return -1;
	printf("\nChecking Name...");
	if(-1==LD_LOADER_LoadModuleName(modname))
		return -1;
	printf("\nChecking Dependencies...");
	if(-1==LD_LOADER_LoadDependencies(modname))
		return -1;
	
	printf("\nInitializing Module Table Entry...");
	fflush(stdout);
	MEM_GmtEnt* gmtEnt=LD_LOADER_GetNewGMTEnt();
	LD_LOADER_SetName(gmtEnt,modname);
	printf("\nLoading Code Size\n");
	LD_CODE_LoadCodeSize(gmtEnt);
	printf("Loading Kind Table\n");
	LD_KIND_LoadKst(gmtEnt);
	printf("Loading Type Skeleton Table\n");
	LD_TYSKEL_LoadTst(gmtEnt);
	printf("Loading Constant Table\n");
	LD_CONST_LoadCst(gmtEnt);
	printf("Done\n");
	LD_STRING_LoadStrings(gmtEnt);
	return 0;
}

int LD_LOADER_LinkOpen(char* modname)
{
	if(LD_FILE_Exists(modname,LINKCODE_EXT))
	{
		return LD_FILE_Open(modname,LINKCODE_EXT);
	}
	else
	{
		if(LD_FILE_Link(modname))
			return LD_FILE_Open(modname,LINKCODE_EXT);
		else
			return -1;
	}
}

int LD_LOADER_LoadLinkcodeVer()
{
	int tmp=(int)LD_FILE_GETWORD();
	printf("Version is %d.\n",tmp);
	if(tmp!=1)
		return -1;
	return 0;
}

int LD_LOADER_LoadModuleName(char* modname)
{
	if(LD_FILE_GET1()!=strlen(modname)+1)
		return -1;
	
	char c;
	int i=0;
	do
	{
		c=LD_FILE_GET1();
		if(c!=modname[i++])
			return -1;
	}while(c!='\0');
	return 0;
}

int LD_LOADER_LoadDependencies(char* modname)
{
	char namelen=0;
	char name[256];
	TwoBytes count = LD_FILE_GET2();
	int i;
	int linktime=LD_FILE_ModTime(modname,LINKCODE_EXT);
	for(i=0;i<count;i++)
	{
		namelen=LD_FILE_GET1();
		LD_FILE_GetString(name,namelen);
		if(LD_FILE_ModTime(name,BYTECODE_EXT)>linktime)
		{
			free(name);
			if(LD_FILE_Link(modname))
			{
				LD_FILE_Open(modname,LINKCODE_EXT);
				LD_LOADER_LoadLinkcodeVer();
				LD_LOADER_LoadModuleName(modname);
				count=LD_FILE_GET2();
				i=-1;
				continue;
			}
			else
				return -1;
		}
	}
	return 1;
}

MEM_GmtEnt* LD_LOADER_GetNewGMTEnt()
{
	int i;
	for(i=0;i<MEM_MAX_MODULES;i++)
	{
		if(MEM_modTable[i].modname==NULL)
		{
			MEM_modTable[i].modSpaceEnd=MEM_modTable[i].modSpaceBeg=MEM_memTop;
			MEM_modTable[i].codeSpaceEnd=MEM_memBot;
			return MEM_modTable+i;
		}
	}
	return NULL;
}

void* LD_LOADER_ExtendModSpace(MEM_GmtEnt* ent, int size)
{
	void* tmp=(void*)ent->modSpaceEnd;
	ent->modSpaceEnd+=size;
	return tmp;
}

int LD_LOADER_SetName(MEM_GmtEnt* ent,char* modname)
{
	char* namebuf=(char*)LD_LOADER_ExtendModSpace(ent,strlen(modname)+1);
	strcpy(namebuf,modname);
	ent->modname=namebuf;
	return 0;
}

void LD_CODE_LoadCodeSize(MEM_GmtEnt* ent)
{
	Word codesize=LD_FILE_GETWORD();
	ent->codeSpaceBeg=ent->codeSpaceEnd-(int)codesize;
}