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

struct Particle
{
	vec3	pos;
	vec3	ppos;
	ColorA  color;
	float	damping;
	vec2	texcoord;
	float	invmass;
};

class TextParticlesApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;
	void mouseMove( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseWheel( MouseEvent event ) override;
	void keyDown( KeyEvent event )override;
	void update() override;
	void draw() override;
	void drawTextToFbo();
	
	void lookAtTexture( const CameraPersp &cam, const ci::vec2 &size );
	void setupVBO();
	
	double				mTimer, mTextTimer;
	gl::TextureRef		mTextTexture;
	CameraPersp			mCam;
	CameraUi			mCamUi;
	
	Font				mFont;
	gl::TextureFontRef	mTextureFont;
	string				mString;
	gl::FboRef			mTextFbo;
	vec2				mTextSize;
	int					mTextParticleCount;
	
	gl::GlslProgRef		mUpdateProg, mRenderProg;
	gl::TextureRef		mStaticNoiseTex, mPerlin3dTex;
	
	bool				mActive;

	// store original position and texture so that we can reset everything later
	ci::Surface32f		mOrigPosSurf, mOrigVelSurf;
	gl::VboMeshRef		mVboMesh;
	ci::Surface8u		mTextSurf;
	
	ci::Anim<float>		mStep;
	
	
	// Descriptions of particle data layout.
	gl::VaoRef		mAttributes[2];
	// Buffers holding raw particle data on GPU.
	gl::VboRef		mParticleBuffer[2];

	// Current source and destination buffers for transform feedback.
	// Source and destination are swapped each frame after update.
	std::uint32_t	mSourceIndex		= 0;
	std::uint32_t	mDestinationIndex	= 1;
	
	// params
	params::InterfaceGlRef	mParams;
	vec3					mCenter;
	float					mStartVelocity;
	float					mStepMax;
	float					mDampingSpeed;
	float					mDampingBase;
	
	
};

void TextParticlesApp::setup()
{
	// SET UP camera
	mCam  = CameraPersp( getWindowWidth(), getWindowHeight(), 60, 1.0, 1000.0  );
	mCamUi = CameraUi( &mCam );
	

	// LOAD fonts
	mFont = Font( loadAsset( "SourceSansPro-Bold.ttf" ), 100 );
	mTextureFont = gl::TextureFont::create( mFont );
	
	mTextFbo = gl::Fbo::create( getWindowWidth(), getWindowHeight(), true );
	
	// LOAD shaders
	// TODO: live reload shaders
	try {
//		mParticleShader = gl::GlslProg::create( loadAsset( "shaders/passThrough.vert" ), loadAsset( "shaders/particles.frag" ) );
		// Vertex displacement shader
//		mDisplacementShader = gl::GlslProg::create( loadAsset( "shaders/vertexDisplacement.vert" ), loadAsset( "shaders/vertexDisplacement.frag" ) );
	}catch( ci::gl::GlslProgCompileExc &exc ) {
		std::cout << "Shader compile error: " << endl;
		std::cout << exc.what();
	}
	catch( Exception &exc ) {
		std::cout << "Unable to load shader" << endl;
		std::cout << exc.what();
	}
	
	// LOAD textures to use in the shaders
	mStaticNoiseTex = gl::Texture::create( loadImage( loadAsset( "shaders/staticNoise.png" ) ) );
	mStaticNoiseTex->setWrap( GL_REPEAT, GL_REPEAT );

	mPerlin3dTex = gl::Texture::create( loadImage( loadAsset( "shaders/perlin3d.png" ) ) );
	mPerlin3dTex->setWrap( GL_REPEAT, GL_REPEAT );
	
	mRenderProg = gl::getStockShader( gl::ShaderDef().color() );
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
	
	
	// set up params
	mCenter = vec3( 0, 0, -10.0 );
	mStartVelocity = 10.0;
	mStepMax = 10.0;
	mDampingSpeed = 0.004;
	mDampingBase = 0.3f;
	
	mParams = params::InterfaceGl::create( app::getWindow(), "Params", vec2( 400, 200 ) );
	mParams->addParam( "Center", &mCenter ).updateFn( bind( &TextParticlesApp::setupVBO, this ) );
	mParams->addParam( "Start Velocity", &mStartVelocity ).updateFn( bind( &TextParticlesApp::setupVBO, this ) );
	mParams->addParam( "Step Max", &mStepMax );
	mParams->addParam( "Damping Speed", &mDampingSpeed ).precision( 4 ).step( 0.0005 ).min( 0.0 ).max( 0.04 );
	mParams->addParam( "Damping Base", &mDampingBase ).precision( 2 ).step( 0.05 ).min( 0.0 ).max( 1.0 );
	
	mActive = false;
}


void TextParticlesApp::setupVBO()
{
	if( mString.length() == 0 )
		return;
	
	mStep = 1.0;
	int w = mTextSize.x, h = mTextSize.y;
	int totalParticles = w * h;
	mTextParticleCount = totalParticles;
	
	vector<Particle> particles;
	particles.assign( totalParticles, Particle() );
	
	// We want to change the position of the particles
	// need tex coord to determine which particle
	
	
//	vec3 center = vec3( mTextSize.x/2.0f, 0.0, -10.0 );
	vec3 center = vec3(mTextSize.x/2.0f, mTextSize.y/2.0f, 0 ) + mCenter;
	for( int i = 0; i < particles.size(); ++i )
	{	// assign starting values to particles.
		int x = i % w;
		int y = floor( float(i) / float(w) );
		int z = 1.0;
		vec3 pos = vec3( x, y, z );
		vec3 dir = normalize( pos - center );
		vec3 offsetVel = ( dir * vec3( mStartVelocity ) );
		ColorA color = mTextSurf.getPixel( ivec2( x, y ) );
		
		auto &p = particles.at( i );
		p.pos = pos;
		p.texcoord = vec2( float(x) / float(w), float(y) / float(h) );
//		p.ppos = Rand::randVec3() * 10.0f; // random initial velocity
//		p.ppos = p.pos + ( Rand::randVec3() * vec3( 0.01 ) );
//		p.ppos = p.pos + offsetVel;		// THIS gives reverse dark side of the moon like movement
		p.ppos = p.pos - offsetVel;
//		p.ppos = p.pos;
//		p.damping = Rand::randFloat( 0.965f, 0.985f );
//		p.damping = Rand::randFloat( 0.55f, 0.6f );
		p.damping = Rand::randFloat( mDampingBase, mDampingBase + 0.2f );
//		p.color = ColorA( 1, 1, 1, 1 );
		p.color = ColorA( Color(color), 1);
		p.invmass = Rand::randFloat( 0.1f, 1.0f );
//		if( color.r > 0)
//			CI_LOG_V( x << " - " << y << " " << color );
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
	

	timeline().apply( &mStep, 1.0f, mStepMax, 1.0f );
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

void TextParticlesApp::mouseMove( MouseEvent event )
{
//	mLastMousePos = event.getPos();
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


void TextParticlesApp::keyDown( KeyEvent event )
{
	// RESET a timer every time. Update in update loop. If timer reaches threshold of x seconds, explode
	console() << "------- " << endl;
	CI_LOG_V( "getChar()\t\t\t| " << event.getChar() );
	CI_LOG_V( "getCharUtf32()\t\t| " << event.getCharUtf32() );
	CI_LOG_V( "getCode()\t\t\t| " <<  event.getCode() );
	CI_LOG_V( "getNativeKeyCode()\t| " << event.getNativeKeyCode() );
	
	switch( event.getCode() ){
		case KeyEvent::KEY_RETURN:
			// EXPLODE
			setupVBO();
			break;
		
		case KeyEvent::KEY_BACKSPACE:
			mActive = false;
	
			if( mString.length() > 0 ){
				mString.pop_back();
			}
			// REMOVE last character
			drawTextToFbo();

			break;
		
		default:
			if( event.isControlDown() && event.getCode() == KeyEvent::KEY_r ){
				mActive = false;
			}
			else if( event.getChar() ){
				
				mActive = false;
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
	mStaticNoiseTex->bind(0);
	mPerlin3dTex->bind(1);
	mUpdateProg->uniform( "uNoiseTex", 0 );
	mUpdateProg->uniform( "uPerlinTex", 1 );
	mUpdateProg->uniform( "uStep", mStep.value() );
	mUpdateProg->uniform( "uDampingSpeed", mDampingSpeed );
	
	// Bind the source data (Attributes refer to specific buffers).
	gl::ScopedVao source( mAttributes[mSourceIndex] );
	// Bind destination as buffer base.
	gl::bindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, mParticleBuffer[mDestinationIndex] );
	gl::beginTransformFeedback( GL_POINTS );

	// Draw source into destination, performing our vertex transformations.

	gl::drawArrays( GL_POINTS, 0, mTextParticleCount );
	gl::endTransformFeedback();
	
	mStaticNoiseTex->unbind();
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
		
		
		// string
		gl::color(1, 1, 1);
		mTextureFont->drawString( mString, vec2( 0, stringSize.y - descent ) );
	}
	mTextSurf = Surface( mTextFbo->readPixels8u( mTextFbo->getBounds() ) );
}

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
//	gl::drawSolidRect( Rectf( 10, 10, 100, 100 ) );
	gl::enableAlphaBlending();
	

	{
		gl::ScopedMatrices scpMtrx;
		
		// SET matrices so that by default, we are looking at a rect the size of the window
		lookAtTexture( mCam, getWindowSize() );
		
//		gl::ScopedDepth scpDepth(	true );
	
		gl::ScopedColor scpColor( 1, 0, 0 );
		gl::color( Color::white() );

		gl::translate( mTextSize * vec2( -0.5 ) );
		if( mActive ){
			gl::ScopedGlslProg render( mRenderProg );
			gl::ScopedVao vao( mAttributes[mSourceIndex] );
			gl::context()->setDefaultShaderVars();
			gl::drawArrays( GL_POINTS, 0, mTextParticleCount );
			
		}else{
			if( mString.length() > 0 )
				gl::draw( mTextFbo->getColorTexture() );
		}
//		gl::drawSolidRect( Rectf( 0, 0, mTextSize.x, mTextSize.y ) );
	}
	
	mParams->draw();
}

CINDER_APP( TextParticlesApp, RendererGl, [] ( App::Settings *settings ) {
	settings->setWindowSize( 1280, 720 );
}  )
