
#include "../e6/e6_impl.h"
#include "$xxxx$.h"



using e6::uint;
using e6::ClassInfo;


namespace $xxxx$
{
	struct C$xxxx$
		: public e6::CName< $xxxx$, C$xxxx$ >
	{
		//~ virtual void f0() 
		//~ {
			//~ $();
		//~ }
	};

};

extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"$xxxx$.$xxxx$",	 "$xxxx$",	$xxxx$::C$xxxx$::createInterface, $xxxx$::C$xxxx$::classRef	},
		{	0, 0, 0	}//,
		//0
	};

	*ptr = (ClassInfo *)_ci;

	return 1; // classses
}

#include "../e6/version.h"
extern "C"
uint getVersion( e6::ModVersion * mv )
{
	mv->modVersion = ("$xxxx$ 00.000.0023 (" __DATE__ ")");
	mv->e6Version =	e6::e6_version;
	return 1; 
}
