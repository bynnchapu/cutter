/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2008  g新部 Hiroyuki Ikezoe  <poincare@ikezoe.net>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 *  Boston, MA  02111-1307  USA
 *
 */

#ifndef __CUT_REPORT_H__
#define __CUT_REPORT_H__

#include <glib-object.h>

#include <cutter/cut-private.h>
#include <cutter/cut-test-result.h>
#include <cutter/cut-verbose-level.h>

G_BEGIN_DECLS

#define CUT_TYPE_REPORT            (cut_report_get_type ())
#define CUT_REPORT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CUT_TYPE_REPORT, CutReport))
#define CUT_REPORT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CUT_TYPE_REPORT, CutReportClass))
#define CUT_IS_REPORT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CUT_TYPE_REPORT))
#define CUT_IS_REPORT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CUT_TYPE_REPORT))
#define CUT_REPORT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CUT_TYPE_REPORT, CutReportClass))

typedef struct _CutReport         CutReport;
typedef struct _CutReportClass    CutReportClass;

struct _CutReport
{
    GObject object;
};

struct _CutReportClass
{
    GObjectClass parent_class;
};

GType           cut_report_get_type  (void) G_GNUC_CONST;

void            cut_report_init        (void);
void            cut_report_quit        (void);

const gchar    *cut_report_get_default_module_dir   (void);
void            cut_report_set_default_module_dir   (const gchar *dir);

void            cut_report_load        (const gchar *base_dir);
void            cut_report_unload      (void);
GList          *cut_report_get_registered_types (void);
GList          *cut_report_get_log_domains      (void);

CutReport      *cut_report_new         (const gchar *name,
                                        const gchar *first_property,
                                        ...);

G_END_DECLS

#endif /* __CUT_REPORT_H__ */

/*
vi:nowrap:ai:expandtab:sw=4
*/
