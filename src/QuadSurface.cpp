#include "QuadSurface.h"

QuadSurface::QuadSurface(){
	_perspectiveWarping = true;
	setup();
}

QuadSurface::~QuadSurface(){
	std::cout << "QuadSurface destructor." << std::endl;
}

void QuadSurface::setup(){
	// Create 4 points for the 2 triangles
	Vec3 p1 = Vec3(100, 100, 0.0f);
	Vec3 p4 = Vec3(100, 880, 0.0f);
	Vec3 p3 = Vec3(1720, 880, 0.0f);
	Vec3 p2 = Vec3(1720, 100, 0.0f);

	//Vec3 p1 = Vec3(50, 50, 0.0f);
	//Vec3 p4 = Vec3(50, 650, 0.0f);
	//Vec3 p3 = Vec3(850, 650, 0.0f);
	//Vec3 p2 = Vec3(850, 50, 0.0f);

	// Create 4 point for the texture coordinates
	Vec2 t1 = Vec2(0.0f, 0.0f);
	Vec2 t2 = Vec2(1.0f, 0.0f);
	Vec2 t3 = Vec2(1.0f, 1.0f);
	Vec2 t4 = Vec2(0.0f, 1.0f);

	setup(p1, p2, p3, p4, t1, t2, t3, t4);
}

void QuadSurface::setup(Vec3 p1, Vec3 p2, Vec3 p3, Vec3 p4,
						Vec2 t1, Vec2 t2, Vec2 t3, Vec2 t4)
{
	// Clear mesh
	mesh.clear();

	// Create a surface with the points
	mesh.addVertex(p1.toOf());
	mesh.addVertex(p2.toOf());
	mesh.addVertex(p3.toOf());
	mesh.addVertex(p4.toOf());

	// Add 2 triangles
	mesh.addTriangle(0, 2, 3);
	mesh.addTriangle(0, 1, 2);

	// Add texture coordinates
	mesh.addTexCoord(t1.toOf());
	mesh.addTexCoord(t2.toOf());
	mesh.addTexCoord(t3.toOf());
	mesh.addTexCoord(t4.toOf());
	
	_meshCache = mesh;
	calculateHomography();
}

void QuadSurface::draw(ofTexture& texture){
	if (!texture.isAllocated())
	{
		return;
	}

	if(_perspectiveWarping){
		bool meshChanged = false;
		
		if(
			mesh.getVertices()[0] != _meshCache.getVertices()[0] ||
			mesh.getVertices()[1] != _meshCache.getVertices()[1] ||
			mesh.getVertices()[2] != _meshCache.getVertices()[2] ||
			mesh.getVertices()[3] != _meshCache.getVertices()[3])
		{
			meshChanged = true;
		}
		
		if(meshChanged){
			calculateHomography();
			_meshCache = mesh;
		}
		
		ofRectangle box = getMeshBoundingBox();
		ofMesh m = mesh;
		
		m.setVertex(0, Vec3(0, 0, 0).toOf());
		m.setVertex(1, Vec3(box.width, 0, 0).toOf());
		m.setVertex(2, Vec3(box.width, box.height, 0).toOf());
		m.setVertex(3, Vec3(0, box.height, 0).toOf());
		
		ofPushMatrix();
		if(true){
			bool normalizedTexCoords = ofGetUsingNormalizedTexCoords();
			ofEnableNormalizedTexCoords();
			
			glMultMatrixf(_matrix);
			//fbo.getTexture().bind();
			texture.bind();
			m.draw();
			//fbo.getTexture().unbind();
			texture.unbind();
			
			if(!normalizedTexCoords){
				ofDisableNormalizedTexCoords();
			}
		}
		ofPopMatrix();
	}else{
		bool normalizedTexCoords = ofGetUsingNormalizedTexCoords();
		ofEnableNormalizedTexCoords();
		
		ofPushStyle();
		ofSetColor(255, 255, 255);
	
		//fbo.getTexture().bind();
		texture.bind();
		mesh.draw();
		//fbo.getTexture().unbind();
		texture.unbind();
		
		ofPopStyle();
		
		if(!normalizedTexCoords){
			ofDisableNormalizedTexCoords();
		}
	}
}

void QuadSurface::setVertices(std::vector<Vec3> v){
	if(v.size() != 4){
		throw runtime_error("Wrong number of vertices");
	}
	
	for(int i = 0; i < 4; ++i){
		mesh.setVertex(i, v[i].toOf());
	}
	
	std::vector<Vec3> vertices = Vec3::fromOf(mesh.getVertices());
}

void QuadSurface::setTexCoords(std::vector<Vec2> t){
	if(t.size() != 4){
		throw runtime_error("Wrong number of vertices");
	}
	for(int i = 0; i < 4; ++i){
		mesh.setTexCoord(i, t[i].toOf());
	}
}

Vec2 QuadSurface::getTexCoord(int index){
	if(index > 3){
		throw runtime_error("Texture coordinate index out of bounds.");
	}

	return Vec2(
		mesh.getTexCoord(index).x,
		mesh.getTexCoord(index).y);
}

std::vector<Vec3> QuadSurface::getVertices(){
	return Vec3::fromOf(mesh.getVertices());
}

std::vector<Vec2> QuadSurface::getTexCoords(){
	return Vec2::fromOf(mesh.getTexCoords());
}

void QuadSurface::calculateHomography(){
	float src[4][2];
    float dst[4][2];
	
	ofRectangle box = getMeshBoundingBox();
	
	src[0][0] = 0;
    src[0][1] = 0;
    src[1][0] = box.width;
    src[1][1] = 0;
    src[2][0] = box.width;
    src[2][1] = box.height;
    src[3][0] = 0;
    src[3][1] = box.height;
	
	Vec3 p0(mesh.getVertex(0).x, mesh.getVertex(0).y, mesh.getVertex(0).z);
	Vec3 p1(mesh.getVertex(1).x, mesh.getVertex(1).y, mesh.getVertex(1).z);
	Vec3 p2(mesh.getVertex(2).x, mesh.getVertex(2).y, mesh.getVertex(2).z);
	Vec3 p3(mesh.getVertex(3).x, mesh.getVertex(3).y, mesh.getVertex(3).z);

	dst[0][0] = p0.x;
	dst[0][1] = p0.y;
	dst[1][0] = p1.x;
	dst[1][1] = p1.y;
    dst[2][0] = p2.x;
	dst[2][1] = p2.y;
	dst[3][0] = p3.x;
	dst[3][1] = p3.y;
	
    HomographyHelper::findHomography(src, dst, _matrix);
}

void QuadSurface::setPerspectiveWarping(bool b){
	_perspectiveWarping = b;
}

bool QuadSurface::getPerspectiveWarping(){
	return _perspectiveWarping;
}

ofRectangle QuadSurface::getMeshBoundingBox(){
	float minX = 10000.0f;
	float minY = 10000.0f;
	float maxX = 0.0f;
	float maxY = 0.0f;
	
	for(int i = 0; i < mesh.getVertices().size(); ++i){
		if(mesh.getVertices()[i].x < minX){
			minX = mesh.getVertices()[i].x;
		}
		
		if(mesh.getVertices()[i].y < minY){
			minY = mesh.getVertices()[i].y;
		}
		
		if(mesh.getVertices()[i].x > maxX){
			maxX = mesh.getVertices()[i].x;
		}
		
		if(mesh.getVertices()[i].y > maxY){
			maxY = mesh.getVertices()[i].y;
		}
	}
	
	ofRectangle boundingBox = ofRectangle(ofPoint(minX, minY), ofPoint(maxX, maxY));
	return boundingBox;
}
