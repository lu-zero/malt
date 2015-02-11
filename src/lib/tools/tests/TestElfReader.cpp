/*****************************************************
             PROJECT  : MATT
             VERSION  : 0.1.0-dev
             DATE     : 02/2015
             AUTHOR   : Valat Sébastien
             LICENSE  : CeCILL-C
*****************************************************/

/********************  HEADERS  *********************/
#include <gtest/gtest.h>
#include <tools/ELFReader.hpp>
#include <allocators/SimpleAllocator.hpp>

/***************** USING NAMESPACE ******************/
using namespace MATT;

/********************* GLOBALS **********************/
static const char CST_BINARY_FILE[] = TEST_BIN_DIR "/../../tests/simple-case-no-finstr"; 

/*******************  FUNCTION  *********************/
TEST(ElfReader,constructor)
{
	ElfReader reader(CST_BINARY_FILE);
}

/*******************  FUNCTION  *********************/
bool hasVariable(ElfGlobalVariableVector & vars,std::string name,size_t size,bool tls)
{
	for (ElfGlobalVariableVector::const_iterator it = vars.begin() ; it != vars.end() ; ++it)
	{
		//fprintf(stderr,"%s -- %lu -- %d\n",it->name.c_str(),it->size,it->tls);
		if (it->name == name && it->size == size && it->tls == tls)
			return true;
	}
	
	return false;
}

/*******************  FUNCTION  *********************/
TEST(ElfReader,loadSimpleCaseGlobVars)
{
	ElfReader reader(CST_BINARY_FILE);
	ElfGlobalVariableVector vars;
	reader.loadGlobalVariables(vars);
	
	if (ElfReader::hasLibElf())
	{
		EXPECT_TRUE(hasVariable(vars,"gblArray",1024 * sizeof(int),false));
		EXPECT_TRUE(hasVariable(vars,"_ZL14gblStaticArray",1024 * sizeof(int),false));
		EXPECT_TRUE(hasVariable(vars,"_ZL9gblString",25,false));
		EXPECT_TRUE(hasVariable(vars,"tlsArray",1024 * sizeof(int),true));
	}
}