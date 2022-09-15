/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2022 Stephen Pridham

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef EDITOR_TOOLS_AFEDITOR_H_
#define EDITOR_TOOLS_AFEDITOR_H_

#include "../../edit_public.h"

#include "AfBodyEditor.h"
#include "AfConstraintEditor.h"
#include "AfPropertyEditor.h"

namespace ImGuiTools
{

/**
* Articulated figure imgui editor.
*/
class AfEditor
{
public:
	virtual	~AfEditor();

	void				Init();

	void				ShowIt( bool show );

	bool				IsShown() const;

	void				Draw();

public:

	static AfEditor&	Instance();
	static void			Enable( const idCmdArgs& args );

public:

	AfEditor( AfEditor const& ) = delete;
	void operator=( AfEditor const& ) = delete;

private:

	struct AfList
	{
		AfList() : names(), shouldPopulate( false ) {}
		void populate();

		idList<idStr>	names;
		bool			shouldPopulate;
	};

	AfEditor();

	void					OnNewDecl( idDeclAF* newDecl );

	bool					isShown;
	int						fileSelection;
	int						currentAf;
	int						currentConstraint;
	int						currentBodySelection;
	int						currentEntity;
	idDeclAF*				decl;
	idDeclAF_Body*			body;
	idDeclAF_Constraint*	constraint;

	// Editor dialogs
	AfPropertyEditor*			propertyEditor;
	idList<AfBodyEditor*>		bodyEditors;
	idList<AfConstraintEditor*> constraintEditors;

	AfList					afList; // list with idDeclAF names
	idList<idStr>			afFiles;
	idStr					fileName;

	struct IndexEntityDef
	{
		int index;
		idStr name;
	};
	idList<IndexEntityDef>	entities;
};

inline void AfEditor::ShowIt( bool show )
{
	isShown = show;
}

inline bool AfEditor::IsShown() const
{
	return isShown;
}

}

#endif