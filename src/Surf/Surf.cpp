//#include "e6.h"
#include "../e6/e6_impl.h"
#include "../e6/e6_math.h"

#include "Surf.h"


using e6::uint;
using e6::ClassInfo;



extern ClassInfo ClassInfo_Marching;
extern ClassInfo ClassInfo_HeightField;

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		ClassInfo_Marching,
		ClassInfo_HeightField,
		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 2; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ( "Surf 00.00.1 (" __DATE__ ")" );
	mv->e6Version =	e6::e6_version;
	return 1; 
}
