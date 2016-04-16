#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "cinder/Rand.h"
#include "cinder/Timeline.h"
#include "cinder/CameraUi.h"
#include "glm/gtx/matrix_decompose.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;

// ------------------------------------------------------------------------------------------------- Properties for each unique arrow instance

class InstanceOptions {
  public:
	InstanceOptions()
	    : mTexCoords( ci::Rectf() )
	    , mPosition( glm::vec3( 0.0f ) )
	    , mColor( ci::ColorA::white() )
	    , mDepthFactor( 0.0f )
	    , mSpeed( 0.0f )
	{}

	InstanceOptions &texCoords( const ci::Rectf &texCoords )
	{
		mTexCoords = texCoords;
		return *this;
	}
	const ci::Rectf &getTexCoords() const { return mTexCoords; }

	InstanceOptions &position( const glm::vec3 &position )
	{
		mPosition = position;
		return *this;
	}
	const glm::vec3 &getPosition() const { return mPosition; }

	InstanceOptions &depthFactor( const float &depthFactor )
	{
		mDepthFactor = depthFactor;
		return *this;
	}
	const float &getDepthFactor() const { return mDepthFactor; }

	InstanceOptions &color( const ci::ColorA &color )
	{
		mColor = color;
		return *this;
	}
	const ci::ColorA &getColor() const { return mColor; }

	InstanceOptions &speed( const float &speed )
	{
		mSpeed = speed;
		return *this;
	}
	const float &getSpeed() const { return mSpeed; }

  private:
	glm::vec3  mPosition;
	ci::Rectf  mTexCoords;
	ci::ColorA mColor;
	float      mDepthFactor;
	float      mSpeed;
};


class ImageTransitionsApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void keyDown( KeyEvent event ) override;
	void update() override;
	void draw() override;
	void resize() override;
	
	void transitionImage();
	gl::TextureRef	mTexture1, mTexture2;
	static const int kCols = 20;
	static const int kRows = 10;
	static const int kMaxCount = kCols * kRows;
	CameraPersp mCam;
	CameraUi	mCamUi;
	Anim<vec3>  mEyePt;
	
	gl::GlslProgRef mShader;
	ci::gl::VboRef            mInstanceDataVbo;
	ci::gl::BatchRef		  mBatch;
	std::vector<InstanceOptions> mOptions;
	bool			   mIsPaused = false;
	ci::TimelineRef	   mTimeline;
	
	typedef struct InstanceData {
		ci::mat4 transform;
		ci::vec4 color;
		ci::vec4 texBounds;
	} InstanceData;
	
	
	ci::Anim<float>	mDepthAnim;
	ci::Anim<float> mCamRotation;
};

void ImageTransitionsApp::setup()
{
	// Load test textures
	mTexture1 = gl::Texture::create( loadImage( loadAsset( "test1.jpg" ) ) );
	mTexture2 = gl::Texture::create( loadImage( loadAsset( "test2.jpg" ) ) );
	
	// Generate array of quads to fill the screen
	
	std::vector<InstanceData> instances;
	
	// CREATE maximum size instance data buffer.
	instances.resize( kMaxCount, InstanceData() );

	// DESCRIBE the shader attributes, whcih matches up with how InstanceData is described.
	geom::BufferLayout instanceDataLayout;
	instanceDataLayout.append( geom::Attrib::CUSTOM_0, sizeof( mat4 ) / sizeof( float ), sizeof( InstanceData ), offsetof( InstanceData, transform ), 1 /* per instance */ );
	instanceDataLayout.append( geom::Attrib::CUSTOM_1, sizeof( vec4 ) / sizeof( float ), sizeof( InstanceData ), offsetof( InstanceData, color ), 1 /* per instance */ );
	instanceDataLayout.append( geom::Attrib::CUSTOM_2, sizeof( vec4 ) / sizeof( float ), sizeof( InstanceData ), offsetof( InstanceData, texBounds ), 1 /* per instance */ );
	
	// CREATE instanced batch.
	mShader = gl::GlslProg::create( loadAsset( "texQuad.vert" ), loadAsset( "texQuad.frag" ) );
	mInstanceDataVbo = gl::Vbo::create( GL_ARRAY_BUFFER, instances.size() * sizeof( InstanceData ), instances.data(), GL_STATIC_DRAW );
	auto mesh = gl::VboMesh::create( geom::Rect( Rectf( 0, 0, 1.0, 1.0 ) ) );
	mesh->appendVbo( instanceDataLayout, mInstanceDataVbo );
	mBatch = gl::Batch::create( mesh, mShader, { { geom::Attrib::CUSTOM_0, "vInstanceTransform" }, { geom::Attrib::CUSTOM_1, "vInstanceData" }, { geom::Attrib::CUSTOM_2, "vTexCoord" } } );
	
	// DEFINE the arrow properties
	Rand::randomize();
	float size = 100.0f;
	float xFactor = 1.0 / kCols;
	float yFactor = 1.0 / kRows;
	for( int i = 0; i < kMaxCount; ++i )
	{
		float row = float( i % kCols ) / float( kCols );
		float col = floor( float( i ) / float( kCols ) ) / float( kRows );
		CI_LOG_V( row << " " << col );
		
		Rectf  texCoords = Rectf( row, col + yFactor, row + xFactor, col );
		ColorA color = ColorA( CM_HSV, randFloat( 0, 0.5), 0.8, 0.75 );
//		float x = ( ( float( i % kCols ) / float( kCols ) ) - 0.5f ) * 100.0f;
//		float y = ( ( floor( float( i ) / float( kCols ) ) / float( kRows ) ) - 0.5f) * (100.0f * float(kRows)/float(kCols));
		float x = ( ( float( i % kCols ) / float( kCols ) ) - 0.5f ) * size;
		float y = ( ( floor( float( i ) / float( kCols ) ) / float( kRows ) ) - 0.5f) * (size * float(kRows)/float(kCols));
		
		
		float z = 0.0;
		float maxDepth = randFloat() - 0.5f;
//		float z = 0.0f;
		
		InstanceOptions options = InstanceOptions().position( vec3( x, y, z ) ).texCoords( texCoords ).color( color ).depthFactor( maxDepth );
		mOptions.push_back( options );
	}
	
	mEyePt = vec3( 0, 0, 80 );
	// initialize camera
	mCam.setPerspective( 60.0f, getWindowAspectRatio(), 0.1, 1000 );
	mCam.setWorldUp( vec3( 0, 1, 0 ) );
	mCam.setEyePoint( mEyePt() );
	mCam.lookAt( vec3( 0, 0, 0 ) );
//	mCam.lookAt( vec3( 0, 0, -50 ), vec3( 0, 0, 0 ), vec3( 0, 1, 0 ) );
	
	mTimeline = Timeline::create();
	mDepthAnim = 0.0f;
	mCamRotation = glm::radians( 90.0 );
	
//	auto mBoundingSphere = Sphere::calculateBoundingSphere( vec3() );
//	mCam = mCam.calcFraming( mBoundingSphere );
	mCamUi			= CameraUi( &mCam );
}

void ImageTransitionsApp::mouseDown( MouseEvent event )
{
	mCamUi.mouseDown( event );
}


void ImageTransitionsApp::mouseDrag( MouseEvent event )
{
//	Rectf r	= Rectf( getWindowWidth() / 2, 0, getWindowWidth(), getWindowHeight() );
//	if ( r.contains( event.getPos() )) {
		mCamUi.mouseDrag( event );
//	}
}


void ImageTransitionsApp::keyDown( KeyEvent event )
{
	switch( event.getCode() ) {
		case KeyEvent::KEY_RIGHT:
			transitionImage();
			break;
		
		case KeyEvent::KEY_SPACE:
			mIsPaused = !mIsPaused;
			break;
	}

}

void ImageTransitionsApp::resize()
{
	mCam.setAspectRatio( getWindowAspectRatio() );
}

void ImageTransitionsApp::transitionImage()
{
	CI_LOG_V( "NEXT" );
	mTimeline->apply( &mDepthAnim, 100.0f, 1.5f, EaseInOutQuart() );
	mTimeline->apply( &mEyePt, vec3( 0, 0, 110 ), 1.5f, EaseInOutQuart() );
	
	mTimeline->apply( &mCamRotation, mCamRotation() + float( M_PI ), 1.5f, EaseInOutQuart() ).delay( 0.75f );
	
	mTimeline->appendTo( &mDepthAnim, 0.0f, 1.5f, EaseInOutQuart() ).delay( 0.0f );
	mTimeline->appendTo( &mEyePt, vec3( 0, 0, 80 ), 1.5f, EaseInOutQuart() ).delay( 0.0f );
}

void ImageTransitionsApp::update()
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
		accumulator -= timestep;
		double step  = mIsPaused ? 0.0 : timestep;
		mTimeline->step( step );
		
	}
	
	// UPDATE camera
//	mCam.lookAt( vec3( 0, 0, mCamDepth() ), vec3( 0, 0, 0 ), vec3( 0, 1, 0 ) );
//	vec3 pt = mEyePt();
//	mat4 mtrx = translate( mat4(), mEyePt() );
//	mtrx = rotate( mtrx, mCamRotation(), vec3( 0, 0, 1.0 ) );
	vec3 pt = vec3( cos( mCamRotation() ), 0, sin( mCamRotation()) ) * vec3( mEyePt().z );
	
	
	/*glm::mat4 transformation; // your transformation matrix.
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(transformation, scale, rotation, translation, skew, perspective);*/



//	vec3 pos = mtrx.

	mCam.setEyePoint( pt );
	mCam.lookAt( vec3() );
	
	// UPDATE all of the positions of the arrows
	auto  ptr = (InstanceData *)mInstanceDataVbo->mapReplace();
	int   count = 0;

	for( int i = 0; i < kMaxCount; ++i ) {

		auto options = mOptions[count];
		vec3 position = options.getPosition();
		position.z = options.getDepthFactor() * mDepthAnim();
		
//		position.x += options.getSpeed();
	/*	if( position.x > getWindowWidth() ){
			position.x -= ( getWindowWidth() + ( 200.0 * options.getScale() ) );
			position.y = randFloat( getWindowHeight() );
		}*/
		
		ptr->transform = glm::translate( position );
		ptr->transform *= glm::scale( vec3( 5.0 ) );

		ColorA color = ColorA( Color( options.getColor() ), 1.0f );
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

void ImageTransitionsApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	
	gl::ScopedColor scpCol( ColorA::white() );

	
	// draw all of the quads
	gl::ScopedDepth( true );
	gl::enableDepthRead();
	gl::enableDepthWrite();
	
	
	gl::ScopedModelMatrix scpMtrx;
	gl::setMatrices( mCam );
	
//	gl::drawSolidRect( Rectf( -25, -25, 25, 25 ) );

	gl::ScopedFaceCulling scpCull( true );
//	gl::cullFace( GL_BACK );
	
	{
		gl::cullFace( GL_FRONT );
		gl::ScopedTextureBind scpTex0( mTexture1, 0 );
		mBatch->drawInstanced( kMaxCount );
	}
	
	{
		gl::cullFace( GL_BACK );
		gl::ScopedTextureBind scpTex0( mTexture2, 0 );
		mBatch->drawInstanced( kMaxCount );
	}
	
	
	// draw front with culled back
	
	// draw back with culled front
}

CINDER_APP( ImageTransitionsApp, RendererGl )
