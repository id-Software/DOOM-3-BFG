/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __DECLAF_H__
#define __DECLAF_H__

/*
===============================================================================

	Articulated Figure

===============================================================================
*/

class idDeclAF;

typedef enum
{
	DECLAF_CONSTRAINT_INVALID,
	DECLAF_CONSTRAINT_FIXED,
	DECLAF_CONSTRAINT_BALLANDSOCKETJOINT,
	DECLAF_CONSTRAINT_UNIVERSALJOINT,
	DECLAF_CONSTRAINT_HINGE,
	DECLAF_CONSTRAINT_SLIDER,
	DECLAF_CONSTRAINT_SPRING
} declAFConstraintType_t;

typedef enum
{
	DECLAF_JOINTMOD_AXIS,
	DECLAF_JOINTMOD_ORIGIN,
	DECLAF_JOINTMOD_BOTH
} declAFJointMod_t;

typedef bool ( *getJointTransform_t )( void* model, const idJointMat* frame, const char* jointName, idVec3& origin, idMat3& axis );

class idAFVector
{
public:
	enum
	{
		VEC_COORDS = 0,
		VEC_JOINT,
		VEC_BONECENTER,
		VEC_BONEDIR
	}						type;
	idStr					joint1;
	idStr					joint2;

public:
	idAFVector();

	bool					Parse( idLexer& src );
	bool					Finish( const char* fileName, const getJointTransform_t GetJointTransform, const idJointMat* frame, void* model ) const;
	bool					Write( idFile* f ) const;
	const char* 			ToString( idStr& str, const int precision = 8 );
	const idVec3& 			ToVec3() const
	{
		return vec;
	}
	idVec3& 				ToVec3()
	{
		return vec;
	}

private:
	mutable idVec3			vec;
	bool					negate;
};

class idDeclAF_Body
{
public:
	idStr					name;
	idStr					jointName;
	declAFJointMod_t		jointMod;
	int						modelType;
	idAFVector				v1, v2;
	int						numSides;
	float					width;
	float					density;
	idAFVector				origin;
	idAngles				angles;
	int						contents;
	int						clipMask;
	bool					selfCollision;
	idMat3					inertiaScale;
	float					linearFriction;
	float					angularFriction;
	float					contactFriction;
	idStr					containedJoints;
	idAFVector				frictionDirection;
	idAFVector				contactMotorDirection;
public:
	void					SetDefault( const idDeclAF* file );
};

class idDeclAF_Constraint
{
public:
	idStr					name;
	idStr					body1;
	idStr					body2;
	declAFConstraintType_t	type;
	float					friction;
	float					stretch;
	float					compress;
	float					damping;
	float					restLength;
	float					minLength;
	float					maxLength;
	idAFVector				anchor;
	idAFVector				anchor2;
	idAFVector				shaft[2];
	idAFVector				axis;
	enum
	{
		LIMIT_NONE = -1,
		LIMIT_CONE,
		LIMIT_PYRAMID
	}						limit;
	idAFVector				limitAxis;
	float					limitAngles[3];

public:
	void					SetDefault( const idDeclAF* file );
};

class idDeclAF : public idDecl
{
	friend class idAFFileManager;
public:
	idDeclAF();
	virtual					~idDeclAF();

	virtual size_t			Size() const;
	virtual const char* 	DefaultDefinition() const;
	virtual bool			Parse( const char* text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();

	virtual void			Finish( const getJointTransform_t GetJointTransform, const idJointMat* frame, void* model ) const;

	bool					Save();

	void					NewBody( const char* name );
	void					RenameBody( const char* oldName, const char* newName );
	void					DeleteBody( const char* name );

	void					NewConstraint( const char* name );
	void					RenameConstraint( const char* oldName, const char* newName );
	void					DeleteConstraint( const char* name );

	static int				ContentsFromString( const char* str );
	static const char* 		ContentsToString( const int contents, idStr& str );

	static declAFJointMod_t	JointModFromString( const char* str );
	static const char* 		JointModToString( declAFJointMod_t jointMod );

public:
	bool					modified;
	idStr					model;
	idStr					skin;
	float					defaultLinearFriction;
	float					defaultAngularFriction;
	float					defaultContactFriction;
	float					defaultConstraintFriction;
	float					totalMass;
	idVec2					suspendVelocity;
	idVec2					suspendAcceleration;
	float					noMoveTime;
	float					noMoveTranslation;
	float					noMoveRotation;
	float					minMoveTime;
	float					maxMoveTime;
	int						contents;
	int						clipMask;
	bool					selfCollision;
	idList<idDeclAF_Body*, TAG_IDLIB_LIST_PHYSICS>			bodies;
	idList<idDeclAF_Constraint*, TAG_IDLIB_LIST_PHYSICS>	constraints;

private:
	bool					ParseContents( idLexer& src, int& c ) const;
	bool					ParseBody( idLexer& src );
	bool					ParseFixed( idLexer& src );
	bool					ParseBallAndSocketJoint( idLexer& src );
	bool					ParseUniversalJoint( idLexer& src );
	bool					ParseHinge( idLexer& src );
	bool					ParseSlider( idLexer& src );
	bool					ParseSpring( idLexer& src );
	bool					ParseSettings( idLexer& src );

	bool					WriteBody( idFile* f, const idDeclAF_Body& body ) const;
	bool					WriteFixed( idFile* f, const idDeclAF_Constraint& c ) const;
	bool					WriteBallAndSocketJoint( idFile* f, const idDeclAF_Constraint& c ) const;
	bool					WriteUniversalJoint( idFile* f, const idDeclAF_Constraint& c ) const;
	bool					WriteHinge( idFile* f, const idDeclAF_Constraint& c ) const;
	bool					WriteSlider( idFile* f, const idDeclAF_Constraint& c ) const;
	bool					WriteSpring( idFile* f, const idDeclAF_Constraint& c ) const;
	bool					WriteConstraint( idFile* f, const idDeclAF_Constraint& c ) const;
	bool					WriteSettings( idFile* f ) const;

	bool					RebuildTextSource();
};

#endif /* !__DECLAF_H__ */
