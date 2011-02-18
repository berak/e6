#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "../Video/Video.h"

#define NOISE_DIM 20
#define FNOISE_DIM 20.0f

float xFactor=0.42f;
float yFactor=0.321f;
float zFactor=0.73f;

float cRed=255.0f;
float cGreen=0.0f;
float cBlue=255.0f;

float X=0,dX=0;
float Y=0,dY=0;

int reinit = 2;
int bypass = 0;
int solo = 0x77;
int clip = 0x30;

static float noise_table[NOISE_DIM+1][NOISE_DIM+1][NOISE_DIM+1];

inline
void init_noise_one(int i,int j,int k) {
	int ii,jj,kk;
    noise_table[i][j][k] = (float)(rand()&0x7FFF);
    ii = (i==NOISE_DIM)?0:i; 
    jj = (j==NOISE_DIM)?0:j; 
    kk = (k==NOISE_DIM)?0:k; 
    noise_table[i][j][k] = noise_table[ii][jj][kk];
}

void init_noise(unsigned int seed) {
	int i,j,k;
	srand(seed);
	for (i=0; i<=NOISE_DIM; i++)
	for (j=0; j<=NOISE_DIM; j++)
	for (k=0; k<=NOISE_DIM; k++) 
		init_noise_one(i,j,k);
}

inline
float noise(float x, float y, float z) {
	int ix,iy,iz;
	float fx,fy,fz,mx,my,mz;
	float n,n00,n01,n10,n11,n0,n1;
	mx = (float)fmod(x,FNOISE_DIM); if (mx<0) mx += FNOISE_DIM;
	my = (float)fmod(y,FNOISE_DIM); if (my<0) my += FNOISE_DIM;
	mz = (float)fmod(z,FNOISE_DIM); if (mz<0) mz += FNOISE_DIM;
	ix = (int)mx;
	iy = (int)my;
	iz = (int)mz;
	fx = (float)fmod(mx,1.0f);
	fy = (float)fmod(my,1.0f);
	fz = (float)fmod(mz,1.0f);
	n = noise_table[ix][iy][iz];
	n00 = n + fx*(noise_table[ix+1][iy][iz]-n);
	n = noise_table[ix][iy][iz+1];
	n01 = n + fz*(noise_table[ix+1][iy][iz+1]-n);
	n = noise_table[ix][iy+1][iz];
	n10 = n + fy*(noise_table[ix+1][iy+1][iz]-n);
	n = noise_table[ix][iy+1][iz+1];
	n11 = n + fx*(noise_table[ix+1][iy+1][iz+1]-n);
	n0 = n00 + fy*(n10-n00);
	n1 = n01 + fy*(n11-n01);
	return(((float)(n0+fz*(n1-n0)))/32768.0f);
}



extern //"C"
void transform ( void *pData, uint w, uint h ) 
{
    if ( bypass ) return ;

    if ( reinit )
    {
        int r = reinit;
        while ( r-- )
        {
            int i = rand() % NOISE_DIM;
            int j = rand() % NOISE_DIM;
            int k = rand() % NOISE_DIM;

            init_noise_one(i,j,k);
        }
    }


    static int ticks = 0;
    ++ ticks;

	Video::RGBPixel *prgb = (Video::RGBPixel*) pData;

    float dy =  xFactor*FNOISE_DIM / h;
    float dx =  yFactor*FNOISE_DIM / w;
    float dz =  zFactor*FNOISE_DIM / 255.0f;
    float px,py,pz=0,pq=0;
    float r,g,b;

    X+=dX; Y+=dY;
    px=X; py=Y;

    unsigned int x,y;    
    for (y = 0 ; y < h; y++) 
    {
        for (x = 0 ; x < w; x++) 
        {
            pz = dz * prgb->Blue;
            r = ( prgb->Red & solo ) + (cRed * noise( px, py, pz ));
            prgb->Red = ( clip&&(r>255) ? 255 : (unsigned char)r );

            pz = dz * prgb->Green;
            b = ( prgb->Blue & solo )  + (cBlue * noise( pz, px, py ));
            prgb->Blue = ( clip&&(b>255) ? 255 : (unsigned char)b );

            pz = dz * prgb->Red;
            g = ( prgb->Green & solo )  + (cGreen * noise( py, pz, px ));
            prgb->Green = ( clip&&(g>255) ? 255 : (unsigned char)g );

            //prgb->Green    = (BYTE)(255.0f*noise( pz,px,pq ));
            prgb++;
            px += dx;
        }
        py += dy;
        px=X;
    }
}
