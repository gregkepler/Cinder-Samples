#include "Instanced.h"

using namespace ci;

InstancedBase::InstancedBase( size_t maxCount )
    : mMaxCount( maxCount )
    , mInstanceCount( 0 )
{
	std::vector<Data> instances;
	instances.resize( mMaxCount, Data() );

	mDataVbo = gl::Vbo::create( GL_ARRAY_BUFFER, instances.size() * sizeof( Data ), instances.data(), GL_DYNAMIC_DRAW );

	// Describe the shader attributes.
	mLayout.append( geom::Attrib::CUSTOM_0, sizeof( mat4 ) / sizeof( float ), sizeof( Data ), offsetof( Data, transform ), 1 /* per instance */ );
	mLayout.append( geom::Attrib::CUSTOM_1, sizeof( vec4 ) / sizeof( float ), sizeof( Data ), offsetof( Data, data ), 1 /* per instance */ );
}

void InstancedBase::draw()
{
	assert( mInstanceCount <= mMaxCount );

	if( mBatch && mInstanceCount > 0 ) {
		mBatch->drawInstanced( mInstanceCount );
	}
}

void InstancedBase::replaceGlslProg( const ci::gl::GlslProgRef &glsl )
{
	if( mBatch )
		mBatch->replaceGlslProg( glsl );
}

void InstancedBase::createBatch( const ci::geom::Source &source, const ci::gl::GlslProgRef &glsl )
{
	// Create mesh.
	mMesh = gl::VboMesh::create( source );
	createBatch( mMesh, glsl );
}

void InstancedBase::createBatch( const gl::VboMeshRef &mesh, const ci::gl::GlslProgRef &glsl )
{
	// Create mesh.
	mMesh = mesh;
	mMesh->appendVbo( mLayout, mDataVbo );

	// Create batch.
	mBatch = gl::Batch::create( mMesh, glsl, { { geom::Attrib::CUSTOM_0, "vInstanceTransform" }, { geom::Attrib::CUSTOM_1, "vInstanceData" } } );
}