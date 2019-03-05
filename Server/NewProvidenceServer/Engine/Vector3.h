/*****************************************************************/
/*	           ___                          _  _                 */
/*	    	  / _ \                        | |(_)                */
/*           / /_\ \ _ __   ___   __ _   __| | _   __ _          */
/*    	     |  _  || '__| / __| / _` | / _` || | / _` |         */
/*	         | | | || |   | (__ | (_| || (_| || || (_| |         */
/*	         \_| |_/|_|    \___| \__,_| \__,_||_| \__,_|         */
/*                                                               */
/*                                      Engine Version 01.00.00  */
/*****************************************************************/
/*  File:       Vector3.h                                        */
/*                                                               */
/*  Purpose:    This file contains a class which can be used to  */
/*              create a group of 3 of any data type which can   */
/*              be manipulated with basic mathematic functions.  */
/*                                                               */
/*  Created:    12/07/2008                                       */
/*  Last Edit:  03/02/2014                                       */
/*****************************************************************/

#ifndef VECTOR_CLASS_H
#define VECTOR_CLASS_H

#include <math.h>

#define VECTOR3_BINARY_COMPONENT_OPERATOR( oper ) \
Vector3<data_type> operator oper( const Vector3<data_type>& other ) const \
{ \
	Vector3<data_type> vector; \
	vector.x = x oper other.x; \
	vector.y = y oper other.y; \
	vector.z = z oper other.z; \
	return vector; \
}

#define VECTOR3_BINARY_BROADCAST_OPERATOR( oper ) \
template <typename data_type> \
Vector3<data_type> operator oper( data_type data ) const \
{ \
	Vector3<data_type> vector; \
	vector.x = x oper data; \
	vector.y = y oper data; \
	vector.z = z oper data; \
	return vector; \
}


#define VECTOR3_COMPONENT_OPERATOR( oper ) \
Vector3& operator oper( const Vector3<data_type>& vector ) \
{ \
	if ( static_cast<void*>(this) != static_cast<const void*>(&vector) ) \
	{ \
		x oper vector.x; \
		y oper vector.y; \
		z oper vector.z; \
	} \
	return (*this); \
}


#define VECTOR3_BROADCAST_OPERATOR( oper ) \
template <typename data_type> \
Vector3& operator oper( data_type value ) \
{ \
	x oper value; \
	y oper value; \
	z oper value; \
	return (*this); \
}


template <typename data_type>
struct Vector3
{
	union
	{
		// data
		data_type data[3];

		// position
		struct
		{
			data_type x;
			data_type y;
			data_type z;
		};
	};

	Vector3(void) { }
	~Vector3(void) { }

	Vector3(data_type first, data_type second, data_type third) :
		x(first),
		y(second),
		z(third)
	{}

	Vector3(const Vector3<data_type>& vector) :
		x(vector.x),
		y(vector.y),
		z(vector.z)
	{}

	Vector3<data_type>& Nullify(void)
	{
		x = (data_type)(0);
		y = (data_type)(0);
		z = (data_type)(0);
	}

	data_type Magnitude_Squared(void)
	{
		return data_type(x * x + y * y + z * z);
	}

	data_type Magnitude(void)
	{
		return data_type(sqrt(x * x + y * y + z * z));
	}

	data_type Magnitude_X_Z(void)
	{
		return data_type(sqrt(x * x + z * z));
	}

	Vector3& Normalize(void)
	{
		data_type reverse_square_root = data_type(1) / Magnitude();
		x *= reverse_square_root;
		y *= reverse_square_root;
		z *= reverse_square_root;
		return (*this);
	}

	static data_type Dot_Product(const Vector3<data_type>& vector_a, const Vector3<data_type>& vector_b)
	{
		return (vector_a.x * vector_b.x + vector_a.y * vector_b.y + vector_a.z * vector_b.z);
	}

	static void Cross_Product(Vector3& output, const Vector3& vector_a, const Vector3& vector_b)
	{
		output.x = vector_a.y * vector_b.z - vector_a.z * vector_b.y;
		output.y = vector_a.z * vector_b.x - vector_a.x * vector_b.z;
		output.z = vector_a.x * vector_b.y - vector_a.y * vector_b.x;
	}

	static Vector3<data_type> Interpolate(const Vector3& vector_a, const Vector3& vector_b, float lamda)
	{
		return (vector_a + ((vector_b - vector_a) * lamda));
	}

	//  "array" access
	data_type* array(void) { return this->data; }
	data_type const* array(void) const { return this->data; }

	//  Mathematical operators
	VECTOR3_BINARY_COMPONENT_OPERATOR(+);
	VECTOR3_BINARY_COMPONENT_OPERATOR(-);
	VECTOR3_BINARY_COMPONENT_OPERATOR(*);
	VECTOR3_BINARY_COMPONENT_OPERATOR(/ );

	VECTOR3_BINARY_BROADCAST_OPERATOR(+);
	VECTOR3_BINARY_BROADCAST_OPERATOR(-);
	VECTOR3_BINARY_BROADCAST_OPERATOR(*);
	VECTOR3_BINARY_BROADCAST_OPERATOR(/ );

	VECTOR3_COMPONENT_OPERATOR(= );
	VECTOR3_COMPONENT_OPERATOR(+= );
	VECTOR3_COMPONENT_OPERATOR(-= );
	VECTOR3_COMPONENT_OPERATOR(*= );
	VECTOR3_COMPONENT_OPERATOR(/= );

	VECTOR3_BROADCAST_OPERATOR(= );
	VECTOR3_BROADCAST_OPERATOR(+= );
	VECTOR3_BROADCAST_OPERATOR(-= );
	VECTOR3_BROADCAST_OPERATOR(*= );
	VECTOR3_BROADCAST_OPERATOR(/= );
};

#endif