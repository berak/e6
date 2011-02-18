#include "../e6/e6_impl.h"
#include "../e6/e6_sys.h"
#include "../e6/e6_enums.h"
#include "../Core/Core.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h> // FILE



using e6::uint;
using e6::ClassInfo;


namespace I3ds
{

	struct CImporter 
		: public e6::Class< Core::Importer, CImporter >
	{
	    struct Map3ds
	    {
		    char filename[256];
		    short int type,amount,options;
		    float filtering;
		    float u_scale,v_scale,u_offset,v_offset;
		    float rotation;
		    int piclibindx;
	    };

	
	    struct Mat3ds
	    {
		    float reflection;
		    float ambient[4];
		    float diffuse[4];
		    float specular[4];
		    float shininess;
		    float shininess_strength;
		    float transparency;
		    float transparency_falloff;
		    float reflect_blur;
		    short int material_type;
		    float self_illum;
	
		    char name[256];
	
		    Map3ds 
			    map_texture1,map_texture2,
			    map_opacity,map_bump,map_specular,
			    map_shininess,map_selfillum,map_reflection;

		    Mat3ds()
		    {
			    memset(this,0,sizeof(Mat3ds));
		    }

		    int l;
	    };

		FILE *fp;
		char *chunk;
		int chunklen;
		unsigned short chunkid;
		int filepos,nmat,mmat;
		Mat3ds *matlib;
		float masterscale,scale;

		e6::Engine *engine;
	    Core::World * world;
		Core::Node * root;
	
		CImporter()
		{
			chunk=0; chunklen=0; nmat=0; mmat=0; matlib=0; masterscale=1.0f; scale=1.0; filepos=0;
		}
		~CImporter()
		{
			if (chunk) delete chunk; 
			if (matlib) delete matlib; 
		}

		void mat34to16(float in[], float out[])
		{   // no-swap, no invert !
		    out[0] = in[0];    out[1] = in[1];    out[2] = in[2];
		    out[4] = in[3];    out[5] = in[4];    out[6] = in[5];
		    out[8] = in[6];    out[9] = in[7];    out[10]= in[8];
		    // swap, invert !
		    out[12]= in[9];    out[13]= in[11];   out[14]= -in[10];
		}


		int read_chunk()
		{
		    int i=0;
		    
		    i+=fread(&chunkid,1,2,fp);
		    i+=fread(&chunklen,1,4,fp);
		    if (i!=6)
		        return 0;
		    chunklen-=6;
		    filepos+=6;
		    
		    return 1;
		}
		
		void jump_chunk()
		{
		    fseek(fp,chunklen,SEEK_CUR);
		    filepos+=chunklen;
		}

		void load_chunk()
		{
		    if (chunk)
		        delete chunk;
		    chunk=new char[chunklen];
		    filepos+=chunklen;
		    fread(chunk,chunklen,1,fp);
		}

		void load_string(char *str)
		{
		    int a=0;
		    while( (str[a++]=fgetc(fp))!=0 )
		        ;
		    chunklen-=a;
		    filepos+=a;
		}

		void load_texture( const char * orig, Core::Mesh * mesh, uint stage )
		{
			char path[200];
			char name[200];
			if ( ! engine->chopPath( orig, path, name ) )
			{
				e6::sys::alert( __FUNCTION__, "path(%s) contains no name!", orig );
				return;
			}
			world->load( engine, name );
			Core::Texture *t = world->textures().get( name );
			if (t)
			{
				mesh->setTexture ( stage, t );
				E_RELEASE(t);
			}
		}

		void load_mesh(char *objname)
		{
		    int nvert=0,nfaces=0,ntextcoord=0,nfacematerial=0;
		    char *vert=0,*faces=0,*textcoord=0,*facematerial=0;
		    float local_axis[12]={ 1,0,0, 0,1,0, 0,0,1, 0,0,0 };
		    int a,fpos=filepos+chunklen;
		    while( filepos<fpos)
		    {
		        read_chunk();
		        switch(chunkid)
		        {
			        case 0x4110:
		            {
		                load_chunk();
		                nvert=*((unsigned short *)chunk);
		                vert=chunk;
		                chunk=0;
		                break;
		            }
			        case 0x4120:
		            {
		                int fpos2=filepos+chunklen;
		                fread(&nfaces,2,1,fp);
		                faces=new char[nfaces*2*4+2];
		                fread(&faces[2],nfaces*2,4,fp);
		                filepos+=nfaces*2*4+2;
		                *((unsigned short *)faces)=nfaces;
		                while( filepos<fpos2)
		                {
		                    read_chunk();
		                    switch(chunkid)
		                    {
			                    case 0x4130:
		                        {
		                            load_chunk();
		                            if (nfacematerial==0)
		                            {
		                                facematerial=chunk;
		                                chunk=0;
		                                nfacematerial=chunklen;
		                                mmat = 1;
		                            }
		                            else 
		                            {
		                                char *temp=new char[nfacematerial+chunklen];
		                                memcpy(temp,facematerial,nfacematerial);
		                                memcpy(&temp[nfacematerial],chunk,chunklen);
		                                delete facematerial;
		                                facematerial=temp;
		                                nfacematerial+=chunklen;
		                                
		                                mmat ++;
		                            }
		                            break;
		                        }
			                    default: jump_chunk();
		                    }
		                }
		                break;
		            }

			        case 0x4140:
		            {
		                load_chunk();
		                ntextcoord=*((unsigned short *)chunk);
		                textcoord=chunk;
		                chunk=0;
		            }
		            break;

			        case 0x4160:
		            {
		                load_chunk();
		                memcpy(local_axis,chunk,sizeof(float)*12);
		            }
		            break;

			        default:
			            jump_chunk();
		        }
		    }
		
		    if (nvert!=0 && nfaces!=0)
		    {
				Core::Mesh  * mesh = (Core::Mesh *)engine->createInterface("Core",  "Core.Mesh" );	
		        mesh->setName( objname );
				mesh->setPos( e6::float3( local_axis[9], local_axis[11], -local_axis[10] ) );
				root->link( mesh );
				// world->meshes().add( mesh->getName(), mesh );
				
				uint format = e6::VF_NOR;
				if( textcoord ) 
				{
					format |= e6::VF_UV0;
				}
				mesh->setup( format, nfaces );				
		        
				uint idx = 0;
				Core::VertexFeature *pos = mesh->getFeature( e6::VF_POS );
				assert( pos );
		        for ( a=0;a<nfaces;a++ ) 
				{
		            unsigned short * fi = ((unsigned short *)(2+faces+a*8));
		            for ( int i=0; i<3; i++ ) 
					{               
		                float *f=(float *)(2+vert+fi[i]*12);
						float v[3] = { f[0], f[2], -f[1] };
		                pos->setElement( idx++, v );
		            }
		        }
				E_RELEASE( pos );

		        if ( textcoord ) 
				{
					idx = 0;		
		            Core::VertexFeature *uv = mesh->getFeature( e6::VF_UV0 );
					assert( uv );
		            for ( a=0;a<nfaces;a++ ) 
					{
		                unsigned short * fi = ((unsigned short *)(2+faces+a*8));
		                for ( int i=0; i<3; i++ ) 
						{
		                    float *f = (float *)(2+textcoord+fi[i]*8);
		                    float u[2] = { f[0], f[1] }; //FIXME -y?
		                    uv->setElement( idx++, u );
		                }
		            }
					E_RELEASE( uv );
		        }
				mesh->recalcNormals( 0 );
				mesh->recalcSphere();

		        if (1)
		        {
		            a=0;
		            int b,c,d=0,len=0,start=0;
					while( a<nfacematerial ) {
		
		                for( b=0;b<nmat;b++ )
		                    if (!strcmp(matlib[b].name,&facematerial[a]))
		                        break;
		                
		                c=strlen(&facematerial[a]);
		                len=*((unsigned short *)&facematerial[a+c+1]);
		                if ( b < nmat ) {
		         //           unsigned short *mi = (unsigned short *)&facematerial[a+c+3];
		         //           for ( int i=0; i<len; i++ )
		         //               mesh->faces[mi[i]].s = d; // shader index
				

							//unsigned _cr = unsigned(matlib[b].diffuse[0] * 255.0f);   
							//unsigned _cg = unsigned(matlib[b].diffuse[1] * 255.0f);   
							//unsigned _cb = unsigned(matlib[b].diffuse[2] * 255.0f);   
							//unsigned _ca = unsigned(matlib[b].diffuse[3] * 255.0f);   
							//unsigned _cc = (_ca<<24) | (_cr<<16) | (_cg<<8) | (_cb);
							//mt->setColor( 0, _cc );
							mesh->setVertexShaderConstant( e6::VS_MATERIAL_COLOR_DIFFUSE, matlib[b].diffuse );
							char *tm = matlib[b].map_texture1.filename;
		                    if ( tm && tm[0]!=0 ) 
							{
								load_texture( tm, mesh, 0 );
		                    }
		
		                    char *lm = matlib[b].map_specular.filename;
		                    if ( lm && lm[0]!=0 ) 
							{
								load_texture( lm, mesh, 1 );
		                    }
		
		                    d++;
							start+=len;
		                }
		                a+=len*2+c+3;
		            }
		        }
				E_RELEASE( mesh );
		    }


		    if (vert)
		        delete vert;
		    if (faces)
		        delete faces;
		    if (facematerial)
		        delete facematerial;
		    if (textcoord)
		        delete textcoord;
		}

		void load_light(char *objname)
		{
			float pos[3],target[3],color[3],hotspot,falloff;
		    int type=0;
		    int fpos=filepos+chunklen;
		    fread(pos,12,1,fp);
		    filepos+=12;
		    while( filepos<fpos )
		    {
		        read_chunk();
		        switch(chunkid)
		        {
		        case 0x0010:
		            load_chunk();
		            color[0]=*((float *)&chunk[0]);
		            color[1]=*((float *)&chunk[4]);
		            color[2]=*((float *)&chunk[8]);
		            break;
		        case 0x0011:
		            load_chunk();
		            color[0]=(float)(((unsigned char)chunk[0])/255.0);
		            color[1]=(float)(((unsigned char)chunk[1])/255.0);
		            color[2]=(float)(((unsigned char)chunk[2])/255.0);
		            break;
		        case 0x4610:
		            load_chunk();
		            target[0]=*((float *)&chunk[0]);
		            target[1]=*((float *)&chunk[4]);
		            target[2]=*((float *)&chunk[8]);
		            hotspot=*((float *)&chunk[12]);
		            falloff=*((float *)&chunk[16]);
		            type=1;
		            break;
		            
		        default:
		            jump_chunk();
		        }
		    }
//		    if (type)
//		        spotlight(objname, pos, target, color, hotspot, falloff);
//		    else pointlight(objname, pos, color);
			Core::Light  * light = (Core::Light *)engine->createInterface("Core",  "Core.Light" );	
	        light->setName( objname );
			if ( type )
			{
				light->setType( e6::LT_SPOT );
				light->setDir( target );
			}
			light->setColor( color );
			root->link( light );
			//world->lights().add( light->getName(), light );
			E_RELEASE( light );
		}

		void load_object()
		{
		    char objname[256];
		    
		    load_string(objname);
			read_chunk();
		    switch(chunkid)
		    {
		    case 0x4600:
		        {
		            load_light(objname);
		            break;
		        }
		    case 0x4700:
		        {
		            load_chunk();
				    //void camera(char *name, float *pos, float *target, float bank, float lens)
		            //camera(objname, (float *)chunk,(float *)&chunk[12], *((float *)&chunk[24]), *((float *)&chunk[28]));
					Core::Camera *cam = (Core::Camera *)engine->createInterface("Core", "Core.Camera");
					root->link( cam );
					cam->setName( objname );
					e6::float3  targ = (float*)&chunk[12];
					targ.normalize();
					cam->setPos((float *)chunk);
					cam->setRot( targ );

					float fov=*((float *)&chunk[28]);
					float np=1.0f,fp=699.0f;
					cam->setFov(fov);
					cam->setNearPlane(np);
					cam->setFarPlane(fp);
					// world->cameras().add( cam->getName(), cam );
					E_RELEASE( cam );
		            break;
		        }
		    case 0x4100:
		        {
		            //if (world.findMesh(objname))
	                load_mesh(objname);
		            //else jump_chunk();
		            break;
		        }
		    default:
		        jump_chunk();
		    }
		}


		int import(  const char *name )
		{
		    fp=fopen(name,"rb");

		    if (!fp)
		        return 0;
			
			Core::Node * worldRoot = world->getRoot();
			root = (Core::FreeNode *)engine->createInterface("Core",  "Core.FreeNode" );	
			root->setName( name );
			worldRoot->link( root );


			while(read_chunk())
		    switch(chunkid)
		    {
		        case 0x4d4d: //M3DMAGIC:
		        case 0x3d3d: //MDATA:
		            break;
		        case 0x100: //MASTER_SCALE:
		            {
		                load_chunk();
		                masterscale=*((float *)chunk);
		            }
		            break;
		        case 0x4000: //NAMED_OBJECT:
		            load_object();
		            break;
		        case 0xafff: //MAT_ENTRY:
		            load_material();
		            matlib[nmat-1].ambient[3]=matlib[nmat-1].transparency;
		            matlib[nmat-1].diffuse[3]=matlib[nmat-1].transparency;
		            matlib[nmat-1].specular[3]=matlib[nmat-1].transparency;
	            break;
		        default:
		            jump_chunk();
		    }
			
			root->recalcSphere();
			E_RELEASE( worldRoot );
			E_RELEASE( root );
			    
		    fclose(fp);
		    return 1;
		}



		void load_mapping(Map3ds *m)
		{
		    int fpos=filepos+chunklen;
		    
		    m->u_scale=1.0;
		    m->v_scale=1.0;
		    m->u_offset=0.0;
		    m->v_offset=0.0;
		    m->rotation=0.0;
		    
		    while( filepos<fpos )
		    {
		        read_chunk();
		        switch(chunkid)
		        {
		        case 0x0030:
		            load_chunk();
		            m->amount=*((short int *)chunk);
		            break;
		        case 0xA300:
		            load_string(m->filename);
		            break;
		        case 0xA351:
		            load_chunk();
		            m->options=*((short int *)chunk);
		            break;
		        case 0xA353:
		            load_chunk();
		            m->filtering=*((float *)chunk);
		            break;
		        case 0xA354:
		            load_chunk();
		            m->u_scale=*((float *)chunk);
		            break;
		        case 0xA356:
		            load_chunk();
		            m->v_scale=*((float *)chunk);
		            break;
		        case 0xA358:
		            load_chunk();
		            m->u_offset=*((float *)chunk);
		            break;
		        case 0xA35A:
		            load_chunk();
		            m->v_offset=*((float *)chunk);
		            break;
		        case 0xA35C:
		            load_chunk();
		            m->rotation=*((float *)chunk);
		            break;
		        default:
		            jump_chunk();
		        }
		    }
		}

		void load_material()
		{
		    int fpos=filepos+chunklen;
		    while( filepos<fpos )
		    {
		        read_chunk();
		        switch(chunkid)
		        {
			        case 0xA000:
		            if (matlib)
		            {
		                Mat3ds *temp=new Mat3ds[nmat+1];
		                memcpy(temp,matlib,sizeof(Mat3ds)*nmat);
		                delete [] matlib;
		                matlib=temp;
		            } else matlib=new Mat3ds[1];
		            load_string(matlib[nmat].name);
		            nmat++;
		            break;
		        case 0xA010:
		            load_chunk();
		            matlib[nmat-1].ambient[0]=(float)(((unsigned char)chunk[6])/255.0);
		            matlib[nmat-1].ambient[1]=(float)(((unsigned char)chunk[7])/255.0);
		            matlib[nmat-1].ambient[2]=(float)(((unsigned char)chunk[8])/255.0);
		            break;
		        case 0xA020:
		            load_chunk();
		            matlib[nmat-1].diffuse[0]=(float)(((unsigned char)chunk[6])/255.0);
		            matlib[nmat-1].diffuse[1]=(float)(((unsigned char)chunk[7])/255.0);
		            matlib[nmat-1].diffuse[2]=(float)(((unsigned char)chunk[8])/255.0);
		            break;
		        case 0xA030:
		            load_chunk();
		            matlib[nmat-1].specular[0]=(float)(((unsigned char)chunk[6])/255.0);
		            matlib[nmat-1].specular[1]=(float)(((unsigned char)chunk[7])/255.0);
		            matlib[nmat-1].specular[2]=(float)(((unsigned char)chunk[8])/255.0);
		            break;
		        case 0xA040:
		            load_chunk();
		            matlib[nmat-1].shininess=(float)(((unsigned char)chunk[6])/100.0);
		            break;
		        case 0xA041:
		            load_chunk();
		            matlib[nmat-1].shininess_strength=(float)(((unsigned char)chunk[6])/100.0);
		            break;
		        case 0xA050:
		            load_chunk();
		            matlib[nmat-1].transparency=(float)(((unsigned char)chunk[6])/100.0);
		            break;
		        case 0xA052:
		            load_chunk();
		            matlib[nmat-1].transparency_falloff=(float)(((unsigned char)chunk[6])/100.0);
		            break;
		        case 0xA053:
		            load_chunk();
		            matlib[nmat-1].reflect_blur=(float)(((unsigned char)chunk[6])/100.0);
		            break;
		        case 0xA100:
		            load_chunk();
		            matlib[nmat-1].material_type=*((short *)chunk);
		            break;
		        case 0xA084:
		            load_chunk();
		            matlib[nmat-1].self_illum=(float)(((unsigned char)chunk[6])/100.0);
		            break;
		        case 0xA200:
		            load_mapping(&matlib[nmat-1].map_texture1);
		            break;
		        case 0xA33A:
		            load_mapping(&matlib[nmat-1].map_texture2);
		            break;
		        case 0xA210:
		            load_mapping(&matlib[nmat-1].map_opacity);
		            break;
		        case 0xA230:
		            load_mapping(&matlib[nmat-1].map_bump);
		            break;
		        case 0xA204:
		            load_mapping(&matlib[nmat-1].map_specular);
		            break;
		        case 0xA33C:
		            load_mapping(&matlib[nmat-1].map_shininess);
		            break;
		        case 0xA33D:
		            load_mapping(&matlib[nmat-1].map_selfillum);
		            break;
		        case 0xA220:
		            load_mapping(&matlib[nmat-1].map_reflection);
		            break;
		        default:
		            jump_chunk();
		        }
		    }
		}

	
		virtual uint load(e6::Engine * e, Core::World * w, const char * orig) 
		{
			engine = e;
			world = w;
			uint start = e6::sys::getMicroSeconds();

			uint r = import(orig);

			uint stop = e6::sys::getMicroSeconds();
			printf( "loaded %s in %lu micros.\n", orig, stop-start );
			//E_RELEASE( w );
			//E_RELEASE( e );
			return r;
		}
	};
};



extern "C"
uint getClassInfo( ClassInfo ** ptr )
{
	const static ClassInfo _ci[] =
	{
		{	"Core.Importer",	 "Import3ds",	I3ds::CImporter::createInterface, I3ds::CImporter::classRef	},
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
	mv->modVersion = ( "Import3s 00.00.1 (" __DATE__ ")" );
	mv->e6Version =	e6::e6_version;
	return 1; 
}
