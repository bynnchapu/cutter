/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Copyright (C) 2007  Kouhei Sutou <kou@cozmixng.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "cut-context.h"
#include "cut-test-case.h"

#include "cut-marshalers.h"

#define CUT_CONTEXT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), CUT_TYPE_CONTEXT, CutContextPrivate))

typedef struct _CutContextPrivate	CutContextPrivate;
struct _CutContextPrivate
{
    guint n_tests;
    guint n_assertions;
    guint n_failures;
    guint n_errors;
    guint n_pendings;
    guint n_notifications;
    GList *results;
    gboolean use_multi_thread;
    GMutex *mutex;
    gboolean crashed;
    gchar *stack_trace;
    gchar *source_directory;
    gchar **target_test_case_names;
    gchar **target_test_names;
};

enum
{
    PROP_0,
    PROP_N_TESTS,
    PROP_N_ASSERTIONS,
    PROP_N_FAILURES,
    PROP_N_ERRORS,
    PROP_N_PENDINGS,
    PROP_N_NOTIFICATIONS,
    PROP_USE_MULTI_THREAD
};

enum
{
    START_SIGNAL,

    START_TEST_SUITE,
    START_TEST_CASE,
    START_TEST,

    PASS,
    SUCCESS,
    FAILURE,
    ERROR,
    PENDING,
    NOTIFICATION,

    COMPLETE_TEST,
    COMPLETE_TEST_CASE,
    COMPLETE_TEST_SUITE,

    CRASHED,

    LAST_SIGNAL
};

static gint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE (CutContext, cut_context, G_TYPE_OBJECT)

static void dispose        (GObject         *object);
static void set_property   (GObject         *object,
                            guint            prop_id,
                            const GValue    *value,
                            GParamSpec      *pspec);
static void get_property   (GObject         *object,
                            guint            prop_id,
                            GValue          *value,
                            GParamSpec      *pspec);

static void
cut_context_class_init (CutContextClass *klass)
{
    GObjectClass *gobject_class;
    GParamSpec *spec;

    gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose      = dispose;
    gobject_class->set_property = set_property;
    gobject_class->get_property = get_property;

    spec = g_param_spec_uint("n-tests",
                             "Number of tests",
                             "The number of tests of the context",
                             0, G_MAXUINT32, 0,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_N_TESTS, spec);

    spec = g_param_spec_uint("n-assertions",
                             "Number of assertions",
                             "The number of assertions of the context",
                             0, G_MAXUINT32, 0,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_N_ASSERTIONS, spec);

    spec = g_param_spec_uint("n-failures",
                             "Number of failures",
                             "The number of failures of the context",
                             0, G_MAXUINT32, 0,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_N_FAILURES, spec);

    spec = g_param_spec_uint("n-errors",
                             "Number of errors",
                             "The number of errors of the context",
                             0, G_MAXUINT32, 0,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_N_ERRORS, spec);

    spec = g_param_spec_uint("n-pendings",
                             "Number of pendings",
                             "The number of pendings of the context",
                             0, G_MAXUINT32, 0,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_N_PENDINGS, spec);

    spec = g_param_spec_boolean("use-multi-thread",
                                "Use multi thread",
                                "Whether use multi thread or not in the context",
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
    g_object_class_install_property(gobject_class, PROP_USE_MULTI_THREAD, spec);


	signals[START_TEST_SUITE]
        = g_signal_new ("start-test-suite",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (CutContextClass, start_test_suite),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__OBJECT,
                        G_TYPE_NONE, 1, CUT_TYPE_TEST_SUITE);

	signals[START_TEST_CASE]
        = g_signal_new ("start-test-case",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (CutContextClass, start_test_case),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__OBJECT,
                        G_TYPE_NONE, 1, CUT_TYPE_TEST_CASE);

	signals[START_TEST]
        = g_signal_new ("start-test",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (CutContextClass, start_test),
                        NULL, NULL,
                        _cut_marshal_VOID__OBJECT_OBJECT,
                        G_TYPE_NONE, 2, CUT_TYPE_TEST, CUT_TYPE_TEST_CONTEXT);

	signals[PASS]
        = g_signal_new ("pass",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (CutContextClass, pass),
                        NULL, NULL,
                        _cut_marshal_VOID__OBJECT_OBJECT,
                        G_TYPE_NONE, 2, CUT_TYPE_TEST, CUT_TYPE_TEST_CONTEXT);

	signals[SUCCESS]
        = g_signal_new ("success",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (CutContextClass, success),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__OBJECT,
                        G_TYPE_NONE, 1, CUT_TYPE_TEST);

	signals[FAILURE]
        = g_signal_new ("failure",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (CutContextClass, failure),
                        NULL, NULL,
                        _cut_marshal_VOID__OBJECT_OBJECT_OBJECT,
                        G_TYPE_NONE, 3,
                        CUT_TYPE_TEST, CUT_TYPE_TEST_CONTEXT,
                        CUT_TYPE_TEST_RESULT);

	signals[ERROR]
        = g_signal_new ("error",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (CutContextClass, error),
                        NULL, NULL,
                        _cut_marshal_VOID__OBJECT_OBJECT_OBJECT,
                        G_TYPE_NONE, 3,
                        CUT_TYPE_TEST, CUT_TYPE_TEST_CONTEXT,
                        CUT_TYPE_TEST_RESULT);

	signals[PENDING]
        = g_signal_new ("pending",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (CutContextClass, pending),
                        NULL, NULL,
                        _cut_marshal_VOID__OBJECT_OBJECT_OBJECT,
                        G_TYPE_NONE, 3,
                        CUT_TYPE_TEST, CUT_TYPE_TEST_CONTEXT,
                        CUT_TYPE_TEST_RESULT);

	signals[NOTIFICATION]
        = g_signal_new ("notification",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (CutContextClass, notification),
                        NULL, NULL,
                        _cut_marshal_VOID__OBJECT_OBJECT_OBJECT,
                        G_TYPE_NONE, 3,
                        CUT_TYPE_TEST, CUT_TYPE_TEST_CONTEXT,
                        CUT_TYPE_TEST_RESULT);

    signals[COMPLETE_TEST]
        = g_signal_new ("complete-test",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (CutContextClass, complete_test),
                        NULL, NULL,
                        _cut_marshal_VOID__OBJECT_OBJECT,
                        G_TYPE_NONE, 2, CUT_TYPE_TEST, CUT_TYPE_TEST_CONTEXT);

	signals[COMPLETE_TEST_CASE]
        = g_signal_new ("complete-test-case",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (CutContextClass, complete_test_case),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__OBJECT,
                        G_TYPE_NONE, 1, CUT_TYPE_TEST_CASE);

	signals[COMPLETE_TEST_SUITE]
        = g_signal_new ("complete-test-suite",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (CutContextClass, complete_test_suite),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__OBJECT,
                        G_TYPE_NONE, 1, CUT_TYPE_TEST_SUITE);

	signals[CRASHED]
        = g_signal_new ("crashed",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_LAST,
                        G_STRUCT_OFFSET (CutContextClass, crashed),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__STRING,
                        G_TYPE_NONE, 1, G_TYPE_STRING);

    g_type_class_add_private(gobject_class, sizeof(CutContextPrivate));
}

static void
cut_context_init (CutContext *context)
{
    CutContextPrivate *priv = CUT_CONTEXT_GET_PRIVATE(context);

    priv->n_tests = 0;
    priv->n_assertions = 0;
    priv->n_failures = 0;
    priv->n_errors = 0;
    priv->n_pendings = 0;
    priv->n_notifications = 0;
    priv->results = NULL;
    priv->use_multi_thread = FALSE;
    priv->mutex = g_mutex_new();
    priv->crashed = FALSE;
    priv->stack_trace = NULL;
    priv->source_directory = NULL;
    priv->target_test_case_names = NULL;
    priv->target_test_names = NULL;
}

static void
dispose (GObject *object)
{
    CutContextPrivate *priv = CUT_CONTEXT_GET_PRIVATE(object);

    if (priv->results) {
        g_list_foreach(priv->results, (GFunc)g_object_unref, NULL);
        g_list_free(priv->results);
        priv->results = NULL;
    }

    if (priv->mutex) {
        g_mutex_free(priv->mutex);
        priv->mutex = NULL;
    }

    g_free(priv->stack_trace);
    priv->stack_trace = NULL;

    g_free(priv->source_directory);
    priv->source_directory = NULL;

    g_strfreev(priv->target_test_case_names);
    priv->target_test_case_names = NULL;

    g_strfreev(priv->target_test_names);
    priv->target_test_names = NULL;

    G_OBJECT_CLASS(cut_context_parent_class)->dispose(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
    CutContextPrivate *priv = CUT_CONTEXT_GET_PRIVATE(object);

    switch (prop_id) {
      case PROP_N_TESTS:
        priv->n_tests = g_value_get_uint(value);
        break;
      case PROP_N_ASSERTIONS:
        priv->n_assertions = g_value_get_uint(value);
        break;
      case PROP_N_FAILURES:
        priv->n_failures = g_value_get_uint(value);
        break;
      case PROP_N_ERRORS:
        priv->n_errors = g_value_get_uint(value);
        break;
      case PROP_N_PENDINGS:
        priv->n_pendings = g_value_get_uint(value);
        break;
      case PROP_N_NOTIFICATIONS:
        priv->n_notifications = g_value_get_uint(value);
        break;
      case PROP_USE_MULTI_THREAD:
        priv->use_multi_thread = g_value_get_boolean(value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
    CutContextPrivate *priv = CUT_CONTEXT_GET_PRIVATE(object);

    switch (prop_id) {
      case PROP_N_TESTS:
        g_value_set_uint(value, priv->n_tests);
        break;
      case PROP_N_ASSERTIONS:
        g_value_set_uint(value, priv->n_assertions);
        break;
      case PROP_N_FAILURES:
        g_value_set_uint(value, priv->n_failures);
        break;
      case PROP_N_ERRORS:
        g_value_set_uint(value, priv->n_errors);
        break;
      case PROP_N_PENDINGS:
        g_value_set_uint(value, priv->n_pendings);
        break;
      case PROP_N_NOTIFICATIONS:
        g_value_set_uint(value, priv->n_notifications);
        break;
      case PROP_USE_MULTI_THREAD:
        g_value_set_boolean(value, priv->use_multi_thread);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

CutContext *
cut_context_new (void)
{
    return g_object_new(CUT_TYPE_CONTEXT, NULL);
}

void
cut_context_set_source_directory (CutContext *context, const gchar *directory)
{
    CutContextPrivate *priv = CUT_CONTEXT_GET_PRIVATE(context);

    g_free(priv->source_directory);
    priv->source_directory = g_strdup(directory);
}

const gchar *
cut_context_get_source_directory (CutContext *context)
{
    return CUT_CONTEXT_GET_PRIVATE(context)->source_directory;
}

void
cut_context_set_multi_thread (CutContext *context, gboolean use_multi_thread)
{
    CutContextPrivate *priv = CUT_CONTEXT_GET_PRIVATE(context);

    g_mutex_lock(priv->mutex);
    priv->use_multi_thread = use_multi_thread;
    g_mutex_unlock(priv->mutex);
}

gboolean
cut_context_get_multi_thread (CutContext *context)
{
    return CUT_CONTEXT_GET_PRIVATE(context)->use_multi_thread;
}

void
cut_context_set_target_test_case_names (CutContext *context,
                                        const gchar **names)
{
    CutContextPrivate *priv = CUT_CONTEXT_GET_PRIVATE(context);

    g_strfreev(priv->target_test_case_names);
    priv->target_test_case_names = g_strdupv((gchar **)names);
}

const gchar **
cut_context_get_target_test_case_names (CutContext *context)
{
    CutContextPrivate *priv = CUT_CONTEXT_GET_PRIVATE(context);
    return (const gchar **)(priv->target_test_case_names);
}

void
cut_context_set_target_test_names (CutContext *context,
                                   const gchar **names)
{
    CutContextPrivate *priv = CUT_CONTEXT_GET_PRIVATE(context);

    g_strfreev(priv->target_test_names);
    priv->target_test_names = g_strdupv((gchar **)names);
}

const gchar **
cut_context_get_target_test_names (CutContext *context)
{
    CutContextPrivate *priv = CUT_CONTEXT_GET_PRIVATE(context);
    return (const gchar **)(priv->target_test_names);
}

typedef struct _TestCallBackInfo
{
    CutContext *context;
    CutTestResult *result;
} TestCallBackInfo;

static void
cb_pass_assertion (CutTest *test, CutTestContext *test_context, gpointer data)
{
    TestCallBackInfo *info = data;
    CutContext *context;
    CutContextPrivate *priv;

    context = info->context;
    priv = CUT_CONTEXT_GET_PRIVATE(context);
    g_mutex_lock(priv->mutex);
    priv->n_assertions++;
    g_mutex_unlock(priv->mutex);

    g_signal_emit(context, signals[PASS], 0, test, test_context);
}

static void
cb_success (CutTest *test, gpointer data)
{
    TestCallBackInfo *info = data;
    CutContext *context;
    CutContextPrivate *priv;

    context = info->context;
    priv = CUT_CONTEXT_GET_PRIVATE(context);
    g_signal_emit(context, signals[SUCCESS], 0, test);
}

static void
cb_failure (CutTest *test, CutTestContext *test_context, CutTestResult *result,
            gpointer data)
{
    TestCallBackInfo *info = data;
    CutContext *context;
    CutContextPrivate *priv;

    context = info->context;
    priv = CUT_CONTEXT_GET_PRIVATE(context);
    g_mutex_lock(priv->mutex);
    info->result = g_object_ref(result);
    priv->n_failures++;
    g_mutex_unlock(priv->mutex);

    g_signal_emit(context, signals[FAILURE], 0, test, test_context, result);
}

static void
cb_error (CutTest *test, CutTestContext *test_context, CutTestResult *result,
          gpointer data)
{
    TestCallBackInfo *info = data;
    CutContext *context;
    CutContextPrivate *priv;

    context = info->context;
    priv = CUT_CONTEXT_GET_PRIVATE(context);
    g_mutex_lock(priv->mutex);
    info->result = g_object_ref(result);
    priv->n_errors++;
    g_mutex_unlock(priv->mutex);

    g_signal_emit(context, signals[ERROR], 0, test, test_context, result);
}

static void
cb_pending (CutTest *test, CutTestContext *test_context, CutTestResult *result,
            gpointer data)
{
    TestCallBackInfo *info = data;
    CutContext *context;
    CutContextPrivate *priv;

    context = info->context;
    priv = CUT_CONTEXT_GET_PRIVATE(context);
    g_mutex_lock(priv->mutex);
    info->result = g_object_ref(result);
    priv->n_pendings++;
    g_mutex_unlock(priv->mutex);

    g_signal_emit(context, signals[PENDING], 0, test, test_context, result);
}

static void
cb_notification (CutTest *test, CutTestContext *test_context,
                 CutTestResult *result, gpointer data)
{
    TestCallBackInfo *info = data;
    CutContext *context;
    CutContextPrivate *priv;

    context = info->context;
    priv = CUT_CONTEXT_GET_PRIVATE(context);
    g_mutex_lock(priv->mutex);
    info->result = g_object_ref(result);
    priv->n_notifications++;
    g_mutex_unlock(priv->mutex);

    g_signal_emit(context, signals[NOTIFICATION], 0, test, test_context, result);
}

static void
cb_complete (CutTest *test, gpointer data)
{
    TestCallBackInfo *info = data;
    CutContext *context;
    CutContextPrivate *priv;

    context = info->context;
    priv = CUT_CONTEXT_GET_PRIVATE(context);
    if (info->result) {
        g_mutex_lock(priv->mutex);
        priv->results = g_list_prepend(priv->results, info->result);
        g_mutex_unlock(priv->mutex);
    }

    g_signal_handlers_disconnect_by_func(test,
                                         G_CALLBACK(cb_pass_assertion),
                                         data);
    g_signal_handlers_disconnect_by_func(test,
                                         G_CALLBACK(cb_success),
                                         data);
    g_signal_handlers_disconnect_by_func(test,
                                         G_CALLBACK(cb_failure),
                                         data);
    g_signal_handlers_disconnect_by_func(test,
                                         G_CALLBACK(cb_error),
                                         data);
    g_signal_handlers_disconnect_by_func(test,
                                         G_CALLBACK(cb_pending),
                                         data);
    g_signal_handlers_disconnect_by_func(test,
                                         G_CALLBACK(cb_notification),
                                         data);
    g_signal_handlers_disconnect_by_func(test,
                                         G_CALLBACK(cb_complete),
                                         data);

    g_object_unref(info->context);
    g_free(info);
}

void
cut_context_start_test (CutContext *context, CutTest *test)
{
    TestCallBackInfo *info;
    CutContextPrivate *priv;

    info = g_new0(TestCallBackInfo, 1);
    info->context = context;
    g_object_ref(context);

    priv = CUT_CONTEXT_GET_PRIVATE(context);
    g_mutex_lock(priv->mutex);
    priv->n_tests++;
    g_mutex_unlock(priv->mutex);

    g_signal_connect(test, "pass_assertion",
                     G_CALLBACK(cb_pass_assertion), info);
    g_signal_connect(test, "success", G_CALLBACK(cb_success), info);
    g_signal_connect(test, "failure", G_CALLBACK(cb_failure), info);
    g_signal_connect(test, "error", G_CALLBACK(cb_error), info);
    g_signal_connect(test, "pending", G_CALLBACK(cb_pending), info);
    g_signal_connect(test, "notification", G_CALLBACK(cb_notification), info);
    g_signal_connect(test, "complete", G_CALLBACK(cb_complete), info);
}

static void
cb_start_test (CutTestCase *test_case, CutTest *test,
               CutTestContext *test_context, gpointer data)
{
    CutContext *context = data;

    g_signal_emit(context, signals[START_TEST], 0, test, test_context);
}

static void
cb_complete_test(CutTestCase *test_case, CutTest *test,
                 CutTestContext *test_context, gpointer data)
{
    CutContext *context = data;

    g_signal_emit(context, signals[COMPLETE_TEST], 0, test, test_context);
}

static void
cb_start_test_case(CutTestCase *test_case, gpointer data)
{
    CutContext *context = data;

    g_signal_emit(context, signals[START_TEST_CASE], 0, test_case);
}

static void
cb_complete_test_case(CutTestCase *test_case, gpointer data)
{
    CutContext *context = data;

    g_signal_handlers_disconnect_by_func(test_case,
                                         G_CALLBACK(cb_start_test), data);
    g_signal_handlers_disconnect_by_func(test_case,
                                         G_CALLBACK(cb_complete_test), data);

    g_signal_handlers_disconnect_by_func(test_case,
                                         G_CALLBACK(cb_start_test_case), data);
    g_signal_handlers_disconnect_by_func(test_case,
                                         G_CALLBACK(cb_complete_test_case),
                                         data);

    g_signal_emit(context, signals[COMPLETE_TEST_CASE], 0, test_case);
}

void
cut_context_start_test_case (CutContext *context, CutTestCase *test_case)
{
    g_signal_connect(test_case, "start-test",
                     G_CALLBACK(cb_start_test), context);
    g_signal_connect(test_case, "complete-test",
                     G_CALLBACK(cb_complete_test), context);

    g_signal_connect(test_case, "start",
                     G_CALLBACK(cb_start_test_case), context);
    g_signal_connect(test_case, "complete",
                     G_CALLBACK(cb_complete_test_case), context);
}

static void
cb_start_test_suite(CutTestSuite *test_suite, gpointer data)
{
    CutContext *context = data;

    g_signal_emit(context, signals[START_TEST_SUITE], 0, test_suite);
}

static void
cb_crashed_test_suite(CutTestSuite *test_suite, const gchar *stack_trace,
                      gpointer data)
{
    CutContext *context = data;
    CutContextPrivate *priv;

    priv = CUT_CONTEXT_GET_PRIVATE(context);
    priv->crashed = TRUE;
    g_free(priv->stack_trace);
    priv->stack_trace = g_strdup(stack_trace);

    g_signal_emit(context, signals[CRASHED], 0, priv->stack_trace);
}

static void
cb_complete_test_suite(CutTestSuite *test_suite, gpointer data)
{
    CutContext *context = data;

    g_signal_handlers_disconnect_by_func(test_suite,
                                         G_CALLBACK(cb_start_test_suite), data);
    g_signal_handlers_disconnect_by_func(test_suite,
                                         G_CALLBACK(cb_complete_test_suite),
                                         data);
    g_signal_handlers_disconnect_by_func(test_suite,
                                         G_CALLBACK(cb_crashed_test_suite),
                                         data);

    g_signal_emit(context, signals[COMPLETE_TEST_SUITE], 0, test_suite);
}

void
cut_context_start_test_suite (CutContext *context, CutTestSuite *test_suite)
{
    g_signal_connect(test_suite, "start",
                     G_CALLBACK(cb_start_test_suite), context);
    g_signal_connect(test_suite, "complete",
                     G_CALLBACK(cb_complete_test_suite), context);
    g_signal_connect(test_suite, "crashed",
                     G_CALLBACK(cb_crashed_test_suite), context);
}


guint
cut_context_get_n_tests (CutContext *context)
{
    return CUT_CONTEXT_GET_PRIVATE(context)->n_tests;
}

guint
cut_context_get_n_assertions (CutContext *context)
{
    return CUT_CONTEXT_GET_PRIVATE(context)->n_assertions;
}

guint
cut_context_get_n_failures (CutContext *context)
{
    return CUT_CONTEXT_GET_PRIVATE(context)->n_failures;
}

guint
cut_context_get_n_errors (CutContext *context)
{
    return CUT_CONTEXT_GET_PRIVATE(context)->n_errors;
}

guint
cut_context_get_n_pendings (CutContext *context)
{
    return CUT_CONTEXT_GET_PRIVATE(context)->n_pendings;
}

guint
cut_context_get_n_notifications (CutContext *context)
{
    return CUT_CONTEXT_GET_PRIVATE(context)->n_notifications;
}

const GList *
cut_context_get_results (CutContext *context)
{
    return CUT_CONTEXT_GET_PRIVATE(context)->results;
};

gboolean
cut_context_is_crashed (CutContext *context)
{
    return CUT_CONTEXT_GET_PRIVATE(context)->crashed;
};

const gchar *
cut_context_get_stack_trace (CutContext *context)
{
    return CUT_CONTEXT_GET_PRIVATE(context)->stack_trace;
};

/*
vi:ts=4:nowrap:ai:expandtab:sw=4
*/
