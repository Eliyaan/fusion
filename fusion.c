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
	
	return is_inter_first__f <= 0.0 && is_inter_second__f <= 0.0;
}

fusion_IntersectionArray fusion_find_intersections(fusion_Shape shape_a, fusion_Shape shape_b)
{
	/* For each segment: */
	/* Find if the shape_a segment's line gets intersected by the shape_b segment and then the other way */
	fusion_IntersectionArray inters;
	unsigned int inters_cap;
	int i, j, is_intersecting;
	inters.segments = malloc(2 * 5 * (sizeof)fusion_IntersectionSegments); // TODO do that properly
	inters.length = 0;
	inters_cap = 2 * 5;

	for (i = 0; i < shape_a.coo_array.length; i++) // TODO handle the last segment : n-1 -> 0
	{
		for (j = 0; j < shape_b.coo_array.length; j++) // TODO same
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


/* 0 : shape_a outside shape_b */
/* 1 : shape_a inside shape_b  */
int fusion_is_inside_outside(fusion_Shape shape_a, fusion_Shape shape_b)
{
	/* The two shapes have 0 intersections */
	/* First segment: first segment of shape1
	/* Find closest intersecting segment (shape2) (after the first segment according to the normal of the first segment) of the normal of the first segment */
	/* If no closest intersecting segment (shape2) after the first segment: do the check with shape2, if same: the shapes are not touching */
	/* If the two normals go in the same direction : the first shape is inside the second one */
	/* If the two normals go in the opposite direction : the first shape is outside the second one */
	unsigned int i_intersect_sec; /* index of the intersecting segment in the other shape */
	fo_Coords normal_first; /* normal of the main segment (first segment above) */
	fo_Coords first_a_to_sec_a; /* Vector from first vert of the first segment to first vert of second vector (maybe closest intersecting segment) */
	float is_after; /* result of dot_product(normal_first, first_a_to_sec_a) to see if the second segment is after the first segment */
	int i;

	if (shape_a.coo_array.length <= 1)
	{
		return 0;
	}
	normal_first.x = -(shape_a.coo_array.coos[1].y - shape_a.coo_array.coos[0].y); /* -y */
	normal_first.y = shape_a.coo_array.coos[1].x - shape_a.coo_array.coos[0].x;
	
	/* Find closest intersecting segment */
	for (i = 0; i < shape_b.coo_array.length; i++)
	{
		/* Check if second segment is after first*/
		first_a_to_sec_a.x = shape_b.co_array.coos[i].x - shape_a.coo_array.coos[0].x;
		first_a_to_sec_a.y = shape_b.co_array.coos[i].y - shape_a.coo_array.coos[0].y;
		is_after = fusion_dot_product(normal_first, first_a_to_sec_a); // This is not enough  if the start of sec is behind first : check with sec_b : one or the other
		if (is_after > 0.0)
		{
			/* Find if it is closer than the current closest */
		// Did not find a way to do this without sqrt of the lengths
		}
	}
}
