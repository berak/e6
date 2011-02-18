#include "e6_serialize.h"
#include "e6_impl.h"
#include <map>


namespace e6
{
	namespace Serialize
	{
		struct CStream
		    : Class<Serialize::Stream, CStream>
		{		
			typedef struct V {char k[128] , v[128]; };
			typedef std::vector< V > PV;
			typedef PV::iterator PVIt;

			PV params;

			FILE * prefs;
			CStream() : prefs(0) {}
			virtual ~CStream() {close();}

			bool push( const char * k, const char * v )
			{
				V p;
				strcpy( p.k, k );
				strcpy( p.v, v );
				params.push_back( p );
			}
			const char * get( const char * k )
			{
				for ( int i=0; i<params.size(); i++ )
				{
					if ( ! strcmp( params[i].k, k ) )
					{
						return params[i].v;
					}
				}
				return 0;
			}

		    virtual bool read( Serialize::Client & ph )
			{
			    if ( ! prefs ) 
			    {
			        return 0;
			    }
			    if ( feof( prefs ) ) 
			    {
			        return 0;
			    }

			    char buf[500];
			    char key[200];
			    char value[400];
				char *line;
				while ( fgets( buf, 499, prefs ) )
			    {
					line = buf;
					while ( line[0] == ' ' || line[0] == '\t'  )
						line ++;

			        if ( line[0] == '\n' ) 
			        {
						ph.read( *this );
						params.clear();
						return 1;
			        } 

					if ( line[0] == '#' ) 
			        {
			            continue;
			        } 

					// nuke whitespace at end of line:
					size_t l = strlen(line);
					while (--l)
					{
						if ( line[l] == '\n' || line[l] == '\r' || line[l] == '\t' || line[l] == ' ' )
						{
							line[l] = 0;
							continue;
						}
						break;
					}

					if ( char * end = strchr( line, '=') )
			        {
			            *end = 0;
			            strcpy( key, line );
			            strcpy( value , (end+1) );
			            push(  key , value );
			        }
			    }
			    return 0;
			}
	    	virtual bool write( Serialize::Client & ph, const char *nodeName ) 
			{
			    if ( ! prefs ) 
			    {
			        return 0;
			    }

				fprintf( prefs, "#%s\n", nodeName );

				ph.write( *this );

				fprintf( prefs, "\n" );

				return 1;
			}
		    virtual bool open( const char *fileName, const char *mode ) 
			{
			    prefs = fopen( fileName, mode );
			    if ( ! prefs ) 
			    {
			        return 0;
			    }
			    return 1;
			}
		    virtual bool close() 
			{
			    if ( ! prefs ) 
			    {
			        return 0;
			    }
				fclose(prefs);
			    return 1;
			}

		//struct Writer
			virtual bool setInt( const char * name, int i ) 
			{
			    fprintf( prefs, "%s=%i\n", name, i );
			    fflush( prefs );
				return 1;
			}
			virtual bool setFloat( const char * name, float f ) 
			{
			    fprintf( prefs, "%s=%f\n", name, f );
			    fflush( prefs );
				return 1;
			}
			virtual bool setFloat3( const char * name, float3 & f ) 
			{
			    fprintf( prefs, "%s=%4.4f %4.4f %4.4f\n", name, f[0],f[1],f[2] );
			    fflush( prefs );
				return 1;
			}
			virtual bool setFloat4( const char * name, float4 & f ) 
			{
			    fprintf( prefs, "%s=%4.4f %4.4f %4.4f %4.4f\n", name, f[0],f[1],f[2],f[3] );
			    fflush( prefs );
				return 1;
			}
			virtual bool setString( const char * name, const char * value ) 
			{
			    fprintf( prefs, "%s=%s\n", name, value );
			    fflush( prefs );
				return 1;
			}
			virtual bool setPointer( const char * name, const void * value, uint numBytes ) 
			{
				return 0;
			}
		//struct Reader
			virtual int   getInt( const char * name ) 
			{
				int v=0;
				const char * value = get(name);
				if ( ! value ) return 0;
				sscanf( value, "%i", &v );
				return v;
			}
			virtual float getFloat( const char * name ) 
			{
				float v=0;
				const char * value = get(name);
				if ( ! value ) return 0;
				sscanf( value, "%f", &v );
				return v;
			}
			virtual float3 getFloat3( const char * name ) 
			{
				float3 v=0;
				const char * value = get(name);
				if ( ! value ) return 0;
				sscanf( value, "%f %f %f", &v[0], &v[1], &v[2] );
				return v;
			}
			virtual float4 getFloat4( const char * name ) 
			{
				float4 v=0;
				const char * value = get(name);
				if ( ! value ) return 0;
				sscanf( value, "%f %f %f %f", &v[0], &v[1], &v[2], &v[3] );
				return v;
			}
			virtual const char * getString( const char * name ) 
			{
				const char * value = get(name);
				return value;
			}
			virtual bool getPointer( const char * name, void ** value, uint &numBytes ) 
			{
				return 0;
			}
		}; // CStream

	} //namespace Serialize

} //namespace e6

