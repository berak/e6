#include "Octree.h"

struct Monster
{
    float m[3];
    float M[3];
};
int main()
{
    Monster monster[20] = 
    {
        {{-1,-1,-1},{1,1,1}},
        {{-11,-11,-11},{-5,-5,-5}},
        {{3,3,3},{5,5,5}},
        {{-4,-4,-4},{-1,-1,-1}},
        {{-4,-4,-4},{1,1,1}},
        {{-4,-4,-4},{3,3,3}}
    };
    
    Octree < Monster* > moc;
    for ( uint i=0; i<5; i++ )
    {
        moc.add( &monster[i], e6::BBox(monster[i].m,monster[i].M) );
    }    
    return 0;
}
