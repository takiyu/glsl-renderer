#include "bvh.h"

using namespace glm;
using namespace std;


Triangle::Triangle(vec3 v0, vec3 v1, vec3 v2, int tri_idx){
	this->v[0] = v0;
	this->v[1] = v1;
	this->v[2] = v2;
	this->center = (v0+v1+v2) / 3.0f;
	this->tri_idx = tri_idx;
}


/* BVH Node */
Node::Node(){
}
void Node::walkAndDelete(){
	if(is_leaf) return;
	left->walkAndDelete();
	right->walkAndDelete();
	delete left, right;
}
// Triangle sort functions
bool lessTriangleCenterX(const Triangle* t0, const Triangle* t1){ return t0->center.x < t1->center.x; }
bool lessTriangleCenterY(const Triangle* t0, const Triangle* t1){ return t0->center.y < t1->center.y; }
bool lessTriangleCenterZ(const Triangle* t0, const Triangle* t1){ return t0->center.z < t1->center.z; }
void sortTrianglesCenter(vector<Triangle*>& tris_p, int axis){
	if(axis == 0) sort(tris_p.begin(), tris_p.end(), lessTriangleCenterX);
	else if(axis == 1) sort(tris_p.begin(), tris_p.end(), lessTriangleCenterY);
	else if(axis == 2) sort(tris_p.begin(), tris_p.end(), lessTriangleCenterZ);
}
// AABB functions
inline float min(float a, float b){ return (a > b) ? b : a; }
inline float max(float a, float b){ return (a > b) ? a : b; }
inline vec3 max(const vec3& a, const vec3& b){
	return vec3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}
inline vec3 min(const vec3& a, const vec3& b){
	return vec3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}
void calcBboxMinMaxPoint(const vector<Triangle*>& tris_p, vec3& min_point, vec3& max_point){
	// Get min, max point
	min_point = vec3( INFINITY,  INFINITY,  INFINITY);
	max_point = vec3(-INFINITY, -INFINITY, -INFINITY);
	for(int i = 0; i < tris_p.size(); i++){
		for(int v_idx = 0; v_idx < 3; v_idx++){
			min_point = min(min_point, tris_p[i]->v[v_idx]);
			max_point = max(max_point, tris_p[i]->v[v_idx]);
		}
	}
	return;
}
float surface(const vec3& min_point, const vec3& max_point){
	// Surface
	vec3 diff = max_point - min_point;
	return (diff.x*diff.y + diff.y*diff.z + diff.z*diff.x) * 2;
}
float surface(const vector<Triangle*>& tris_p){
	vec3 min_point, max_point;
	calcBboxMinMaxPoint(tris_p, min_point, max_point);
	return surface(min_point, max_point);
}
// Build tree returning node count
int Node::build(vector<Triangle*>& tris_p, int node_idx, int depth, int max_depth){
	this->node_idx = node_idx;
	int node_count = node_idx + 1;

	//Bounding Box Coordinates
	calcBboxMinMaxPoint(tris_p, this->bbox_min_point, this->bbox_max_point);

	//Search best position to devide
	vector<Triangle*> best_right, best_left;
	float total_surface = surface(this->bbox_min_point, this->bbox_max_point);
	float best_cost = BVH::TRI_TIME * tris_p.size();// *surface(tris_p)/surface(tris_p)
	for(int axis = 0; axis < 3; axis++){
		// Sort center coordinates
		sortTrianglesCenter(tris_p, axis);
		// Devide temporary
		for(int idx = 1; idx < tris_p.size()-1; idx++){
			vector<Triangle*> left(tris_p.begin(), tris_p.begin() + idx);
			vector<Triangle*> right(tris_p.begin() + idx, tris_p.end());
			// Calc SAH
			float cost = 2 * BVH::AABB_TIME +
				(surface(left)*left.size() + surface(right)*right.size()) * BVH::TRI_TIME / total_surface;

			// Update
			if(cost <= best_cost){
				best_cost = cost;
				best_left = left;
				best_right = right;
			}
		}
	}

	if(best_left.size() == 0 || best_right.size() == 0 || (max_depth > 0 && depth >= max_depth)){
		//Set
		this->is_leaf = true;
		this->tris_p = tris_p;
		return node_count;
	}
	//Set
	this->is_leaf = false;
	this->left = new Node();
	this->right = new Node();
	//Recursive call
	node_count = this->left->build(best_left, node_count, depth + 1, max_depth);
	node_count = this->right->build(best_right, node_count, depth + 1, max_depth);
	return node_count;
}
// Walk hit link and get bbox info
void Node::getBboxInfo(vector<glm::vec3>& bbox_minmax_array, vector<int>& tri_array, vector<int>& tri_idx_info){
	// min, max points
// 	assert(bbox_minmax_array.size() == node_idx * 2);
	bbox_minmax_array.push_back(this->bbox_min_point);
	bbox_minmax_array.push_back(this->bbox_max_point);
	// triangles
	if(this->is_leaf){
		// start triangle index
		tri_idx_info.push_back(tri_array.size());
		// triangle array
		for(int i = 0; i < this->tris_p.size(); i++){
			tri_array.push_back(this->tris_p[i]->tri_idx);
		}
		// end triangle index
		tri_idx_info.push_back(tri_array.size());
	}
	else{
		// triangle array index
		tri_idx_info.push_back(-1);
		tri_idx_info.push_back(-1);
	}

	// Exit
	if(this->is_leaf){
		return;
	}
	// Recurrent call
	this->left->getBboxInfo(bbox_minmax_array, tri_array, tri_idx_info);
	this->right->getBboxInfo(bbox_minmax_array, tri_array, tri_idx_info);
	return;
}
// Get miss index array
void Node::getMissIdxInfo(std::vector<int>& miss_idx_array, Node *next_right){
	// Set miss idx
	if(next_right == NULL) miss_idx_array.push_back(-1); // terminal
	else miss_idx_array.push_back(next_right->node_idx);

	// Exit
	if(this->is_leaf) return;
	// Recurrent call
	this->left->getMissIdxInfo(miss_idx_array, this->right);
	this->right->getMissIdxInfo(miss_idx_array, next_right);
}

/* BVH */
BVH::BVH(){
}
BVH::~BVH(){
	root.walkAndDelete();
}
void BVH::build(const vector<vec3>& tri_vertices){
	// Original triangles
	this->tris.clear();
	for(int tri_idx = 0; tri_idx < tri_vertices.size()/3; tri_idx++){
		Triangle tri(tri_vertices[3*tri_idx+0],
		             tri_vertices[3*tri_idx+1],
		             tri_vertices[3*tri_idx+2],
					 tri_idx);
		this->tris.push_back(tri);
	}

	// Create pointer array
	vector<Triangle*> tris_p(this->tris.size());
	for(int i = 0; i < this->tris.size(); i++){
		tris_p[i] = &tris[i];
	}

	// Start dividing
	this->bbox_count = this->root.build(tris_p, 0, 1);
}
void BVH::getInfo(vector<vec3>& bbox_minmax_array, std::vector<int>& tri_array, std::vector<int>& tri_idx_info, vector<int>& miss_idx_array){
	//hit link and bbox
	bbox_minmax_array.clear();
	tri_array.clear();
	tri_idx_info.clear();
	root.getBboxInfo(bbox_minmax_array, tri_array, tri_idx_info);
	//miss link
	miss_idx_array.clear();
	root.getMissIdxInfo(miss_idx_array, NULL);
}
