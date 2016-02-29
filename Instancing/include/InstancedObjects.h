#pragma once

#include "cinder/Timeline.h"
#include "Instanced.h"

typedef std::shared_ptr<class InstancedArrows> InstancedArrowsRef;
typedef std::shared_ptr<class InstancedDots>   InstancedDotsRef;


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


// ------------------------------------------------------------------------------------------------- Instanced Arrows

class InstancedArrows {
  public:
	static InstancedArrowsRef create() { return std::make_shared<InstancedArrows>(); };
	
	InstancedArrows();
	~InstancedArrows(){};
	
	typedef struct InstanceData {
		ci::mat4 transform;
		ci::vec4 color;
		ci::vec4 texBounds;
	} InstanceData;
	
	void update( double elapsed = 0.0 );
	void draw();

  private:
	double					  mTime;
	ci::gl::TextureRef	      mArrowTexture;
	ci::gl::GlslProgRef       mShader;
	ci::gl::VboRef            mInstanceDataVbo;
	ci::gl::BatchRef		  mBatch;
	std::vector<ArrowOptions> mOptions;
	bool					  mIsPaused = false;
	
	static const int kMaxCount = 30;
};


// ------------------------------------------------------------------------------------------------- Instanced Dots

class InstancedDots {
  public:
	static InstancedDotsRef create() { return std::make_shared<InstancedDots>(); };
	
	InstancedDots();
	~InstancedDots(){};
	
	class Dots : public InstancedBase {
	  public:
		typedef struct DotData {
			ci::vec2 direction;
			ci::vec3 color;
			float    offset;
		} DotData;

	  public:
		Dots();

		void update( double elapsed = 0.0 );

	  private:
		ci::gl::GlslProgRef  mShader;
		std::vector<DotData> mDotData;
		double               mTime;
		
		static const int kMaxRows = 4;
		static const int kDotsPerRow = 8;
	};
	
	void update( double elapsed = 0.0 );
	void draw();
	
  private:
  
	const int kViewportSize = 150;
	
	Dots			mDots;
	ci::TimelineRef mTimeline;
	ci::Anim<float> mScale, mAlpha;
	float			mAlphaOffset;
	
	
	ci::gl::Texture2dRef mTexture;
	ci::CameraOrtho      mCamera;
};
