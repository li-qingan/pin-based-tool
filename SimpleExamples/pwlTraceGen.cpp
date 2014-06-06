/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2013 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
//
// @ORIGINAL_AUTHOR: Artur Klauser
// @EXTENDED: Rodric Rabbah (rodric@gmail.com) 
//

/*! @file
 *  This file contains an ISA-portable cache simulator
 *  data cache hierarchies
 */


#include "pin.H"

#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <sstream>
#include <assert.h>

//#define ONLY_MAIN

#define MALLOC "__libc_malloc"
#define FREE "__cfree"

#define MEM_WIDTH_POWER 2     // 2^n bytes. So 2 means 2^2=4bytes memory width, i.e., 32-bit

/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,    "pintool",
    "o", "stack.out", "specify dcache file name");
KNOB<UINT32> KnobProfDistance(KNOB_MODE_WRITEONCE, "pintool",
    "d","5", "profing distance power: n -> 2^n");



/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This tool represents a cache simulator.\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary() << endl; 
    return -1;
}

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */

//typedef int64_t INT64;

struct Object
{
	UINT64 _id;
	UINT32 _nSize;
	ADDRINT _nStartAddr;
	std::map<UINT32, UINT64> _hOffset2W;   // assuming MEM_WIDTH data bus; store write stats for each offset
	
	Object(UINT64 id, UINT32 nSize)
	{
		_id = id; _nSize = nSize;
	}
	
	Object(Object *obj)
	{
		_id = obj->_id;
		_nSize = obj->_nSize;
	}
};
struct LifeElement
{
	struct Object *_obj;
	bool _begin;      // TRUE for begin; FALSE for end
	
	LifeElement(Object *obj, bool begin) 
	{
		_obj = obj;
		_begin = begin;
	}
};

UINT32 g_nProfDistPower = 5;
std::ofstream g_outFile;



UINT64 g_nID = 0;		  // unique ID for each frame
//////////////////////////////////////////////////////
//////// stack, heap and objects /////////////////////
std::list<Object *> g_FrameStack;   // to match end and begin
//std::list<ADDRINT> g_RetStack; // stack for return addresses

std::list<Object *> g_HeapStack;  // to associate heap size with heap starting address
std::map<ADDRINT, Object *> g_HeapMap; // the map of starting address to heap objects, to match end and begin

std::map<ADDRINT, UINT32> g_GlobalTmp; // <address, size> of a global data
std::map<ADDRINT, Object *> g_GlobalMap; // to match end and begin
//////////////////////////////////////////////////////
//////// Auxiliary variables /////////////////////////
ADDRINT g_nEndofImage = 0;
bool g_bInStack = false;


std::list<LifeElement> g_AllocTrace; 
std::map<ADDRINT, UINT32> g_hFunc2StackSize; // map function address to stack size

/* ===================================================================== */
/* ===================Auxiliary routines============================== */
/* ===================================================================== */
Object *SearchObjectByAddr(ADDRINT addr)
{
	//cerr << "begin of search" << endl;
	Object *obj = NULL;
	bool bFound = false;	
	if (addr < g_nEndofImage )
	{
		std::map<ADDRINT, Object *>::iterator I = g_GlobalMap.begin(), E = g_GlobalMap.end();		
		for(; I != E; ++ I )
		{
			obj = I->second;
			if( addr >= obj->_nStartAddr && addr < obj->_nStartAddr + obj->_nSize)
			{
				bFound = true;
				break;
			}
		}
	}
	else
	{
		std::map<ADDRINT, Object *>::iterator I = g_HeapMap.begin(), E = g_HeapMap.end();
		for( ; I != E; ++ I )
		{
			obj = I->second;
			if( addr >= obj->_nStartAddr && addr < obj->_nStartAddr + obj->_nSize)
			{
				bFound = true;
				break;
			}
		}
	}
	//cerr << "end of search" << endl;
	if( !bFound )
		return NULL;
	return obj;
}

/* ===================================================================== */
/* =======Analysis routines to identify the life of frame, heap and globals=========== */
/* ===================================================================== */
/* VOID FrameBegin(ADDRINT nextAddr, ADDRINT callee )
{	
	//DumpSpace(g_instanceStack.size());	
	// 1. recording the frame start <
	++ g_nID;	
	//g_hFrame2Addr[g_nID] = calleeAddr;    // to record the map from frame id to function address
	UINT32 nFrameSize = g_hFunc2StackSize[calleeAddr];	
	Object *obj = new Object(g_nID, nFrameSize);	
	g_AllocTrace.push_back(LifeElement(obj, true)); 	
	
	// prepare to identify the matching > via stack mechanism
	g_RetStack.push_back(nextAddr);
	g_FrameStack.push_back(obj);		
	
	// 2.1 recording the heap start <, if any
	if( szFunc == MALLOC )
	{
		g_hHeap2Size[g_nID] = nHeapSize;
		Object *obj = new Object(g_nID, nHeapSize);	
		g_AllocTrace.push_back(LifeElement(obj, true) );
		// prepare to identify the matching > via stack mechanism
		g_HeapList[nStartAddr] = obj;		
	}
	// 2.2 recording the heap end >
	else if( szFunc == FREE )
	{
		assert(g_HeapList.find(nStartAddr) != g_HeapList.end());
		Object *obj = g_HeapList[nStartAddr];
		g_AllocTrace.push_back(LifeElement(obj, false) );
		// erase the expired object
		g_HeapList.erase(nStartAddr);		
	}
}

VOID FrameEnd(ADDRINT iAddr )
{
	// ???? possible bug ???
	assert(!g_RetStack.empty());
	if( g_RetStack.empty() )
		return;
	
	if( iAddr == g_RetStack.back() )  // to identify a function return address
	{
		g_AllocTrace.push_back(LifeElement(g_FrameStack.back(), false) );

		// erase the expired object
		g_FrameStack.pop_back();
		g_RetStack.pop_back();
	}
} */
VOID FrameBegin(ADDRINT funcAddr, ADDRINT nStartAddr)
{
	
	++ g_nID;
	cerr <<hex <<g_nID << "<<" << funcAddr << endl;
	UINT32 nFrameSize = g_hFunc2StackSize[funcAddr];	
	Object *obj = new Object(g_nID, nFrameSize);	
	obj->_nStartAddr = nStartAddr;
	g_AllocTrace.push_back(LifeElement(obj, true));
	// prepare to match
	g_FrameStack.push_back(obj);
	g_bInStack = true;
}
VOID FrameEnd(ADDRINT funcAddr)
{   
	cerr <<hex << ">>" << funcAddr << endl;
	Object *obj = g_FrameStack.back();
	//<<debug
	UINT32 nFrameSize = g_hFunc2StackSize[funcAddr];
	assert(nFrameSize == obj->_nSize);
	//>>debug
	g_AllocTrace.push_back(LifeElement(obj,false));
	// erase the expired object
	g_FrameStack.pop_back();
	g_bInStack = false;
}
VOID HeapBeginArg(UINT32 nHeapSize)
{
	++ g_nID;
	Object *obj = new Object(g_nID, nHeapSize);
	g_AllocTrace.push_back(LifeElement(obj, true) );
	// prepare to match
	g_HeapStack.push_back(obj);
}
VOID HeapBeginRet(ADDRINT nStartAddr)
{
	Object *obj = g_HeapStack.back();
	obj->_nStartAddr = nStartAddr;
	g_HeapMap[nStartAddr] = obj;
}

VOID HeapEnd(ADDRINT nStartAddr)
{
	Object *obj = g_HeapMap[nStartAddr];
	g_AllocTrace.push_back(LifeElement(obj, false));
	// erase expired object
	g_HeapMap.erase(nStartAddr);
}

VOID GlobalBegin()
{
    // read global and static symbols via libelf and libdwarf, to fill g_GlobalTmp !!!
	ifstream symFile;
	symFile.open("symbol.txt");
	string szLine;
	while(symFile.good())
	{
		getline(symFile, szLine);
		if( szLine.size() < 2)
			continue;
		ADDRINT addr;
		UINT32 size;
		stringstream ss(szLine);
		ss >> addr >> size;
		g_GlobalTmp[addr]=size;		
	}
	symFile.close();
	
	// traverse g_GlobalTmp to collect global and static objects
	std::map<ADDRINT, UINT32>::iterator I = g_GlobalTmp.begin(), E = g_GlobalTmp.end();
	for(; I != E; ++ I )
	{
		++ g_nID;
		Object *obj = new Object(g_nID, I->second);
		obj->_nStartAddr = I->first;
		g_AllocTrace.push_back(LifeElement(obj, true) );	
		// prepare to match
		g_GlobalMap[I->first] = obj;
	}	
}
VOID GlobalEnd()
{
	std::map<ADDRINT, UINT32>::iterator I = g_GlobalTmp.begin(), E = g_GlobalTmp.end();
	for(; I != E; ++ I )
	{
		Object *obj = g_GlobalMap[I->first];
		g_AllocTrace.push_back(LifeElement(obj, false) );
		// erase expired objects
		g_GlobalMap.erase(I->first);
	}	
}
/* ===================================================================== */
/* =======Analysis routines to get write stats for each object=========== */
/* ===================================================================== */
// obtain write stats for frame objects
VOID StackSingle(ADDRINT addr, UINT32 nOffset)
{	
	//cerr << hex << "store in offset: " << nOffset << endl;
	if( !g_bInStack )
		return;
	Object *obj = g_FrameStack.back();
	UINT32 alignedOffset = nOffset - nOffset%(1<<MEM_WIDTH_POWER);
	++ obj->_hOffset2W[alignedOffset];	
}
VOID StackMulti(ADDRINT addr, UINT32 nOffset, UINT32 nSize)
{
	Object *obj = g_FrameStack.back();
	UINT32 nOffset2 = nOffset + nSize-1;
	UINT32 alignedOff1 = nOffset - nOffset%(1<<MEM_WIDTH_POWER);	
	UINT32 alignedOff2 = nOffset2 - nOffset2%(1<<MEM_WIDTH_POWER);
	for( UINT32 i = alignedOff1; i <= alignedOff2; ++ i)
		++ obj->_hOffset2W[i];	
}
// obtain write stats for heap and global objects
VOID StoreSingle(ADDRINT addr)
{
	//cerr << hex << "store in address: " << addr << endl;
	Object *obj = SearchObjectByAddr(addr);
	// ???ignoring some addresses for dynamic linking insdead of for global data, such as 0x8048a84 <_dl_use_load_bias>
	if(obj == NULL )
		return;
	UINT32 nOffset = addr - obj->_nStartAddr;
	UINT32 alignedOffset = nOffset - nOffset%(1<<MEM_WIDTH_POWER);
	++ obj->_hOffset2W[alignedOffset];	
}
VOID StoreMulti(ADDRINT addr, UINT32 nSize)
{
	Object *obj = SearchObjectByAddr(addr);
	UINT32 nOffset = addr - obj->_nStartAddr;
	UINT32 nOffset2 = nOffset + nSize-1;
	UINT32 alignedOff1 = nOffset - nOffset%(1<<MEM_WIDTH_POWER);	
	UINT32 alignedOff2 = nOffset2 - nOffset2%(1<<MEM_WIDTH_POWER);
	for( UINT32 i = alignedOff1; i <= alignedOff2; ++ i)
		++ obj->_hOffset2W[i];	
}
/* ===================================================================== */
VOID Image(IMG img, void *v)
{	
	g_nEndofImage = IMG_HighAddress(img);
	for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec) )
	{
		for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn) )
		{
			RTN_Open(rtn);
			string szFunc = RTN_Name(rtn);
			//1. Instrument each function for frame objects
			//ADDRINT funcAddr = RTN_Address(rtn);
			//RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR) FrameBegin,
			//					IARG_ADDRINT, funcAddr,
			//					IARG_END);
			//RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR) FrameEnd,
			//					IARG_ADDRINT, funcAddr,
			//					IARG_END);
			//1. Instrument malloc/free functions for heap objects
			if( RTN_Name(rtn) == MALLOC )
			{				
				RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR) HeapBeginArg,
								//IARG_ADDRINT, MALLOC,
								IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
								IARG_END);
				RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)HeapBeginRet,
								IARG_FUNCRET_EXITPOINT_VALUE, IARG_END);				
			}
			else if( RTN_Name(rtn) == FREE )
			{
				RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR) HeapEnd,
								IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
								IARG_END);
			}
			//2. Search each function for stack size
			// collect stack size via "SUB 0x20, %esp" or "ADD 0xffffffe0, %esp"
			ADDRINT fAddr = RTN_Address(rtn);				
			//g_hAddr2Func[fAddr] = szFunc;
			//cerr << fAddr << ":\t" << szFunc << endl;			
			bool bNormal1 = false, bNormal2 = false;
			INS ins1;
			std::list<INS> ins2;
			for(INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins) )
			{
			// note that, a function may have one entry (sub) but multiple exits (add)				
				if( INS_Opcode(ins) == XED_ICLASS_SUB && 
					INS_OperandReg(ins, 0) == REG_STACK_PTR  &&
					INS_OperandIsImmediate(ins, 1) )
				{		                         
					UINT32 nOffset = INS_OperandImmediate(ins, 1);					         	
					g_hFunc2StackSize[fAddr] = nOffset;	
					bNormal1 = true;    
					ins1 = ins;					                 
				}
				else if( INS_Opcode(ins) == XED_ICLASS_ADD && 
					INS_OperandReg(ins, 0) == REG_STACK_PTR  &&
					INS_OperandIsImmediate(ins, 1) )
				{		              
					bNormal2 = true;     
					ins2.push_back(ins); 					                     
				}		
			}	
			
			// 3.1 instrument frame allocation and deallocation
			if( bNormal1 && bNormal2 )
			{
				//cerr << hex <<"=" << fAddr << INS_Disassemble(ins1) << "-" << INS_Disassemble(*ins2.begin()) << endl;
				INS_InsertPredicatedCall(
					ins1, IPOINT_BEFORE,  (AFUNPTR) FrameBegin,
					IARG_ADDRINT, fAddr,
					IARG_END);   
				std::list<INS>::iterator i_p = ins2.begin(), i_e = ins2.end();
				for(; i_p != i_e; ++ i_p )
					INS_InsertPredicatedCall(
						*i_p, IPOINT_BEFORE,  (AFUNPTR) FrameEnd,
						IARG_ADDRINT, fAddr,
						IARG_END); 	
			}
			RTN_Close(rtn);
			// ignoring function stacks without "sub esp, 0x4", which appears for small library functions, like _start, etc
			// the skip is somehow reasonable, since these frames are small and not write-intensive
			if( !bNormal1 || !bNormal2)	
				continue;
			RTN_Open(rtn);
			// 4. to instrument write operations, only considering function with normal stack
			for(INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins) )
			{				
				if ( INS_IsStackWrite(ins) )
				{		
					// skip call instruction, like: call   0 <__libc_tsd_LOCALE>, which has 
					if( INS_Opcode(ins) == XED_ICLASS_CALL_NEAR )
								;
					// skip: push 0x0
					else if( INS_MemoryBaseReg(ins) != REG_STACK_PTR )
					{	
						//cerr << INS_Disassemble(ins) << "not esp" << endl;
						//assert(INS_MemoryBaseReg(ins) == REG_STACK_PTR);
						;
					}
					else
					{
						INT32 disp = INS_MemoryDisplacement(ins);
						if( disp < 0)
						{							
							cerr <<disp << " in function " << szFunc << endl;
							cerr << INS_Disassemble(ins) << endl;
							assert( disp > 0);							
						}	
						else					
							INS_InsertPredicatedCall(
									ins, IPOINT_BEFORE,  (AFUNPTR) StackSingle,				
									IARG_MEMORYWRITE_EA,
									IARG_UINT32, (UINT32)disp,
									IARG_END);
					}
				}
				else if ( INS_IsMemoryWrite(ins) )
				{
					// map sparse INS addresses to dense IDs  
					INS_InsertPredicatedCall(
						ins, IPOINT_BEFORE,  (AFUNPTR) StoreSingle,
						IARG_MEMORYWRITE_EA,
						IARG_END);		
				}
			}
			
			RTN_Close(rtn);
		}
	}		
}

/* ===================================================================== */

VOID Fini(int code, VOID * v)
{
    g_nProfDistPower = KnobProfDistance.Value();
	char buf[256];
    sprintf(buf, "%u",g_nProfDistPower);
    string szOutFile = KnobOutputFile.Value() +"_" + buf;

    g_outFile.open(szOutFile.c_str());
    g_outFile << "PIN:MEMLATENCIES 1.0. 0x0\n";
            
    g_outFile <<
        "#\n"
        "# Write stats with distance size:\t";
	g_outFile << (1<<g_nProfDistPower);
	g_outFile <<	"\n\n";
	g_outFile << "Total objects count:\t" << g_nID << endl;	
	cerr << "Output" << endl;
	std::list<LifeElement>::iterator I = g_AllocTrace.begin(), E = g_AllocTrace.end();
	for(; I != E; ++ I)
	{
		Object *obj = I->_obj; 
		bool begin = I->_begin;
		
		// print object allocation request as well as object write stats
		g_outFile << hex << obj->_id << "\t" << obj->_nSize << "\t" << begin << endl;
		if( !begin)
			continue;
		if( !obj->_hOffset2W.empty() )
			g_outFile << "\t";
		std::map<UINT32, UINT64>::iterator J_p = obj->_hOffset2W.begin(), J_e = obj->_hOffset2W.end();
		for(; J_p != J_e; ++ J_p )
		{
			g_outFile << hex << J_p->first << ":" << J_p->second << "\t";
		}
		if( !obj->_hOffset2W.empty() )
			g_outFile << endl;
	}
    g_outFile.close();
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    PIN_InitSymbols();

    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
	
    
	IMG_AddInstrumentFunction(Image, 0);

    PIN_AddFiniFunction(Fini, 0);

    // Never returns	
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
