#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/CameraUi.h"
#include "cinder/Log.h"
#include "cinder/Rand.h"
#include "cinder/Timeline.h"

using namespace ci;
using namespace ci::app;
using namespace std;

const int COUNT_X = 350;	// make this same as window width
const int COUNT_Y = 100;	// make this the same as text max height
const int PARTICLE_NUM = COUNT_X * COUNT_Y;

struct Particle
{
	vec3	pos;
	vec3	home;
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
	
	void textToTexture();
	void lookAtTexture( const CameraPersp &cam, const ci::vec2 &size );
	void setupPingPongFbo();
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
	
	gl::GlslProgRef		mUpdateProg, mRenderProg;
	gl::TextureRef		mStaticNoiseTex, mPerlin3dTex;
	
	bool				mActive;
//	gl::TextureRef		mTextTexture;

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
	
	
};

void TextParticlesApp::setup()
{
	// LOAD fonts
	
	// SET UP camera
	mCam  = CameraPersp( getWindowWidth(), getWindowHeight(), 60, 1.0, 1000.0  );
//	mCam.setPerspective( 60, getWindowAspectRatio(), 1, 1000 );
//	gl::setMatricesWindowPersp( getWindowWidth(), getWindowHeight() );
//	mCam.setEyePoint( vec3( 0, 0, 10.0 ) );
//	mCam.lookAt( vec3() );
	mCamUi = CameraUi( &mCam );
	
//	Font( loadAsset( "SourceSansPro-Bold.ttf" ), 16 );
	mFont = Font( loadAsset( "SourceSansPro-Bold.ttf" ), 80 );
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
			.feedbackVaryings( { "position", "pposition", "home", "color", "damping", "texcoord", "invmass" } )
			.attribLocation( "iPosition", 0 )
			.attribLocation( "iColor", 1 )
			.attribLocation( "iPPosition", 2 )
			.attribLocation( "iHome", 3 )
			.attribLocation( "iDamping", 4 )
			.attribLocation( "iTexCoord", 5 )
			.attribLocation( "iInvMass", 6 )
			);
	
	
	mActive = false;
//	mTextSurface = textSurf;
//	mTextTexture = gl::Texture::create( mTextSurface );
//	mSize = mTextTexture->getSize() * 0.5;
	
//	mString = "TEST";
//	drawTextToFbo();
	setupPingPongFbo();

//	setupVBO();
	
}

void TextParticlesApp::setupPingPongFbo()
{
	/*std::vector<Surface32f> surfaces;
	
	int w = 500, h = 500;
	// Position 2D texture array
	surfaces.push_back( Surface32f( w, h, true ) );
	// We don't need to do below because these are the positions that we want. We just want a grid. For now
	Surface32f::Iter pixelIter = surfaces[0].getIter();
	while( pixelIter.line() ) {
		while( pixelIter.pixel() ) {
			// Initial particle positions are passed in as R,G,B
			// float values. Alpha is used as particle invMass.
			float invMass = Rand::randFloat( 0.1f, 1.0f );
			surfaces[0].setPixel( pixelIter.getPos(),
								  ColorAf( pixelIter.getPos().x - ( w / 2 ),
								  pixelIter.getPos().y - ( h / 2 ),
								  0.0,
								  invMass ) );
		}
	}
	mOrigPosSurf = surfaces[0].clone();

	//Velocity 2D texture array
	surfaces.push_back( Surface32f( w, h, true ) );
	pixelIter = surfaces[1].getIter();
	while( pixelIter.line() ) {
		while( pixelIter.pixel() ) {
			// Initial particle velocities are
			// passed in as R,G,B float values.
			// texture alpha is in alpha value
			//            surfaces[1].setPixel( pixelIter.getPos(), ColorAf( 0.0f, 0.0f, 0.0f, 1.0f ) );
			float textureColor = mTextSurface.getPixel( pixelIter.getPos() * ivec2( 2 ) ).a;
			if( textureColor > 0 )
				textureColor = 1.0;
			else
				textureColor = 0.0;
			surfaces[1].setPixel( pixelIter.getPos(), ColorAf( 0.0f, 0.0f, 0.0f, textureColor ) );
		}
	}
	
//	console() << "pixel 1" << mTextSurface.getPixel( Vec2i::zero() ) << endl;
	mOrigVelSurf = surfaces[1].clone();

	mParticlesFboRef = std::make_shared<PingPongFbo>( surfaces );*/
}

void TextParticlesApp::setupVBO()
{
	if( mString.length() == 0 )
		return;
	
	mStep = 1.0;
	int w = COUNT_X, h = COUNT_Y;
	int totalParticles = w * h;
	
	vector<Particle> particles;
	particles.assign( totalParticles, Particle() );
	
	// turn fbo texture into surface
	
	
	// We want to change the position of the particles
	// need tex coord to determine which particle
	
	vec3 center = vec3( mTextSize.x/2.0f, mTextSize.y/2.0f, -10.0 );
//	vec3 center = vec3( mTextSize.x/2.0f, 0.0, -10.0 );
	
//	CI_LOG_V( center );
	for( int i = 0; i < particles.size(); ++i )
	{	// assign starting values to particles.
		int x = i % w;
		int y = floor( float(i) / float(w) );
		int z = 1.0;
		vec3 pos = vec3( x, y, z );
		vec3 dir = normalize(pos - center);
//		vec3 offsetVel = ( dir * ( Rand::randVec3() * vec3( 0.01 ) ) );
		vec3 offsetVel = ( dir * vec3( 10.0 ) );
//		vec3 offsetVel = ( dir * ( Rand::randVec3() * vec3( 1.0 ) ) );
//		CI_LOG_V( pos << " " << dir << " " << offsetVel );
		
		ColorA color = mTextSurf.getPixel( ivec2(x, y) );
		
		auto &p = particles.at( i );
		p.pos = pos;
		p.texcoord = vec2( float(x) / float(w), float(y) / float(h) );
		p.home = p.pos;
//		p.ppos = Rand::randVec3() * 10.0f; // random initial velocity
//		p.ppos = p.pos + ( Rand::randVec3() * vec3( 0.01 ) );
//		p.ppos = p.pos + offsetVel;		// THIS gives reverse dark side of the moon like movement
		p.ppos = p.pos - offsetVel;
//		p.ppos = p.pos;
//		p.damping = Rand::randFloat( 0.965f, 0.985f );
		p.damping = Rand::randFloat( 0.55f, 0.6f );
//		p.damping = Rand::randFloat( 0.3f, 0.5f );
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
		gl::enableVertexAttribArray( 6 );
		gl::vertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, pos ) );
		gl::vertexAttribPointer( 1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, color ) );
		gl::vertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, ppos ) );
		gl::vertexAttribPointer( 3, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, home ) );
		gl::vertexAttribPointer( 4, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, damping ) );
		gl::vertexAttribPointer( 5, 2, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, texcoord ) );
		gl::vertexAttribPointer( 6, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (const GLvoid*)offsetof(Particle, invmass ) );
	}
	

	

/*
	// create some geometry using a geom::Plane
	auto plane = geom::Plane().size( vec2( 500, 500 ) ).subdivisions( ivec2( 200, 50 ) );

	// Specify two planar buffers - positions are dynamic because they will be modified
	// in the update() loop. Tex Coords are static since we don't need to update them.
	vector<gl::VboMesh::Layout> bufferLayout = {
		gl::VboMesh::Layout().usage( GL_DYNAMIC_DRAW ).attrib( geom::Attrib::POSITION, 3 ),
		gl::VboMesh::Layout().usage( GL_STATIC_DRAW ).attrib( geom::Attrib::TEX_COORD_0, 2 ),
		gl::VboMesh::Layout().usage( GL_STATIC_DRAW ),attib( geom::Attrib::)
	};

	mVboMesh = gl::VboMesh::create( plane, bufferLayout );
	
	
	//	app::console() << "SET UP VBO" << endl;

	//  A dummy VboMesh the same size as the
	// texture to keep the vertices on the GPU
	
	vector<vec2> texCoords;
	vector<uint32_t> indices;
	gl::VboMesh::Layout layout;
	gl::VboRef indexVbo = gl::Vbo::create(<#GLenum target#>, <#const std::vector<T> &v#>)

	layout.setStaticIndices();
	layout.setStaticPositions();
	layout.setStaticTexCoords2d();
	layout.setStaticNormals();
	glPointSize( 1.0f );
	mVboMesh = gl::VboMesh::create( totalVertices, totalVertices, layout, GL_POINTS );
	for( int x = 0; x < w; ++x ) {
		for( int y = 0; y < h; ++y ) {
			indices.push_back( x * h + y );
			texCoords.push_back( vec2( x / w, y / h ) );
		}
	}
	mVboMesh->bufferIndices( indices );
	mVboMesh->bufferTexCoords2d( 0, texCoords );
	mVboMesh->unbindBuffers();
*/
	timeline().apply( &mStep, 1.0f, 100.0f, 1.0f );
	mActive = true;
}

void TextParticlesApp::textToTexture()
{

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
			// REMOVE last character
			drawTextToFbo();

			break;
		
		default:
			if( event.getChar() ){
				// ADD new character
				mString.append( string( 1, event.getChar() ) );
				drawTextToFbo();
	//			CI_LOG_V( mString );
	//			setupVBO();eg
				CI_LOG_V( "mString\t\t\t| " << mString );
			}
			
		break;
	}
	
}

void TextParticlesApp::update()
{
	if( !mActive )
		return;
	
//	mStep = 10.0;
	
	

	// Update particles on the GPU
	gl::ScopedGlslProg prog( mUpdateProg );
	gl::ScopedState rasterizer( GL_RASTERIZER_DISCARD, true );	// turn off fragment stage
	mStaticNoiseTex->bind(0);
	mPerlin3dTex->bind(1);
	mUpdateProg->uniform( "uNoiseTex", 0 );
	mUpdateProg->uniform( "uPerlinTex", 1 );
	
//	mUpdateProg->uniform( "uMouseForce", mMouseForce );
//	mUpdateProg->uniform( "uMousePos", mMousePos );
//	CI_LOG_V( mStep.value() );
	mUpdateProg->uniform( "uStep", mStep.value() );
	
	// Bind the source data (Attributes refer to specific buffers).
	gl::ScopedVao source( mAttributes[mSourceIndex] );
	// Bind destination as buffer base.
	gl::bindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, 0, mParticleBuffer[mDestinationIndex] );
	gl::beginTransformFeedback( GL_POINTS );

	// Draw source into destination, performing our vertex transformations.

	gl::drawArrays( GL_POINTS, 0, PARTICLE_NUM );

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

	//	gl::ScopedMatrices scpMatrx;
	//	mTextureFont->drawString( mString, stringSize * vec2( -0.5, 0 ) );
		vec2 centerPos = (size * vec2( 0.5 )) + (stringSize * vec2( -0.5, 0 ) );
		CI_LOG_V( stringSize.y << " " << ascent << " " << descent );
//		mTextureFont->drawString( mString,  centerPos );
		
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
//	mTextSurf = Surface( mTextFbo->getColorTexture()->createSource() );
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
//		gl::setMatrices( mCam );
//		gl::setMatricesWindowPersp( getWindowWidth(), getWindowHeight(), 60, 1.0, 1000.0, true );
		
		// SET matrices so that by default, we are looking at a rect the size of the window
		lookAtTexture( mCam, getWindowSize() );
//		gl::translate( ivec2(-getWindowCenter()) );
		
//		gl::setMatr( getWindowWidth(), getWindowHeight() );
		
//		gl::ScopedDepth scpDepth(	true );
	
		gl::ScopedColor scpColor( 1, 0, 0 );
		
//		gl::drawSolidRect( Rectf( 10, 10, getWindowWidth() - 10, getWindowHeight() - 10 ) );


//		gl::translate( vec3( 0, 0, 10.0 ) );
//		gl::drawSolidRect( getWindowBounds() - getWindowCenter() );
//		Rectf boundsRect( 40, mTextureFont->getAscent() + 40, getWindowWidth() - 40, getWindowHeight() - 40 );
//		mTextureFont->drawString( "Hello", vec2() );
		/*
		{
			vec2 stringSize = mTextureFont->measureString( mString );
			gl::ScopedMatrices scpMatrx;
//			gl::translate( stringSize * vec2( -0.5 ) );
			mTextureFont->drawString( mString, stringSize * vec2( -0.5, 0 ) );
		}
		*/
		
		gl::color( Color::white() );
//		gl::draw( mTextFbo->getColorTexture() );
		
//		mTextureFont->draw
//		gl::drawSolidRect( Rectf( 0, 0, 10, 10 ) );
//		gl::drawColorCube( vec3(), vec3( 10, 10, 10 ) );

		gl::translate( mTextSize * vec2( -0.5 ) );
		if( mActive ){
			gl::ScopedGlslProg render( mRenderProg );
			gl::ScopedVao vao( mAttributes[mSourceIndex] );
			gl::context()->setDefaultShaderVars();
			gl::drawArrays( GL_POINTS, 0, PARTICLE_NUM );
			
		}else{
			gl::draw( mTextFbo->getColorTexture() );
		}
//		gl::drawSolidRect( Rectf( 0, 0, mTextSize.x, mTextSize.y ) );
	}
	
	

	
//	gl::color( 1, 1, 0 );
//	gl::TextureRef tex = gl::Texture::create( mTextSurf );
//	gl::draw( tex, vec2( 100, 0 ) );
}

CINDER_APP( TextParticlesApp, RendererGl, [] ( App::Settings *settings ) {
	settings->setWindowSize( 1280, 720 );
}  )
