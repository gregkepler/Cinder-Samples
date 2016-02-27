#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Rand.h"
#include "cinder/Timeline.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;


// ------------------------------------------------------------------------------------------------- Properties for each unique arrow instance

class ArrowOptions {
  public:
	ArrowOptions()
	    : mTexCoords( ci::Rectf() )
	    , mPosition( glm::vec3( 0.0f ) )
	    , mColor( ci::ColorA::white() )
	    , mScale( 1.0f )
	    , mSpeed( 0.0f )
	{}

	ArrowOptions &texCoords( const ci::Rectf &texCoords )
	{
		mTexCoords = texCoords;
		return *this;
	}
	const ci::Rectf &getTexCoords() const { return mTexCoords; }

	ArrowOptions &position( const glm::vec3 &position )
	{
		mPosition = position;
		return *this;
	}
	const glm::vec3 &getPosition() const { return mPosition; }

	ArrowOptions &scale( const float &scale )
	{
		mScale = scale;
		return *this;
	}
	const float &getScale() const { return mScale; }

	ArrowOptions &color( const ci::ColorA &color )
	{
		mColor = color;
		return *this;
	}
	const ci::ColorA &getColor() const { return mColor; }

	ArrowOptions &speed( const float &speed )
	{
		mSpeed = speed;
		return *this;
	}
	const float &getSpeed() const { return mSpeed; }

  private:
	glm::vec3  mPosition;
	ci::Rectf  mTexCoords;
	ci::ColorA mColor;
	float      mScale;
	float      mSpeed;
};


// ------------------------------------------------------------------------------------------------- Main app

class InstancingApp : public App {
  public:
	void setup() override;
	void keyDown( KeyEvent event ) override;
	void update() override;
	void draw() override;
	
	typedef struct InstanceData {
		ci::mat4 transform;
		ci::vec4 color;
		ci::vec4 texBounds;
	} InstanceData;
	
	
  private:
	ci::gl::TextureRef	      mArrowTexture;
	ci::gl::GlslProgRef       mShader;
	ci::gl::VboRef            mInstanceDataVbo;
	ci::gl::BatchRef		  mBatch;
	std::vector<ArrowOptions> mOptions;
	ci::TimelineRef			  mTimeline;
	double					  mTime;
	bool					  mIsPaused = false;
	
	static const int kMaxCount = 30;
};


void InstancingApp::setup()
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
		float  scale = randFloat( 1.0, 1.5 );
		
		ArrowOptions options = ArrowOptions().position( vec3( x * getWindowWidth(), y * getWindowHeight(), scale - 1.0 ) ).texCoords( texCoords ).color( color ).scale( scale ).speed( scale * 3.0 );
		mOptions.push_back( options );
	}
}


void InstancingApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
	
	case KeyEvent::KEY_SPACE:
		mIsPaused = !mIsPaused;
		break;
	}
	
}


void InstancingApp::update()
{
	// USE a fixed time step for a steady 60 updates per second.
	static const double timestep = 1.0 / 60.0;

	// KEEP TRACK of time.
	static double time = getElapsedSeconds();
	static double accumulator = 0.0;

	// CALCULATE elapsed time since last frame.
	double elapsed = getElapsedSeconds() - time;
	time += elapsed;

	
	accumulator += math<double>::min( elapsed, 0.1 ); // prevents 'spiral of death'
	while( accumulator >= timestep ) {
		mTime += mIsPaused ? 0.0 : timestep;
		accumulator -= timestep;
	}
	
	
	// RETURN on pause
	if( mIsPaused )
		return;
	
	// UPDATE all of the positions of the arrows
	auto  ptr = (InstanceData *)mInstanceDataVbo->mapReplace();
	int   count = 0;

	for( int i = 0; i < kMaxCount; ++i ) {

		auto options = mOptions[count];
		vec3 position = options.getPosition();
		float alpha = options.getScale() - 0.5;
		
		
		position.x += options.getSpeed();
		if( position.x > getWindowWidth() ){
			position.x -= ( getWindowWidth() + (150.0 * options.getScale()) );
			position.y = randFloat( getWindowHeight() );
		}
		
		ptr->transform = glm::translate( position );
		ptr->transform *= glm::scale( vec3( options.getScale() * 150.0 ) );

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


void InstancingApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	
	// DRAW the arrows
	{
		gl::ScopedDepth( true );
		gl::ScopedModelMatrix scpMtrx;
		gl::ScopedTextureBind scpTex0( mArrowTexture, 0 );
		mBatch->drawInstanced( kMaxCount );
	}
}

CINDER_APP( InstancingApp, RendererGl( RendererGl::Options().msaa( 8 ) ) )
