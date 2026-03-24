// TODO: setup an allocator for the module to replace mallocs

typedef struct {
	float x;
	float y;
} fusion__Coords;

typedef struct {
	*fusion__Coords coos;
	unsigned int length;
} fusion__CoordsArray;

// Traversal: clockwise
typedef struct {
	fusion__CoordsArray coo_array;
	unsigned char positive;
} fusion__Shape;

// Output of baking the primitives/vertices 
// Array of shapes
typedef struct {
	*fusion__Shape shapes;
	unsigned int length;
} fusion__BakedShapeArray;


// Ouput of applying each shape's addition/substraction on top of each other to have only one shape
// Array of coordinates
typedef fusion__CoordsArray fusion__FinalShape;

// Indices of the first vertices (traversing clockwise) of the intersecting segments in the 2 shapes
// i_first: index of the vertex in the first shape; i_second: same for the second shape
typedef struct {
	unsigned int i_first;
	unsigned int i_second;
} fusion__IntersectionSegments;

// All the intersections of 2 shapes
typedef struct {
	*fusion__IntersectionSegments segments;
	unsigned int length;
} fusion__IntersectionArray;

/* 0 if the segments do not intersect */
/* 1 if the segments intersect */
int fusion__segments_intersect(fusion__Coords first_a, fusion__Coords first_b, fusion__Coords second_a, fusion__Coords second_b)
{
	// Find if a segment's line gets intersected by a segment:
	// Dot product of the orthogonal (to first segment) with vector (point of first segment to 1st point of other segment)
	// multiply it with the dot product of the orthogonal (to first segment) with vector (point of first segment to 2nd point of other segment)
	// if the result is positive : either - * - or + * + : the two points of the second segment are both above(/below) the first segment's line
	// if the result is negative : - * + : one point of the second segment is above the first segment's line and the other below
	// (Uses the cosine encoded in the dot product ||first||*||second||*cos(angle) with the orthogonal vector to the first segment
	// as it puts the 0 of the cosine on the first segment's line so anything above(/below) the line will be positive(/negative))
	
	fusion__Coords first_seg, sec_seg, first_orth, second_orth;
	fusion__Coords f_a_to_sec_a, f_a_to_sec_b, sec_a_to_f_a, sec_a_to_f_b;
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

	/* dot_product * dot_product -> are the two dot products of different signs -> if yes -> intersection */
	is_inter_first__f = (first_orth.a * f_a_to_sec_a.a + first_orth.b * f_a_to_sec_a.b) * (first_orth.a * f_a_to_sec_b.a + first_orth.b * f_a_to_sec_b.b);
	is_inter_second__f = (second_orth.a * sec_a_to_f_a.a + second_orth.b * sec_a_to_f_a.b) * (second_orth.a * sec_a_to_f_b.a + second_orth.b * sec_a_to_f_b.b);
	
	return is_inter_first__f < 0.0 && is_inter_second__f < 0.0;
}

fusion__IntersectionArray fusion__intersections_find(fusion__Shape shape_a, fusion__Shape shape_b)
{
	/* For each segment: */
	/* Find if the shape_a segment's line gets intersected by the shape_b segment and then the other way */
	fusion__IntersectionArray inters;
	unsigned int inters_cap;
	int i, j, is_intersecting;
	inters.segments = malloc(2 * 5 * (sizeof)fusion__IntersectionSegments); // TODO do that properly
	inters.length = 0;
	inters_cap = 2 * 5;

	for (i = 0; i < shape_a.coo_array.length - 1; i++) 
	{
		for (j = 0; j < shape_b.coo_array.length - 1; j++)
		{
			is_intersecting = fusion__segments_intersect(shape_a.coo_array.coos[i], shape_a.coo_array.coos[i + 1],
				shape_b.coo_array.coos[j], shape_b.coo_array.coos[j + 1]);
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
		/* j = shape_b.coo_array.length - 1 */
		is_intersecting = fusion__segments_intersect(shape_a.coo_array.coos[i], shape_a.coo_array.coos[i + 1],
			shape_b.coo_array.coos[j], shape_b.coo_array.coos[0]);
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
	/* i = shape_a.coo_array.length - 1 */
	/* j = shape_b.coo_array.length - 1 */
	is_intersecting = fusion__segments_intersect(shape_a.coo_array.coos[i], shape_a.coo_array.coos[0],
		shape_b.coo_array.coos[j], shape_b.coo_array.coos[0]);
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
	return inters;
}


/* Prerequisite: The two shapes have 0 intersections */
/* 0 : shape_a outside shape_b */
/* 1 : shape_a inside shape_b  */
int fusion__shape_is_a_inside_b(fusion__Shape shape_a, fusion__Shape shape_b)
{
	/* Fire a ray from a point of shape_a and count the number of intersections with shape_b: odd -> inside, even -> outside */
	unsigned int count_intersect; /* number of intersections */
	fusion__Coords shape_a_vec; /* Vector ray fired from shape_a */
	fusion__Coords shape_a_point; /* origin of the vector, a point from shape a */
	fusion__Coords shape_b_f_point, shape_b_s_point; /* two connected points from shape_b */
	fusion__Coords a_f_vec, a_s_vec; /* Vectors from shape_a_point to shape_b_(f/s)_point */
	fusion__Coords shape_b_normal; 
	float a_f_a_dot, a_s_a_dot; /* results of the dot products */
	float is_above;
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
	/* Find all intersecting segments with shape_a_vec */
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
	
		a_f_a_dot = (a_f_vec.a * shape_a_vec.a + a_f_vec.b * shape_a_vec.b);
		a_s_a_dot = (a_s_vec.a * shape_a_vec.a + a_s_vec.b * shape_a_vec.b);
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
				is_above = a_s_vec.a * shape_b_normal.a + a_s_vec.b * shape_b_normal.b;
				count_intersect = count_intersect + (is_above > 0.0);
			} 
			else /* a_s_a_dot < 0.0 */
			{
				shape_b_normal.x = -a_s_vec.y;
				shape_b_normal.y = a_s_vec.x;
				is_above = a_f_vec.a * shape_b_normal.a + a_f_vec.b * shape_b_normal.b;
				count_intersect = count_intersect + (is_above > 0.0);
			}
		}
	}
	return count_intersect & 1;
}

fusion__Shape fusion__shape_fill(fusion__Shape shape_a, fusion__Shape shape_b, fusion__IntersectionArray inters)
{
	fusion__Shape new_shape;
	unsigned int new_shape_cap, i, j, k;
	
	if (inters.length == 0)
	{
		if (fusion__shape_is_a_inside_b(shape_a, shape_b))
		{
			return shape_b;
		}
		else if (fusion__shape_is_a_inside_b(shape_b, shape_a))
		{
			return shape_a;
		} else {
			// TODO find a place to link the two shape without intersecting geometry
		}
	}
	
	new_shape_cap = 2 * shape_a.coo_array.length + shape_b.coo_array.length + inters.length;
	new_shape.coo_array.coos = malloc(new_shape_cap); /* Overallocate, shrink it down (Arena) once the length is known */
	new_shape.coo_array.length = 0;
	new_shape.positive = 1;

	/* Get all the segments of the first shape between intersection 1 and intersection N - 1 */
	for (i = 0; i < inters.lenght - 2; i++) /* stop on the one before the last (intersection N-1 if we have N intersections) */
	{
		for (j = inters.segments[i].first; j <= inters.segments[i+1].first; j++)
		{
			new_shape.coo_array.coos[new_shape.coo_array.length] = shape_a.coo_array.coos[j];
			new_shape.coo_array.length = new_shape.coo_array.length + 1;
		}
	}
	
	/* Here is just on the vertex of the shape_a segment of the intersection N-1 (N inters) */

	/* Iterate backwards on the list of intersections, once on shape_b, the other time on shape_a */
	for (i = inters.length - 2; i > 0; i--)
	{
		// TODO Add the new intersection vertex
		j = inters.length - 2 - i; /* even: shape_b, odd: shape_a */
		if (j & 1) /* on shape_a */
		{
			for (k = inters.segments[i].first; k <= inters.segments[i-1].first - 1; k--)
			{
				new_shape.coo_array.coos[new_shape.coo_array.length] = shape_a.coo_array.coos[k];
				new_shape.coo_array.length = new_shape.coo_array.length + 1;
			}
		}
		else /* on shape_b */
		{
			for (	k = inters.segments[i].second + 1;
				k != 1 + inters.segments[i-1].second; /* stops after inters.segments[i-1].second */
				)
				/* k % shape_b.coo_array.length != 1 + inters.segments[i-1].second;  */
				/* k = (k + 1) % shape_b.coo_array.length	) */
			{
				new_shape.coo_array.coos[new_shape.coo_array.length] = shape_b.coo_array.coos[k];
				new_shape.coo_array.length = new_shape.coo_array.length + 1;

				k = k + 1;
				if (k == shape_b.coo_array.length)
				{
					k = 0;
				}
			}
		}
			
	}

	/* Here is on first intersection (last shape was shape_a) */
	// TODO Add the new intersection vertex

	for (	i = inters.segments[0].second; 
		i != 1 + inters.segments[inters.length - 1].second;
		)
		/* i % shape_b.coo_array.length != 1 + inters.segments[inters.length - 1].second; */
		/* i = (i+1) % shape_b.coo_array.length	) */
	{
		new_shape.coo_array.coos[new_shape.coo_array.length] = shape_b.coo_array.coos[i];
		new_shape.coo_array.length = new_shape.coo_array.length + 1;

		i = i + 1;
		if (i == shape_b.coo_array.length)
		{
			i = 0;
		}
	}

	// TODO Add the new intersection vertex
	
	/* close the shape by iterating on the shape_a from N to 1 */
	for (	i = inters.segments[inters.length - 1].first + 1;
		i != inters.segments[0].first; 
		) 
		/* i % shape_a.coo_array.length != inters.segments[0].first; */
		/* i = (i+1) % shape_a.coo_array.length	) */
	{
		new_shape.coo_array.coos[new_shape.coo_array.length] = shape_a.coo_array.coos[i];
		new_shape.coo_array.length = new_shape.coo_array.length + 1;
		
		i = i + 1;
		if (i == shape_a.coo_array.length)
		{
			i = 0;
		}
	}

	// TODO shrink new_array.coo_array.coos to new_shape.coo_array.length
	
	return new_shape;
}
