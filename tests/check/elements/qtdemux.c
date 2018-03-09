/* GStreamer
 *
 * unit test for qtdemux
 *
 * Copyright (C) <2016> Edward Hervey <edward@centricular.com>
 * Copyright (C) <2018> Igalia S.L. <aboya@igalia.com>
 * Copyright (C) <2018> Metrological
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "qtdemux.h"
#include "gst/gst.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct
{
  GstPad *srcpad;
  guint expected_size;
  GstClockTime expected_time;
} CommonTestData;

static GstPadProbeReturn
qtdemux_probe (GstPad * pad, GstPadProbeInfo * info, CommonTestData * data)
{
  GstBuffer *buf = GST_PAD_PROBE_INFO_BUFFER (info);

  fail_unless_equals_int (gst_buffer_get_size (buf), data->expected_size);
  fail_unless_equals_uint64 (GST_BUFFER_PTS (buf), data->expected_time);
  gst_buffer_unref (buf);
  return GST_PAD_PROBE_HANDLED;
}

static void
qtdemux_pad_added_cb (GstElement * element, GstPad * pad, CommonTestData * data)
{
  data->srcpad = pad;
  gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BUFFER,
      (GstPadProbeCallback) qtdemux_probe, data, NULL);
}

GST_START_TEST (test_qtdemux_input_gap)
{
  GstElement *qtdemux;
  GstPad *sinkpad;
  CommonTestData data = { 0, };
  GstBuffer *inbuf;
  GstSegment segment;
  GstEvent *event;
  guint i, offset;
  GstClockTime pts;

  /* The goal of this test is to check that qtdemux can properly handle
   * fragmented input from dashdemux, with gaps in it.
   *
   * Input segment :
   *   - TIME
   * Input buffers :
   *   - The offset is set on buffers, it corresponds to the offset
   *     within the current fragment.
   *   - Buffer of the beginning of a fragment has the PTS set, others
   *     don't.
   *   - By extension, the beginning of a fragment also has an offset
   *     of 0.
   */

  qtdemux = gst_element_factory_make ("qtdemux", NULL);
  gst_element_set_state (qtdemux, GST_STATE_PLAYING);
  sinkpad = gst_element_get_static_pad (qtdemux, "sink");

  /* We'll want to know when the source pad is added */
  g_signal_connect (qtdemux, "pad-added", (GCallback) qtdemux_pad_added_cb,
      &data);

  /* Send the initial STREAM_START and segment (TIME) event */
  event = gst_event_new_stream_start ("TEST");
  GST_DEBUG ("Pushing stream-start event");
  fail_unless (gst_pad_send_event (sinkpad, event) == TRUE);
  gst_segment_init (&segment, GST_FORMAT_TIME);
  event = gst_event_new_segment (&segment);
  GST_DEBUG ("Pushing segment event");
  fail_unless (gst_pad_send_event (sinkpad, event) == TRUE);

  /* Feed the init buffer, should create the source pad */
  inbuf = gst_buffer_new_and_alloc (init_mp4_len);
  gst_buffer_fill (inbuf, 0, init_mp4, init_mp4_len);
  GST_BUFFER_PTS (inbuf) = 0;
  GST_BUFFER_OFFSET (inbuf) = 0;
  GST_DEBUG ("Pushing header buffer");
  fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);

  /* Now send the trun of the first fragment */
  inbuf = gst_buffer_new_and_alloc (seg_1_moof_size);
  gst_buffer_fill (inbuf, 0, seg_1_m4f, seg_1_moof_size);
  GST_BUFFER_PTS (inbuf) = 0;
  GST_BUFFER_OFFSET (inbuf) = 0;
  /* We are simulating that this fragment can happen at any point */
  GST_BUFFER_FLAG_SET (inbuf, GST_BUFFER_FLAG_DISCONT);
  GST_DEBUG ("Pushing trun buffer");
  fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);
  fail_if (data.srcpad == NULL);

  /* We are now ready to send some buffers with gaps */
  offset = seg_1_sample_0_offset;
  pts = 0;

  GST_DEBUG ("Pushing gap'ed buffers");
  for (i = 0; i < 129; i++) {
    /* Let's send one every 3 */
    if ((i % 3) == 0) {
      GST_DEBUG ("Pushing buffer #%d offset:%" G_GUINT32_FORMAT, i, offset);
      inbuf = gst_buffer_new_and_alloc (seg_1_sample_sizes[i]);
      gst_buffer_fill (inbuf, 0, seg_1_m4f + offset, seg_1_sample_sizes[i]);
      GST_BUFFER_OFFSET (inbuf) = offset;
      GST_BUFFER_FLAG_SET (inbuf, GST_BUFFER_FLAG_DISCONT);
      data.expected_time =
          gst_util_uint64_scale (pts, GST_SECOND, seg_1_timescale);
      data.expected_size = seg_1_sample_sizes[i];
      fail_unless (gst_pad_chain (sinkpad, inbuf) == GST_FLOW_OK);
    }
    /* Finally move offset forward */
    offset += seg_1_sample_sizes[i];
    pts += seg_1_sample_duration;
  }

  gst_object_unref (sinkpad);
  gst_element_set_state (qtdemux, GST_STATE_NULL);
  gst_object_unref (qtdemux);
}

GST_END_TEST;

typedef struct _MovieTemplate
{
  GstMemory *mem;
  int free_offset;
  int free_size;
} MovieTemplate;

static MovieTemplate ibpibp_non_frag_template = { NULL, 0x00f8, 128 };
static MovieTemplate ibpibp_frag_template = { NULL, 0x00f8, 128 };

#define gst_object_unref_last(x) fail_unless_equals_int(GST_OBJECT_REFCOUNT_VALUE(x), 1); gst_object_unref(x)

typedef struct _EditListEntry
{
  guint64 segment_duration;
  gint64 media_time;
  gint16 media_rate_integer;
  gint16 media_rate_fraction;
} EditListEntry;

typedef enum _ElstVersion
{
  ELST_VERSION_0 = 0,
  ELST_VERSION_1 = 1
} ElstVersion;

#define MAX_EDIT_LIST_ENTRY_COUNT 5

typedef struct _EditListBuilder
{
  ElstVersion elst_version;
  guint entry_count;
  EditListEntry entries[MAX_EDIT_LIST_ENTRY_COUNT];
} EditListBuilder;

static void
edit_list_builder_init (EditListBuilder * builder, ElstVersion elst_version)
{
  builder->entry_count = 0;
  builder->elst_version = elst_version;
}

static void
edit_list_builder_add_entry (EditListBuilder * builder,
    guint64 segment_duration, guint64 media_time, guint16 media_rate_integer,
    guint16 media_rate_fraction)
{
  g_assert (builder->entry_count < MAX_EDIT_LIST_ENTRY_COUNT);

  builder->entries[builder->entry_count].segment_duration = segment_duration;
  builder->entries[builder->entry_count].media_time = media_time;
  builder->entries[builder->entry_count].media_rate_integer =
      media_rate_integer;
  builder->entries[builder->entry_count].media_rate_fraction =
      media_rate_fraction;

  builder->entry_count++;
}

static GstMemory *
edit_list_builder_build_memory (const EditListBuilder * builder,
    int template_free_size)
{
  GstMemory *mem;
  GstMapInfo info;
  int ret_map;
  int i;
  int offset;

  mem = gst_allocator_alloc (NULL, template_free_size, NULL);
  ret_map = gst_memory_map (mem, &info, GST_MAP_WRITE);
  g_assert (ret_map);

  /* poison, to ease debugging of unset memory */
  memset (info.data, 0xaa, info.size);

  /* edts and elst size will be filled later */
  memcpy (info.data, "\0\0\0\0edts\0\0\0\0elst", 16);
  GST_WRITE_UINT32_BE (info.data + 16, builder->elst_version);
  GST_WRITE_UINT32_BE (info.data + 20, builder->entry_count);
  offset = 24;
  for (i = 0; i < builder->entry_count; i++) {
    if (builder->elst_version == ELST_VERSION_1) {
      GST_WRITE_UINT64_BE (info.data + offset,
          (guint64) builder->entries[i].segment_duration);
      GST_WRITE_UINT64_BE (info.data + offset + 8,
          (guint64) builder->entries[i].media_time);
      offset += 16;
    } else {
      GST_WRITE_UINT32_BE (info.data + offset,
          builder->entries[i].segment_duration);
      GST_WRITE_UINT32_BE (info.data + offset + 4,
          builder->entries[i].media_time);
      offset += 8;
    }
    GST_WRITE_UINT16_BE (info.data + offset,
        builder->entries[i].media_rate_integer);
    GST_WRITE_UINT16_BE (info.data + offset + 2,
        builder->entries[i].media_rate_fraction);
    offset += 4;
  }
  GST_WRITE_UINT32_BE (info.data + 0, offset);  /* edts size */
  GST_WRITE_UINT32_BE (info.data + 8, offset - 8);      /* elts size */

  /* Write padding free box */
  g_assert (template_free_size - offset >= 8);
  GST_WRITE_UINT32_BE (info.data + offset, template_free_size - offset);        /* free size */
  memcpy (info.data + offset + 4, "free", 4);
  /* Fill content with zeroes */
  offset += 8;
  memset (info.data + offset, '\x00', template_free_size - offset);

  gst_memory_unmap (mem, &info);
  return mem;
}


typedef enum _EventType
{
  EVENT_SEGMENT,
  EVENT_BUFFER
} EventType;

typedef struct _SegmentEventContent
{
  GstSegment segment;
} SegmentEventContent;

typedef struct _BufferEventContent
{
  guint64 buffer_pts;
  gint64 stream_pts;
  guint64 duration;
} BufferEventContent;

typedef struct _Event
{
  EventType type;
  union
  {
    struct _SegmentEventContent segment;
    struct _BufferEventContent buffer;
  } content;
} Event;

#define MAX_EVENT_COUNT 20

typedef struct _EventList
{
  int entry_count;
  Event events[MAX_EVENT_COUNT];
} EventList;

static void
event_list_init (EventList * list)
{
  list->entry_count = 0;
}

static void
event_list_add_buffer (EventList * list, gint64 stream_pts, guint64 buffer_pts,
    guint64 duration)
{
  g_assert (list->entry_count < MAX_EVENT_COUNT);
  Event *event = &list->events[list->entry_count++];

  event->type = EVENT_BUFFER;
  event->content.buffer.buffer_pts = buffer_pts;
  event->content.buffer.stream_pts = stream_pts;
  event->content.buffer.duration = duration;
}

static void
event_list_add_segment (EventList * list, GstSegment * segment)
{
  g_assert (list->entry_count < MAX_EVENT_COUNT);
  Event *event = &list->events[list->entry_count++];

  event->type = EVENT_SEGMENT;
  event->content.segment.segment = *segment;
}

static GString *
event_list_to_string (const EventList * list, int mismatch_index)
{
  GString *string;
  int i;

  string = g_string_new ("");
  for (i = 0; i < list->entry_count; i++) {
    g_string_append (string, i == mismatch_index ? "> " : "  ");

    if (list->events[i].type == EVENT_BUFFER) {
      const BufferEventContent *content = &list->events[i].content.buffer;
      g_string_append_printf (string, "buffer: stream_pts=%" GST_STIME_FORMAT
          " buffer_pts=%" GST_TIME_FORMAT " duration=%" GST_TIME_FORMAT,
          GST_STIME_ARGS (content->stream_pts),
          GST_TIME_ARGS (content->buffer_pts),
          GST_TIME_ARGS (content->duration));
    } else if (list->events[i].type == EVENT_SEGMENT) {
      const SegmentEventContent *content = &list->events[i].content.segment;
      g_string_append_printf (string, "segment: rate=%g"
          " applied_rate=%g"
          " base=%" GST_TIME_FORMAT
          " offset=%" GST_TIME_FORMAT
          " start=%" GST_TIME_FORMAT
          " stop=%" GST_TIME_FORMAT
          " time=%" GST_TIME_FORMAT,
          content->segment.rate,
          content->segment.applied_rate,
          GST_TIME_ARGS (content->segment.base),
          GST_TIME_ARGS (content->segment.offset),
          GST_TIME_ARGS (content->segment.start),
          GST_TIME_ARGS (content->segment.stop),
          GST_TIME_ARGS (content->segment.time));
    } else {
      g_assert_not_reached ();
    }

    g_string_append (string, i == mismatch_index ? " <<" : "");
    g_string_append (string, "\n");
  }

  if (list->entry_count == 0) {
    g_string_append (string, "<no buffers or segments>\n");
  }

  return string;
}

static gboolean
event_list_equals (const EventList * list, const EventList * other,
    int *dst_mismatch_index)
{
  int i;

  for (i = 0; i < MIN (list->entry_count, other->entry_count); i++) {
    if (list->events[i].type != other->events[i].type)
      goto mismatch;

    if (list->events[i].type == EVENT_BUFFER) {
      const BufferEventContent *content = &list->events[i].content.buffer;
      const BufferEventContent *content_other =
          &other->events[i].content.buffer;

      if (content->buffer_pts != content_other->buffer_pts)
        goto mismatch;
      if (content->stream_pts != content_other->stream_pts)
        goto mismatch;
      if (content->duration != content_other->duration)
        goto mismatch;
    } else if (list->events[i].type == EVENT_SEGMENT) {
      const SegmentEventContent *content = &list->events[i].content.segment;
      const SegmentEventContent *content_other =
          &other->events[i].content.segment;

      if (fabs (content->segment.rate - content_other->segment.rate) >=
          0.0000001)
        goto mismatch;
      if (fabs (content->segment.applied_rate -
              content_other->segment.applied_rate) >= 0.0000001)
        goto mismatch;
      if (content->segment.base != content_other->segment.base)
        goto mismatch;
      if (content->segment.offset != content_other->segment.offset)
        goto mismatch;
      if (content->segment.start != content_other->segment.start)
        goto mismatch;
      if (content->segment.stop != content_other->segment.stop)
        goto mismatch;
      if (content->segment.time != content_other->segment.time)
        goto mismatch;
    } else {
      g_assert_not_reached ();
    }
  }

  if (list->entry_count != other->entry_count)
    goto mismatch;

  return TRUE;

mismatch:
  *dst_mismatch_index = i;
  return FALSE;
}

static gint64
buffer_time_to_stream_time (const GstSegment * segment, guint64 buffer_time)
{
  g_assert (buffer_time != GST_CLOCK_TIME_NONE);
  guint64 stream_time_unsigned;
  int stream_time_sign =
      gst_segment_to_stream_time_full (segment, GST_FORMAT_TIME, buffer_time,
      &stream_time_unsigned);
  g_assert (stream_time_sign != 0);
  return stream_time_sign * ((gint64) stream_time_unsigned);
}

typedef struct _EventWriter
{
  EventList *event_list;
  GstSegment latest_segment;
  gboolean latest_segment_set;
} EventWriter;

static void
event_writer_init (EventWriter * writer, EventList * event_list)
{
  writer->event_list = event_list;
  writer->latest_segment_set = FALSE;
}

static GstPadProbeReturn
event_writer_probe (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  EventWriter *writer = (EventWriter *) user_data;

  if (info->type & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM
      && ((GstEvent *) info->data)->type == GST_EVENT_SEGMENT) {
    GstEvent *event;

    event = (GstEvent *) info->data;
    gst_event_copy_segment (event, &writer->latest_segment);
    event_list_add_segment (writer->event_list, &writer->latest_segment);
    writer->latest_segment_set = TRUE;
  } else if (info->type & GST_PAD_PROBE_TYPE_BUFFER) {
    GstBuffer *buffer;

    buffer = (GstBuffer *) info->data;
    g_assert (writer->latest_segment_set);
    event_list_add_buffer (writer->event_list,
        buffer_time_to_stream_time (&writer->latest_segment, buffer->pts),
        buffer->pts, buffer->duration);
  }

  return GST_PAD_PROBE_OK;
}

static void
fail_if_events_not_equal (const EventList * actual, const EventList * expected)
{
  gboolean event_lists_are_equal;
  int mismatched_index;

  event_lists_are_equal =
      event_list_equals (actual, expected, &mismatched_index);
  if (!event_lists_are_equal) {
    GString *expected_str = event_list_to_string (expected, mismatched_index);
    GString *actual_str = event_list_to_string (actual, mismatched_index);
    ck_assert_msg (event_lists_are_equal,
        "\nExpected events:\n%sActual events:\n%sMismatch starts at event with index=%d (marked)",
        expected_str->str, actual_str->str, mismatched_index);
  }
}

static GstBuffer *
create_test_vector_from_template (EditListBuilder * edit_list_builder,
    MovieTemplate * template)
{
  GstBuffer *buffer;
  buffer = gst_buffer_new ();
  gst_buffer_append_memory (buffer, gst_memory_share (template->mem, 0,
          template->free_offset));
  gst_buffer_append_memory (buffer,
      edit_list_builder_build_memory (edit_list_builder, template->free_size));
  gst_buffer_append_memory (buffer, gst_memory_share (template->mem,
          template->free_offset + template->free_size,
          template->mem->size - (template->free_offset + template->free_size)));
  return buffer;
}

static GstMemory *
read_file_to_memory (const gchar * test_filename)
{
  gchar *full_filename;
  gchar *data;
  gsize length;
  GError *error = NULL;

  full_filename = g_build_filename (GST_TEST_FILES_PATH, test_filename, NULL);
  g_file_get_contents (full_filename, &data, &length, &error);
  g_assert_no_error (error);
  g_free (full_filename);

  return gst_memory_new_wrapped (GST_MEMORY_FLAG_READONLY, data, length, 0,
      length, data, g_free);
}

static char *
write_temp_file (GstBuffer * movie_buffer)
{
  gchar *file_path;
  GError *error = NULL;
  gint fd;
  int i;

  fd = g_file_open_tmp ("qtdemux-test-XXXXXX", &file_path, &error);
  g_assert_no_error (error);
  /* Hint: alternatively, uncomment the following for easier debugging,
   * at the cost of not being able to run two instances of the tests in
   * parallel: */
  /* file_path = g_strdup("/tmp/qtdemux-test.mp4"); */
  /* fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644); */
  /* g_assert(fd != -1); */

  for (i = 0; i < gst_buffer_n_memory (movie_buffer); i++) {
    GstMemory *mem;
    GstMapInfo info;
    mem = gst_buffer_get_memory (movie_buffer, i);
    gst_memory_map (mem, &info, GST_MAP_READ);
    write (fd, info.data, info.size);
  }

  close (fd);
  return file_path;
}

typedef enum _TestSchedulingMode
{
  TEST_SCHEDULING_PUSH_NO_SEEK,
  TEST_SCHEDULING_PUSH,
  TEST_SCHEDULING_PULL
} TestSchedulingMode;

static GstPadProbeReturn
no_seek_probe (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  if (info->type & GST_PAD_PROBE_TYPE_QUERY_UPSTREAM
      && ((GstQuery *) info->data)->type == GST_QUERY_SEEKING) {
    return GST_PAD_PROBE_DROP;
  } else if (info->type & GST_PAD_PROBE_TYPE_EVENT_UPSTREAM
      && ((GstEvent *) info->data)->type == GST_EVENT_SEEK) {
    g_assert_not_reached ();
  } else {
    return GST_PAD_PROBE_OK;
  }
}

static void
test_qtdemux_expected_events (TestSchedulingMode scheduling_mode,
    GstBuffer * movie_buffer, EventList * expected_events)
{
  GstElement *pipeline;
  GstElement *src;
  GstElement *fakesink;
  GstBus *bus;
  GstMessage *bus_message;
  GstPad *fakesink_pad;
  GstPad *src_pad;
  GError *error = NULL;
  EventList actual_events;
  EventWriter event_writer;
  GstStateChangeReturn state_change_ret;
  gchar *movie_buffer_file_path;

  event_list_init (&actual_events);
  event_writer_init (&event_writer, &actual_events);
  movie_buffer_file_path = write_temp_file (movie_buffer);

  if (scheduling_mode == TEST_SCHEDULING_PULL)
    pipeline =
        gst_parse_launch
        ("filesrc name=src ! qtdemux ! fakesink name=fakesink sync=false async=false",
        &error);
  else
    pipeline =
        gst_parse_launch
        ("pushfilesrc name=src ! qtdemux ! fakesink name=fakesink sync=false async=false",
        &error);
  g_assert_no_error (error);

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN (pipeline), GST_DEBUG_GRAPH_SHOW_ALL,
      "qtdemux-edit-list");

  src = gst_bin_get_by_name (GST_BIN (pipeline), "src");
  g_assert (src);
  g_object_set (src, "location", movie_buffer_file_path, NULL);

  src_pad = gst_element_get_static_pad (src, "src");
  g_assert (src_pad);
  if (scheduling_mode == TEST_SCHEDULING_PUSH_NO_SEEK)
    gst_pad_add_probe (src_pad,
        GST_PAD_PROBE_TYPE_EVENT_UPSTREAM | GST_PAD_PROBE_TYPE_QUERY_UPSTREAM,
        no_seek_probe, NULL, NULL);

  fakesink = gst_bin_get_by_name (GST_BIN (pipeline), "fakesink");
  g_assert (fakesink);
  fakesink_pad = gst_element_get_static_pad (fakesink, "sink");
  g_assert (fakesink_pad);
  gst_pad_add_probe (fakesink_pad,
      GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
      event_writer_probe, &event_writer, NULL);

  state_change_ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
  fail_unless_equals_int (state_change_ret, GST_STATE_CHANGE_SUCCESS);

  /* Wait for demuxing to end */
  bus_message =
      gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
      GST_MESSAGE_EOS | GST_MESSAGE_ERROR);
  g_assert (bus_message);
  if (bus_message->type == GST_MESSAGE_ERROR) {
    GError *err;
    gchar *dbg;

    gst_message_parse_error (bus_message, &err, &dbg);
    g_printerr ("ERROR from element %s: %s\n%s\n",
        GST_OBJECT_NAME (bus_message->src), err->message, GST_STR_NULL (dbg));
    g_error_free (err);
    g_free (dbg);
  }
  g_assert (bus_message->type == GST_MESSAGE_EOS);

  state_change_ret = gst_element_set_state (pipeline, GST_STATE_NULL);
  fail_unless_equals_int (state_change_ret, GST_STATE_CHANGE_SUCCESS);

  fail_if_events_not_equal (&actual_events, expected_events);

  /* Delete temporary movie file, we no longer need it.
   * The file is kept if the test fails for easier debugging. */
  g_unlink (movie_buffer_file_path);

  gst_message_unref (bus_message);
  gst_object_unref (fakesink_pad);
  gst_object_unref (fakesink);
  gst_object_unref (src_pad);
  gst_object_unref (src);
  gst_object_unref (bus);
  gst_object_unref_last (pipeline);
  g_free (movie_buffer_file_path);
}

static void
load_template_files ()
{
  ibpibp_non_frag_template.mem =
      read_file_to_memory ("ibpibp-non-frag-template.mp4");
  ibpibp_frag_template.mem = read_file_to_memory ("ibpibp-frag-template.mp4");
}

GstSegment *
typical_segment (GstSegment * segment, guint64 start, guint64 duration)
{
  g_assert (start != GST_CLOCK_TIME_NONE);

  guint64 prev_segment_stream_duration = 0;
  guint64 prev_segment_running_duration = 0;

  if (segment->stop != GST_CLOCK_TIME_NONE) {
    prev_segment_stream_duration =
        segment->applied_rate * (segment->stop - segment->start);
    prev_segment_running_duration =
        (segment->stop - segment->start) / segment->rate;
  }

  segment->time += prev_segment_stream_duration;
  segment->base += prev_segment_running_duration;

  segment->applied_rate = 1;
  segment->rate = 1;
  segment->start = start;
  segment->stop =
      (duration ==
      GST_CLOCK_TIME_NONE ? GST_CLOCK_TIME_NONE : start + duration);
  segment->duration = duration;
  segment->offset = 0;
  return segment;
}

/* Used only for EXPECT_EMPTY_EDIT_SEGMENTS workaround.
 * Returned value is invalidated by successive calls. */
GstSegment *
empty_segment (guint64 start, guint64 duration)
{
  static GstSegment segment;
  gst_segment_init (&segment, GST_FORMAT_TIME);
  segment.time = start;
  segment.base = start;
  segment.start = start;
  segment.stop = start + duration;
  return &segment;
}

static GstSegment *
default_time_segment ()
{
  static GstSegment segment;
  static gboolean initialized = FALSE;

  if (!initialized) {
    gst_segment_init (&segment, GST_FORMAT_TIME);
  }
  return &segment;
}

/* Edit list template files have:
 *   Frame rate       = 1/3
 *   Movie timescale  = 1/30
 *   Track timescale  = 1/300
 *   Duration         = 2 s
 *   GOP layout       = I-B-P-I-B-P
 */

/* Multiple tests are generated by combining the following 3 variables:
 *
 * Fragmentation:
 *  - frag
 *  - no_frag
 *
 * Scheduling mode:
 *  - pull
 *  - push
 *  - push_no_seek
 *
 * Edit list:
 *  - no_edts
 *  - basic
 *  - basic_zero_dur
 *  - skipping
 *  - skipping_non_rap
 *  - empty_edit_start
 *  - empty_edit_middle
 *  - reorder
 *  - repeating
 */

/* Workarounds for gst behaviors that don't make sense but are not severe enough to consider a test as a failure:
 * If any of these behaviors is fixed, update the constant before running the tests. */

/* In push mode a dummy default segment is sent before the first, useful one. */
static const gboolean EXPECT_BUGGY_DUMMY_SEGMENT = TRUE;

/* An extra frame that is out-of-segment is sent after an edit. */
static const gboolean EXPECT_SPURIOUS_EXTRA_FRAME = TRUE;

/* Empty edits generate unnecessary GstSegments. Their start and stop properties refer to stream time, not buffer time. */
static const gboolean EXPECT_EMPTY_EDIT_SEGMENTS = TRUE;

void
test_qtdemux_edit_lists_no_edts (TestSchedulingMode scheduling_mode,
    MovieTemplate * movie_template)
{
  GstBuffer *movie_buffer;
  EventList expected_events;
  GstSegment segment;

  /* Use the template in raw form, without filling a edts element. */
  movie_buffer = gst_buffer_new ();
  gst_buffer_append_memory (movie_buffer, movie_template->mem);

  gst_segment_init (&segment, GST_FORMAT_TIME);
  event_list_init (&expected_events);
  if (scheduling_mode != TEST_SCHEDULING_PULL && EXPECT_BUGGY_DUMMY_SEGMENT)
    event_list_add_segment (&expected_events, default_time_segment ());

  /* Bug workaround: the end of the segment should actually be 2333333333.
   * When there is no edit list, qtdemux takes 2 seconds from the mhvd duration
   * field, but that field is not reliable: it is optional and the spec does
   * not specify whether that duration includes the first few moments in track
   * time which have no presentation. Instead, it would be more reliable to
   * compute the duration from the sample index, as ffmpeg does.
   * The generated test cases don't update this field. */
  event_list_add_segment (&expected_events, typical_segment (&segment, 0,
          2000000000));

  /* *INDENT-OFF* */
  event_list_add_buffer (&expected_events,  333333333,  333333333, 333333333);
  event_list_add_buffer (&expected_events, 1000000000, 1000000000, 333333333);
  event_list_add_buffer (&expected_events,  666666666,  666666666, 333333334);
  event_list_add_buffer (&expected_events, 1333333333, 1333333333, 333333333);
  event_list_add_buffer (&expected_events, 2000000000, 2000000000, 333333333);
  event_list_add_buffer (&expected_events, 1666666666, 1666666666, 333333334);

  /* *INDENT-ON* */
  test_qtdemux_expected_events (scheduling_mode, movie_buffer,
      &expected_events);
  gst_buffer_unref (movie_buffer);
}

void
test_qtdemux_edit_lists_basic (TestSchedulingMode scheduling_mode,
    MovieTemplate * movie_template)
{
  EditListBuilder edit_list;
  GstBuffer *movie_buffer;
  EventList expected_events;
  GstSegment segment;

  edit_list_builder_init (&edit_list, ELST_VERSION_0);
  edit_list_builder_add_entry (&edit_list, 60, 100, 1, 0);

  movie_buffer = create_test_vector_from_template (&edit_list, movie_template);

  gst_segment_init (&segment, GST_FORMAT_TIME);
  event_list_init (&expected_events);
  if (scheduling_mode != TEST_SCHEDULING_PULL && EXPECT_BUGGY_DUMMY_SEGMENT)
    event_list_add_segment (&expected_events, default_time_segment ());

  /* *INDENT-OFF* */
  event_list_add_segment (&expected_events, typical_segment (&segment, 333333333, 2 * GST_SECOND));
  event_list_add_buffer (&expected_events,          0,  333333333, 333333333);
  event_list_add_buffer (&expected_events,  666666667, 1000000000, 333333333);
  event_list_add_buffer (&expected_events,  333333333,  666666666, 333333334);
  event_list_add_buffer (&expected_events, 1000000000, 1333333333, 333333333);
  event_list_add_buffer (&expected_events, 1666666667, 2000000000, 333333333);
  event_list_add_buffer (&expected_events, 1333333333, 1666666666, 333333334);

  /* *INDENT-ON* */
  test_qtdemux_expected_events (scheduling_mode, movie_buffer,
      &expected_events);
  gst_buffer_unref (movie_buffer);
}

void
test_qtdemux_edit_lists_basic_zero_dur (TestSchedulingMode scheduling_mode,
    MovieTemplate * movie_template)
{
  EditListBuilder edit_list;
  GstBuffer *movie_buffer;
  EventList expected_events;
  GstSegment segment;

  edit_list_builder_init (&edit_list, ELST_VERSION_0);
  /* duration=0 must be interpreted as "until the end of the track".
   * This test case must produce the same frames as test_qtdemux_edit_lists_basic. */
  edit_list_builder_add_entry (&edit_list, 0, 100, 1, 0);

  movie_buffer = create_test_vector_from_template (&edit_list, movie_template);

  gst_segment_init (&segment, GST_FORMAT_TIME);
  event_list_init (&expected_events);
  if (scheduling_mode != TEST_SCHEDULING_PULL && EXPECT_BUGGY_DUMMY_SEGMENT)
    event_list_add_segment (&expected_events, default_time_segment ());

  event_list_add_segment (&expected_events, typical_segment (&segment,
          333333333, 2 * GST_SECOND));

  /* *INDENT-OFF* */
  event_list_add_buffer (&expected_events,          0,  333333333, 333333333);
  event_list_add_buffer (&expected_events,  666666667, 1000000000, 333333333);
  event_list_add_buffer (&expected_events,  333333333,  666666666, 333333334);
  event_list_add_buffer (&expected_events, 1000000000, 1333333333, 333333333);
  event_list_add_buffer (&expected_events, 1666666667, 2000000000, 333333333);
  event_list_add_buffer (&expected_events, 1333333333, 1666666666, 333333334);

  /* *INDENT-ON* */
  test_qtdemux_expected_events (scheduling_mode, movie_buffer,
      &expected_events);
  gst_buffer_unref (movie_buffer);
}

void
test_qtdemux_edit_lists_skipping (TestSchedulingMode scheduling_mode,
    MovieTemplate * movie_template)
{
  EditListBuilder edit_list;
  GstBuffer *movie_buffer;
  EventList expected_events;
  GstSegment segment;

  edit_list_builder_init (&edit_list, ELST_VERSION_0);
  edit_list_builder_add_entry (&edit_list, 10, 100, 1, 0);      /* 1 I-frame */
  edit_list_builder_add_entry (&edit_list, 20, 400, 1, 0);      /* 2 frames (I-B) with 1 P-frame dep */

  movie_buffer = create_test_vector_from_template (&edit_list, movie_template);

  gst_segment_init (&segment, GST_FORMAT_TIME);
  event_list_init (&expected_events);
  if (scheduling_mode != TEST_SCHEDULING_PULL && EXPECT_BUGGY_DUMMY_SEGMENT)
    event_list_add_segment (&expected_events, default_time_segment ());

  /* *INDENT-OFF* */
  event_list_add_segment (&expected_events, typical_segment (&segment, 333333333, 333333333));
  event_list_add_buffer (&expected_events,          0,  333333333, 333333333);
  if (EXPECT_SPURIOUS_EXTRA_FRAME)
    event_list_add_buffer (&expected_events,  666666667, 1000000000, 333333333);

  event_list_add_segment (&expected_events, typical_segment (&segment, 1333333333, 666666667));
  event_list_add_buffer (&expected_events,  333333333, 1333333333, 333333333);
  event_list_add_buffer (&expected_events, 1000000000, 2000000000, 333333333);
  event_list_add_buffer (&expected_events,  666666666, 1666666666, 333333334);

  /* *INDENT-ON* */
  test_qtdemux_expected_events (scheduling_mode, movie_buffer,
      &expected_events);
  gst_buffer_unref (movie_buffer);
}

void
test_qtdemux_edit_lists_skipping_non_rap (TestSchedulingMode scheduling_mode,
    MovieTemplate * movie_template)
{
  EditListBuilder edit_list;
  GstBuffer *movie_buffer;
  EventList expected_events;
  GstSegment segment;

  edit_list_builder_init (&edit_list, ELST_VERSION_0);
  edit_list_builder_add_entry (&edit_list, 10, 100, 1, 0);      /* 1 I-frame */
  edit_list_builder_add_entry (&edit_list, 10, 600, 1, 0);      /* 1 P-frame with 1 decode dependency */

  movie_buffer = create_test_vector_from_template (&edit_list, movie_template);

  gst_segment_init (&segment, GST_FORMAT_TIME);
  event_list_init (&expected_events);
  if (scheduling_mode != TEST_SCHEDULING_PULL && EXPECT_BUGGY_DUMMY_SEGMENT)
    event_list_add_segment (&expected_events, default_time_segment ());

  event_list_add_segment (&expected_events, typical_segment (&segment,
          333333333, 333333333));

  /* *INDENT-OFF* */
  event_list_add_buffer (&expected_events,          0,  333333333, 333333333);
  if (EXPECT_SPURIOUS_EXTRA_FRAME)
    event_list_add_buffer (&expected_events,  666666667, 1000000000, 333333333);
  event_list_add_segment (&expected_events, typical_segment (&segment, 2000000000, 333333333));
  event_list_add_buffer (&expected_events, -333333334, 1333333333, 333333333);
  event_list_add_buffer (&expected_events,  333333333, 2000000000, 333333333);
  if (EXPECT_SPURIOUS_EXTRA_FRAME)
    event_list_add_buffer (&expected_events,         -1, 1666666666, 333333334);

  /* *INDENT-ON* */
  test_qtdemux_expected_events (scheduling_mode, movie_buffer,
      &expected_events);
  gst_buffer_unref (movie_buffer);
}

void
test_qtdemux_edit_lists_empty_edit_start (TestSchedulingMode scheduling_mode,
    MovieTemplate * movie_template)
{
  EditListBuilder edit_list;
  GstBuffer *movie_buffer;
  EventList expected_events;
  GstSegment segment;

  edit_list_builder_init (&edit_list, ELST_VERSION_0);
  edit_list_builder_add_entry (&edit_list, 30, -1, 1, 0);       /* 1 empty second */
  edit_list_builder_add_entry (&edit_list, 10, 400, 1, 0);      /* 1 I-frame */

  movie_buffer = create_test_vector_from_template (&edit_list, movie_template);

  gst_segment_init (&segment, GST_FORMAT_TIME);
  event_list_init (&expected_events);
  if (scheduling_mode != TEST_SCHEDULING_PULL && EXPECT_BUGGY_DUMMY_SEGMENT)
    event_list_add_segment (&expected_events, default_time_segment ());

  if (EXPECT_EMPTY_EDIT_SEGMENTS)
    event_list_add_segment (&expected_events, empty_segment (0, 1000000000));
  segment.time = segment.base = 1000000000;
  event_list_add_segment (&expected_events, typical_segment (&segment,
          1333333333, 333333333));
  event_list_add_buffer (&expected_events, 1000000000, 1333333333, 333333333);
  if (EXPECT_SPURIOUS_EXTRA_FRAME)
    event_list_add_buffer (&expected_events, 1666666667, 2000000000, 333333333);

  test_qtdemux_expected_events (scheduling_mode, movie_buffer,
      &expected_events);
  gst_buffer_unref (movie_buffer);
}

void
test_qtdemux_edit_lists_empty_edit_middle (TestSchedulingMode scheduling_mode,
    MovieTemplate * movie_template)
{
  EditListBuilder edit_list;
  GstBuffer *movie_buffer;
  EventList expected_events;
  GstSegment segment;

  edit_list_builder_init (&edit_list, ELST_VERSION_0);
  edit_list_builder_add_entry (&edit_list, 10, 100, 1, 0);      /* 1 I-frame */
  edit_list_builder_add_entry (&edit_list, 20, -1, 1, 0);       /* 2/3 s empty */
  edit_list_builder_add_entry (&edit_list, 10, 400, 1, 0);      /* 1 I-frame */

  movie_buffer = create_test_vector_from_template (&edit_list, movie_template);

  gst_segment_init (&segment, GST_FORMAT_TIME);
  event_list_init (&expected_events);
  if (scheduling_mode != TEST_SCHEDULING_PULL && EXPECT_BUGGY_DUMMY_SEGMENT)
    event_list_add_segment (&expected_events, default_time_segment ());

  event_list_add_segment (&expected_events, typical_segment (&segment,
          333333333, 333333333));
  event_list_add_buffer (&expected_events, 0, 333333333, 333333333);
  if (EXPECT_SPURIOUS_EXTRA_FRAME)
    event_list_add_buffer (&expected_events, 666666667, 1000000000, 333333333);

  if (EXPECT_EMPTY_EDIT_SEGMENTS)
    event_list_add_segment (&expected_events, empty_segment (333333333,
            666666667));

  gst_segment_init (&segment, GST_FORMAT_TIME);
  segment.time = segment.base = 1000000000;
  event_list_add_segment (&expected_events, typical_segment (&segment,
          1333333333, 333333333));
  event_list_add_buffer (&expected_events, 1000000000, 1333333333, 333333333);
  if (EXPECT_SPURIOUS_EXTRA_FRAME)
    event_list_add_buffer (&expected_events, 1666666667, 2000000000, 333333333);

  test_qtdemux_expected_events (scheduling_mode, movie_buffer,
      &expected_events);
  gst_buffer_unref (movie_buffer);
}

void
test_qtdemux_edit_lists_reorder (TestSchedulingMode scheduling_mode,
    MovieTemplate * movie_template)
{
  EditListBuilder edit_list;
  GstBuffer *movie_buffer;
  EventList expected_events;
  GstSegment segment;

  edit_list_builder_init (&edit_list, ELST_VERSION_0);
  edit_list_builder_add_entry (&edit_list, 30, 400, 1, 0);      /* 3 frames */
  edit_list_builder_add_entry (&edit_list, 30, 100, 1, 0);      /* 3 frames, from the past */

  movie_buffer = create_test_vector_from_template (&edit_list, movie_template);

  gst_segment_init (&segment, GST_FORMAT_TIME);
  event_list_init (&expected_events);
  if (scheduling_mode != TEST_SCHEDULING_PULL && EXPECT_BUGGY_DUMMY_SEGMENT)
    event_list_add_segment (&expected_events, default_time_segment ());

  event_list_add_segment (&expected_events, typical_segment (&segment,
          1333333333, 1000000000));

  /* *INDENT-OFF* */
  event_list_add_buffer (&expected_events,          0, 1333333333, 333333333);
  event_list_add_buffer (&expected_events,  666666667, 2000000000, 333333333);
  event_list_add_buffer (&expected_events,  333333333, 1666666666, 333333334);

  event_list_add_segment (&expected_events, typical_segment (&segment, 333333333, 1000000000));
  event_list_add_buffer (&expected_events, 1000000000,  333333333, 333333333);
  event_list_add_buffer (&expected_events, 1666666667, 1000000000, 333333333);
  event_list_add_buffer (&expected_events, 1333333333,  666666666, 333333334);
  if (EXPECT_SPURIOUS_EXTRA_FRAME)
    event_list_add_buffer (&expected_events, 2000000000, 1333333333, 333333333);

  /* *INDENT-ON* */
  test_qtdemux_expected_events (scheduling_mode, movie_buffer,
      &expected_events);
  gst_buffer_unref (movie_buffer);
}

void
test_qtdemux_edit_lists_repeating (TestSchedulingMode scheduling_mode,
    MovieTemplate * movie_template)
{
  EditListBuilder edit_list;
  GstBuffer *movie_buffer;
  EventList expected_events;
  GstSegment segment;

  edit_list_builder_init (&edit_list, ELST_VERSION_0);
  edit_list_builder_add_entry (&edit_list, 30, 400, 1, 0);      /* 3 frames */
  edit_list_builder_add_entry (&edit_list, 30, 400, 1, 0);      /* the same 3 frames again */

  movie_buffer = create_test_vector_from_template (&edit_list, movie_template);

  gst_segment_init (&segment, GST_FORMAT_TIME);
  event_list_init (&expected_events);
  if (scheduling_mode != TEST_SCHEDULING_PULL && EXPECT_BUGGY_DUMMY_SEGMENT)
    event_list_add_segment (&expected_events, default_time_segment ());

  /* *INDENT-OFF* */
  event_list_add_segment (&expected_events, typical_segment (&segment, 1333333333, 1000000000));
  event_list_add_buffer (&expected_events,          0, 1333333333, 333333333);
  event_list_add_buffer (&expected_events,  666666667, 2000000000, 333333333);
  event_list_add_buffer (&expected_events,  333333333, 1666666666, 333333334);

  event_list_add_segment (&expected_events, typical_segment (&segment, 1333333333, 1000000000));
  event_list_add_buffer (&expected_events, 1000000000, 1333333333, 333333333);
  event_list_add_buffer (&expected_events, 1666666667, 2000000000, 333333333);
  event_list_add_buffer (&expected_events, 1333333333, 1666666666, 333333334);

  /* *INDENT-ON* */
  test_qtdemux_expected_events (scheduling_mode, movie_buffer,
      &expected_events);
  gst_buffer_unref (movie_buffer);
}


#include "qtdemux-edit-list-generated/implementations.gen.cpp"

/** Use this macro instead of just commenting out failing tests in order to avoid compiler warnings */
#define tcase_skip_failing_test(tc_chain, test) (void)tc_chain; (void) test

static Suite *
qtdemux_suite (void)
{
  Suite *s = suite_create ("qtdemux");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_qtdemux_input_gap);

  load_template_files ();
  tc_chain = tcase_create ("editlists");
  suite_add_tcase (s, tc_chain);

#include "qtdemux-edit-list-generated/calls.gen.cpp"

  return s;
}

GST_CHECK_MAIN (qtdemux)
