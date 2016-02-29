//  Created by Paul Houx
//
//

#pragma once

#include "cinder/gl/Batch.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/VboMesh.h"

class InstancedBase {
  public:
	struct Data {
		ci::mat4 transform;
		ci::vec4 data;
	};

  public:
	InstancedBase( size_t maxCount = 1024 );
	virtual ~InstancedBase() {}

	virtual void update( double elapsed = 0.0 ) = 0;

	//! Draws the batch.
	virtual void draw();
	//! Replaces the batch's shader.
	virtual void replaceGlslProg( const ci::gl::GlslProgRef &glsl );

  protected:
	size_t mInstanceCount;

	//! Creates a batch from a geom::Source.
	void createBatch( const ci::geom::Source &source, const ci::gl::GlslProgRef &glsl );

	//! Creates a batch from a VboMeshRef
	void createBatch( const ci::gl::VboMeshRef &mesh, const ci::gl::GlslProgRef &glsl );

  protected:
	const size_t mMaxCount;

	ci::gl::VboMeshRef     mMesh;
	ci::gl::BatchRef       mBatch;
	ci::gl::VboRef         mDataVbo;
	ci::geom::BufferLayout mLayout;
};