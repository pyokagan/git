#include "cache.h"
#include "rebase-common.h"
#include "lockfile.h"
#include "revision.h"
#include "refs.h"
#include "unpack-trees.h"
#include "branch.h"

void refresh_and_write_cache(unsigned int flags)
{
	struct lock_file *lock_file = xcalloc(1, sizeof(struct lock_file));

	hold_locked_index(lock_file, 1);
	refresh_cache(flags);
	if (write_locked_index(&the_index, lock_file, COMMIT_LOCK))
		die(_("unable to write index file"));
}

int cache_has_unstaged_changes(void)
{
	struct rev_info rev_info;
	int result;

	init_revisions(&rev_info, NULL);
	DIFF_OPT_SET(&rev_info.diffopt, IGNORE_SUBMODULES);
	DIFF_OPT_SET(&rev_info.diffopt, QUICK);
	diff_setup_done(&rev_info.diffopt);
	result = run_diff_files(&rev_info, 0);
	return diff_result_code(&rev_info.diffopt, result);
}

int cache_has_uncommitted_changes(void)
{
	struct rev_info rev_info;
	int result;

	if (is_cache_unborn())
		return 0;

	init_revisions(&rev_info, NULL);
	DIFF_OPT_SET(&rev_info.diffopt, IGNORE_SUBMODULES);
	DIFF_OPT_SET(&rev_info.diffopt, QUICK);
	add_head_to_pending(&rev_info);
	diff_setup_done(&rev_info.diffopt);
	result = run_diff_index(&rev_info, 1);
	return diff_result_code(&rev_info.diffopt, result);
}

void rebase_die_on_unclean_worktree(void)
{
	int do_die = 0;

	refresh_and_write_cache(REFRESH_QUIET);

	if (cache_has_unstaged_changes()) {
		error(_("Cannot rebase: You have unstaged changes."));
		do_die = 1;
	}

	if (cache_has_uncommitted_changes()) {
		if (do_die)
			error(_("Additionally, your index contains uncommitted changes."));
		else
			error(_("Cannot rebase: Your index contains uncommitted changes."));
		do_die = 1;
	}

	if (do_die)
		exit(1);
}

static void reset_refs(const struct object_id *oid)
{
	struct object_id *orig, oid_orig;
	struct object_id *old_orig, oid_old_orig;

	if (!get_oid("ORIG_HEAD", &oid_old_orig))
		old_orig = &oid_old_orig;
	if (!get_oid("HEAD", &oid_orig)) {
		orig = &oid_orig;
		update_ref("updating ORIG_HEAD", "ORIG_HEAD",
				orig ? orig->hash : NULL,
				old_orig ? old_orig->hash : NULL,
				0, UPDATE_REFS_MSG_ON_ERR);
	} else if (old_orig)
		delete_ref("ORIG_HEAD", old_orig->hash, 0);
	update_ref("updating HEAD", "HEAD", oid->hash, orig ? orig->hash : NULL, 0, UPDATE_REFS_MSG_ON_ERR);
}

int reset_hard(const struct object_id *commit)
{
	struct tree *tree;
	struct tree_desc desc[1];
	struct unpack_trees_options opts;
	struct lock_file *lock_file;

	tree = parse_tree_indirect(commit->hash);
	if (!tree)
		return error(_("Could not parse object '%s'."), oid_to_hex(commit));

	lock_file = xcalloc(1, sizeof(*lock_file));
	hold_locked_index(lock_file, 1);

	if (refresh_cache(REFRESH_QUIET) < 0) {
		rollback_lock_file(lock_file);
		return -1;
	}

	memset(&opts, 0, sizeof(opts));
	opts.head_idx = 1;
	opts.src_index = &the_index;
	opts.dst_index = &the_index;
	opts.fn = oneway_merge;
	opts.merge = 1;
	opts.update = 1;
	opts.reset = 1;
	init_tree_desc(&desc[0], tree->buffer, tree->size);

	if (unpack_trees(1, desc, &opts) < 0) {
		rollback_lock_file(lock_file);
		return -1;
	}

	if (write_locked_index(&the_index, lock_file, COMMIT_LOCK) < 0)
		die(_("unable to write new index file"));

	reset_refs(commit);
	remove_branch_state();

	return 0;
}
