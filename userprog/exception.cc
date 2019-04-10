// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "stats.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
void PageFaultExceptionHandler();
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

	switch(which)
	{
		case SyscallException:
			switch(type)
			{
				case SC_Halt:
					DEBUG('a', "Shutdown, initiated by user program.\n");
					interrupt->Halt();
				break;
			}
		break;
		case PageFaultException:	//Practica 2-- Paso 3.------------------------------------------
			PageFaultExceptionHandler();
		break;				//--------------------------------------------------------------
		default:
			printf("Unexpected user mode exception. Excception:%d. Type: %d.\n", which, type);
			ASSERT(FALSE);
		break;
	}
}

void PageFaultExceptionHandler()
{
	int PhysAddr, LogAddr, p, d, pageFrame;

	//pageFrame=machine->ReadRegister(BadVAddrReg)/PageSize;		//Obtencion del Num de Marco
	pageFrame=stats->numPageFaults;						//Obtencion del Num de Marco v2
	stats->numPageFaults++;							//Inc. cont. Fallo dde pagina
	if(!strcmp(stats->op, "-M"))
	{
		printf("Num. Page Fault: %d\n", stats->numPageFaults);
	}
	LogAddr=machine->ReadRegister(BadVAddrReg);				//Obtencion de Dir. Log.
	p=(unsigned)LogAddr/PageSize;						//Calculo del Num. de Pagina
	d=(unsigned)LogAddr%PageSize;						//Calculo del Desplazamiento
	if(!strcmp(stats->op, "-F"))
	{
		printf("%d, ", p);
	}
	//if(LogAddr<MemorySize)						//Si es una direccion valida
	if(currentThread->space->pageFaults < NumPhysPages)//machine->NumPhysPages)
	{
		PhysAddr=(pageFrame*PageSize)+d;
		currentThread->space->EnablePage(currentThread->space->pageFaults, p);
		machine->Run();
	}
	else
	{
		if(!strcmp(stats->alg,"FIFO"))
		{
			currentThread->space->algFIFO(p);
		}
		else
		{
			if(!strcmp(stats->op,"-D"))
			{
				interrupt->Halt();
			}
		}
	}

//	else
	{
	//machine->Run();
//		interrupt->Halt();	//temp/deb----------------------------
	}

}

