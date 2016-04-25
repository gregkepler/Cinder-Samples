#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/CameraUi.h"
#include "cinder/Log.h"
#include "cinder/Rand.h"
#include "cinder/Timeline.h"
#include "cinder/params/Params.h"

using namespace ci;
using namespace ci::app;
using namespace std;

// -------------------------------------------------------------------------------------------------
// Definition for each particle
// -------------------------------------------------------------------------------------------------
struct Particle
{
	vec3	pos;		// current position
	vec3	ppos;		// previous position - velocity is determined as diff between pos and ppos
	ColorA  color;
	float	damping;	// velocity damping factor
	vec2	texcoord;
	float	invmass;	// arbitrary mass value for randomizing the physics a bit
};


// -------------------------------------------------------------------------------------------------
// Main app
// -------------------------------------------------------------------------------------------------
class TextParticlesApp : public App {
  public:
	void setup() override;
	void mouseDown	( MouseEvent event ) override;
	void mouseUp	( MouseEvent event ) override;
	void mouseDrag	( MouseEvent event ) override;
	void mouseWheel	( MouseEvent event ) override;
	void keyDown	( KeyEvent event ) override;
	void update() override;
	void draw() override;
	
	void drawTextToFbo();
	void lookAtTexture( const CameraPersp &cam, const ci::vec2 &size );
	void setupVBO();
	void editMode();
	
	CameraPersp			mCam;
	CameraUi			mCamUi;
	
	Font				mFont;
	gl::TextureFontRef	mTextureFont;
	string				mString;
	gl::FboRef			mTextFbo;				// draws the text to this FBO
	vec2				mTextSize;				// actual pixel size of text texture
	int					mTextParticleCount;		// number of pixels in text texture
	
	gl::GlslProgRef		mUpdateProg, mRenderProg;
	gl::TextureRef		mPerlin3dTex;
	
	bool				mActive;
	ci::Surface8u		mTextSurf;
	
	// Descriptions of particle data layout.
	gl::VaoRef			mAttributes[2];
	// Buffers holding raw particle data on GPU.
	gl::VboRef			mParticleBuffer[2];

	// Current source and destination buffers for transform feedback.
	// Source and destination are swapped each frame after update.
	std::uint32_t		mSourceIndex		= 0;
	std::uint32_t		mDestinationIndex	= 1;
	
	// Params for controlling the particle simulation
	params::InterfaceGlRef	mParams;
	vec3					mCenter;			// determines direction of initial particle velocity
	float					mStartVelocity;		// scalar for initial particle velocity
	float					mStepMax;			// max step for a particle to move - increases over time
	float					mDampingSpeed;		// rate at which damping moves to zero
	float					mDampingBase;		// base at which to determine start damping value
	vec3					mNoiseOffset;		// scales the texture coordinates to determine perlin input in the shader
	Color					mEndColor;			// color that the particles fade to as they die
	
	ci::Anim<float>			mStep;				// current step value, as animated on each explosion
};


void TextParticlesApp::setup()
{
	// SET UP camera
	mCam  = CameraPersp( getWindowWidth(), getWindowHeight(), 60, 1.0, 10000.0  );
	mCamUi = CameraUi( &mCam );
	
	// LOAD fonts
	mFont = Font( loadAsset( "SourceSansPro-Bold.ttf" ), 120 );
	mTextureFont = gl::TextureFont::create( mFont );
	
	// DEFINE FBO
	mTextFbo = gl::Fbo::create( getWindowWidth(), getWindowHeight(), true );
	
	// LOAD shaders
	mRenderProg = gl::getStockShader( gl::ShaderDef().color() );
	try {
		mUpdateProg = gl::GlslProg::create( gl::GlslProg::Format().vertex( loadAsset( "shaders/particleUpdate.vs" ) )
			.feedbackFormat( GL_INTERLEAVED_ATTRIBS )
			.feedbackVaryings( { "position", "pposition", "color", "damping", "texcoord", "invmass" } )
			.attribLocation( "iPosition", 0 )
			.attribLocation( "iColor", 1 )
			.attribLocation( "iPPosition", 2 )
			.attribLocation( "iDamping", 3 )
			.attribLocation( "iTexCoord", 4 )
			.attribLocation( "iInvMass", 5 )
		);
	}catch( ci::gl::GlslProgCompileExc &exc ) {
		std::cout << "Shader compile error: " << endl;
		std::cout << exc.what();
	}
	catch( Exception &exc ) {
		std::cout << "Unable to load shader" << endl;
		std::cout << exc.what();
	}
	
	// LOAD perlin texture to use in the shaders
	mPerlin3dTex = gl::Texture::create( loadImage( loadAsset( "shaders/perlin3d.png" ) ) );
	mPerlin3dTex->setWrap( GL_REPEAT, GL_REPEAT );
	
	// SET UP param defaults
	mCenter			= vec3( 0, 0, -10.0 );
	mStartVelocity	= 5.0;
	mStepMax		= 10.0;
	mDampingSpeed	= 0.003;
	mDampingBase	= 0.45f;
	mNoiseOffset	= vec3( 1.0f, 1.0f, 0.0 );
	mEndColor		= Color( 1.0, 1.0, 1.0 );
	
	// SET UP params
	mParams = params::InterfaceGl::create( app::getWindow(), "Params", vec2( 400, 350 ) );
	mParams->addParam( "Center", &mCenter ).updateFn( bind( &TextParticlesApp::setupVBO, this ) );
	mParams->addParam( "Start Velocity", &mStartVelocity ).updateFn( bind( &TextParticlesApp::setupVBO, this ) );
	mParams->addParam( "Step Max", &mStepMax );
	mParams->addParam( "Damping Speed", &mDampingSpeed ).precision( 4 ).step( 0.0005 ).min( 0.0 ).max( 0.04 );
	mParams->addParam( "Damping Base", &mDampingBase ).precision( 2 ).step( 0.05 ).min( 0.0 ).max( 1.0 );
	mParams->addParam( "Noise Offset", &mNoiseOffset );
	mParams->addParam( "EndColor", &mEndColor );
	mParams->addButton( "Enter Edit Mode", bind( &TextParticlesApp::editMode, this ) );
	mParams->addButton( "EXPLODE!", bind( &TextParticlesApp::setupVBO, this ) );
	
	mStep = 1.0;
	mActive = false;
}


void TextParticlesApp::setupVBO()
{
	if( mString.length() == 0 )
		return;
	
	// amount of pixels in the biffer is dependent on the text area
	int w = mTextSize.x, h = mTextSize.y;
	int totalParticles = w * h;
	mTextParticleCount = totalParticles;
	
	// INITIALIZE the particles
	vector<Particle> particles;
	particles.assign( totalParticles, Particle() );
	
	// CENTER of the text texture
	vec3 center = vec3(mTextSize.x/2.0f, mTextSize.y/2.0f, 0 ) + mCenter;
	for( int i = 0; i < particles.size(); ++i )
	{	// assign starting values to particles.
		int x = i % w;
		int y = floor( float(i) / float(w) );
		int z = 1.0;
		
		vec3 pos	   = vec3( x, y, z );
		vec3 dir	   = normalize( pos - center );
		vec3 offsetVel = ( dir * vec3( mStartVelocity ) );
		ColorA color   = mTextSurf.getPixel( ivec2( x, y ) );
		
		auto &p = particles.at( i );
		p.pos      = pos;
		p.texcoord = vec2( float(x) / float(w), float(y) / float(h) );
		p.ppos	   = p.pos - offsetVel;
		p.damping  = Rand::randFloat( mDampingBase, mDampingBase + 0.2f );
		p.color    = ColorA( Color(color), color.a );
		p.invmass  = Rand::randFloat( 0.1f, 1.0f );
	}
	
	// Create particle buffers on GPU and copy data into the first buffer.
	// Mark as static since we only write from the CPU once.
	mParticleBuffer[mSourceIndex] = gl::Vbo::create( GL_ARRAY_BUFFER, particles.size() * sizeof(Particle), particles.data(), GL_STATIC_DRAW );
	mParticleBuffer[mDestinationIndex] = gl::Vbo::create( GL_ARRAY_BUFFER, particles.size() * sizeof(Particle), nullptr, GL_STATIC_DRAW );
	
	for( int i = 0; i < 2; ++i )
	{	// Describe the particle layout for OpenGL.
		mAttributes[i] = gl::Vao::create();
		gl::ScopedVao vao( mAttributes[i] );
		
		// Define attributes as offsets into the bound particle buffer
		gl::ScopedBuffer buffer( mParticleBuffer[i] );
		gl::enableVertexAttribArray( 0 );
		gl::enableVertexAttribArray( 1 );
		gl::enableVertexAttribArray( 2 );
		gl::enableVertexAttribArray( 3 );
		gl::enableVertexAttribArray( 4 );
		gl::enableVertexAttribArray( 5 );
		gl::vertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, pos ) );
		gl::vertexAttribPointer( 1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, color ) );
		gl::vertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, ppos ) );
		gl::vertexAttribPointer( 3, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, damping ) );
		gl::vertexAttribPointer( 4, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, texcoord ) );
		gl::vertexAttribPointer( 5, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, invmass ) );
	}
	
	// ANIMATE mStep
	timeline().apply( &mStep, 1.0f, mStepMax, 1.0f );
	
	// As long as we're active, we'll draw the particles
	mActive = true;
}


void TextParticlesApp::mouseDown( MouseEvent event )
{
	mCamUi.mouseDown( event );
}


void TextParticlesApp::mouseUp( MouseEvent event )
{
	mCamUi.mouseUp( event );
}


void TextParticlesApp::mouseWheel( MouseEvent event )
{
	mCamUi.mouseWheel( event );
}


void TextParticlesApp::mouseDrag( MouseEvent event )
{
	Rectf r	= Rectf( 0, 0, getWindowWidth(), getWindowHeight() );
	if ( r.contains( event.getPos() )) {
		mCamUi.mouseDrag( event );
	}
}


void TextParticlesApp::editMode()
{
	mActive = false;
}


void TextParticlesApp::keyDown( KeyEvent event )
{
	/*
	console() << "------- " << endl;
	CI_LOG_V( "getChar()\t\t\t| " << event.getChar() );
	CI_LOG_V( "getCharUtf32()\t\t| " << event.getCharUtf32() );
	CI_LOG_V( "getCode()\t\t\t| " <<  event.getCode() );
	CI_LOG_V( "getNativeKeyCode()\t| " << event.getNativeKeyCode() );
	*/
	
	switch( event.getCode() ){
		case KeyEvent::KEY_RETURN:
			// EXPLODE
			setupVBO();
			break;
		
		case KeyEvent::KEY_BACKSPACE:
			editMode();
	
			// REMOVE last character
			if( mString.length() > 0 ){
				mString.pop_back();
			}
			drawTextToFbo();
			break;
		
		default:
			if( event.isControlDown() && event.getCode() == KeyEvent::KEY_r ){
				editMode();
			}
			else if( event.getChar() ){
				
				editMode();
				// ADD new character
				mString.append( string( 1, event.getChar() ) );
				drawTextToFbo();
			}
			
		break;
	}
}


void TextParticlesApp::update()
{
	if( !mActive )
		return;

	// Update particles on the GPU
	gl::ScopedGlslProg prog( mUpdateProg );
	gl::ScopedState rasterizer( GL_RASTERIZER_DISCARD, true );	// turn off fragment stage
	mPerlin3dTex->bind(0);
	mUpdateProg->uniform( "uPerlinTex", 0 );
	mUpdateProg->uniform( "uStep", mStep.value() );
	mUpdateProg->uniform( "uDampingSpeed", mDampingSpeed );
	mUpdateProg->uniform( "uNoiseOffset", mNoiseOffset );
	mUpdateProg->uniform( "uEndColor", mEndColor );
	
	// Bind the source data (Attributes refer to specific buffers).
	gl::ScopedVao source( mAttributes[mSourceIndex] );
	// Bind destination as buffer base.
	gl::bindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, mParticleBuffer[mDestinationIndex] );
	gl::beginTransformFeedback( GL_POINTS );

	// Draw source into destination, performing our vertex transformations.
	gl::drawArrays( GL_POINTS, 0, mTextParticleCount );
	gl::endTransformFeedback();
	
	mPerlin3dTex->unbind();
	
	// Swap source and destination for next loop
	std::swap( mSourceIndex, mDestinationIndex );
}


void TextParticlesApp::drawTextToFbo()
{
	{
		vec2 size = mTextFbo->getSize();
		gl::ScopedFramebuffer scpFbo( mTextFbo );
		gl::clear( ColorA( 0 , 0, 0, 0 ) );
		gl::ScopedViewport scpVp( ivec2(), size );
		gl::ScopedMatrices scpMtrx;
		gl::ScopedColor scpCol( 1, 1, 1);
		
		vec2 stringSize = mTextureFont->measureString( mString );
		float descent = mTextureFont->getDescent();
		float ascent = mTextureFont->getAscent();
		mTextSize = stringSize + vec2( 0, descent );
		
		// OPTIONALLY, draw the text metrics
		/*
		// string size - RED
		gl::color(1, 0, 0);
		gl::drawSolidRect( Rectf( -10, 0, stringSize.x + 10.0, stringSize.y ) );
		// ascent - CYAN
		gl::color(0, 1, 1);
		gl::drawSolidRect( Rectf( 0, stringSize.y - ascent, stringSize.x, ascent ) );
		// descent - YELLOW
		gl::color(1, 1, 0);
		gl::drawSolidRect( Rectf( 0, stringSize.y-descent, stringSize.x, stringSize.y ) );
		// above ascent
		gl::color( 0, 1, 0 );
		gl::drawSolidRect( Rectf( 0, 0, stringSize.x, stringSize.y-ascent ) );*/
		
		// DRAW string to FBO
		gl::color(1, 1, 1);
		mTextureFont->drawString( mString, vec2( 0, stringSize.y - descent ) );
	}
	
	// GET Texture from FBO
	mTextSurf = Surface( mTextFbo->readPixels8u( mTextFbo->getBounds() ) );
}


// Sets the current view matrix to fit a fullscreen texture exactly in the camera
void TextParticlesApp::lookAtTexture( const CameraPersp &cam, const ci::vec2 &size  )
{
	auto ctx = gl::context();
	ctx->getModelMatrixStack().back() = mat4();
	ctx->getProjectionMatrixStack().back() = cam.getProjectionMatrix();
	ctx->getViewMatrixStack().back() = cam.getViewMatrix();
	ctx->getViewMatrixStack().back() *= glm::scale( vec3( 1, -1, 1 ) );									// invert Y axis so increasing Y goes down.
	ctx->getViewMatrixStack().back() *= glm::translate( vec3( size.x / 2, (float) - size.y / 2, 0 ) );	// shift origin up to upper-left corner.
}


void TextParticlesApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	gl::color( 1, 1, 1 );
	gl::enableAlphaBlending();
	
	{
		gl::ScopedMatrices scpMtrx;
		
		// SET matrices so that by default, we are looking at a rect the size of the window
		lookAtTexture( mCam, getWindowSize() );
		
		gl::translate( mTextSize * vec2( -0.5 ) );
		
		gl::ScopedDepth scpDepth(	true );
//		gl::ScopedColor scpColor( 1, 0, 0 );
//		gl::drawStrokedRect( Rectf( 0, 0, mTextSize.x, mTextSize.y ) );
		
		gl::color( Color::white() );
		if( mActive ){
			gl::ScopedGlslProg render( mRenderProg );
			gl::ScopedVao vao( mAttributes[mSourceIndex] );
			gl::context()->setDefaultShaderVars();
			gl::drawArrays( GL_POINTS, 0, mTextParticleCount );
			
		}else{
			if( mString.length() > 0 )
				gl::draw( mTextFbo->getColorTexture() );
		}
		
	}
	
	mParams->draw();
}


CINDER_APP( TextParticlesApp, RendererGl, [] ( App::Settings *settings ) {
	settings->setWindowSize( 1280, 720 );
}  )
