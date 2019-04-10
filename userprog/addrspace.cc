// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "stats.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable, char* fileName)
{
    NoffHeader noffH;
    unsigned int i, size;
	//OpenFile* file;		//Archivo swp
	//OpenFile* fileX;	//Archivo ejecutablde
	char buffer[8];		//Buffer de tamaño 1 byte
	stackFIFO=new int[NumPhysPages];
	spFIFO=0;
	pageFaults=0;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
	//Practica Swap
	printf("Nombre Archivo: %s .\n", fileName);
	fileX=fileSystem->Open(fileName);				//Se abre el archivo ejecutable
	fileNameS=fileName;

	strcat(fileName, ".swp");					//Se crea el nombre del archivo de intercambio
	printf("Archivo: %s . %d,tamaño ant mult \n", fileName, size);
	switch(fileSystem->Create(fileName, 0))				//Se crea el archivo de intercambio
	{
		case 0:
			printf("ERROR AL CREAR EL ARCHIVO\n");
		break;
		case 1:
			printf("Archivo creado\n");
			file=fileSystem->Open(fileName);		//Se abre el archivo de intercambio
			if(file!=NULL && fileX!=NULL)			//Se verifica si el archivo de intercambio
			{						// y el archivo ejecutable se abrieron correctamente
									//Si los dos archivos se abrieron correctamente entonces
				printf("Archivos abiertos con exito\n");	//comienza el copiado dde los datos
				
				for(int it=40;it < fileX->Length();it++)
				{
					fileX->ReadAt(buffer, 1, it);
					file->WriteAt(buffer, 1, it-40);
				}
				printf("Archivo X:%d, Archivo SWP:%d", fileX->Length(), file->Length());
//				fileSystem->Remove(fileName);
//				fileSystem->Remove(fileNameS);
				delete(file);
				delete(fileX);
				fileNameS=fileName;
			}
			else
			{
				printf("ERROR AL ABRIR LOS ARCHIVOS\n");
			}
		break;
	}
	//-----------------------------------------------------------------------------
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;
	if(!strcmp(stats->op, "-D"))
	{
	//Practica 0------------------------------------------
	printf("\nLa cantidad de marcos que requiere el proceso para ejecutarse son: %d \n ", numPages);
	printf("Tamaño del proceso %d bytes\n", size);
	printf("\nTabla de Pagínas\n\n");
	printf("Indice\t No. Marco\t Bit Validez\n ");
	//----------------------
	}
	if(!strcmp(stats->op, "-C")||!strcmp(stats->op, "-M")||!strcmp(stats->op, "-F"))
	{
		printf("\n\nAlgoritm: %s\n", stats->alg);
		if(!strcmp(stats->op, "-C")||!strcmp(stats->op, "-F"))
		{
			printf("Process size: %d bytes\n", size);
			printf("Num. Page: %d\n", numPages);
			printf("VPN: ");
		}
	}


   // ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
	pageTable[i].physicalPage = i;
	//pageTable[i].valid = TRUE;
	pageTable[i].valid = FALSE;	//Practica 2-- Paso 1.
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
	if(!strcmp(stats->op, "-D"))
		printf("%d\t %d\t\t %d\n", i, pageTable[i].physicalPage, pageTable[i].valid);
    }
    	if(!strcmp(stats->op, "-D") || !strcmp(stats->op, "-M"))
    	{
		printf("\nMapeo de Direcciones Logicas a Fisicas\n\n");
		printf("Dir. Lógica\t No. de Página(p)\t Desplazamiento(d)\t Dir. Fisica=Formula\n ");
	}

    
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
    //bzero(machine->mainMemory, size);

// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
//-----------------------------

        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
			noffH.code.size, noffH.code.inFileAddr);

//-------------------------------------------------------------------------
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

void AddrSpace::EnablePage(int pageFrame, int p)
{
	int addr;

	addr=PageSize*p;
	pageTable[p].physicalPage=pageFrame;
	//-----
	stackFIFO[pageFrame]=p;
	//-----
	pageTable[p].valid=true;
	file=fileSystem->Open(fileNameS);
	if(file!=NULL)
	{
		file->ReadAt(&(machine->mainMemory[pageFrame*PageSize]), PageSize, addr);
		pageFaults++;
		stats->numDiskReads++;
		delete(file);
	}
	else
	{
		printf("Error al leer archivo de intercambio\n");
	}
}

void AddrSpace::algFIFO(int numPage)
{
	pageTable[stackFIFO[0]].valid=FALSE;
	pageTable[numPage].physicalPage=pageTable[stackFIFO[0]].physicalPage;
	if(pageTable[stackFIFO[0]].dirty)
	{
		swapOut(stackFIFO[0]);
	}
	shift();
	pageTable[numPage].valid=TRUE;
	stackFIFO[NumPhysPages-1]=numPage;
	file=fileSystem->Open(fileNameS);
	if(file!=NULL)
	{
		file->ReadAt(&(machine->mainMemory[pageTable[numPage].physicalPage*PageSize]), PageSize, PageSize*numPage);
		stats->numDiskReads++;
		delete(file);
	}
	else
	{
		printf("Error al leer archivo de intercambio\n");
	}
	
}

void AddrSpace::swapOut(int numPage)
{
	file=fileSystem->Open(fileNameS);
	if(file!=NULL)
	{	
		/*
		file->ReadAt(&(machine->mainMemory[pageTable[numPage].physicalPage*PageSize]), PageSize, PageSize*numPage);
		stats->numDiskReads++;
		*/
		stats->numDiskWrites++;
		file->WriteAt(&(machine->mainMemory[pageTable[numPage].physicalPage*PageSize]), PageSize, PageSize*numPage);
		delete(file);
	}
	else
	{
		printf("Error al leer archivo de intercambio\n");
	}
}

void AddrSpace::shift()//reordena la pila
{
	/*
	for(int i=1; i<NumPhysPages;i++)
		stackFIFO[i-1]=stackFIFO[i];
	*/
	for(int i=0;i<NumPhysPages;i++)
		stackFIFO[i]=stackFIFO[i+1];
}
