//

#define MAX_LOADSTRING 100

#include <map>
#include <string>
#include <iostream>
#include <cstdio>
#include <assert.h>
#include <direct.h>

#define FAILRET(x) assert(x);//if(!(x)) return 0;

using namespace std;

struct Generator {

    typedef map<string, string> RepMap;
	typedef map<string, string>::iterator RMI;


	struct  CodeTemplate 
    {

	protected:
		RepMap      reps;
		string      txt;

	public:

		CodeTemplate( const char * buffer=0 ) : txt(buffer) {}

		bool addRep(const char * key, const char * value ) {
			reps.insert( make_pair( key, value ) );
			return true;		
		}		
		bool clear() {
			reps.clear();
			return true;		
		}		
		const char * compile( char *t=0 ) {
			static string s;
            s = ( t ? t : txt );

            for ( RMI it = reps.begin(); it != reps.end(); ++it ) {
    			const string & key  = it->first;
  				const string & val  = it->second;
  				while ( s.find(key) < s.length() )
   	               	s.replace( s.find(key), key.length(), val );
            }
			return s.c_str();		
		}				
	};

    static bool action( const string & inDir, const string & outDir, const string & ext, const string & name ) 
    {
        string inname = inDir + "xxxx" + ext;
        FILE *in = fopen( inname.c_str(), "rb" );
        bool ok = false;
        if ( in )
        {
            char si[0xffff] = {0};      
            fread( si, 0xffff,1,in );
            fclose(in);

            CodeTemplate *ct = new CodeTemplate(si);
            ct->addRep( "$xxxx$", name.c_str() );
            const char * so = ct->compile();

            string outname= outDir + name + ext;
            FILE *of = fopen( outname.c_str(), "wb" );
            if ( of )
            {
                fwrite( so, strlen(so),1,of );
                fclose(of);
                ok = true;
            }
			else
			{
				printf(  string( outname + " not created !\n" ).c_str() );
			}
			delete ct;
        }
        else
        {
            printf(  string( inname + " not found !\n" ).c_str() );
        }
        return ok;
    }
};






bool GenerateModule( const string & inDir, const string & outDir, const string & name )
{
    // write files:
    mkdir( outDir.c_str() );

    FAILRET( Generator::action( inDir , outDir, ".h", name) );
    FAILRET( Generator::action( inDir , outDir, ".cpp", name) );
    return 1;
}

bool GenerateProject( const string & inDir, const string & outDir, const string & name )
{
    // write files:
    mkdir( outDir.c_str() );
    FAILRET( Generator::action( inDir , outDir , ".vcproj", name) );
    return 1;
}




int main( int argc, char **argv)
{
    string idir = "./templates/";
    string odir = "./";
    string name = "newMod";
    int mode = 0;
    
    for ( int a=1; a<argc; a++ )
    {
        if ( argv[a][0] == '-' )
        {
            if ( argv[a][1] == 'i' )
            {
                idir = argv[++a];
            }
            else
            if ( argv[a][1] == 'o' )
            {
                odir = argv[++a];
                odir += "/";
            }
        }
        name = argv[a];
    }
    if ( odir == "./" )
    {
        odir = name;
        odir += "/";
    }
    switch( mode )
    {
        case 0:
            GenerateModule ( idir + "mod/", "src/" + odir, name );
            GenerateProject( idir + "mod/", "vc/" + odir, name );
			break;
    }
	return 0;
}

