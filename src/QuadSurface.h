#pragma once

#include "ofMain.h"
#include "HomographyHelper.h"
#include "Vec2.h"
#include "Vec3.h"


class QuadSurface {
	public:
		QuadSurface();
		~QuadSurface();

		void setup();
		void setup(
			Vec3 p1, Vec3 p2, Vec3 p3, Vec3 p4,
			Vec2 t1, Vec2 t2, Vec2 t3, Vec2 t4);
		void draw(ofTexture& texture);
		void setVertices(std::vector<Vec3> v);
		void setTexCoords(std::vector<Vec2> t);

		Vec2 getTexCoord(int index);
		std::vector<Vec3> getVertices();
		std::vector<Vec2> getTexCoords();
	
		void setPerspectiveWarping(bool b);
		bool getPerspectiveWarping();
	
		ofRectangle getMeshBoundingBox();

	private:
		void calculateHomography();
		float _matrix[16];
		bool _perspectiveWarping;
		ofMesh _meshCache;
		ofMesh mesh;
};
