/*
 * BEGIN AUTO-GENERATED EDIT LIST TEST IMPLEMENTATIONS
 *
 * Don't edit manually! Edit and run `edit-list-test-gen.js` to generate more
 * of these functions.
 */

GST_START_TEST (test_qtdemux_edit_lists_non_frag_pull_no_edts)
{
  test_qtdemux_edit_lists_no_edts (TEST_SCHEDULING_PULL, &ibpibp_non_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_non_frag_pull_basic)
{
  test_qtdemux_edit_lists_basic (TEST_SCHEDULING_PULL, &ibpibp_non_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_non_frag_pull_basic_zero_dur)
{
  test_qtdemux_edit_lists_basic_zero_dur (TEST_SCHEDULING_PULL, &ibpibp_non_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_non_frag_pull_basic_empty_edit_start)
{
  test_qtdemux_edit_lists_basic_empty_edit_start (TEST_SCHEDULING_PULL, &ibpibp_non_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_non_frag_pull_skipping)
{
  test_qtdemux_edit_lists_skipping (TEST_SCHEDULING_PULL, &ibpibp_non_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_non_frag_pull_skipping_non_rap)
{
  test_qtdemux_edit_lists_skipping_non_rap (TEST_SCHEDULING_PULL, &ibpibp_non_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_non_frag_pull_empty_edit_start_then_clip)
{
  test_qtdemux_edit_lists_empty_edit_start_then_clip (TEST_SCHEDULING_PULL, &ibpibp_non_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_non_frag_pull_empty_edit_middle)
{
  test_qtdemux_edit_lists_empty_edit_middle (TEST_SCHEDULING_PULL, &ibpibp_non_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_non_frag_pull_reorder)
{
  test_qtdemux_edit_lists_reorder (TEST_SCHEDULING_PULL, &ibpibp_non_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_non_frag_pull_repeating)
{
  test_qtdemux_edit_lists_repeating (TEST_SCHEDULING_PULL, &ibpibp_non_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_non_frag_push_no_edts)
{
  test_qtdemux_edit_lists_no_edts (TEST_SCHEDULING_PUSH, &ibpibp_non_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_non_frag_push_basic_zero_dur)
{
  test_qtdemux_edit_lists_basic_zero_dur (TEST_SCHEDULING_PUSH, &ibpibp_non_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_non_frag_push_basic_empty_edit_start)
{
  test_qtdemux_edit_lists_basic_empty_edit_start (TEST_SCHEDULING_PUSH, &ibpibp_non_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_non_frag_push_no_seek_no_edts)
{
  test_qtdemux_edit_lists_no_edts (TEST_SCHEDULING_PUSH_NO_SEEK, &ibpibp_non_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_non_frag_push_no_seek_basic_zero_dur)
{
  test_qtdemux_edit_lists_basic_zero_dur (TEST_SCHEDULING_PUSH_NO_SEEK, &ibpibp_non_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_non_frag_push_no_seek_basic_empty_edit_start)
{
  test_qtdemux_edit_lists_basic_empty_edit_start (TEST_SCHEDULING_PUSH_NO_SEEK, &ibpibp_non_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_pull_no_edts)
{
  test_qtdemux_edit_lists_no_edts (TEST_SCHEDULING_PULL, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_pull_basic)
{
  test_qtdemux_edit_lists_basic (TEST_SCHEDULING_PULL, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_pull_basic_zero_dur)
{
  test_qtdemux_edit_lists_basic_zero_dur (TEST_SCHEDULING_PULL, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_pull_basic_zero_dur_no_mehd)
{
  test_qtdemux_edit_lists_basic_zero_dur_no_mehd (TEST_SCHEDULING_PULL, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_pull_basic_empty_edit_start)
{
  test_qtdemux_edit_lists_basic_empty_edit_start (TEST_SCHEDULING_PULL, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_pull_skipping)
{
  test_qtdemux_edit_lists_skipping (TEST_SCHEDULING_PULL, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_pull_skipping_non_rap)
{
  test_qtdemux_edit_lists_skipping_non_rap (TEST_SCHEDULING_PULL, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_pull_empty_edit_start_then_clip)
{
  test_qtdemux_edit_lists_empty_edit_start_then_clip (TEST_SCHEDULING_PULL, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_pull_empty_edit_middle)
{
  test_qtdemux_edit_lists_empty_edit_middle (TEST_SCHEDULING_PULL, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_pull_reorder)
{
  test_qtdemux_edit_lists_reorder (TEST_SCHEDULING_PULL, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_pull_repeating)
{
  test_qtdemux_edit_lists_repeating (TEST_SCHEDULING_PULL, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_push_no_edts)
{
  test_qtdemux_edit_lists_no_edts (TEST_SCHEDULING_PUSH, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_push_basic_zero_dur)
{
  test_qtdemux_edit_lists_basic_zero_dur (TEST_SCHEDULING_PUSH, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_push_basic_zero_dur_no_mehd)
{
  test_qtdemux_edit_lists_basic_zero_dur_no_mehd (TEST_SCHEDULING_PUSH, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_push_basic_empty_edit_start)
{
  test_qtdemux_edit_lists_basic_empty_edit_start (TEST_SCHEDULING_PUSH, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_push_no_seek_no_edts)
{
  test_qtdemux_edit_lists_no_edts (TEST_SCHEDULING_PUSH_NO_SEEK, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_push_no_seek_basic_zero_dur)
{
  test_qtdemux_edit_lists_basic_zero_dur (TEST_SCHEDULING_PUSH_NO_SEEK, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_push_no_seek_basic_zero_dur_no_mehd)
{
  test_qtdemux_edit_lists_basic_zero_dur_no_mehd (TEST_SCHEDULING_PUSH_NO_SEEK, &ibpibp_frag_template);
}
GST_END_TEST;

GST_START_TEST (test_qtdemux_edit_lists_frag_push_no_seek_basic_empty_edit_start)
{
  test_qtdemux_edit_lists_basic_empty_edit_start (TEST_SCHEDULING_PUSH_NO_SEEK, &ibpibp_frag_template);
}
GST_END_TEST;

/*
 * END AUTO-GENERATED EDIT LIST TEST IMPLEMENTATIONS
 */