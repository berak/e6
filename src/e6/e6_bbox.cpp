#include "e6_BBox.h"

namespace e6
{
    
    
    //
    // ray-aabb intersection:
    //
    bool BBox::rayHitBox( const float o[3], const float d[3] ,float& tnear,float& tfar)
    {
        const static float EPSILON = 0.0001f;
        static float t1,t2,t;
        tnear = -0xffffff;
        tfar  =  0xffffff;

        for ( int a=0; a<3; a++ ) {
            if ( d[a]>-EPSILON && d[a]<EPSILON ) {
                if ( o[a]<m[a] || o[a]>M[a] )
                    return 0;
            } else {
                t1=(m[a]-o[a])/d[a];
                t2=(M[a]-o[a])/d[a];
                if (t1>t2)  { t=t1; t1=t2; t2=t; }
                if (t1>tnear) tnear=t1;
                if (t2<tfar)  tfar=t2;
                if (tnear>tfar || tfar<EPSILON)
                    return 0;
            }
        }
        if (tnear>tfar || tfar<EPSILON)
            return 0;
        return 1;
    }

    
    
} // e6

