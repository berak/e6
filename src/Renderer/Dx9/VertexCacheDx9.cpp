#include "Dx9.h"

namespace Dx9
{
	IDirect3DVertexBuffer9 * VertexCache::addBuffer( Core::VertexBuffer * mesh )
	{
		HRESULT hr;
		assert( mesh );
		assert( mesh->numFaces() );
		printf( "%s %s\n", __FUNCTION__, (mesh?mesh->getName():"nil"));

		uint vSiz=0;
		uint fvf=0;
		uint nVerts = mesh->numFaces() * 3;
		getFVF( mesh->format(), fvf, vSiz );
		printf( "%s %i verts, %x/%x %i\n", mesh->getName(), nVerts, mesh->format(), fvf, vSiz);
		
		IDirect3DVertexBuffer9 *vb = 0;
		if( FAILED( hr=device->CreateVertexBuffer( nVerts*vSiz,//sizeof(CUSTOMVERTEX),
													  0, fvf,//D3DFVF_CUSTOMVERTEX,
													  D3DPOOL_MANAGED, &vb, NULL ) ) )
		{
			E_TRACE(hr);
			return 0;
		}


		float* pV;
		if( FAILED( hr=vb->Lock( 0, 0, (void**)&pV, 0 ) ) )
		{
			E_TRACE(hr);
			return 0;
		}

		Core::VertexFeature * pos = mesh->getFeature( e6::VF_POS );
		Core::VertexFeature * nor = mesh->getFeature( e6::VF_NOR );
		Core::VertexFeature * col = mesh->getFeature( e6::VF_COL );
		Core::VertexFeature * uv0 = mesh->getFeature( e6::VF_UV0 );
		Core::VertexFeature * uv1 = mesh->getFeature( e6::VF_UV1 );
		Core::VertexFeature * tan = mesh->getFeature( e6::VF_TAN );
		assert(pos);
		float *v=0;

		for ( uint i=0; i<nVerts; i++ )
		{
			{
				v = (float*)pos->getElement( i );
				*pV++ = *v++;
				*pV++ = *v++;
				*pV++ = *v++;
			}
			if ( nor )
			{
				v = (float*)nor->getElement( i );
				*pV++ = *v++;
				*pV++ = *v++;
				*pV++ = *v++;
			}
			if ( col )
			{
				*(uint*)pV = *(uint*)uv0->getElement( i );
				pV ++;
			}
			if ( uv0 )
			{
				v = (float*)uv0->getElement( i );
				*pV++ = *v++;
				*pV++ = *v++;
			}
			if ( uv1 )
			{
				v = (float*)uv1->getElement( i );
				*pV++ = *v++;
				*pV++ = *v++;
			}
			if ( tan )
			{
				v = (float*)tan->getElement( i );
				*pV++ = *v++;
				*pV++ = *v++;
				*pV++ = *v++;
			}
		}
		vb->Unlock();

		insert( mesh, vb );

		E_RELEASE( pos );
		E_RELEASE( uv0 );
		E_RELEASE( uv1 );
		E_RELEASE( nor );
		E_RELEASE( col );
		E_RELEASE( tan );
		return vb;
	}

    IDirect3DVertexBuffer9 * VertexCache::getBuffer( Core::VertexBuffer * m )
	{
		//printf( "%s %s\n", __FUNCTION__, (m?m->getName():"nil"));
		Item * it = find( m );
		if ( ! it || ! it->val )
		{
			return addBuffer(m);
		}
		it->ttl = 500;
		return it->val;
	}
	

}; // dx9

