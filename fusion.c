// TODO: setup an allocator for the module to replace mallocs

typedef struct {
	float x;
	float y;
} fusion_Coords;

typedef struct {
	*fusion_Coords coos;
	unsigned int length;
} fusion_CoordsArray;

// Traversal: clockwise
typedef struct {
	fusion_CoordsArray coo_array;
	char positive;
} fusion_Shape;

// Output of baking the primitives/vertices 
// Array of shapes
typedef struct {
	*fusion_Shape shapes;
	unsigned int length;
} fusion_BakedShapeArray;


// Ouput of applying each shape's addition/substraction on top of each other to have only one shape
// Array of coordinates
typedef struct {
	*fusion_Coords coo_array;
	unsigned int length;
} fusion_FinalShape;

// Indices of the first vertices (traversing clockwise) of the intersecting segments in the 2 shapes
// i_first: index of the vertex in the first shape; i_second: same for the second shape
typedef struct {
	unsigned int i_first;
	unsigned int i_second;
} fusion_IntersectionSegments;

// All the intersections of 2 shapes
typedef struct {
	*fusion_IntersectionSegments segments;
	unsigned int length;
} fusion_IntersectionArray;

float fusion_dot_product(fusion_Coords a, fusion_Coords b)
{
	return a.x * b.x + a.y * b.y;
}

/* 0 if the segments do not intersect */
/* 1 if the segments intersect */
int fusion_segments_intersect(fusion_Coords first_a, fusion_Coords first_b, fusion_Coords second_a, fusion_Coords second_b)
{
	// Find if a segment's line gets intersected by a segment:
	// Dot product of the orthogonal (to first segment) with vector (point of first segment to 1st point of other segment)
	// multiply it with the dot product of the orthogonal (to first segment) with vector (point of first segment to 2nd point of other segment)
	// if the result is positive : either - * - or + * + : the two points of the second segment are both above(/below) the first segment's line
	// if the result is negative : - * + : one point of the second segment is above the first segment's line and the other below
	// (Uses the cosine encoded in the dot product ||first||*||second||*cos(angle) with the orthogonal vector to the first segment
	// as it puts the 0 of the cosine on the first segment's line so anything above(/below) the line will be positive(/negative))
	
	fusion_Coords first_seg, sec_seg, first_orth, second_orth;
	fusion_Coords f_a_to_sec_a, f_a_to_sec_b, sec_a_to_f_a, sec_a_to_f_b;
	float is_inter_first__f, is_inter_second__f;
	first_seg.x = first_b.x - first_a.x;
	first_seg.y = first_b.y - first_a.y;
	second_seg.x = second_b.x - second_a.x;
	second_seg.y = second_b.y - second_a.y;

	first_orth.x = -first_seg.y;
	first_orth.y = first_seg.x;
	second_orth.x = -second_seg.y;
	second_orth.y = second_seg.x;

	f_a_to_sec_a.x = second_a.x - first_a.x;
	f_a_to_sec_a.y = second_a.y - first_a.y;
	f_a_to_sec_b.x = second_b.x - first_a.x;
	f_a_to_sec_b.y = second_b.y - first_a.y;
	
	sec_a_to_f_a.x = first_a.x - second_a.x;
	sec_a_to_f_a.y = first_a.y - second_a.y;
	sec_a_to_f_b.x = first_b.x - second_a.x;
	sec_a_to_f_b.y = first_b.y - second_a.y;

	is_inter_first__f = fusion_dot_product(first_orth, f_a_to_sec_a) * fusion_dot_product(first_orth, f_a_to_sec_b);
	is_inter_second__f = fusion_dot_product(second_orth, sec_a_to_f_a) * fusion_dot_product(second_orth, sec_a_to_f_b);
	
	return is_inter_first__f < 0.0 && is_inter_second__f < 0.0;
}

fusion_IntersectionArray fusion_intersections_find(fusion_Shape shape_a, fusion_Shape shape_b)
{
	/* For each segment: */
	/* Find if the shape_a segment's line gets intersected by the shape_b segment and then the other way */
	fusion_IntersectionArray inters;
	unsigned int inters_cap;
	int i, j, is_intersecting;
	inters.segments = malloc(2 * 5 * (sizeof)fusion_IntersectionSegments); // TODO do that properly
	inters.length = 0;
	inters_cap = 2 * 5;

	for (i = 0; i < shape_a.coo_array.length; i++) 
	{
		for (j = 0; j < shape_b.coo_array.length; j++)
		{
			is_intersecting = fusion_segments_intersect(shape_a.coo_array.coos[i], shape_a.coo_array.coos[(i + 1) % shape_a.coo_array.length],
				shape_b.coo_array.coos[j], shape_b.coo_array.coos[(j + 1) % shape_b.coo_array.length]);
			if (is_intersecting)
			{
				if (inters.length == inters_cap)
				{
					inters.segments = realloc(inters.segments, inters_cap * 2);
					inters_cap = inters_cap * 2;
				}
				inters.segments[inters.length].first = i;
				inters.segments[inters.length].second = j;
				inters.length = inters.length + 1;
			}
		}
	}
	return inters;
}


/* Prerequisite: The two shapes have 0 intersections */
/* 0 : shape_a outside shape_b */
/* 1 : shape_a inside shape_b  */
int fusion_shape_is_a_inside_b(fusion_Shape shape_a, fusion_Shape shape_b)
{
	/* Fire a ray from a point of shape_a and count the number of intersections with shape_b: odd -> inside, even -> outside */
	unsigned int count_intersect; /* number of intersections */
	fusion_Coords shape_a_vec; /* Vector ray fired from shape_a */
	fusion_Coords shape_a_point; /* origin of the vector, a point from shape a */
	fusion_Coords shape_b_f_point, shape_b_s_point; /* two connected points from shape_b */
	fusion_Coords a_f_vec, a_s_vec; /* Vectors from shape_a_point to shape_b_(f/s)_point */
	fusion_Coords shape_b_normal; 
	float a_f_a_dot, a_s_a_dot; /* results of the dot products */
	unsigned int are_intersecting;
	unsigned int shape_b_length;
	unsigned int i;
	
	shape_b_length = shape_b.coo_array.length;

	if (shape_a.coo_array.length <= 1)
	{
		return 0;
	}
	shape_a_point.x = shape_a.coo_array.coos[0].x;
	shape_a_point.x = shape_a.coo_array.coos[0].y;
	shape_a_vec.x = shape_a.coo_array.coos[1].x - shape_a_point.x;
	shape_a_vec.y = shape_a.coo_array.coos[1].y - shape_a_point.y;
	
	shape_b_s_point.x = shape_b.coo_array.coos[shape_b_length - 1].x; /* handles : N-1 -> 0 the last link */
	shape_b_s_point.y = shape_b.coo_array.coos[shape_b_length - 1].y;
	/* Find all intersecting segments */
	for (i = 0; i < shape_b.coo_array.length; i++)
	{
		shape_b_f_point.x = shape_b_s_point.x;
		shape_b_f_point.y = shape_b_s_point.y;
		shape_b_s_point.x = shape_b.coo_array.coos[i].x;
		shape_b_s_point.y = shape_b.coo_array.coos[i].y;

		a_f_vec.x = shape_b_f_point.x - shape_a_point.x;
		a_f_vec.y = shape_b_f_point.y - shape_a_point.y;
		a_s_vec.x = shape_b_s_point.x - shape_a_point.x;
		a_s_vec.y = shape_b_s_point.y - shape_a_point.y;
	
		a_f_a_dot = fusion_dot_product(a_f_vec, shape_a_vec);
		a_s_a_dot = fusion_dot_product(a_s_vec, shape_a_vec);
		are_intersecting = a_f_a_dot * a_s_a_dot < 0.0;

		if (are_intersecting)
		{
			/* Check if the dot product of the vector that has a positive dot product with shape_a_vec and */
			/* the normal of the vector that has a negative dot product with shape_a_vec to see if shape_b */
			/* segment is above or below the origin of the shape_a vector */
			/* dot product > 0.0 ? above : below */
			if (a_f_a_dot < 0.0 )
			{
				shape_b_normal.x = -a_f_vec.y;
				shape_b_normal.y = a_f_vec.x;
				count_intersect = count_intersect + (fusion_dot_product(a_s_vec, shape_b_normal) > 0.0);
			} 
			else /* a_s_a_dot < 0.0 */
			{
				shape_b_normal.x = -a_s_vec.y;
				shape_b_normal.y = a_s_vec.x;
				count_intersect = count_intersect + (fusion_dot_product(a_f_vec, shape_b_normal) > 0.0);
			}
		}
	}
	return count_intersect & 1;
}
