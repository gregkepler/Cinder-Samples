//
//  SpiderWeb.cpp
//  SpiderWeb
//
//  Created by Greg Kepler on 12/18/15.
//
//


using ParticleRef = std::shared_ptr<class Particle>;

class Particle : public std::enable_shared_from_this<Particle> {
	
public:
	static std::shared_ptr<Particle> create( ci::vec2 pos )
	{
		auto p = std::make_shared<Particle>(); p->setup(pos); return p;
	}
	
	Particle() {};
	virtual ~Particle() {};
	
	void draw()
	{
//		ci::gl::drawSolidCircle( mPosition, 2 );
		// draw lines to neighbors
		for( auto iter = mNeighbors.begin(); iter != mNeighbors.end(); ++iter ){
			
			ci::gl::drawLine( ci::vec3( mPosition, 0.0f ), ci::vec3( (*iter)->getPosition(), 0.0f ) );
		}
	}
	
	void connectTo( ParticleRef pt )
	{
		// make sure they aren't neighbors already
		for( auto iter = mNeighbors.begin(); iter != mNeighbors.end(); ++iter )
		{
			if( *iter == pt ){
				return;
			}
		}
		
		mNeighbors.push_back( pt );
		
//		pt->connectTo( shared_from_this() );
	}
	
	void						setId( int num )	{ mId = num; }
	int							getId()				{ return mId; };
	ci::vec2					getPosition()		{ return ci::vec2( mPosition ); };
	std::vector<ParticleRef>	getNeighbors()		{ return mNeighbors; };
	
	
	
private:
	void setup( const ci::vec2 &pos )
	{
		mPosition = pos;
	}
	
	int							mId;
	ci::vec2					mPosition;
	std::vector<ParticleRef>	mNeighbors;
};

/*
 
 */

using WebRayRef = std::shared_ptr<class WebRay>;

class WebRay {
	
public:
	static std::shared_ptr<WebRay> create( int order, ParticleRef startPt, ParticleRef endPt, float noise )
	{
		auto p = std::make_shared<WebRay>();
		p->setup( order, startPt, endPt, noise );
		return p;
	}
	
	static float getRandomPoint( ci::vec2 vec, float range, float maxLength)
	{
		float pointDist = sqrt( ( vec.x * vec.x ) + ( vec.y * vec.y ) )
						+ ( ( ci::randFloat() * ( range * 2.0 ) ) - range );
		// make sure they are not connecting to points that don't exist
		pointDist = (pointDist > maxLength) ? maxLength : pointDist;
		return pointDist;
	};
	
	
	
	WebRay() {};
	virtual ~WebRay() {};
	
	void makePoints( ParticleRef webCenter, float avgLen, int pointCount );
	void connectStrands( const std::vector<WebRayRef> &rays );
	void connectRay();
	ParticleRef getPtByIndex( int index )		{ return mPoints[index]; };
	std::vector<ParticleRef> getPoints()		{ return mPoints; };
	std::vector<ParticleRef> getRayPoints()		{ return mRayPoints; };
	std::vector<ParticleRef> getAllPoints()		{ return mAllPoints; };
	
	void draw()
	{
//		ci::gl::drawSolidCircle( mPosition, 3 );
		// draw lines to neighbors
//		for( auto iter = mPoints.begin(); iter != mPoints.end(); ++iter ){
		for( auto iter = mRayPoints.begin(); iter != mRayPoints.end(); ++iter ){
			(*iter)->draw();
//			ci::gl::drawLine( ci::vec3( mPosition, 0.0f ), ci::vec3( (*iter)->getPosition(), 0.0f ) );
		}
	}
	
	std::vector<ParticleRef>	mRayPoints, mAllPoints;
	ParticleRef					mStartPt, mEndPt;
	float mAngle;
	
private:
	void setup( int order, ParticleRef &startPt, const ParticleRef &endPt, float noise )
	{
		mOrder = order;
		mStartPt = startPt;
		mEndPt = endPt;
		mNoise = noise;
	}
	
	void addRayPoint( ParticleRef p );
	void addStrand( ParticleRef thisPoint, float nextAngle, WebRayRef nextStrand, float pointDist );
	void addYStrand( ParticleRef thisPoint, ParticleRef nextPoint, float nextAngle, WebRayRef nextStrand, float pointDist );
	
	
	int							mOrder, mRayPointAmt;
	float						mNoise;
	std::vector<ParticleRef>	mPoints;
	
	ParticleRef					mWebCenter;
	float						mStrandLength;
};


// -----------------------------------------------------------------------------
//
// SpiderWeb
//
// -----------------------------------------------------------------------------

using SpiderWebRef = std::shared_ptr<class SpiderWeb>;

class SpiderWeb {
	
public:
	
	typedef class Options {
	public:
		Options()
		: mAnchorCount( 5 ), mRadiusBase( 200.0f ), mRayPointCount( 10 ), mRaySpacing( 40.0 )
		{ }
		
		// NUMBER of anchor strands
		Options& anchorCount( int count ) { mAnchorCount = count; return *this; }
		int getAnchorCount() const { return mAnchorCount; }
		
		// SIZE of radius base
		Options& radiusBase( float length ) { mRadiusBase = length; return *this; }
		float getRadiusBase() const { return mRadiusBase; }
		
		// NUMBER of points per strand
		Options& rayPointCount( int count ) { mRayPointCount = count; return *this; }
		int getRayPointCount() const { return mRayPointCount; }
		
		// SIZE of minimum spacing between rays
		Options& raySpacing( float spacing ) { mRaySpacing = spacing; return *this; }
		float getRaySpacing() const { return mRaySpacing; }
		
		
	private:
		int			mAnchorCount;
		float		mRadiusBase;
		int			mRayPointCount;
		float		mRaySpacing;
		
	} Options;
	
	
	SpiderWeb( const Options &options = Options() );
	~SpiderWeb(){};
	
	static std::shared_ptr<SpiderWeb> create( const Options &options = Options() )
	{
		return std::make_shared<SpiderWeb>( options );
	}

	
	// Returns 1 if the lines intersect, otherwise 0. In addition, if the lines
	// intersect the intersection point may be stored in the floats i_x and i_y.
	static ci::vec2 intersect( ci::vec2 pt1, ci::vec2 pt2, ci::vec2 pt3, ci::vec2 pt4 ){
		float p0_x = pt1.x; float p1_x = pt2.x; float p2_x = pt3.x; float p3_x = pt4.x;
		float p0_y = pt1.y; float p1_y = pt2.y; float p2_y = pt3.y; float p3_y = pt4.y;
		
		float s1_x, s1_y, s2_x, s2_y;
		s1_x = p1_x - p0_x;     s1_y = p1_y - p0_y;
		s2_x = p3_x - p2_x;     s2_y = p3_y - p2_y;
		
		float s, t;
		s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / (-s2_x * s1_y + s1_x * s2_y);
		t = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);
		
		ci::vec2 pos = ci::vec2( NAN, NAN );
		if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
		{
			// Collision detected
			pos.x = p0_x + (t * s1_x);
			pos.y = p0_y + (t * s1_y);
		}
		
		return pos;
	}


	
	void update();
	void draw();
	void make();
	void reset();
	std::vector<ParticleRef>	getPoints() { return mPoints; };
	std::vector<std::pair<ParticleRef, ParticleRef>> getStrands() { return mStrands; };
	std::vector<std::pair<ParticleRef, ParticleRef>> getUniqueStrands() { return mUniqueStrands; };
	
private:

	void		addAchors();
	void		addSubAnchors();
	ParticleRef makeParticle( const ci::vec2 &pos );
	void		addParticle( ParticleRef particle );
	void		addStrand( ParticleRef particle1, ParticleRef particle2 );
	ci::vec2	findEdgePoint( const ci::vec2 &origPos );
	
	std::vector<std::pair<ParticleRef, ParticleRef>>	mStrands, mUniqueStrands;
	std::vector<ParticleRef>			mPoints;
	std::vector<ParticleRef>			mAnchors;
	std::vector<ParticleRef>			mSubAnchors;
	ParticleRef							mWebCenter;
	float								mAvgLen;
	std::vector<WebRayRef>				mRays;
	Options								mOptions;
};
