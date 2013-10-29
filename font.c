#include <math.h>
#include <glib.h>
#include <GL/gl.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_BITMAP_H
#include FT_SYNTHESIS_H

#include "font.h"
#include "common.h"
#include "conf.h"
#include "util.h"

typedef struct _Glyph Glyph;
typedef struct _GlyphStore GlyphStore;
typedef struct _Face Face;

struct _Glyph
{
	int index;
	int advance;
	GLuint gl_list;
	GLuint gl_texture;
	GLuint gl_shadow_list;
	GLuint gl_shadow_texture;
};

struct _GlyphStore
{
	float size;
	Glyph *glyphs[128];
};

struct _Face
{
	char *name;
	FT_Face ft_face;
	GSList *glyph_stores;
};

static FT_Library Library;
static GHashTable *Faces;

static Face *CurrentFace;
static GlyphStore *CurrentGlyphs;

static Glyph *font_glyph_new(unsigned char code);
static FT_Glyph font_glyph_load(Glyph *glyph);
static void font_glyph_compile(GLuint *texture, GLuint *list,
		FT_BitmapGlyph bitmap_glyph, FT_Bitmap *bitmap,
		gboolean shadow);

void font_init()
{
	FT_Error error;
	
	if((error = FT_Init_FreeType(&Library)))
	{
		g_critical("Error initialisaing FreeType library");
		exit(1);
	}

	Faces = g_hash_table_new(g_str_hash, g_str_equal);
}

void font_set_face(const char *name)
{
	Face *face = g_hash_table_lookup(Faces, name);

	if(!face)
	{
		face = g_new0(Face, 1);
		face->name = g_strdup(name);

		FT_Error error;
		if((error = FT_New_Face(Library, name, 0, &face->ft_face)))
		{
			g_critical("%s: error reading font face", name);
			exit(1);
		}

		g_hash_table_insert(Faces, face->name, face);
	}

	CurrentFace = face;
	CurrentGlyphs = NULL;
}

void font_set_size(float size)
{
	if(CurrentGlyphs && CurrentGlyphs->size == size)
		return;

	FT_Set_Char_Size(CurrentFace->ft_face, 0, (int) (size*64.0), 96, 96);

	GlyphStore *glyphs;
	for(GSList *i = CurrentFace->glyph_stores; i; i = i->next)
	{
		glyphs = i->data;

		if(glyphs->size == size)
		{
			CurrentGlyphs = glyphs;
			return;
		}
	}

	glyphs = g_new0(GlyphStore, 1);
	glyphs->size = size;

	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);

	for(int i = 0; i < 128; ++i)
		glyphs->glyphs[i] = font_glyph_new(i);

	glPopAttrib();

	CurrentFace->glyph_stores = g_slist_append(CurrentFace->glyph_stores,
			glyphs);
	CurrentGlyphs = glyphs;
}

void font_set_element(const char *element)
{
	for(GList *i = ConfFonts; i; i = i->next)
	{
		ConfFont *conf_font = i->data;

		if(strcmp(conf_font->element, element) == 0)
		{
			if(conf_font->filename)
				font_set_face(conf_font->filename);
			else
				font_set_face(ConfFontDefault);

			if(conf_font->size > 0)
				font_set_size(conf_font->size);
			else
				font_set_size(ConfFontDefaultSize);

			return;
		}
	}

	font_set_face(ConfFontDefault);
	font_set_size(ConfFontDefaultSize);
}

int font_get_width(const char *s)
{
	int width = 0;

	for(const char *c = s; *c; ++c)
		width += CurrentGlyphs->glyphs[(int)(*c)]->advance;

	return width;
}

int font_get_line_height()
{
	return CurrentFace->ft_face->size->metrics.height;
}

int font_get_height()
{
	return font_get_ascender() + font_get_descender();
}

int font_get_ascender()
{
	return CurrentFace->ft_face->size->metrics.ascender;
}

int font_get_descender()
{
	return CurrentFace->ft_face->size->metrics.descender;
}

int font_get_wrapped_lines(const char *s, int width)
{
	int lines = 1;
	int x = 0;
	const char *last_wrap_point = NULL;

	for(const char *c = s; *c; ++c)
	{
		const Glyph *glyph = CurrentGlyphs->glyphs[(int)(*c)];

		if(*c == ' ')
			last_wrap_point = c;

		if(x + glyph->advance/64 >= width)
		{
			++lines;
			c = last_wrap_point;
			x = 0;
		} else
			x += glyph->advance/64;
	}

	g_debug("lines=%d", lines);

	return lines;
}

void font_draw_wrapped(const char *s, int x, int y, int width)
{
	GSList *wrap_points = NULL;

	int cur_x = 0;
	const char *last_wrap_point = NULL;

	for(const char *c = s; *c; ++c)
	{
		if(*c < 0)
			continue;

		const Glyph *glyph = CurrentGlyphs->glyphs[(int)(*c)];

		if(*c == ' ')
			last_wrap_point = c;

		if(cur_x + glyph->advance/64 >= width)
		{
			wrap_points = g_slist_append(wrap_points,
					(char *) last_wrap_point);
			c = last_wrap_point;
			cur_x = 0;
		} else
			cur_x += glyph->advance/64;
	}

	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glPushMatrix();
	glTranslatef(x, y, 0);

	GSList *wrap_point = wrap_points;

	for(const char *c = s; *c; ++c)
	{
		if(*c < 0)
			continue;

		const Glyph *glyph = CurrentGlyphs->glyphs[(int)(*c)];

		if(wrap_point && c == wrap_point->data)
		{
			wrap_point = wrap_point->next;
			glPopMatrix();
			glPushMatrix();
			y -= font_get_line_height()/64;
			glTranslatef(x, y, 0);
		} else
		{
			if(ConfFontShadow)
			{
				int offset = ConfFontShadowSize/128;
				glPushAttrib(GL_CURRENT_BIT);
				glPushMatrix();
				glTranslatef(-offset, offset, 0);
				set_colour("font-shadow", TRUE);
				glCallList(glyph->gl_shadow_list);
				glPopMatrix();
				glPopAttrib();
			}

			glCallList(glyph->gl_list);
			glTranslatef(glyph->advance/64, 0, 0);
		}
	}

	glPopMatrix();
	glPopAttrib();

	g_slist_free(wrap_points);
}

void font_draw(const char *s, int x, int y)
{
	glPushAttrib(GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glPushMatrix();
	glTranslatef(x, y, 0);

	for(const char *c = s; *c; ++c)
	{
		if(*c < 0)
			continue;

		const Glyph *glyph = CurrentGlyphs->glyphs[(int)(*c)];

		if(ConfFontShadow)
		{
			int offset = ConfFontShadowSize/128;
			glPushAttrib(GL_CURRENT_BIT);
			glPushMatrix();
			glTranslatef(-offset, offset, 0);
			set_colour("font-shadow", TRUE);
			glCallList(glyph->gl_shadow_list);
			glPopMatrix();
			glPopAttrib();
		}

		glCallList(glyph->gl_list);
		glTranslatef(glyph->advance/64, 0, 0);
	}

	glPopMatrix();
	glPopAttrib();
}

static Glyph *font_glyph_new(unsigned char code)
{
	Glyph *glyph = g_new0(Glyph, 1);

	glyph->index = FT_Get_Char_Index(CurrentFace->ft_face, code);

	FT_Glyph ft_glyph = font_glyph_load(glyph);

	FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph) ft_glyph;
	FT_Bitmap *bitmap = &bitmap_glyph->bitmap;

	font_glyph_compile(&glyph->gl_texture, &glyph->gl_list,
			bitmap_glyph, bitmap, FALSE);
	glyph->advance = CurrentFace->ft_face->glyph->advance.x;

	if(ConfFontShadow)
	{
		FT_Bitmap_Embolden(Library, bitmap,
				ConfFontShadowSize,
				ConfFontShadowSize);
		font_glyph_compile(&glyph->gl_shadow_texture,
				&glyph->gl_shadow_list,
				bitmap_glyph, bitmap, TRUE);
		glyph->advance += ConfFontShadowSize/3;
	}

	return glyph;
}

static FT_Glyph font_glyph_load(Glyph *glyph)
{
	FT_Glyph ft_glyph;

	FT_Load_Glyph(CurrentFace->ft_face, glyph->index, FT_LOAD_DEFAULT);

	FT_Get_Glyph(CurrentFace->ft_face->glyph, &ft_glyph);

	if(ft_glyph->format != FT_GLYPH_FORMAT_BITMAP)
	{
		FT_Error error;
		error = FT_Glyph_To_Bitmap(&ft_glyph,
				FT_RENDER_MODE_NORMAL, 0, 0);
		if(error)
		{
			g_critical("FT_Glyph_To_Bimap error");
			exit(1);
		}
	}

	return ft_glyph;
}


static void font_glyph_compile(GLuint *texture, GLuint *list,
		FT_BitmapGlyph bitmap_glyph, FT_Bitmap *bitmap,
		gboolean shadow)
{
	int width = 1 << (int)ceil(log(MAX(bitmap->width, 8))/M_LN2);
	int height = 1 << (int)ceil(log(MAX(bitmap->rows, 8))/M_LN2);

	GLubyte *data = g_malloc(2*width*height);
	for(int y = 0; y < bitmap->rows; ++y)
	{
		for(int x = 0; x < bitmap->width; ++x)
		{
			unsigned char p = bitmap->buffer[x + y*bitmap->pitch];
			if(shadow)
				data[2*(x + y*width) + 0] = p;
			else
				data[2*(x + y*width) + 0] = p ? 0xFF : 0x00;

			data[2*(x + y*width) + 1] = p;
		}
	}

	glGenTextures(1, texture);
	glBindTexture(GL_TEXTURE_2D, *texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
			GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data);

	g_free(data);

	*list = glGenLists(1);
	glNewList(*list, GL_COMPILE);

	glBindTexture(GL_TEXTURE_2D, *texture);
	glPushMatrix();

	glTranslatef(bitmap_glyph->left, bitmap_glyph->top - bitmap->rows, 0);

	float x = (float)bitmap->width/(float)width;
	float y = (float)bitmap->rows/(float)height;

	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2i(0, bitmap->rows);
	glTexCoord2f(0, y);
	glVertex2i(0, 0);
	glTexCoord2f(x, y);
	glVertex2i(bitmap->width, 0);
	glTexCoord2f(x, 0);
	glVertex2i(bitmap->width, bitmap->rows);
	glEnd();

	glPopMatrix();

	glEndList();

}

