#pragma hdrstop
#include "../../idlib/precompiled.h"

#include "../tr_local.h"

/*
=================
RB_ShutdownDebugTools
=================
*/
void RB_ShutdownDebugTools() {
	// TODO
	/*for (int i = 0; i < MAX_DEBUG_POLYGONS; i++) {
		rb_debugPolygons[i].winding.Clear();
	}*/
}

void DumpAllDisplayDevices() {
	// TODO: Print all display devices.
}

/*
================
RB_DrawTextLength

  returns the length of the given text
================
*/
float RB_DrawTextLength(const char* text, float scale, int len) {
	int i, num, index, charIndex;
	float spacing, textLen = 0.0f;

	// TODO: Implement
	/*if (text && *text) {
		if (!len) {
			len = strlen(text);
		}
		for (i = 0; i < len; i++) {
			charIndex = text[i] - 32;
			if (charIndex < 0 || charIndex > NUM_SIMPLEX_CHARS) {
				continue;
			}
			num = simplex[charIndex][0] * 2;
			spacing = simplex[charIndex][1];
			index = 2;

			while (index - 2 < num) {
				if (simplex[charIndex][index] < 0) {
					index++;
					continue;
				}
				index += 2;
				if (simplex[charIndex][index] < 0) {
					index++;
					continue;
				}
			}
			textLen += spacing * scale;
		}
	}*/
	return textLen;
}

void RB_AddDebugText(const char* text, const idVec3& origin, float scale, const idVec4& color, const idMat3& viewAxis, const int align, const int lifetime, const bool depthTest) {
	/*debugText_t* debugText;

	if (rb_numDebugText < MAX_DEBUG_TEXT) {
		debugText = &rb_debugText[rb_numDebugText++];
		debugText->text = text;
		debugText->origin = origin;
		debugText->scale = scale;
		debugText->color = color;
		debugText->viewAxis = viewAxis;
		debugText->align = align;
		debugText->lifeTime = rb_debugTextTime + lifetime;
		debugText->depthTest = depthTest;
	}*/
}

/*
================
RB_ClearDebugText
================
*/
void RB_ClearDebugText(int time) {
	//TODO: Implement
	/*int			i;
	int			num;
	debugText_t* text;

	rb_debugTextTime = time;

	if (!time) {
		// free up our strings
		text = rb_debugText;
		for (i = 0; i < MAX_DEBUG_TEXT; i++, text++) {
			text->text.Clear();
		}
		rb_numDebugText = 0;
		return;
	}

	// copy any text that still needs to be drawn
	num = 0;
	text = rb_debugText;
	for (i = 0; i < rb_numDebugText; i++, text++) {
		if (text->lifeTime > time) {
			if (num != i) {
				rb_debugText[num] = *text;
			}
			num++;
		}
	}
	rb_numDebugText = num;*/
}

/*
================
RB_AddDebugLine
================
*/
void RB_AddDebugLine(const idVec4& color, const idVec3& start, const idVec3& end, const int lifeTime, const bool depthTest) {
	// TODO: Implement
	/*debugLine_t* line;

	if (rb_numDebugLines < MAX_DEBUG_LINES) {
		line = &rb_debugLines[rb_numDebugLines++];
		line->rgb = color;
		line->start = start;
		line->end = end;
		line->depthTest = depthTest;
		line->lifeTime = rb_debugLineTime + lifeTime;
	}*/
}

/*
================
RB_ClearDebugLines
================
*/
void RB_ClearDebugLines(int time) {
	// TODO: Implement
	//int			i;
	//int			num;
	//debugLine_t* line;

	//rb_debugLineTime = time;

	//if (!time) {
	//	rb_numDebugLines = 0;
	//	return;
	//}

	//// copy any lines that still need to be drawn
	//num = 0;
	//line = rb_debugLines;
	//for (i = 0; i < rb_numDebugLines; i++, line++) {
	//	if (line->lifeTime > time) {
	//		if (num != i) {
	//			rb_debugLines[num] = *line;
	//		}
	//		num++;
	//	}
	//}
	//rb_numDebugLines = num;
}

/*
================
RB_ClearDebugPolygons
================
*/
void RB_ClearDebugPolygons(int time) {
	// TODO: Implement
	//int				i;
	//int				num;
	//debugPolygon_t* poly;

	//rb_debugPolygonTime = time;

	//if (!time) {
	//	rb_numDebugPolygons = 0;
	//	return;
	//}

	//// copy any polygons that still need to be drawn
	//num = 0;

	//poly = rb_debugPolygons;
	//for (i = 0; i < rb_numDebugPolygons; i++, poly++) {
	//	if (poly->lifeTime > time) {
	//		if (num != i) {
	//			rb_debugPolygons[num] = *poly;
	//		}
	//		num++;
	//	}
	//}
	//rb_numDebugPolygons = num;
}

/*
================
RB_AddDebugPolygon
================
*/
void RB_AddDebugPolygon(const idVec4& color, const idWinding& winding, const int lifeTime, const bool depthTest) {
	// TODO: Implement
	/*debugPolygon_t* poly;

	if (rb_numDebugPolygons < MAX_DEBUG_POLYGONS) {
		poly = &rb_debugPolygons[rb_numDebugPolygons++];
		poly->rgb = color;
		poly->winding = winding;
		poly->depthTest = depthTest;
		poly->lifeTime = rb_debugPolygonTime + lifeTime;
	}*/
}