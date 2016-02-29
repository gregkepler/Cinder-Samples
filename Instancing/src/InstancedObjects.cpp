//
//  Instancing Objects
//  Created by Greg Kepler
//

#include "cinder/app/App.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Rand.h"
#include "cinder/Log.h"
#include "InstancedObjects.h"

using namespace ci;
using namespace ci::app;
using namespace std;

InstancedArrows::InstancedArrows()
{
	mTime = 0.0;
	
	// LOAD the spritesheet texture
	auto fmt = gl::Texture2d::Format().mipmap().minFilter( GL_LINEAR_MIPMAP_LINEAR ).wrap( GL_CLAMP_TO_EDGE ).loadTopDown();
	mArrowTexture = gl::Texture::create( loadImage( loadAsset( "arrows.png" ) ), fmt );

	// ADD the arrows to a vbo mesh
	std::vector<InstanceData> instances;
	
	// CREATE maximum size instance data buffer.
	instances.resize( kMaxCount, InstanceData() );

	// DESCRIBE the shader attributes, whcih matches up with how InstanceData is described.
	geom::BufferLayout instanceDataLayout;
	instanceDataLayout.append( geom::Attrib::CUSTOM_0, sizeof( mat4 ) / sizeof( float ), sizeof( InstanceData ), offsetof( InstanceData, transform ), 1 /* per instance */ );
	instanceDataLayout.append( geom::Attrib::CUSTOM_1, sizeof( vec4 ) / sizeof( float ), sizeof( InstanceData ), offsetof( InstanceData, color ), 1 /* per instance */ );
	instanceDataLayout.append( geom::Attrib::CUSTOM_2, sizeof( vec4 ) / sizeof( float ), sizeof( InstanceData ), offsetof( InstanceData, texBounds ), 1 /* per instance */ );


	// CREATE instanced batch.
	mShader = gl::GlslProg::create( loadAsset( "arrows.vert" ), loadAsset( "arrows.frag" ) );
	mInstanceDataVbo = gl::Vbo::create( GL_ARRAY_BUFFER, instances.size() * sizeof( InstanceData ), instances.data(), GL_STATIC_DRAW );
	auto mesh = gl::VboMesh::create( geom::Rect( Rectf( 0, 0, 1.0, 0.25 ) ) );
	mesh->appendVbo( instanceDataLayout, mInstanceDataVbo );
	mBatch = gl::Batch::create( mesh, mShader, { { geom::Attrib::CUSTOM_0, "vInstanceTransform" }, { geom::Attrib::CUSTOM_1, "vInstanceData" }, { geom::Attrib::CUSTOM_2, "vTexCoord" } } );
	
	// DEFINE possible sprite sheet texture coords
	ivec2 cellSize = ivec2( 800, 200 );
	std::vector<Rectf> areas;
	areas.resize( 4 );
	areas[0] = mArrowTexture->getAreaTexCoords( Area( ivec2( 2, 2 ), ivec2( 2, 2 ) + cellSize ) );
	areas[1] = mArrowTexture->getAreaTexCoords( Area( ivec2( 2, 204 ), ivec2( 2, 204 ) + cellSize ) );
	areas[2] = mArrowTexture->getAreaTexCoords( Area( ivec2( 2, 406 ), ivec2( 2, 406 ) + cellSize ) );
	areas[3] = mArrowTexture->getAreaTexCoords( Area( ivec2( 2, 608 ), ivec2( 2, 608 ) + cellSize ) );

	// DEFINE the arrow properties
	Rand::randomize();
	int rows = 5;
	int cols = 5;
	float xFactor = 1.0f / float( cols );
	float yFactor = 1.0f / float( rows );
	
	for( int i = 0; i < kMaxCount; ++i )
	{
		Rectf  texCoords = areas[randInt( areas.size() )];
		ColorA color = ColorA( CM_HSV, randFloat( 0, 0.5), 0.7, 0.5 );
		float  y = ( float( floor( i / cols ) ) * yFactor ) + randFloat( -0.25, 0.25 );
		float  x = ( float( i % rows ) * xFactor ) + randFloat( -0.25, 0.25 );
		float  scale = randFloat( 0.4, 1.0 );
		
		ArrowOptions options = ArrowOptions().position( vec3( x * getWindowWidth(), y * getWindowHeight(), scale ) ).texCoords( texCoords ).color( color ).scale( scale ).speed( scale * 4.0 );
		mOptions.push_back( options );
	}
}

void InstancedArrows::update( double elapsed )
{
	if( elapsed < 0.001 )
		return;
	
	// UPDATE all of the positions of the arrows
	auto  ptr = (InstanceData *)mInstanceDataVbo->mapReplace();
	int   count = 0;

	for( int i = 0; i < kMaxCount; ++i ) {

		auto options = mOptions[count];
		vec3 position = options.getPosition();
		position.x += options.getSpeed();
		if( position.x > getWindowWidth() ){
			position.x -= ( getWindowWidth() + ( 200.0 * options.getScale() ) );
			position.y = randFloat( getWindowHeight() );
		}
		
		ptr->transform = glm::translate( position );
		ptr->transform *= glm::scale( vec3( options.getScale() * 200.0 ) );

		float alpha = options.getScale();
		ColorA color = ColorA( Color( options.getColor() ), alpha );
		ptr->color = vec4( color );
		Rectf area = options.getTexCoords();
		vec4  texCoords = vec4( area.getX1(), area.getY1(), area.getWidth(), area.getHeight() );
		ptr->texBounds = texCoords;
		
		// UPDATE object
		mOptions[count] = options.position( position );
		ptr++;
		count++;
	}
	mInstanceDataVbo->unmap();
}

void InstancedArrows::draw()
{
	// DRAW the arrows
	if( mArrowTexture )
	{
		gl::ScopedDepth( true );
		gl::ScopedModelMatrix scpMtrx;
		gl::ScopedTextureBind scpTex0( mArrowTexture, 0 );
		mBatch->drawInstanced( kMaxCount );
	}
}


// ------------------------------------------------------------------------------------------------- Instanced Dots


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  Dots

InstancedDots::Dots::Dots()
    : InstancedBase( kDotsPerRow * kMaxRows )
	, mTime( 0.0 )
{
	mShader = gl::GlslProg::create( loadAsset( "dots.vert" ), loadAsset( "dots.frag" ) );

	createBatch( geom::Circle().radius( 3.0 ), mShader );

	// blue dots
	float rFactor = glm::radians( 360.0f / float( kDotsPerRow ) );
	float rOffset = 0.0;
	for( float i = 0.0; i < kDotsPerRow; ++i ) {
		DotData d;
		d.color = vec3( 0.35, 0.67, 0.97 );
		d.direction = vec2( cos( ( rFactor * i ) + rOffset ), sin( ( rFactor * i ) + rOffset ) );
		d.offset = 0.0;
		mDotData.push_back( d );
	}

	// yellow dots
	rOffset = -rFactor * 0.5;
	for( float i = 0.0; i < kDotsPerRow; ++i ) {
		DotData d;
		d.color = vec3( 1.0, 1.0, 0.0 );
		d.direction = vec2( cos( ( rFactor * i ) + rOffset ), sin( ( rFactor * i ) + rOffset ) );
		d.offset = 1.0;
		mDotData.push_back( d );
	}

	// orange dots
	rOffset = 0.0;
	for( float i = 0.0; i < kDotsPerRow; ++i ) {
		DotData d;
		d.color = vec3( 1.0, 0.47, 0.19 );
		d.direction = vec2( cos( ( rFactor * i ) + rOffset ), sin( ( rFactor * i ) + rOffset ) );
		d.offset = 2.0;
		mDotData.push_back( d );
	}
	
	// yellow dots
	rOffset = -rFactor * 0.5;
	for( float i = 0.0; i < kDotsPerRow; ++i ) {
		DotData d;
		d.color = vec3( 1.0, 1.0, 0.0 );
		d.direction = vec2( cos( (rFactor * i) + rOffset ), sin( (rFactor * i) + rOffset ) );
		d.offset = 3.0;
		mDotData.push_back( d );
	}

}

void InstancedDots::Dots::update( double elapsed )
{
	mTime += elapsed;
	double progress = mTime;

	auto ptr = (Data *)mDataVbo->mapReplace();
	mInstanceCount = 0;
	float maxDist = 60.0;
	for( int i = 0; i < mMaxCount; ++i ) {
		DotData d = mDotData[i];
		
		float distance = fmod( (0.25f * d.offset) + progress, 1.0f );
		float alpha = 1.0f - fabs( ( distance * 2.0f ) - 1.0f );
		vec2  pos = d.direction * vec2( distance * maxDist );
		ptr->transform = glm::translate( vec3( pos, 0 ) );
		ptr->transform *= glm::scale( vec3( alpha ) );

		ptr->data = vec4( d.color, alpha );
		ptr++;
		mInstanceCount++;
	}
	mDataVbo->unmap();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - Instanced Dots (Parent)

InstancedDots::InstancedDots()
	: mAlpha( 1.0f )
	, mScale( 1.0f )
	, mAlphaOffset( 1.0 )
{
	mCamera.setOrtho( -50, 50, -50, 50, -10.0f, 1000.0f );
	mCamera.lookAt( vec3( 0, 0, 100 ), vec3( 0, 0, 0 ), vec3( 0, 1, 0 ) );
	mTimeline = Timeline::create();
}

void InstancedDots::update( double elapsed )
{
	mDots.update( elapsed );
}

void InstancedDots::draw()
{
	gl::ScopedColor    scpColor;
	gl::ScopedMatrices scpMtrx;
	gl::setMatrices( mCamera );
	gl::scale( vec2( mScale() ) );
	
	Area vp = Area( ivec2(), ivec2( kViewportSize ) );
	vp = Area::proportionalFit( vp, getWindowBounds(), true, false );
	gl::ScopedViewport scpView( vp.getUL(), vp.getSize() );

	gl::color( 1, 1, 1, mAlpha() * mAlphaOffset );
	gl::ScopedBlendPremult scpBlend;

	mDots.draw();
}