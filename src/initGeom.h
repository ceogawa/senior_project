#ifndef INIT_GEOM_H
#define INIT_GEOM_H

#include <chrono>
#include <iostream>
#include <glad/glad.h>

#include "Program.h"
#include "Shape.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;


vector<shared_ptr<Shape>> initMesh(string local_path, vector<shared_ptr<Shape>> mesh) {
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	string errStr;
	string resourceDir = "../resources";
	//cout << resourceDir + local_path << endl;
	bool rc = tinyobj::LoadObj(shapes, materials, errStr, (resourceDir + local_path).c_str());
	if (!rc) {
		cerr << errStr << endl;
	}
	else {
		shared_ptr<Shape> shapePart;
		for (int i = 0; i < shapes.size(); i++) {
			shapePart = make_shared<Shape>();
			shapePart->createShape(shapes[i]);
			shapePart->measure();
			shapePart->init();
			mesh.push_back(shapePart);
		}
	}
	return mesh;
}

#endif