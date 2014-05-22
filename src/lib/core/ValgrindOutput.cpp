/*****************************************************
             PROJECT  : MATT
             VERSION  : 0.1.0-dev
             DATE     : 01/2014
             AUTHOR   : Valat Sébastien
             LICENSE  : CeCILL-C
*****************************************************/

/********************  HEADERS  *********************/
#include <cstring>
#include <fstream>
#include <iostream>
#include <cstring>
#include "ValgrindOutput.hpp"
#include <portability/OS.hpp>

using namespace std;

/********************  MACROS  **********************/
#define LINE_BREAK "\n"

namespace MATT
{

/*******************  FUNCTION  *********************/
void ValgrindOutput::pushStackInfo(const MATT::Stack& stack, const MATT::CallStackInfo& info, const MATT::SymbolResolver& symbols)
{
	int shift = 0;
	
	//get addresses
	void * leafCalleePtr = stack.getCallee();
	
	//shift if operator new
	while (isNewOperator(symbols,leafCalleePtr))
		leafCalleePtr = stack[++shift];
	
	//search function info in caller map and reduce data
	ValgrindCaller & funcInfo = callers[leafCalleePtr];
	funcInfo.info.push(info);
	
	//if as caller/callee, register inclusive costs
	for (int i = 1 + shift ; i < stack.getSize() ; i++)
	{
		//extrace callee/caller
		void * callerPtr = stack[i];
		void * calleePtr = stack[i-1];
		
		//get location info
		const CallSite * callerSite = symbols.getCallSiteInfo(callerPtr);
		const CallSite * calleeSite = symbols.getCallSiteInfo(calleePtr);
		
		//check if already done to avoid accounting multipl times on recursive calls
		bool alreadySeenLink = false;
		for (int j = 1 ; j < i ; j++)
		{
			if (callerPtr == stack[j] || symbols.isSameFuntion(callerSite,stack[j]))
			{
				//check links
				if (calleePtr == stack[j-1] || symbols.isSameFuntion(calleeSite,stack[j-1]))
				{
					alreadySeenLink = true;
				}
			}
		}

		//check if has info
		if (!alreadySeenLink)
		{
			//search info in map
			ValgrindCaller & callerInfo = callers[callerPtr];

			//cumulate on callee
			CallStackInfo & calleeInfo = callerInfo.callees[calleePtr];
			calleeInfo.push(info);
		}
	}
}

/*******************  FUNCTION  *********************/
void ValgrindOutput::pushStackInfo(SimpleCallStackNode& stackNode,const SymbolResolver & symbols)
{
	pushStackInfo(stackNode.getCallStack(),stackNode.getInfo(),symbols);
}

/*******************  FUNCTION  *********************/
bool ValgrindOutput::isNewOperator(const SymbolResolver& symbols, void* addr)
{
	const CallSite * leafInfo = symbols.getCallSiteInfo(addr);
	if (leafInfo == NULL)
		return false;

	const std::string & symbol = symbols.getString(leafInfo->function);
	return (strncmp("_Znw",symbol.c_str(),4) == 0 || strncmp("_Zna",symbol.c_str(),4) == 0);
}


/*******************  FUNCTION  *********************/
void ValgrindOutput::writeAsCallgrind(const std::string& filename, const SymbolResolver & dic)
{
	ofstream out;
	out.open(filename.c_str());
	writeAsCallgrind(out,dic);
	out.close();
}

/*******************  FUNCTION  *********************/
void ValgrindOutput::writeLocation(ostream& out, const SymbolResolver& dic, const CallSite * site, void * addr, bool call)
{
	const char * callPrefix = "";
	if (call)
		callPrefix = "c";
	
	//object
	if (site == NULL || site->mapEntry == NULL)
		out << callPrefix << "ob=unknown" << LINE_BREAK;
	else
		out << callPrefix << "ob=" << site->mapEntry->file << LINE_BREAK;
	
	//file
	if (site == NULL || site->file == -1)
		out << callPrefix << "fi=unknown" << LINE_BREAK;
	else
		out << callPrefix << "fl=" << dic.getString(site->file) << LINE_BREAK;
	
	//function
	if (site == NULL || site->function == -1)
		out << callPrefix << "fn=" << addr << LINE_BREAK;
	else
		out << callPrefix << "fn=" << dic.getString(site->function) << LINE_BREAK;
}

/*******************  FUNCTION  *********************/
void ValgrindOutput::writeAsCallgrind(ostream& out, const SymbolResolver& dic)
{
	int line;
	
	//header
	out << "version: 1" << LINE_BREAK;
	out << "creator: ATT-0.0.0" << LINE_BREAK;
	out << "pid: " << OS::getPID() << LINE_BREAK;
	out << "cmd: " << OS::getExeName() << LINE_BREAK;
	out << "part: 1" << LINE_BREAK;
	out << LINE_BREAK;
	out << "desc: I1 cache: " << LINE_BREAK;
	out << "desc: D1 cache: " << LINE_BREAK;
	out << "desc: LL cache: " << LINE_BREAK;
	out << LINE_BREAK;
	out << "desc: Trigger: Program termination" << LINE_BREAK;
	out << LINE_BREAK;
	out << "positions: line" << LINE_BREAK;
	CallStackInfo::writeCallgrindEventDef(out);
	out << LINE_BREAK;
	

	//loop on data
	for (ValgrindCallerMap::const_iterator it = callers.begin() ; it != callers.end() ; ++it)
	{
		//serch call site info
		const CallSite * info = dic.getCallSiteInfo(it->first);
		
		//location
		writeLocation(out,dic,info,it->first,false);
		if (info == NULL || info->line == -1)
			line = 0;
		else
			line = info->line;

		//local values
		it->second.info.writeAsCallgrindEntry(line,out);
		out << LINE_BREAK;

		//childs
		for (ValgrindCalleeMap::const_iterator itChild = it->second.callees.begin() ; itChild != it->second.callees.end() ; ++itChild)
		{
			const CallSite * infoChild = dic.getCallSiteInfo(itChild->first);
			writeLocation(out,dic,infoChild,itChild->first,true);
			if (infoChild == NULL || infoChild->line == -1)
				out << "calls=" << itChild->second.free.count + itChild->second.alloc.count + itChild->second.cntZeros << " 0" << LINE_BREAK;
			else
				out << "calls=" << itChild->second.free.count + itChild->second.alloc.count + itChild->second.cntZeros << ' ' << infoChild->line << LINE_BREAK;
			itChild->second.writeAsCallgrindEntry(line,out);
			out << LINE_BREAK;
		}
		out << LINE_BREAK;
	}
}

}