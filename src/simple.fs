#version 330 core

in vec2 position;
uniform int bbox_size;
uniform vec3 camera_org;
uniform vec3 camera_dir_base;
uniform vec3 camera_xvec;
uniform vec3 camera_yvec;
uniform vec2 rand_vec2_a;
uniform vec2 rand_vec2_b;
uniform vec3 rand_vec3;
uniform int accum_frame;

//General textures
uniform sampler2DRect triangle_tex;
uniform sampler2DRect normal_tex;
uniform sampler2DRect texcoord_tex;
uniform isampler2DRect mat_idx_tex;
uniform sampler2DRect material_tex;
uniform sampler2DRect accum_pixel_tex;

//BVH textures
uniform sampler2DRect bbox_minmax_tex;
uniform isampler2DRect bbox_info_tex;

const int TRI_TEX_COL = 512;
const int M_ID_TEX_COL = 512;
const int M_TEX_COL = 2;
const int BVH_TEX_COL = 512;

const int DEPTH_COUNT = 3;
/* const int DEPTH_COUNT = 1; */

const vec3 LightPosRange = vec3(0.10, 0, 0.10);
/* const vec3 LightPos = vec3(0.50,0.70,0.70); */
const vec3 LightPos = vec3(0.50,0.75,0.50);

struct Ray {
	vec3 org, dir;
};

const float NEAR_ZERO = 1e-6;
const float INFINITY = 1e5;
struct Intersection {
	float dist;
	int tri_idx;
	vec3 hit_position;
	vec3 normal;
	vec2 texcoord;
};
void intersectTriangle(const Ray ray, const int tri_idx, inout Intersection result) {
	int col_idx = tri_idx % TRI_TEX_COL;
	int row_idx = tri_idx / TRI_TEX_COL;
	vec3 position0 = texture(triangle_tex, vec2(3*col_idx+0, row_idx)).xyz;
	vec3 edge0 = texture(triangle_tex, vec2(3*col_idx+1, row_idx)).xyz - position0;
	vec3 edge1 = texture(triangle_tex, vec2(3*col_idx+2, row_idx)).xyz - position0;

	/* Möller–Trumbore intersection algorithm */
	vec3 P = cross(ray.dir, edge1);
	float det = dot(P, edge0);
	if(-NEAR_ZERO < det && det < NEAR_ZERO) return;
	float inv_det = 1.0 / det;
	vec3 T = ray.org - position0;
	float u = dot(T, P) * inv_det;
	if(u < 0.0 || 1.0 < u) return;
	vec3 Q = cross(T, edge0);
	float v = dot(ray.dir, Q) * inv_det;
	if(v < 0.0 || 1.0 < u + v) return;
	float t = dot(edge1, Q) * inv_det;
	if(NEAR_ZERO < t){ // Hit
		// Check distance
		if(t < result.dist){
			// Get nearest triangle info
			result.dist = t;
			result.tri_idx = tri_idx;
			result.hit_position = ray.dir * t + ray.org;

			float uv1 = 1.0 - u - v;

			vec3 n0 = texture(normal_tex, vec2(3*col_idx+0, row_idx)).xyz;
			vec3 n1 = texture(normal_tex, vec2(3*col_idx+1, row_idx)).xyz;
			vec3 n2 = texture(normal_tex, vec2(3*col_idx+2, row_idx)).xyz;
			n0 = n0 * 2.0 - 1.0;
			n1 = n1 * 2.0 - 1.0;
			n2 = n2 * 2.0 - 1.0;

			if((length(n0) < 0.5) && (length(n1) < 0.5) && (length(n2) < 0.5)){
				vec3 ref_normal = normalize(cross(edge0, edge1));
				result.normal = ref_normal;
			}else{
				if(dot(n1, n0) < 0) n1 *= -1.0;
				if(dot(n2, n0) < 0) n2 *= -1.0;

				result.normal = n0 * uv1 + n1 * u + n2 * v;
			}

			vec2 t0 = texture(texcoord_tex, vec2(3*col_idx+0, row_idx)).xy;
			vec2 t1 = texture(texcoord_tex, vec2(3*col_idx+1, row_idx)).xy;
			vec2 t2 = texture(texcoord_tex, vec2(3*col_idx+2, row_idx)).xy;
			result.texcoord = t0 * uv1 + t1 * u + t2 * v;

		}
		return;
	}
	return;
}
bool intersectBBox(const Ray ray, const int bbox_idx){
	float t_far = INFINITY;
	float t_near =  -INFINITY;

	int col_idx = bbox_idx % BVH_TEX_COL;
	int row_idx = bbox_idx / BVH_TEX_COL;
	vec3 min_point = texture(bbox_minmax_tex, vec2(2*col_idx+0, row_idx)).xyz;
	vec3 max_point = texture(bbox_minmax_tex, vec2(2*col_idx+1, row_idx)).xyz;

	for(int i = 0; i < 3; i++){
		float t1 = (min_point[i] - ray.org[i]) / ray.dir[i];
		float t2 = (max_point[i] - ray.org[i]) / ray.dir[i];
		if(t1 < t2){
			t_far = min(t_far, t2);
			t_near = max(t_near, t1);
		}else{
			t_far = min(t_far, t1);
			t_near = max(t_near, t2);
		}
		if(t_far < t_near) return false;
	}

	return true;
}
Intersection intersect(const Ray ray){
	Intersection result = Intersection(INFINITY, 0, vec3(0), vec3(0), vec2(0));

	/* int BBOX_SIZE = textureSize(bbox_info_tex).y; */
	int bbox_idx = 0;
	while(true){
		int col_idx = bbox_idx % BVH_TEX_COL;
		int row_idx = bbox_idx / BVH_TEX_COL;
		if(intersectBBox(ray, bbox_idx)){
			int tri_idx = texture(bbox_info_tex, vec2(3*col_idx+0, row_idx)).r;
			// Leaf check (internal node is -1)
			if(tri_idx >= 0){
				int end_tri_idx = texture(bbox_info_tex, vec2(3*col_idx+1, row_idx)).r;
				// Linear search
				for(; tri_idx < end_tri_idx; tri_idx++){
					intersectTriangle(ray, tri_idx, result);
				}
			}
			// hit link
			bbox_idx++;
			if(bbox_idx >= bbox_size) break;
		}else{
			// miss link
			bbox_idx = texture(bbox_info_tex, vec2(3*col_idx+2, row_idx)).r;
			if(bbox_idx < 0) break;
		}
	}
	return result;
}

const float glossiness = 8.0;//光沢度
vec3 sampleDiffuse(const vec3 light_dir, const vec3 look_dir, const vec3 normal,
                   const int tri_idx, const vec2 texcoord) {
	int col_idx = tri_idx % M_ID_TEX_COL;
	int row_idx = tri_idx / M_ID_TEX_COL;
	int mat_id = texture(mat_idx_tex, ivec2(col_idx, row_idx)).r;
	int col_idx2 = mat_id % M_TEX_COL;
	int row_idx2 = mat_id / M_TEX_COL;

	vec3 Kd = texture(material_tex, vec2(2 * col_idx2 + 0, row_idx2)).rgb;
	vec3 Ks = texture(material_tex, vec2(2 * col_idx2 + 1, row_idx2)).rgb;

	vec3 L = vec3(0,0,0);
	float Ld = dot(light_dir, normal);

	// for neg normal
	bool visible = (dot(-look_dir, normal) > 0.0);
	if(!visible){
		Ld *= -1;
	}

	if(Ld > 0.0){
		L += Ld * Kd;

		vec3 r = reflect(-light_dir, normal);
		float Ls = pow(dot(-look_dir,r), glossiness);
		if(Ls > 0){
			L += Ls * Ks;
		}
	}

	return clamp(L, 0, 1);
}

vec3 render(const Ray ray) {
	vec3 L = vec3(0,0,0);
	Ray rays[DEPTH_COUNT+1];
	Intersection results[DEPTH_COUNT];

	int i;
	/* Trace reverse ray */
	for(i = 0; i < DEPTH_COUNT; i++){
		/* Create new ray (dir) */
		if(i == 0){
			rays[i] = ray;
		} else {
			vec3 reflected = reflect(rays[i-1].dir, results[i-1].normal);// length(reflected) == 1
			vec3 w, u, v;
			w = results[i-1].normal;
			if(dot(w, reflected) < 0.0) w *= -1;
			if (abs(w.x) > 0.001) u = normalize(cross(vec3(0.0f, 1.0f, 0.0f),w));
			else                  u = normalize(cross(vec3(1.0f, 0.0f, 0.0f),w));
			v = cross(w,u);
			float r1 = rand_vec2_a.x * 2 * 3.141592;
			float r2 = rand_vec2_a.y;
			float sqrt_r2 = sqrt(r2);
			vec3 dir = normalize((u*cos(r1)*sqrt_r2 + v*sin(r1)*sqrt_r2 + w*sqrt(1.0-r2)));
			rays[i].dir = dir;// update dir
		}
		/* Emit */
		results[i] = intersect(rays[i]);
		if(results[i].dist >= INFINITY){ // miss Check
			break;//finish (Last lay will not be used.)
		}

		/* Create new ray (org) */
		rays[i+1].org = results[i].hit_position - 0.001*rays[i].dir;// update org
	}

	//i==DEPTH_COUNT+1 or i-th ray did not hit.
	i -= 1;

	/* Sampling at each hit point */
	for(; i >= 0; i--){
		/* Direct Light */
		vec3 direct_color = vec3(0,0,0);
		vec3 light_rel_pos = (LightPos + LightPosRange * (rand_vec3 * 2.0 - 1.0))
		                      - results[i].hit_position;
		Ray s_ray = Ray(rays[i+1].org, normalize(light_rel_pos));
		Intersection s_result = intersect(s_ray);
		// Check arrival of the light TODO LightColor
		if(s_result.dist > length(light_rel_pos)){ // Check far or miss
			direct_color += vec3(1,1,1) * sampleDiffuse(s_ray.dir, rays[i].dir,
			                results[i].normal, results[i].tri_idx, results[i].texcoord);
		}
		direct_color /= 1;

		/* Update */
		L = direct_color + L*sampleDiffuse(rays[i+1].dir, rays[i].dir,
		                                   results[i].normal, results[i].tri_idx,
		                                   results[i].texcoord);
		L = clamp(L, 0, 1);
	}

	return L;
}

void main() {
	vec3 camera_dir = normalize(camera_dir_base + (position.x+rand_vec2_b.x) * camera_xvec
	                                            - (position.y+rand_vec2_b.y) * camera_yvec);
	Ray ray = Ray(camera_org, camera_dir);
	vec3 color = render(ray);
	if(accum_frame == 0){
		gl_FragColor = vec4(color, 1);
	}else{
		// Add pre-frame pixel color
		vec3 old_color = texture(accum_pixel_tex, position).xyz;
		vec3 new_color = (old_color*accum_frame + color) /(accum_frame+1);
		gl_FragColor = vec4(new_color, 1);
	}
}
