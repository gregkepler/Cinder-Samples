#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/gl/BufferTexture.h"
#include "cinder/Log.h"
#include "cinder/params/Params.h"
#include "SpiderWeb.h"

using namespace ci;
using namespace ci::app;
using namespace std;

const uint32_t MAX_POINTS = 50000;
const uint32_t POSITION_INDEX		= 0;
const uint32_t VELOCITY_INDEX		= 1;
const uint32_t CONNECTION_INDEX		= 2;
const uint32_t CONNECTION_LEN_INDEX	= 3;

typedef class Options {
	public:
		Options()
		: mSpringConstant( 8.0 ), mDamping( 2.8 ), mTension( 1.0 ), mTimestep( 0.07f )
		{ }
		
		// STREND of strand spring
		Options& springConstant( float constant ) { mSpringConstant = constant; return *this; }
		float getSpringConstant() const { return mSpringConstant; }
	
		// AMOUNT of damping
		Options& damping( float damping ) { mDamping = damping; return *this; }
		float getDamping() const { return mDamping; }
	
		// AMOUNT of tension
		Options& tension( float tension ) { mTension = tension; return *this; }
		float getTension() const { return mTension; }
	
		// AMOUNT of time to step ahead per iteration
		Options& timestep( float timestep ) { mTimestep = timestep; return *this; }
		float getTimestep() const { return mTimestep; }
		
	float			mSpringConstant;
	float			mDamping;
	float			mTension;
	float			mTimestep;
		
	private:
	
	} Options;



class SpiderWebApp : public App {
  public:
	SpiderWebApp();
	void update() override;
	void draw() override;
	
	void mouseDown( MouseEvent event ) override;
	void mouseDrag( MouseEvent event ) override;
	void mouseUp( MouseEvent event ) override;
	void updateRayPosition( const ci::ivec2 &mousePos, bool useDistance );
	
	void reset();
	void generateWeb();
	void setupBuffers();
	void setupGlsl();
	
	SpiderWebRef	mWeb;
	int				mConnectionCount;
	
	std::array<gl::VaoRef, 2>			mVaos;
	std::array<gl::VboRef, 2>			mPositions, mVelocities, mConnections, mConnectionLen;
	std::array<gl::BufferTextureRef, 2>	mPositionBufTexs;
	gl::VboRef							mLineIndices;
	gl::GlslProgRef						mUpdateGlsl, mRenderGlsl;
	uint32_t							mIterationsPerFrame, mIterationIndex;
	CameraPersp							mCam;
	float								mCurrentCamRotation;
	params::InterfaceGlRef				mParams;
	std::shared_ptr<Options>			mOptions;
};

SpiderWebApp::SpiderWebApp()
: mIterationsPerFrame( 2 ), mIterationIndex( 0 ),
	mCurrentCamRotation( 0.0f ),
	mCam( getWindowWidth(), getWindowHeight(), 60.0f, 0.01f, 1000.0f )
{
//	vec3 eye = vec3( getWindowCenter().x, getWindowCenter().y, 1000.0f );
//	vec3 target = vec3( getWindowCenter().x, getWindowCenter().y, 0 );
//	mCam.lookAt( eye, target );
	
	// set up params
	static vec3 gravity = vec3( 0.0, 0.01, 0.0 );
	mParams = params::InterfaceGl::create( getWindow(), "App parameters", toPixels( ivec2( 200, 300 ) ) );
	mOptions = make_shared<Options>();
	mOptions->springConstant( 8.5 );
	mOptions->tension( 0.8 );
	mOptions->timestep( 0.2 );
	
	mParams->addParam( "spring constant", &mOptions->mSpringConstant ).min( 0.1f ).max( 20.5f ).keyIncr( "z" ).keyDecr( "Z" ).precision( 2 ).step( 0.25f ).updateFn(
	[&](){
		mUpdateGlsl->uniform( "k", mOptions->getSpringConstant() );
	});
	mParams->addParam( "Gravity", &gravity ).updateFn(
	[&](){
		mUpdateGlsl->uniform( "gravity", gravity );
	});
	mParams->addParam( "Damping Constant", &mOptions->mDamping ).min( 2.0f ).max( 25.0f ).precision( 2 ).step( 0.1f ).updateFn(
		[&](){
			mUpdateGlsl->uniform( "c", mOptions->getDamping() );
		});
	mParams->addParam( "Tension", &mOptions->mTension ).min( 0.1f ).max( 2.0f ).precision( 2 ).step( 0.1f ).updateFn(
		[&](){
			mUpdateGlsl->uniform( "tension", mOptions->getTension() );
		});
	mParams->addParam( "Timestep", &mOptions->mTimestep ).min( 0.01f ).max( 0.5f ).precision( 2 ).step( 0.01f ).updateFn(
		[&](){
			mUpdateGlsl->uniform( "t", mOptions->getTimestep() );
		});
	mParams->addSeparator();
	mParams->addButton( "Randomize Web", bind( &SpiderWebApp::reset, this ) );
	
	setupGlsl();
	generateWeb();
	
	gl::enable( GL_LINE_SMOOTH );
}


void SpiderWebApp::reset()
{
	CI_LOG_V( "RESET" );
	mWeb->reset();
	mWeb = nullptr;
	
	mIterationIndex = 0;
	mPositionBufTexs[0] = nullptr;
	mPositionBufTexs[1] = nullptr;
	generateWeb();
}


void SpiderWebApp::generateWeb()
{
	randSeed( (int32_t)time( NULL ) );
//	randSeed( 50 );
	mWeb = SpiderWeb::create( SpiderWeb::Options()
		.anchorCount( randInt(3, 8) )
		.radiusBase( getWindowWidth() / 2.0 )
		.rayPointCount( 20 )
		.raySpacing( randFloat( 80.0, 150.0) )
	);
	mWeb->make();
	
	setupBuffers();
}

void SpiderWebApp::setupBuffers()
{
	// reset
	
	/*mVaos[0] = nullptr;
	mVaos[1] = nullptr;
	mPositions[0] = nullptr;
	mPositions[1] = nullptr;
	mVelocities[0] = nullptr;
	mVelocities[1] = nullptr;
	mConnections[0] = nullptr;
	mConnections[1] = nullptr;
	mConnectionLen[0] = nullptr;
	mConnectionLen[1] = nullptr;
	mLineIndices = nullptr;
	mPositionBufTexs[0] = nullptr;
	mPositionBufTexs[1] = nullptr;*/
	
	// get all of the points from the spider web
	
	 // buffer the positions
	std::array<vec4, MAX_POINTS> positions;
	std::array<vec3, MAX_POINTS> velocities;
	std::array<ivec4, MAX_POINTS> connections;
	std::array<vec4, MAX_POINTS> connectionLen;
	
	vector<ParticleRef> webPoints = mWeb->getPoints();
	int n = 0;
	for( auto iter = webPoints.begin(); iter != webPoints.end(); ++iter ) {
		auto point = (*iter);
		vec2 pos = point->getPosition();
		// create our initial positions
		positions[point->getId()] = vec4(
			pos.x, pos.y,
			0.0,
//			randFloat() );
			1.0f );
		// zero out velocities
		velocities[point->getId()] = vec3( 0.0f );
		
		connections[n] = ivec4( -1 );
		connectionLen[n] = vec4( 0.0 );
		auto conn = point->getNeighbors();
		// use first 4 connections of there are more
		int max = min(int(conn.size()), 4);

		for( int i = 0; i < max; ++i ){
			auto connection = conn[i];
			connections[n][i] = connection->getId();
//			connectionLen[n][i] = distance( normalize(pos), normalize(connection->getPosition()) );
			connectionLen[n][i] = distance( pos, connection->getPosition() );
		}
		n++;
	}
	
	for ( int i = 0; i < 2; i++ ) {
		mVaos[i] = gl::Vao::create();
		gl::ScopedVao scopeVao( mVaos[i] );
		{
			// buffer the positions
			mPositions[i] = gl::Vbo::create( GL_ARRAY_BUFFER, positions.size() * sizeof(vec4), positions.data(), GL_STATIC_DRAW );
			{
				// bind and explain the vbo to your vao so that it knows how to distribute vertices to your shaders.
				gl::ScopedBuffer sccopeBuffer( mPositions[i] );
				gl::vertexAttribPointer( POSITION_INDEX, 4, GL_FLOAT, GL_FALSE, 0, (const GLvoid*) 0 );
				gl::enableVertexAttribArray( POSITION_INDEX );
			}
			
			// buffer the velocities
			mVelocities[i] = gl::Vbo::create( GL_ARRAY_BUFFER, velocities.size() * sizeof(vec3), velocities.data(), GL_STATIC_DRAW );
			{
				// bind and explain the vbo to your vao so that it knows how to distribute vertices to your shaders.
				gl::ScopedBuffer scopeBuffer( mVelocities[i] );
				gl::vertexAttribPointer( VELOCITY_INDEX, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*) 0 );
				gl::enableVertexAttribArray( VELOCITY_INDEX );
			}
			// buffer the connections
			mConnections[i] = gl::Vbo::create( GL_ARRAY_BUFFER, connections.size() * sizeof(ivec4), connections.data(), GL_STATIC_DRAW );
			{
				// bind and explain the vbo to your vao so that it knows how to distribute vertices to your shaders.
				gl::ScopedBuffer scopeBuffer( mConnections[i] );
				gl::vertexAttribIPointer( CONNECTION_INDEX, 4, GL_INT, 0, (const GLvoid*) 0 );
				gl::enableVertexAttribArray( CONNECTION_INDEX );
			}
			// buffer the connection lengths
			mConnectionLen[i] = gl::Vbo::create( GL_ARRAY_BUFFER, connectionLen.size() * sizeof(vec4), connectionLen.data(), GL_STATIC_DRAW );
			{
				// bind and explain the vbo to your vao so that it knows how to distribute vertices to your shaders.
				gl::ScopedBuffer scopeBuffer( mConnectionLen[i] );
				gl::vertexAttribPointer( CONNECTION_LEN_INDEX, 4, GL_FLOAT, GL_FLOAT, 0, (const GLvoid*) 0 );
				gl::enableVertexAttribArray( CONNECTION_LEN_INDEX );
			}
		}
	}
	// create your two BufferTextures that correspond to your position buffers.
	mPositionBufTexs[0] = gl::BufferTexture::create( mPositions[0], GL_RGBA32F );
	mPositionBufTexs[1] = gl::BufferTexture::create( mPositions[1], GL_RGBA32F );
	
	auto strands = mWeb->getUniqueStrands();
	int lines = strands.size();
	mConnectionCount = lines;
	// create the indices to draw links between the cloth points
	mLineIndices = gl::Vbo::create( GL_ELEMENT_ARRAY_BUFFER, lines * 2 * sizeof(int), nullptr, GL_STATIC_DRAW );
//
	auto e = (int *) mLineIndices->mapReplace();
	for( auto iter = strands.begin(); iter != strands.end(); ++iter ){
		auto strand = *iter;
		*e++ = strand.first->getId();
		*e++ = strand.second->getId();
	}
	mLineIndices->unmap();
}


void SpiderWebApp::setupGlsl()
{
	// These are the names of our out going vertices. GlslProg needs to
	// know which attributes should be captured by Transform FeedBack.
	std::vector<std::string> feedbackVaryings({
		"tf_position_mass",
		"tf_velocity"
	});
	
	gl::GlslProg::Format updateFormat;
	updateFormat.vertex( loadAsset( "update.vert" ) )
				// Because we have separate buffers with which
				// to capture attributes, we're using GL_SEPERATE_ATTRIBS
				.feedbackFormat( GL_SEPARATE_ATTRIBS )
				// We also send the names of the attributes to capture
				.feedbackVaryings( feedbackVaryings );
	
	mUpdateGlsl = gl::GlslProg::create( updateFormat );
	// Set this, otherwise it will be set to vec3( 0, 0, 0 ),
	// which is in the center of the cloth
	mUpdateGlsl->uniform( "rayPosition", vec3( 0 ) );
	mUpdateGlsl->uniform( "gravity", vec3(0, 0.08f, 0) );
//	mUpdateGlsl->uniform( "rest_length", 20.0 );
	mUpdateGlsl->uniform( "c", mOptions->getDamping() );
	mUpdateGlsl->uniform( "k", mOptions->getSpringConstant() );
	mUpdateGlsl->uniform( "tension", mOptions->getTension() );
	mUpdateGlsl->uniform( "t", mOptions->getTimestep() );
	
	gl::GlslProg::Format renderFormat;
	renderFormat.vertex( loadAsset( "render.vert" ) )
				.fragment( loadAsset( "render.frag" ) );
	
	mRenderGlsl = gl::GlslProg::create( renderFormat );
}


void SpiderWebApp::mouseDown( MouseEvent event )
{
	updateRayPosition( event.getPos(), true );
}

void SpiderWebApp::mouseDrag( MouseEvent event )
{
	updateRayPosition( event.getPos(), true );
}

void SpiderWebApp::mouseUp( MouseEvent event )
{
	updateRayPosition( event.getPos(), false );
}

void SpiderWebApp::updateRayPosition( const ci::ivec2 &mousePos, bool useDistance )
{
//	auto ray = mCam.generateRay( mousePos, getWindowSize() );
//	auto dist = ci::distance( mCam.getEyePoint(), vec3() );
//	auto rayPosition = ray.calcPosition( useDistance ? dist : 0 );
//	vec3 rayPosition = vec3( mousePos.x, mousePos.y, 0.0 ) - vec3( getWindowCenter().x, getWindowCenter().y, 0.0 );
//	rayPosition /= vec3( getWindowWidth(), getWindowHeight(), 1.0 );
//	rayPosition *= vec3( 25.0, 25.0, 1.0 );
//	vec3 rayPosition = vec3( vec2(mousePos) / vec2(getWindowSize()), 0.0 );
	vec3 rayPosition = vec3();
	if( useDistance )
		rayPosition = vec3( vec2(mousePos), 0.0 );
	mUpdateGlsl->uniform( "rayPosition", rayPosition );
//	CI_LOG_V( rayPosition );
}

void SpiderWebApp::update()
{
	gl::ScopedGlslProg	scopeGlsl( mUpdateGlsl );
	gl::ScopedState		scopeState( GL_RASTERIZER_DISCARD, true );
	
	for( auto i = mIterationsPerFrame; i != 0; --i ) {
		// Bind the vao that has the original vbo attached,
		// these buffers will be used to read from.
		gl::ScopedVao scopedVao( mVaos[mIterationIndex & 1] );
		// Bind the BufferTexture, which contains the positions
		// of the first vbo. We'll cycle through the neighbors
		// using the connection buffer so that we can derive our
		// next position and velocity to write to Transform Feedback
		gl::ScopedTextureBind scopeTex( mPositionBufTexs[mIterationIndex & 1]->getTarget(), mPositionBufTexs[mIterationIndex & 1]->getId() );
		
		// We iterate our index so that we'll be using the
		// opposing buffers to capture the data
		mIterationIndex++;
		
		// Now bind our opposing buffers to the correct index
		// so that we can capture the values coming from the shader
		gl::bindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, POSITION_INDEX, mPositions[mIterationIndex & 1] );
		gl::bindBufferBase( GL_TRANSFORM_FEEDBACK_BUFFER, VELOCITY_INDEX, mVelocities[mIterationIndex & 1] );
		
		// Begin Transform feedback with the correct primitive,
		// In this case, we want GL_POINTS, because each vertex
		// exists by itself
		gl::beginTransformFeedback( GL_POINTS );
		// Now we issue our draw command which puts all of the
		// setup in motion and processes all the vertices
		gl::drawArrays( GL_POINTS, 0, MAX_POINTS );
		// After that we issue an endTransformFeedback command
		// to tell OpenGL that we're finished capturing vertices
		gl::endTransformFeedback();
	}
}

void SpiderWebApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) );
	gl::enableAlphaBlending();
	
//	gl::setMatrices( mCam );
	{
		gl::ScopedColor color;
		gl::color(1, 1, 1, 0.2);
//		mWeb->draw();
	}
	
	// Notice that this vao holds the buffers we've just
	// written to with Transform Feedback. It will show
	// the most recent positions
	gl::ScopedVao scopeVao( mVaos[mIterationIndex & 1] );
	gl::ScopedGlslProg scopeGlsl( mRenderGlsl );
//	gl::setMatrices( mCam );
	gl::setDefaultShaderVars();
	
	gl::ScopedColor color( Color::white() );
	
	
	// draw points
	gl::pointSize( 3.0f);
	gl::drawArrays( GL_POINTS, 0, MAX_POINTS );
	
	// draw lines
	gl::ScopedBuffer scopeBuffer( mLineIndices );
	gl::drawElements( GL_LINES, mConnectionCount * 2, GL_UNSIGNED_INT, nullptr );
	
	mParams->draw();
}

CINDER_APP( SpiderWebApp, RendererGl(),
[&]( App::Settings *settings ) {
	settings->setWindowSize( 1024, 768 );
})