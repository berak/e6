//#include "../e6/e6.h"
//#include "../e6/e6_impl.h"
//#include "../e6/e6_enums.h"
#include "../Video/Video.h"
#include <windows.h>
#include <stdio.h>



using e6::uint;




int main(int argc, char **argv)
{
	int frame = 0;
	e6::Engine * e = e6::CreateEngine();	

	Video::ArtDetector * art = (Video::ArtDetector *) e->createInterface( "Video", "Video.ArtDetector" );

	if ( art )
	{
		const char * here = e->getPath();
		char path[200];
		sprintf(path,"%s/res/pat/patt.kanji",here );
		art->addPattern( path );
		sprintf(path,"%s/res/pat/patt.hiro",here );
		art->addPattern( path );
		sprintf(path,"%s/res/pat/patt.my",here );
		art->addPattern( path );

		art->setThreshold( 60 );
		if ( art->start() )
		{
			// allow some time for the start of video capture:
			Sleep(300);
			
			while( 1 )
			{
				//printf( "<art %d : %d>\n", frame, n );
				for ( uint i=0; i<art->numPatterns(); i++ )
				{
					if ( art->getPatternLost(i) )
						continue;

					printf( "%d : <%d %3.2f %3.2f>\n", 
						frame, i,
						art->getPatternX(i),
						art->getPatternY(i)
						);
					e6::print( art->getPatternRot(i) );
				}
				frame ++;
				Sleep(20);
			}
		}
	}
	E_RELEASE(art);
	E_RELEASE(e);	
	return 0;
}
