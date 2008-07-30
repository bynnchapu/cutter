/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
 *
 *  This library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __CUT_ANALYZER_H__
#define __CUT_ANALYZER_H__

#include <glib-object.h>

#include <cutter/cut-run-context.h>

G_BEGIN_DECLS

#define CUT_TYPE_ANALYZER            (cut_analyzer_get_type ())
#define CUT_ANALYZER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CUT_TYPE_ANALYZER, CutAnalyzer))
#define CUT_ANALYZER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CUT_TYPE_ANALYZER, CutAnalyzerClass))
#define CUT_IS_ANALYZER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CUT_TYPE_ANALYZER))
#define CUT_IS_ANALYZER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CUT_TYPE_ANALYZER))
#define CUT_ANALYZER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CUT_TYPE_ANALYZER, CutAnalyzerClass))

typedef struct _CutAnalyzer      CutAnalyzer;
typedef struct _CutAnalyzerClass CutAnalyzerClass;

struct _CutAnalyzer
{
    CutRunContext object;
};

struct _CutAnalyzerClass
{
    CutRunContextClass parent_class;
};

GType          cut_analyzer_get_type  (void) G_GNUC_CONST;

CutRunContext *cut_analyzer_new       (void);
CutRunContext *cut_analyzer_new_from_run_context
                                      (CutRunContext *run_context);

G_END_DECLS

#endif /* __CUT_ANALYZER_H__ */

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
