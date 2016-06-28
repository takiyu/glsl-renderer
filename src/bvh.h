#ifndef BVH_H_150401
#define BVH_H_150401

#include <iostream>
#include <vector>
#include <algorithm>
#include <cassert>
#define GLM_FORCE_RADIANS 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Triangle {
	Triangle(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, int tri_idx);
	glm::vec3 v[3];
	glm::vec3 center;
	int tri_idx;
};

struct Node {
	/* BVH Node (AABB) */
public:
	Node();
	void walkAndDelete();
	/* Build tree
	 *   return : node count */
	int build(std::vector<Triangle*>& tris_p, int node_idx, int depth, int max_depth=-1);
	/* Walk hit link and get bbox info */
	void getBboxInfo(std::vector<glm::vec3>& bbox_minmax_array,
	                 std::vector<int>& tri_array,
	                 std::vector<int>& tri_idx_info);
	/* Get miss index array */
	void getMissIdxInfo(std::vector<int>& miss_idx_array, Node* next_right);
private:
	int node_idx;
	bool is_leaf;
	Node *right, *left;
	glm::vec3 bbox_min_point, bbox_max_point;
	std::vector<Triangle*> tris_p;
};

//SAH constant time value
const static float AABB_TIME = 3.0f, TRI_TIME = 1.0f;

class BVH {
public:
	BVH() {}
	~BVH();
	void build(const std::vector<glm::vec3>& tri_vertices);
	// Get built tree info
	//    (hit_idx is current_idx+1)
	void getInfo(std::vector<glm::vec3>& bbox_minmax_array,
	             std::vector<int>& tri_array, // triangle idx array referred by tri_array_idx
	             std::vector<int>& tri_idx_info, // start and end idx of tri_array in each bbox
	             std::vector<int>& miss_idx_array);
private:
	Node root;
	std::vector<Triangle> tris;
	int bbox_count;
};

#endif
