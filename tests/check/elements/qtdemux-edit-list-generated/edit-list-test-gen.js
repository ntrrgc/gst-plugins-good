#!/usr/bin/env node
/* GStreamer
 *
 * Code generator for edit list handling tests for qtdemux.
 *
 * Copyright (C) <2018> Igalia S.L. <aboya@igalia.com>
 * Copyright (C) <2018> Metrological
 *
 * Generates the following files in this directory:
 *  - implementations.gen.cpp
 *  - calls.gen.cpp
 */

const fs = require("fs");

const frag_modes = [
    "non_frag",
    "frag",
];
const sched_modes = [
    "pull",
    "push",
    "push_no_seek",
];
const edit_list_modes = [
    "no_edts",
    "basic",
    "basic_zero_dur",
    "basic_zero_dur_no_mehd",
    "basic_empty_edit_start",
    "skipping",
    "skipping_non_rap",
    "empty_edit_start_then_clip",
    "empty_edit_middle",
    "reorder",
    "repeating",
];

class Test {
    constructor(frag, sched, edit_list) {
        this.frag = frag;
        this.sched = sched;
        this.edit_list = edit_list;
    }

    is_supported() {
        /* Used to disable combinations that are not supported (by design). */

        if (this.frag != "frag" && this.edit_list == "basic_zero_dur_no_mehd") {
            /* There is no mehd to remove in non-fragmented media. */
            return false;
        }

        if (this.sched === "pull") {
            /* All edit list types supported in pull mode, at least in theory. */
            return true;
        } else {
            /* Push mode is more limited, since it needs to react to whatever it is fed.
             * Only basic edit lists (with an optional empty edit at the start) are supported. */
            return this.edit_list == "no_edts" || this.edit_list.indexOf("basic_") == 0;
        }
    }

    is_currently_broken() {
        /* Used to disable failing tests */

        if (this.sched !== "pull" && this.edit_list == "no_edts") {
            /* in push mode, duration is not computed from the sample table, but from min(mvhd.duration, mdhd.duration),
             * which is unreliable. In consequence, the presentation ends at PTS=2, losing the two last frames. */
            return true;
        } else if (this.frag === "frag" && (this.edit_list == "skipping" || this.edit_list == "skipping_non_rap")) {
            /* after the new second, the frame following the last frame before the edit in decoded order is wrongly emitted. */
            return true;
        } else if (this.frag === "frag" && (this.edit_list == "empty_edit_start_then_clip" || this.edit_list == "empty_edit_middle")) {
            /* empty edits are being ignored in fragmented media. */
            return true;
        } else if (this.sched === "pull" && this.frag === "frag" && (this.edit_list === "reorder" || this.edit_list == "repeating")) {
            /* frame reordering does not work with fragmented media: buffers are emitted in track decoding order from
             * the beginning of the track and GstSegments are emitted when the bounds of an edit are crossed. */
            return true;
        }
        /* not broken (too much) */
        return false;
    }

    test_name() {
        return `test_qtdemux_edit_lists_${this.frag}_${this.sched}_${this.edit_list}`;
    }

    test_implementation() {
        return `GST_START_TEST (${this.test_name()})
{
  test_qtdemux_edit_lists_${this.edit_list} (TEST_SCHEDULING_${this.sched.toUpperCase()}, &ibpibp_${this.frag}_template);
}
GST_END_TEST;`;
    }

    test_call() {
        const fn = this.is_currently_broken() ? "tcase_skip_failing_test" : "tcase_add_test";
        return `${fn} (tc_chain, ${this.test_name()});`
    }
}

let tests = [];
for (let frag of frag_modes) {
    for (let sched of sched_modes) {
        for (let edit_list of edit_list_modes) {
            tests.push(new Test(frag, sched, edit_list));
        }
    }
}
tests = tests.filter(x => x.is_supported());

function gen_implementations() {
    return `/*
 * BEGIN AUTO-GENERATED EDIT LIST TEST IMPLEMENTATIONS
 *
 * Don't edit manually! Edit and run \`edit-list-test-gen.js\` to generate more
 * of these functions.
 */

${tests.map(x => x.test_implementation()).join("\n\n")}

/*
 * END AUTO-GENERATED EDIT LIST TEST IMPLEMENTATIONS
 */`;
}

function gen_calls() {
    return `/*
 * BEGIN AUTO-GENERATED EDIT LIST TEST CALLS
 *
 * Don't edit manually! Edit and run \`edit-list-test-gen.js\` to update the list.
 * Comment out the conditions in \`is_currently_broken()\` there to enable disabled
 * tests.
 */

${tests.map(x => x.test_call()).join("\n\n")}

/*
 * END AUTO-GENERATED EDIT LIST TEST CALLS
 */`;
}

fs.writeFileSync(__dirname + "/implementations.gen.cpp", gen_implementations());
fs.writeFileSync(__dirname + "/calls.gen.cpp", gen_calls());

const enabledTests = tests.filter(x => !x.is_currently_broken());
console.log(`Enabled ${enabledTests.length}/${tests.length} tests:\n${enabledTests.map(x=>x.test_name()).join("\n")}`);